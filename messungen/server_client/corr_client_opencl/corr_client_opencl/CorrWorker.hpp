//
//  CorrWorker.hpp
//  corr_client_opencl
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef CorrWorker_hpp
#define CorrWorker_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <stdint.h>
#include <vector>
#include "GPUDevice.hpp"
#include <iostream>
#include <thread>


class CorrWorker {
    int _serverFd;
    std::string _tracesIndir;
    std::vector<GPUDevice*> _gpus;
    std::string _kernelsource;
    size_t _availableRamSize;
    
    //recieved
    uint32_t _point_per_trace;
    std::vector<std::string> _powermodels;

    std::thread *_workerThread;
    std::atomic<bool> _workerShouldStop;
    std::atomic<bool> _workerIsRunning;

    
    void worker(uint16_t maxLoaderThreads = 0);
    
public:
    CorrWorker(std::string tracesIndir, const char *serverAddress, uint16_t port, std::vector<int> gpus, std::string kernelsource, size_t availableRamSize, float workermultiplier = 1);
    ~CorrWorker();
    
    
    void startWorker(uint16_t maxLoaderThreads = 0);
    void stopWorker();

};

#endif /* CorrWorker_hpp */
