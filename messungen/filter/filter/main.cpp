//
//  main.cpp
//  filter
//
//  Created by tihmstar on 16.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include <iostream>
#include <signal.h>
#include <stdint.h>

#define assure_(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#include <filesystem>
#ifdef __APPLE__
using namespace std::__fs;
#endif //__APPLE__

using namespace std;

void doFile(const char *filepath){
    FILE *f = fopen(filepath, "rb");
    double *buf = NULL;
    size_t bufSize = 0;
    assure_(f);
    fseek(f, 0, SEEK_END);
    bufSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    buf = (double *)malloc(bufSize);
    fread(buf, 1, bufSize, f);
    fclose(f);
    
    uint64_t tracesInFile = bufSize/sizeof(*buf);
    
    double avgVal = 0;
    double maxVal = 0;
    
    for (uint64_t i=0; i<tracesInFile; i++){
        avgVal += buf[i]/tracesInFile;
        if (buf[i] > maxVal) {
            maxVal = buf[i];
        }
    }
    
    if (maxVal > avgVal*30 && maxVal > 15) {
        printf("%s\n",filepath);
    }
    free(buf);
}

void doDirectory(const char *dir){
    for(auto& p: filesystem::directory_iterator(dir)){
        if (p.is_directory()) {
            doDirectory(p.path().c_str());
            continue;
        }
        
        std::string path = p.path();
        std::size_t pos = path.rfind("/");
        
        assure_(pos != std::string::npos);
        
        if (strncmp(path.substr(pos+1).c_str(), "SNR-", 4) == 0) {
            doFile(p.path().c_str());
        }
   }
}


int main(int argc, const char * argv[]) {

    if (argc < 2) {
        printf("Usage: %s <dir path>\n",argv[0]);
        return -1;
    }

    const char *snrdir = argv[1];


    doDirectory(snrdir);
    
    
    printf("done\n");
    return 0;
}
