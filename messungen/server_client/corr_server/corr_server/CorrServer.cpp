//
//  CorrServer.cpp
//  corr_server
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "CorrServer.hpp"
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "numerics.hpp"
#include "myfile.hpp"
#include <math.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: CorrServer ASSURE FAILED ON LINE=%d\n",__LINE__); throw(__LINE__);}}while (0)

using namespace std;

CorrServer::CorrServer(int port, uint32_t point_per_trace, std::vector<std::string> powermodels, std::queue<std::string> fileQueue)
:   _port(port), _serverfd(-1), _acceptWorkerThread(NULL), _acceptWorkerShouldStop(false), _acceptWorkerIsRunning(false),
    _clientWorkers{}, _point_per_trace(point_per_trace), _powermodels(powermodels), _fileQueue(fileQueue)
{
    struct sockaddr_in servaddr = {};
    
    assure((_serverfd = socket(AF_INET, SOCK_STREAM, 0)) != -1);
    
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(_port);

    if (::bind(_serverfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
        printf("[CorrServer] failed to bind port %u\n",_port);
        assure(0);
    }
    
    assure(!listen(_serverfd, 5)); //5 backlog connections
    printf("[CorrServer] now listening on port %u\n",_port);

    
    _acceptWorkerThread = new std::thread([this]{
        acceptWorker();
    });
    
}

CorrServer::~CorrServer(){
    _acceptWorkerShouldStop = true;
    
    {
        //stop accepting new clients
        int cfd = _serverfd; _serverfd = -1;
        close(cfd);
    }

    //self connect to return from final accept syscall
    {
        struct sockaddr_in selfdst = {};
        int selfclient = -1;

        //ignore create socket fail
        if ((selfclient = socket(AF_INET, SOCK_STREAM, 0)) != -1) {
            // assign IP, PORT
            selfdst.sin_family = AF_INET;
            selfdst.sin_addr.s_addr = inet_addr("127.0.0.1");
            selfdst.sin_port = htons(_port);

            //ignore connect fail
            connect(selfclient, (struct sockaddr *)&selfdst, sizeof(selfdst));

            close(selfclient);
        }
    }
    
    
    for (auto &c : _clientWorkers) {
        c->kill(); //kill all existing clients
    }
    
    _acceptWorkerThread->join();
    if (_acceptWorkerIsRunning) raise(SIGINT); //sanity check
    delete _acceptWorkerThread;
    
    
    //wait for clients to die
    _clientWorkersSetLock.lock();usleep(10);
    while (_clientWorkers.size()) {
        _clientWorkersSetLock.unlock();usleep(10);
        sleep(1);
        _clientWorkersSetLock.lock();usleep(10);
    }
    
    for (auto &category : _allmodelresults){
        for (auto &modelguess : category.second) {
            delete modelguess.second; modelguess.second = NULL;
        }
    }
    
    //leave with lock held
}

void CorrServer::acceptWorker(){
    _acceptWorkerIsRunning = true;

    std::atomic<uint16_t> wipClients{0};
    
    while (!_acceptWorkerShouldStop) {
        struct sockaddr_in clientaddr = {};
        socklen_t socklen = sizeof(clientaddr);
        int clientFd = 0;
        
        if ((clientFd = accept(_serverfd, (struct sockaddr *)&clientaddr, &socklen)) != -1) {
            char ip_addr[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &clientaddr.sin_addr, ip_addr, INET_ADDRSTRLEN);
            printf("[CorrServer] new client at: %s\n",ip_addr);
            
            ++wipClients;
            std::thread bg([this,clientFd, &wipClients](std::string ipaddr){
                CorrClient *c = NULL;
                try {
                    c = new CorrClient(this, clientFd, ipaddr);
                    _clientWorkersSetLock.lock();usleep(10);
                    _clientWorkers.insert(c);
                    _clientWorkersSetLock.unlock();usleep(10);
                } catch (...) {
                    printf("[CorrServer] failed to accept client on fd=%d\n",clientFd);
                    close(clientFd);
                }
                --wipClients;
            },ip_addr);
            bg.detach();
        }
        
    }
    while ((uint16_t)wipClients) {
        sleep(1);//wait for all wipclients to finish
    }
    _acceptWorkerIsRunning = false;
}

std::set<std::string> CorrServer::dequeueBatch(uint32_t size){
    std::set<std::string> ret;
        
    _fileQueueLock.lock();usleep(10);
    
    while (size-- && _fileQueue.size()) {
        std::string e = _fileQueue.front();
        _fileQueue.pop();
        ret.insert(e);
    }
    
    _fileQueueLock.unlock();usleep(10);

    
    _workInProgressBatchesLock.lock();usleep(10);
    _workInProgressBatches.insert(ret);
    _workInProgressBatchesLock.unlock();usleep(10);

    _filesQueueUpdateEventLock.unlock();usleep(10);
    return ret;
}

void CorrServer::giveBackUnfinishedBatch(std::set<std::string> batch){
    _workInProgressBatchesLock.lock();usleep(10);

    if (_workInProgressBatches.find(batch) == _workInProgressBatches.end()){
        raise(SIGABRT);
    }
    
    _workInProgressBatches.erase(batch);
    _workInProgressBatchesLock.unlock();usleep(10);

    
    _fileQueueLock.lock();usleep(10);
    for (auto e : batch) {
        _fileQueue.push(e);
    }
    _fileQueueLock.unlock();usleep(10);
    
    
    //notify clients who are possibly waiting that there is again work to do
    notifyClientsWorkAvailable();
    _filesQueueUpdateEventLock.unlock();usleep(10);
}

void CorrServer::combineAllModelResults(std::map<std::string,std::map<std::string,CPUModelResults*>> allmodelresults, std::set<std::string> batch){
    _allmodelresultsLock.lock();usleep(10);
    printf("[CorrServer] combineAllModelResults ...\n");
    for (auto &category : allmodelresults) {
        for (auto &model : category.second) {
            CPUModelResults *newResults = allmodelresults[category.first][model.first];
            CPUModelResults *globalResults = _allmodelresults[category.first][model.first];

            if (!globalResults) {
                _allmodelresults[category.first][model.first] = new CPUModelResults(*newResults);
            }else{
                //combine shit
                uint32_t point_per_trace = 0;
                assure((point_per_trace = globalResults->point_per_trace()) == newResults->point_per_trace());
                
                combineMeans(globalResults->_meanTrace, newResults->_meanTrace, point_per_trace);
                combineMeans(globalResults->_meanHypot, newResults->_meanHypot, 1);

                combineCenteredSum(globalResults->_censumTrace, globalResults->_meanTrace, newResults->_censumTrace, newResults->_meanTrace, point_per_trace);
                combineCenteredSum(globalResults->_censumHypot, globalResults->_meanHypot, newResults->_censumHypot, newResults->_meanHypot, 1);

                for (int i=0; i<point_per_trace; i++) {
                    globalResults->_upperPart.trace[i] += newResults->_upperPart.trace[i];
                }
            }
        }
    }
    printf("[CorrServer] combineAllModelResults done!\n");
    _allmodelresultsLock.unlock();usleep(10);
    
    _workInProgressBatchesLock.lock();usleep(10);
    if (_workInProgressBatches.find(batch) == _workInProgressBatches.end()){
        raise(SIGABRT);
    }
    _workInProgressBatches.erase(batch);
    _workInProgressBatchesLock.unlock();usleep(10);
    _filesQueueUpdateEventLock.unlock();usleep(10);
}



size_t CorrServer::queuedFilesCnt(){
    size_t ret = 0;
    _workInProgressBatchesLock.lock();usleep(10);
    for (auto &b : _workInProgressBatches) {
        ret += b.size();
    }
    _workInProgressBatchesLock.unlock();usleep(10);
    return ret;
}

size_t CorrServer::unqueuedFilesCnt(){
    return _fileQueue.size();
}

size_t CorrServer::remainingFilesCnt(){ //queuedFilesCnt + unqueuedFilesCnt
    size_t ret = 0;
    _workInProgressBatchesLock.lock();usleep(10);
    for (auto &b : _workInProgressBatches) {
        ret += b.size();
    }
    
    ret += _fileQueue.size();
    
    _workInProgressBatchesLock.unlock();usleep(10);
    return ret;
}

uint32_t CorrServer::clientsCnt(){
    return (uint32_t)_clientWorkers.size();
}


void CorrServer::hangUntilFilesUpdate(){
    _filesQueueUpdateEventLock.lock();usleep(10);
}

void CorrServer::notifyMannualFilesUpdate(){
    _filesQueueUpdateEventLock.unlock();usleep(10);
}

void CorrServer::notifyClientsWorkAvailable(){
    _clientWorkersSetLock.lock();usleep(10);
    for (auto &client : _clientWorkers) {
        client->_workAvailablilityUpdateEventLock.unlock();usleep(10);
    }
    _clientWorkersSetLock.unlock();usleep(10);
}


void CorrServer::saveCorrToDir(std::string outdir){
    _allmodelresultsLock.lock();usleep(10);
    
    atomic<int> doneThreads{0};
    std::mutex doneThreadsEventLock;
    
    if (outdir.back() != '/') {
        outdir += '/';
    }
    
    atomic<uint64_t> numTraces{0};
    
    for (auto &modelresults : _allmodelresults) {
        std::thread k([&modelresults,outdir,&doneThreads,&doneThreadsEventLock, &numTraces](uint32_t point_per_trace){
            auto trace_vals = modelresults.second.at("trace");

            double *censumTrace = trace_vals->censumTrace().trace;
            
            numTraces = trace_vals->meanTrace().cnt;//gets overwritten multiple times with the same value (hopefully)
            
            int doneKeyguesses = 0;
            int totalKeyguesses = (int)modelresults.second.size()-1;
            for (auto &model : modelresults.second) {
                if (!isdigit(model.first.c_str()[0])) {
                    printf("skipping model=%s\n",model.first.c_str());
                    continue;
                }

                std::string outfile = outdir;
                
                int32_t guess = atoi(model.first.c_str());
                
                outfile += "CORR-";
                outfile += modelresults.first;
                outfile += "-KEYGUESS_";
                outfile += std::to_string(guess);

                printf("computing CORR - %s - Keyguess=0x%04x (0x%08x of 0x%08x)\n",modelresults.first.c_str(),guess,doneKeyguesses,totalKeyguesses);

                myfile out(outfile.c_str(),point_per_trace*sizeof(double));
                memset(&out[0], 0, point_per_trace*sizeof(double));
                
                double *upTrace = model.second->upperPart().trace;
                double hypotCensum = model.second->censumHypot().trace[0];
                
                
                for (int i=0; i<point_per_trace; i++) {
                    double upper = upTrace[i];
                    
                    double realCensum = censumTrace[i];
                    
                    double lower = sqrt(realCensum) * sqrt(hypotCensum);
                                        
                    out[i] = upper/lower;
                }
                doneKeyguesses++;
            }
            
            ++doneThreads;
            doneThreadsEventLock.unlock();usleep(10);
        },_point_per_trace);
        k.detach();
    }
    
    printf("waiting for corr threads to finish...\n");
    while (doneThreads < _allmodelresults.size()) {
        printf("waiting for corr threads to finish. %d of %lu done!\n",(int)doneThreads,_allmodelresults.size());
        doneThreadsEventLock.lock();usleep(10);
    }
    
    {//wait until at least one thread set numTraces correctly
        std::string numTracesFileName = outdir + "numTraces.txt";
        if(FILE *f = fopen(numTracesFileName.c_str(), "w")){
            fprintf(f, "%llu\n",(uint64_t)numTraces);
            fclose(f);
        }
    }

    
    
    printf("Done saving!\n");

    _allmodelresultsLock.unlock();usleep(10);
}

void CorrServer::printClientStats(){
    _clientWorkersSetLock.lock();usleep(10);
    for (auto &client : _clientWorkers) {
        client->printStats();
    }
    _clientWorkersSetLock.unlock();usleep(10);
}

void CorrServer::killClient(int clientfd){
    bool didFind = false;
    _clientWorkersSetLock.lock();usleep(10);
    for (auto &client : _clientWorkers) {
        if (client->_clientFd == clientfd) {
            printf("Found client (%d) ! killing!\n",clientfd);
            client->kill();
            didFind = true;
        }
    }
    _clientWorkersSetLock.unlock();usleep(10);
    if (!didFind) {
        printf("Failed to find client (%d)\n",clientfd);
    }
}
