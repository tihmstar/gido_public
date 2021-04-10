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
#include <string.h>
#include <unistd.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUDevice ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUDevice ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})



GPUDevice::GPUDevice(int deviceNum, cl_device_id device_id, std::string kernelsource, uint32_t point_per_trace, std::vector<std::string> models)
: _deviceNum(deviceNum), _device_id(device_id), _point_per_trace(point_per_trace), _deviceMemSize(0),
    _deviceMemAlloced(0),_GpuFreeMem(0),_activeTraces(0),_processedTraceFiles(0),_workersWaitingForMemoryCnt(0), _workersOnGPUCnt(0)
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
            
            std::string doublestring = "double";
            std::string floatstring = "float";
            while (true){
                ssize_t pos = kernelsource.find(doublestring);
                
                if (pos == std::string::npos) break;
                kernelsource.replace(pos, doublestring.size(), floatstring);
            }
            
            assure_(strstr(kernelsource.c_str(), "double") == 0);
            
            printf("Info: replaced all occurences of \"double\" with \"float\"\n");
        }
    }

    cl_ulong deviceWorkgroupSize = 0;
    clret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(deviceWorkgroupSize), &deviceWorkgroupSize, NULL);assure(!clret);
    printf("[D-%d] deviceWorkgroupSize      =%lu\n",_deviceNum,deviceWorkgroupSize);
    clret = clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(_deviceMemSize), &_deviceMemSize, NULL);assure(!clret);
    printf("[D-%d] total deviceMemSize      =%lu\n",_deviceNum,_deviceMemSize);

    
    for (auto &m : models) {
        _gpuPowerModels.push_back(new GPUPowerModel{this,_deviceNum, _device_id, _context, kernelsource, point_per_trace, m});
    }

    cl_ulong fixedAllocDeviceMem = 256 + 256*4*4 + 15*16; //INV_SBOX, T-Table, aeskey
    
    for (auto &m : _gpuPowerModels) {
        _deviceMemAlloced += m->getDeviceMemAllocated();
    }

    _workersOnGPUCnt = (uint32_t)(_deviceMemSize / (fixedAllocDeviceMem + _deviceMemAlloced + (uint64_t)300e6)); //at least 200MB trace size
    
    //LIMIT here
#define WORKERS_LIMIT 4
    if (_workersOnGPUCnt > WORKERS_LIMIT)
        _workersOnGPUCnt = WORKERS_LIMIT;
        
    assure_(_workersOnGPUCnt > 0);//we need at least one trace and worker

    printf("[D-%d] workersOnGPUCnt           =%u\n",_deviceNum,_workersOnGPUCnt);

    _deviceMemAlloced = 0;
    for (auto &m : _gpuPowerModels) {
        m->spawnExecutionWorker(_workersOnGPUCnt);
        _deviceMemAlloced += m->getDeviceMemAllocated();
    }
    _GpuFreeMem = _deviceMemSize - _deviceMemAlloced;
}

GPUDevice::~GPUDevice(){
    //wait for all traces to be processed and freed
    assure_(_activeTraces == 0);

    printf("[D-%u] freeing resources\n",_deviceNum);

    for (int i=0; i<_gpuPowerModels.size(); i++) {
        GPUPowerModel *pm = _gpuPowerModels.at(i);
        printf("[D-%u] deleting pm=%s\n",_deviceNum,pm->getModelName().c_str());
        delete pm; pm = NULL;
        _gpuPowerModels.at(i) = NULL;
    }
    
    safeFreeCustom(_context,clReleaseContext);
}

void GPUDevice::resetDevice(){
    _processedTraceFiles = 0;
    for (auto &m : _gpuPowerModels) {
        m->clearMemory();
        m->spawnExecutionWorker(_workersOnGPUCnt);
    }
}

void GPUDevice::transferTrace(LoadFile *trace){
    cl_int clret = 0;
    size_t traceFileSize = 0;
    printf("[D-%u] transferTrace!\n",_deviceNum);

    traceFileSize = trace->size();
    
    while (_GpuFreeMem.fetch_sub(traceFileSize) < traceFileSize + (size_t)100e6 || _activeTraces > 15) { //leave 100MB free space after trace alloc
        _GpuFreeMem.fetch_add(traceFileSize);
        printf("[D-%u] waiting for mem need=%lu avail=%lu\n",_deviceNum,traceFileSize + (size_t)100e6,(size_t)_GpuFreeMem);
        ++_workersWaitingForMemoryCnt;
        _GpuFreeMemEventLock.lock();usleep(100);
        --_workersWaitingForMemoryCnt;
    }
    _GpuFreeMemEventLock.unlock();
    
    //upload file to GPU
    cl_mem cl_trace = NULL;
    ++_activeTraces;
    cl_trace = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, traceFileSize, (void*)trace->buf(), &clret);assure(!clret);
    GPUMem *gpu_trace = new GPUMem(cl_trace);
    
    for (auto &m : _gpuPowerModels){
        m->enqueuTrace(gpu_trace);
    }
    
    std::thread cleaner([this,gpu_trace](size_t traceFileSize){
        gpu_trace->release();//drop own reference
        delete gpu_trace; //blocks until everyone is done with gpu_trace
        _GpuFreeMem.fetch_add(traceFileSize);
        printf("[D-%u] Done with %3u traces (%u traces still active)!\n",_deviceNum,_processedTraceFiles.fetch_add(1)+1,(uint32_t)_activeTraces-1);
        --_activeTraces;
        _GpuFreeMemEventLock.unlock();
        _activeTracesEventLock.unlock();
    },traceFileSize);
    cleaner.detach();

}

int GPUDevice::deviceNum(){
    return _deviceNum;
}

void GPUDevice::getResults(std::map<std::string,std::map<std::string,CPUModelResults*>> &modelresults){
    
    while (_activeTraces > 0) {
        printf("[D-%u] waiting for models to get processed (%u traces left)\n",_deviceNum,(uint32_t)_activeTraces);
        _activeTracesEventLock.lock();usleep(100);
    }

    
    //all loader threads are finished now    
    for (auto &m : _gpuPowerModels) {
        for (uint32_t i = 0; i < m->_modelSize; i++) {
            if (modelresults[m->getModelName()].find(std::to_string(i)) == modelresults[m->getModelName()].end()) {
                modelresults[m->getModelName()][std::to_string(i)] = new CPUModelResults(std::to_string(i),_point_per_trace);
            }
        }
        if (modelresults[m->getModelName()].find("trace") == modelresults[m->getModelName()].end()) {
            //special "model" which contains censum of real trace files
            modelresults[m->getModelName()]["trace"] = new CPUModelResults("trace",_point_per_trace);
        }
        
        m->getResults(modelresults[m->getModelName()]);
    }
    

}
