//
//  CPUModelResults.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 17.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef CPUModelResults_hpp
#define CPUModelResults_hpp

#include <iostream>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "combtrace.hpp"

class CPUDevice;

class CPUModelResults {
    std::string _modelName;
    uint32_t _point_per_trace;

    combtrace _meanTrace;
    combtrace _censumTrace;

    combtrace _meanHypot;
    combtrace _censumHypot;

    combtrace _upperPart;

public:
    CPUModelResults(std::string modelName, uint32_t point_per_trace);
    CPUModelResults(void *data, size_t dataSize);

    CPUModelResults(const CPUModelResults &cpy) = delete;
    ~CPUModelResults();
    
    
    const combtrace &meanTrace(){return _meanTrace;}
    const combtrace &censumTrace(){return _censumTrace;}
    const combtrace &meanHypot(){return _meanHypot;}
    const combtrace &censumHypot(){return _censumHypot;}
    const combtrace &upperPart(){return _upperPart;}
    
    std::string modelName(){return _modelName;}

    
    //outBuffer is allocated by this function. caller is responsible for freeing it
    void serialize(void **outBuffer, size_t *outBufferSize);


    
    friend CPUDevice;
};


#endif /* CPUModelResults_hpp */
