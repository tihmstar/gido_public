//
//  CorrWorker.cpp
//  corr_client_opencl
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "CorrWorker.hpp"
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
#include "CPULoader.hpp"
#include <stdlib.h>


//#warning custom point_per_trace
//#define NUMBER_OF_POINTS_OVERWRITE 2000


#define assure(cond) do {if (!(cond)) {printf("ERROR: CorrWorker ASSURE FAILED ON LINE=%d\n",__LINE__); throw(__LINE__); }}while (0)
#define assure_(cond) do {cl_int clret = 0; if ((clret = cond)) {printf("ERROR: CorrWorker ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); throw(__LINE__);}}while (0)

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

#define senduint32_t(num) do { uint32_t s = htonl(num);assure(write(_serverFd, &s, sizeof(s)) == sizeof(s)); }while(0)
#define receiveuint32_t() [this]()->uint32_t{ uint32_t s = 0; assure(doread(_serverFd, &s, sizeof(s)) == sizeof(s)); s = htonl(s); return s;}()

#define senduint64_t(num) do { uint64_t s = ntohll(num);assure(write(_serverFd, &s, sizeof(s)) == sizeof(s)); }while(0)
#define receiveuint64_t() [this]()->uint64_t{ uint64_t s = 0; assure(doread(_serverFd, &s, sizeof(s)) == sizeof(s)); s = ntohll(s); return s;}()

using namespace std;


static ssize_t doread(int fd, void *buf, ssize_t size){
    ssize_t didread = 0;
    do{
        ssize_t curdidread = read(fd, static_cast<uint8_t*>(buf)+didread, size-didread);
        assure(curdidread>0);
        didread += curdidread;
    }while (didread<size);
    return didread;
}

CorrWorker::CorrWorker(std::string tracesIndir, const char *serverAddress, uint16_t port, std::vector<int> gpus, std::string kernelsource, size_t availableRamSize, float workermultiplier)
: _serverFd(-1), _tracesIndir(tracesIndir), _kernelsource(kernelsource), _availableRamSize(availableRamSize), _point_per_trace(0), _workerThread(NULL),
    _workerShouldStop(false), _workerIsRunning(false)
{
    struct sockaddr_in servaddr = {};
    uint32_t num_of_powermodels = 0;
    
    if (_tracesIndir.c_str()[_tracesIndir.size()-1] != '/') {
        _tracesIndir += '/';
    }
    
    printf("[CorrWorker] init\n");
    assure((_serverFd = socket(AF_INET, SOCK_STREAM, 0)) != -1);
    
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serverAddress);
    servaddr.sin_port = htons(port);
    
    printf("[CorrWorker] connecting to server=%s:%u ...\n",serverAddress,port);
    assure(!connect(_serverFd, (struct sockaddr *)&servaddr, sizeof(servaddr)));
    printf("[CorrWorker] connected!\n");

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
     
     ---  Cient -> Server ---
     send num_of_gpus //uint32_t

     */
    
    _point_per_trace = receiveuint32_t();
    num_of_powermodels = receiveuint32_t();
    
#ifdef NUMBER_OF_POINTS_OVERWRITE
    assure(_point_per_trace == NUMBER_OF_POINTS_OVERWRITE);
#endif


    for (int i=0; i<num_of_powermodels; i++) {
        char c = 0;
        std::string curstr;
        do{
            read(_serverFd, &c, 1);
            curstr += c;
        }while(c);
        _powermodels.push_back(curstr.substr(0,curstr.size()-1));
    }


    for (auto &m : _powermodels) {
        printf("[CorrWorker] Model=%s\n",m.c_str());
    }
    
    printf("[CorrWorker] Got %3lu powerModels!\n",_powermodels.size());
    
    
#pragma mark init GPUS
    
   // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_uint num_platforms = 0;
    cl_uint num_devices = 0;

    
    assure_(clGetPlatformIDs(1, &platform_id, &num_platforms));

    assure_(clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices));
    
    {
        cl_device_id device_ids[num_devices];
        assure_(clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, num_devices, device_ids, &num_devices));

        for (int index : gpus) {
            printf("[CorrWorker] Init GPU %d!\n",index);
            assure(index < num_devices);
            _gpus.push_back(new GPUDevice(index,device_ids[index],kernelsource, _point_per_trace, _powermodels));
        }
    }

    printf("[CorrWorker] All GPUs initialized\n");
    
    uint32_t workers = (uint32_t)gpus.size();

    workers *= workermultiplier;

    assure(workers != 0);
    
    printf("[CorrWorker] workers=%u\n",workers);
    senduint32_t(workers);
}

CorrWorker::~CorrWorker(){
    _workerShouldStop = true;
    {
        int fd = _serverFd;_serverFd = -1;
        close(fd);
    }
    if (_workerThread) _workerThread->join();
    if (!_workerIsRunning) raise(SIGINT); //sanity check
    if (_workerThread) {
        delete _workerThread;
        _workerThread = NULL;
    }
    
}

void CorrWorker::worker(uint16_t maxLoaderThreads){
    _workerIsRunning = true;
    while (!_workerShouldStop) {
        uint32_t batchSize = 0;
        
        batchSize = receiveuint32_t();
        printf("[CorrWorker] Got batchsize=%u\n",batchSize);

        std::vector<std::string> bachFilenames;
        
        
        for (int i=0; i<batchSize; i++) {
            char c = 0;
            std::string curstr;
            do{
                read(_serverFd, &c, 1);
                curstr += c;
            }while(c);
            bachFilenames.push_back(_tracesIndir + curstr.substr(0,curstr.size()-1));
        }
        
        for (auto &f : bachFilenames) {
            printf("[CorrWorker] Batch Filename=%s\n",f.c_str());
        }
        
        senduint32_t(batchSize);

#pragma mark doGPUComputation

        CPULoader cloader(bachFilenames, _availableRamSize, maxLoaderThreads);
        std::queue<std::thread *> gpuTransferThreads;
            
            
#define MIN_TRANSFER_CNT 0
#define MAX_TRANSFER_CNT 25000
        
        for (auto &gpu : _gpus) {
            gpuTransferThreads.push(new std::thread([&cloader](GPUDevice *gpu)->void{
                LoadFile *toTransfer = NULL;
                
                while (LoadFile *file = cloader.dequeue()) {
                    if (toTransfer) { //if we have "toTransfer"
                        
                        if (
                            toTransfer->tracesInFile() >= MIN_TRANSFER_CNT //if "toTransfer" already large enough
                            || toTransfer->tracesInFile() + file->tracesInFile() > MAX_TRANSFER_CNT //or combination would exceed MAX limit
                            ) {
                            //then transfer the pendin trace
                            gpu->transferTrace(toTransfer);
                            delete toTransfer;
                            toTransfer = NULL;
                        }
                    }
                    
                    if (!toTransfer) { //if we don't have a pending trace anymore, make "file" be our new pending trace
                        toTransfer = file; file = NULL;
                        continue;
                    }
                    
                    //if we have both: "transferTrace" and "file", then we should combine them now
                    printf("[DW-%u] combining traces (%5u + %5u = %5u)\n",gpu->deviceNum(),toTransfer->tracesInFile(), file->tracesInFile(), toTransfer->tracesInFile() + file->tracesInFile());
                    *toTransfer += *file;
                    
                    //once we combined traces, we can discard "file"
                    if (file) {
                        delete file; file = NULL;
                    }
                }
                
                //final transfer
                if (toTransfer) {
                    gpu->transferTrace(toTransfer);
                    delete toTransfer;
                    toTransfer = NULL;
                }
                
                printf("GPU %d done transfer!\n",gpu->deviceNum());
            },gpu));
        }
            
        printf("Waiting for gpu transfers to finish (%lu threads)!\n",gpuTransferThreads.size());
        while (gpuTransferThreads.size()) {
            auto t = gpuTransferThreads.front();
            gpuTransferThreads.pop();
            t->join();
            delete t;
        }

        printf("All GPU transfers finished, getting results...\n");
        std::map<std::string,std::map<std::string,CPUModelResults*>> allmodelresults;
        
        for (auto &gpu : _gpus) {
            gpu->getResults(allmodelresults);
        }
        
#pragma mark sendBackResults
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
        srand((unsigned int)time(NULL) ^ rand());

        
        uint32_t myrand = rand();
        uint32_t remoteRand = 0;
        senduint32_t(myrand);
        remoteRand = receiveuint32_t();
        
        senduint32_t(allmodelresults.size());
        
        int allmodelresultsCounter = 0;
        for (auto &category : allmodelresults) {
            assure(allmodelresultsCounter++ < allmodelresults.size()); //don't send more than we announced
            
            assure(write(_serverFd, category.first.c_str(), category.first.size()+1) == category.first.size()+1); //modelresultcategory\x00 //terminated by zero byte
            
            senduint32_t(category.second.size()); //send allmodelresults[x].size() //uint32_t
            
            int modelresultcategorycounter = 0;
            for (auto &mr : category.second) {
                assure(modelresultcategorycounter++ < category.second.size()); //don't send more than we announced
                
                
                assure(write(_serverFd, mr.first.c_str(), mr.first.size()+1) == mr.first.size()+1); // modelresultname\x00 //terminated by zero byte

                void *data = NULL;
                size_t dataSize = 0;

                mr.second->serialize(&data, &dataSize);
                
                uint64_t dataSizeSend = (uint64_t)dataSize;
                assure(dataSizeSend == dataSize); //no casting issues here
                
                senduint64_t(dataSizeSend); //cpumodelresultsize //uint64_t

                assure(write(_serverFd, data, dataSizeSend) == dataSizeSend); // < raw data >
                
                free(data);
            }
        }
        senduint32_t(myrand ^ remoteRand); //final check
        
        
        for (auto &gpu : _gpus) {
            gpu->resetDevice();
        }
        
        for (auto & category : allmodelresults) {
            for (auto &model: category.second) {
                delete model.second; model.second = NULL;
            }
        }
        
    }
    _workerIsRunning = false;
}

void CorrWorker::startWorker(uint16_t maxLoaderThreads){
    _workerThread = new std::thread([this](uint16_t loadthreads){
        worker(loadthreads);
    }, maxLoaderThreads);
}

void CorrWorker::stopWorker(){
    _workerShouldStop = true;
}
