//
//  combtrace.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 17.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef combtrace_hpp
#define combtrace_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <stdint.h>
#include <stdlib.h>

struct combtrace{
public:
    gpu_float_type *trace;
    uint64_t cnt;
public:
    combtrace() : trace(NULL),cnt(0){}
    combtrace(const combtrace &cpy) = delete;
    combtrace(combtrace &&mv){
        cnt = mv.cnt;
        trace = mv.trace;
        mv.trace = NULL;
    }
    
    ~combtrace(){
        if (trace) {
            free(trace);trace=NULL;
        }
    }
};
#endif /* combtrace_hpp */
