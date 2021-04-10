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

//#define SEQUENTIAL

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
    long double *_mem;
    size_t _size;
    size_t _increment;
public:
    myfile(const char *fname, size_t size) : _fd(-1),_size(size),_increment(0){
        assure((_fd = open(fname, O_RDWR | O_CREAT, 0644))>0);
        lseek(_fd, _size-1, SEEK_SET);
        write(_fd, "", 1);
        assure(_mem = (long double*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
    }
    ~myfile(){
        munmap(_mem, _size);
        if (_fd > 0) close(_fd);
    }
    long double &operator[](int at){
        return _mem[at+_increment];
    }
};

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int doTTest(const char *indir, const char *outpath, int threadsCnt, int maxTraces){
    printf("start\n");
#ifdef SEQUENTIAL
    int seqCnt = 0;
#endif
    vector<future<void>> workers;

    printf("threads=%d\n",threadsCnt);
    printf("indir=%s\n",indir);
    printf("outpath=%s\n",outpath);
    
    vector<string> toNormalizeList;
    mutex listLock;
    mutex updateLock;
    
    printf("reading trace\n");
    for(auto& p: filesystem::directory_iterator(indir)){
        if (!ends_with(p.path(), ".dat") && !ends_with(p.path(), ".dat.tar.gz")) {
            continue;
        }
        printf("adding to list=%s\n",p.path().c_str());
        toNormalizeList.push_back(p.path());
        
        if (maxTraces && toNormalizeList.size() >= maxTraces) {
            printf("limiting list to %d files\n",maxTraces);
            break;
        }
    }
    uint32_t point_per_trace = 0;
    {
        Traces tr(toNormalizeList.at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
    }
    printf("point_per_trace=%u\n",point_per_trace);
    auto t1trace = myfile(outpath,point_per_trace*sizeof(long double));

    memset(&t1trace[0], 0, point_per_trace*sizeof(long double));
    
    uint64_t cntMeanStatic = 0;
    uint64_t cntMeanRandom = 0;

    uint64_t cntcensumStatic = 0;
    uint64_t cntcensumRandom = 0;

    long double *meanStatic = new long double[point_per_trace];
    long double *meanRandom = new long double[point_per_trace];
        
    long double *censumStatic = new long double[point_per_trace];
    long double *censumRandom = new long double[point_per_trace];

    memset(meanStatic, 0, point_per_trace * sizeof(long double));
    memset(meanRandom, 0, point_per_trace * sizeof(long double));
    memset(censumStatic, 0, point_per_trace * sizeof(long double));
    memset(censumRandom, 0, point_per_trace * sizeof(long double));

    //BEGIN MAIN
    auto workerfunc = [&](int workerNum)->void{
        printf("[T-%d] worker started\n",workerNum);
        while (true) {
            printf("[T-%d] getting tracesfile...\n",workerNum);
            listLock.lock();
            if (toNormalizeList.size()) {
                string tpath = toNormalizeList.back();
                toNormalizeList.pop_back();
                listLock.unlock();
                //START WORK
                printf("[T-%d] working on trace=%s\n",workerNum,tpath.c_str());
                Traces curtrace(tpath.c_str());

                uint64_t curcntMeanStatic = 0;
                uint64_t curcntMeanRandom = 0;

                uint64_t curcntcensumStatic = 0;
                uint64_t curcntcensumRandom = 0;

                
                long double *curmeanStatic = new long double[point_per_trace];
                long double *curmeanRandom = new long double[point_per_trace];
                    
                long double *curcensumStatic = new long double[point_per_trace];
                long double *curcensumRandom = new long double[point_per_trace];

                memset(curmeanStatic, 0, point_per_trace * sizeof(long double));
                memset(curmeanRandom, 0, point_per_trace * sizeof(long double));
                memset(curcensumStatic, 0, point_per_trace * sizeof(long double));
                memset(curcensumRandom, 0, point_per_trace * sizeof(long double));
                
                debug("[T-%d] static\n",workerNum);
                curcntMeanStatic = computeMean(curtrace, curmeanStatic, false);
                curcntcensumStatic = computeCenteredSum(curtrace, curmeanStatic, curcensumStatic, false);
                
                debug("[T-%d] random\n",workerNum);
                curcntMeanRandom = computeMean(curtrace, curmeanRandom, true);
                curcntcensumRandom = computeCenteredSum(curtrace, curmeanRandom, curcensumRandom, true);

                dbg_assure(curcntMeanStatic == curcntcensumStatic);
                dbg_assure(curcntMeanRandom == curcntcensumRandom);

                debug("[T-%d] waiting to combine\n",workerNum);
                updateLock.lock();
                debug("[T-%d] combine means\n",workerNum);
                cntMeanStatic = combineMeans({meanStatic,cntMeanStatic}, {curmeanStatic,curcntMeanStatic}, point_per_trace);
                cntMeanRandom = combineMeans({meanRandom,cntMeanRandom}, {curmeanRandom,curcntMeanRandom}, point_per_trace);

                debug("[T-%d] combine censums\n",workerNum);
                cntcensumStatic = combineCenteredSum({censumStatic, meanStatic, cntcensumStatic}, {curcensumStatic, curmeanStatic, curcntcensumStatic}, point_per_trace);
                cntcensumRandom = combineCenteredSum({censumRandom, meanRandom, cntcensumRandom}, {curcensumRandom, curmeanRandom, curcntcensumRandom}, point_per_trace);
                
                
#ifdef SEQUENTIAL
                {
                    seqCnt++;
                    std::string tseqname = outpath;
                    tseqname+="_";
                    tseqname+= to_string(seqCnt);

                    auto t1traceSeq = myfile(tseqname.c_str(),point_per_trace*sizeof(long double));

                    memset(&t1traceSeq[0], 0, point_per_trace*sizeof(long double));

                    printf("computing t1-Value for file=%s\n",tseqname.c_str());
                    for (int i=0; i<point_per_trace; i++) {

                        long double upper = meanRandom[i] - meanStatic[i];

                        long double varRandom = censumRandom[i]/cntcensumRandom;
                        long double varStatic = censumStatic[i]/cntcensumStatic;

                        long double inlower = (varRandom/cntMeanRandom) + (varStatic/cntMeanStatic);
                        long double lower = sqrt(inlower);
                        long double tmp = upper/lower;
                        t1traceSeq[i] = tmp;
                    }
                }
#endif //SEQUENTIAL
                
                updateLock.unlock();
                
                delete [] curmeanStatic;
                delete [] curmeanRandom;
                    
                delete [] curcensumStatic;
                delete [] curcensumRandom;
                
                
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
        sleep(1); //delay worker spawn
    }

    printf("waiting for workers to finish...\n");
    for (int i=0; i<threadsCnt; i++){
        workers[i].wait();
    }
    printf("all workers finished!\n");
    
    dbg_assure(cntMeanStatic == cntcensumStatic);
    dbg_assure(cntMeanRandom == cntcensumRandom);
    
    printf("computing t1-Value\n");
    for (int i=0; i<point_per_trace; i++) {
        
        long double upper = meanRandom[i] - meanStatic[i];

        long double varRandom = censumRandom[i]/cntcensumRandom;
        long double varStatic = censumStatic[i]/cntcensumStatic;

        long double inlower = (varRandom/cntMeanRandom) + (varStatic/cntMeanStatic);
        long double lower = sqrt(inlower);
        long double tmp = upper/lower;;
        t1trace[i] = tmp;
    }
    
    
    delete [] meanStatic;
    delete [] meanRandom;
        
    delete [] censumStatic;
    delete [] censumRandom;
    
    
    printf("done!\n");
    return 0;
}

int r_main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outpath> {threadsCnt} {maxTraces}\n",argv[0]);
        return -1;
    }

    int threadsCnt = (int)sysconf(_SC_NPROCESSORS_ONLN);
    int maxTraces = 0;
    
    const char *indir = argv[1];
    const char *outpath = argv[2];
        
    if (argc > 3) {
        threadsCnt = atoi(argv[3]);
    }

    if (argc > 4) {
        maxTraces = atoi(argv[4]);
    }
    
    return doTTest(indir, outpath, threadsCnt, maxTraces);
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

