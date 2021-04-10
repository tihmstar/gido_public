//
//  myfile.cpp
//  corr_server
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "myfile.hpp"
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define assure_(cond) do {if (!(cond)) {printf("ERROR: myfile ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

myfile::myfile(const char *fname, size_t size) : _fd(-1),_size(size){
    for (int i=0; i<100; i++) {
        if ((_fd = open(fname, O_RDWR | O_CREAT, 0644))>0) break;
        printf("failed to open file, retrying... (%d - %s)\n",errno,strerror(errno));
        sleep(3);
    }
    assure_(_fd > 0);
    lseek(_fd, _size-1, SEEK_SET);
    write(_fd, "", 1);
    assure_(_mem = (double*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
}
myfile::~myfile(){
    munmap(_mem, _size);
    if (_fd > 0) close(_fd);
}
double &myfile::operator[](int at){
    return _mem[at];
}
