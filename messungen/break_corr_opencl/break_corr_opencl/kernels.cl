
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define AES_BLOCK_SELECTOR 4

#define AES_BLOCKS_TOTAL 8 //8*16 = 128bytes

#define KEY_GUESS_RANGE 0x100


#warning custom point_per_trace
#define NUMBER_OF_POINTS_OVERWRITE 8000


struct MeasurementStructure{
    uint    NumberOfPoints;

    uchar   _shiftp[AES_BLOCK_SELECTOR*16]; //48
    uchar   Input[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16]; //8*16 - 3*16 = 5*16
//128
    uchar   _shiftc[AES_BLOCK_SELECTOR*16];
    uchar   Output[AES_BLOCKS_TOTAL*16 - AES_BLOCK_SELECTOR*16];

    char    Trace[0];
};

struct Tracefile{
    uint  tracesInFile;
    struct MeasurementStructure ms[0];
};


// Test/Debug Key
__constant uchar gaeskey[16*2] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

__constant uchar sbox[256] = {
0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};


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

inline uchar hamming_weight_p128(const uchar s[16]){
    return
    hamming_weight(s[ 0]) +
    hamming_weight(s[ 1]) +
    hamming_weight(s[ 2]) +
    hamming_weight(s[ 3]) +
    hamming_weight(s[ 4]) +
    hamming_weight(s[ 5]) +
    hamming_weight(s[ 6]) +
    hamming_weight(s[ 7]) +
    hamming_weight(s[ 8]) +
    hamming_weight(s[10]) +
    hamming_weight(s[11]) +
    hamming_weight(s[12]) +
    hamming_weight(s[13]) +
    hamming_weight(s[14]) +
    hamming_weight(s[15]);
}

inline uchar hamming_weight_p64(const uchar s[8]){
    return
    hamming_weight(s[ 0]) +
    hamming_weight(s[ 1]) +
    hamming_weight(s[ 2]) +
    hamming_weight(s[ 3]) +
    hamming_weight(s[ 4]) +
    hamming_weight(s[ 5]) +
    hamming_weight(s[ 6]) +
    hamming_weight(s[ 7]);
}

inline uchar hamming_weight_p32(const uchar s[4]){
    return
    hamming_weight(s[ 0]) +
    hamming_weight(s[ 1]) +
    hamming_weight(s[ 2]) +
    hamming_weight(s[ 3]);
}

inline uchar hamming_weight_p16(const uchar s[2]){
    return
    hamming_weight(s[ 0]) +
    hamming_weight(s[ 1]);
}


inline void MixColumns(uchar* state){
#define xtime(x) ((x<<1) ^ (((x>>7) & 1) * 0x1b))
    uchar Tmp, Tm, t;
    for (uchar i = 0; i < 4; ++i)
    {
      t   = state[i*4+0];
      Tmp = state[i*4+0] ^ state[i*4+1] ^ state[i*4+2] ^ state[i*4+3] ;
      Tm  = state[i*4+0] ^ state[i*4+1] ; Tm = xtime(Tm);  state[i*4+0] ^= Tm ^ Tmp ;
      Tm  = state[i*4+1] ^ state[i*4+2] ; Tm = xtime(Tm);  state[i*4+1] ^= Tm ^ Tmp ;
      Tm  = state[i*4+2] ^ state[i*4+3] ; Tm = xtime(Tm);  state[i*4+2] ^= Tm ^ Tmp ;
      Tm  = state[i*4+3] ^ t ;            Tm = xtime(Tm);  state[i*4+3] ^= Tm ^ Tmp ;
    }
}

#define Multiply(x, y)                          \
(  ((y & 1) * x) ^                              \
((y>>1 & 1) * xtime(x)) ^                       \
((y>>2 & 1) * xtime(xtime(x))) ^                \
((y>>3 & 1) * xtime(xtime(xtime(x)))) ^         \
((y>>4 & 1) * xtime(xtime(xtime(xtime(x))))))   \

inline void InvMixColumns(uchar* state){
  uchar a, b, c, d;
  for (uchar i = 0; i < 4; ++i)
  {
    a = state[i*4+0];
    b = state[i*4+1];
    c = state[i*4+2];
    d = state[i*4+3];

    state[i*4+0] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
    state[i*4+1] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
    state[i*4+2] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
    state[i*4+3] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
  }
}



#define KEY_SELECTOR 0

//BEGIN TRACE SELECTORS
#if KEY_SELECTOR < 16
inline uchar powerModel_ATK(__global const struct MeasurementStructure *msm, const uint key_val){
        uchar state0[16];
        uchar state1[16];
        
#if AES_BLOCK_SELECTOR == 0
#   define plaintext(i) (msm->Output[ i])
#else
#   define plaintext(i) (msm->Output[ i] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + i])
#endif

        state0[ 0] = plaintext( 0);
        state0[ 1] = plaintext( 1);
        state0[ 2] = plaintext( 2);
        state0[ 3] = plaintext( 3);
        state0[ 4] = plaintext( 4);
        state0[ 5] = plaintext( 5);
        state0[ 6] = plaintext( 6);
        state0[ 7] = plaintext( 7);
        state0[ 8] = plaintext( 8);
        state0[ 9] = plaintext( 9);
        state0[10] = plaintext(10);
        state0[11] = plaintext(11);
        state0[12] = plaintext(12);
        state0[13] = plaintext(13);
        state0[14] = plaintext(14);
        state0[15] = plaintext(15);

        /* This is for 0-15 */
        //remove real key (will be added in next step) and add target key
        state0[KEY_SELECTOR] ^= gaeskey[KEY_SELECTOR] ^ (key_val & 0xff);

        
        state1[ 0] = sbox[state0[ 0] ^ gaeskey[ 0]];
        state1[13] = sbox[state0[ 1] ^ gaeskey[ 1]];
        state1[10] = sbox[state0[ 2] ^ gaeskey[ 2]];
        state1[ 7] = sbox[state0[ 3] ^ gaeskey[ 3]];

        state1[ 4] = sbox[state0[ 4] ^ gaeskey[ 4]];
        state1[ 1] = sbox[state0[ 5] ^ gaeskey[ 5]];
        state1[14] = sbox[state0[ 6] ^ gaeskey[ 6]];
        state1[11] = sbox[state0[ 7] ^ gaeskey[ 7]];

        state1[ 8] = sbox[state0[ 8] ^ gaeskey[ 8]];
        state1[ 5] = sbox[state0[ 9] ^ gaeskey[ 9]];
        state1[ 2] = sbox[state0[10] ^ gaeskey[10]];
        state1[15] = sbox[state0[11] ^ gaeskey[11]];

        state1[12] = sbox[state0[12] ^ gaeskey[12]];
        state1[ 9] = sbox[state0[13] ^ gaeskey[13]];
        state1[ 6] = sbox[state0[14] ^ gaeskey[14]];
        state1[ 3] = sbox[state0[15] ^ gaeskey[15]];

        //now this is after sbox, after perm

        //undo modification to state0
        state0[KEY_SELECTOR] ^= gaeskey[KEY_SELECTOR] ^ (key_val & 0xff);

    
        state1[ 0] ^= state0[ 0];
        state1[ 1] ^= state0[ 1];
        state1[ 2] ^= state0[ 2];
        state1[ 3] ^= state0[ 3];
        state1[ 4] ^= state0[ 4];
        state1[ 5] ^= state0[ 5];
        state1[ 6] ^= state0[ 6];
        state1[ 7] ^= state0[ 7];
        state1[ 8] ^= state0[ 8];
        state1[ 9] ^= state0[ 9];
        state1[10] ^= state0[10];
        state1[11] ^= state0[11];
        state1[12] ^= state0[12];
        state1[13] ^= state0[13];
        state1[14] ^= state0[14];
        state1[15] ^= state0[15];

    return hamming_weight(state1[((KEY_SELECTOR + 16 - 4 * (KEY_SELECTOR % 4)) % 16)]);
}
#elif KEY_SELECTOR < 32
inline uchar powerModel_ATK(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar state0[16];
    uchar state1[16];
    
#if AES_BLOCK_SELECTOR == 0
#   define plaintext(i) (msm->Output[ i])
#else
#   define plaintext(i) (msm->Output[ i] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + i])
#endif

    state0[ 0] = plaintext( 0);
    state0[ 1] = plaintext( 1);
    state0[ 2] = plaintext( 2);
    state0[ 3] = plaintext( 3);
    state0[ 4] = plaintext( 4);
    state0[ 5] = plaintext( 5);
    state0[ 6] = plaintext( 6);
    state0[ 7] = plaintext( 7);
    state0[ 8] = plaintext( 8);
    state0[ 9] = plaintext( 9);
    state0[10] = plaintext(10);
    state0[11] = plaintext(11);
    state0[12] = plaintext(12);
    state0[13] = plaintext(13);
    state0[14] = plaintext(14);
    state0[15] = plaintext(15);

    
    //start round 1
    
    state1[ 0] = sbox[state0[ 0] ^ gaeskey[ 0]];
    state1[13] = sbox[state0[ 1] ^ gaeskey[ 1]];
    state1[10] = sbox[state0[ 2] ^ gaeskey[ 2]];
    state1[ 7] = sbox[state0[ 3] ^ gaeskey[ 3]];

    state1[ 4] = sbox[state0[ 4] ^ gaeskey[ 4]];
    state1[ 1] = sbox[state0[ 5] ^ gaeskey[ 5]];
    state1[14] = sbox[state0[ 6] ^ gaeskey[ 6]];
    state1[11] = sbox[state0[ 7] ^ gaeskey[ 7]];

    state1[ 8] = sbox[state0[ 8] ^ gaeskey[ 8]];
    state1[ 5] = sbox[state0[ 9] ^ gaeskey[ 9]];
    state1[ 2] = sbox[state0[10] ^ gaeskey[10]];
    state1[15] = sbox[state0[11] ^ gaeskey[11]];

    state1[12] = sbox[state0[12] ^ gaeskey[12]];
    state1[ 9] = sbox[state0[13] ^ gaeskey[13]];
    state1[ 6] = sbox[state0[14] ^ gaeskey[14]];
    state1[ 3] = sbox[state0[15] ^ gaeskey[15]];

    //now this is after sbox, after perm

    MixColumns(state1);
    
    //now this is after mult

    /* This is for 16-31 */
    //remove real key (will be added in next step) and add target key
    state1[(KEY_SELECTOR-16)] ^= gaeskey[16+(KEY_SELECTOR-16)] ^ (key_val & 0xff);

    state0[ 0] = sbox[state1[ 0] ^ gaeskey[16+ 0]];
    state0[13] = sbox[state1[ 1] ^ gaeskey[16+ 1]];
    state0[10] = sbox[state1[ 2] ^ gaeskey[16+ 2]];
    state0[ 7] = sbox[state1[ 3] ^ gaeskey[16+ 3]];

    state0[ 4] = sbox[state1[ 4] ^ gaeskey[16+ 4]];
    state0[ 1] = sbox[state1[ 5] ^ gaeskey[16+ 5]];
    state0[14] = sbox[state1[ 6] ^ gaeskey[16+ 6]];
    state0[11] = sbox[state1[ 7] ^ gaeskey[16+ 7]];

    state0[ 8] = sbox[state1[ 8] ^ gaeskey[16+ 8]];
    state0[ 5] = sbox[state1[ 9] ^ gaeskey[16+ 9]];
    state0[ 2] = sbox[state1[10] ^ gaeskey[16+10]];
    state0[15] = sbox[state1[11] ^ gaeskey[16+11]];

    state0[12] = sbox[state1[12] ^ gaeskey[16+12]];
    state0[ 9] = sbox[state1[13] ^ gaeskey[16+13]];
    state0[ 6] = sbox[state1[14] ^ gaeskey[16+14]];
    state0[ 3] = sbox[state1[15] ^ gaeskey[16+15]];

    
    //now this is after sbox2, after perm2

    //undo modification to state1
    state1[(KEY_SELECTOR-16)] ^= gaeskey[16+(KEY_SELECTOR-16)] ^ (key_val & 0xff);
    InvMixColumns(state1);
    
    //state 1 is back to being after only 1 round

    state1[ 0] ^= state0[ 0];
    state1[ 1] ^= state0[ 1];
    state1[ 2] ^= state0[ 2];
    state1[ 3] ^= state0[ 3];
    state1[ 4] ^= state0[ 4];
    state1[ 5] ^= state0[ 5];
    state1[ 6] ^= state0[ 6];
    state1[ 7] ^= state0[ 7];
    state1[ 8] ^= state0[ 8];
    state1[ 9] ^= state0[ 9];
    state1[10] ^= state0[10];
    state1[11] ^= state0[11];
    state1[12] ^= state0[12];
    state1[13] ^= state0[13];
    state1[14] ^= state0[14];
    state1[15] ^= state0[15];

    return hamming_weight(state1[(((KEY_SELECTOR-16) + 16 - 4 * ((KEY_SELECTOR-16) % 4)) % 16)]);
}
#else
#error keyselector out of range
#endif

//END TRACE SELECTORS

__kernel void computeMean(__global const struct Tracefile *tr, __global double *output, __global ulong *curcntMean){
    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
    
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;

    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

#ifdef NUMBER_OF_POINTS_OVERWRITE
    pointsPerTrace = NUMBER_OF_POINTS_OVERWRITE;
#endif
    
    if (i>=pointsPerTrace) return; //these guys are out of bounds
    
    double factor = (1.0/tracesInFile);
    
    double out = 0; //clear buffer before access
    
    __global const struct MeasurementStructure *msm = tr->ms;
    for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){
        out += msm->Trace[i] * factor;
    }
    
    output[i] = out; //store output
    
    
    //only thread 0 should write curcntMean
    if (i == 0) *curcntMean = tracesInFile;
}

__kernel void computeMeanHypot(__global const struct Tracefile *tr, __global double *meanHypot, __global ulong *meanHypotCnt){
    uint k = get_global_id(0) + get_global_id(1) * get_global_size(0);
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;
    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

    if (k>=KEY_GUESS_RANGE) return; //these guys are out of bounds
    
    double factor = (1.0/tracesInFile);
    
    double out = 0; //clean buffer before access
    
    __global const struct MeasurementStructure *msm = tr->ms;
    for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){
        uchar hypot = powerModel(msm,k);
        out += hypot * factor;
    }
    
    meanHypot[k] = out;
    meanHypotCnt[k] = tracesInFile;
}

__kernel void computeCenteredSum(__global const struct Tracefile *tr, __global const double *mean, __global const double *meansHypot, __global double *censum, __global ulong *censumCnt){
    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
    
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;
    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

#ifdef NUMBER_OF_POINTS_OVERWRITE
    pointsPerTrace = NUMBER_OF_POINTS_OVERWRITE;
#endif
    if (i>=pointsPerTrace) return; //these guys are out of bounds

    double curMean = mean[i];

    double out = 0; //clean buffer before access

    __global const struct MeasurementStructure *msm = tr->ms;
    for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){

        //normal censum
        double tmp = msm->Trace[i] - curMean;
        out += tmp*tmp;
    }
    
    censum[i] = out;
    
    
    //only thread 0 should write curcntMean
    if (i == 0) *censumCnt = tracesInFile;
}

__kernel void computeUpper(__global const struct Tracefile *tr, __global const double *mean, __global const double *meansHypot, __global double *upperPart){
    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
    
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;
    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

#ifdef NUMBER_OF_POINTS_OVERWRITE
    pointsPerTrace = NUMBER_OF_POINTS_OVERWRITE;
#endif
    if (i>=pointsPerTrace) return; //these guys are out of bounds
    
    
    double curMean = mean[i];
        
    for (uint k=0; k < KEY_GUESS_RANGE; k++){

        double curMeansHypot = meansHypot[k];
        
        double curUpperPartVal = 0;
        
        __global const struct MeasurementStructure *msm = tr->ms;
        for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){
            //normal censum
            double tmp = msm->Trace[i] - curMean;

            uchar hypot = powerModel(msm,k);
            double hypotTmp = hypot - curMeansHypot;
            curUpperPartVal += hypotTmp*tmp;
        }
        
        upperPart[k*pointsPerTrace + i] = curUpperPartVal;
    }
}

__kernel void computeCenteredSumHypot(__global const struct Tracefile *tr, __global const double *meansHypot, __global double *censumHypot, __global ulong *censumHypotCnt){
    uint k = get_global_id(0) + get_global_id(1) * get_global_size(0);
    uint tracesInFile = tr->tracesInFile;
    uint pointsPerTrace = tr->ms->NumberOfPoints;
    uint nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

    if (k>=KEY_GUESS_RANGE) return; //these guys are out of bounds

    double curMeansHypot = meansHypot[k];
    
    double out = 0; //clean buffer before access
    
    __global const struct MeasurementStructure *msm = tr->ms;
    for (uint j=0; j < tracesInFile; j++,msm = (__global const struct MeasurementStructure *)(((__global uchar*)msm)+nextMsmOffset)){

        //hypot
        uchar hypot = powerModel(msm,k);
        double hypotTmp = hypot - curMeansHypot;
        
        out += hypotTmp*hypotTmp;
    }
    
    censumHypot[k] = out;
    censumHypotCnt[k] = tracesInFile;
}

__kernel void combineMeanAndCenteredSum(__global double *censumQ1, __global ulong *censumQ1cnt, __global const double *censumQ2, __global const ulong *censumQ2cnt, __global double *meanQ1, __global ulong *meanQ1cnt, __global const double *meanQ2, __global const ulong *meanQ2cnt, uint pointsPerTrace){

    uint i = get_global_id(0) + get_global_id(1) * get_global_size(0);
   
    if (i>=pointsPerTrace) return; //these guys are out of bounds
    
   /* COMBINE MEANS */

    ulong meanQmergedcnt = *meanQ1cnt + *meanQ2cnt;
    if (meanQmergedcnt){
        double tmp0 = meanQ2[i] - meanQ1[i];
        double tmp = (*meanQ2cnt * tmp0) / meanQmergedcnt;

        meanQ1[i] = meanQ1[i] + tmp;
    }

    
    /* COMBINE CENSUMS */

    ulong censumQmergedcnt = *censumQ1cnt + *censumQ2cnt;
    if (censumQmergedcnt){
        double tmp = meanQ2[i] - meanQ1[i];

        double tmp1 = (tmp*tmp) / censumQmergedcnt;

        double tmp2 = (*censumQ1cnt) * (*censumQ2cnt) * tmp1;

        censumQ1[i] = censumQ1[i] + censumQ2[i] + tmp2;
    }
}

__kernel void mergeCoutners(__global ulong *censumQ1cnt, __global const ulong *censumQ2cnt, __global ulong *meanQ1cnt, __global const ulong *meanQ2cnt, uint countersCnt){
    uint s = get_global_id(0) + get_global_id(1) * get_global_size(0);
    if (s>=countersCnt) return; //these guys are out of bounds
    
    meanQ1cnt[s] += meanQ2cnt[s];
    censumQ1cnt[s] += censumQ2cnt[s];
}

__kernel void kernelAddTrace(__global double *v1, __global double *v2, uint countersCnt){
    uint s = get_global_id(0) + get_global_id(1) * get_global_size(0);
    if (s>=countersCnt) return; //these guys are out of bounds
    
    v1[s] += v2[s];
}
