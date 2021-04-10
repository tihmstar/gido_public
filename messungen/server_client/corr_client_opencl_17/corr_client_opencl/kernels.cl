
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define AES_BLOCK_SELECTOR 4

#define AES_BLOCKS_TOTAL 8 //8*16 = 128bytes

#define KEY_GUESS_RANGE 0x100


//#warning custom point_per_trace
//#define NUMBER_OF_POINTS_OVERWRITE 2000
//
//#define OFFSET_START_OVERWRITE 9000


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


/* Test/Debug Key*/
__constant uchar gaeskey[16*2] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

/*
 TEST KEY
 */
__constant uchar aeskey[16*15] = {
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

__constant uint T0_inv[256]={
0x50a7f451,0x5365417e,0xc3a4171a,0x965e273a,0xcb6bab3b,0xf1459d1f,0xab58faac,0x9303e34b,0x55fa3020,0xf66d76ad,0x9176cc88,0x254c02f5,0xfcd7e54f,0xd7cb2ac5,0x80443526,0x8fa362b5,
0x495ab1de,0x671bba25,0x980eea45,0xe1c0fe5d,0x02752fc3,0x12f04c81,0xa397468d,0xc6f9d36b,0xe75f8f03,0x959c9215,0xeb7a6dbf,0xda595295,0x2d83bed4,0xd3217458,0x2969e049,0x44c8c98e,
0x6a89c275,0x78798ef4,0x6b3e5899,0xdd71b927,0xb64fe1be,0x17ad88f0,0x66ac20c9,0xb43ace7d,0x184adf63,0x82311ae5,0x60335197,0x457f5362,0xe07764b1,0x84ae6bbb,0x1ca081fe,0x942b08f9,
0x58684870,0x19fd458f,0x876cde94,0xb7f87b52,0x23d373ab,0xe2024b72,0x578f1fe3,0x2aab5566,0x0728ebb2,0x03c2b52f,0x9a7bc586,0xa50837d3,0xf2872830,0xb2a5bf23,0xba6a0302,0x5c8216ed,
0x2b1ccf8a,0x92b479a7,0xf0f207f3,0xa1e2694e,0xcdf4da65,0xd5be0506,0x1f6234d1,0x8afea6c4,0x9d532e34,0xa055f3a2,0x32e18a05,0x75ebf6a4,0x39ec830b,0xaaef6040,0x069f715e,0x51106ebd,
0xf98a213e,0x3d06dd96,0xae053edd,0x46bde64d,0xb58d5491,0x055dc471,0x6fd40604,0xff155060,0x24fb9819,0x97e9bdd6,0xcc434089,0x779ed967,0xbd42e8b0,0x888b8907,0x385b19e7,0xdbeec879,
0x470a7ca1,0xe90f427c,0xc91e84f8,0x00000000,0x83868009,0x48ed2b32,0xac70111e,0x4e725a6c,0xfbff0efd,0x5638850f,0x1ed5ae3d,0x27392d36,0x64d90f0a,0x21a65c68,0xd1545b9b,0x3a2e3624,
0xb1670a0c,0x0fe75793,0xd296eeb4,0x9e919b1b,0x4fc5c080,0xa220dc61,0x694b775a,0x161a121c,0x0aba93e2,0xe52aa0c0,0x43e0223c,0x1d171b12,0x0b0d090e,0xadc78bf2,0xb9a8b62d,0xc8a91e14,
0x8519f157,0x4c0775af,0xbbdd99ee,0xfd607fa3,0x9f2601f7,0xbcf5725c,0xc53b6644,0x347efb5b,0x7629438b,0xdcc623cb,0x68fcedb6,0x63f1e4b8,0xcadc31d7,0x10856342,0x40229713,0x2011c684,
0x7d244a85,0xf83dbbd2,0x1132f9ae,0x6da129c7,0x4b2f9e1d,0xf330b2dc,0xec52860d,0xd0e3c177,0x6c16b32b,0x99b970a9,0xfa489411,0x2264e947,0xc48cfca8,0x1a3ff0a0,0xd82c7d56,0xef903322,
0xc74e4987,0xc1d138d9,0xfea2ca8c,0x360bd498,0xcf81f5a6,0x28de7aa5,0x268eb7da,0xa4bfad3f,0xe49d3a2c,0x0d927850,0x9bcc5f6a,0x62467e54,0xc2138df6,0xe8b8d890,0x5ef7392e,0xf5afc382,
0xbe805d9f,0x7c93d069,0xa92dd56f,0xb31225cf,0x3b99acc8,0xa77d1810,0x6e639ce8,0x7bbb3bdb,0x097826cd,0xf418596e,0x01b79aec,0xa89a4f83,0x656e95e6,0x7ee6ffaa,0x08cfbc21,0xe6e815ef,
0xd99be7ba,0xce366f4a,0xd4099fea,0xd67cb029,0xafb2a431,0x31233f2a,0x3094a5c6,0xc066a235,0x37bc4e74,0xa6ca82fc,0xb0d090e0,0x15d8a733,0x4a9804f1,0xf7daec41,0x0e50cd7f,0x2ff69117,
0x8dd64d76,0x4db0ef43,0x544daacc,0xdf0496e4,0xe3b5d19e,0x1b886a4c,0xb81f2cc1,0x7f516546,0x04ea5e9d,0x5d358c01,0x737487fa,0x2e410bfb,0x5a1d67b3,0x52d2db92,0x335610e9,0x1347d66d,
0x8c61d79a,0x7a0ca137,0x8e14f859,0x893c13eb,0xee27a9ce,0x35c961b7,0xede51ce1,0x3cb1477a,0x59dfd29c,0x3f73f255,0x79ce1418,0xbf37c773,0xeacdf753,0x5baafd5f,0x146f3ddf,0x86db4478,
0x81f3afca,0x3ec468b9,0x2c342438,0x5f40a3c2,0x72c31d16,0x0c25e2bc,0x8b493c28,0x41950dff,0x7101a839,0xdeb30c08,0x9ce4b4d8,0x90c15664,0x6184cb7b,0x70b632d5,0x745c6c48,0x4257b8d0};

__constant uint T1_inv[256]={
0xa7f45150,0x65417e53,0xa4171ac3,0x5e273a96,0x6bab3bcb,0x459d1ff1,0x58faacab,0x03e34b93,0xfa302055,0x6d76adf6,0x76cc8891,0x4c02f525,0xd7e54ffc,0xcb2ac5d7,0x44352680,0xa362b58f,
0x5ab1de49,0x1bba2567,0x0eea4598,0xc0fe5de1,0x752fc302,0xf04c8112,0x97468da3,0xf9d36bc6,0x5f8f03e7,0x9c921595,0x7a6dbfeb,0x595295da,0x83bed42d,0x217458d3,0x69e04929,0xc8c98e44,
0x89c2756a,0x798ef478,0x3e58996b,0x71b927dd,0x4fe1beb6,0xad88f017,0xac20c966,0x3ace7db4,0x4adf6318,0x311ae582,0x33519760,0x7f536245,0x7764b1e0,0xae6bbb84,0xa081fe1c,0x2b08f994,
0x68487058,0xfd458f19,0x6cde9487,0xf87b52b7,0xd373ab23,0x024b72e2,0x8f1fe357,0xab55662a,0x28ebb207,0xc2b52f03,0x7bc5869a,0x0837d3a5,0x872830f2,0xa5bf23b2,0x6a0302ba,0x8216ed5c,
0x1ccf8a2b,0xb479a792,0xf207f3f0,0xe2694ea1,0xf4da65cd,0xbe0506d5,0x6234d11f,0xfea6c48a,0x532e349d,0x55f3a2a0,0xe18a0532,0xebf6a475,0xec830b39,0xef6040aa,0x9f715e06,0x106ebd51,
0x8a213ef9,0x06dd963d,0x053eddae,0xbde64d46,0x8d5491b5,0x5dc47105,0xd406046f,0x155060ff,0xfb981924,0xe9bdd697,0x434089cc,0x9ed96777,0x42e8b0bd,0x8b890788,0x5b19e738,0xeec879db,
0x0a7ca147,0x0f427ce9,0x1e84f8c9,0x00000000,0x86800983,0xed2b3248,0x70111eac,0x725a6c4e,0xff0efdfb,0x38850f56,0xd5ae3d1e,0x392d3627,0xd90f0a64,0xa65c6821,0x545b9bd1,0x2e36243a,
0x670a0cb1,0xe757930f,0x96eeb4d2,0x919b1b9e,0xc5c0804f,0x20dc61a2,0x4b775a69,0x1a121c16,0xba93e20a,0x2aa0c0e5,0xe0223c43,0x171b121d,0x0d090e0b,0xc78bf2ad,0xa8b62db9,0xa91e14c8,
0x19f15785,0x0775af4c,0xdd99eebb,0x607fa3fd,0x2601f79f,0xf5725cbc,0x3b6644c5,0x7efb5b34,0x29438b76,0xc623cbdc,0xfcedb668,0xf1e4b863,0xdc31d7ca,0x85634210,0x22971340,0x11c68420,
0x244a857d,0x3dbbd2f8,0x32f9ae11,0xa129c76d,0x2f9e1d4b,0x30b2dcf3,0x52860dec,0xe3c177d0,0x16b32b6c,0xb970a999,0x489411fa,0x64e94722,0x8cfca8c4,0x3ff0a01a,0x2c7d56d8,0x903322ef,
0x4e4987c7,0xd138d9c1,0xa2ca8cfe,0x0bd49836,0x81f5a6cf,0xde7aa528,0x8eb7da26,0xbfad3fa4,0x9d3a2ce4,0x9278500d,0xcc5f6a9b,0x467e5462,0x138df6c2,0xb8d890e8,0xf7392e5e,0xafc382f5,
0x805d9fbe,0x93d0697c,0x2dd56fa9,0x1225cfb3,0x99acc83b,0x7d1810a7,0x639ce86e,0xbb3bdb7b,0x7826cd09,0x18596ef4,0xb79aec01,0x9a4f83a8,0x6e95e665,0xe6ffaa7e,0xcfbc2108,0xe815efe6,
0x9be7bad9,0x366f4ace,0x099fead4,0x7cb029d6,0xb2a431af,0x233f2a31,0x94a5c630,0x66a235c0,0xbc4e7437,0xca82fca6,0xd090e0b0,0xd8a73315,0x9804f14a,0xdaec41f7,0x50cd7f0e,0xf691172f,
0xd64d768d,0xb0ef434d,0x4daacc54,0x0496e4df,0xb5d19ee3,0x886a4c1b,0x1f2cc1b8,0x5165467f,0xea5e9d04,0x358c015d,0x7487fa73,0x410bfb2e,0x1d67b35a,0xd2db9252,0x5610e933,0x47d66d13,
0x61d79a8c,0x0ca1377a,0x14f8598e,0x3c13eb89,0x27a9ceee,0xc961b735,0xe51ce1ed,0xb1477a3c,0xdfd29c59,0x73f2553f,0xce141879,0x37c773bf,0xcdf753ea,0xaafd5f5b,0x6f3ddf14,0xdb447886,
0xf3afca81,0xc468b93e,0x3424382c,0x40a3c25f,0xc31d1672,0x25e2bc0c,0x493c288b,0x950dff41,0x01a83971,0xb30c08de,0xe4b4d89c,0xc1566490,0x84cb7b61,0xb632d570,0x5c6c4874,0x57b8d042};

__constant uint T2_inv[256]={
0xf45150a7,0x417e5365,0x171ac3a4,0x273a965e,0xab3bcb6b,0x9d1ff145,0xfaacab58,0xe34b9303,0x302055fa,0x76adf66d,0xcc889176,0x02f5254c,0xe54ffcd7,0x2ac5d7cb,0x35268044,0x62b58fa3,
0xb1de495a,0xba25671b,0xea45980e,0xfe5de1c0,0x2fc30275,0x4c8112f0,0x468da397,0xd36bc6f9,0x8f03e75f,0x9215959c,0x6dbfeb7a,0x5295da59,0xbed42d83,0x7458d321,0xe0492969,0xc98e44c8,
0xc2756a89,0x8ef47879,0x58996b3e,0xb927dd71,0xe1beb64f,0x88f017ad,0x20c966ac,0xce7db43a,0xdf63184a,0x1ae58231,0x51976033,0x5362457f,0x64b1e077,0x6bbb84ae,0x81fe1ca0,0x08f9942b,
0x48705868,0x458f19fd,0xde94876c,0x7b52b7f8,0x73ab23d3,0x4b72e202,0x1fe3578f,0x55662aab,0xebb20728,0xb52f03c2,0xc5869a7b,0x37d3a508,0x2830f287,0xbf23b2a5,0x0302ba6a,0x16ed5c82,
0xcf8a2b1c,0x79a792b4,0x07f3f0f2,0x694ea1e2,0xda65cdf4,0x0506d5be,0x34d11f62,0xa6c48afe,0x2e349d53,0xf3a2a055,0x8a0532e1,0xf6a475eb,0x830b39ec,0x6040aaef,0x715e069f,0x6ebd5110,
0x213ef98a,0xdd963d06,0x3eddae05,0xe64d46bd,0x5491b58d,0xc471055d,0x06046fd4,0x5060ff15,0x981924fb,0xbdd697e9,0x4089cc43,0xd967779e,0xe8b0bd42,0x8907888b,0x19e7385b,0xc879dbee,
0x7ca1470a,0x427ce90f,0x84f8c91e,0x00000000,0x80098386,0x2b3248ed,0x111eac70,0x5a6c4e72,0x0efdfbff,0x850f5638,0xae3d1ed5,0x2d362739,0x0f0a64d9,0x5c6821a6,0x5b9bd154,0x36243a2e,
0x0a0cb167,0x57930fe7,0xeeb4d296,0x9b1b9e91,0xc0804fc5,0xdc61a220,0x775a694b,0x121c161a,0x93e20aba,0xa0c0e52a,0x223c43e0,0x1b121d17,0x090e0b0d,0x8bf2adc7,0xb62db9a8,0x1e14c8a9,
0xf1578519,0x75af4c07,0x99eebbdd,0x7fa3fd60,0x01f79f26,0x725cbcf5,0x6644c53b,0xfb5b347e,0x438b7629,0x23cbdcc6,0xedb668fc,0xe4b863f1,0x31d7cadc,0x63421085,0x97134022,0xc6842011,
0x4a857d24,0xbbd2f83d,0xf9ae1132,0x29c76da1,0x9e1d4b2f,0xb2dcf330,0x860dec52,0xc177d0e3,0xb32b6c16,0x70a999b9,0x9411fa48,0xe9472264,0xfca8c48c,0xf0a01a3f,0x7d56d82c,0x3322ef90,
0x4987c74e,0x38d9c1d1,0xca8cfea2,0xd498360b,0xf5a6cf81,0x7aa528de,0xb7da268e,0xad3fa4bf,0x3a2ce49d,0x78500d92,0x5f6a9bcc,0x7e546246,0x8df6c213,0xd890e8b8,0x392e5ef7,0xc382f5af,
0x5d9fbe80,0xd0697c93,0xd56fa92d,0x25cfb312,0xacc83b99,0x1810a77d,0x9ce86e63,0x3bdb7bbb,0x26cd0978,0x596ef418,0x9aec01b7,0x4f83a89a,0x95e6656e,0xffaa7ee6,0xbc2108cf,0x15efe6e8,
0xe7bad99b,0x6f4ace36,0x9fead409,0xb029d67c,0xa431afb2,0x3f2a3123,0xa5c63094,0xa235c066,0x4e7437bc,0x82fca6ca,0x90e0b0d0,0xa73315d8,0x04f14a98,0xec41f7da,0xcd7f0e50,0x91172ff6,
0x4d768dd6,0xef434db0,0xaacc544d,0x96e4df04,0xd19ee3b5,0x6a4c1b88,0x2cc1b81f,0x65467f51,0x5e9d04ea,0x8c015d35,0x87fa7374,0x0bfb2e41,0x67b35a1d,0xdb9252d2,0x10e93356,0xd66d1347,
0xd79a8c61,0xa1377a0c,0xf8598e14,0x13eb893c,0xa9ceee27,0x61b735c9,0x1ce1ede5,0x477a3cb1,0xd29c59df,0xf2553f73,0x141879ce,0xc773bf37,0xf753eacd,0xfd5f5baa,0x3ddf146f,0x447886db,
0xafca81f3,0x68b93ec4,0x24382c34,0xa3c25f40,0x1d1672c3,0xe2bc0c25,0x3c288b49,0x0dff4195,0xa8397101,0x0c08deb3,0xb4d89ce4,0x566490c1,0xcb7b6184,0x32d570b6,0x6c48745c,0xb8d04257};

__constant uint T3_inv[256]={
0x5150a7f4,0x7e536541,0x1ac3a417,0x3a965e27,0x3bcb6bab,0x1ff1459d,0xacab58fa,0x4b9303e3,0x2055fa30,0xadf66d76,0x889176cc,0xf5254c02,0x4ffcd7e5,0xc5d7cb2a,0x26804435,0xb58fa362,
0xde495ab1,0x25671bba,0x45980eea,0x5de1c0fe,0xc302752f,0x8112f04c,0x8da39746,0x6bc6f9d3,0x03e75f8f,0x15959c92,0xbfeb7a6d,0x95da5952,0xd42d83be,0x58d32174,0x492969e0,0x8e44c8c9,
0x756a89c2,0xf478798e,0x996b3e58,0x27dd71b9,0xbeb64fe1,0xf017ad88,0xc966ac20,0x7db43ace,0x63184adf,0xe582311a,0x97603351,0x62457f53,0xb1e07764,0xbb84ae6b,0xfe1ca081,0xf9942b08,
0x70586848,0x8f19fd45,0x94876cde,0x52b7f87b,0xab23d373,0x72e2024b,0xe3578f1f,0x662aab55,0xb20728eb,0x2f03c2b5,0x869a7bc5,0xd3a50837,0x30f28728,0x23b2a5bf,0x02ba6a03,0xed5c8216,
0x8a2b1ccf,0xa792b479,0xf3f0f207,0x4ea1e269,0x65cdf4da,0x06d5be05,0xd11f6234,0xc48afea6,0x349d532e,0xa2a055f3,0x0532e18a,0xa475ebf6,0x0b39ec83,0x40aaef60,0x5e069f71,0xbd51106e,
0x3ef98a21,0x963d06dd,0xddae053e,0x4d46bde6,0x91b58d54,0x71055dc4,0x046fd406,0x60ff1550,0x1924fb98,0xd697e9bd,0x89cc4340,0x67779ed9,0xb0bd42e8,0x07888b89,0xe7385b19,0x79dbeec8,
0xa1470a7c,0x7ce90f42,0xf8c91e84,0x00000000,0x09838680,0x3248ed2b,0x1eac7011,0x6c4e725a,0xfdfbff0e,0x0f563885,0x3d1ed5ae,0x3627392d,0x0a64d90f,0x6821a65c,0x9bd1545b,0x243a2e36,
0x0cb1670a,0x930fe757,0xb4d296ee,0x1b9e919b,0x804fc5c0,0x61a220dc,0x5a694b77,0x1c161a12,0xe20aba93,0xc0e52aa0,0x3c43e022,0x121d171b,0x0e0b0d09,0xf2adc78b,0x2db9a8b6,0x14c8a91e,
0x578519f1,0xaf4c0775,0xeebbdd99,0xa3fd607f,0xf79f2601,0x5cbcf572,0x44c53b66,0x5b347efb,0x8b762943,0xcbdcc623,0xb668fced,0xb863f1e4,0xd7cadc31,0x42108563,0x13402297,0x842011c6,
0x857d244a,0xd2f83dbb,0xae1132f9,0xc76da129,0x1d4b2f9e,0xdcf330b2,0x0dec5286,0x77d0e3c1,0x2b6c16b3,0xa999b970,0x11fa4894,0x472264e9,0xa8c48cfc,0xa01a3ff0,0x56d82c7d,0x22ef9033,
0x87c74e49,0xd9c1d138,0x8cfea2ca,0x98360bd4,0xa6cf81f5,0xa528de7a,0xda268eb7,0x3fa4bfad,0x2ce49d3a,0x500d9278,0x6a9bcc5f,0x5462467e,0xf6c2138d,0x90e8b8d8,0x2e5ef739,0x82f5afc3,
0x9fbe805d,0x697c93d0,0x6fa92dd5,0xcfb31225,0xc83b99ac,0x10a77d18,0xe86e639c,0xdb7bbb3b,0xcd097826,0x6ef41859,0xec01b79a,0x83a89a4f,0xe6656e95,0xaa7ee6ff,0x2108cfbc,0xefe6e815,
0xbad99be7,0x4ace366f,0xead4099f,0x29d67cb0,0x31afb2a4,0x2a31233f,0xc63094a5,0x35c066a2,0x7437bc4e,0xfca6ca82,0xe0b0d090,0x3315d8a7,0xf14a9804,0x41f7daec,0x7f0e50cd,0x172ff691,
0x768dd64d,0x434db0ef,0xcc544daa,0xe4df0496,0x9ee3b5d1,0x4c1b886a,0xc1b81f2c,0x467f5165,0x9d04ea5e,0x015d358c,0xfa737487,0xfb2e410b,0xb35a1d67,0x9252d2db,0xe9335610,0x6d1347d6,
0x9a8c61d7,0x377a0ca1,0x598e14f8,0xeb893c13,0xceee27a9,0xb735c961,0xe1ede51c,0x7a3cb147,0x9c59dfd2,0x553f73f2,0x1879ce14,0x73bf37c7,0x53eacdf7,0x5f5baafd,0xdf146f3d,0x7886db44,
0xca81f3af,0xb93ec468,0x382c3424,0xc25f40a3,0x1672c31d,0xbc0c25e2,0x288b493c,0xff41950d,0x397101a8,0x08deb30c,0xd89ce4b4,0x6490c156,0x7b6184cb,0xd570b632,0x48745c6c,0xd04257b8};

__constant uint IK0[256] = {
0x00000000,0x0b0d090e,0x161a121c,0x1d171b12,0x2c342438,0x27392d36,0x3a2e3624,0x31233f2a,0x58684870,0x5365417e,0x4e725a6c,0x457f5362,0x745c6c48,0x7f516546,0x62467e54,0x694b775a,
0xb0d090e0,0xbbdd99ee,0xa6ca82fc,0xadc78bf2,0x9ce4b4d8,0x97e9bdd6,0x8afea6c4,0x81f3afca,0xe8b8d890,0xe3b5d19e,0xfea2ca8c,0xf5afc382,0xc48cfca8,0xcf81f5a6,0xd296eeb4,0xd99be7ba,
0x7bbb3bdb,0x70b632d5,0x6da129c7,0x66ac20c9,0x578f1fe3,0x5c8216ed,0x41950dff,0x4a9804f1,0x23d373ab,0x28de7aa5,0x35c961b7,0x3ec468b9,0x0fe75793,0x04ea5e9d,0x19fd458f,0x12f04c81,
0xcb6bab3b,0xc066a235,0xdd71b927,0xd67cb029,0xe75f8f03,0xec52860d,0xf1459d1f,0xfa489411,0x9303e34b,0x980eea45,0x8519f157,0x8e14f859,0xbf37c773,0xb43ace7d,0xa92dd56f,0xa220dc61,
0xf66d76ad,0xfd607fa3,0xe07764b1,0xeb7a6dbf,0xda595295,0xd1545b9b,0xcc434089,0xc74e4987,0xae053edd,0xa50837d3,0xb81f2cc1,0xb31225cf,0x82311ae5,0x893c13eb,0x942b08f9,0x9f2601f7,
0x46bde64d,0x4db0ef43,0x50a7f451,0x5baafd5f,0x6a89c275,0x6184cb7b,0x7c93d069,0x779ed967,0x1ed5ae3d,0x15d8a733,0x08cfbc21,0x03c2b52f,0x32e18a05,0x39ec830b,0x24fb9819,0x2ff69117,
0x8dd64d76,0x86db4478,0x9bcc5f6a,0x90c15664,0xa1e2694e,0xaaef6040,0xb7f87b52,0xbcf5725c,0xd5be0506,0xdeb30c08,0xc3a4171a,0xc8a91e14,0xf98a213e,0xf2872830,0xef903322,0xe49d3a2c,
0x3d06dd96,0x360bd498,0x2b1ccf8a,0x2011c684,0x1132f9ae,0x1a3ff0a0,0x0728ebb2,0x0c25e2bc,0x656e95e6,0x6e639ce8,0x737487fa,0x78798ef4,0x495ab1de,0x4257b8d0,0x5f40a3c2,0x544daacc,
0xf7daec41,0xfcd7e54f,0xe1c0fe5d,0xeacdf753,0xdbeec879,0xd0e3c177,0xcdf4da65,0xc6f9d36b,0xafb2a431,0xa4bfad3f,0xb9a8b62d,0xb2a5bf23,0x83868009,0x888b8907,0x959c9215,0x9e919b1b,
0x470a7ca1,0x4c0775af,0x51106ebd,0x5a1d67b3,0x6b3e5899,0x60335197,0x7d244a85,0x7629438b,0x1f6234d1,0x146f3ddf,0x097826cd,0x02752fc3,0x335610e9,0x385b19e7,0x254c02f5,0x2e410bfb,
0x8c61d79a,0x876cde94,0x9a7bc586,0x9176cc88,0xa055f3a2,0xab58faac,0xb64fe1be,0xbd42e8b0,0xd4099fea,0xdf0496e4,0xc2138df6,0xc91e84f8,0xf83dbbd2,0xf330b2dc,0xee27a9ce,0xe52aa0c0,
0x3cb1477a,0x37bc4e74,0x2aab5566,0x21a65c68,0x10856342,0x1b886a4c,0x069f715e,0x0d927850,0x64d90f0a,0x6fd40604,0x72c31d16,0x79ce1418,0x48ed2b32,0x43e0223c,0x5ef7392e,0x55fa3020,
0x01b79aec,0x0aba93e2,0x17ad88f0,0x1ca081fe,0x2d83bed4,0x268eb7da,0x3b99acc8,0x3094a5c6,0x59dfd29c,0x52d2db92,0x4fc5c080,0x44c8c98e,0x75ebf6a4,0x7ee6ffaa,0x63f1e4b8,0x68fcedb6,
0xb1670a0c,0xba6a0302,0xa77d1810,0xac70111e,0x9d532e34,0x965e273a,0x8b493c28,0x80443526,0xe90f427c,0xe2024b72,0xff155060,0xf418596e,0xc53b6644,0xce366f4a,0xd3217458,0xd82c7d56,
0x7a0ca137,0x7101a839,0x6c16b32b,0x671bba25,0x5638850f,0x5d358c01,0x40229713,0x4b2f9e1d,0x2264e947,0x2969e049,0x347efb5b,0x3f73f255,0x0e50cd7f,0x055dc471,0x184adf63,0x1347d66d,
0xcadc31d7,0xc1d138d9,0xdcc623cb,0xd7cb2ac5,0xe6e815ef,0xede51ce1,0xf0f207f3,0xfbff0efd,0x92b479a7,0x99b970a9,0x84ae6bbb,0x8fa362b5,0xbe805d9f,0xb58d5491,0xa89a4f83,0xa397468d
};

#define INV_SUBROUND_0 \
        w[0] = T0_inv[ptextAddkey[ 0]] ^ T1_inv[ptextAddkey[13]] ^ T2_inv[ptextAddkey[10]] ^ T3_inv[ptextAddkey[ 7]] \
            ^ IK0[aeskey[(14-i)*16 + 0 ]] \
            ^ ((IK0[aeskey[(14-i)*16 + 1 ]]<<8 )   ^ (IK0[aeskey[(14-i)*16 + 1]]>>24)) \
            ^ ((IK0[aeskey[(14-i)*16 + 2 ]]<<16)   ^ (IK0[aeskey[(14-i)*16 + 2]]>>16)) \
            ^ ((IK0[aeskey[(14-i)*16 + 3 ]]<<24)   ^ (IK0[aeskey[(14-i)*16 + 3]]>>8))

#define INV_SUBROUND_1 \
        w[1] = T0_inv[ptextAddkey[ 4]] ^ T1_inv[ptextAddkey[ 1]] ^ T2_inv[ptextAddkey[14]] ^ T3_inv[ptextAddkey[11]] \
            ^ IK0[aeskey[(14-i)*16 + 4 ]] \
            ^ ((IK0[aeskey[(14-i)*16 + 5 ]]<<8 )   ^ (IK0[aeskey[(14-i)*16 + 5]]>>24)) \
            ^ ((IK0[aeskey[(14-i)*16 + 6 ]]<<16)   ^ (IK0[aeskey[(14-i)*16 + 6]]>>16)) \
            ^ ((IK0[aeskey[(14-i)*16 + 7 ]]<<24)   ^ (IK0[aeskey[(14-i)*16 + 7]]>>8))

#define INV_SUBROUND_2 \
        w[2] = T0_inv[ptextAddkey[ 8]] ^ T1_inv[ptextAddkey[ 5]] ^ T2_inv[ptextAddkey[ 2]] ^ T3_inv[ptextAddkey[15]] \
            ^ IK0[aeskey[(14-i)*16 + 8 ]] \
            ^ ((IK0[aeskey[(14-i)*16 + 9 ]]<<8 )   ^ (IK0[aeskey[(14-i)*16 + 9]]>>24)) \
            ^ ((IK0[aeskey[(14-i)*16 + 10]]<<16)   ^ (IK0[aeskey[(14-i)*16 + 10]]>>16)) \
            ^ ((IK0[aeskey[(14-i)*16 + 11]]<<24)   ^ (IK0[aeskey[(14-i)*16 + 11]]>>8))

#define INV_SUBROUND_3 \
        w[3] = T0_inv[ptextAddkey[12]] ^ T1_inv[ptextAddkey[ 9]] ^ T2_inv[ptextAddkey[ 6]] ^ T3_inv[ptextAddkey[ 3]] \
            ^ IK0[aeskey[(14-i)*16 + 12 ]] \
            ^ ((IK0[aeskey[(14-i)*16 + 13]]<<8 )   ^ (IK0[aeskey[(14-i)*16 + 13]]>>24)) \
            ^ ((IK0[aeskey[(14-i)*16 + 14]]<<16)   ^ (IK0[aeskey[(14-i)*16 + 14]]>>16)) \
            ^ ((IK0[aeskey[(14-i)*16 + 15]]<<24)   ^ (IK0[aeskey[(14-i)*16 + 15]]>>8)) \


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

#define G_SELECTOR_ROUND KEY_SELECTOR
inline uchar powerModel_ROUND_XOR_NEXT_HW128(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p128(ptextAddkey);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW64_0(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p64(&ptextAddkey[0]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW64_1(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p64(&ptextAddkey[8]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW32_0(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p32(&ptextAddkey[0]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW32_1(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p32(&ptextAddkey[4]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW32_2(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p32(&ptextAddkey[8]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW32_3(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p32(&ptextAddkey[12]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW16_0(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p16(&ptextAddkey[0]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW16_1(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight_p16(&ptextAddkey[2]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW8_0(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight(ptextAddkey[0]);
}

inline uchar powerModel_ROUND_XOR_NEXT_HW8_1(__global const struct MeasurementStructure *msm, const uint key_val){
    uchar ptextAddkey[16];
    ptextAddkey[ 0] = msm->Input[ 0] ^ aeskey[16*14 +  0];
    ptextAddkey[ 1] = msm->Input[ 1] ^ aeskey[16*14 +  1];
    ptextAddkey[ 2] = msm->Input[ 2] ^ aeskey[16*14 +  2];
    ptextAddkey[ 3] = msm->Input[ 3] ^ aeskey[16*14 +  3];
    ptextAddkey[ 4] = msm->Input[ 4] ^ aeskey[16*14 +  4];
    ptextAddkey[ 5] = msm->Input[ 5] ^ aeskey[16*14 +  5];
    ptextAddkey[ 6] = msm->Input[ 6] ^ aeskey[16*14 +  6];
    ptextAddkey[ 7] = msm->Input[ 7] ^ aeskey[16*14 +  7];
    ptextAddkey[ 8] = msm->Input[ 8] ^ aeskey[16*14 +  8];
    ptextAddkey[ 9] = msm->Input[ 9] ^ aeskey[16*14 +  9];
    ptextAddkey[10] = msm->Input[10] ^ aeskey[16*14 + 10];
    ptextAddkey[11] = msm->Input[11] ^ aeskey[16*14 + 11];
    ptextAddkey[12] = msm->Input[12] ^ aeskey[16*14 + 12];
    ptextAddkey[13] = msm->Input[13] ^ aeskey[16*14 + 13];
    ptextAddkey[14] = msm->Input[14] ^ aeskey[16*14 + 14];
    ptextAddkey[15] = msm->Input[15] ^ aeskey[16*14 + 15];

    uint w[4] = {0};

    for (int i=1; i<G_SELECTOR_ROUND+1; i++) { //4 middle rounds
        INV_SUBROUND_0;
        INV_SUBROUND_1;
        INV_SUBROUND_2;
        INV_SUBROUND_3;
        
        if (i == G_SELECTOR_ROUND){ //this is last round
            if (i == 14){
#if AES_BLOCK_SELECTOR == 0
                ptextAddkey[ 0] ^= msm->Output[ 0];
                ptextAddkey[ 1] ^= msm->Output[ 1];
                ptextAddkey[ 2] ^= msm->Output[ 2];
                ptextAddkey[ 3] ^= msm->Output[ 3];
                ptextAddkey[ 4] ^= msm->Output[ 4];
                ptextAddkey[ 5] ^= msm->Output[ 5];
                ptextAddkey[ 6] ^= msm->Output[ 6];
                ptextAddkey[ 7] ^= msm->Output[ 7];
                ptextAddkey[ 8] ^= msm->Output[ 8];
                ptextAddkey[ 9] ^= msm->Output[ 9];
                ptextAddkey[10] ^= msm->Output[10];
                ptextAddkey[11] ^= msm->Output[11];
                ptextAddkey[12] ^= msm->Output[12];
                ptextAddkey[13] ^= msm->Output[13];
                ptextAddkey[14] ^= msm->Output[14];
                ptextAddkey[15] ^= msm->Output[15];
#else
                ptextAddkey[ 0] ^= msm->Output[ 0] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 0];
                ptextAddkey[ 1] ^= msm->Output[ 1] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 1];
                ptextAddkey[ 2] ^= msm->Output[ 2] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 2];
                ptextAddkey[ 3] ^= msm->Output[ 3] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 3];
                ptextAddkey[ 4] ^= msm->Output[ 4] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 4];
                ptextAddkey[ 5] ^= msm->Output[ 5] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 5];
                ptextAddkey[ 6] ^= msm->Output[ 6] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 6];
                ptextAddkey[ 7] ^= msm->Output[ 7] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 7];
                ptextAddkey[ 8] ^= msm->Output[ 8] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 8];
                ptextAddkey[ 9] ^= msm->Output[ 9] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 + 9];
                ptextAddkey[10] ^= msm->Output[10] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +10];
                ptextAddkey[11] ^= msm->Output[11] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +11];
                ptextAddkey[12] ^= msm->Output[12] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +12];
                ptextAddkey[13] ^= msm->Output[13] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +13];
                ptextAddkey[14] ^= msm->Output[14] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +14];
                ptextAddkey[15] ^= msm->Output[15] ^ msm->_shiftp[AES_BLOCK_SELECTOR*16 -16 +15];
#endif
            }else{
                *(uint*)&ptextAddkey[ 0] ^= w[0];
                *(uint*)&ptextAddkey[ 4] ^= w[1];
                *(uint*)&ptextAddkey[ 8] ^= w[2];
                *(uint*)&ptextAddkey[12] ^= w[3];
            }
            break;
        }
        
        *(uint*)&ptextAddkey[ 0] = w[0];
        *(uint*)&ptextAddkey[ 4] = w[1];
        *(uint*)&ptextAddkey[ 8] = w[2];
        *(uint*)&ptextAddkey[12] = w[3];
    }
    
    return hamming_weight(ptextAddkey[1]);
}


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
#ifdef OFFSET_START_OVERWRITE
        out += msm->Trace[OFFSET_START_OVERWRITE+i] * factor;
#else
        out += msm->Trace[i] * factor;
#endif
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
#ifdef OFFSET_START_OVERWRITE
        double tmp = msm->Trace[OFFSET_START_OVERWRITE+i] - curMean;
#else
        double tmp = msm->Trace[i] - curMean;
#endif

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
#ifdef OFFSET_START_OVERWRITE
        double tmp = msm->Trace[OFFSET_START_OVERWRITE+i] - curMean;
#else
        double tmp = msm->Trace[i] - curMean;
#endif


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
