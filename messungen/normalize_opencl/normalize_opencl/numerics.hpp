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
    cl_float *trace;
    uint64_t cnt;
public:
    combtrace() : trace(NULL),cnt(0){}
    combtrace(cl_float *trace_,uint64_t cnt_) : trace(trace_),cnt(cnt_){}
    combtrace(const combtrace &cpy) = delete;

    combtrace(combtrace &&mv){
        cnt = mv.cnt;
        trace = mv.trace;
        mv.trace = NULL;
    }

    
    ~combtrace(){
        if (trace) {
            free(trace);
        }
    }
};

void precompute_tables(void); //AES T-Tables

/*
    Input:
        tr = Tracefile
        output = buffer of size "sizeof(cl_float)*tr.pointsPerTrace()"
        doRandom = if true, use random plaintext traces, else use fixed plaintext traces
    Output:
        returns the number of members which were "mean-ed"
 */
void computeMeans(const Traces &tr, std::vector<combtrace> &output);

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
       tr = Tracefile
       mean = mean trace of Tracefile tr
       output = buffer of size "sizeof(cl_float)*tr.pointsPerTrace()"
       doRandom = if true, use random plaintext traces, else use fixed plaintext traces
   Output:
       returns the number of members which were "variance-ed"
*/
void computeCenteredSums(const Traces &tr, const std::vector<combtrace> &mean, std::vector<combtrace> &output);
//void computeVariaces(const Traces &tr, const std::vector<combtrace> &mean, std::vector<combtrace> &output);

/*
    Input:
        Q1,Q2 centeredSum traces with size
        pointsPerTrace
    Computes: Q1 = Q1 + Q2
    Output:
        sum of elements in both groups
 */
void combineCenteredSum(combtrace &censumQ1, const combtrace &meanQ1, combtrace &censumQ2, const combtrace &meanQ2, uint32_t pointsPerTrace);



void computeMeanOfMeans(const std::vector<combtrace> &means, cl_float *output, uint32_t pointsPerTrace);

void computeVariancesOfMeans(const std::vector<combtrace> &means, const cl_float *meanOfMeans, cl_float* output, uint32_t pointsPerTrace);

void computeMeanOfVariancesFromCensums(const std::vector<combtrace> &cenSums, cl_float *output, uint32_t pointsPerTrace);

#endif /* numerics_hpp */
