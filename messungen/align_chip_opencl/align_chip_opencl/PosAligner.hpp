//
//  PosAligner.hpp
//  align_chip_opencl
//
//  Created by tihmstar on 10.01.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef PosAligner_hpp
#define PosAligner_hpp


#include <iostream>
#include <vector>
#include "GPUDevice.hpp"
#include <queue>
#include <mutex>

class PosAligner{
    std::string _posPath;
    GPUDevice *_gpu;
        
    std::queue<std::vector<std::string>> _doMeanQueue;
    
    std::queue<GPUWorker *> _gpuWorkers;
    std::mutex _gpuWorkersLock;
    std::mutex _gpuWorkersEventLock;

    
public:
    PosAligner(GPUDevice *gpu);
    ~PosAligner();
        
    void enqueuePosList(const std::vector<std::string> posList);
    
    void run(const char *outdir);
    
};


#endif /* PosAligner_hpp */
