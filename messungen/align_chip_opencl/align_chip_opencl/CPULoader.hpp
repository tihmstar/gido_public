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
#include "LoadFile.hpp"
#include <mutex>
#include <atomic>

class CPULoader {
    std::vector<std::string> _toLoadList;
    std::mutex _listLock;

    std::vector<std::thread *> _workers;
    std::atomic<uint16_t> _runningWorkersCnt;

    std::mutex _queueLock;
    std::mutex _queueAppendEventLock;
    std::queue<LoadFile *> _loadedQueue;
    size_t _minAllocSize;
    size_t _filesCnt;
    
    size_t _maxRamAlloc;
    std::mutex _maxRamModLock;
    std::mutex _maxRamModEventLock;

    
    void worker(int loaderNum);
    
public:
    CPULoader(const std::vector<std::string> &toLoadList, size_t maxRamAlloc, uint16_t maxLoaderThreads = 0, size_t minAllocSize = 0);
    ~CPULoader();
    
    LoadFile *dequeue();
};
#endif /* CPULoader_hpp */
