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
#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN main.cpp ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)


#define safeFree(ptr) do { if (ptr){ free(ptr); ptr = NULL;} }while(0)

#define SIMD

#ifdef SIMD
#include "immintrin.h" // for AVX 
#endif


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

void increase_file_limit() {
    struct rlimit rl = {};
    int error = getrlimit(RLIMIT_NOFILE, &rl);
    assure(error == 0);
    rl.rlim_cur = 10240;
    rl.rlim_max = rl.rlim_cur;
    error = setrlimit(RLIMIT_NOFILE, &rl);
    if (error != 0) {
        printf("could not increase file limit\n");
    }
    error = getrlimit(RLIMIT_NOFILE, &rl);
    assure(error == 0);
    if (rl.rlim_cur != 10240) {
        printf("file limit is %llu\n", rl.rlim_cur);
    }
}

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
#else
#error no non-SIMD implementation
#endif //SIMD


int doNormalize(const char *indir, const char *outdir, int threadsCnt, int maxTraces){
    printf("start\n");
    atomic<uint16_t> wantWriteCounter{0};
    vector<future<void>> workers;

    int8_t *meanTrace = NULL;
    int8_t *meanTrace2 = NULL;
    int8_t *meanTrace3 = NULL;

    printf("threads=%d\n",threadsCnt);
    printf("indir=%s\n",indir);
    printf("outdir=%s\n",outdir);

    
#pragma mark list files
    printf("reading trace\n");
    vector<string> toNormalizeList;
    mutex meanLock;
    mutex listLock;
    mutex fileWriteEvent;
    
    for(auto& p: filesystem::directory_iterator(indir)){
        if (!ends_with(p.path(), ".dat") && !ends_with(p.path(), ".dat.tar.gz")) {
            continue;
        }
        printf("adding to list=%s\n",p.path().c_str());
        toNormalizeList.push_back(p.path());
        
        if (maxTraces && (int)toNormalizeList.size() >= maxTraces) {
            printf("limiting list to %d files\n",maxTraces);
            break;
        }
    }
    size_t filesCnt = toNormalizeList.size();
    std::reverse(toNormalizeList.begin(),toNormalizeList.end());
    
    atomic_uint16_t activeWorkerCnt{0};

    auto workerfunc = [&](int workerNum)->void{
        printf("[T-%d] worker started\n",workerNum);
        ++activeWorkerCnt;
        while (true) {
            printf("[T-%d] getting tracesfile...\n",workerNum);
            listLock.lock();
            size_t todoCnt = toNormalizeList.size();
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
                
                printf("[T-%d] [ %3lu of %3lu] normalizing trace=%s\n",workerNum,(filesCnt+1-todoCnt),filesCnt,tpath.c_str());

                Traces *curtrace = new Traces(tpath.c_str());


#define START_POS 0
#define FALT_SIZE 3000
                
#define FALT_SIZE2 3000
#define FALT_SIZE3 1000
//#define SAMPLE_SIZE 35000
#define SAMPLE_SIZE 74000

                
//#define MIN_PEAK_VALUE (-65)
//#define PRE_SIZE  (1000)
                
                uint32_t tracesInFile = 0;
                int tnum = -1;
                for (auto ms : *curtrace) {
                    tnum++;
                    int maxPos = START_POS;
                    int maxVal = 0;

                    for (int p=47000; p<53000; p++) {
                        if (ms->Trace[p]<-65) {
                           maxPos = p;
                           goto afterBadTrace;
                           break;
                        }
                    }
                    
                badtrace:
#ifdef DEBUG
                    printf("badtrace=%d\n",tnum);
#endif
                    continue;
                afterBadTrace:
                    
   
                    
                    
                    int off = 0;
                    
                    off -=1000;
                    if (!meanTrace) {
                        meanLock.lock();
                        if (!meanTrace) {
                            //first set a mean
                            assure(meanTrace = (int8_t*)malloc((FALT_SIZE + MOVE_RADIUS) * sizeof(*meanTrace)));
                            memcpy(meanTrace, (int8_t*)&ms->Trace[maxPos], (FALT_SIZE + MOVE_RADIUS) * sizeof(*meanTrace));
                        }
                        meanLock.unlock();
                    }else{
                        off = maxfaltung(meanTrace, &ms->Trace[maxPos], FALT_SIZE);
                    }

                    off -= 47000;
                    
                    for (int p=26500; p<33500; p++) {
                        if (ms->Trace[maxPos+off+p] < -2) {
                            goto badtrace;
                        }
                    }

                    for (int p=37500; p<46000; p++) {
                        if (ms->Trace[maxPos+off+p] < -65) {
                            goto badtrace;
                        }
                    }

                    for (int p=36700; p<44000; p++) {
                        if (ms->Trace[maxPos+off+p] < 10) {
                            goto badtrace;
                        }
                    }

                    
                    tracesInFile++;
                    tstarts->push_back({(int8_t*)&ms->Trace[maxPos+off]/*Trace*/,(uint8_t*)ms->Input/*Plaintext*/,(uint8_t*)ms->Output/*Ciphertext*/});
                    
#ifdef DEBUG
                    if (tnum % 100 == 0) {
                        printf("[T-%d] tracenum=%d\n",workerNum,tnum);
                    }
#warning DEBUG
                    if (tnum > 2000) break;
#endif
                }
                                        
                auto compressedwritefunc = [&fileWriteEvent, &wantWriteCounter](Traces *curtrace,vector<tuple<int8_t*, uint8_t *, uint8_t *>> *tstarts, uint32_t tracesInFile, string outfilepath, int workerNum)->void{
                    printf("[WT-%d] compressing normalized trace...\n",workerNum);
                    ssize_t lastSlash = outfilepath.rfind("/");
                    std::string outfilename = outfilepath;
                    if (lastSlash != std::string::npos) {
                        outfilename = outfilepath.substr(lastSlash+1);
                    }
                    
                    if (outfilepath.substr(outfilepath.size()-sizeof(".tar.gz")+1) != ".tar.gz") {
                        outfilepath += ".tar.gz";
                    }
                    
                    std::string tmpoutfilepath = outfilepath;
                    tmpoutfilepath += ".partial";

                    
                    struct archive *a = NULL;
                    struct archive_entry *entry = NULL;
                    a = archive_write_new();
                    archive_write_add_filter_gzip(a);
                    archive_write_set_format_pax_restricted(a);
                    archive_write_open_filename(a, tmpoutfilepath.c_str());
                    
                    uint32_t np = SAMPLE_SIZE;
                    size_t fileSize = sizeof(tracesInFile) + tstarts->size() * (sizeof(np) + sizeof(Traces::MeasurementStructure::Input) + sizeof(Traces::MeasurementStructure::Output) + np);
                    
                    if (outfilename.substr(outfilename.size()-sizeof(".tar.gz")+1) == ".tar.gz") {
                        outfilename = outfilename.substr(0,outfilename.size()-sizeof(".tar.gz")+1);
                    }
                    
                    entry = archive_entry_new();
                    archive_entry_set_pathname(entry, outfilename.c_str());
                    archive_entry_set_size(entry, fileSize); // Note 3
                    archive_entry_set_filetype(entry, AE_IFREG);
                    archive_entry_set_perm(entry, 0644);
                    archive_entry_set_mtime(entry, time(NULL), 0);
                    archive_write_header(a, entry);

                    archive_write_data(a, &tracesInFile, sizeof(tracesInFile)); //TracesInFile (actual data!)
                    for (auto p : *tstarts) {
                        archive_write_data(a, &np, sizeof(np)); //PointsPerTrace
                        archive_write_data(a, get<1>(p), sizeof(Traces::MeasurementStructure::Input)); //Plaintext
                        archive_write_data(a, get<2>(p), sizeof(Traces::MeasurementStructure::Output)); //Ciphertext
                        archive_write_data(a, get<0>(p), np); //Trace
                    }
                    archive_entry_free(entry);
                    
                    archive_write_close(a); // Note 4
                    archive_write_free(a); // Note 5

                    delete curtrace;
                    delete tstarts;

                    rename(tmpoutfilepath.c_str(), outfilepath.c_str());

                    printf("[WT-%d] done saving compressed normalized trace to %s! (%u remaining writers)\n",workerNum, outfilepath.c_str(),((uint16_t)wantWriteCounter)-1);
                    wantWriteCounter.fetch_sub(1);
                    fileWriteEvent.unlock(); //send wantWriteCounter update event
                };
                
                while (wantWriteCounter.fetch_add(1) > threadsCnt) {
                    wantWriteCounter.fetch_sub(1);
                    printf("[T-%d] writer limit reached, waiting for some writers to finish...\n",workerNum);
                    fileWriteEvent.lock();
                }
                fileWriteEvent.unlock(); //always send event when counter is modified
                
                std::thread wthread(compressedwritefunc,curtrace,tstarts,tracesInFile,outfilepath,workerNum);
                wthread.detach();
            }else{
                listLock.unlock();
                printf("[T-%d] no more traces available\n",workerNum);
                break;
            }
        }
        --activeWorkerCnt;
        printf("[T-%d] worker finished (%d remaining workers)\n",workerNum,(uint16_t)activeWorkerCnt);
    };
    
    printf("spinning up %d worker threads\n",threadsCnt);
    
    for (int i=0; i<threadsCnt; i++){
        workers.push_back(std::async(std::launch::async,workerfunc,i));
//        workerfunc(i);
        sleep(1);
    }

    printf("waiting for workers to finish...\n");
    for (int i=0; i<threadsCnt; i++){
        workers[i].wait();
    }
    printf("all workers finished!\n");

    printf("waiting for writers to finish...\n");
    while ((uint16_t)wantWriteCounter > 0) {
        //we are only waiting for a few writers here, which are about to finish
        //eventlocks lead to locking issues.
        //don't hog CPU and sping here, sleep should be good enough, we won't stay here very long
        sleep(5);
    }
    printf("all writers finished!\n");


//    assure(beginOffset < (PRE_SIZE_WORK - PRE_SIZE));
//    assure(-endOffset < (SAMPLE_SIZE_WORK - SAMPLE_SIZE));


    safeFree(meanTrace);
    safeFree(meanTrace2);
    safeFree(meanTrace3);

    printf("done!\n");
    return 0;
}

int r_main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outdir>\n",argv[0]);
        return -1;
    }

    increase_file_limit();
    
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
