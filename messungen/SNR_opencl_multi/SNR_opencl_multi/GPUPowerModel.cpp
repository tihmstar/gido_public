//
//  GPUPowerModel.cpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 26.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "GPUPowerModel.hpp"
#include <sys/mman.h>
#include <string.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUPowerModel ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUPowerModel ASSURE FAILED ON LINE=%d\n",__LINE__); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})

class guard{
    std::function<void()> _f;
public:
    guard(std::function<void()> cleanup) : _f(cleanup) {}
    guard(const guard&) = delete; //delete copy constructor
    guard(guard &&o) = delete; //move constructor
    
    ~guard(){_f();}
};
#define cleanup(f) guard _cleanup(f);

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

#pragma mark ExecutionWorker

GPUPowerModel::ExecutionWorker::ExecutionWorker(GPUPowerModel *parent, int workerNum)
: _stopWorker(false), _workerNum(workerNum),_parent(parent), _tracefileGPU(NULL), _myPrevWaitEvent(NULL)
    ,_kern_avg_runner_cnt(0), _kern_avg_run_time(0)
{
    cl_int clret = 0;
    
    //BEGIN ALLOC TRANSFER MEM
    /*
    std::vector<combtrace> curgroupsMean{0x100};
    for (auto &cmbtr : curgroupsMean){
        cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
    }
     
     std::vector<combtrace> curgroupsCenSum{0x100};
     for (auto &cmbtr : curgroupsCenSum){
         cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
         cmbtr.cnt = 0;
         memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
     }
    */

    _curgroupsMean = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type);

    _curgroupsCenSum = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type);


    /*
     cl_ulong curgroupsMeanCnt[0x100] = {};
     cl_ulong curgroupsCenSumCnt[0x100] = {};
     */
    _curgroupsMeanCnt = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, _parent->_modelSize * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += _parent->_modelSize * sizeof(cl_ulong);

    _curgroupsCenSumCnt = clCreateBuffer(_parent->_context, CL_MEM_READ_WRITE, _parent->_modelSize * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _parent->_deviceMemAlloced += _parent->_modelSize * sizeof(cl_ulong);
    //BEGIN ALLOC TRANSFER MEM

    
    // BEGIN WORKER CREATE KERNELS
    _kernel_computeMeans = clCreateKernel(_parent->_program, "computeMeans", &clret);assure(!clret);
    _kernel_computeCensums = clCreateKernel(_parent->_program, "computeCenteredSums", &clret);assure(!clret);

    _kernel_combineMeansAndCenteredSums = clCreateKernel(_parent->_program, "combineMeanAndCenteredSums", &clret);assure(!clret);
    _kernel_mergeCounters = clCreateKernel(_parent->_program, "mergeCoutners", &clret);assure(!clret);
    // END WORKER CREATE KERNELS

    // BEGIN WORKER SET KERNEL PARAMS

    //trace 0 param is set later
    clret = clSetKernelArg(_kernel_computeMeans, 1, sizeof(cl_mem), &_curgroupsMean);assure(!clret);
    clret = clSetKernelArg(_kernel_computeMeans, 2, sizeof(cl_mem), &_curgroupsMeanCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_computeMeans, 3, sizeof(cl_uint), &_parent->_modelSize);assure(!clret);

    //trace 0 param is set later
    clret = clSetKernelArg(_kernel_computeCensums, 1, sizeof(cl_mem), &_curgroupsMean);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCensums, 2, sizeof(cl_mem), &_curgroupsMeanCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCensums, 3, sizeof(cl_mem), &_curgroupsCenSum);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCensums, 4, sizeof(cl_mem), &_curgroupsCenSumCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_computeCensums, 5, sizeof(cl_uint), &_parent->_modelSize);assure(!clret);

    
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 0, sizeof(cl_mem), &_parent->_groupsCensum);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 1, sizeof(cl_mem), &_parent->_groupsCensumCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 2, sizeof(cl_mem), &_curgroupsCenSum);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 3, sizeof(cl_mem), &_curgroupsCenSumCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 4, sizeof(cl_mem), &_parent->_groupsMean);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 5, sizeof(cl_mem), &_parent->_groupsMeanCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 6, sizeof(cl_mem), &_curgroupsMean);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 7, sizeof(cl_mem), &_curgroupsMeanCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 8, sizeof(cl_uint), &_parent->_point_per_trace);assure(!clret);
    clret = clSetKernelArg(_kernel_combineMeansAndCenteredSums, 9, sizeof(cl_uint), &_parent->_modelSize);assure(!clret);

    
    clret = clSetKernelArg(_kernel_mergeCounters, 0, sizeof(cl_mem), &_parent->_groupsCensumCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCounters, 1, sizeof(cl_mem), &_curgroupsCenSumCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCounters, 2, sizeof(cl_mem), &_parent->_groupsMeanCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCounters, 3, sizeof(cl_mem), &_curgroupsMeanCnt);assure(!clret);
    clret = clSetKernelArg(_kernel_mergeCounters, 4, sizeof(cl_uint), &_parent->_modelSize);assure(!clret);
    // END WORKER SET KERNEL PARAMS
    
    _workerThread = new std::thread([this]{
        worker();
    });
}

GPUPowerModel::ExecutionWorker::~ExecutionWorker(){
    waitForCompletion();
    
    safeFreeCustom(_curgroupsMean,clReleaseMemObject);
    safeFreeCustom(_curgroupsCenSum,clReleaseMemObject);
    safeFreeCustom(_curgroupsMeanCnt,clReleaseMemObject);
    safeFreeCustom(_curgroupsCenSumCnt,clReleaseMemObject);

    
    safeFreeCustom(_kernel_computeMeans, clReleaseKernel);
    safeFreeCustom(_kernel_computeCensums, clReleaseKernel);
    safeFreeCustom(_kernel_combineMeansAndCenteredSums, clReleaseKernel);
    safeFreeCustom(_kernel_mergeCounters, clReleaseKernel);
}

void GPUPowerModel::ExecutionWorker::waitForCompletion(){
    if (_workerThread) { //wait for waiter thread first
        _workerThread->join();
        delete _workerThread;
        _workerThread = NULL;
    }
    //then wait for event if needed (shouldn't be needed)
    waitForMyPrevEventCompletion();
}

void GPUPowerModel::ExecutionWorker::worker(){
    cl_int clret = 0;
    uint8_t zero = 0;
    
    size_t global_work_size[2] = {};
    global_work_size[0] = _parent->_deviceWorkgroupSize;
    global_work_size[1] = (_parent->_point_per_trace / _parent->_deviceWorkgroupSize);
    
    if ((_parent->_point_per_trace % _parent->_deviceWorkgroupSize)) {
        global_work_size[1]++;
    }

    size_t local_work_size[2] = {};
    local_work_size[0] = _parent->_deviceWorkgroupSize;
    local_work_size[1] = 1;
    printf("[D-%u][M-%s][T-%u] Worker started\n",_parent->_deviceNum,_parent->_modelname.c_str(),_workerNum);

    while (true) {
        GPUMem *myTraceFile = NULL;
        waitForMyPrevEventCompletion();

        _parent->_tracesQueueLock.lock(); //grab lock for modiying queue

        if (_parent->_tracesQueue.size() == 0){
            _parent->_tracesQueueLock.unlock(); //release lock for modiying queue, before waiting for event

            _parent->_tracesQueueLockEvent.lock(); //wait for event
            
            _parent->_tracesQueueLock.lock(); //grab lock for modiying queue and handle event
        }
        

        if (_parent->_tracesQueue.size() == 0){
//            printf("[D-%u][M-%s][T-%u] Worker called but there is no work!\n",_parent->_deviceNum,_parent->_modelname.c_str(),_workerNum);
            _parent->_tracesQueueLock.unlock(); //release lock for modiying queue

            if (_stopWorker) {
                printf("[D-%u][M-%s][T-%u] Stopping worker!\n",_parent->_deviceNum,_parent->_modelname.c_str(),_workerNum);
                _parent->_tracesQueueLockEvent.unlock(); //we didn't handle the event, "re-throw" it
                return;
            }
            continue;
        }
        myTraceFile = _parent->_tracesQueue.front();

        _parent->_tracesQueue.pop();

        _parent->_tracesQueueInProcessLockEvent.unlock(); //send event that we are working on a trace
        //we have a trace now, do stuff!
        printf("[D-%u][T-%u] Worker got trace! (in queue=%lu)\n",_parent->_deviceNum,_workerNum,_parent->_tracesQueue.size());

        if (_parent->_tracesQueue.size() > 0){
            _parent->_tracesQueueLockEvent.unlock(); //make others re-check if work is available
        }
        
        // BEGIN WORKER UPDATE KERNEL PARAMS
        clret = clSetKernelArg(_kernel_computeMeans, 0, sizeof(cl_mem), &myTraceFile->mem());assure(!clret);
        clret = clSetKernelArg(_kernel_computeCensums, 0, sizeof(cl_mem), &myTraceFile->mem());assure(!clret);
        // END WORKER UPDATE KERNEL PARAMS

        //clear memory
        cl_event clearMemoryEvent[4] = {};
                                    
        if (_parent->_prevWaitEvent) {
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsMean, &zero, 1, 0, _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type), 1, &_parent->_prevWaitEvent, &clearMemoryEvent[0]);assure(!clret);
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsCenSum, &zero, 1, 0, _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type), 1, &_parent->_prevWaitEvent, &clearMemoryEvent[1]);assure(!clret);
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsMeanCnt, &zero, 1, 0, _parent->_modelSize * sizeof(cl_ulong), 1, &_parent->_prevWaitEvent, &clearMemoryEvent[2]);assure(!clret);
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsCenSumCnt, &zero, 1, 0, _parent->_modelSize * sizeof(cl_ulong), 1, &_parent->_prevWaitEvent, &clearMemoryEvent[3]);assure(!clret);
            clret = clReleaseEvent(_parent->_prevWaitEvent);assure(!clret); _parent->_prevWaitEvent = NULL;
        }else{
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsMean, &zero, 1, 0, _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type), 0, NULL, &clearMemoryEvent[0]);assure(!clret);
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsCenSum, &zero, 1, 0, _parent->_modelSize * _parent->_point_per_trace * sizeof(gpu_float_type), 0, NULL, &clearMemoryEvent[1]);assure(!clret);
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsMeanCnt, &zero, 1, 0, _parent->_modelSize * sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[2]);assure(!clret);
            clret = clEnqueueFillBuffer(_parent->_command_queue, _curgroupsCenSumCnt, &zero, 1, 0, _parent->_modelSize * sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[3]);assure(!clret);
        }

        //compute mean
        cl_event computeMeanEvent = 0;
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeMeans, 2, NULL, global_work_size, local_work_size, 4, clearMemoryEvent, &computeMeanEvent);assure(!clret);
        for (int i = 0; i<4; i++) {
            clret = clReleaseEvent(clearMemoryEvent[i]);assure(!clret);clearMemoryEvent[i] = NULL;
        }
                
        //compute censum
        cl_event computeCensumEvent = 0;
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_computeCensums, 2, NULL, global_work_size, local_work_size, 1, &computeMeanEvent, &computeCensumEvent);assure(!clret);
        clret = clReleaseEvent(computeMeanEvent);assure(!clret); computeMeanEvent = NULL;

        
        //combine mean and censum
        cl_event combineMeanAndCensumEvent = 0;
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_combineMeansAndCenteredSums, 2, NULL, global_work_size, local_work_size, 1, &computeCensumEvent, &combineMeanAndCensumEvent);assure(!clret);
        clret = clReleaseEvent(computeCensumEvent);assure(!clret); computeCensumEvent = NULL;

        
        //combine mean and censum
        size_t merge_work_size = _parent->_modelSize;
        cl_event mergeCountersEvent = 0;
        clret = clEnqueueNDRangeKernel(_parent->_command_queue, _kernel_mergeCounters, 1, NULL, &merge_work_size, &merge_work_size, 1, &combineMeanAndCensumEvent, &mergeCountersEvent);assure(!clret);
        clret = clReleaseEvent(combineMeanAndCensumEvent);assure(!clret); combineMeanAndCensumEvent = NULL;
        
        //share last event
        clret = clRetainEvent(mergeCountersEvent); assure(!clret);
        _parent->_prevWaitEvent = mergeCountersEvent;
        
        _parent->_tracesQueueLock.unlock(); //release lock for modifying traces queue *AND* _prevWaitEvent (device command_queue)

        //remember my own last event
        _prevEventAccessLock.lock();
        assure_(_myPrevWaitEvent == NULL);
        _myPrevWaitEvent = mergeCountersEvent;
        assure_(_tracefileGPU == NULL);
        _tracefileGPU = myTraceFile; myTraceFile = NULL;
        _prevEventAccessLock.unlock();
    }
}

cl_int GPUPowerModel::ExecutionWorker::relaxedWaitForEvent(cl_uint num_events, const cl_event * event_list){
    cl_int ret = 0;
    struct timespec start_time;
    get_time(&start_time);
    
    struct timespec t;
    double_to_timespec(_kern_avg_run_time*0.9, &t);
    nanosleep(&t, NULL);
    ret = clWaitForEvents(num_events, event_list);

    struct timespec end_time;
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


void GPUPowerModel::ExecutionWorker::waitForMyPrevEventCompletion(){
    cl_int clret = 0;
//    printf("[D-%u][T-%u] Worker waiting for lock!\n",_parent->_deviceNum,_workerNum);
    _prevEventAccessLock.lock();
    if (_myPrevWaitEvent){
        //block thread until done
//        printf("[D-%u][T-%u] Worker waiting for _myPrevWaitEvent!\n",_parent->_deviceNum,_workerNum);
        
#warning clWaitForEventsWorkaround
//        clret = clWaitForEvents(1, &_myPrevWaitEvent); assure(!clret);
        clret = relaxedWaitForEvent(1, &_myPrevWaitEvent);assure(!clret);
        
        clret = clReleaseEvent(_myPrevWaitEvent); assure(!clret);
        _myPrevWaitEvent = NULL;
    }
    if (_tracefileGPU) {
        _tracefileGPU->release();
        _tracefileGPU = NULL;
        //release tracefile memory on device
        clret = clSetKernelArg(_kernel_computeMeans, 0, sizeof(cl_mem), &_curgroupsMean);assure(!clret); //drop reference on _tracefileGPU
        clret = clSetKernelArg(_kernel_computeCensums, 0, sizeof(cl_mem), &_curgroupsMean);assure(!clret); //drop reference on _tracefileGPU
    }
    _prevEventAccessLock.unlock();
//    printf("[D-%u][T-%u] Worker released event and tracefile!\n",_parent->_deviceNum,_workerNum);
}

#pragma mark GPUPowerModel

GPUPowerModel::GPUPowerModel(int deviceNum, cl_device_id device_id, cl_context cntx, std::string kernelsource, uint32_t points_per_trace, std::string modelname, cl_uint modelSize)
: _deviceNum(deviceNum), _deviceMemAlloced(0), _point_per_trace(points_per_trace), _modelSize(modelSize), _modelname(modelname), _context(cntx), _prevWaitEvent(NULL)
{
    cl_int clret = 0;
    uint8_t zero = 0;
    
    printf("[D-%u][M-%s] Device starting modelsize=%u\n",_deviceNum,_modelname.c_str(),_modelSize);

    
    clret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(_deviceWorkgroupSize), &_deviceWorkgroupSize, NULL);assure(!clret);
    printf("[D-%u][M-%s] deviceWorkgroupSize      =%lu\n",_deviceNum,_modelname.c_str(),_deviceWorkgroupSize);
    
    modelname +="(";
    ssize_t modelPos = kernelsource.find(modelname);
    assure_(modelPos != std::string::npos);
    
    kernelsource = kernelsource.replace(modelPos, modelname.size(), "traceSelector(");

    size_t sourceSize = kernelsource.size();
    const char *sourcecode = kernelsource.c_str();
    _program = clCreateProgramWithSource(_context, 1, &sourcecode, &sourceSize, &clret);assure(!clret);

    // Build the program
    clret = clBuildProgram(_program, 1, &device_id, NULL, NULL, NULL);
    if (clret == CL_BUILD_PROGRAM_FAILURE) {
        // Determine the size of the log
        size_t log_size;
        clret = clGetProgramBuildInfo(_program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);assure(!clret);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        clret = clGetProgramBuildInfo(_program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);assure(!clret);

        // Print the log
        printf("%s\n", log);
        free(log);log= NULL;
        assure(0);
    }
    assure(!clret);
    
    _command_queue = clCreateCommandQueue(_context, device_id, 0, &clret);assure(!clret);

    //BEGIN ALLOC WORKER MEMORY
    /*
    std::vector<combtrace> groupsMean{0x100};
    for (auto &cmbtr : groupsMean){
        cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
    }
     
     std::vector<combtrace> groupsCenSum{0x100};
     for (auto &cmbtr : groupsCenSum){
         cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
         cmbtr.cnt = 0;
         memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
     }
    */

    _groupsMean = clCreateBuffer(_context, CL_MEM_READ_WRITE, _modelSize * _point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _groupsMean, &zero, 1, 0, _modelSize * _point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
    _deviceMemAlloced += _modelSize * _point_per_trace * sizeof(gpu_float_type);

            
    _groupsCensum = clCreateBuffer(_context, CL_MEM_READ_WRITE, _modelSize * _point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _groupsCensum, &zero, 1, 0, _modelSize * _point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
    _deviceMemAlloced += _modelSize * _point_per_trace * sizeof(gpu_float_type);


    /*
     cl_ulong groupsMeanCnt[0x100] = {};
     cl_ulong groupsCensumCnt[0x100] = {};
     */
    _groupsMeanCnt = clCreateBuffer(_context, CL_MEM_READ_WRITE, _modelSize * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _groupsMeanCnt, &zero, 1, 0, _modelSize * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
    _deviceMemAlloced += _modelSize * sizeof(cl_ulong);

    _groupsCensumCnt = clCreateBuffer(_context, CL_MEM_READ_WRITE, _modelSize * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _groupsCensumCnt, &zero, 1, 0, _modelSize * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
    _deviceMemAlloced += _modelSize * sizeof(cl_ulong);
    //END ALLOC WORKER MEMORY
    
    clret = clFinish(_command_queue);assure(!clret);
    
    _tracesQueueLockEvent.lock(); //clear event
}

GPUPowerModel::~GPUPowerModel(){
    cl_uint clret = 0;
        
    while (_tracesQueue.size() > 0) {
        printf("[D-%u][M-%s] waiting for traces to get processed (%lu traces left)\n",_deviceNum,_modelname.c_str(),_tracesQueue.size());
        _tracesQueueInProcessLockEvent.lock();
    }

    for (int i=0; i<_ExecutionWorkers.size(); i++) {
//        printf("[D-%u][M-%s] deleting transfer worker (%lu traces left)\n",_deviceNum,_modelname.c_str(),_ExecutionWorkers.size()-i);
        ExecutionWorker *w = _ExecutionWorkers.at(i);
        delete w; w = NULL;
        _ExecutionWorkers.at(i) = NULL;
    }
    
    printf("[D-%u][M-%s] waiting for final event to finish\n",_deviceNum,_modelname.c_str());
    if (_prevWaitEvent) {
        clret = clWaitForEvents(1, &_prevWaitEvent);
        clReleaseEvent(_prevWaitEvent);_prevWaitEvent = NULL;
    }
    
    printf("[D-%u][M-%s] freeing resources\n",_deviceNum,_modelname.c_str());
    safeFreeCustom(_groupsMean,clReleaseMemObject);
    safeFreeCustom(_groupsCensum,clReleaseMemObject);
    safeFreeCustom(_groupsMeanCnt,clReleaseMemObject);
    safeFreeCustom(_groupsCensumCnt,clReleaseMemObject);

    safeFreeCustom(_command_queue,clReleaseCommandQueue);
    safeFreeCustom(_program,clReleaseProgram);
}

void GPUPowerModel::spawnTranferworkers(uint32_t cnt){
    for (int i=0; i<cnt; i++) {
        _ExecutionWorkers.push_back(new ExecutionWorker(this,(int)_ExecutionWorkers.size()));
    }
}


void GPUPowerModel::enqueuTrace(GPUMem *trace){
    trace->retain();
    
    _tracesQueueLock.lock(); //grab lock for modifying queue
    _tracesQueue.push(trace);
    _tracesQueueLockEvent.unlock(); //notify workers of available work
    _tracesQueueLock.unlock();//release lock for modifying queue

    /*
     one worker will get the lock, and pop the element from the queue and the event lock will be locked again
     */
}

cl_ulong GPUPowerModel::getDeviceMemAllocated(){
    return _deviceMemAlloced;
}

std::string GPUPowerModel::getModelName(){
    return _modelname;
}

cl_uint GPUPowerModel::getModelSize(){
    return _modelSize;
}



void GPUPowerModel::getResults(std::vector<combtrace> &retgroupsMean,std::vector<combtrace> &retgroupsCensum){
    cl_uint clret = 0;
    
    printf("[D-%u][M-%s] getting results\n",_deviceNum,_modelname.c_str());
    
    while (_tracesQueue.size() > 0) {
        printf("[D-%u][M-%s] waiting for traces to get processed (%lu traces left)\n",_deviceNum,_modelname.c_str(),_tracesQueue.size());
        _tracesQueueInProcessLockEvent.lock();
    }
    
    printf("[D-%u][M-%s] all workers finished sending final unlock event!\n",_deviceNum,_modelname.c_str());

    //kill all cpu worker threads
    for (auto &w : _ExecutionWorkers){
        w->_stopWorker = true;
    }
    _tracesQueueLockEvent.unlock(); //send work signal without having actual work, kills all workers

    for (ExecutionWorker *w: _ExecutionWorkers){
        w->waitForCompletion();
    }
    //all worker threads are finished now
    
    for (int i=0; i<_modelSize; i++) {
        retgroupsMean.push_back({0,0});
        retgroupsCensum.push_back({0,0});
    }
    
    //alloc memory
    for (auto &cmbtr : retgroupsMean){
        cmbtr.trace = (gpu_float_type*)malloc(_point_per_trace*sizeof(gpu_float_type));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
    }
    for (auto &cmbtr : retgroupsCensum){
        cmbtr.trace = (gpu_float_type*)malloc(_point_per_trace*sizeof(gpu_float_type));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
    }
    
    printf("[D-%u][M-%s] reading model(%d) results\n",_deviceNum,_modelname.c_str(),_modelSize);
    
    for (int i=0; i<_modelSize; i++) {
        clret = clEnqueueReadBuffer(_command_queue, _groupsMean, false, i*_point_per_trace * sizeof(gpu_float_type), _point_per_trace * sizeof(gpu_float_type), retgroupsMean.at(i).trace, 0, NULL, NULL); assure(!clret);
        clret = clEnqueueReadBuffer(_command_queue, _groupsMeanCnt, false, i * sizeof(cl_ulong), sizeof(cl_ulong), &retgroupsMean.at(i).cnt, 0, NULL, NULL); assure(!clret);

        clret = clEnqueueReadBuffer(_command_queue, _groupsCensum, false, i*_point_per_trace * sizeof(gpu_float_type), _point_per_trace * sizeof(gpu_float_type), retgroupsCensum.at(i).trace, 0, NULL, NULL); assure(!clret);
        clret = clEnqueueReadBuffer(_command_queue, _groupsCensumCnt, false, i * sizeof(cl_ulong), sizeof(cl_ulong), &retgroupsCensum.at(i).cnt, 0, NULL, NULL); assure(!clret);
    }
    
    clret = clFinish(_command_queue); assure(!clret);
}
