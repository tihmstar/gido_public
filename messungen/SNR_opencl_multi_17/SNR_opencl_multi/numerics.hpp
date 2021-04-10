//
//  numerics.hpp
//  StaticVsRandom
//
//  Created by tihmstar on 07.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef numerics_hpp
#define numerics_hpp

#include <stdint.h>
#include "Traces.hpp"
#include <iostream>
#include <tuple>
#include <vector>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif


struct combtrace{
public:
    gpu_float_type *trace;
    uint64_t cnt;
public:
    combtrace() : trace(NULL),cnt(0){}
    combtrace(gpu_float_type *trace_,uint64_t cnt_) : trace(trace_),cnt(cnt_){}
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

void precompute_tables(void); //AES T-Tables

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



void computeMeanOfMeans(const std::vector<combtrace> &means, gpu_float_type *output, uint32_t pointsPerTrace);

void computeVariancesOfMeans(const std::vector<combtrace> &means, const gpu_float_type *meanOfMeans, gpu_float_type* output, uint32_t pointsPerTrace);

void computeMeanOfVariancesFromCensums(const std::vector<combtrace> &cenSums, gpu_float_type *output, uint32_t pointsPerTrace);

#endif /* numerics_hpp */
