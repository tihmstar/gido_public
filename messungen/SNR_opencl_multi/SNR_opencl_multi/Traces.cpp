//
//  Traces.cpp
//  normalize
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "Traces.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN Traces.cpp ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#else
#define dbg_assure(cond) //
#endif

#pragma mark Traces

Traces::Traces(const char* path, size_t minSize){
    struct stat sb;
    
    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    
    size_t realFileSize = _fsize = sb.st_size;
    
    if (_fsize < minSize) { //min alloc size
        _fsize = minSize;
    }
    
    //mmap, but don't map the file
    assure((_fmem = (int8_t*)mmap(NULL, _fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)) != (void*)-1);
    mlock(_fmem, _fsize); //don't care if this fails
            
    assure(read(_fd, _fmem, realFileSize) == realFileSize);
    
}

Traces::~Traces(){
    if (_fmem) {
        munlock(_fmem, _fsize);
        munmap(_fmem,_fsize);
    }
    if (_fd > 0){
        close(_fd);
    }
}
               
const int8_t *Traces::buf(){
   return _fmem;
}
               
size_t Traces::bufSize(){
   return _fsize;
}

uint32_t Traces::tracesInFile() const{
    return *(uint32_t*)_fmem;
}

uint32_t Traces::pointsPerTrace() const{
    return ((uint32_t*)_fmem)[1];
}
