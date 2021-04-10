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

#define assure(cond) do {if (!(cond)) {printf("ERROR: MAIN ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


int r_main(int argc, const char * argv[]) {
    int ret = 0;
    if (argc < 4) {
        printf("Usage: %s <traces dir path> <server address:port> {threads cnt}\n",argv[0]);
        return -1;
    }

    uint16_t threadsCnt = 0;

    
    const char *indir = argv[1];

    const char *serveraddr = argv[2];
    size_t ramsize = atol(argv[3]);
    
    if (argc > 4) {
        threadsCnt = atoi(argv[4]);
    }
    
    const char *portStr = NULL;
    
    assure(portStr = strchr(serveraddr, ':'));
    
    std::string serverAddress = serveraddr;
    serverAddress = serverAddress.substr(0, portStr-serveraddr);
    uint16_t port = (uint16_t)atoi(portStr+1);
    

    
    CorrWorker worker(indir, serverAddress.c_str(),port, ramsize, threadsCnt);
    
    worker.startWorker();
    
    
    std::mutex hang;
    hang.lock();
    hang.lock();

    printf("done!\n");
    return ret;
}

#ifndef NOMAIN
int main(int argc, const char * argv[]) {
    
#ifdef DEBUG
    return r_main(argc, argv);
#else //DEBUG
    try {
        r_main(argc, argv);
    } catch (int e) {
        printf("ERROR: died on line=%d\n",e);
    }
#endif //DEBUG
}
#endif //NOMAIN
