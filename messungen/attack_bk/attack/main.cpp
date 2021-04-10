//
//  main.cpp
//  attack
//
//  Created by tihmstar on 07.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "Traces.hpp"
#include "numerics.hpp"
#include <string.h>
#include <iostream>
#include <future>
#include <vector>
#include <filesystem>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <exception>
#include <stdlib.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); throw __LINE__;}}while (0)

#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#define debug(a ...) printf(a)
#else
#define dbg_assure(cond) //
#define debug(a ...) //
#endif

#ifndef NOMAIN
#define HAVE_FILESYSTEM
#endif

#ifdef HAVE_FILESYSTEM

#ifdef __APPLE__
using namespace std::__fs;
#endif //__APPLE__

#else
#include <dirent.h>
//really crappy implementation in case <filesystem> isn't available :o
class myfile{
    std::string _path;
public:
    myfile(std::string p): _path(p){}
    std::string path(){return _path;}
};

class diriter{
public:
    std::vector<myfile> _file;
    auto begin(){return _file.begin();}
    auto end(){return _file.end();}
};

namespace std {
    namespace filesystem{
        diriter directory_iterator(std::string);
    }
}

diriter std::filesystem::directory_iterator(std::string dirpath){
    DIR *dir = NULL;
    struct dirent *ent = NULL;
    diriter ret;
    
    assure(dir = opendir(dirpath.c_str()));
    
    while ((ent = readdir (dir)) != NULL) {
        if (ent->d_type != DT_REG)
            continue;
        ret._file.push_back({dirpath + "/" + ent->d_name});
    }
    
    if (dir) closedir(dir);
    return ret;
}
#endif

using namespace std;

class myfile{
    int _fd;
    double *_mem;
    size_t _size;
public:
    myfile(const char *fname, size_t size) : _fd(-1),_size(size){
        assure((_fd = open(fname, O_RDWR | O_CREAT, 0644))>0);
        lseek(_fd, _size-1, SEEK_SET);
        write(_fd, "", 1);
        assure(_mem = (double*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
    }
    ~myfile(){
        munmap(_mem, _size);
        if (_fd > 0) close(_fd);
    }
    double &operator[](int at){
        return _mem[at];
    }
};

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int doAttack(const char *indir, const char *outfile, int threadsCnt, int maxTraces){
    printf("start\n");
    vector<future<void>> workers;

    printf("threads=%d\n",threadsCnt);
    printf("indir=%s\n",indir);
    printf("outfile=%s\n",outfile);
    
    vector<string> toNormalizeList;
    mutex listLock;
    mutex updateLock;

        
    printf("reading trace\n");
    for(auto& p: filesystem::directory_iterator(indir)){
        if (!ends_with(p.path(), ".dat")) {
            continue;
        }
        printf("adding to list=%s\n",p.path().c_str());
        toNormalizeList.push_back(p.path());
        
        if (maxTraces && toNormalizeList.size() >= maxTraces) {
            printf("limiting list to %d files\n",maxTraces);
            break;
        }
    }
    std::reverse(toNormalizeList.begin(),toNormalizeList.end());

    uint32_t point_per_trace = 0;
    {
        Traces tr(toNormalizeList.at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
    }
    printf("point_per_trace=%u\n",point_per_trace);
    
    uint64_t gcntMean = 0;
    uint64_t gcntcensum = 0;
    double gmean[point_per_trace];
    double gcensum[point_per_trace];


    memset(gmean, 0, point_per_trace * sizeof(double));
    memset(gcensum, 0, point_per_trace * sizeof(double));


    double ghypotMeanGuessVals[0x100] = {};
    uint64_t ghypotMeanGuessCnt[0x100] = {};

    double ghypotCensumGuessVals[0x100] = {};
    uint64_t ghypotCensumGuessCnt[0x100] = {};

    
    numberedTrace *gupperPart = (numberedTrace *)malloc(0x100*sizeof(numberedTrace));
    for (int k = 0; k<0x100; k++) {
        gupperPart[k].cnt = 0;
        gupperPart[k].trace = (double*)malloc(point_per_trace * sizeof(double));
        memset(gupperPart[k].trace, 0, point_per_trace * sizeof(double));
    }
    
    
    size_t filesCnt = toNormalizeList.size();
    
    //BEGIN MAIN
    auto workerfunc = [&](int workerNum)->void{
        printf("[T-%d] worker started\n",workerNum);
        while (true) {
            printf("[T-%d] getting tracesfile...\n",workerNum);
            listLock.lock();
            size_t todoCnt = toNormalizeList.size();
            if (toNormalizeList.size()) {
                string tpath = toNormalizeList.back();
                toNormalizeList.pop_back();
                listLock.unlock();
                //START WORK
                printf("[T-%d] [ %3lu of %3lu] working on trace=%s\n",workerNum,(filesCnt+1-todoCnt),filesCnt,tpath.c_str());
                Traces curtrace(tpath.c_str());

                double curhypotMeanGuessVals[0x100] = {};
                uint64_t curhypotMeanGuessCnt[0x100] = {};

                double curhypotCensumGuessVals[0x100] = {};
                uint64_t curhypotCensumGuessCnt[0x100] = {};
                
                numberedTrace *curupperPart = (numberedTrace *)malloc(0x100*sizeof(numberedTrace));
                for (int k = 0; k<0x100; k++) {
                    curupperPart[k].cnt = 0;
                    curupperPart[k].trace = (double*)malloc(point_per_trace * sizeof(double));
                    memset(curupperPart[k].trace, 0, point_per_trace * sizeof(double));
                }

                
                uint64_t curcntMean = 0;
                uint64_t curcntcensum = 0;

                
                double curmean[point_per_trace];
                double curcensum[point_per_trace];

                memset(curmean, 0, point_per_trace * sizeof(double));
                memset(curcensum, 0, point_per_trace * sizeof(double));
                
                debug("[T-%d] compute mean\n",workerNum);
                curcntMean = computeMean(curtrace, curmean, {curhypotMeanGuessVals,curhypotMeanGuessCnt});

                debug("[T-%d] compute censum\n",workerNum);
                curcntcensum = computeCenteredSum(curtrace, curmean, curcensum, {curhypotMeanGuessVals,curhypotMeanGuessCnt}, {curhypotCensumGuessVals,curhypotCensumGuessCnt}, curupperPart);
                
                debug("[T-%d] waiting to combine\n",workerNum);
                updateLock.lock();
                debug("[T-%d] combine means\n",workerNum);
                gcntMean = combineMeans({gmean,gcntMean}, {curmean,curcntMean}, point_per_trace);
                for (int k=0; k<0x100; k++) {
                    ghypotMeanGuessCnt[k] = combineMeans({&ghypotMeanGuessVals[k], ghypotMeanGuessCnt[k]}, {&curhypotMeanGuessVals[k], curhypotCensumGuessCnt[k]}, 1);
                }
                debug("[T-%d] combine censum\n",workerNum);
                gcntcensum = combineCenteredSum({gcensum, gmean, gcntcensum}, {curcensum, curmean, curcntcensum}, point_per_trace);

                for (int k=0; k<0x100; k++) {                    
                    ghypotCensumGuessCnt[k] = combineCenteredSum({&ghypotCensumGuessVals[k], &ghypotMeanGuessVals[k], ghypotCensumGuessCnt[k]}, {&curhypotCensumGuessVals[k], &curhypotMeanGuessVals[k], curhypotCensumGuessCnt[k]}, 1);
                }
                debug("[T-%d] combine upper\n",workerNum);
                for (int k=0; k<0x100; k++) {
                    for (int i=0; i<point_per_trace; i++) {
                        gupperPart[k].trace[i] += curupperPart[k].trace[i];
                    }
                    gupperPart[k].cnt += curupperPart[k].cnt;
                }
                
                updateLock.unlock();
                for (int k = 0; k<0x100; k++) {
                    free(curupperPart[k].trace);
                }
                free(curupperPart);

                //END WORK
            }else{
                listLock.unlock();
                printf("[T-%d] no more traces available\n",workerNum);
                break;
            }
        }
        printf("[T-%d] worker finished\n",workerNum);
    };
    
    //END MAIN
    
    printf("spinning up %d worker threads\n",threadsCnt);
    
    for (int i=0; i<threadsCnt; i++){
        workers.push_back(std::async(std::launch::async,workerfunc,i));
//        workerfunc(i); //for debugging
        
        sleep(1); //delay spawning workers
    }

    printf("waiting for workers to finish...\n");
    for (int i=0; i<threadsCnt; i++){
        workers[i].wait();
    }
    printf("all workers finished!\n");
    
    printf("Computing correlation...\n");
    
    for (int k=0; k<0x100; k++) {
        std::string filename = outfile;
        filename += '_';
        filename += to_string(k);
        printf("generating file=%s\n",filename.c_str());
        myfile out(filename.c_str(),point_per_trace*sizeof(double));
        memset(&out[0], 0, point_per_trace*sizeof(double));
        
        double *upTrace = gupperPart[k].trace;
        double hypotCensum = ghypotCensumGuessVals[k];
        for (int i=0; i<point_per_trace; i++) {
            double upper = upTrace[i];
            
            double realCensum = gcensum[i];
            
            long double preLower = realCensum * hypotCensum;
            
            long double lower = sqrt(preLower);
            
            out[i] = upper/lower;
        }
        
    }
    
    
    for (int k = 0; k<0x100; k++) {
        free(gupperPart[k].trace);
    }
    free(gupperPart);

    printf("done!\n");
    return 0;
}

int r_main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outfile> {threadsCnt} {maxTraces}\n",argv[0]);
        return -1;
    }

    int threadsCnt = (int)sysconf(_SC_NPROCESSORS_ONLN);
    int maxTraces = 0;
    
    const char *indir = argv[1];
    const char *outfile = argv[2];
        
    if (argc > 3) {
        threadsCnt = atoi(argv[3]);
    }

    if (argc > 4) {
        maxTraces = atoi(argv[4]);
    }
    
    return doAttack(indir, outfile, threadsCnt, maxTraces);
}

#ifndef NOMAIN
int main(int argc, const char * argv[]) {
#ifdef DEBUG
    return r_main(argc, argv);
#else //DEBUG
    try {
        r_main(argc, argv);
    } catch (int e) {
        printf("ERROR: died on line=%d\n",e);
    }
#endif //DEBUG
}
#endif //NOMAIN

