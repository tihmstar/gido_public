//
//  GPUPowerModel.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "GPUPowerModel.hpp"
#include <signal.h>
#include "GPUPowerModelExecutionWorker.hpp"
#include "numerics.hpp"
#include <string.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUPowerModel ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUPowerModel ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})


GPUPowerModel::GPUPowerModel(GPUDevice *parent, int deviceNum, cl_device_id device_id, cl_context context, std::string kernelsource, uint32_t point_per_trace, std::string modelname)
: _parent(parent), _deviceNum(deviceNum), _device_id(device_id), _context(context), _point_per_trace(point_per_trace), _modelname(modelname), _deviceWorkgroupSize(0), _program(NULL), _command_queue(NULL)
,_deviceMemAlloced(0)

,_prevWaitEvents{}

,_gDeviceMeanTrace(NULL)
,_gDeviceCensumTrace(NULL)
,_gDeviceMeanTraceCnt(NULL)
,_gDeviceCensumTraceCnt(NULL)
,_gDeviceHypotMeans(NULL)
,_gDeviceHypotCensums(NULL)
,_gDeviceHypotMeansCnt(NULL)
,_gDeviceHypotCensumsCnt(NULL)
,_gDeviceUpperPart(NULL)
{
    cl_int clret = 0;
    
    printf("[D-%u][M-%s] Device starting\n",_deviceNum,_modelname.c_str());

    clret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(_deviceWorkgroupSize), &_deviceWorkgroupSize, NULL);assure(!clret);
    printf("[D-%u][M-%s] deviceWorkgroupSize      =%lu\n",_deviceNum,_modelname.c_str(),_deviceWorkgroupSize);
    
    int keybyte = -1;
    ssize_t keyByteStrPos = modelname.rfind("_");
    keybyte = atoi(modelname.substr(keyByteStrPos+1).c_str());
    
    modelname = modelname.substr(0,keyByteStrPos);
    assure_(keybyte != -1);
    
    {
        std::string oldKeySelector = "#define KEY_SELECTOR";
        ssize_t oldKeySelectorPos = kernelsource.find(oldKeySelector);
        std::string newKeySelector = "#define KEY_SELECTOR ";
        newKeySelector += std::to_string(keybyte);
        newKeySelector += " //";
        
        kernelsource.replace(oldKeySelectorPos, oldKeySelector.size(), newKeySelector);
    }
    
    
    modelname +="(";
    while (true){
        std::string replModelName = modelname;
        ssize_t modelPos = kernelsource.find(replModelName);
        
        if (modelPos == std::string::npos) break;
        kernelsource.replace(modelPos, replModelName.size(), "powerModel(");
    }
    
    
    

        
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

    /* --- BEGIN DeviceGlobalMemory --- */
    /*
     gpu_float_type gmean[point_per_trace];
     gpu_float_type gcensum[point_per_trace];
    */

    _gDeviceMeanTrace = clCreateBuffer(context, CL_MEM_READ_WRITE, point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += point_per_trace * sizeof(gpu_float_type);

            
    _gDeviceCensumTrace = clCreateBuffer(context, CL_MEM_READ_WRITE, point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += point_per_trace * sizeof(gpu_float_type);

    /*
     uint64_t gcntMean = 0;
     uint64_t gcntcensum = 0;
     */
    _gDeviceMeanTraceCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _deviceMemAlloced += sizeof(cl_ulong);

    _gDeviceCensumTraceCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _deviceMemAlloced += sizeof(cl_ulong);

    /*
    gpu_float_type ghypotMeanGuessVals[_modelSize] = {};
    gpu_float_type ghypotCensumGuessVals[_modelSize] = {};
    */

    _gDeviceHypotMeans = clCreateBuffer(context, CL_MEM_READ_WRITE, _modelSize * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += _modelSize * sizeof(gpu_float_type);

    _gDeviceHypotCensums = clCreateBuffer(context, CL_MEM_READ_WRITE, _modelSize * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += _modelSize * sizeof(gpu_float_type);

    clret = clFinish(_command_queue); assure(!clret);

    /*
     uint64_t ghypotMeanGuessCnt[_modelSize] = {};
     uint64_t ghypotCensumGuessCnt[_modelSize] = {};
     */
    _gDeviceHypotMeansCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, _modelSize * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _deviceMemAlloced += _modelSize * sizeof(cl_ulong);
    clret = clFinish(_command_queue); assure(!clret);

    _gDeviceHypotCensumsCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, _modelSize * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _deviceMemAlloced += _modelSize * sizeof(cl_ulong);
    clret = clFinish(_command_queue); assure(!clret);

    /*
     numberedTrace *gupperPart = (numberedTrace *)malloc(_modelSize*sizeof(numberedTrace));
     for (int k = 0; k<_modelSize; k++) {
         gupperPart[k].trace = (gpu_float_type*)malloc(point_per_trace * sizeof(gpu_float_type));
         memset(gupperPart[k].trace, 0, point_per_trace * sizeof(gpu_float_type));
     }
     */
    _gDeviceUpperPart = clCreateBuffer(context, CL_MEM_READ_WRITE, _modelSize * point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += _modelSize * point_per_trace * sizeof(gpu_float_type);
    clret = clFinish(_command_queue); assure(!clret);

    clearMemory();
}

GPUPowerModel::~GPUPowerModel(){
       
    while (_tracesQueue.size() > 0) {
        printf("[D-%u][M-%s] waiting for traces to get processed (%lu traces left)\n",_deviceNum,_modelname.c_str(),_tracesQueue.size());
        _tracesQueueInProcessLockEvent.lock();
    }
    
    for (int i=0; i<_ExecutionWorkers.size(); i++) {
//        printf("[D-%u][M-%s] deleting transfer worker (%lu traces left)\n",_deviceNum,_modelname.c_str(),_ExecutionWorkers.size()-i);
        GPUPowerModelExecutionWorker *w = _ExecutionWorkers.at(i);
        delete w; w = NULL;
        _ExecutionWorkers.at(i) = NULL;
    }
    
    for (int i=0; i<prevWaitEventsCnt; i++) {
        assure_(_prevWaitEvents[i] == NULL);
    }

    printf("[D-%u][M-%s] freeing resources\n",_deviceNum,_modelname.c_str());

    safeFreeCustom(_gDeviceMeanTrace, clReleaseMemObject);
    safeFreeCustom(_gDeviceCensumTrace, clReleaseMemObject);
    safeFreeCustom(_gDeviceMeanTraceCnt, clReleaseMemObject);
    safeFreeCustom(_gDeviceCensumTraceCnt, clReleaseMemObject);
    safeFreeCustom(_gDeviceHypotMeans, clReleaseMemObject);
    safeFreeCustom(_gDeviceHypotCensums, clReleaseMemObject);
    safeFreeCustom(_gDeviceHypotMeansCnt, clReleaseMemObject);
    safeFreeCustom(_gDeviceHypotCensumsCnt, clReleaseMemObject);
    safeFreeCustom(_gDeviceUpperPart, clReleaseMemObject);
    
    safeFreeCustom(_command_queue,clReleaseCommandQueue);
    safeFreeCustom(_program,clReleaseProgram);
}

void GPUPowerModel::clearMemory(){
    cl_int clret = 0;
    uint8_t zero = 0;

    printf("[D-%u][M-%s] clearing model memory\n",_deviceNum,_modelname.c_str());

    
    /* --- BEGIN DeviceGlobalMemory --- */
    /*
     gpu_float_type gmean[point_per_trace];
     gpu_float_type gcensum[point_per_trace];
    */
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceMeanTrace, &zero, 1, 0, _point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceCensumTrace, &zero, 1, 0, _point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);


    /*
     uint64_t gcntMean = 0;
     uint64_t gcntcensum = 0;
     */
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceMeanTraceCnt, &zero, 1, 0, sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceCensumTraceCnt, &zero, 1, 0, sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);


    /*
    gpu_float_type ghypotMeanGuessVals[_modelSize] = {};
    gpu_float_type ghypotCensumGuessVals[_modelSize] = {};
    */
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotMeans, &zero, 1, 0, _modelSize * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotCensums, &zero, 1, 0, _modelSize * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);


    /*
     uint64_t ghypotMeanGuessCnt[_modelSize] = {};
     uint64_t ghypotCensumGuessCnt[_modelSize] = {};
     */
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotMeansCnt, &zero, 1, 0, _modelSize * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotCensumsCnt, &zero, 1, 0, _modelSize * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);


    /*
     numberedTrace *gupperPart = (numberedTrace *)malloc(_modelSize*sizeof(numberedTrace));
     for (int k = 0; k<_modelSize; k++) {
         gupperPart[k].trace = (gpu_float_type*)malloc(point_per_trace * sizeof(gpu_float_type));
         memset(gupperPart[k].trace, 0, point_per_trace * sizeof(gpu_float_type));
     }
     */

    for (uint32_t i=0; i<_modelSize; i++) {
        clret = clEnqueueFillBuffer(_command_queue, _gDeviceUpperPart, &zero, 1, i * _point_per_trace * sizeof(gpu_float_type), _point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
        
        if (i % 0x100 == 0) {
            clret = clFinish(_command_queue);
            if (clret){
                printf("failed clearing upper part for key=0x%04x\n",i);
                assure(!clret);
            }
        }
    }

    clret = clFinish(_command_queue); assure(!clret);
}


void GPUPowerModel::spawnExecutionWorker(uint32_t cnt){
    cl_int clret = 0;
    for (int i=0; i<cnt; i++) {
        _ExecutionWorkers.push_back(new GPUPowerModelExecutionWorker(this,(int)_ExecutionWorkers.size()));
    }
    clret = clFinish(_command_queue); assure(!clret);
}


cl_ulong GPUPowerModel::getDeviceMemAllocated(){
    return _deviceMemAlloced;
}

std::string GPUPowerModel::getModelName(){
    return _modelname;
}

void GPUPowerModel::enqueuTrace(GPUMem *trace){
    trace->retain();

//    printf("[D-%u][M-%s] enqueueTrace\n",_deviceNum,_modelname.c_str());

    _tracesQueueLock.lock(); //grab lock for modifying queue
    _tracesQueue.push(trace);
    _tracesQueueLockEvent.unlock(); //notify workers of available work
    _tracesQueueLock.unlock();//release lock for modifying queue

    /*
     one worker will get the lock, and pop the element from the queue and the event lock will be locked again
     */
}

void GPUPowerModel::getResults(std::map<std::string,CPUModelResults*> &modelresults, bool isMidResult){
    cl_uint clret = 0;
    
    printf("[D-%u][M-%s] getting results\n",_deviceNum,_modelname.c_str());
    
    if (!isMidResult) {
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

        for (GPUPowerModelExecutionWorker *w: _ExecutionWorkers){
            w->waitForCompletion();
        }
        //all worker threads are finished now
        printf("[D-%u][M-%s] waiting for final event to finish\n",_deviceNum,_modelname.c_str());
        _tracesQueueLock.lock();
        if (_prevWaitEvents[0]) {
            clret = clWaitForEvents(prevWaitEventsCnt, _prevWaitEvents);
            for (int i=0; i<prevWaitEventsCnt; i++) {
                clReleaseEvent(_prevWaitEvents[i]);_prevWaitEvents[i] = NULL;
            }
        }
    }else{
        _tracesQueueLock.lock(); //grab a lock while we are getting midResult
    }
    

    combtrace curmean;
    combtrace curcensum;
    curmean.trace = (gpu_float_type*)malloc(_point_per_trace * sizeof(gpu_float_type));
    curcensum.trace = (gpu_float_type*)malloc(_point_per_trace * sizeof(gpu_float_type));

    printf("[D-%u][M-%s] reading model results\n",_deviceNum,_modelname.c_str());

    clret = clEnqueueReadBuffer(_command_queue, _gDeviceMeanTrace, false, 0, _point_per_trace * sizeof(gpu_float_type), curmean.trace, 0, NULL, NULL); assure(!clret);
    clret = clEnqueueReadBuffer(_command_queue, _gDeviceMeanTraceCnt, false, 0, sizeof(cl_ulong), &curmean.cnt, 0, NULL, NULL); assure(!clret);
        
    clret = clEnqueueReadBuffer(_command_queue, _gDeviceCensumTrace, false, 0, _point_per_trace * sizeof(gpu_float_type), curcensum.trace, 0, NULL, NULL); assure(!clret);
    clret = clEnqueueReadBuffer(_command_queue, _gDeviceCensumTraceCnt, false, 0, sizeof(cl_ulong), &curcensum.cnt, 0, NULL, NULL); assure(!clret);

    clret = clFinish(_command_queue); assure(!clret);

    printf("[D-%u][M-%s] read trace mean/censum\n",_deviceNum,_modelname.c_str());

    auto &traceout = modelresults.at("trace");
    
    combineMeans(traceout->_meanTrace, curmean, _point_per_trace);
    combineCenteredSum(traceout->_censumTrace, traceout->_meanTrace, curcensum, curmean, _point_per_trace);
    printf("[D-%u][M-%s] combined trace mean/censum\n",_deviceNum,_modelname.c_str());

    
    for (uint32_t z=0; z<_modelSize; z++) {
        auto &out = modelresults.at(std::to_string(z));

        clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotMeans, false, z * sizeof(gpu_float_type), sizeof(gpu_float_type), curmean.trace, 0, NULL, NULL); assure(!clret);
        clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotMeansCnt, false, z * sizeof(cl_ulong), sizeof(cl_ulong), &curmean.cnt, 0, NULL, NULL); assure(!clret);

        clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotCensums, false, z * sizeof(gpu_float_type), sizeof(gpu_float_type), curcensum.trace, 0, NULL, NULL); assure(!clret);
        clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotCensumsCnt, false, z * sizeof(cl_ulong), sizeof(cl_ulong), &curcensum.cnt, 0, NULL, NULL); assure(!clret);

        clret = clFinish(_command_queue); assure(!clret);
        
        printf("[D-%u][M-%s][Key=0x%04x] read hypot mean/censum\n",_deviceNum,_modelname.c_str(),z);

        combineMeans(out->_meanHypot, curmean, 1);
        combineCenteredSum(out->_censumHypot, out->_meanTrace, curcensum, curmean, 1);

        printf("[D-%u][M-%s][Key=0x%04x] combined hypot mean/censum\n",_deviceNum,_modelname.c_str(),z);

        clret = clEnqueueReadBuffer(_command_queue, _gDeviceUpperPart, false, z * _point_per_trace * sizeof(gpu_float_type), _point_per_trace * sizeof(gpu_float_type), curmean.trace, 0, NULL, NULL); assure(!clret);
        clret = clFinish(_command_queue); assure(!clret);

        printf("[D-%u][M-%s][Key=0x%04x] read upper\n",_deviceNum,_modelname.c_str(),z);

        for (int i=0; i<_point_per_trace; i++) {
            out->_upperPart.trace[i] += curmean.trace[i];
        }
        
        printf("[D-%u][M-%s][Key=0x%04x] combined upper\n",_deviceNum,_modelname.c_str(),z);
    }
    
    
    //if we aquired this lock because of midResult, release it now and let workers continue.
    //otherwise this has no effect
    _tracesQueueLock.unlock();
}
