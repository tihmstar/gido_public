//
//  main.cpp
//  corr_client_opencl
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include <iostream>
#include "CorrWorker.hpp"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: MAIN ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


int r_main(int argc, const char * argv[]) {
    int ret = 0;
    if (argc < 5) {
        printf("Usage: %s <traces dir path> <server address:port> <ram size> <GPU1,GPU2,GPU3...> {CPULoader Threads} {worker multiplier}\n",argv[0]);
        return -1;
    }
    
    
    const char *indir = argv[1];

    const char *serveraddr = argv[2];
    size_t ramsize = atol(argv[3]);
    const char *gpulist = argv[4];
    
    uint16_t cpuloaderthreads = 0;

    float workermultiplier = 1;

    
    if (argc >=6) {
        cpuloaderthreads = (uint16_t)atoi(argv[5]);
    }

    if (argc >=7) {
        sscanf(argv[6], "%f",&workermultiplier);
        printf("workermultiplier=%f\n",workermultiplier);
    }

    const char *portStr = NULL;
    
    assure(portStr = strchr(serveraddr, ':'));
    
    std::string serverAddress = serveraddr;
    serverAddress = serverAddress.substr(0, portStr-serveraddr);
    uint16_t port = (uint16_t)atoi(portStr+1);
    
    std::vector<int> gpus;
    
    const char *tmpgpu = gpulist-1;
    
    do{
        gpus.push_back(atoi(tmpgpu+1));
    }while((tmpgpu = strchr(tmpgpu+1, ',')));
    
    
    // Create a program from the kernel source
#ifdef XCODE
    LoadFile kernelfile("../../../corr_client_opencl/kernels.cl");
#else
    LoadFile kernelfile("kernels.cl");
#endif
    
    std::string kernelsource{(char*)kernelfile.buf()};

    
    CorrWorker worker(indir, serverAddress.c_str(),port, gpus, kernelsource, ramsize, workermultiplier);
    
    worker.startWorker(cpuloaderthreads);

    std::mutex hang;
    hang.lock();usleep(100);
    hang.lock();usleep(100);

    printf("done!\n");
    return ret;
}

#ifndef NOMAIN
int main(int argc, const char * argv[]) {
    
#ifdef DEBUG
    return r_main(argc, argv);
#else //DEBUG
    try {
        return r_main(argc, argv);
    } catch (int e) {
        printf("ERROR: died on line=%d\n",e);
    }
#endif //DEBUG
    return -1;
}
#endif //NOMAIN
