//
//  GPUDevice.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "GPUDevice.hpp"
#include <signal.h>
#include "GPUMem.hpp"
#include <thread>

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUDevice ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUDevice ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})



GPUDevice::GPUDevice(int deviceNum, cl_device_id device_id, std::string kernelsource, uint32_t point_per_trace)
: _deviceNum(deviceNum), _device_id(device_id), _point_per_trace(point_per_trace), _deviceMemSize(0),_deviceWorkgroupSize(0), _GpuFreeMem(0)
,_workersWaitingForMemoryCnt(0), _program(NULL), _activeTraces(0), _processedTraceFiles(0)
{
    cl_int clret = 0;
    printf("[D-%d] starting device\n",_deviceNum);

    _context = clCreateContext(NULL, 1, &_device_id, NULL, NULL, &clret);assure(!clret);

    cl_device_fp_config fpconfig = 0;
    clret = clGetDeviceInfo(_device_id, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(fpconfig), &fpconfig, NULL);assure(!clret);

    if(fpconfig == 0){
        if (sizeof(gpu_float_type) != 4) {
            printf("Error: No double precision support, but compiled with double\n");
            exit(-1);
        }else{
            printf("Warning: No double precision support, but we are using float\n");
        }
    }

    clret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(_deviceWorkgroupSize), &_deviceWorkgroupSize, NULL);assure(!clret);
    printf("[D-%d] deviceWorkgroupSize      =%lu\n",_deviceNum,_deviceWorkgroupSize);
    clret = clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(_deviceMemSize), &_deviceMemSize, NULL);assure(!clret);
    printf("[D-%d] total deviceMemSize      =%lu\n",_deviceNum,_deviceMemSize);
    _GpuFreeMem = _deviceMemSize - 50e6; //50MB buffer
    
    
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
        ::free(log);log= NULL;
        assure(0);
    }
    assure(!clret);

    
}

GPUDevice::~GPUDevice(){

    safeFreeCustom(_context,clReleaseContext);
}

cl_mem GPUDevice::alloc(size_t size, cl_mem_flags flags){
    cl_int clret = 0;
    while (_GpuFreeMem.fetch_sub(size) < size + (size_t)50e6) { //leave 50MB free space after alloc
        _GpuFreeMem.fetch_add(size);
        printf("[D-%u] waiting for mem need=%lu avail=%lu\n",_deviceNum,size + (size_t)50e6,(size_t)_GpuFreeMem);
        ++_workersWaitingForMemoryCnt;
        _GpuFreeMemEventLock.lock();
        --_workersWaitingForMemoryCnt;
    }
    _GpuFreeMemEventLock.unlock();
    cl_mem gpumem = NULL;
    gpumem = clCreateBuffer(_context, flags, size, NULL, &clret);assure(!clret);
    return gpumem;
}

void GPUDevice::free(cl_mem mem, size_t size){
    cl_int clret = 0;
    clret = clRetainMemObject(mem);assure(!clret);
    _GpuFreeMem.fetch_add(size);
    _GpuFreeMemEventLock.unlock();
}


GPUMem *GPUDevice::transferTrace(LoadFile *trace){
    cl_int clret = 0;
    size_t traceFileSize = 0;
    printf("[D-%u] transferTrace!\n",_deviceNum);

    traceFileSize = trace->size();

    while (_GpuFreeMem.fetch_sub(traceFileSize) < traceFileSize + (size_t)50e6) { //leave 50MB free space after trace alloc
        _GpuFreeMem.fetch_add(traceFileSize);
        printf("[D-%u] waiting for mem need=%lu avail=%lu\n",_deviceNum,traceFileSize + (size_t)50e6,(size_t)_GpuFreeMem);
        ++_workersWaitingForMemoryCnt;
        _GpuFreeMemEventLock.lock();
        --_workersWaitingForMemoryCnt;
    }

    //upload file to GPU
    cl_mem cl_trace = NULL;
    ++_activeTraces;
    cl_trace = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, traceFileSize, (void*)trace->buf(), &clret);assure(!clret);
    GPUMem *gpu_trace = new GPUMem(cl_trace);

    std::thread cleaner([this](size_t traceFileSize, GPUMem *gpu_trace){
        delete gpu_trace; //blocks until everyone is done with gpu_trace
        _GpuFreeMem.fetch_add(traceFileSize);
        printf("[D-%u] Done with %3u traces (%u traces still active)!\n",_deviceNum,_processedTraceFiles.fetch_add(1)+1,(uint32_t)_activeTraces-1);
        --_activeTraces;
        _GpuFreeMemEventLock.unlock();
        _activeTracesEventLock.unlock();
    },traceFileSize, gpu_trace);
    cleaner.detach();
    return gpu_trace;
}

//
//int GPUDevice::deviceNum(){
//    return _deviceNum;
//}
//
//void GPUDevice::getResults(std::map<std::string,CPUModelResults*> &modelresults){
//    while (_activeTraces > 0) {
//        printf("[D-%u] waiting for models to get processed (%u traces left)\n",_deviceNum,(uint32_t)_activeTraces);
//        _activeTracesEventLock.lock();
//    }
//
//
//    //all loader threads are finished now
//
//    for (auto &m : _gpuPowerModels) {
//        if (modelresults.find(m->getModelName()) == modelresults.end()) {
//            modelresults[m->getModelName()] = new CPUModelResults(m->getModelName(),_point_per_trace);
//        }
//        m->getResults(modelresults.at(m->getModelName()));
//    }
//}
