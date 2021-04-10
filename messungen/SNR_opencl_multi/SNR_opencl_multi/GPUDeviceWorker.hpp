//
//  GPUDeviceWorker.hpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 28.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef GPUDeviceWorker_hpp
#define GPUDeviceWorker_hpp


#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <vector>
#include <map>
#include "GPUPowerModel.hpp"
#include "CPUModelResults.hpp"
#include "CPUMem.hpp"

class GPUDeviceWorker{
public:
    class GPULoaderWorker{
        bool _stopWorker;
        int _workerNum;
        GPUDeviceWorker *_parent; //not owned
        
        std::atomic<uint16_t> _workersWaitingForMemoryCnt;
        
        std::thread *_workerThread;
                
        void worker();
        
    public:
        GPULoaderWorker(GPUDeviceWorker *parent, int workerNum);
        GPULoaderWorker(const GPULoaderWorker &cpy) = delete;
        ~GPULoaderWorker();
                
        friend GPUDeviceWorker;
    };
    
private:
    int _deviceWorkerNum;
    cl_device_id _device_id;
    cl_context _context;
    cl_ulong _deviceMemSize;
    cl_ulong _deviceMemAlloced;
    cl_uint _tracesOnGPUCnt;
    uint32_t _point_per_trace;
    std::atomic<size_t> _GpuFreeMem;
    std::mutex _GpuFreeMemEventLock;
    
    std::vector<GPUPowerModel*> _gpuPowerModels;
    
    std::vector<GPULoaderWorker *> _loaderWorkers;

    
    std::queue<CPUMem *> _tracesQueue;
    std::mutex _tracesQueueLock;
    std::mutex _tracesQueueLockEvent;
    std::mutex _tracesQueueInProcessLockEvent;


public:
    GPUDeviceWorker(int deviceWorkerNum, cl_device_id device_id, std::string kernelsource, uint32_t point_per_trace, size_t traceFileSize, std::vector<std::pair<std::string,uint16_t>> models);
    ~GPUDeviceWorker();
    
    void enqueuTrace(CPUMem *trace);
    
    void getResults(std::map<std::string,CPUModelResults*> &modelresults);

    
};

#endif /* GPUDeviceWorker_hpp */
