//
//  GPUWorker.hpp
//  align_chip_opencl
//
//  Created by tihmstar on 10.01.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef GPUWorker_hpp
#define GPUWorker_hpp

#include "GPUDevice.hpp"
#include "GPUMem.hpp"
#include <thread>
#include "combtrace.hpp"
#include "LoadFile.hpp"
#include <queue>

class GPUWorker{
    int _workernum;
    GPUDevice *_parent; //not owned
    std::thread *_workerThread;
    
    bool _stopWorker;

    cl_command_queue _command_queue;
    cl_mem _meanTrace;
    cl_mem _meanTraceCnt;
    cl_kernel _kernel_computeMean;
    cl_kernel _kernel_updateMeanCnt;

    std::queue<GPUMem *> _tracesQueue;
    std::mutex _tracesQueueLock;
    std::mutex _tracesQueueLockEvent;
    std::mutex _tracesQueueInProcessLockEvent;
        
    void worker();
    
    std::queue<LoadFile *> _cputraces;

    
public:
    GPUWorker(int workernum, GPUDevice *parent);
    ~GPUWorker();
    
    void enqueue(GPUMem *mem);
    
    std::queue<LoadFile *> &cputraces();
    
    combtrace getResults();
};

#endif /* GPUWorker_hpp */
