//
//  numerics.cpp
//  StaticVsRandom
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "numerics.hpp"
#include <string.h>

uint8_t aeskey[16*15] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0xd6, 0xaa, 0x74, 0xfd, 0xd2, 0xaf, 0x72, 0xfa, 0xda, 0xa6, 0x78, 0xf1, 0xd6, 0xab, 0x76, 0xfe,
    0xf6, 0x63, 0x3a, 0xb8, 0xf2, 0x66, 0x3c, 0xbf, 0xfa, 0x6f, 0x36, 0xb4, 0xf6, 0x62, 0x38, 0xbb,
    0x7e, 0xad, 0x9e, 0xbf, 0xac, 0x02, 0xec, 0x45, 0x76, 0xa4, 0x94, 0xb4, 0xa0, 0x0f, 0xe2, 0x4a,
    0x16, 0x15, 0xa2, 0x6e, 0xe4, 0x73, 0x9e, 0xd1, 0x1e, 0x1c, 0xa8, 0x65, 0xe8, 0x7e, 0x90, 0xde,
    0x89, 0xcd, 0x83, 0x24, 0x25, 0xcf, 0x6f, 0x61, 0x53, 0x6b, 0xfb, 0xd5, 0xf3, 0x64, 0x19, 0x9f,
    0x1b, 0x56, 0x76, 0xb5, 0xff, 0x25, 0xe8, 0x64, 0xe1, 0x39, 0x40, 0x01, 0x09, 0x47, 0xd0, 0xdf,
    0x21, 0xbd, 0x1d, 0x25, 0x04, 0x72, 0x72, 0x44, 0x57, 0x19, 0x89, 0x91, 0xa4, 0x7d, 0x90, 0x0e,
    0x52, 0xa9, 0x16, 0x1e, 0xad, 0x8c, 0xfe, 0x7a, 0x4c, 0xb5, 0xbe, 0x7b, 0x45, 0xf2, 0x6e, 0xa4,
    0xb8, 0x22, 0x54, 0x4b, 0xbc, 0x50, 0x26, 0x0f, 0xeb, 0x49, 0xaf, 0x9e, 0x4f, 0x34, 0x3f, 0x90,
    0xd6, 0xb1, 0x63, 0x7e, 0x7b, 0x3d, 0x9d, 0x04, 0x37, 0x88, 0x23, 0x7f, 0x72, 0x7a, 0x4d, 0xdb,
    0x42, 0xc1, 0xed, 0x0b, 0xfe, 0x91, 0xcb, 0x04, 0x15, 0xd8, 0x64, 0x9a, 0x5a, 0xec, 0x5b, 0x0a,
    0x68, 0x7f, 0x5a, 0x19, 0x13, 0x42, 0xc7, 0x1d, 0x24, 0xca, 0xe4, 0x62, 0x56, 0xb0, 0xa9, 0xb9,
    0xe5, 0x12, 0xbb, 0xba, 0x1b, 0x83, 0x70, 0xbe, 0x0e, 0x5b, 0x14, 0x24, 0x54, 0xb7, 0x4f, 0x2e
};

inline uint8_t hamming_weight(uint8_t c){
    // From Advances in Visual Computing: 4th International Symposium, ISVC 2008
    uint8_t d = c & 0b01010101;
    uint8_t e = (c >> 1) & 0b01010101;
    uint8_t f = d+e;
    uint8_t g = f & 0b00110011;
    uint8_t h = (f >> 2) & 0b00110011;
    uint8_t i = g+h;
    uint8_t j = i & 0b00001111;
    uint8_t k  = (i >> 4) & 0b00001111;
    
    return j + k;
}

inline uint8_t hamming_weight_32(uint32_t b){
    return hamming_weight(b >> 0) + hamming_weight(b >> 8) + hamming_weight(b >> 16) + hamming_weight(b >> 24);
}

uint32_t computeMean(const Traces &tr, cl_double *output, const trCollection &hypotMeanGuess){
    uint32_t tracesInFile = tr.tracesInFile();
    uint32_t pointsPerTrace = tr.pointsPerTrace();
    memset(output, 0, sizeof(cl_double)*pointsPerTrace);
    
    for (uint32_t j=0; j<tracesInFile; j++) {
        const Traces::MeasurementStructure *msm = tr[j];
        uint8_t ptextAddkey[16];
        ((uint64_t*)ptextAddkey)[0] = ((uint64_t*)&msm->Plaintext)[0] ^ ((uint64_t*)aeskey)[0];
        ((uint64_t*)ptextAddkey)[1] = ((uint64_t*)&msm->Plaintext)[1] ^ ((uint64_t*)aeskey)[1];

        cl_double factor = (1.0/tracesInFile);
        for (uint16_t k = 0; k<0x100; k++) {
            uint8_t hypot = ptextAddkey[0] == 0;
            
            hypotMeanGuess.vals[k] += (hypot) * factor;
        }
        
        for (uint32_t i=0; i<pointsPerTrace; i++) {
            output[i] += msm->Trace[i] * factor;
        }
    }
    
    for (uint16_t k = 0; k<0x100; k++) {
        hypotMeanGuess.cnt[k] = tracesInFile;
    }
    
    return tracesInFile;
}

uint64_t combineMeans(meanTrace Q1, meanTrace Q2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = Q1.cnt + Q2.cnt;
    if (!Qmergedcnt) return 0;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        cl_double tmp = (Q2.mean[i] - Q1.mean[i]);
        tmp = (Q2.cnt * tmp) / Qmergedcnt;
        Q1.mean[i] = Q1.mean[i] + tmp;
    }
    
    return Qmergedcnt;
}

uint32_t computeCenteredSum(const Traces &tr, const cl_double *mean, cl_double *output, const trCollection &hypotMeanGuess, const trCollection &hypotCensumGuess, numberedTrace *curupperPart){
    uint32_t tracesInFile = tr.tracesInFile();
    uint32_t pointsPerTrace = tr.pointsPerTrace();
    memset(output, 0, sizeof(cl_double)*pointsPerTrace);
    
    cl_double *hypotMeanGuessVals = hypotMeanGuess.vals;
    cl_double *hypotCensumGuessVals = hypotCensumGuess.vals;
    cl_double realTmp[pointsPerTrace];

    for (uint32_t j=0; j<tracesInFile; j++) {
        const Traces::MeasurementStructure *msm = tr[j];
        const int8_t *msmTrace = msm->Trace;
        
        for (uint32_t i=0; i<pointsPerTrace; i++) {
            cl_double tmp = realTmp[i] = msmTrace[i] - mean[i];
            output[i] += tmp*tmp;
        }
        
        uint8_t ptextAddkey[16];
        ((uint64_t*)ptextAddkey)[0] = ((uint64_t*)&msm->Plaintext)[0] ^ ((uint64_t*)aeskey)[0];
        ((uint64_t*)ptextAddkey)[1] = ((uint64_t*)&msm->Plaintext)[1] ^ ((uint64_t*)aeskey)[1];

        for (uint16_t k = 0; k<0x100; k++) {
            uint8_t hypot = ptextAddkey[0] == 0;

            cl_double hypotTmp = (hypot) - hypotMeanGuessVals[k];
            hypotCensumGuessVals[k] += hypotTmp*hypotTmp;
    
            cl_double *curUpperTrace = curupperPart[k].trace;
            for (uint32_t i=0; i<pointsPerTrace; i++) {
                curUpperTrace[i] += hypotTmp*realTmp[i];
            }
        }
    }
    for (uint16_t k = 0; k<0x100; k++) {
        hypotCensumGuess.cnt[k] = tracesInFile;
    }

    return tracesInFile;
}

uint64_t combineCenteredSum(varTrace Q1, varTrace Q2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = Q1.cnt + Q2.cnt;
    if (!Qmergedcnt) return 0;

    for (uint32_t i=0; i<pointsPerTrace; i++) {
        cl_double tmp = (Q2.mean[i]-Q1.mean[i]);
        
        cl_double tmp1 = (tmp*tmp)/Qmergedcnt;
        
        cl_double tmp2 = Q1.cnt * Q2.cnt * tmp1;
        
        Q1.centeredSum[i] = Q1.centeredSum[i] + Q2.centeredSum[i] + tmp2;
    }
    
    return Qmergedcnt;
}
