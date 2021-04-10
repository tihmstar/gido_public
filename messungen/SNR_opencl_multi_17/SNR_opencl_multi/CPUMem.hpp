//
//  CPUMem.hpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 09.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef CPUMem_hpp
#define CPUMem_hpp

#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include "Traces.hpp"

class CPUMem{
    Traces *_mem;
    std::mutex _eventlock;
    std::mutex _userLock;
    std::atomic_uint16_t _refCnt;
public:
    CPUMem(Traces *mem, size_t maxCPUAlloc);
    CPUMem(const CPUMem &cpy) = delete;
        
    void retain();
    void release();
    uint16_t getRefcnt();

    Traces *mem();
    
    ~CPUMem();
};


#endif /* CPUMem_hpp */
