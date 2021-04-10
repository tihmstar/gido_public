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


GPUPowerModel::GPUPowerModel(GPUDevice *parent, int deviceNum, cl_device_id device_id, cl_context context, std::string kernelsource, uint32_t point_per_trace, std::string modelname, int8_t bytepos)
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
    
    #warning DEBUG
    {
        if (bytepos != -1){
            while (true){
                ssize_t bp = kernelsource.find("#define BYTE_SELECTOR");
                if (bp == std::string::npos) break;
                kernelsource.replace(bp, strlen("#define BYTE_SELECTOR"), "//BYTE_SELECTOR");
            }

            while (true){
                ssize_t bp = kernelsource.find("BYTE_SELECTOR");
                if (bp == std::string::npos) break;
                kernelsource.replace(bp, strlen("BYTE_SELECTOR"), std::to_string(bytepos));
            }
        }
    }
    
    
    modelname +="(";
    std::string replModelName = modelname;
    ssize_t modelPos = kernelsource.find(replModelName);

    if (modelPos == std::string::npos){
        ssize_t roundPos = modelname.find("_ROUND");
        if (roundPos != std::string::npos) {
            std::string substr = modelname.substr(roundPos+strlen("_ROUND"));
            ssize_t dashpos = substr.find("_");
            assure_(dashpos != std::string::npos);
            std::string num = substr.substr(0,dashpos);
            
            int roundNum = atoi(num.c_str());
            assure_(roundNum > 0 && roundNum < 15);
            
            ssize_t selDef = kernelsource.find("#define G_SELECTOR_ROUND");
            assure_(selDef != std::string::npos);
            ssize_t newline = kernelsource.substr(selDef).find("\n");
            assure_(newline != std::string::npos);
            
            std::string replStr = kernelsource.substr(selDef, newline);
            
            std::string newStr{"#define G_SELECTOR_ROUND "};
            newStr += num;
            
            kernelsource.replace(selDef, replStr.size(), newStr);
            printf("[D-%u][M-%s] Replaced round selector (%s -> %s) according to model name!\n",_deviceNum,_modelname.c_str(),replStr.c_str(),newStr.c_str());
            
            std::string modelNameRoundReplacer{"_ROUND"};
            modelNameRoundReplacer+=num;
            modelNameRoundReplacer+="_";
            
            ssize_t modelNameRoundReplacerPos = modelname.find(modelNameRoundReplacer);
            assure_(modelNameRoundReplacerPos != std::string::npos);
            
            replModelName.replace(modelNameRoundReplacerPos, modelNameRoundReplacer.size(), "_ROUND_");
            modelPos = kernelsource.find(replModelName);
        }
    }
    
    assure_(modelPos != std::string::npos);
    kernelsource.replace(modelPos, replModelName.size(), "powerModel(");

        
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
    gpu_float_type ghypotMeanGuessVals[1] = {};
    gpu_float_type ghypotCensumGuessVals[1] = {};
    */

    _gDeviceHypotMeans = clCreateBuffer(context, CL_MEM_READ_WRITE, 1 * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += 1 * sizeof(gpu_float_type);

    _gDeviceHypotCensums = clCreateBuffer(context, CL_MEM_READ_WRITE, 1 * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += 1 * sizeof(gpu_float_type);


    /*
     uint64_t ghypotMeanGuessCnt[1] = {};
     uint64_t ghypotCensumGuessCnt[1] = {};
     */
    _gDeviceHypotMeansCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 1 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _deviceMemAlloced += 1 * sizeof(cl_ulong);

    _gDeviceHypotCensumsCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 1 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
    _deviceMemAlloced += 1 * sizeof(cl_ulong);
    
    /*
     numberedTrace *gupperPart = (numberedTrace *)malloc(1*sizeof(numberedTrace));
     for (int k = 0; k<1; k++) {
         gupperPart[k].trace = (gpu_float_type*)malloc(point_per_trace * sizeof(gpu_float_type));
         memset(gupperPart[k].trace, 0, point_per_trace * sizeof(gpu_float_type));
     }
     */
    _gDeviceUpperPart = clCreateBuffer(context, CL_MEM_READ_WRITE, 1 * point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
    _deviceMemAlloced += 1 * point_per_trace * sizeof(gpu_float_type);

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
    gpu_float_type ghypotMeanGuessVals[1] = {};
    gpu_float_type ghypotCensumGuessVals[1] = {};
    */
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotMeans, &zero, 1, 0, 1 * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotCensums, &zero, 1, 0, 1 * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);


    /*
     uint64_t ghypotMeanGuessCnt[1] = {};
     uint64_t ghypotCensumGuessCnt[1] = {};
     */
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotMeansCnt, &zero, 1, 0, 1 * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceHypotCensumsCnt, &zero, 1, 0, 1 * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);


    /*
     numberedTrace *gupperPart = (numberedTrace *)malloc(1*sizeof(numberedTrace));
     for (int k = 0; k<1; k++) {
         gupperPart[k].trace = (gpu_float_type*)malloc(point_per_trace * sizeof(gpu_float_type));
         memset(gupperPart[k].trace, 0, point_per_trace * sizeof(gpu_float_type));
     }
     */
    clret = clEnqueueFillBuffer(_command_queue, _gDeviceUpperPart, &zero, 1, 0, 1 * _point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);

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

void GPUPowerModel::getResults(CPUModelResults *out){
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

    combineMeans(out->_meanTrace, curmean, _point_per_trace);
    combineCenteredSum(out->_censumTrace, out->_meanTrace, curcensum, curmean, _point_per_trace);

    printf("[D-%u][M-%s] combined trace mean/censum\n",_deviceNum,_modelname.c_str());

    
    clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotMeans, false, 0, sizeof(gpu_float_type), curmean.trace, 0, NULL, NULL); assure(!clret);
    clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotMeansCnt, false, 0, sizeof(cl_ulong), &curmean.cnt, 0, NULL, NULL); assure(!clret);

    clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotCensums, false, 0, sizeof(gpu_float_type), curcensum.trace, 0, NULL, NULL); assure(!clret);
    clret = clEnqueueReadBuffer(_command_queue, _gDeviceHypotCensumsCnt, false, 0, sizeof(cl_ulong), &curcensum.cnt, 0, NULL, NULL); assure(!clret);

    clret = clFinish(_command_queue); assure(!clret);
    
    printf("[D-%u][M-%s] read hypot mean/censum\n",_deviceNum,_modelname.c_str());

    combineMeans(out->_meanHypot, curmean, 1);
    combineCenteredSum(out->_censumHypot, out->_meanTrace, curcensum, curmean, 1);
    
    printf("[D-%u][M-%s] combined hypot mean/censum\n",_deviceNum,_modelname.c_str());

    
    clret = clEnqueueReadBuffer(_command_queue, _gDeviceUpperPart, false, 0, _point_per_trace * sizeof(gpu_float_type), curmean.trace, 0, NULL, NULL); assure(!clret);
    clret = clFinish(_command_queue); assure(!clret);
    
    printf("[D-%u][M-%s] read upper\n",_deviceNum,_modelname.c_str());

    
    for (int i=0; i<_point_per_trace; i++) {
        out->_upperPart.trace[i] += curmean.trace[i];
    }
    
    printf("[D-%u][M-%s] combined upper\n",_deviceNum,_modelname.c_str());

}
