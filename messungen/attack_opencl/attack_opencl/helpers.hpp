//
//  helpers.hpp
//  attack_opencl
//
//  Created by tihmstar on 05.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef helpers_hpp
#define helpers_hpp

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


void relaxedWaitForEvent(cl_uint num_events, const cl_event * event_list);

#endif /* helpers_hpp */
