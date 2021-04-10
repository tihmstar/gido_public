//
//  GPUPowerModelExecutionWorker.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef GPUPowerModelExecutionWorker_hpp
#define GPUPowerModelExecutionWorker_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <iostream>
#include <thread>
#include "GPUMem.hpp"

class GPUPowerModel;

class GPUPowerModelExecutionWorker {
    GPUPowerModel *_parent; //no owned
    int _workernum;

    GPUMem *_tracefileGPU; //owns reference

    bool _stopWorker;
    std::thread *_workerThread;

    //clWaifForEventsWorkaround
    long   _kern_avg_runner_cnt;
    double _kern_avg_run_time;
    std::mutex _kern_avg_runLock;

    cl_event _myPrevWaitEvents[3];
    static constexpr int myPrevWaitEventsCnt = sizeof(_myPrevWaitEvents)/sizeof(*_myPrevWaitEvents);
    
    cl_mem _curMeanTrace;
    cl_mem _curCensumTrace;
    cl_mem _curMeanTraceCnt;
    cl_mem _curCensumTraceCnt;
    cl_mem _curHypotMeans;
    cl_mem _curHypotCensums;
    cl_mem _curHypotMeansCnt;
    cl_mem _curHypotCensumsCnt;
    cl_mem _curUpperPart;
    
    cl_kernel _kernel_computeMean;
    cl_kernel _kernel_computeMeanHypot;
    cl_kernel _kernel_computeCenteredSum;
    cl_kernel _kernel_computeUpper;
    cl_kernel _kernel_computeCenteredSumHypot;
    cl_kernel _kernel_combineMeanAndCenteredSum;
    cl_kernel _kernel_mergeCoutnersReal;
    cl_kernel _kernel_combineMeanAndCenteredSumHypot;
    cl_kernel _kernel_mergeCoutnersHypot;
    cl_kernel _kernel_combineUpper;
    
    std::mutex _prevEventAccessLock;

    
    void waitForMyPrevEventCompletion();
    void worker();
    
    cl_int relaxedWaitForEvent(cl_uint num_events, const cl_event * event_list);

    
public:
    GPUPowerModelExecutionWorker(GPUPowerModel *parent, int workerNum);
    GPUPowerModelExecutionWorker(const GPUPowerModelExecutionWorker &cpy) = delete;
    ~GPUPowerModelExecutionWorker();
    
    void waitForCompletion();

    friend GPUPowerModel;

};



#endif /* GPUPowerModelExecutionWorker_hpp */
