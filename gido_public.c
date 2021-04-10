//
//  main.c
//  gido
//

#include <stdint.h>
#include <stdlib.h>

#pragma mark myAES

uint8_t sbox[256] =
{
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
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

uint8_t roundkeys[11][16] =
  {
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
      {0xd6, 0xaa, 0x74, 0xfd, 0xd2, 0xaf, 0x72, 0xfa, 0xda, 0xa6, 0x78, 0xf1, 0xd6, 0xab, 0x76, 0xfe},
      {0xb6, 0x92, 0xcf, 0x0b, 0x64, 0x3d, 0xbd, 0xf1, 0xbe, 0x9b, 0xc5, 0x00, 0x68, 0x30, 0xb3, 0xfe},
      {0xb6, 0xff, 0x74, 0x4e, 0xd2, 0xc2, 0xc9, 0xbf, 0x6c, 0x59, 0x0c, 0xbf, 0x04, 0x69, 0xbf, 0x41},
      {0x47, 0xf7, 0xf7, 0xbc, 0x95, 0x35, 0x3e, 0x03, 0xf9, 0x6c, 0x32, 0xbc, 0xfd, 0x05, 0x8d, 0xfd},
      {0x3c, 0xaa, 0xa3, 0xe8, 0xa9, 0x9f, 0x9d, 0xeb, 0x50, 0xf3, 0xaf, 0x57, 0xad, 0xf6, 0x22, 0xaa},
      {0x5e, 0x39, 0x0f, 0x7d, 0xf7, 0xa6, 0x92, 0x96, 0xa7, 0x55, 0x3d, 0xc1, 0x0a, 0xa3, 0x1f, 0x6b},
      {0x14, 0xf9, 0x70, 0x1a, 0xe3, 0x5f, 0xe2, 0x8c, 0x44, 0x0a, 0xdf, 0x4d, 0x4e, 0xa9, 0xc0, 0x26},
      {0x47, 0x43, 0x87, 0x35, 0xa4, 0x1c, 0x65, 0xb9, 0xe0, 0x16, 0xba, 0xf4, 0xae, 0xbf, 0x7a, 0xd2},
      {0x54, 0x99, 0x32, 0xd1, 0xf0, 0x85, 0x57, 0x68, 0x10, 0x93, 0xed, 0x9c, 0xbe, 0x2c, 0x97, 0x4e},
      {0x13, 0x11, 0x1d, 0x7f, 0xe3, 0x94, 0x4a, 0x17, 0xf3, 0x07, 0xa7, 0x8b, 0x4d, 0x2b, 0x30, 0xc5}
  };

uint8_t fixedKey[32] ={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

void aes_precompute_tables();
void aes_encrypt(uint8_t plaintext[16], uint8_t roundkeys[11][16], uint8_t ciphertext[16]);


uint32_t *T0 = (uint32_t*)(0x41414141);
uint32_t *T1 = (uint32_t*)(0x41414141);
uint32_t *T2 = (uint32_t*)(0x41414141);
uint32_t *T3 = (uint32_t*)(0x41414141);

#pragma mark typedefs

typedef struct CmdArg {
    signed int unk1;
    unsigned int uinteger;
    signed int integer;
    unsigned int type;
    char* string;
} CmdArg;

typedef struct CmdInfo {
    char* name;
    void *handler;
    char* description;
} CmdInfo;

typedef enum {
    kAesEncrypt = 0x10,
    kAesDecrypt = 0x11
} AesOption;

typedef enum {
    kAesType128 = 0x00000000,
    kAesType192 = 0x10000000,
    kAesType256 = 0x20000000
} AesType;

typedef enum {
    kAesSize128 = 0x20,
    kAesSize192 = 0x28,
    kAesSize256 = 0x30
} AesSize;

typedef enum {
    kAesTypeUser = 0x0,
    kAesTypeGid = 0x200,
    kAesTypeUid = 0x201
} AesMode;

#define AES_CMD_ECB	      (0x00000000)
#define AES_CMD_CBC	      (0x00000010)

#pragma mark ibec funcs

int (*_printf)(const char *,...) = (void*)(0x5ff33ca4+1);
int (*_snprintf)(char *str, size_t size, const char *format, ...) = (void*)(0x5ff3425c+1);
void *(*_malloc)(size_t) = (void*)(0x5ff185cc+1);
void (*_free)(void*) = (void*)(0x5ff18680+1);
int(*aes_crypto_cmd)(AesOption option, void* input, void* output, unsigned int size, AesMode mode, void* key, void* iv) = (void*)(0x5ff208ac+1);
int (*uart_getc)(int port, int wait) = (void*)(0x5ff14748+1);
int (*uart_puts)(int port, const char *s) = (void*)(0x5ff1472c+1);


uint32_t (*gpio_read)(uint32_t gpio) = (void*)(0x5ff02768+1);
void (*gpio_write)(uint32_t gpio, uint32_t val) = (void*)(0x5ff02790+1);
void (*gpio_configure)(uint32_t gpio, uint32_t config) = (void*)(0x5ff0280c+1);

void (*watchdog_tickle)(void) = (void*)(0x5ff1f88c+1);

int (*env_set)(const char *name, const char *val, uint32_t zero) = (void*)(0x5ff16ef0+1);
size_t (*env_get_uint)(const char *name, size_t fallback) = (void*)(0x5ff16d5c+1);

#define GPIO_POWER    1
#define GPIO_VOL_UP   2
#define GPIO_VOL_DOWN 3

#pragma mark ibec constants
CmdInfo ***ibec_cmd_struct_pointer = (void*)(0x5ff1ed3c);

#pragma mark my functions
int cmd_echo(int argc, CmdArg* argv);
int cmd_md(int argc, CmdArg* argv);
int cmd_mw(int argc, CmdArg* argv);
int aes_cmd(int argc, CmdArg* argv);
int cmd_call(int argc, CmdArg* argv);
int cmd_measure(int argc, CmdArg* argv);
int cmd_decypt(int argc, CmdArg* argv);


unsigned int aes_encrypt_key(char* in, char* out, unsigned int size);
unsigned int aes_decrypt_key(char* in, char* out, unsigned int size);
unsigned int aes_encrypt_key_custom(char* in, char* out, unsigned int size, void *key);
unsigned int aes_decrypt_key_custom(char* in, char* out, unsigned int size, void *key);
void hook_aes_func();


void DumpHex(const void* data, size_t size);
int parseHex(const char *instr, char *ret, size_t *outSize);

void memset(void *a, unsigned char b, size_t len);
void memmove(void *a, void *b, size_t len);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, uint32_t n);
int strlen(const char *s);
long atol(const char *s);
void clear_icache();


// /* --- this HAS to be TOP function! --- */
int main(int argc, const char * argv[]);
void reposition();
asm(
  "b _reposition\n\t"
  ".skip 0x100\n\t"
);

void reposition(){
  uint8_t *newspace = _malloc(0x6000);
  newspace += 0x1000;
  newspace = (uint8_t*)(((uint32_t)newspace) & ~0xfff);
  memmove(newspace, 0x40000000, 0x6000-0x1000);

  uint8_t *np = main;
  np -= 0x40000000;
  np += (uint32_t)newspace;
  np = (uint8_t*)(((uint32_t)np) | 1);

  void (*newmain)() = np;

  clear_icache();
  newmain();
}

int main(int argc, const char * argv[]) {
    CmdInfo **newArgs = NULL;
    _printf("Initializing GIDO!\n");

    CmdInfo *bootx = (void*)0x5ff419c0;
    bootx->name = "echo";
    bootx->handler = cmd_echo;
    bootx->description = "echo something to console";

    CmdInfo *memboot = (void*)0x5ff419d0;
    memboot->name = "aes";
    memboot->handler = aes_cmd;
    memboot->description = "aes talk to aes engin";

    CmdInfo *ticket = (void*)0x5ff4217c;
    ticket->name = "md";
    ticket->handler = cmd_md;
    ticket->description = "dump memory";

    CmdInfo *devicetree = (void*)0x5ff4211c;
    devicetree->name = "decrypt";
    devicetree->handler = cmd_decypt;
    devicetree->description = "decrypt AES hook";

    CmdInfo *setpicture = (void*)0x5ff41a54;
    setpicture->name = "mw";
    setpicture->handler = cmd_mw;
    setpicture->description = "write memory";

    CmdInfo *reboot = (void*)0x5ff419f4;
    reboot->name = "call";
    reboot->handler = cmd_call;
    reboot->description = "call function";

    CmdInfo *go = (void*)0x5ff41a64;
    go->name = "measure";
    go->handler = cmd_measure;
    go->description = "switch to measure mode";

    _printf("reconfiguring VOL_DOWN button ... ");
    gpio_configure(GPIO_VOL_DOWN,1); //set gpio to be output
    gpio_write(GPIO_VOL_DOWN,0);
    _printf("ok\n");

    _printf("precompute TTables ... ");
    aes_precompute_tables();
    _printf("ok\n");

    hook_aes_func();

    _printf("Done GIDO!\n");
    return 0;
}

void aes_function_hook(){ //realAES
asm(
    //prepare GPIO write
    "movs r4, #0xc\n\t"
    "movt r4, #0xbfa0\n\t"
    "mov  r0, #0x92\n\t"
    "mov  r1, #0x93\n\t"


    //prepare DMA access
    "movw r5, #0x2000 \t\n" //RX (r1)
    "movt r5, #0x8700 \t\n"

    "ldr r7, [r5]\t\n" //RX_val
    "orr r7, r7, #0x1\t\n" //RX_val
    "str r7, [r5]\t\n" //store RX  first!


    "movw r6, #0x1000 \t\n"
    "movt r6, #0x8700 \t\n" //TX (r0)

    "ldr r7, [r6]\t\n" //TX_val
    "orr r7, r7, #0x1\t\n" //TX_val

   //about to enter time critical section!
   "strb r1, [r4]\n\t"  //set GPIO high (AES_START)
   //START_CRITICAL_SECTION

    "str r7, [r6]\t\n" //store TX (AES_START)

    "w1:\t\n"
    "ldr r7, [r5]\t\n" //RX_val
    "and r7, r7, #0x30000\t\n" //load RX_val and check for idle
    "cmp r7, #0x10000\t\n"
    "beq w1\t\n"
     //-- END_CRITICAL_SECTION
     "strb r0, [r4]\n\t" //set GPIO low
    );
}


void hook_aes_func(){
    _printf("hook_aes_func\n");
    char trampolin[] = "\xDF\xF8\x02\x00\x01\xE0\x44\x43\x42\x41\x80\x47";
    void **dstVal = (void **)&trampolin[6];
    *dstVal = aes_function_hook;

    uint16_t *startHook = (void*)0x5ff02138;
    uint16_t *endHook = (void*)0x5ff0215a; //not inclusive

    for (int i=0; i<sizeof(trampolin)-1; i+=2) {
        *startHook = *(uint16_t*)&trampolin[i];
        startHook++;
    }

    while (startHook<endHook) {
        *startHook++ = 0xbf00; //nop unneeded stuff
    }
    clear_icache();
    _printf("hook_aes_func done!\n");
}

int cmd_echo(int argc, CmdArg* argv) {
    int i = 0;
    if(argc >= 2) {
        for(i = 1; i < argc; i++) {
            _printf("%s ", argv[i].string);
        }
        _printf("\n");
        return 0;
    }
    _printf("usage: echo <message>\n");
    return 0;
}

uint32_t bswap32(uint32_t num){
  return ((num>>24)&0xff) | // move byte 3 to byte 0
                    ((num<<8)&0xff0000) | // move byte 1 to byte 2
                    ((num>>8)&0xff00) | // move byte 2 to byte 1
                    ((num<<24)&0xff000000); // byte 0 to byte 3
}

int cmd_decypt(int argc, CmdArg* argv) {
  size_t payloadSize = env_get_uint("filesize",0);
  char *inBuf = 0x40000000;

  char outBuf[0x300];
  char printBuf[0x600];
  aes_decrypt_key(inBuf, outBuf, payloadSize);

  for (int i = 0; i < payloadSize; i++) {
    unsigned char c = (unsigned char)outBuf[i];
    if((printBuf[2*i + 0] = c >> 4) < 10){
      printBuf[2*i + 0] += '0';
    }else{
      printBuf[2*i + 0] += 'a' - 10;
    }
    if((printBuf[2*i + 1] = c & 0xf) < 10){
      printBuf[2*i + 1] += '0';
    }else{
      printBuf[2*i + 1] += 'a' - 10;
    }
    printBuf[2*i + 2] = 0;
  }
  env_set("cmd-results",printBuf,0);
  watchdog_tickle();
  return 0;
}

int cmd_call(int argc, CmdArg* argv) {
    int i = 0;
    uint32_t args[4] = {0,0,0,0};
    uint32_t func = 0;
    if(argc >= 2) {
        size_t rt = 0;
        parseHex(argv[1].string, &func, &rt);
        func = bswap32(func);
        if (rt > 4){
          _printf("error funcpointer too large\n");
          return 0;
        }
        for (size_t i = 2; i-2 < 4 && i < argc; i++) {
          size_t rt = 0;
          parseHex(argv[i].string, &args[i-2], &rt);
          args[i-2] = bswap32(args[i-2]);
        }
        _printf("calling (0x%08x)(0x%08x,0x%08x,0x%08x,0x%08x) ",func,args[0],args[1],args[2],args[3]);

        uint32_t retval = ((uint32_t (*) (uint32_t,uint32_t,uint32_t,uint32_t))func)(args[0],args[1],args[2],args[3]);
        _printf("reval=0x%08x (%lu)\n",retval,retval);
        return 0;
    }
    _printf("usage: call <func> <args...>\n");
    return 0;
}

int cmd_measure(int argc, CmdArg* argv) {
  uart_puts(0,"enter measure mode\n");
  char buf[0x200];
  char printBuf[100];

  _printf("disable Idle poweroff ...");
  *(uint16_t*)0x5ff00f68 = 0x4770;
  clear_icache();
  _printf("ok\n");

  //patch sched_yield to return 0;
  *((uint16_t*)0x5ff1f008) = 0x4770;
  clear_icache();

  while (gpio_read(GPIO_VOL_UP)) {
    memset(buf, 0, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf) && gpio_read(GPIO_VOL_UP); i++) {
      while ((buf[i] = uart_getc(0,0)) == -1 && gpio_read(GPIO_VOL_UP));
      if (i > 4 && !strncmp(buf+i-3,"END",3)) {
        uart_puts(0,"[M] found END tag!\n");
        buf[i] = '\0';//zero terminate buf

        if (i> 10 && !strncmp(buf,"BEGIN",5)) {
          uart_puts(0,"[M] valid command detected\n");
          size_t rt = 0;
          uint8_t plainFixed[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
          int seedsLen = 0;
          uint8_t seeds[3][16];
          unsigned long encCount = 0;
          unsigned long mode = 0;
          uint8_t cipherEnc[16];
          uint8_t aesIn[48];
          uint8_t aesOut[48];

          //expecting command like: "BEGIN<seed0><seed1> <enccount> <mode>"

          //zero terminate after seeds
          char *encStr = &buf[5/*BEGIN*/];
          while (*++encStr != ' '){
            if (*encStr == 0){
              uart_puts(0,"[M] failed parsing encStr!\n");
              break;
            }
          }
          *encStr = '\0';
          if (parseHex(&buf[5],&seeds,&rt) || (rt != 16 && rt != 32 && rt != 48)){
            uart_puts(0,"[M] failed parsing seeds failed!\n");
            break;
          }
          seedsLen = rt;

          if (!(encCount = atol(&buf[5/*BEGIN*/+2*seedsLen/*<seed0><seed1>*/+1/*space*/]))){
            uart_puts(0,"[M] failed parsing encCount!\n");
            break;
          }

          char *modeStr = &buf[5/*BEGIN*/+2*seedsLen/*<seed0><seed1>*/+1/*space*/];
          while (*++modeStr != ' '){
            if (*modeStr == 0){
              uart_puts(0,"[M] failed parsing modeStr!\n");
              break;
            }
          }

          if (!(mode = atol(modeStr))){
            uart_puts(0,"[M] failed parsing mode!\n");
            break;
          }

          if (mode == 1) { //MODE: staticVSrandom
            if (seedsLen != 32){
              uart_puts(0,"[M] invalid seedsLen=");
              _snprintf(printBuf, sizeof(printBuf), "%d", seedsLen);
              uart_puts(0,printBuf);
              uart_puts(0," but expected 32\n");
              break;
            }
            uint8_t cipherSel[16];
            uart_puts(0,"[M] seed encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] seed select=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] encCount=");
            _snprintf(printBuf, sizeof(printBuf), "%lu\n",encCount);
            uart_puts(0,printBuf);


            for (long z = 0; z < encCount; z++) {
              aes_encrypt(seeds[0],roundkeys,cipherEnc);
              aes_encrypt(seeds[1],roundkeys,cipherSel);

              if (cipherSel[0] & 1) {
                aes_encrypt_key(cipherEnc, (char*)aesOut, 16);
              }else{
                aes_encrypt_key(plainFixed, (char*)aesOut, 16);
              }

              memmove(seeds[0], cipherEnc, 16);
              memmove(seeds[1], cipherSel, 16);
              watchdog_tickle();
            }

            gpio_write(GPIO_VOL_DOWN,1);
            gpio_write(GPIO_VOL_DOWN,0);


            uart_puts(0,"[M] final encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] final select=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n");
          }else if (mode == 2){ //MODE: random with userkey
            if (seedsLen != 48){
              uart_puts(0,"[M] invalid seedsLen=");
              _snprintf(printBuf, sizeof(printBuf), "%d", seedsLen);
              uart_puts(0,printBuf);
              uart_puts(0," but expected 48\n");
              break;
            }
            uint8_t cipherMesh[16];
            memset(cipherMesh, 0, sizeof(cipherMesh));

            uart_puts(0,"[M] seed encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] user key=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[2][0],seeds[2][1],seeds[2][2],seeds[2][3],seeds[2][4],seeds[2][5],seeds[2][6],seeds[2][7],seeds[2][8],seeds[2][9],seeds[2][10],seeds[2][11],seeds[2][12],seeds[2][13],seeds[2][14],seeds[2][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] encCount=");
            _snprintf(printBuf, sizeof(printBuf), "%lu\n",encCount);
            uart_puts(0,printBuf);

            for (long z = 0; z < encCount; z++) {
              aes_encrypt(seeds[0],roundkeys,cipherEnc);

              aes_encrypt_key_custom(cipherEnc, (char*)aesOut, 16, seeds[1]);

              memmove(seeds[0], cipherEnc, 16);

              ((uint64_t *)cipherMesh)[0] ^= ((uint64_t *)aesOut)[0];
              ((uint64_t *)cipherMesh)[1] ^= ((uint64_t *)aesOut)[1];

              watchdog_tickle();
            }

            gpio_write(GPIO_VOL_DOWN,1);
            gpio_write(GPIO_VOL_DOWN,0);

            uart_puts(0,"[M] final encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] final ciphermesh=");
            memmove(seeds[1], cipherMesh, 16);
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n");

          }else if (mode == 3){ //MODE: random with userkey decrypt cbc
            if (seedsLen != 48){
              uart_puts(0,"[M] invalid seedsLen=");
              _snprintf(printBuf, sizeof(printBuf), "%d", seedsLen);
              uart_puts(0,printBuf);
              uart_puts(0," but expected 48\n");
              break;
            }
            uint8_t plainmesh[16];
            memset(plainmesh, 0, sizeof(plainmesh));

            uart_puts(0,"[M] seed encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] user key=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[2][0],seeds[2][1],seeds[2][2],seeds[2][3],seeds[2][4],seeds[2][5],seeds[2][6],seeds[2][7],seeds[2][8],seeds[2][9],seeds[2][10],seeds[2][11],seeds[2][12],seeds[2][13],seeds[2][14],seeds[2][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] encCount=");
            _snprintf(printBuf, sizeof(printBuf), "%lu\n",encCount);
            uart_puts(0,printBuf);

            for (long z = 0; z < encCount; z++) {
              aes_encrypt(seeds[0],roundkeys,cipherEnc);

              for (size_t m = 0; m <sizeof(aesIn); m+=16) {
                  memmove(&aesIn[m], cipherEnc, 16);
              }

              aes_decrypt_key_custom((char*)aesIn, (char*)aesOut, sizeof(aesIn), seeds[1]);

              memmove(seeds[0], cipherEnc, 16);

              ((uint64_t *)plainmesh)[0] ^= ((uint64_t *)aesOut)[0];
              ((uint64_t *)plainmesh)[1] ^= ((uint64_t *)aesOut)[1];

              watchdog_tickle();
            }

            gpio_write(GPIO_VOL_DOWN,1);
            gpio_write(GPIO_VOL_DOWN,0);

            uart_puts(0,"[M] final encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] final plainmesh=");
            memmove(seeds[1], plainmesh, 16);
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n");

          }
          else if (mode == 4){ //MODE: random with userkey decrypt cbc,but 128 seq bytes
            if (seedsLen != 48){
              uart_puts(0,"[M] invalid seedsLen=");
              _snprintf(printBuf, sizeof(printBuf), "%d", seedsLen);
              uart_puts(0,printBuf);
              uart_puts(0," but expected 48\n");
              break;
            }
            uint8_t plainmesh[16];
            memset(plainmesh, 0, sizeof(plainmesh));

            uart_puts(0,"[M] seed encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] user key=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[2][0],seeds[2][1],seeds[2][2],seeds[2][3],seeds[2][4],seeds[2][5],seeds[2][6],seeds[2][7],seeds[2][8],seeds[2][9],seeds[2][10],seeds[2][11],seeds[2][12],seeds[2][13],seeds[2][14],seeds[2][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] encCount=");
            _snprintf(printBuf, sizeof(printBuf), "%lu\n",encCount);
            uart_puts(0,printBuf);

            uint8_t aesIn[128];
            uint8_t aesOut[128];

            for (long z = 0; z < encCount; z++) {
              for (int mult = 0; mult<sizeof(aesOut); mult +=16){
                aes_encrypt(seeds[0], roundkeys, cipherEnc);
                memmove(&aesIn[mult], cipherEnc, 16);
                memmove(seeds[0], cipherEnc, 16);
              }

              aes_decrypt_key_custom((char*)aesIn, (char*)aesOut, sizeof(aesIn), seeds[1]);

              memmove(seeds[0], cipherEnc, 16);

              for (int mult = 0; mult<sizeof(aesOut); mult +=16){
                ((uint64_t *)plainmesh)[0] ^= ((uint64_t *)aesOut)[(mult/16)*2];
                ((uint64_t *)plainmesh)[1] ^= ((uint64_t *)aesOut)[(mult/16)*2+1];
              }


              watchdog_tickle();
            }

            gpio_write(GPIO_VOL_DOWN,1);
            gpio_write(GPIO_VOL_DOWN,0);

            uart_puts(0,"[M] final encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] final plainmesh=");
            memmove(seeds[1], plainmesh, 16);
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n");

          }else if (mode == 5){ //MODE: random with GID key decrypt cbc,but 128 seq bytes
            if (seedsLen != 16){
              uart_puts(0,"[M] invalid seedsLen=");
              _snprintf(printBuf, sizeof(printBuf), "%d", seedsLen);
              uart_puts(0,printBuf);
              uart_puts(0," but expected 16\n");
              break;
            }
            uint8_t plainmesh[16];
            memset(plainmesh, 0, sizeof(plainmesh));

            uart_puts(0,"[M] seed encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] encCount=");
            _snprintf(printBuf, sizeof(printBuf), "%lu\n",encCount);
            uart_puts(0,printBuf);

            uint8_t aesIn[128];
            uint8_t aesOut[128];

            for (long z = 0; z < encCount; z++) {
              for (int mult = 0; mult<sizeof(aesOut); mult +=16){
                aes_encrypt(seeds[0], roundkeys, cipherEnc);
                memmove(&aesIn[mult], cipherEnc, 16);
                memmove(seeds[0], cipherEnc, 16);
              }

              aes_decrypt_key((char*)aesIn, (char*)aesOut, sizeof(aesIn));

              memmove(seeds[0], cipherEnc, 16);

              for (int mult = 0; mult<sizeof(aesOut); mult +=16){
                ((uint64_t *)plainmesh)[0] ^= ((uint64_t *)aesOut)[(mult/16)*2];
                ((uint64_t *)plainmesh)[1] ^= ((uint64_t *)aesOut)[(mult/16)*2+1];
              }


              watchdog_tickle();
            }

            gpio_write(GPIO_VOL_DOWN,1);
            gpio_write(GPIO_VOL_DOWN,0);

            uart_puts(0,"[M] final encrypt=");
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[0][0],seeds[0][1],seeds[0][2],seeds[0][3],seeds[0][4],seeds[0][5],seeds[0][6],seeds[0][7],seeds[0][8],seeds[0][9],seeds[0][10],seeds[0][11],seeds[0][12],seeds[0][13],seeds[0][14],seeds[0][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n[M] final plainmesh=");
            memmove(seeds[1], plainmesh, 16);
            _snprintf(printBuf, sizeof(printBuf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",seeds[1][0],seeds[1][1],seeds[1][2],seeds[1][3],seeds[1][4],seeds[1][5],seeds[1][6],seeds[1][7],seeds[1][8],seeds[1][9],seeds[1][10],seeds[1][11],seeds[1][12],seeds[1][13],seeds[1][14],seeds[1][15]);
            uart_puts(0,printBuf);
            uart_puts(0,"\n");

          }else{
            uart_puts(0,"[M] bad mode=");
            _snprintf(printBuf, sizeof(printBuf), "%lu", mode);
            uart_puts(0,printBuf);
            uart_puts(0,"\n");
            break;
          }

        }

        break;
      }
    }
    uart_puts(0,"[M] restart (MSG: \"BEGIN<16byte enc seed><16byte sel seed> <encCount> <mode>END\")\n");
  }
  uart_puts(0,"exit measure mode\n");

  //restore sched_yield to original value
  *((uint16_t*)0x5ff1f008) = 0xb5f0;
  clear_icache();

  return 0;
}

int cmd_md(int argc, CmdArg* argv){
    if(argc != 3) {
        _printf("usage: md <addr> <size>\n");
        return 0;
    }
    char *addrstr = argv[1].string;
    char *sizestr = argv[2].string;

    if (sizestr[0] == '0' && sizestr[1] == 'x') {
        sizestr+=2;
    }
    if (addrstr[0] == '0' && addrstr[1] == 'x') {
        addrstr+=2;
    }
    uint32_t addr = 0;
    uint32_t size = 0;
    size_t rt = 0;

    parseHex(addrstr,&addr,&rt);
    if (rt > 4) {
        _printf("ERROR: cmd_md addr %s too large (%d)\n",addrstr,rt);
        return 0;
    }
    addr = bswap32(addr);
    _printf("addrstr=%s (0x%08x)\n",addrstr,addr);

    parseHex(sizestr,(char*)&size,&rt);
    if (rt > 4) {
        _printf("ERROR: cmd_md size %s too large (%d)\n",sizestr,rt);
        return 0;
    }
    size = bswap32(size);

    _printf("dumping 0x%s (0x%08x) of size 0x%s (0x%08x)\n",addrstr,addr,sizestr,size);
    DumpHex((void*)addr,size);
    _printf("md done!\n");
    return 0;
}

int cmd_mw(int argc, CmdArg* argv){
    if(argc != 3) {
        _printf("usage: md <addr> <HEXDATA>\n");
        return 0;
    }
    char *addrstr = argv[1].string;
    char *datastr = argv[2].string;

    if (addrstr[0] == '0' && addrstr[1] == 'x') {
        addrstr+=2;
    }

    unsigned char *addr = 0;
    uint32_t size = 0;
    size_t rt = 0;
    uint32_t tmp = 0;


    size_t bindatalen = strlen(datastr);
    if (bindatalen & 1) {
        _printf("ERROR: cmd_mw data not multiple of 2\n");
        return 0;
    }

    unsigned char *bindata = _malloc(bindatalen+1);

    parseHex(addrstr,(char*)&addr,&rt);
    if (rt > 4) {
        _printf("ERROR: cmd_mw addr %s too large (%d)\n",addrstr,rt);
        return 0;
    }
    addr = bswap32(addr);

    if (!parseHex(datastr,(char*)bindata,&rt)){
        for (int i=0; i<rt; i++) {
            addr[i] = bindata[i];
        }
        _printf("wrote to 0x%s (0x%08x) this data:\n",addrstr,addr);
        DumpHex(addr,rt);
    }else{
        _printf("ERROR: cmd_mw failed to parse data\n");
    }
    _free(bindata);
    _printf("md done!\n");
    return 0;
}

int aes_cmd(int argc, CmdArg* argv) {
    int i = 0;
    char* kbag = NULL;
    char* action = NULL;
    unsigned int size = 0;
    unsigned char* aesIN = NULL;
    unsigned char* aesOUT = NULL;

    if(argc != 3) {
        _printf("usage: aes <enc/dec> [data]\n");
        return 0;
    }

    action = (char*)argv[1].string;
    kbag = (char*)argv[2].string;

    size = atol(kbag);
    _printf("aes_key insize=%d\n",size);
    if((size&1)) {
        _printf("ERROR: aes_encrypt_key\n");
        return 0;
    }
    aesIN = _malloc(size);
    aesOUT = _malloc(size);
    memset(aesIN,0,size);
    memset(aesOUT,0,size);

    if(!strcmp(action, "enc")) {
        _printf("aes do enc\n");
        aes_encrypt_key_custom((char*)aesIN, (char*)aesOUT, size, fixedKey);
    } else if(!strcmp(action, "loopSame")) {
        while (gpio_read(GPIO_VOL_UP)){
            aes_decrypt_key_custom((char*)aesIN, (char*)aesOUT, size, fixedKey);
        }
    } else if(!strcmp(action, "loopSameGid")) {
        while (gpio_read(GPIO_VOL_UP)){
            aes_decrypt_key((char*)aesIN, (char*)aesOUT, size);
        }
    } else if(!strcmp(action, "loopDiff")) {
        while (gpio_read(GPIO_VOL_UP)){
            aes_encrypt_key_custom((char*)aesIN, (char*)aesOUT, size, fixedKey);
            aes_encrypt_key_custom((char*)aesOUT, (char*)aesIN, size, fixedKey);
        }
    } else {
        _free(aesOUT);
        _free(aesIN);
        return -1;
    }

    for(i = 0; i < size; i++) {
        _printf("%02x", aesOUT[i]);
    }
    _printf("\n");

    _free(aesOUT);
    _free(aesIN);
    return 0;
}

unsigned int aes_encrypt_key(char* in, char* out, unsigned int size) {
    aes_crypto_cmd(kAesEncrypt, in, out, size, kAesTypeGid, 0, 0);
    return size;
}

unsigned int aes_decrypt_key(char* in, char* out, unsigned int size) {
    aes_crypto_cmd(kAesDecrypt | AES_CMD_CBC, in, out, size, kAesTypeGid | kAesType256, 0, 0);
    return size;
}

unsigned int aes_encrypt_key_custom(char* in, char* out, unsigned int size, void *key) {
    aes_crypto_cmd(kAesEncrypt | AES_CMD_CBC, in, out, size, kAesTypeUser | kAesType256, key, 0);
    return size;
}

unsigned int aes_decrypt_key_custom(char* in, char* out, unsigned int size, void *key) {
    aes_crypto_cmd(kAesDecrypt | AES_CMD_CBC, in, out, size, kAesTypeUser | kAesType256, key, 0);
    return size;
}


#pragma mark stdfuncs

void memset(void *a, unsigned char b, size_t len){
    while(len--){
        *((unsigned char*)a) = b;
        a = ((unsigned char*)a)+1;
    }
}

void memmove(void *dst, void *src, size_t len){
  uint8_t *d = (uint8_t *)dst;
  uint8_t *s = (uint8_t *)src;
  while (len--) {
    *d++ = *s++;
  }
}

int strncmp(const char *a, const char *b, uint32_t n){
    char d = 0;
    while (n-- && *a && !(d = *a++ - *b++));
    return d;
}

int strcmp(const char *a, const char *b){
    char d = 0;
    while (*a && !(d = *a++ - *b++));
    return d;
}

int strlen(const char *s){
    int res = 0;
    for (; *s++; res++);
    return res;
}

long atol(const char *s){
  long res = 0;
  int neg = 0;
  while (*s == ' ') s++;

  if (*s == '-') {
    s++;
    neg=1;
  }

  do{
    if (*s < '0' || *s > '9') return (neg) ? -res : res;
    res *= 10;
    res += *s - '0';
  }while(*++s);

  return (neg) ? -res : res;
}

int parseHex(const char *instr, char *ret, size_t *outSize){
    size_t nonceLen = strlen(instr);
    nonceLen = (nonceLen>>1) + (nonceLen&1); //one byte more if len is odd

    if (outSize) *outSize = (nonceLen)*sizeof(char);
    if (!ret) return 0;

    memset(ret, 0, *outSize);
    unsigned int nlen = 0;

    int next = (strlen(instr)&1) == 0;
    char tmp = 0;
    while (*instr) {
        char c = *(instr++);

        tmp *=16;
        if (c >= '0' && c<='9') {
            tmp += c - '0';
        }else if (c >= 'a' && c <= 'f'){
            tmp += 10 + c - 'a';
        }else if (c >= 'A' && c <= 'F'){
            tmp += 10 + c - 'A';
        }else{
            return -1; //ERROR parsing failed
        }
        if ((next =! next) && nlen < nonceLen) ret[nlen++] = tmp,tmp=0;
    }

    if (outSize) *outSize = nlen;
    return 0;
}

void DumpHex(const void* data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        _printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            _printf(" ");
            if ((i+1) % 16 == 0) {
                _printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    _printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    _printf("   ");
                }
                _printf("|  %s \n", ascii);
            }
        }
    }
}

void clear_icache() {
    __asm__("mov r0, #0");
    __asm__("mcr p15, 0, r0, c7, c5, 0");
    __asm__("isb");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
};

#pragma mark AES

void aes_precompute_tables(){
  T0 = _malloc(256*sizeof(uint32_t));
  T1 = _malloc(256*sizeof(uint32_t));
  T2 = _malloc(256*sizeof(uint32_t));
  T3 = _malloc(256*sizeof(uint32_t));
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
}


void aes_encrypt(uint8_t plaintext[16], uint8_t roundkeys[11][16], uint8_t ciphertext[16]){
    uint8_t tmp[16] = {0};

    for (int i=0; i<16; i++){ //pre key add
        tmp[i] = plaintext[i] ^ roundkeys[0][i];
    }

    uint32_t w[4] = {0};
    for (int i=1; i<10; i++) { //9 middle rounds

        w[0] = T0[tmp[ 0]] ^ T1[tmp[ 5]] ^ T2[tmp[10]] ^ T3[tmp[15]] ^ *(uint32_t*)&roundkeys[i][0];
        w[1] = T0[tmp[ 4]] ^ T1[tmp[ 9]] ^ T2[tmp[14]] ^ T3[tmp[ 3]] ^ *(uint32_t*)&roundkeys[i][4];
        w[2] = T0[tmp[ 8]] ^ T1[tmp[13]] ^ T2[tmp[ 2]] ^ T3[tmp[ 7]] ^ *(uint32_t*)&roundkeys[i][8];
        w[3] = T0[tmp[12]] ^ T1[tmp[ 1]] ^ T2[tmp[ 6]] ^ T3[tmp[11]] ^ *(uint32_t*)&roundkeys[i][12];

        *(uint32_t*)&tmp[ 0] = w[0];
        *(uint32_t*)&tmp[ 4] = w[1];
        *(uint32_t*)&tmp[ 8] = w[2];
        *(uint32_t*)&tmp[12] = w[3];
    }

    for (int i=0; i<4; i++) { //final round
        ciphertext[4*i + 0] = sbox[tmp[4*i          ]] ^ roundkeys[10][4*i + 0];
        ciphertext[4*i + 1] = sbox[tmp[(4*i +  5)&0xf]] ^ roundkeys[10][4*i + 1];
        ciphertext[4*i + 2] = sbox[tmp[(4*i + 10)&0xf]] ^ roundkeys[10][4*i + 2];
        ciphertext[4*i + 3] = sbox[tmp[(4*i + 15)&0xf]] ^ roundkeys[10][4*i + 3];
    }
}

//
