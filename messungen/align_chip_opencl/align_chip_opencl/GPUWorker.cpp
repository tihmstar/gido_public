//
//  GPUWorker.cpp
//  align_chip_opencl
//
//  Created by tihmstar on 10.01.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "GPUWorker.hpp"
#include <signal.h>
#include <tuple>
#include <unistd.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUWorker ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUWorker ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})

GPUWorker::GPUWorker(int workernum, GPUDevice *parent)
:   _workernum(workernum), _parent(parent), _workerThread(NULL), _stopWorker(false),
    _meanTrace(NULL), _kernel_computeMean(NULL), _command_queue(NULL)
{
    cl_int clret = 0;
    uint8_t zero = 0;

    _command_queue = clCreateCommandQueue(_parent->_context, _parent->_device_id, 0, &clret);assure(!clret);

    _meanTrace = _parent->alloc(_parent->_point_per_trace*sizeof(gpu_float_type), CL_MEM_READ_WRITE);
    _meanTraceCnt = _parent->alloc(sizeof(cl_ulong), CL_MEM_READ_WRITE);

    clret = clEnqueueFillBuffer(_command_queue, _meanTrace, &zero, 1, 0, _parent->_point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _meanTraceCnt, &zero, 1, 0, sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);

    _kernel_computeMean = clCreateKernel(_parent->_program, "computeMean", &clret);assure(!clret);
    _kernel_updateMeanCnt = clCreateKernel(_parent->_program, "updateMeanCnt", &clret);assure(!clret);
    
    //set arg 0 later
    clret = clSetKernelArg(_kernel_computeMean, 1, sizeof(cl_mem), &_meanTrace);assure(!clret);
    clret = clSetKernelArg(_kernel_computeMean, 2, sizeof(cl_mem), &_meanTraceCnt);assure(!clret);
    
    //set arg 0 later
    clret = clSetKernelArg(_kernel_updateMeanCnt, 1, sizeof(cl_mem), &_meanTraceCnt);assure(!clret);
    clret = clFinish(_command_queue);assure(!clret);
    
    _workerThread = new std::thread([this]{
        worker();
    });
}

GPUWorker::~GPUWorker(){
    assure_(!_workerThread);
    
    safeFreeCustom(_kernel_updateMeanCnt,clRetainKernel);
    safeFreeCustom(_kernel_computeMean,clRetainKernel);

    safeFreeCustom(_meanTraceCnt,clRetainMemObject);
    safeFreeCustom(_meanTrace,clRetainMemObject);

    safeFreeCustom(_command_queue,clRetainCommandQueue);
}

void GPUWorker::worker(){
    cl_int clret = 0;
    
    size_t local_work_size[2] = {};
    local_work_size[0] = _parent->_deviceWorkgroupSize;
    local_work_size[1] = 1;

    size_t global_work_size_trace[2] = {};
    global_work_size_trace[0] = _parent->_deviceWorkgroupSize;
    global_work_size_trace[1] = (_parent->_point_per_trace / _parent->_deviceWorkgroupSize);
    if (_parent->_point_per_trace % _parent->_deviceWorkgroupSize != 0) global_work_size_trace[1] ++;

    size_t global_work_size_combine[2] = {};
    global_work_size_combine[0] = _parent->_deviceWorkgroupSize;
    global_work_size_combine[1] = 1;
    
    printf("[D-%u][W-%u] Worker started\n",_parent->_deviceNum,_workernum);
    
    std::atomic<uint32_t> activeTracefiles{0};
    std::mutex activeTracefilesEventLock;
    
    cl_event prevEvent = NULL;
    GPUMem *myTracefileGPU = NULL;
    while (true) {

        if (_tracesQueue.size() == 0){
            _tracesQueueLockEvent.lock(); //wait for event
        }
        
        _tracesQueueLock.lock(); //grab lock for modiying queue

        if (_tracesQueue.size() == 0){
            _tracesQueueLock.unlock(); //release lock for modiying queue

            if (_stopWorker) {
                _tracesQueueLockEvent.unlock(); //we didn't handle the event, "re-throw" it
                break;
            }
            continue;
        }
        myTracefileGPU = _tracesQueue.front();
        activeTracefiles.fetch_add(1);

        _tracesQueue.pop();
        
        // BEGIN WORKER UPDATE KERNEL PARAMS
        clret = clSetKernelArg(_kernel_computeMean, 0, sizeof(cl_mem), &myTracefileGPU->mem());assure(!clret);
        clret = clSetKernelArg(_kernel_updateMeanCnt, 0, sizeof(cl_mem), &myTracefileGPU->mem());assure(!clret);
        // END WORKER UPDATE KERNEL PARAMS

        cl_event computeMeanEvent = {};
        if (prevEvent) {
            clret = clEnqueueNDRangeKernel(_command_queue, _kernel_computeMean, 2, NULL, global_work_size_trace, local_work_size, 1, &prevEvent, &computeMeanEvent);assure(!clret);
            clret = clReleaseEvent(prevEvent);assure(!clret);
            prevEvent = NULL;
        }else{
            clret = clEnqueueNDRangeKernel(_command_queue, _kernel_computeMean, 2, NULL, global_work_size_trace, local_work_size, 0, NULL, &computeMeanEvent);assure(!clret);
        }

        
        cl_event combineMeanEvent = {};
        clret = clEnqueueNDRangeKernel(_command_queue, _kernel_updateMeanCnt, 2, NULL, global_work_size_combine, local_work_size, 1, &computeMeanEvent, &combineMeanEvent);assure(!clret);

        clret = clReleaseEvent(computeMeanEvent);assure(!clret);
        computeMeanEvent = NULL;
                
        clret = clSetEventCallback(combineMeanEvent,  CL_COMPLETE, [](cl_event, cl_int, void *arg){
            std::tuple<void*,void*,void*> *decarg = (std::tuple<void*,void*,void*> *)arg;
            GPUMem *localmyTracefileGPU = (GPUMem*)std::get<0>(*decarg);
            std::atomic<uint32_t> *localactiveTracefiles = (std::atomic<uint32_t> *)std::get<1>(*decarg);
            std::mutex *localactiveTracefilesEventLock = (std::mutex *)std::get<2>(*decarg);
            
            localmyTracefileGPU->release();
            localactiveTracefiles->fetch_sub(1);
            localactiveTracefilesEventLock->unlock();
            delete decarg;
        }, new std::tuple<void*,void*,void*>(myTracefileGPU, &activeTracefiles, &activeTracefilesEventLock));
        
        prevEvent = combineMeanEvent; combineMeanEvent = NULL;

        _tracesQueueInProcessLockEvent.unlock();
        _tracesQueueLock.unlock(); //release lock for modiying queue
    }
    
    if (prevEvent) {
//#warning CPU hang here!!
//        clret = clWaitForEvents(1, &prevEvent);assure(!clret);
        {
            do{
                cl_int readStatus = CL_COMPLETE;
                clGetEventInfo(prevEvent, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof (cl_int), &readStatus, NULL);

                if (readStatus == CL_COMPLETE) break;
                usleep(420);
            }while (true);
        }
        
        
        clret = clReleaseEvent(prevEvent);assure(!clret);
        prevEvent = NULL;
    }
    
    while (activeTracefiles > 0) activeTracefilesEventLock.lock();
    assure_(activeTracefiles == 0);
    
    printf("[D-%u][W-%u] Stopping worker!\n",_parent->_deviceNum,_workernum);
}


void GPUWorker::enqueue(GPUMem *trace){
    trace->retain();
    
    _tracesQueueLock.lock(); //grab lock for modifying queue
    _tracesQueue.push(trace);
    _tracesQueueLockEvent.unlock(); //notify workers of available work
    _tracesQueueLock.unlock();//release lock for modifying queue
}

std::queue<LoadFile *> &GPUWorker::cputraces(){
    return _cputraces;
}

combtrace GPUWorker::getResults(){
    cl_int clret = 0;
    printf("[D-%u][W-%u] getting results\n",_parent->_deviceNum,_workernum);
    
    while (_tracesQueue.size() > 0) {
        printf("[D-%u][W-%u] waiting for traces to get processed (%lu traces left)\n",_parent->_deviceNum,_workernum,_tracesQueue.size());
        _tracesQueueInProcessLockEvent.lock();
    }
    
    printf("[D-%u][W-%u] worker finished sending final unlock event!\n",_parent->_deviceNum,_workernum);

    _stopWorker = true;
    _tracesQueueLockEvent.unlock(); //send work signal without having actual work, kills all workers

    _workerThread->join();
    delete _workerThread;
    _workerThread = NULL;
    
    combtrace meanTrace;

    meanTrace.cnt = 0;
    meanTrace.trace = (gpu_float_type*)malloc(_parent->_point_per_trace * sizeof(gpu_float_type));
    
    printf("[D-%u][W-%u] reading mean\n",_parent->_deviceNum,_workernum);

    clret = clEnqueueReadBuffer(_command_queue, _meanTrace, false, 0, _parent->_point_per_trace * sizeof(gpu_float_type), meanTrace.trace, 0, NULL, NULL); assure(!clret);
    clret = clEnqueueReadBuffer(_command_queue, _meanTraceCnt, false, 0, sizeof(cl_ulong), &meanTrace.cnt, 0, NULL, NULL); assure(!clret);

    clret = clFinish(_command_queue); assure(!clret);

    return meanTrace;
}
