//
//  GPUMem.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 16.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "GPUMem.hpp"
#include <signal.h>
#include <unistd.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: GPUMem ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: GPUMem ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); raise(SIGABRT); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})


GPUMem::GPUMem(cl_mem mem)
: _mem(mem), _refCnt(1)//takes one reference
{
    //
}

GPUMem::~GPUMem(){
    while (_refCnt > 0) {
        _eventlock.lock();usleep(100);
    }
    _userLock.lock();usleep(100);
    assure_(_refCnt == 0); //deallocating alive object
    
}

void GPUMem::retain(){
    _userLock.lock();usleep(100);
    assure_(_refCnt.fetch_add(1) != 0); //revived dead object
    
    _eventlock.unlock();
    _userLock.unlock();
}

void GPUMem::release(){
    _userLock.lock();usleep(100);
    cl_uint clret = 0;
    assure_(_refCnt.fetch_sub(1) != 0); //overreleasing dead object
    if (_refCnt == 0) {
        assure_(_mem);
        clret = clReleaseMemObject(_mem);assure(!clret);
        _mem = NULL;
    }
    
    _eventlock.unlock();
    _userLock.unlock();
}

cl_mem &GPUMem::mem(){
    assure_(_refCnt != 0); //operating on dead object
    return _mem;
}


