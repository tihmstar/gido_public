//
//  GPUPowerModel.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef GPUPowerModel_hpp
#define GPUPowerModel_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <iostream>
#include <vector>
#include "GPUMem.hpp"
#include <queue>
#include "CPUModelResults.hpp"

class GPUPowerModelExecutionWorker;
class GPUDevice;

class GPUPowerModel {
    GPUDevice *_parent; //not owned
    int _deviceNum;
    cl_device_id _device_id;
    cl_context _context;
    uint32_t _point_per_trace;
    std::string _modelname;

    cl_ulong _deviceWorkgroupSize;
    cl_program _program;
    cl_command_queue _command_queue;

    size_t _deviceMemAlloced;
    std::vector<GPUPowerModelExecutionWorker *> _ExecutionWorkers;

    cl_event _prevWaitEvents[3];
    static constexpr int prevWaitEventsCnt = sizeof(_prevWaitEvents)/sizeof(*_prevWaitEvents);

    
    cl_mem _gDeviceMeanTrace;
    cl_mem _gDeviceCensumTrace;
    cl_mem _gDeviceMeanTraceCnt;
    cl_mem _gDeviceCensumTraceCnt;
    cl_mem _gDeviceHypotMeans;
    cl_mem _gDeviceHypotCensums;
    cl_mem _gDeviceHypotMeansCnt;
    cl_mem _gDeviceHypotCensumsCnt;
    cl_mem _gDeviceUpperPart;
    
    std::queue<GPUMem *> _tracesQueue;
    std::mutex _tracesQueueLock;
    std::mutex _tracesQueueLockEvent;
    std::mutex _tracesQueueInProcessLockEvent;

    
public:
    GPUPowerModel(GPUDevice *parent, int deviceNum, cl_device_id device_id, cl_context context, std::string kernelsource, uint32_t point_per_trace, std::string modelname, int8_t bytepos = -1);
    GPUPowerModel(const GPUPowerModel &cpy) = delete; //delete copy constructor
    ~GPUPowerModel();
    
    void clearMemory();
    
    void spawnExecutionWorker(uint32_t cnt);
    
    cl_ulong getDeviceMemAllocated();
    std::string getModelName();
    
    void enqueuTrace(GPUMem *trace);

    void getResults(CPUModelResults *out);

    
    friend GPUPowerModelExecutionWorker;
};

#endif /* GPUPowerModel_hpp */
