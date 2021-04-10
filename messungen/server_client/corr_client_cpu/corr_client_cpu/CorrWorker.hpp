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
#include "CPUDevice.hpp"
#include <iostream>
#include <thread>


class CorrWorker {
    int _serverFd;
    std::string _tracesIndir;
    size_t _availableRamSize;
    uint16_t _threadCnt;
    CPUDevice *_cpudevice;
    
    //recieved
    uint32_t _point_per_trace;
    std::vector<std::string> _powermodels;

    std::thread *_workerThread;
    std::atomic<bool> _workerShouldStop;
    std::atomic<bool> _workerIsRunning;

    
    void worker();
    
public:
    CorrWorker(std::string tracesIndir, const char *serverAddress, uint16_t port, size_t availableRamSize, uint16_t threadCnt = 0);
    ~CorrWorker();
    
    
    void startWorker();
    void stopWorker();

};

#endif /* CorrWorker_hpp */
