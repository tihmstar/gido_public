//
//  CPULoader.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CPULoader.hpp"
#include <unistd.h>

CPULoader::CPULoader(const std::vector<std::tuple<int,int,std::string>> &toLoadList, uint16_t maxLoaderThreads)
: _toLoadList{toLoadList}, _runningWorkersCnt(0), _filesCnt(0)
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
        auto loadFilename = _toLoadList.back();
        _toLoadList.pop_back();
        printf("[L-%02d] (%3lu of %3lu) loading=%s\n",loaderNum,_filesCnt-_toLoadList.size(),_filesCnt,std::get<2>(loadFilename).c_str());
        _listLock.unlock();

        ResArray *lf = new ResArray(std::get<2>(loadFilename).c_str());
        
        _queueLock.lock();
        _loadedQueue.push({std::get<0>(loadFilename),std::get<1>(loadFilename),lf});
        _queueAppendEventLock.unlock();
        _queueLock.unlock();
    }
   
    --_runningWorkersCnt;
    _queueAppendEventLock.unlock(); //make sure dequeue will never deadlock
    printf("[L-%02d] loader finished! (%2d loaders still running)\n",loaderNum,(uint16_t)_runningWorkersCnt);
}


std::tuple<int,int,ResArray *> CPULoader::dequeue(){
    std::tuple<int,int,ResArray *>ret{0,0,NULL};
    while (true){
        _queueLock.lock();
        if (_loadedQueue.size()){
            ret = _loadedQueue.front();
            _loadedQueue.pop();

        }
        _queueLock.unlock();

        if (std::get<2>(ret)) //we already have a result, return now
            break;
        
        if (_runningWorkersCnt == 0)
            break; // there is no more work coming at us, bail out
                //if we get through here without having more stuff in the queue, we purposely return NULL

        //there might be more file coming, so lets hang here and wait
        _queueAppendEventLock.lock(); //wait
    }

    _queueAppendEventLock.unlock();// let other dequeue threads check for work
    return ret;
}
