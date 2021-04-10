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
#include "GPUPowerModel.hpp"
#include <atomic>
#include "LoadFile.hpp"
#include "CPUModelResults.hpp"
#include <map>

class GPUPowerModelExecutionWorker;
class GPUDevice {
    int _deviceNum;
    cl_device_id _device_id;
    uint32_t _point_per_trace;
    cl_context _context;
    cl_ulong _deviceMemSize;
    cl_ulong _deviceMemAlloced;
    std::atomic<size_t> _GpuFreeMem;

    std::vector<GPUPowerModel*> _gpuPowerModels;

    std::atomic<uint32_t> _activeTraces;
    std::atomic<uint32_t> _processedTraceFiles;
    std::atomic<uint32_t> _workersWaitingForMemoryCnt;
    std::mutex _GpuFreeMemEventLock;
    std::mutex _activeTracesEventLock;
    
    
public:
    GPUDevice(int deviceNum, cl_device_id device_id, std::string kernelsource, uint32_t point_per_trace, std::vector<std::string> models);
    ~GPUDevice();
    
    void transferTrace(LoadFile *trace);
    int deviceNum();
    
    void getResults(std::map<std::string,std::map<std::string,CPUModelResults*>> &modelresults, bool isMidResult = false);

    friend GPUPowerModelExecutionWorker;
};

#endif /* GPUDevice_hpp */
