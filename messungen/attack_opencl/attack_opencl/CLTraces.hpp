//
//  CLTrace.hpp
//  attack_opencl
//
//  Created by tihmstar on 05.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef CLTraces_hpp
#define CLTraces_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


#include <stdint.h>
#include <unistd.h>

class CLTraces {
    size_t _fsize;
    int _fd;
    int8_t *_fmem;
    cl_mem _dmem;
    cl_context _context;
    
public:
    CLTraces(const char *path, cl_context context, cl_device_id device);
    ~CLTraces();

    cl_mem getHostMem();
    size_t size();
    
};


#endif /* CLTraces_hpp */
