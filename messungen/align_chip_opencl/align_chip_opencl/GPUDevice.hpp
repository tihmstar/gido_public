//
//  GPUDevice.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef GPUDevice_hpp
#define GPUDevice_hpp

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
#include <map>
#include "GPUMem.hpp"

class GPUWorker;
class GPUDevice {
    int _deviceNum;
    cl_device_id _device_id;
    cl_context _context;
    cl_ulong _deviceWorkgroupSize;
    cl_ulong _deviceMemSize;
    std::atomic<size_t> _GpuFreeMem;


    std::atomic<uint32_t> _workersWaitingForMemoryCnt;
    std::mutex _GpuFreeMemEventLock;
    std::atomic<uint32_t> _activeTraces;
    std::atomic<uint32_t> _processedTraceFiles;
    std::mutex _activeTracesEventLock;

    
    cl_program _program;

public:
    const uint32_t _point_per_trace;

private:
    cl_mem alloc(size_t size, cl_mem_flags flags);
    void free(cl_mem mem, size_t size);
    
public:
    GPUDevice(int deviceNum, cl_device_id device_id, std::string kernelsource, uint32_t point_per_trace);
    ~GPUDevice();
        
    
    GPUMem *transferTrace(LoadFile *trace);
    int deviceNum();
    
    friend GPUWorker;
};

#endif /* GPUDevice_hpp */
