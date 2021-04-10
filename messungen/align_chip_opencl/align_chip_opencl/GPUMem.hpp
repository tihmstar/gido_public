//
//  GPUMem.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 16.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef GPUMem_hpp
#define GPUMem_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <mutex>
#include <atomic>

class GPUMem{
    cl_mem _mem;
    std::mutex _eventlock;
    std::mutex _userLock;
    std::atomic_uint16_t _refCnt;

public:
    GPUMem(cl_mem mem);
    GPUMem(const GPUMem &cpy) = delete;
        
    void retain();
    void release();

    cl_mem &mem();
    
    ~GPUMem();
};

#endif /* GPUMem_hpp */
