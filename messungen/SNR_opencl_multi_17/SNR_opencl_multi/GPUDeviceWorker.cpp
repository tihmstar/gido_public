//
//  GPUDeviceWorker.cpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 28.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "GPUDeviceWorker.hpp"

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUDeviceWorker ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUDeviceWorker ASSURE FAILED ON LINE=%d\n",__LINE__); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})

#pragma mark ExecutionWorker

GPUDeviceWorker::GPULoaderWorker::GPULoaderWorker(GPUDeviceWorker *parent, int workerNum)
: _stopWorker(false),_workerNum(workerNum), _parent(parent), _workersWaitingForMemoryCnt(0)
{
    _workerThread = new std::thread([this]{
        worker();
    });
}

GPUDeviceWorker::GPULoaderWorker::~GPULoaderWorker(){
    _workerThread->join();
    delete _workerThread;
    _workerThread = NULL;
}


void GPUDeviceWorker::GPULoaderWorker::worker(){
    cl_int clret = 0;
    CPUMem *tracefile = NULL; //owns reference
    printf("[D-%u][L-%u] Loader started\n",_parent->_deviceWorkerNum,_workerNum);
    while (true) {
        _parent->_tracesQueueLock.lock(); //grab lock for modiying queue
        
        if (_parent->_tracesQueue.size() == 0) {
            _parent->_tracesQueueLock.unlock(); //release lock for modiying queue, before going to wait for event
            
            _parent->_tracesQueueLockEvent.lock(); //wait for event if thre is no
            
            _parent->_tracesQueueLock.lock();  //grab lock and handle event
        }

        
        if (_parent->_tracesQueue.size() == 0){
            printf("[D-%u][L-%u] Loader called but there is no work!\n",_parent->_deviceWorkerNum,_workerNum);
            _parent->_tracesQueueLock.unlock(); //release lock for modiying queue
            
            if (_stopWorker) {
                _parent->_tracesQueueLockEvent.unlock(); //we didn't handle the event, "re-throw" it
                return;
            }
            continue;
        }
        assure_(tracefile = _parent->_tracesQueue.front());
        _parent->_tracesQueue.pop();
        //we have a trace now, do stuff!
        printf("[D-%u][L-%u] Loader got trace! (elements in RAM =%lu) (workers waiting for memory =%u)\n",_parent->_deviceWorkerNum,_workerNum,_parent->_tracesQueue.size(),(uint16_t)_workersWaitingForMemoryCnt);
        _parent->_tracesQueueInProcessLockEvent.unlock(); //send event that we are working on a trace
        _parent->_tracesQueueLock.unlock(); //release lock for modifying traces queue *AND* _prevWaitEvent (device command_queue)

        if (_parent->_tracesQueue.size() > 0) {
            //make sure workers aren't sleeping if there is work to do
            _parent->_tracesQueueLockEvent.unlock();
        }
        
        
        size_t traceFileSize = 0;
        {
            assure(traceFileSize = tracefile->mem()->bufSize());
            
            while (_parent->_GpuFreeMem.fetch_sub(traceFileSize) < traceFileSize + (size_t)100e6) { //leave 100MB free space after trace alloc
                _parent->_GpuFreeMem.fetch_add(traceFileSize);
                printf("[D-%u][L-%u] waiting for mem need=%lu avail=%lu\n",_parent->_deviceWorkerNum,_workerNum,traceFileSize + (size_t)100e6,(size_t)_parent->_GpuFreeMem);
                ++_workersWaitingForMemoryCnt;
                _parent->_GpuFreeMemEventLock.lock();
                --_workersWaitingForMemoryCnt;
            }
            _parent->_GpuFreeMemEventLock.unlock();//make other threads re-check memory status //make sure no threads hangs in wait forever

            
            cl_mem cl_trace = NULL;
            cl_trace = clCreateBuffer(_parent->_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, traceFileSize, (void*)tracefile->mem()->buf(), &clret);assure(!clret);

            tracefile->release(); tracefile = NULL;
            GPUMem gmem(cl_trace);
            
            for (auto &pmodel : _parent->_gpuPowerModels) {
                pmodel->enqueuTrace(&gmem);
            }
            printf("[D-%u][L-%u] Loader enqueued trace!\n",_parent->_deviceWorkerNum,_workerNum);

            
            gmem.release(); //drop my own ref and make destructor block until mem was freed on gpu
            //gmem destructor blocks until free is legal
        }
        _parent->_GpuFreeMem.fetch_add(traceFileSize);
        _parent->_GpuFreeMemEventLock.unlock();


        printf("[D-%u][L-%u] Loader released trace!\n",_parent->_deviceWorkerNum,_workerNum);
    }
}

#pragma mark GPUDeviceWorker

GPUDeviceWorker::GPUDeviceWorker(int deviceWorkerNum, cl_device_id device_id, std::string kernelsource, uint32_t point_per_trace, size_t traceFileSize, std::vector<std::pair<std::string,uint16_t>> models)
: _deviceWorkerNum(deviceWorkerNum), _device_id(device_id), _deviceMemSize(0), _deviceMemAlloced(0), _point_per_trace(point_per_trace)
{
    cl_int clret = 0;
    printf("[D-%d] starting device\n",deviceWorkerNum);

    _context = clCreateContext( NULL, 1, &_device_id, NULL, NULL, &clret);assure(!clret);

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

    cl_ulong deviceWorkgroupSize = 0;
    clret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(deviceWorkgroupSize), &deviceWorkgroupSize, NULL);assure(!clret);
    printf("[D-%d] deviceWorkgroupSize      =%lu\n",_deviceWorkerNum,deviceWorkgroupSize);
    clret = clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(_deviceMemSize), &_deviceMemSize, NULL);assure(!clret);
    printf("[D-%d] total deviceMemSize      =%lu\n",_deviceWorkerNum,_deviceMemSize);

//    for (auto &m : models) {
//        _gpuPowerModels.push_back(new GPUPowerModel{_deviceWorkerNum, _device_id, _context, kernelsource, point_per_trace, m.first, m.second});
//    }

    { /* --- */
        for (auto &m : models) {
            for (int i=0; i<8; i++) {
                std::string newsource = kernelsource;
                std::string findstr = "#define AES_BLOCK_SELECTOR";
                std::string replacestr = "#define AES_BLOCK_SELECTOR ";
                replacestr+= std::to_string(i) + " //";
                
                ssize_t findpos = newsource.find(findstr);
                assure_(findpos != std::string::npos);

                newsource = newsource.replace(findpos, findstr.size(), replacestr);
                
                GPUPowerModel *model = new GPUPowerModel{_deviceWorkerNum, _device_id, _context, newsource, point_per_trace, m.first, m.second};
                model->_modelname = std::to_string(i) + "_" + model->_modelname;
                _gpuPowerModels.push_back(model);
            }
        }
    } /* --- */
    
    
    
    cl_ulong fixedAllocDeviceMem = 256 + 256*4*4 + 15*16; //INV_SBOX, T-Table, aeskey
    
    for (auto &m : _gpuPowerModels) {
        _deviceMemAlloced += m->getDeviceMemAllocated();
    }
    
    _tracesOnGPUCnt = (uint32_t)(_deviceMemSize / (fixedAllocDeviceMem + _deviceMemAlloced + traceFileSize));
    
    if (_tracesOnGPUCnt > 3) {
#warning LIMIT HERE
        _tracesOnGPUCnt = 3;
    }

    _GpuFreeMem = _deviceMemSize - _deviceMemAlloced - fixedAllocDeviceMem*_tracesOnGPUCnt;

    
    if (_tracesOnGPUCnt > 1) //leave some room to breathe
        _tracesOnGPUCnt--;
        
    assure_(_tracesOnGPUCnt > 0);//we need at least one trace and worker
    
    printf("[D-%d] tracesOnGPUCnt           =%u\n",_deviceWorkerNum,_tracesOnGPUCnt);

    _deviceMemAlloced = 0;
    for (auto &m : _gpuPowerModels) {
        m->spawnTranferworkers(_tracesOnGPUCnt);
        _deviceMemAlloced += m->getDeviceMemAllocated();
    }
        
    printf("[D-%d] deviceMemAlloced         =%lu\n",deviceWorkerNum,_deviceMemAlloced+fixedAllocDeviceMem);
    ssize_t freeMemAfterFullStall = _deviceMemSize-(_deviceMemAlloced+fixedAllocDeviceMem);
    printf("[D-%d] free mem after full stall=%lu\n",deviceWorkerNum,freeMemAfterFullStall);
    assure_(freeMemAfterFullStall > 0);//we can't alloc more mem than we have :o

        
    //spawn loader workers
    for (int i=0; i<_tracesOnGPUCnt; i++) {
        _loaderWorkers.push_back(new GPULoaderWorker(this,(int)_loaderWorkers.size()));
    }
    
//    ssize_t freeMemAfterFullStallWithExtraLoader =freeMemAfterFullStall;
//    int extraLoaderSpawned=0;
//    while (freeMemAfterFullStallWithExtraLoader-traceFileSize > traceFileSize) {
//        freeMemAfterFullStallWithExtraLoader -= traceFileSize;
//        extraLoaderSpawned++;
//        _loaderWorkers.push_back(new GPULoaderWorker(this,(int)_loaderWorkers.size()));
//
//#warning LIMIT HERE
//        if (extraLoaderSpawned >= 5) {
//            break;
//        }
//    }
//
//    if (freeMemAfterFullStallWithExtraLoader != freeMemAfterFullStall) {
//        printf("[D-%d] free mem after full stall with %d extraloaders=%lu\n",deviceWorkerNum,extraLoaderSpawned,freeMemAfterFullStallWithExtraLoader);
//    }
}

GPUDeviceWorker::~GPUDeviceWorker(){
    
    while (_tracesQueue.size() > 0) {
        printf("[D-%u] waiting for traces to get loaded (%lu traces left)\n",_deviceWorkerNum,_tracesQueue.size());
        _tracesQueueInProcessLockEvent.lock();
    }

    for (int i=0; i<_loaderWorkers.size(); i++) {
//        printf("[D-%u] deleting loader (%lu traces left)\n",_deviceWorkerNum,_loaderWorkers.size()-i);
        GPULoaderWorker *w = _loaderWorkers.at(i);
        delete w; w = NULL;
        _loaderWorkers.at(i) = NULL;
    }
    

    printf("[D-%u] freeing resources\n",_deviceWorkerNum);

    for (int i=0; i<_gpuPowerModels.size(); i++) {
        GPUPowerModel *pm = _gpuPowerModels.at(i);
        printf("[D-%u] deleting pm=%s\n",_deviceWorkerNum,pm->getModelName().c_str());
        delete pm; pm = NULL;
        _gpuPowerModels.at(i) = NULL;
    }
    
    safeFreeCustom(_context,clReleaseContext);    
}


void GPUDeviceWorker::enqueuTrace(CPUMem *trace){
    trace->retain();
    
    _tracesQueueLock.lock(); //grab lock for modifying queue
    _tracesQueue.push(trace);
    _tracesQueueLockEvent.unlock(); //notify workers of available work
    _tracesQueueLock.unlock();//release lock for modifying queue

    /*
     one worker will get the lock, and pop the element from the queue and the event lock will be locked again
     */
}

void GPUDeviceWorker::getResults(std::map<std::string,CPUModelResults*> &modelresults){
    
    //wait until all transfers are on the gpu
    while (_tracesQueue.size() > 0) {
        printf("[D-%u] waiting for models to get processed (%lu traces left)\n",_deviceWorkerNum,_tracesQueue.size());
        _tracesQueueInProcessLockEvent.lock();
    }
    
    printf("[D-%u] all models processed sending final unlock event!\n",_deviceWorkerNum);
    //kill all loader threads
    for (auto &w : _loaderWorkers) {
        w->_stopWorker = true;
    }
    _tracesQueueLockEvent.unlock(); //send work signal without having actual work, kills all loaders

    //all loader threads are finished now

    for (auto &m : _gpuPowerModels) {
        if (modelresults.find(m->getModelName()) == modelresults.end()) {
            modelresults[m->getModelName()] = new CPUModelResults(m->getModelName(),m->getModelSize(),_point_per_trace);
        }
        std::vector<combtrace> retgroupsMean;
        std::vector<combtrace> retgroupsCensum;
        m->getResults(retgroupsMean, retgroupsCensum);

        modelresults[m->getModelName()]->combineResults(retgroupsMean, retgroupsCensum);
    }
}
