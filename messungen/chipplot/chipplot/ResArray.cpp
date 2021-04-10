//
//  ResArray.cpp
//  chipplot
//
//  Created by tihmstar on 23.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "ResArray.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN ResArray.cpp ON LINE=%d\n",__LINE__); throw __LINE__;}}while (0)

ResArray::ResArray(const char *path){
    struct stat sb;

    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    assure((_fmem = (double*)mmap(NULL, _fsize = sb.st_size, PROT_READ, MAP_PRIVATE | MCL_CURRENT, _fd, 0)) != (void*)-1);

    mlock(_fmem, _fsize); //don't care if this fails

}

ResArray::ResArray(size_t nop){
    _fd = -1;
    _fsize = nop*sizeof(*_fmem);
    assure((_fmem = (double*)mmap(NULL, _fsize, PROT_READ, MAP_ANON | MAP_PRIVATE, -1, 0)) != (void*)-1);
    mlock(_fmem, _fsize); //don't care if this fails
    memset(_fmem, 0, _fsize);
}


ResArray::~ResArray(){
    if (_fmem) {
        munlock(_fmem, _fsize);
        munmap(_fmem, _fsize);
    }
    if (_fd > 0){
        close(_fd);
    }
}


double ResArray::operator[](unsigned i) const{
    return _fmem[i];
}

size_t ResArray::size(){
    return _fsize/sizeof(*_fmem);
}
