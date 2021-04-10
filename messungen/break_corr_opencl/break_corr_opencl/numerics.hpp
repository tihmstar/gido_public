//
//  numerics.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 17.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef numerics_hpp
#define numerics_hpp

#include <stdint.h>
#include <iostream>
#include <tuple>
#include <vector>
#include "combtrace.hpp"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


/*
    Input:
        Q1,Q2 Mean traces with size
        pointsPerTrace
    Computes: Q1 = Q1 + Q2
    Output:
        sum of elements in both groups
*/
void combineMeans(combtrace &Q1, const combtrace &Q2, uint32_t pointsPerTrace);

/*
    Input:
        Q1,Q2 centeredSum traces with size
        pointsPerTrace
    Computes: Q1 = Q1 + Q2
    Output:
        sum of elements in both groups
 */
void combineCenteredSum(combtrace &censumQ1, const combtrace &meanQ1, combtrace &censumQ2, const combtrace &meanQ2, uint32_t pointsPerTrace);


#endif /* numerics_hpp */
