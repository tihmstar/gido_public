//
//  GPUPowerModel.hpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 26.11.19.
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
#include <stdint.h>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <set>

#include "numerics.hpp"
#include "GPUMem.hpp"

class GPUDeviceWorker;

class GPUPowerModel{
private:
    class ExecutionWorker{
        bool _stopWorker;
        int _workerNum;
        GPUPowerModel *_parent; //not owned
        GPUMem *_tracefileGPU; //owns reference

        cl_event _myPrevWaitEvent;
        
        cl_kernel _kernel_computeMeans;
        cl_kernel _kernel_computeCensums;
        cl_kernel _kernel_combineMeansAndCenteredSums;
        cl_kernel _kernel_mergeCounters;
        
        //curMem
        cl_mem _curgroupsMean;
        cl_mem _curgroupsCenSum;
        cl_mem _curgroupsMeanCnt;
        cl_mem _curgroupsCenSumCnt;

        std::thread *_workerThread;
        std::mutex _prevEventAccessLock;
        
        //clWaifForEventsWorkaround
        long   _kern_avg_runner_cnt;
        double _kern_avg_run_time;
        std::mutex _kern_avg_runLock;

        
        cl_int relaxedWaitForEvent(cl_uint num_events, const cl_event * event_list);

        
        void waitForMyPrevEventCompletion();
        
        void worker();
        
    public:
        ExecutionWorker(GPUPowerModel *parent, int workerNum);
        ExecutionWorker(const ExecutionWorker &cpy) = delete;
        ~ExecutionWorker();
        
        void waitForCompletion();

        friend GPUPowerModel;
    };
    
private:
    uint8_t _deviceNum;
    cl_ulong _deviceMemAlloced;

    cl_ulong _deviceWorkgroupSize;
    cl_uint _point_per_trace;
    cl_uint _modelSize;
    
    std::string _modelname;
    
    cl_context _context; //not owned
    cl_program _program;
    cl_command_queue _command_queue;
    
    //globalMem
    cl_mem _groupsMean;
    cl_mem _groupsCensum;
    cl_mem _groupsMeanCnt;
    cl_mem _groupsCensumCnt;
        
    std::mutex _prevWaitEventLock;
    cl_event _prevWaitEvent;
    
    std::vector<ExecutionWorker *> _ExecutionWorkers;
    
    std::queue<GPUMem *> _tracesQueue;
    std::mutex _tracesQueueLock;
    std::mutex _tracesQueueLockEvent;
    std::mutex _tracesQueueInProcessLockEvent;

    
public:
    GPUPowerModel(int deviceNum, cl_device_id device_id, cl_context context, std::string kernelsource, uint32_t point_per_trace, std::string modelname, cl_uint modelSize = 0x100);
    GPUPowerModel(const GPUPowerModel &cpy) = delete; //delete copy constructor
    ~GPUPowerModel();

    void spawnTranferworkers(uint32_t cnt);
    
    void enqueuTrace(GPUMem *trace);
    
    cl_ulong getDeviceMemAllocated();
    std::string getModelName();
    cl_uint getModelSize();

    void getResults(std::vector<combtrace> &retgroupsMean,std::vector<combtrace> &retgroupsCensum);
    
    friend ExecutionWorker;
    
    //TEST
    friend GPUDeviceWorker;
};

#endif /* GPUPowerModel_hpp */
