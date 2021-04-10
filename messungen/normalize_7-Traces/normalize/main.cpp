//
//  main.cpp
//  normalize
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include <iostream>
#include "Traces.hpp"
#include <vector>
#include <future>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <tuple>
#include <algorithm>

#define SIMD

#ifdef SIMD
#include "immintrin.h" // for AVX 
#endif

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); throw __LINE__;}}while (0)

#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#else
#define dbg_assure(cond) //
#endif

#define MAX(a,b) (a > b) ? (a) : (b)
#define MIN(a,b) (a < b) ? (a) : (b)

#define MOVE_RADIUS 500


#ifdef NOMAIN
#define DONT_HAVE_FILESYSTEM
#endif

#ifndef DONT_HAVE_FILESYSTEM
#include <filesystem>

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

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

#ifdef SIMD
__v16qi abs(__v16qi val){
    __v16qi zero = _mm_set1_epi8(0);
    __v16qi neg = _mm_sub_epi8(zero, val);
    __v16qi absv = _mm_max_epi8(neg, val);
    return absv;
}

//Sub method
int maxfaltung(const int8_t *t1, const int8_t *t2, int size){
    int faltn = 0;
    int64_t minFaltWert = LLONG_MAX;

    __v16qi zero = _mm_setzero_si128();
    for (int n=-MOVE_RADIUS; n<MOVE_RADIUS; n++) {
        __v2di faltWerte = _mm_setzero_si128();

        for (int k=0; k<size-sizeof(__v16qi); k+=sizeof(__v16qi)) {
            __v16qi v1 = *(__v16qi*)&t1[k]; //aligned access

            __v16qi v2; //usually unaligned access
            ((__v8qi*)&v2)[0] = *(__v8qi*)&t2[k+n];
            ((__v8qi*)&v2)[1] = *(__v8qi*)&t2[k+n+sizeof(__v8qi)];

            __v16qi diff = v1 - v2;
            __v16qi ndiff = _mm_sub_epi8(zero, diff);
            __v16qi adiff = _mm_max_epi8(ndiff, diff);

            __v2di falt = _mm_sad_epu8(adiff, zero);
            faltWerte = _mm_add_epi64(faltWerte, falt);
        }

        int64_t faltWert = ((int64_t*)&faltWerte)[0] + ((int64_t*)&faltWerte)[1];
        if (minFaltWert>faltWert) {
            minFaltWert = faltWert;
            faltn = n;
        }
    }

    return faltn;
}

////mult method
//int maxfaltung(const int8_t *t1, const int8_t *t2, int size){
//    int faltn = 0;
//    int64_t maxFaltWert = 0;
//
//    __v8hi zero = _mm_setzero_si128();
//    __v8hi mask = _mm_set1_epi16(0xff);
//    for (int n=-MOVE_RADIUS; n<MOVE_RADIUS; n++) {
//        int64_t faltWert = 0;
//
//        for (int k=0; k<size-sizeof(__v8hi); k+=sizeof(__v8hi)) {
//            __v8hi v1 = *(__v8hi*)&t1[k]; //aligned access
//
//            __v8hi v2; //usually unaligned access
//            ((__v8qi*)&v2)[0] = *(__v8qi*)&t2[k+n];
//            ((__v8qi*)&v2)[1] = *(__v8qi*)&t2[k+n+sizeof(__v8qi)];
//
//            __v8hi v11 = _mm_and_si128(mask, v1);
//            __v8hi v21 = _mm_and_si128(mask, v2);
//
//            v11 = _mm_cvtepi8_epi16(v1);
//            v21 = _mm_cvtepi8_epi16(v2);
//
//            __v8hi val = v11 * v21;
//            __v8hi nval = _mm_sub_epi16(zero, val);
//            __v8hi aval = _mm_max_epi16(nval, val);
//
//            __v8hi v12 = _mm_and_si128(mask, v1 >> 8);
//            __v8hi v22 = _mm_and_si128(mask, v2 >> 8);
//
//            v12 = _mm_cvtepi8_epi16(v12);
//            v22 = _mm_cvtepi8_epi16(v12);
//            val = v12 * v22;
//            nval = _mm_sub_epi16(zero, val);
//            __v8hi bval = _mm_max_epi16(nval, val);
//
//            __v4si sum4 = _mm_unpacklo_epi16(aval, zero);
//            aval = _mm_unpackhi_epi16(aval, zero);
//            sum4 += aval;
//
//            aval = _mm_unpacklo_epi16(bval, zero);
//            sum4 += aval;
//            aval = _mm_unpackhi_epi16(bval, zero);
//            sum4 += aval;
//
//            faltWert += ((int32_t*)&sum4)[0];
//            faltWert += ((int32_t*)&sum4)[1];
//            faltWert += ((int32_t*)&sum4)[2];
//            faltWert += ((int32_t*)&sum4)[3];
//        }
//
//        if (maxFaltWert<faltWert) {
//            maxFaltWert = faltWert;
//            faltn = n;
//        }
//    }
//
//    return faltn;
//}
#else
int maxfaltung(const int8_t *t1, const int8_t *t2, int size){
    int faltn = 0;
    int64_t maxFaltWert = 0;

    for (int n=-MOVE_RADIUS; n<MOVE_RADIUS; n++) {
        int64_t faltWert = 0;
        for (int k=0; k<size; k++) {
            int64_t tmp = (int64_t)t1[k] * t2[k+n];
            faltWert += tmp;
        }
        if (maxFaltWert<faltWert) {
            maxFaltWert = faltWert;
            faltn = n;
        }
    }
    
    return faltn;
}
#endif //SIMD


int doNormalize(const char *indir, const char *outdir, int threadsCnt, int maxTraces){
    printf("start\n");
    atomic<int> wantWriteCounter{0};
    vector<future<void>> workers;

    int8_t *meanTrace = NULL;

    printf("threads=%d\n",threadsCnt);
    printf("indir=%s\n",indir);
    printf("outdir=%s\n",outdir);

    
    printf("reading trace\n");
    vector<string> toNormalizeList;
    mutex meanLock;
    mutex listLock;
    mutex fileWriteEvent;
    
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
    
    
    auto workerfunc = [&](int workerNum)->void{
        printf("[T-%d] worker started\n",workerNum);
        while (true) {
            printf("[T-%d] getting tracesfile...\n",workerNum);
            listLock.lock();
            if (toNormalizeList.size()) {
                vector<tuple<int8_t*, uint8_t *, uint8_t *>> *tstarts = new vector<tuple<int8_t*, uint8_t *, uint8_t *>>();
                struct stat fstat = {};
                string tpath = toNormalizeList.back();
                toNormalizeList.pop_back();
                listLock.unlock();
                
                string filename = tpath.substr(tpath.find_last_of("/")+1);
                string outfilepath = outdir;
                if (outfilepath.back() != '/') {
                    outfilepath+= '/';
                }
                outfilepath+="normalized_";
                outfilepath+=filename;

                if (stat(outfilepath.c_str(),&fstat) != -1) {
                    printf("[T-%d] normalized trace already exists=%s\n",workerNum,outfilepath.c_str());
                    continue;
                }
                
                printf("[T-%d] normalizing trace=%s\n",workerNum,tpath.c_str());

                Traces *curtrace = new Traces(tpath.c_str());
#define START_POS 1500

#define FALT_SIZE 1400
#define SAMPLE_SIZE 4000

#define MIN_PEAK_VALUE (-35)
#define PRE_SIZE  (1000)
                
                uint32_t tracesInFile = 0;
                int tnum = -1;
                for (auto ms : *curtrace) {
                    tnum++;

                    int minPos = 0;
                    for (int p=0; p<ms->NumberOfPoints; p++) {
                        if (ms->Trace[p] < MIN_PEAK_VALUE) {
                            minPos = p;
                            break;
                        }
                    }
                                        
                    minPos -= PRE_SIZE;

                    if (minPos-MOVE_RADIUS < 0 || minPos + MOVE_RADIUS + SAMPLE_SIZE > ms->NumberOfPoints) {
#ifdef DEBUG
                        printf("badtrace=%d\n",tnum);
#endif
                        continue;
                    }
                    
                    
                    int off = 0;
                    if (!meanTrace) {
                        meanLock.lock();
                        if (!meanTrace) {
                            //first set a mean
                            assure(meanTrace = (int8_t*)malloc(SAMPLE_SIZE * sizeof(*meanTrace)));
                            memcpy(meanTrace, (int8_t*)&ms->Trace[minPos], SAMPLE_SIZE);
                        }
                        meanLock.unlock();
                    }else{
                        off = maxfaltung(meanTrace, &ms->Trace[minPos], FALT_SIZE);
                    }
                                        
                    tracesInFile++;
                    tstarts->push_back({(int8_t*)&ms->Trace[minPos+off]/*Trace*/,(uint8_t*)ms->Plaintext/*Plaintext*/,(uint8_t*)ms->Ciphertext/*Ciphertext*/});
                    
                    if (tnum % 1000 == 0) {
                        printf("[T-%d] tracenum=%d\n",workerNum,tnum);
                    }
//#warning DEBUG
//                    if (tnum && (tnum % 1000 == 0)) break;
                }
                
                wantWriteCounter++;
                std::thread wthread([&fileWriteEvent, &wantWriteCounter](Traces *curtrace,vector<tuple<int8_t*, uint8_t *, uint8_t *>> *tstarts, uint32_t tracesInFile, string outfilepath, int workerNum)->void{
                    printf("[WT-%d] saving normalized trace...\n",workerNum);
                    FILE *f = fopen(outfilepath.c_str(), "wb");
                    fwrite(&tracesInFile, 1, sizeof(tracesInFile), f); //TracesInFile (actual data!)

                    for (auto p : *tstarts) {
                        uint32_t np = SAMPLE_SIZE;
                        fwrite(&np, 1, sizeof(np), f); //PointsPerTrace
                        fwrite(get<1>(p), 1, 16, f); //Plaintext
                        fwrite(get<2>(p), 1, 16, f); //Ciphertext
                        fwrite(get<0>(p), 1, SAMPLE_SIZE, f); //Trace
                    }
                    fclose(f);
                    delete curtrace;
                    delete tstarts;
                    printf("[WT-%d] done saving normalized trace to %s!\n",workerNum, outfilepath.c_str());
                    wantWriteCounter--;
                    fileWriteEvent.unlock(); //send wantWriteCounter update event
                },curtrace,tstarts,tracesInFile,outfilepath,workerNum);
                wthread.detach();
            }else{
                listLock.unlock();
                printf("[T-%d] no more traces available\n",workerNum);
                break;
            }
        }
        printf("[T-%d] worker finished\n",workerNum);
    };
    
    printf("spinning up %d worker threads\n",threadsCnt);
    
    for (int i=0; i<threadsCnt; i++){
        workers.push_back(std::async(std::launch::async,workerfunc,i));
//        workerfunc(i);
    }

    printf("waiting for workers to finish...\n");
    for (int i=0; i<threadsCnt; i++){
        workers[i].wait();
    }
    printf("all workers finished!\n");

    printf("waiting for writers to finish...\n");
    while (wantWriteCounter > 0) {
        fileWriteEvent.lock(); //if someone can't write because lock is locked, this will make this thread sleep until something happened!
    }
    printf("all writers finished!\n");


//    assure(beginOffset < (PRE_SIZE_WORK - PRE_SIZE));
//    assure(-endOffset < (SAMPLE_SIZE_WORK - SAMPLE_SIZE));


    free(meanTrace);
    
    printf("done!\n");
    return 0;
}

int r_main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outdir>\n",argv[0]);
        return -1;
    }

    int threadsCnt = (int)sysconf(_SC_NPROCESSORS_ONLN);
    int maxTraces = 0;

    const char *indir = argv[1];
    const char *outdir = argv[2];
    
    if (argc > 3) {
        threadsCnt = atoi(argv[3]);
    }

    if (argc > 4) {
        maxTraces = atoi(argv[4]);
    }

    
    return doNormalize(indir, outdir, threadsCnt, maxTraces);
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
