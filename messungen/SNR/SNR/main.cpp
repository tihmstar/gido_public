//
//  main.cpp
//  StaticVsRandom
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
#include <algorithm>

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
    float *_mem;
    size_t _size;
public:
    myfile(const char *fname, size_t size) : _fd(-1),_size(size){
        assure((_fd = open(fname, O_RDWR | O_CREAT, 0644))>0);
        lseek(_fd, _size-1, SEEK_SET);
        write(_fd, "", 1);
        assure(_mem = (float*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
    }
    ~myfile(){
        munmap(_mem, _size);
        if (_fd > 0) close(_fd);
    }
    float &operator[](int at){
        return _mem[at];
    }
};

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int doSNR(const char *indir, const char *outfile, int threadsCnt, int maxTraces){
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
        
        if (maxTraces && (int)toNormalizeList.size() >= maxTraces) {
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
    
    std::vector<combtrace> groupsMean{0x100};
    for (auto &cmbtr : groupsMean){
        cmbtr.trace = (float*)malloc(point_per_trace*sizeof(float));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, point_per_trace*sizeof(float));
    }
    std::vector<combtrace> groupsCenSum{0x100};
    for (auto &cmbtr : groupsCenSum){
        cmbtr.trace = (float*)malloc(point_per_trace*sizeof(float));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, point_per_trace*sizeof(float));
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
                
                std::vector<combtrace> curgroupsMean{0x100};
                for (auto &cmbtr : curgroupsMean){
                    cmbtr.trace = (float*)malloc(point_per_trace*sizeof(float));
                    cmbtr.cnt = 0;
                    memset(cmbtr.trace, 0, point_per_trace*sizeof(float));
                }
                std::vector<combtrace> curgroupsCenSum{0x100};
                for (auto &cmbtr : curgroupsCenSum){
                    cmbtr.trace = (float*)malloc(point_per_trace*sizeof(float));
                    cmbtr.cnt = 0;
                    memset(cmbtr.trace, 0, point_per_trace*sizeof(float));
                }


                debug("[T-%d] mean\n",workerNum);
                computeMeans(curtrace, curgroupsMean);
                
                debug("[T-%d] centeredSums\n",workerNum);
                computeCenteredSums(curtrace, curgroupsMean, curgroupsCenSum);
                
                debug("[T-%d] waiting to combine\n",workerNum);
                updateLock.lock();
                debug("[T-%d] combine means\n",workerNum);
                for (size_t i=0; i<groupsMean.size(); i++) {
                    combineMeans(groupsMean.at(i), curgroupsMean.at(i), point_per_trace);
                }
                
                debug("[T-%d] combine censums\n",workerNum);
                for (size_t i=0; i<groupsCenSum.size(); i++) {
                    combineCenteredSum(groupsCenSum.at(i), groupsMean.at(i), curgroupsCenSum.at(i), curgroupsMean.at(i), point_per_trace);
                }
                
                updateLock.unlock();
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
        
    }

    printf("waiting for workers to finish...\n");
    for (auto &w : workers){
        w.wait();
    }
    printf("all workers finished!\n");
        
    printf("computing SNRs\n");
    float meanOfMeans[point_per_trace];
    memset(meanOfMeans, 0, sizeof(float)*point_per_trace);

    float meanOfVariances[point_per_trace];
    memset(meanOfVariances, 0, sizeof(float)*point_per_trace);

    float variancesOfMeans[point_per_trace];
    memset(variancesOfMeans, 0, sizeof(float)*point_per_trace);
    
    computeMeanOfMeans(groupsMean, meanOfMeans, point_per_trace);
    computeVariancesOfMeans(groupsMean, meanOfMeans, variancesOfMeans, point_per_trace);
    computeMeanOfVariancesFromCensums(groupsCenSum, meanOfVariances, point_per_trace);
    
    
    printf("generating file=%s\n",outfile);
    auto snrTrace = myfile(outfile,point_per_trace*sizeof(float));
    memset(&snrTrace[0], 0, point_per_trace*sizeof(float));

    for (uint32_t i=0; i<point_per_trace; i++) {
        if (!meanOfVariances[i]) continue;
        float tmp = variancesOfMeans[i] / meanOfVariances[i];
        snrTrace[i] = tmp;
    }
    
    printf("done!\n");
    return 0;
}

int r_main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outpath> {threadsCnt} {maxTraces}\n",argv[0]);
        return -1;
    }
    doInit();

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
    
    return doSNR(indir, outfile, threadsCnt, maxTraces);
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

