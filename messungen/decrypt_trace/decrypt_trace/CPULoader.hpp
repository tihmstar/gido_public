//
//  CPULoader.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef CPULoader_hpp
#define CPULoader_hpp

#include <vector>
#include <iostream>
#include <thread>
#include <queue>
#include "LoadTraceFile.hpp"
#include <mutex>
#include <atomic>
#include <functional>

class CPULoader {
    std::vector<std::string> _toLoadList;
    std::mutex _listLock;

    std::vector<std::thread *> _workers;
    std::atomic<uint16_t> _runningWorkersCnt;

    std::mutex _queueLock;
    std::mutex _queueAppendEventLock;
    std::queue<LoadTraceFile *> _loadedQueue;
    size_t _filesCnt;
    
    std::atomic<ssize_t> _maxRamAlloc;
    std::mutex _maxRamModLock;
    std::mutex _maxRamModEventLock;

    std::function<bool(LoadTraceFile*)> _filterfunc;
    
    void worker(int loaderNum);
    
public:
    CPULoader(const std::vector<std::string> &toLoadList, ssize_t maxRamAlloc, uint16_t maxLoaderThreads = 0, std::function<bool(LoadTraceFile*)> filterfunc = NULL);
    ~CPULoader();
    
    LoadTraceFile *dequeue();
};
#endif /* CPULoader_hpp */
