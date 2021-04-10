
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define AES_BLOCK_SELECTOR 0
#define AES_BLOCKS_TOTAL 4 //4*16 = 64bytes

struct MeasurementStructure{
    uint    NumberOfPoints;

    uchar   _shiftp[AES_BLOCK_SELECTOR*16];
    uchar   Plaintext[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16];

    uchar   _shiftc[AES_BLOCK_SELECTOR*16];
    uchar   Ciphertext[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16];
    char    Trace[0];
};


struct Tracefile{
    uint  tracesInFile;
    struct MeasurementStructure ms[0];
};


__kernel void computeMean(__global const struct Tracefile *tr, __global double *output, __global ulong *curcntMean){
    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;
    if (i>=pointsPerTrace) return; //these guys are out of bounds

    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

    double factor = (1.0/tracesInFile);
    
    double cmean = 0;
    
    /* COMPUTE MEANS */
    __global const struct MeasurementStructure *msm = tr->ms;
    for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){
        cmean += msm->Trace[i] * factor;
    }
    
    /* COMBINE MEANS */
    ulong meanQmergedcnt = *curcntMean /*Q1cnt*/ + tracesInFile /*Q2cnt*/;
    double tmp0 = cmean/*meanQ2[i]*/ - output[i]/*meanQ1[i]*/;
    double tmp = (tracesInFile/*Q2cnt*/ * tmp0) / meanQmergedcnt;
    output[i]/*meanQ1[i]*/ = output[i]/*meanQ1[i]*/ + tmp;
}

__kernel void updateMeanCnt(__global const struct Tracefile *tr, __global ulong *curcntMean){
    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
    uint tracesInFile = tr->tracesInFile;
    
    //only thread 0 should write curcntMean
    if (i == 0) *curcntMean += tracesInFile;
}
