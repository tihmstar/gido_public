//
//  numerics.cpp
//  StaticVsRandom
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "numerics.hpp"
#include <string.h>
#include "aes.h"
extern "C"{
#include "aes_github.h"
};
//uint8_t aeskey[32] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

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

struct AES_ctx gstate = {};

void doInit(void){
    AES_init_ctx(&gstate, aeskey);
}

#define G_SELECTOR_VALUE 8
#define G_SELECTOR_ROUND 4
inline uint8_t traceSelector(const Traces::MeasurementStructure *msm){
    return msm->Plaintext[G_SELECTOR_VALUE]; //plaintext value
//    uint8_t ptextAddkey[16];
//    uint32_t *aeskeywords = (uint32_t *)aeskey;
//    ptextAddkey[ 0] = msm->Plaintext[ 0] ^ aeskey[ 0];
//    ptextAddkey[ 1] = msm->Plaintext[ 1] ^ aeskey[ 1];
//    ptextAddkey[ 2] = msm->Plaintext[ 2] ^ aeskey[ 2];
//    ptextAddkey[ 3] = msm->Plaintext[ 3] ^ aeskey[ 3];
//    ptextAddkey[ 4] = msm->Plaintext[ 4] ^ aeskey[ 4];
//    ptextAddkey[ 5] = msm->Plaintext[ 5] ^ aeskey[ 5];
//    ptextAddkey[ 6] = msm->Plaintext[ 6] ^ aeskey[ 6];
//    ptextAddkey[ 7] = msm->Plaintext[ 7] ^ aeskey[ 7];
//    ptextAddkey[ 8] = msm->Plaintext[ 8] ^ aeskey[ 8];
//    ptextAddkey[ 9] = msm->Plaintext[ 9] ^ aeskey[ 9];
//    ptextAddkey[10] = msm->Plaintext[10] ^ aeskey[10];
//    ptextAddkey[11] = msm->Plaintext[11] ^ aeskey[11];
//    ptextAddkey[12] = msm->Plaintext[12] ^ aeskey[12];
//    ptextAddkey[13] = msm->Plaintext[13] ^ aeskey[13];
//    ptextAddkey[14] = msm->Plaintext[14] ^ aeskey[14];
//    ptextAddkey[15] = msm->Plaintext[15] ^ aeskey[15];
//
//    uint32_t w[4] = {0};
//
//    for (int i=1; i<G_SELECTOR_ROUND; i++) { //4 middle rounds
//
//        w[0] = T0_inv[ptextAddkey[ 0]] ^ T1_inv[ptextAddkey[ 5]] ^ T2_inv[ptextAddkey[10]] ^ T3_inv[ptextAddkey[15]] ^ aeskeywords[i*4 + 0];
//        w[1] = T0_inv[ptextAddkey[ 4]] ^ T1_inv[ptextAddkey[ 9]] ^ T2_inv[ptextAddkey[14]] ^ T3_inv[ptextAddkey[ 3]] ^ aeskeywords[i*4 + 1];
//        w[2] = T0_inv[ptextAddkey[ 8]] ^ T1_inv[ptextAddkey[13]] ^ T2_inv[ptextAddkey[ 2]] ^ T3_inv[ptextAddkey[ 7]] ^ aeskeywords[i*4 + 2];
//        w[3] = T0_inv[ptextAddkey[12]] ^ T1_inv[ptextAddkey[ 1]] ^ T2_inv[ptextAddkey[ 6]] ^ T3_inv[ptextAddkey[11]] ^ aeskeywords[i*4 + 3];
//
//        *(uint32_t*)&ptextAddkey[ 0] = w[0];
//        *(uint32_t*)&ptextAddkey[ 4] = w[1];
//        *(uint32_t*)&ptextAddkey[ 8] = w[2];
//        *(uint32_t*)&ptextAddkey[12] = w[3];
//    }
//    return hamming_weight(ptextAddkey[G_SELECTOR_VALUE]);
}

void computeMeans(const Traces &tr, std::vector<combtrace> &output){
    uint32_t tracesInFile = tr.tracesInFile();
    uint32_t pointsPerTrace = tr.pointsPerTrace();

    for (uint32_t j=0; j<tracesInFile; j++) {
        const Traces::MeasurementStructure *msm = tr[j];

        auto &cur = output.at(traceSelector(msm));
        for (uint32_t i=0; i<pointsPerTrace; i++) {
            cur.trace[i] += msm->Trace[i]; //specific
        }
        cur.cnt++;
    }
    
    for (auto &cmbtr : output) {
        if (cmbtr.cnt == 0) continue; //skip if this trace has no members
        for (size_t i=0; i<pointsPerTrace; i++) {
            cmbtr.trace[i] /= cmbtr.cnt;
        }
    }
}

void combineMeans(combtrace &Q1, const combtrace &Q2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = Q1.cnt + Q2.cnt;
    if (!Qmergedcnt) return;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        float tmp = (Q2.trace[i] - Q1.trace[i]);
        tmp = (Q2.cnt * tmp) / Qmergedcnt;
        Q1.trace[i] = Q1.trace[i] + tmp;
    }
    
    Q1.cnt = Qmergedcnt;
}

void computeCenteredSums(const Traces &tr, const std::vector<combtrace> &mean, std::vector<combtrace> &output){
    uint32_t tracesInFile = tr.tracesInFile();
    uint32_t pointsPerTrace = tr.pointsPerTrace();

    for (uint32_t j=0; j<tracesInFile; j++) {
        const Traces::MeasurementStructure *msm = tr[j];

        auto &outCur = output.at(traceSelector(msm));
        auto &meanCur = mean.at(traceSelector(msm));
        for (uint32_t i=0; i<pointsPerTrace; i++) {
            float tmp = msm->Trace[i] - meanCur.trace[i];
            outCur.trace[i] += tmp*tmp;
        }
        outCur.cnt ++;
    }
}

void combineCenteredSum(combtrace &censumQ1, const combtrace &meanQ1, combtrace &censumQ2, const combtrace &meanQ2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = censumQ1.cnt + censumQ2.cnt;
    if (!Qmergedcnt) return;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        float tmp = (meanQ2.trace[i]-meanQ1.trace[i]);
        
        float tmp1 = (tmp*tmp)/Qmergedcnt;
        
        float tmp2 = censumQ1.cnt * censumQ2.cnt * tmp1;
        
        censumQ1.trace[i] = censumQ1.trace[i] + censumQ2.trace[i] + tmp2;
    }
    censumQ1.cnt = Qmergedcnt;
}


void computeMeanOfMeans(const std::vector<combtrace> &means, float *output, uint32_t pointsPerTrace){
    uint32_t members = 0;
    memset(output, 0, sizeof(float)*pointsPerTrace);
    for (auto &m : means) {
        if (m.cnt == 0) continue; //skip if this trace has no members
        for (uint32_t i=0; i<pointsPerTrace; i++) {
            output[i] += m.trace[i];
        }
        members++;
    }
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        output[i] /= members;
    }
}

void computeVariancesOfMeans(const std::vector<combtrace> &means, const float *meanOfMeans, float* output, uint32_t pointsPerTrace){
    uint32_t members = 0;
    memset(output, 0, sizeof(float)*pointsPerTrace);
    for (auto &m : means) {
        
        if (m.cnt == 0) continue; //skip if this trace has no members
        
        for (uint32_t i=0; i<pointsPerTrace; i++) {
            float tmp = 0;
            tmp = m.trace[i] - meanOfMeans[i];
            output[i] += tmp*tmp;
        }
        members++;
    }

    for (uint32_t i=0; i<pointsPerTrace; i++) {
        output[i] /= members;
    }
}

void computeMeanOfVariancesFromCensums(const std::vector<combtrace> &cenSums, float *output, uint32_t pointsPerTrace){
    uint32_t members = 0;
    memset(output, 0, sizeof(float)*pointsPerTrace);
    for (auto &cm : cenSums) {

        if (cm.cnt == 0) continue; //skip if this trace has no members

        for (size_t i=0; i<pointsPerTrace; i++) {
            output[i] += (cm.trace[i] / cm.cnt);
        }
        members++;
    }
    for (size_t i=0; i<pointsPerTrace; i++) {
        output[i] /= members;
    }
}
