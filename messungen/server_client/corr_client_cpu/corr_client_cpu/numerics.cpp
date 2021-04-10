//
//  numerics.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 17.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "numerics.hpp"

void combineMeans(combtrace &Q1, const combtrace &Q2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = Q1.cnt + Q2.cnt;
    if (!Qmergedcnt) return;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        double tmp = (Q2.trace[i] - Q1.trace[i]);
        tmp = (Q2.cnt * tmp) / Qmergedcnt;
        Q1.trace[i] = Q1.trace[i] + tmp;
    }
    
    Q1.cnt = Qmergedcnt;
}

void combineCenteredSum(combtrace &censumQ1, const combtrace &meanQ1, combtrace &censumQ2, const combtrace &meanQ2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = censumQ1.cnt + censumQ2.cnt;
    if (!Qmergedcnt) return;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        double tmp = (meanQ2.trace[i]-meanQ1.trace[i]);
        
        double tmp1 = (tmp*tmp)/Qmergedcnt;
        
        double tmp2 = censumQ1.cnt * censumQ2.cnt * tmp1;
        
        censumQ1.trace[i] = censumQ1.trace[i] + censumQ2.trace[i] + tmp2;
    }
    censumQ1.cnt = Qmergedcnt;
}
