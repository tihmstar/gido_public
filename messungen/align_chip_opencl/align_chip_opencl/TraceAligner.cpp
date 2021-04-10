//
//  TraceAligner.cpp
//  align_chip_opencl
//
//  Created by tihmstar on 11.01.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "TraceAligner.hpp"

#include <iostream>
#include <vector>
#include <future>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <tuple>
#include <algorithm>
#include <queue>
#include "LoadFile.hpp"
#include <signal.h>
#include <unistd.h>


#include <archive.h>
#include <archive_entry.h>


#define SIMD

#ifdef SIMD
#include "immintrin.h" // for AVX
#endif

#define assure(cond) do {if (!(cond)) {printf("ERROR: TraceAligner ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); raise(SIGABRT); exit(-1);}}while (0)


#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#else
#define dbg_assure(cond) //
#endif

#define MAX(a,b) (a > b) ? (a) : (b)
#define MIN(a,b) (a < b) ? (a) : (b)


#ifdef HAVE_FILESYSTEM
#include <filesystem>

#ifdef __APPLE__
using namespace std::__fs;
#endif //__APPLE__

#else
#include <dirent.h>
//really crappy implementation in case <filesystem> isn't available :o
class myfile_dir{
    std::string _path;
public:
    myfile_dir(std::string p): _path(p){}
    std::string path(){return _path;}
};

class diriter{
public:
    std::vector<myfile_dir> _file;
    auto begin(){return _file.begin();}
    auto end(){return _file.end();}
};

namespace std {
    namespace filesystem{
        static diriter directory_iterator(std::string);
    }
}

static diriter std::filesystem::directory_iterator(std::string dirpath){
    DIR *dir = NULL;
    struct dirent *ent = NULL;
    diriter ret;
    
    assure(dir = opendir(dirpath.c_str()));
    
    while ((ent = readdir (dir)) != NULL) {
        if ((ent->d_type != DT_REG && ent->d_type != DT_DIR) || ent->d_name[0] == '.')
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

__inline__ __v16qi abs(__v16qi val){
    __v16qi zero = _mm_set1_epi8(0);
    __v16qi neg = _mm_sub_epi8(zero, val);
    __v16qi absv = _mm_max_epi8(neg, val);
    return absv;
}

//Sub method
int maxfaltung(const int8_t *t1, const int8_t *t2, int size, int move_radius){
    int faltn = 0;
    int64_t minFaltWert = LLONG_MAX;

    __v16qi zero = _mm_setzero_si128();
    for (int n=-move_radius; n<move_radius; n++) {
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


int doNormalize(std::queue<LoadFile *> &intraces, const char *outdir, int threadsCnt, uint32_t point_per_trace, uint32_t align_window_start, uint32_t align_window_size, int move_radius){
    printf("start\n");
    atomic<uint16_t> wantWriteCounter{0};
    vector<future<void>> workers;

    int8_t *meanTrace = NULL;

    printf("threads=%d\n",threadsCnt);
    printf("outdir=%s\n",outdir);

    
    printf("reading trace\n");
    mutex meanLock;
    mutex listLock;
    mutex fileWriteEvent;
    
    size_t filesCnt = intraces.size();
        
    atomic_uint16_t activeWorkerCnt{0};

    auto workerfunc = [&](int workerNum)->void{
        printf("[T-%d] worker started\n",workerNum);
        ++activeWorkerCnt;
        while (true) {
            printf("[T-%d] getting tracesfile...\n",workerNum);
            listLock.lock();
            size_t todoCnt = intraces.size();
            if (intraces.size()) {
                vector<tuple<int8_t*, uint8_t *, uint8_t *>> *tstarts = new vector<tuple<int8_t*, uint8_t *, uint8_t *>>();
                struct stat fstat = {};
                LoadFile *lf = intraces.front();
                intraces.pop();
                listLock.unlock();
                
                std::string tpath = lf->path();
                string filename = tpath.substr(tpath.find_last_of("/")+1);
                string outfilepath = outdir;
                if (outfilepath.back() != '/') {
                    outfilepath+= '/';
                }
                outfilepath+="normalized_";
                outfilepath+=filename;

                printf("[T-%d] [ %3lu of %3lu] normalizing trace=%s\n",workerNum,(filesCnt+1-todoCnt),filesCnt,tpath.c_str());

                
                uint32_t tracesInFile = 0;
                int tnum = -1;
                for (auto ms : *lf) {
                    tnum++;

                    int off = 0;
                    if (!meanTrace) {
                        meanLock.lock();
                        if (!meanTrace) {
                            //first set a mean
                            assure(meanTrace = (int8_t*)malloc(align_window_size * sizeof(*meanTrace)));
                            memcpy(meanTrace, (int8_t*)&ms->Trace[align_window_start], align_window_size); // make sure it's allocated aligned
                        }
                        meanLock.unlock();
                    }else{
                        off = maxfaltung(meanTrace, &ms->Trace[align_window_start], align_window_size, move_radius);
                    }


                    tracesInFile++;
                    tstarts->push_back({(int8_t*)&ms->Trace[move_radius + off]/*Trace*/,(uint8_t*)ms->Plaintext/*Plaintext*/,(uint8_t*)ms->Ciphertext/*Ciphertext*/});
                    
#ifdef DEBUG
                    if (tnum % 100 == 0) {
                        printf("[T-%d] tracenum=%d\n",workerNum,tnum);
                    }
#endif
                }
                
                auto writefunc = [&fileWriteEvent, &wantWriteCounter, point_per_trace, move_radius](LoadFile *curtrace,vector<tuple<int8_t*, uint8_t *, uint8_t *>> *tstarts, uint32_t tracesInFile, string outfilepath, int workerNum)->void{
                    printf("[WT-%d] saving normalized trace...\n",workerNum);
                    FILE *f = fopen(outfilepath.c_str(), "wb");
                    fwrite(&tracesInFile, 1, sizeof(tracesInFile), f); //TracesInFile (actual data!)

                    uint32_t np = point_per_trace - 2*move_radius;
                    for (auto p : *tstarts) {
                        fwrite(&np, 1, sizeof(np), f); //PointsPerTrace
                        fwrite(get<1>(p), 1, sizeof(LoadFile::MeasurementStructure::Plaintext), f); //Plaintext
                        fwrite(get<2>(p), 1, sizeof(LoadFile::MeasurementStructure::Ciphertext), f); //Ciphertext
                        fwrite(get<0>(p), 1, np, f); //Trace
                    }
                    fclose(f);
                    delete curtrace;
                    delete tstarts;
                    printf("[WT-%d] done saving normalized trace to %s! (%u remaining writers)\n",workerNum, outfilepath.c_str(),(uint16_t)wantWriteCounter-1);
                    wantWriteCounter--;
                    fileWriteEvent.unlock(); //send wantWriteCounter update event
                };
                
                
                auto compressedwritefunc = [&fileWriteEvent, &wantWriteCounter, point_per_trace, move_radius](LoadFile *curtrace,vector<tuple<int8_t*, uint8_t *, uint8_t *>> *tstarts, uint32_t tracesInFile, string outfilepath, int workerNum)->void{
                    printf("[WT-%d] compressing normalized trace...\n",workerNum);
                    ssize_t lastSlash = outfilepath.rfind("/");
                    std::string outfilename = outfilepath;
                    if (lastSlash != std::string::npos) {
                        outfilename = outfilepath.substr(lastSlash+1);
                    }
                    
                    outfilepath += ".tar.gz";
                    
                    struct archive *a = NULL;
                    struct archive_entry *entry = NULL;
                    a = archive_write_new();
                    archive_write_add_filter_gzip(a);
                    archive_write_set_format_pax_restricted(a);
                    archive_write_open_filename(a, outfilepath.c_str());
                    
                    uint32_t np = point_per_trace - 2*move_radius;
                    size_t fileSize = sizeof(tracesInFile) + tstarts->size() * (sizeof(np) + sizeof(LoadFile::MeasurementStructure::Plaintext) + sizeof(LoadFile::MeasurementStructure::Ciphertext) + np);
                    
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
                        archive_write_data(a, get<1>(p), sizeof(LoadFile::MeasurementStructure::Plaintext)); //Plaintext
                        archive_write_data(a, get<2>(p), sizeof(LoadFile::MeasurementStructure::Ciphertext)); //Ciphertext
                        archive_write_data(a, get<0>(p), np); //Trace
                    }
                    archive_entry_free(entry);
                    
                    archive_write_close(a); // Note 4
                    archive_write_free(a); // Note 5

                    delete curtrace;
                    delete tstarts;
                    printf("[WT-%d] done saving compressed normalized trace to %s! (%u remaining writers)\n",workerNum, outfilepath.c_str(),(uint16_t)wantWriteCounter-1);
                    wantWriteCounter--;
                    fileWriteEvent.unlock(); //send wantWriteCounter update event
                };
                
                
                
                wantWriteCounter++;
//                std::thread wthread(writefunc,lf,tstarts,tracesInFile,outfilepath,workerNum);
                std::thread wthread(compressedwritefunc,lf,tstarts,tracesInFile,outfilepath,workerNum);
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

    free(meanTrace);
    
    printf("done!\n");
    return 0;
}
