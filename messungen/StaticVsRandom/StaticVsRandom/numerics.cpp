//
//  numerics.cpp
//  StaticVsRandom
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "numerics.hpp"
#include <string.h>

inline bool skipTrace(const Traces::MeasurementStructure *msm){
//    if (abs(msm->Trace[1000]) >= 10) return true;

    return false;
}


uint32_t computeMean(const Traces &tr, long double *output, bool doRandom){
    uint32_t cnt = 0;
    memset(output, 0, sizeof(long double)*tr.pointsPerTrace());
    for (uint32_t j=0; j<tr.tracesInFile(); j++) {
        const Traces::MeasurementStructure *msm = tr[j];
        if (skipTrace(msm)) continue;
        if (Traces::isFixed(msm) != doRandom) {
            for (uint32_t i=0; i<tr.pointsPerTrace(); i++) {
                output[i] += msm->Trace[i];
            }
            cnt ++;
        }
    }
    for (int i=0; i<tr.pointsPerTrace(); i++) {
        output[i] /= cnt;
    }
    return cnt;
}

uint64_t combineMeans(meanTrace Q1, meanTrace Q2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = Q1.cnt + Q2.cnt;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        long double tmp = (Q2.mean[i] - Q1.mean[i]);
        tmp = (Q2.cnt * tmp) / Qmergedcnt;
        Q1.mean[i] = Q1.mean[i] + tmp;
    }
    
    return Qmergedcnt;
}

uint32_t computeVariace(const Traces &tr, const long double *mean, long double *output, uint8_t doRandom){
    uint32_t cnt = 0;
    cnt = computeCenteredSum(tr, mean, output, doRandom);
    for (int i=0; i<tr.pointsPerTrace(); i++) {
        output[i] /= cnt;
    }
    return cnt;
}

uint32_t computeCenteredSum(const Traces &tr, const long double *mean, long double *output, uint8_t doRandom){
    memset(output, 0, sizeof(long double)*tr.pointsPerTrace());
    uint32_t cnt = 0;
    for (uint32_t j=0; j<tr.tracesInFile(); j++) {
        const Traces::MeasurementStructure *msm = tr[j];
        if (skipTrace(msm)) continue;
        if (Traces::isFixed(msm) != doRandom) {
            for (uint32_t i=0; i<tr.pointsPerTrace(); i++) {
                long double tmp = msm->Trace[i] - mean[i];
                output[i] += tmp*tmp;
            }
            cnt ++;
        }
    }
    return cnt;
}

uint64_t combineCenteredSum(varTrace Q1, varTrace Q2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = Q1.cnt + Q2.cnt;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        long double tmp = (Q2.mean[i]-Q1.mean[i]);
        long double tmp1 = (tmp*tmp)/Qmergedcnt;
        long double tmp2 = Q1.cnt * Q2.cnt * tmp1;
        Q1.centeredSum[i] = Q1.centeredSum[i] + Q2.centeredSum[i] + tmp2;
    }
    
    return Qmergedcnt;
}
