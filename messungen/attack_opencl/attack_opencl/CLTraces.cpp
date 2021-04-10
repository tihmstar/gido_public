//
//  CLTrace.cpp
//  attack_opencl
//
//  Created by tihmstar on 05.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CLTraces.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN CLTraces.cpp ON LINE=%d\n",__LINE__); exit(-1);}}while (0)

CLTraces::CLTraces(const char *path, cl_context context, cl_device_id device)
:_context(context)
{
    cl_int clret = 0;
    struct stat sb;
    
    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    
    _fsize = sb.st_size;
    
#ifndef MCL_CURRENT
#define MCL_CURRENT 0//sometimes this is not available *cought WINDOWS cought*
#endif
    //mmap, but don't map the file
    assure((_fmem = (int8_t*)mmap(NULL, _fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MCL_CURRENT, -1, 0)) != (void*)-1);
    mlock(_fmem, _fsize); //don't care if this fails
            
    assure(read(_fd, _fmem, _fsize) == _fsize);
    
    _dmem = clCreateBuffer(_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, _fsize, _fmem, &clret);assure(!clret);
}

CLTraces::~CLTraces(){
    cl_int clret = 0;
    if (_dmem) {
        clret = clReleaseMemObject(_dmem);_dmem = NULL;assure(!_dmem);
    }
    if (_fmem) {
        munmap(_fmem, _fsize);
    }
    if (_fd > 0){
        close(_fd);
    }
}

cl_mem CLTraces::getHostMem(){
    return _dmem;
}

size_t CLTraces::size(){
    return _fsize;
}
