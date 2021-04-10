//
//  CPULoader.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CPULoader.hpp"
#include <unistd.h>

CPULoader::CPULoader(const std::vector<std::string> &toLoadList, size_t maxRamAlloc, uint16_t maxLoaderThreads, size_t minAllocSize)
: _toLoadList{toLoadList}, _runningWorkersCnt(0), _minAllocSize(minAllocSize), _filesCnt(0), _maxRamAlloc(maxRamAlloc)
{
    if (!maxLoaderThreads) {
        maxLoaderThreads = sysconf(_SC_NPROCESSORS_ONLN);
    }
    
    _filesCnt = toLoadList.size();
    
    printf("CPULoader::CPULoader spinning up %u loaderThreads\n",maxLoaderThreads);
    for (int i=0; i<maxLoaderThreads; i++) {
        _workers.push_back(new std::thread([this](int i)->void{
            worker(i);
        },i));
    }
}

CPULoader::~CPULoader(){
    while (_workers.size()) {
        printf("CPULoader::~CPULoader waiting for %2lu workers to finish\n",_workers.size());
        std::thread *w =_workers.back();
        _workers.pop_back();
        w->join();
        delete w; w = NULL;
    }
}


void CPULoader::worker(int loaderNum){
    ++_runningWorkersCnt;
    printf("[L-%02d] loader started!\n",loaderNum);
    while (_toLoadList.size()) {
        _listLock.lock();
        if (_toLoadList.size() == 0) {
            printf("[L-%02d] loader out of work!\n",loaderNum);
            _listLock.unlock();
            break;
        }
        std::string loadFilename = _toLoadList.back();
        _toLoadList.pop_back();
        printf("[L-%02d] (%3lu of %3lu) loading=%s\n",loaderNum,_filesCnt-_toLoadList.size(),_filesCnt,loadFilename.c_str());
        _listLock.unlock();

        LoadFile *lf = new LoadFile(loadFilename.c_str(),_minAllocSize, false);
        
        _maxRamModLock.lock(); //grab lock for read/write
        while (lf->size() > _maxRamAlloc) {//access happens while we have lock
            _maxRamModLock.unlock();//release lock befor hang

            printf("[L-%02d] hanging until free event\n",loaderNum);
            _maxRamModEventLock.lock(); //hang here
            
            _maxRamModLock.lock();//re-grab lock before re-read
        }
        _maxRamAlloc -= lf->size();
        
        _maxRamModLock.unlock(); //release lock
        
        lf->load();
        
        _queueLock.lock();
        _loadedQueue.push(lf);
        _queueAppendEventLock.unlock();
        _queueLock.unlock();
    }
   
    --_runningWorkersCnt;
    _queueAppendEventLock.unlock(); //make sure dequeue will never deadlock
    _maxRamModLock.unlock(); //make sure dequeue will never deadlock
    printf("[L-%02d] loader finished! (%2d loaders still running)\n",loaderNum,(uint16_t)_runningWorkersCnt);
}


LoadFile *CPULoader::dequeue(){
    LoadFile *ret = NULL;
    while (true){
        _queueLock.lock();
        if (_loadedQueue.size()){
            ret = _loadedQueue.front();
            _loadedQueue.pop();

            _maxRamModLock.lock(); //grab lock for read/write
            _maxRamAlloc += ret->size();
            _maxRamModEventLock.unlock(); //send event
            _maxRamModLock.unlock(); //relase lock for read/write
        }
        _queueLock.unlock();

        if (ret) //we already have a result, return now
            break;
        
        if (_runningWorkersCnt == 0 && _toLoadList.size() == 0)
            break; // there is no more work coming at us, bail out
                //if we get through here without having more stuff in the queue, we purposely return NULL

        //there might be more file coming, so lets hang here and wait
        _queueAppendEventLock.lock(); //wait
    }

    _maxRamModLock.unlock(); // let other dequeue threads check for work
    _queueAppendEventLock.unlock();// let other dequeue threads check for work
    return ret;
}
