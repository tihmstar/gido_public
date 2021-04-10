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

/*
    Input:
        tr = Tracefile
        output = buffer of size "sizeof(double)*tr.pointsPerTrace()"
        doRandom = if true, use random plaintext traces, else use fixed plaintext traces
    Output:
        returns the number of members which were "mean-ed"
 */
struct trCollection {
    double *vals;
    uint64_t *cnt;
};
uint32_t computeMean(const Traces &tr, double *output, const trCollection &hypotMeanGuess);

/*
    Input:
        Q1,Q2 Mean traces with size
        pointsPerTrace
    Computes: Q1 = Q1 + Q2
    Output:
        sum of elements in both groups
*/
struct meanTrace {
    double *mean;
    uint64_t cnt;
};
uint64_t combineMeans(meanTrace Q1, meanTrace Q2, uint32_t pointsPerTrace);

/*
   Input:
       tr = Tracefile
       mean = mean trace of Tracefile tr
       output = buffer of size "sizeof(double)*tr.pointsPerTrace()"
       doRandom = if true, use random plaintext traces, else use fixed plaintext traces
   Output:
       returns the number of members which were "variance-ed"
*/
struct numberedTrace {
    double *trace;
    uint64_t cnt;
};
uint32_t computeCenteredSum(const Traces &tr, const double *mean, double *output, const trCollection &hypotMeanGuess, const trCollection &hypotCensumGuess, numberedTrace *curupperPart);

/*
    Input:
        Q1,Q2 centeredSum traces with size
        pointsPerTrace
    Computes: Q1 = Q1 + Q2
    Output:
        sum of elements in both groups
 */
struct varTrace {
    double *centeredSum;
    double *mean;
    uint64_t cnt;
};

uint64_t combineCenteredSum(varTrace Q1, varTrace Q2, uint32_t pointsPerTrace);


#endif /* numerics_hpp */
