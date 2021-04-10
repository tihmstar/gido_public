//
//  CPUMem.cpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 09.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CPUMem.hpp"

#define assure(cond) do {if (!(cond)) {printf("ERROR: CPUMem ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: CPUMem ASSURE FAILED ON LINE=%d\n",__LINE__); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})


static std::atomic<size_t> gCurCPUAlloc{0};
std::mutex gWaitLock;

CPUMem::CPUMem(Traces *mem, size_t maxCPUAlloc)
: _mem(mem), _refCnt(1)//takes one reference
{
    bool didSleep = false;
    while (gCurCPUAlloc > maxCPUAlloc) {
        didSleep = true;
        gWaitLock.lock(); //wait for free event
    }
    gCurCPUAlloc.fetch_add(_mem->bufSize());
    
    if (didSleep && gCurCPUAlloc < maxCPUAlloc) {
        gWaitLock.unlock(); //wake other threads in case someone was sleeping
    }
}

CPUMem::~CPUMem(){
    while (_refCnt > 0) {
//        printf("[%p] CPUMem::~CPUMem() waiting=%d\n",this,(int)_refCnt);
        _eventlock.lock();
    }
    _userLock.lock();
    assure_(_refCnt == 0); //deallocating alive object
//    printf("[%p] CPUMem::~CPUMem() delete!\n",this);
}

void CPUMem::retain(){
    _userLock.lock();
    assure_(_refCnt.fetch_add(1) != 0); //revived dead object
    
//    printf("[%p] CPUMem::retain() refcnt=%d\n",this,(int)_refCnt);
    _eventlock.unlock();
    _userLock.unlock();
}

void CPUMem::release(){
    _userLock.lock();
    assure_(_refCnt.fetch_sub(1) != 0); //overreleasing dead object
    if (_refCnt == 0) {
        gCurCPUAlloc.fetch_sub(_mem->bufSize());
        delete _mem; _mem = NULL;
        gWaitLock.unlock(); //send mem free event
    }
//    printf("[%p] CPUMem::release() refcnt=%d\n",this,(int)_refCnt);
    _eventlock.unlock();
    _userLock.unlock();
}

uint16_t CPUMem::getRefcnt(){
    return _refCnt;
}


Traces *CPUMem::mem(){
    return _mem;
}
