//
//  CorrClient.cpp
//  corr_server
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "CorrClient.hpp"
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "CorrServer.hpp"
#include <netinet/in.h>
#include <map>
#include <iostream>
#include "CPUModelResults.hpp"
#include <stdlib.h>
#include <sys/socket.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: CorrClient ASSURE FAILED ON LINE=%d\n",__LINE__); throw(__LINE__);}}while (0)

#define senduint32_t(num) do { uint32_t s = htonl(num);assure(write(_clientFd, &s, sizeof(s)) == sizeof(s)); }while(0)
#define receiveuint32_t() [this]()->uint32_t{ uint32_t s = 0; assure(doread(_clientFd, &s, sizeof(s)) == sizeof(s)); s = htonl(s); return s;}()

#define senduint64_t(num) do { uint64_t s = ntohll(num);assure(write(_clientFd, &s, sizeof(s)) == sizeof(s)); }while(0)
#define receiveuint64_t() [this]()->uint64_t{ uint64_t s = 0; assure(doread(_clientFd, &s, sizeof(s)) == sizeof(s)); s = ntohll(s); return s;}()


#define receivestring() [this]()->std::string{\
    std::string rt;\
    char c = 0;\
    while (true){\
        assure(recv(_clientFd, &c, 1, NULL) > 0);\
        if (!c) break;\
        rt += c;\
    }\
    return rt;\
}()


#define BATCHSIZE_PER_WORKER 5

#ifdef DEBUG
#   undef BATCHSIZE_PER_WORKER 
#   define BATCHSIZE_PER_WORKER 5
#endif


#define BATCHSIZE_LIMIT 1000

#ifndef ntohll
uint64_t ntohll(uint64_t in){
    volatile uint64_t out[1] = {1};
    if ((*(uint8_t*)out) == 1){
        //little endian, do swap
        
        ((uint8_t*)out)[7] = (in >> 0x00) & 0xff;
        ((uint8_t*)out)[6] = (in >> 0x08) & 0xff;
        ((uint8_t*)out)[5] = (in >> 0x10) & 0xff;
        ((uint8_t*)out)[4] = (in >> 0x18) & 0xff;
        ((uint8_t*)out)[3] = (in >> 0x20) & 0xff;
        ((uint8_t*)out)[2] = (in >> 0x28) & 0xff;
        ((uint8_t*)out)[1] = (in >> 0x30) & 0xff;
        ((uint8_t*)out)[0] = (in >> 0x38) & 0xff;

    }else{
        out[0] = in;
    }
    return out[0];
}
#endif

static ssize_t doread(int fd, void *buf, ssize_t size){
    ssize_t didread = 0;
    do{
        if (size-didread == 0) break;
        ssize_t curdidread = recv(fd, static_cast<uint8_t*>(buf)+didread, size-didread, NULL);
        assure(curdidread>0);
        didread += curdidread;
    }while (didread<size);
    return didread;
}

CorrClient::CorrClient(CorrServer *parent, int clientFd, std::string ipaddr)
:   _parent(parent), _clientFd(clientFd), _workerShouldStop(false), _workerIsRunning(false), _processed(0), _currentBatchStatus(BatchStatusUninitialized), _ipaddress(ipaddr)
{
    uint32_t powerModelSize = (uint32_t)_parent->_powermodels.size();

    /*
     init sequence:
     
     ---  Server -> Cient ---
     send point_per_trace //uint32_t
     send num_of_powermodels //uint32_t
     send power models seperated by \x00 byte
     [
        model\x00
        model\x00 ...
     ]
     
     ---  Client -> Server ---
     send num_of_workers //uint32_t

     */
    
    senduint32_t(_parent->_point_per_trace);
    senduint32_t(powerModelSize);

    for (int i=0; i<powerModelSize; i++) {
        std::string &pm = _parent->_powermodels.at(i);
        size_t sendlen = pm.size()+1; //send with nullbyte
        assure(write(_clientFd, pm.c_str(), sendlen) == sendlen);
    }
    
    _num_of_workers = receiveuint32_t();
    
    if (_num_of_workers == 0) {
        _num_of_workers = 1; //less than one worker doesn't make sense and breaks stuff
    }
    
    
    _workerThread = new std::thread([this]{
        _workerIsRunning = true;
        try {
            worker();
        } catch (...) {
            printf("[CorrClient] (%d) client died!\n",_clientFd);
        }
        
        //if a disconnect happens and the worker dies, this client becomes useless
        std::thread delme([this]{
            delete this;
        });
        delme.detach();
        
        _workerIsRunning = false;
    });
}

CorrClient::~CorrClient(){
    kill();
    
    _workerThread->join();
    if (_workerIsRunning) raise(SIGINT); //sanity check
    delete _workerThread; _workerThread = NULL;
    
    if (_currentBatch.size()) {
        //we claimed a batch, but we never finished it. give it back!
        
        printf("[CorrClient] (%d) giving back unfinished batch\n",_clientFd);
        _parent->giveBackUnfinishedBatch(_currentBatch);
        _currentBatch.clear();
        _currentBatchStatus = BatchStatusUninitialized;
    }

    
    //remove from parent list
    _parent->_clientWorkersSetLock.lock();usleep(10);
    _parent->_clientWorkers.erase(this);

    //not a file event, but a client-got-removed event
    //make it re-check anyways
    _parent->_filesQueueUpdateEventLock.unlock();usleep(10);

    _parent->_clientWorkersSetLock.unlock();usleep(10); //very last thing we do here
}

void CorrClient::worker(){
    
    uint32_t suggestedBatchSize = BATCHSIZE_PER_WORKER * _num_of_workers;
    if (suggestedBatchSize > BATCHSIZE_LIMIT) {
        suggestedBatchSize = BATCHSIZE_LIMIT;
    }
    
    assure(suggestedBatchSize <= BATCHSIZE_LIMIT);
    
    
    /*
    Protocol:
    
    ---  Server -> Cient ---
    send batchSize //uint32_t
    send power models seperated by \x00 byte
    [
       tracename\x00
       tracename\x00 ...
    ]
    
    ---  Client -> Server ---
    send batchSize //uint32_t //used as ACK

    */
    
    while (!_workerShouldStop) {
        uint32_t batchSize = 0;
        
        _currentBatchStatus = BatchStatusWaitingForWork;
        printf("[CorrClient-%02d] BatchStatusWaitingForWork\n",_clientFd);

        _currentBatch = _parent->dequeueBatch(suggestedBatchSize);
        batchSize = (uint32_t)_currentBatch.size(); //real batch size is possibly smaller than requested
        
        if (!batchSize) {
            if (_parent->remainingFilesCnt() == 0) {
                printf("[CorrClient-%02d] Server has no more work for me, retireing!\n",_clientFd);
                _parent->notifyClientsWorkAvailable();
                this->kill();
            }
            printf("[CorrClient-%02d] didn't get a batch, waiting for work....\n",_clientFd);
            _workAvailablilityUpdateEventLock.lock();usleep(10);
            continue;
        }
        _parent->notifyClientsWorkAvailable(); //if we can get work, maybe there is enough work for others too
        
        
        _currentBatchStatus = BatchStatusQueued;
        printf("[CorrClient-%02d] BatchStatusQueued\n",_clientFd);
        senduint32_t(batchSize);
        
        _currentBatchStatus = BatchStatusSending;
        printf("[CorrClient-%02d] BatchStatusSending\n",_clientFd);

        {
            uint32_t i = 0;
            for (auto &bf : _currentBatch) {
                assure(i++ < batchSize);
                size_t sendlen = bf.size()+1; //send with nullbyte
                assure(write(_clientFd, bf.c_str(), sendlen) == sendlen);
            }
        }
        

        {
            uint32_t ack = receiveuint32_t();
            if (ack != batchSize) {
                _currentBatchStatus = BatchStatusError;
                assure(false);
            }
        }
        
        _currentBatchStatus = BatchStatusWorking;
        printf("[CorrClient-%02d] BatchStatusWorking\n",_clientFd);

        
        /*
        Protocol:
        
        ---  Client -> Server ---
        send allmodelresults.size() //uint32_t
        send models
        [
           modelresultcategory\x00 //terminated by zero byte
           [
            send allmodelresults[x].size() //uint32_t
            modelresultname\x00 //terminated by zero byte
            cpumodelresultsize //uint64_t
            < raw data >
           ]
         ....
        ]
        send 00000000 //uint32_t
         

        */
        
        std::map<std::string,std::map<std::string,CPUModelResults*>> allmodelresults;

        srand((unsigned int)time(NULL) ^ rand());
        
        uint32_t myrand = rand();
        uint32_t clientRand = 0;
        clientRand = receiveuint32_t();
        senduint32_t(myrand);

        
        uint32_t allmodelresults_size = receiveuint32_t();
        
        _currentBatchStatus = BatchStatusReceiving;
        printf("[CorrClient-%02d] BatchStatusReceiving\n",_clientFd);


        for (int allmodelresults_cnt = 0; allmodelresults_cnt<allmodelresults_size; allmodelresults_cnt++) {
            std::string modelresultcategory = receivestring();
            
            uint32_t allmodelresults_x_size = receiveuint32_t();
            
            for (int allmodelresults_x_cnt = 0; allmodelresults_x_cnt<allmodelresults_x_size; allmodelresults_x_cnt++) {
                std::string modelresultname = receivestring();
                
                uint8_t *data = NULL;
                uint64_t dataSize = 0;
                uint64_t dataDidReceive = 0;

                dataSize = receiveuint64_t();
                data = (uint8_t *)malloc(dataSize);

                dataDidReceive = doread(_clientFd, data, dataSize);
                
                assure(dataSize == dataDidReceive);
                
                allmodelresults[modelresultcategory][modelresultname] = new CPUModelResults(data, dataDidReceive);
                
                free(data);
            }
        }
        
        uint32_t checkrands = receiveuint32_t();
        assure(checkrands == (myrand ^ clientRand));

        
        _currentBatchStatus = BatchStatusCombining;
        printf("[CorrClient-%02d] BatchStatusCombining\n",_clientFd);

        _parent->combineAllModelResults(allmodelresults, _currentBatch);

        //free memory
        for (auto &category : allmodelresults) {
            for (auto &model : category.second) {
                delete model.second; model.second = NULL;
            }
        }
        
        _processed += _currentBatch.size();
        
        _currentBatchStatus = BatchStatusDone;
        printf("[CorrClient-%02d] BatchStatusDone\n",_clientFd);

        _currentBatch.clear();
    }
}

void CorrClient::kill(){
    _workerShouldStop = true;
    {
        //kill connection
        int cfd = _clientFd; _clientFd = -1;
        if (cfd > 0) {
            shutdown(cfd, SHUT_RDWR);
            close(cfd);
        }
    }
    _workAvailablilityUpdateEventLock.unlock();usleep(10); //make sure worker doesn't hang
}

void CorrClient::printStats(){
    printf("[CorrClient-%02d] (%s) [processed files=%u] (queued files=%lu)\n",_clientFd,_ipaddress.c_str(),_processed,_currentBatch.size());
}
