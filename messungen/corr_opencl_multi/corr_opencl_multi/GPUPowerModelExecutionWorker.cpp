//
//  GPUPowerModelExecutionWorker.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "GPUPowerModelExecutionWorker.hpp"
#include "GPUPowerModel.hpp"
#include <signal.h>
#include "GPUDevice.hpp"
#include <unistd.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUPowerModelExecutionWorker[%s] ASSURE FAILED ON LINE=%d with err=%d\n",_parent->_modelname.c_str(),__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUPowerModelExecutionWorker[%s] ASSURE FAILED ON LINE=%d\n",_parent->_modelname.c_str(),__LINE__); raise(SIGABRT); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})


#pragma mark helpers

#define NSEC 1e-9

double timespec_to_double(struct timespec *t)
{
    return ((double)t->tv_sec) + ((double) t->tv_nsec) * NSEC;
}

void double_to_timespec(double dt, struct timespec *t)
{
    t->tv_sec = (long)dt;
    t->tv_nsec = (long)((dt - t->tv_sec) / NSEC);
}

void get_time(struct timespec *t)
{
    clock_gettime(CLOCK_MONOTONIC, t);
}

#pragma mark GPUPowerModelExecutionWorker

GPUPowerModelExecutionWorker::GPUPowerModelExecutionWorker(GPUPowerModel *parent, int workerNum)
: _parent(parent), _workernum(workerNum), _tracefileGPU(NULL), _stopWorker(false), _workerThread(NULL)
//clWaifForEventsWorkaround
,_kern_avg_runner_cnt(0)
,_kern_avg_run_time(0)

,_myPrevWaitEvents{}

, _curMeanTrace(NULL)
, _curCensumTrace(NULL)
, _curMeanTraceCnt(NULL)
, _curCensumTraceCnt(NULL)
, _curHypotMeans(NULL)
, _curHypotCensums(NULL)
, _curHypotMeansCnt(NULL)
, _curHypotCensumsCnt(NULL)
, _curUpperPart(NULL)

, _kernel_computeMean(NULL)
, _kernel_computeMeanHypot(NULL)
, _kernel_computeCenteredSum(NULL)
, _kernel_computeUpper(NULL)
, _kernel_computeCenteredSumHypot(NULL)
, _kernel_combineMeanAndCenteredSum(NULL)
, _kernel_mergeCoutnersReal(NULL)
, _kernel_combineMeanAndCenteredSumHypot(NULL)
, _kernel_mergeCoutnersHypot(NULL)
, _kernel_combineUpper(NULL)
{
    cl_int clret = 0;
    
    /* --- BEGIN Device Trace Memory --- */

    /*
     gpu_float_type curmean[_parent->_point_per_trace];
     gpu_float_type curcensum[_parent->_point_per_trace];
    */

    _curMeanTrace = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, _parent->_point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += _parent->_point_per_trace * sizeof(gpu_float_type);

            
    _curCensumTrace = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, _parent->_point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += _parent->_point_per_trace * sizeof(gpu_float_type);

    /*
     uint64_t curcntMean = 0;
     uint64_t curcntcensum = 0;
     */
    _curMeanTraceCnt = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += sizeof(cl_ulong);

    _curCensumTraceCnt = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += sizeof(cl_ulong);

    /*
    gpu_float_type curhypotMeanGuessVals[1] = {};
    gpu_float_type curhypotCensumGuessVals[1] = {};
    */

    _curHypotMeans = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, 1 * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += 1 * sizeof(gpu_float_type);

    _curHypotCensums = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, 1 * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += 1 * sizeof(gpu_float_type);


    /*
     uint64_t curhypotMeanGuessCnt[1] = {};
     uint64_t curhypotCensumGuessCnt[1] = {};
     */
    _curHypotMeansCnt = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, 1 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += 1 * sizeof(cl_ulong);

    _curHypotCensumsCnt = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, 1 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += 1 * sizeof(cl_ulong);
    
    /*
     numberedTrace curgupperPart = (numberedTrace *)malloc(1*sizeof(numberedTrace));
     for (int k = 0; k<1; k++) {
         curgupperPart[k].cnt = 0;
         curgupperPart[k].trace = (gpu_float_type*)malloc(_parent->_point_per_trace * sizeof(gpu_float_type));
         memset(curgupperPart[k].trace, 0, _parent->_point_per_trace * sizeof(gpu_float_type));
     }
     */
    _curUpperPart = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, 1 * _parent->_point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += 1 * _parent->_point_per_trace * sizeof(gpu_float_type);
    
    /* --- END Device Trace Memory --- */
    
    
    
    // BEGIN WORKER CREATE KERNELS
    _kernel_computeMean = clCreateKernel(_parent->_program, "computeMean", &clret);assure(!clret);
    _kernel_computeMeanHypot = clCreateKernel(_parent->_program, "computeMeanHypot", &clret);assure(!clret);

    _kernel_computeCenteredSum = clCreateKernel(_parent->_program, "computeCenteredSum", &clret);assure(!clret);
    _kernel_computeUpper = clCreateKernel(_parent->_program, "computeUpper", &clret);assure(!clret);
    _kernel_computeCenteredSumHypot = clCreateKernel(_parent->_program, "computeCenteredSumHypot", &clret);assure(!clret);

    _kernel_combineMeanAndCenteredSum = clCreateKernel(_parent->_program, "combineMeanAndCenteredSum", &clret);assure(!clret);
    _kernel_mergeCoutnersReal = clCreateKernel(_parent->_program, "mergeCoutners", &clret);assure(!clret);

    _kernel_combineMeanAndCenteredSumHypot = clCreateKernel(_parent->_program, "combineMeanAndCenteredSum", &clret);assure(!clret);
    _kernel_mergeCoutnersHypot = clCreateKernel(_parent->_program, "mergeCoutners", &clret);assure(!clret);

    _kernel_combineUpper = clCreateKernel(_parent->_program, "kernelAddTrace", &clret);assure(!clret);
    // END WORKER CREATE KERNELS

    
    // BEGIN WORKER SET KERNEL PARAMS

    //trace 0 param is set later
    clret = clSetKernelArg(_kernel_computeMean, 1, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_computeMean, 2, sizeof(cl_mem), &_curMeanTraceCnt);assure(!clret);

    //trace 0 param is set later
    clret = clSetKernelArg(_kernel_computeMeanHypot, 1, sizeof(cl_mem), &_curHypotMeans);assure(!clret);
    clret = clSetKernelArg(_kernel_computeMeanHypot, 2, sizeof(cl_mem), &_curHypotMeansCnt);assure(!clret);

    //trace 0 param is set later
    clret = clSetKernelArg(_kernel_computeCenteredSum, 1, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCenteredSum, 2, sizeof(cl_mem), &_curHypotMeans);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCenteredSum, 3, sizeof(cl_mem), &_curCensumTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCenteredSum, 4, sizeof(cl_mem), &_curCensumTraceCnt);assure(!clret);

    //trace 0 param is set later
    clret = clSetKernelArg(_kernel_computeUpper, 1, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_computeUpper, 2, sizeof(cl_mem), &_curHypotMeans);assure(!clret);
    clret = clSetKernelArg(_kernel_computeUpper, 3, sizeof(cl_mem), &_curUpperPart);assure(!clret);

    //trace 0 param is set later
    clret = clSetKernelArg(_kernel_computeCenteredSumHypot, 1, sizeof(cl_mem), &_curHypotMeans);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCenteredSumHypot, 2, sizeof(cl_mem), &_curHypotCensums);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCenteredSumHypot, 3, sizeof(cl_mem), &_curHypotCensumsCnt);assure(!clret);

    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 0, sizeof(cl_mem), &_parent->_gDeviceCensumTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 1, sizeof(cl_mem), &_parent->_gDeviceCensumTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 2, sizeof(cl_mem), &_curCensumTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 3, sizeof(cl_mem), &_curCensumTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 4, sizeof(cl_mem), &_parent->_gDeviceMeanTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 5, sizeof(cl_mem), &_parent->_gDeviceMeanTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 6, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 7, sizeof(cl_mem), &_curMeanTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSum, 8, sizeof(cl_uint), &_parent->_point_per_trace);assure(!clret);

    cl_uint one = 1;
    clret = clSetKernelArg(_kernel_mergeCoutnersReal, 0, sizeof(cl_mem), &_parent->_gDeviceCensumTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersReal, 1, sizeof(cl_mem), &_curCensumTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersReal, 2, sizeof(cl_mem), &_parent->_gDeviceMeanTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersReal, 3, sizeof(cl_mem), &_curMeanTraceCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersReal, 4, sizeof(cl_uint), &one);assure(!clret);

    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 0, sizeof(cl_mem), &_parent->_gDeviceHypotCensums);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 1, sizeof(cl_mem), &_parent->_gDeviceHypotCensumsCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 2, sizeof(cl_mem), &_curHypotCensums);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 3, sizeof(cl_mem), &_curHypotCensumsCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 4, sizeof(cl_mem), &_parent->_gDeviceHypotMeans);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 5, sizeof(cl_mem), &_parent->_gDeviceHypotMeansCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 6, sizeof(cl_mem), &_curHypotMeans);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 7, sizeof(cl_mem), &_curHypotMeansCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeanAndCenteredSumHypot, 8, sizeof(cl_uint), &one);assure(!clret);

    clret = clSetKernelArg(_kernel_mergeCoutnersHypot, 0, sizeof(cl_mem), &_parent->_gDeviceHypotCensumsCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersHypot, 1, sizeof(cl_mem), &_curHypotCensumsCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersHypot, 2, sizeof(cl_mem), &_parent->_gDeviceHypotMeansCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersHypot, 3, sizeof(cl_mem), &_curHypotMeansCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCoutnersHypot, 4, sizeof(cl_uint), &one);assure(!clret);

    
    cl_uint valUpperTotal = 1 * _parent->_point_per_trace;
    clret = clSetKernelArg(_kernel_combineUpper, 0, sizeof(cl_mem), &_parent->_gDeviceUpperPart);assure(!clret);
    clret = clSetKernelArg(_kernel_combineUpper, 1, sizeof(cl_mem), &_curUpperPart);assure(!clret);
    clret = clSetKernelArg(_kernel_combineUpper, 2, sizeof(cl_uint), &valUpperTotal);assure(!clret);
    
    // END WORKER SET KERNEL PARAMS
    _workerThread = new std::thread([this]{
        worker();
    });
}

GPUPowerModelExecutionWorker::~GPUPowerModelExecutionWorker(){
    waitForCompletion();

    safeFreeCustom(_curMeanTrace,clReleaseMemObject);
    safeFreeCustom(_curMeanTrace,clReleaseMemObject);
    safeFreeCustom(_curCensumTrace,clReleaseMemObject);
    safeFreeCustom(_curMeanTraceCnt,clReleaseMemObject);
    safeFreeCustom(_curCensumTraceCnt,clReleaseMemObject);
    safeFreeCustom(_curHypotMeans,clReleaseMemObject);
    safeFreeCustom(_curHypotCensums,clReleaseMemObject);
    safeFreeCustom(_curHypotMeansCnt,clReleaseMemObject);
    safeFreeCustom(_curHypotCensumsCnt,clReleaseMemObject);
    safeFreeCustom(_curUpperPart,clReleaseMemObject);
    
    safeFreeCustom(_kernel_computeMean, clReleaseKernel);
    safeFreeCustom(_kernel_computeMeanHypot, clReleaseKernel);
    safeFreeCustom(_kernel_computeCenteredSum, clReleaseKernel);
    safeFreeCustom(_kernel_computeUpper, clReleaseKernel);
    safeFreeCustom(_kernel_computeCenteredSumHypot, clReleaseKernel);
    safeFreeCustom(_kernel_combineMeanAndCenteredSum, clReleaseKernel);
    safeFreeCustom(_kernel_mergeCoutnersReal, clReleaseKernel);
    safeFreeCustom(_kernel_combineMeanAndCenteredSumHypot, clReleaseKernel);
    safeFreeCustom(_kernel_mergeCoutnersHypot, clReleaseKernel);
    safeFreeCustom(_kernel_combineUpper, clReleaseKernel);
}

void GPUPowerModelExecutionWorker::waitForMyPrevEventCompletion(){
    cl_int clret = 0;
    _prevEventAccessLock.lock();
    if (_myPrevWaitEvents[0]){
        clret = relaxedWaitForEvent(myPrevWaitEventsCnt, _myPrevWaitEvents);assure(!clret);
        for (int i=0; i<myPrevWaitEventsCnt; i++) {
            clret = clReleaseEvent(_myPrevWaitEvents[i]);assure(!clret); _myPrevWaitEvents[i] = NULL;
        }
//        _parent->_parent->_activeWorkersRunning.fetch_sub(1);
    }
    if (_tracefileGPU) {
        //release tracefile memory on device
        clret = clSetKernelArg(_kernel_computeMean, 0, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
        clret = clSetKernelArg(_kernel_computeMeanHypot, 0, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
        clret = clSetKernelArg(_kernel_computeCenteredSum, 0, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
        clret = clSetKernelArg(_kernel_computeUpper, 0, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
        clret = clSetKernelArg(_kernel_computeCenteredSumHypot, 0, sizeof(cl_mem), &_curMeanTrace);assure(!clret);
        //release file on gpu
        _tracefileGPU->release();
        _tracefileGPU = NULL;
    }
    _prevEventAccessLock.unlock();
//    _parent->_parent->_activeWorkersDoneworkLockEvent.unlock();
}

//#define assureNotResourceError(cond) do {if (!(cond)) {assure(clret == CL_OUT_OF_RESOURCES); printf("ERROR: GPUPowerModelExecutionWorker out of resources in line=%u, but retrying...\n",__LINE__); \
//assure_(_parent->_parent->_activeWorkersPossible.fetch_sub(1) >1);\
//_parent->_parent->_activeWorkersRunning.fetch_sub(1);/*this was me, but i'm not active atm*/\
//_parent->_tracesQueueLock.unlock();\
//printf("WARNING: reducing max concurrent workers to=%u\n",(uint32_t)_parent->_parent->_activeWorkersPossible);goto retryBecauseOfOutOfResourcesLoc;\
//}}while (0)

#define assureNotResourceError(cond) assure(cond)

void GPUPowerModelExecutionWorker::worker(){
    cl_int clret = 0;
    
    size_t local_work_size[2] = {};
    local_work_size[0] = _parent->_deviceWorkgroupSize;
    local_work_size[1] = 1;

    size_t global_work_size_trace[2] = {};
    global_work_size_trace[0] = _parent->_deviceWorkgroupSize;
    global_work_size_trace[1] = (_parent->_point_per_trace / _parent->_deviceWorkgroupSize)+1;

    size_t global_work_size_key[2] = {};
    global_work_size_key[0] = _parent->_deviceWorkgroupSize;
    global_work_size_key[1] = (1 / _parent->_deviceWorkgroupSize);
    if (1 % _parent->_deviceWorkgroupSize != 0) global_work_size_key[1] ++;
                
    size_t global_work_size_upper[2] = {};
    global_work_size_upper[0] = _parent->_deviceWorkgroupSize;
    global_work_size_upper[1] = ((_parent->_point_per_trace*1) / _parent->_deviceWorkgroupSize);
    if ((_parent->_point_per_trace*1) % _parent->_deviceWorkgroupSize != 0) global_work_size_upper[1] ++;
    
    printf("[D-%u][M-%s][T-%u] Worker started\n",_parent->_deviceNum,_parent->_modelname.c_str(),_workernum);

    GPUMem *myTracefileGPU = NULL;
    while (true) {

        waitForMyPrevEventCompletion();
        
        
        if (_parent->_tracesQueue.size() == 0){
            _parent->_tracesQueueLockEvent.lock(); //wait for event
        }
        
        _parent->_tracesQueueLock.lock(); //grab lock for modiying queue

        if (_parent->_tracesQueue.size() == 0){
//            printf("[D-%u][M-%s][T-%u] Worker called but there is no work!\n",_parent->_deviceNum,_parent->_modelname.c_str(),_workerNum);
            _parent->_tracesQueueLock.unlock(); //release lock for modiying queue

            if (_stopWorker) {
                printf("[D-%u][M-%s][T-%u] Stopping worker!\n",_parent->_deviceNum,_parent->_modelname.c_str(),_workernum);
                _parent->_tracesQueueLockEvent.unlock(); //we didn't handle the event, "re-throw" it
                return;
            }
            continue;
        }
        myTracefileGPU = _parent->_tracesQueue.front();

        _parent->_tracesQueue.pop();
        
        _parent->_tracesQueueInProcessLockEvent.unlock(); //send event that we are working on a trace
        
        if (_parent->_tracesQueue.size() > 0){
            _parent->_tracesQueueLockEvent.unlock(); //make others re-check if work is available
        }
        
        // BEGIN WORKER UPDATE KERNEL PARAMS
        clret = clSetKernelArg(_kernel_computeMean, 0, sizeof(cl_mem), &myTracefileGPU->mem());assureNotResourceError(!clret);
        clret = clSetKernelArg(_kernel_computeMeanHypot, 0, sizeof(cl_mem), &myTracefileGPU->mem());assureNotResourceError(!clret);

        clret = clSetKernelArg(_kernel_computeCenteredSum, 0, sizeof(cl_mem), &myTracefileGPU->mem());assureNotResourceError(!clret);
        clret = clSetKernelArg(_kernel_computeUpper, 0, sizeof(cl_mem), &myTracefileGPU->mem());assureNotResourceError(!clret);
        clret = clSetKernelArg(_kernel_computeCenteredSumHypot, 0, sizeof(cl_mem), &myTracefileGPU->mem());assureNotResourceError(!clret);
        // END WORKER UPDATE KERNEL PARAMS

        //compute mean
        cl_event computeMeanEvents[2] = {};
        
        if (_parent->_prevWaitEvents[0]) {
            clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeMean, 2, NULL, global_work_size_trace, local_work_size, _parent->prevWaitEventsCnt, _parent->_prevWaitEvents, &computeMeanEvents[0]);assureNotResourceError(!clret);
            clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeMeanHypot, 2, NULL, global_work_size_key, local_work_size, _parent->prevWaitEventsCnt, _parent->_prevWaitEvents, &computeMeanEvents[1]);assureNotResourceError(!clret);
        }else{
            clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeMean, 2, NULL, global_work_size_trace, local_work_size, 0, NULL, &computeMeanEvents[0]);assureNotResourceError(!clret);
            clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeMeanHypot, 2, NULL, global_work_size_key, local_work_size, 0, NULL, &computeMeanEvents[1]);assureNotResourceError(!clret);
        }
                            
        //compute censum
        cl_event computeCensumEvents[3] = {};
        
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeCenteredSum, 2, NULL, global_work_size_trace, local_work_size, 2, computeMeanEvents, &computeCensumEvents[0]);assureNotResourceError(!clret);
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeCenteredSumHypot, 2, NULL, global_work_size_key, local_work_size, 1, &computeMeanEvents[1], &computeCensumEvents[1]);assureNotResourceError(!clret);
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeUpper, 2, NULL, global_work_size_trace, local_work_size, 2, computeMeanEvents, &computeCensumEvents[2]);assureNotResourceError(!clret);

        
        clret = clReleaseEvent(computeMeanEvents[0]);assureNotResourceError(!clret); computeMeanEvents[0] = NULL;
        clret = clReleaseEvent(computeMeanEvents[1]);assureNotResourceError(!clret); computeMeanEvents[1] = NULL;
                          
        constexpr int mergeEventsCnt = 3;
        cl_event mergeEvents[mergeEventsCnt];
        static_assert(mergeEventsCnt*sizeof(*mergeEvents) == sizeof(_myPrevWaitEvents), "prevEvents size mismatch");
        
        
        //combine mean and censum hypot
        cl_event combineMeanAndCensumHypotEvent = 0;
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_combineMeanAndCenteredSumHypot, 2, NULL, global_work_size_key, local_work_size, 1, &computeCensumEvents[1], &combineMeanAndCensumHypotEvent);assureNotResourceError(!clret);
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_mergeCoutnersHypot, 2, NULL, global_work_size_key, local_work_size, 1, &combineMeanAndCensumHypotEvent, &mergeEvents[0]);assureNotResourceError(!clret);

        //combine mean and censum
        cl_event combineMeanAndCensumEvent = 0;
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_combineMeanAndCenteredSum, 2, NULL, global_work_size_trace, local_work_size, 1, &computeCensumEvents[0], &combineMeanAndCensumEvent);assureNotResourceError(!clret);
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_mergeCoutnersReal, 2, NULL, global_work_size_key, local_work_size, 1, &combineMeanAndCensumEvent, &mergeEvents[1]);assureNotResourceError(!clret);

        //combine upper
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_combineUpper, 2, NULL, global_work_size_upper, local_work_size, 1, &computeCensumEvents[2], &mergeEvents[2]);assureNotResourceError(!clret);
        
        
        clret = clReleaseEvent(computeCensumEvents[0]);assureNotResourceError(!clret); computeCensumEvents[0] = NULL;
        clret = clReleaseEvent(computeCensumEvents[1]);assureNotResourceError(!clret); computeCensumEvents[1] = NULL;
        clret = clReleaseEvent(computeCensumEvents[2]);assureNotResourceError(!clret); computeCensumEvents[2] = NULL;

        clret = clReleaseEvent(combineMeanAndCensumEvent);assureNotResourceError(!clret); combineMeanAndCensumEvent = NULL;
        clret = clReleaseEvent(combineMeanAndCensumHypotEvent);assureNotResourceError(!clret); combineMeanAndCensumHypotEvent = NULL;
        
        //share last event
        for (int i=0; i<myPrevWaitEventsCnt; i++) {
            if (_parent->_prevWaitEvents[i]) {
                clret = clReleaseEvent(_parent->_prevWaitEvents[i]);assure(!clret); _parent->_prevWaitEvents[i] = NULL;
            }

            clret = clRetainEvent(mergeEvents[i]);assure(!clret); //increment ref for prevWaitEvents
            _parent->_prevWaitEvents[i] = mergeEvents[i];
        }
        clret = clFlush(_parent->_command_queue); assure(!clret);
        _parent->_tracesQueueLock.unlock(); //release lock for modifying traces queue *AND* _prevWaitEvent (device command_queue)

        //remember my own last event
        _prevEventAccessLock.lock();
        assure_(!_tracefileGPU);
        _tracefileGPU = myTracefileGPU; myTracefileGPU = NULL;
        for (int i=0; i<myPrevWaitEventsCnt; i++) {
            assure_(_myPrevWaitEvents[i] == NULL);
            _myPrevWaitEvents[i] = mergeEvents[i];
        }
        _prevEventAccessLock.unlock();
        
    }
}


cl_int GPUPowerModelExecutionWorker::relaxedWaitForEvent(cl_uint num_events, const cl_event * event_list){
    cl_int ret = 0;
    struct timespec start_time;
    struct timespec end_time;
    get_time(&start_time);
    
    if (_kern_avg_run_time) {
        struct timespec t;
        double_to_timespec(_kern_avg_run_time*0.98, &t);
        nanosleep(&t, NULL);
        ret = clWaitForEvents(num_events, event_list);
    }else{
        do{
            cl_int readStatus = CL_COMPLETE;
            for (int i=0; i<num_events; i++) {
                clGetEventInfo(event_list[i], CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof (cl_int), &readStatus, NULL);
                if (readStatus != CL_COMPLETE) break;
            }
            if (readStatus == CL_COMPLETE) break;
            sleep(1);
        }while (true);
    }

    get_time(&end_time);

    double dstart, dend, delta;
    dstart = timespec_to_double(&start_time);
    dend = timespec_to_double(&end_time);

    delta = dend - dstart;
    _kern_avg_runLock.lock();
    _kern_avg_run_time += (delta-_kern_avg_run_time)/(++_kern_avg_runner_cnt);
//    printf("[D-%u][M-%s][T-%u] kern_avg_run_time=%f\n",_parent->_deviceNum,_parent->_modelname.c_str(),_workerNum,_kern_avg_run_time);
    _kern_avg_runLock.unlock();
    return ret;
}

void GPUPowerModelExecutionWorker::waitForCompletion(){
    if (_workerThread) { //wait for waiter thread first
        _workerThread->join();
        delete _workerThread;
        _workerThread = NULL;
    }
    //then wait for event if needed (shouldn't be needed)
    waitForMyPrevEventCompletion();
}
