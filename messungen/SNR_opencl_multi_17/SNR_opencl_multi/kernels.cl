
#pragma OPENCL EXTENSION cl_khr_fp64 : enable


#define AES_BLOCK_SELECTOR 0
#define AES_BLOCKS_TOTAL 8 //8*16 = 128bytes

struct MeasurementStructure{
    uint    NumberOfPoints;

    uchar   _shiftp[AES_BLOCK_SELECTOR*16];
    uchar   Input[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16];

    uchar   _shiftc[AES_BLOCK_SELECTOR*16];
    uchar   Output[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16];
    char    Trace[0];
};

struct Tracefile{
    uint  tracesInFile;
    struct MeasurementStructure ms[0];
};


inline uchar hamming_weight(uchar c){
    // From Advances in Visual Computing: 4th International Symposium, ISVC 2008
    uchar d = c & 0b01010101;
    uchar e = (c >> 1) & 0b01010101;
    uchar f = d+e;
    uchar g = f & 0b00110011;
    uchar h = (f >> 2) & 0b00110011;
    uchar i = g+h;
    uchar j = i & 0b00001111;
    uchar k  = (i >> 4) & 0b00001111;
    
    return j + k;
}

inline uchar hamming_weight_32(uint b){
    return hamming_weight(b >> 0) + hamming_weight(b >> 8) + hamming_weight(b >> 16) + hamming_weight(b >> 24);
}

#define G_SELECTOR_VALUE 8
//BEGIN TRACE SELECTORS


/* BEGIN BLOCK 0 */
inline uchar traceSelector_INPUT(__global const struct MeasurementStructure *msm){
    return msm->Input[G_SELECTOR_VALUE];
}

inline uchar traceSelector_INPUT_HW(__global const struct MeasurementStructure *msm){
    return hamming_weight(msm->Input[G_SELECTOR_VALUE]);
}

inline uchar traceSelector_OUTPUT(__global const struct MeasurementStructure *msm){
    return msm->Output[G_SELECTOR_VALUE];
}

inline uchar traceSelector_OUTPUT_HW(__global const struct MeasurementStructure *msm){
    return hamming_weight(msm->Output[G_SELECTOR_VALUE]);
}
/* END BLOCK 0 */

//END TRACE SELECTORS

__kernel void computeMeans(__global const struct Tracefile *tr, __global double *curgroupsMean, __global ulong *curgroupsMeanCnt, uint modelSize){
    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;
    if (i>=pointsPerTrace) return; //these guys are out of bounds

    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

    ulong meanCounts[0x100] = {};//all threads compute a local meanCounts
    
    __global const struct MeasurementStructure *msm = tr->ms;
    for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){
        uchar selector = traceSelector(msm);
        
        curgroupsMean[pointsPerTrace*selector + i] += msm->Trace[i]; //add up
        meanCounts[selector]++; //keep count
    }
    
    for (uint s = 0; s<modelSize; s++){
        if (meanCounts[s] == 0) continue;
        curgroupsMean[pointsPerTrace*s + i] /= meanCounts[s]; //divide by count
    }
  
    //only warp desync here
    if (i<modelSize){ //let the first 0x100 threads write their curgroupsMeanCnt
        curgroupsMeanCnt[i] = meanCounts[i];
    }
}

__kernel void computeCenteredSums(__global const struct Tracefile *tr, __global const double *curgroupsMean, __global const ulong *curgroupsMeanCnt, __global double *curgroupsCenSum, __global ulong *curgroupsCenSumCnt, uint modelSize){
    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;
    if (i>=pointsPerTrace) return; //these guys are out of bounds

    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

    ulong cenSumCounts[0x100] = {};//all threads compute a local meanCounts
    
    __global const struct MeasurementStructure *msm = tr->ms;
    for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){
        uchar selector = traceSelector(msm);

        double tmp = msm->Trace[i] - curgroupsMean[pointsPerTrace*selector + i];
        curgroupsCenSum[pointsPerTrace*selector + i] += tmp*tmp;
        cenSumCounts[selector]++; //keep count
    }
    
    if (i<modelSize){ //let the first 0x100 threads write their curgroupsMeanCnt
        curgroupsCenSumCnt[i] = cenSumCounts[i];
    }
}

__kernel void combineMeanAndCenteredSums(__global double *censumQ1, __global ulong *censumQ1cnt, __global const double *censumQ2, __global const ulong *censumQ2cnt, __global double *meanQ1, __global ulong *meanQ1cnt, __global const double *meanQ2, __global const ulong *meanQ2cnt, uint pointsPerTrace, uint modelSize){

    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
   
    if (i>=pointsPerTrace) return; //these guys are out of bounds
    
   /* COMBINE MEANS */

    for (uint s=0; s<modelSize; s++){
        ulong meanQmergedcnt = meanQ1cnt[s] + meanQ2cnt[s];
        if (!meanQmergedcnt) continue;
        
        double tmp0 = meanQ2[pointsPerTrace*s + i] - meanQ1[pointsPerTrace*s + i];
        double tmp = (meanQ2cnt[s] * tmp0) / meanQmergedcnt;

        meanQ1[pointsPerTrace*s + i] = meanQ1[pointsPerTrace*s + i] + tmp;
    }
    
    
    /* COMBINE CENSUMS */

    for (uint s=0; s<modelSize; s++){
        ulong censumQmergedcnt = censumQ1cnt[s] + censumQ2cnt[s];
        if (!censumQmergedcnt) continue;

        double tmp = meanQ2[pointsPerTrace*s + i] - meanQ1[pointsPerTrace*s + i];

        double tmp1 = (tmp*tmp) / censumQmergedcnt;

        double tmp2 = censumQ1cnt[s] * censumQ2cnt[s] * tmp1;

        censumQ1[pointsPerTrace*s + i] = censumQ1[pointsPerTrace*s + i] + censumQ2[pointsPerTrace*s + i] + tmp2;
    }
}

__kernel void mergeCoutners(__global ulong *censumQ1cnt, __global const ulong *censumQ2cnt, __global ulong *meanQ1cnt, __global const ulong *meanQ2cnt, uint modelSize){
    uint s = get_global_id(0) + get_global_id(1) * get_global_size(0);
    if (s>=modelSize) return; //these guys are out of bounds
    
    meanQ1cnt[s] += meanQ2cnt[s];
    censumQ1cnt[s] += censumQ2cnt[s];
}
