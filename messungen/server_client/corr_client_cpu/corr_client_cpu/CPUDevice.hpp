//
//  GPUDevice.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef CPUDevice_hpp
#define CPUDevice_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <iostream>
#include <atomic>
#include "LoadFile.hpp"
#include "CPUModelResults.hpp"
#include <map>
#include <mutex>
#include <thread>
#include <queue>

class CPUDevice {
    uint32_t _point_per_trace;
    uint16_t _threadsCnt;
    
    LoadFile *_wipTrace;
    std::mutex _wipTraceLock;
    std::mutex _wipTraceEventLock;
    std::atomic<uint16_t> _wipTraceUsageCnt;


    std::queue<std::thread *> _workers;
    std::atomic<bool> _workerShouldStop;
    std::atomic<uint16_t> _workerIsRunningCnt;

    std::atomic<uint32_t> _workerSyncCounter;
    std::mutex _workerSyncLock;

    
    std::map<std::string,std::map<std::string,CPUModelResults*>> _gmodelresults;
    std::map<std::string,std::map<std::string,CPUModelResults*>> _curmodelresults;

    void worker(int workernum);
    void resetCur();

    
public:
    CPUDevice(uint32_t point_per_trace, std::vector<std::string> models, uint16_t threadsCnt);
    ~CPUDevice();

    void resetDevice();

    void transferTrace(LoadFile *trace);
    void getResults(std::map<std::string,std::map<std::string,CPUModelResults*>> &modelresults);
};

#endif /* CPUDevice_hpp */
