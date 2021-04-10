//
//  powermodel.hpp
//  corr_client_cpu
//
//  Created by tihmstar on 21.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef powermodel_hpp
#define powermodel_hpp

#include <stdint.h>

#define AES_BLOCK_SELECTOR 3

#define AES_BLOCKS_TOTAL 8 //8*16 = 128bytes

#define KEY_GUESS_RANGE 0x100


struct MeasurementStructure{
    uint32_t    NumberOfPoints;

    uint8_t   _shiftp[AES_BLOCK_SELECTOR*16]; //48
    uint8_t   Input[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16]; //8*16 - 3*16 = 5*16
//128
    uint8_t   _shiftc[AES_BLOCK_SELECTOR*16];
    uint8_t   Output[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16];

    int8_t    Trace[0];
};

struct Tracefile{
    uint32_t  tracesInFile;
    struct MeasurementStructure ms[0];
};

uint8_t powerModel_ATK(const struct MeasurementStructure *msm, const uint32_t key_val, const uint8_t key_selector);



#endif /* powermodel_hpp */
