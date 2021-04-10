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
#include "ResArray.hpp"
#include <mutex>
#include <atomic>

class CPULoader {
    std::vector<std::tuple<int,int,std::string>> _toLoadList;
    std::mutex _listLock;

    std::vector<std::thread *> _workers;
    std::atomic<uint16_t> _runningWorkersCnt;

    std::mutex _queueLock;
    std::mutex _queueAppendEventLock;
    std::queue<std::tuple<int,int,ResArray *>> _loadedQueue;
    size_t _filesCnt;
    
    void worker(int loaderNum);
    
public:
    CPULoader(const std::vector<std::tuple<int,int,std::string>> &toLoadList, uint16_t maxLoaderThreads = 0);
    ~CPULoader();
    
    std::tuple<int,int,ResArray *> dequeue();
};
#endif /* CPULoader_hpp */
