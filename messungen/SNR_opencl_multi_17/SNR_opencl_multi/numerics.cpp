//
//  numerics.cpp
//  StaticVsRandom
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "numerics.hpp"
#include <string.h>


uint8_t aeskey[32] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };


uint8_t sbox[] = {
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67,
0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59,
0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7,
0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1,
0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05,
0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83,
0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29,
0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa,
0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c,
0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc,
0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19,
0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee,
0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49,
0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4,
0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6,
0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70,
0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9,
0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e,
0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1,
0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0,
0x54, 0xbb, 0x16};

uint32_t T0[256], T1[256], T2[256], T3[256];


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

void precompute_tables(uint32_t *T0_,uint32_t *T1_,uint32_t *T2_,uint32_t *T3_){
    for (int x=0; x<0x100; x++) {
        uint8_t b[4];
        b[0] = sbox[x];
        b[1] = sbox[x];
        b[2] = sbox[x];
        b[3] = sbox[x];
        
        b[0] = ((b[0] & 0x80) == 0) ? (b[0] << 1) : (b[0] << 1) ^ 0x1b;
        b[3] = ((b[3] & 0x80) == 0) ? (b[3] << 1) : (b[3] << 1) ^ 0x1b;
        b[3] ^= b[1];
        
        T0[x] = (b[0] <<  0) | (b[1] <<  8) | (b[2] << 16) | (b[3] << 24);
        T1[x] = (b[3] <<  0) | (b[0] <<  8) | (b[1] << 16) | (b[2] << 24);
        T2[x] = (b[2] <<  0) | (b[3] <<  8) | (b[0] << 16) | (b[1] << 24);
        T3[x] = (b[1] <<  0) | (b[2] <<  8) | (b[3] << 16) | (b[0] << 24);
    }
    
    if (T0_) memcpy(T0_, T0, sizeof(T0));
    if (T1_) memcpy(T1_, T1, sizeof(T1));
    if (T2_) memcpy(T2_, T2, sizeof(T2));
    if (T3_) memcpy(T3_, T3, sizeof(T3));
}

void combineMeans(combtrace &Q1, const combtrace &Q2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = Q1.cnt + Q2.cnt;
    if (!Qmergedcnt) return;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        gpu_float_type tmp = (Q2.trace[i] - Q1.trace[i]);
        tmp = (Q2.cnt * tmp) / Qmergedcnt;
        Q1.trace[i] = Q1.trace[i] + tmp;
    }
    
    Q1.cnt = Qmergedcnt;
}

void combineCenteredSum(combtrace &censumQ1, const combtrace &meanQ1, combtrace &censumQ2, const combtrace &meanQ2, uint32_t pointsPerTrace){
    uint64_t Qmergedcnt = censumQ1.cnt + censumQ2.cnt;
    if (!Qmergedcnt) return;
    
    for (uint32_t i=0; i<pointsPerTrace; i++) {
        gpu_float_type tmp = (meanQ2.trace[i]-meanQ1.trace[i]);
        
        gpu_float_type tmp1 = (tmp*tmp)/Qmergedcnt;
        
        gpu_float_type tmp2 = censumQ1.cnt * censumQ2.cnt * tmp1;
        
        censumQ1.trace[i] = censumQ1.trace[i] + censumQ2.trace[i] + tmp2;
    }
    censumQ1.cnt = Qmergedcnt;
}


void computeMeanOfMeans(const std::vector<combtrace> &means, gpu_float_type *output, uint32_t pointsPerTrace){
    uint32_t members = 0;
    memset(output, 0, sizeof(gpu_float_type)*pointsPerTrace);
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

void computeVariancesOfMeans(const std::vector<combtrace> &means, const gpu_float_type *meanOfMeans, gpu_float_type* output, uint32_t pointsPerTrace){
    uint32_t members = 0;
    memset(output, 0, sizeof(gpu_float_type)*pointsPerTrace);
    for (auto &m : means) {
        
        if (m.cnt == 0) continue; //skip if this trace has no members
        
        for (uint32_t i=0; i<pointsPerTrace; i++) {
            gpu_float_type tmp = 0;
            tmp = m.trace[i] - meanOfMeans[i];
            output[i] += tmp*tmp;
        }
        members++;
    }

    for (uint32_t i=0; i<pointsPerTrace; i++) {
        output[i] /= members;
    }
}

void computeMeanOfVariancesFromCensums(const std::vector<combtrace> &cenSums, gpu_float_type *output, uint32_t pointsPerTrace){
    uint32_t members = 0;
    memset(output, 0, sizeof(gpu_float_type)*pointsPerTrace);
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
