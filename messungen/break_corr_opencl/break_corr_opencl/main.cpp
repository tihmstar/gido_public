//
//  main.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <iostream>
#include <vector>
#include <algorithm>
#include <signal.h>
#include <fcntl.h>
#include "LoadFile.hpp"
#include "GPUDevice.hpp"
#include "CPULoader.hpp"
#include <future>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <unistd.h>
#include <string.h>


#define assure(cond) do {if (!(cond)) {printf("ERROR: MAIN ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: MAIN ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)


#warning custom point_per_trace
#define NUMBER_OF_POINTS_OVERWRITE 8000

using namespace std;

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
        diriter directory_iterator(std::string);
    }
}

diriter std::filesystem::directory_iterator(std::string dirpath){
    DIR *dir = NULL;
    struct dirent *ent = NULL;
    diriter ret;
    
    assure_(dir = opendir(dirpath.c_str()));
    
    while ((ent = readdir (dir)) != NULL) {
        if ((ent->d_type != DT_REG && ent->d_type != DT_DIR) || ent->d_name[0] == '.')
            continue;
        ret._file.push_back({dirpath + "/" + ent->d_name});
    }
    
    if (dir) closedir(dir);
    return ret;
}
#endif

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

class myfile{
    int _fd;
    gpu_float_type *_mem;
    size_t _size;
public:
    myfile(const char *fname, size_t size) : _fd(-1),_size(size){
        assure_((_fd = open(fname, O_RDWR | O_CREAT, 0644))>0);
        lseek(_fd, _size-1, SEEK_SET);
        write(_fd, "", 1);
        assure_(_mem = (gpu_float_type*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
    }
    ~myfile(){
        munmap(_mem, _size);
        if (_fd > 0) close(_fd);
    }
    gpu_float_type &operator[](int at){
        return _mem[at];
    }
};

int doCorr(const char *indir, const char *outdir, int beginKeyByte, int endKeyByte){
    printf("start doCorr\n");

    printf("indir=%s\n",indir);
    printf("outdir=%s\n",outdir);

    vector<string> toNormalizeList;
     
    printf("reading trace\n");
    for(auto& p: filesystem::directory_iterator(indir)){
        if (!ends_with(p.path(), ".dat") && !ends_with(p.path(), ".dat.tar.gz")) {
            continue;
        }
        printf("adding to list=%s\n",p.path().c_str());
        toNormalizeList.push_back(p.path());
    }

    cl_uint point_per_trace = 0;
    {
        printf("reading one trace for getting filesize...\n");
        LoadFile tr(toNormalizeList.at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
    }
    
#ifdef NUMBER_OF_POINTS_OVERWRITE
    point_per_trace = NUMBER_OF_POINTS_OVERWRITE;
#endif
    
    
    printf("point_per_trace=%u\n",point_per_trace);

    printf("\n");

    std::reverse(toNormalizeList.begin(),toNormalizeList.end());

    // Create a program from the kernel source
#ifdef XCODE
    LoadFile kernelfile("../../../break_corr_opencl/kernels.cl");
#else
    LoadFile kernelfile("kernels.cl");
#endif
    
    std::string kernelsource{(char*)kernelfile.buf()};

    std::vector<std::string> powermodels;


    
    if (beginKeyByte < 0 || beginKeyByte >= 32) {
        beginKeyByte = 0;
    }
    
    if (!endKeyByte) {
        endKeyByte = 32;
    }
    
    for (int i=beginKeyByte; i<endKeyByte; i++) {
        powermodels.push_back("powerModel_ATK_"+std::to_string(i));
    }
    

    for (auto &m : powermodels) {
        printf("Model=%s\n",m.c_str());
    }
    
    printf("Got %3lu powerModels!\n",powermodels.size());
    
    // Get platform and device information
    cl_int clret = 0;

    cl_platform_id platform_id = NULL;
    cl_uint num_platforms = 0;
    clret = clGetPlatformIDs(1, &platform_id, &num_platforms);assure(!clret);
    cl_uint num_devices = 0;
    cl_device_id device_ids[4] = {};
    clret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 4, device_ids, &num_devices);assure(!clret);

    std::vector<GPUDevice*> gpus;

#pragma mark setup gpu here

#ifdef DEBUG
    GPUDevice gpu0(0,device_ids[0],kernelsource, point_per_trace,powermodels);gpus.push_back(&gpu0);
#else
    assure_(num_devices>3);

    GPUDevice gpu1(1,device_ids[1],kernelsource, point_per_trace,powermodels);gpus.push_back(&gpu1);
    GPUDevice gpu2(2,device_ids[2],kernelsource, point_per_trace,powermodels);gpus.push_back(&gpu2);
    GPUDevice gpu3(3,device_ids[3],kernelsource, point_per_trace,powermodels);gpus.push_back(&gpu3);
#endif

    size_t systemRam = (size_t)5e9;

    uint64_t cpuloadersCnt = sysconf(_SC_NPROCESSORS_ONLN);
#define MAX_CPU_LOADER_THREADS 5
    if (cpuloadersCnt > MAX_CPU_LOADER_THREADS) {
        cpuloadersCnt = MAX_CPU_LOADER_THREADS;
    }
    
    CPULoader cloader(toNormalizeList, systemRam, cpuloadersCnt);
    std::queue<std::thread *> gpuTransferThreads;
    
    
#define MIN_TRANSFER_CNT 20000
#define MAX_TRANSFER_CNT 25000

    
    for (auto &gpu : gpus) {
        gpuTransferThreads.push(new std::thread([&cloader](GPUDevice *gpu)->void{
            LoadFile *toTransfer = NULL;
            
            while (LoadFile *file = cloader.dequeue()) {
                if (toTransfer) { //if we have "toTransfer"
                    
                    if (
                        toTransfer->tracesInFile() >= MIN_TRANSFER_CNT //if "toTransfer" already large enough
                        || toTransfer->tracesInFile() + file->tracesInFile() > MAX_TRANSFER_CNT //or combination would exceed MAX limit
                        ) {
                        //then transfer the pendin trace
                        gpu->transferTrace(toTransfer);
                        delete toTransfer;
                        toTransfer = NULL;
                    }
                }
                
                if (!toTransfer) { //if we don't have a pending trace anymore, make "file" be our new pending trace
                    toTransfer = file; file = NULL;
                    continue;
                }
                
                //if we have both: "transferTrace" and "file", then we should combine them now
                printf("[DW-%u] combining traces (%5u + %5u = %5u)\n",gpu->deviceNum(),toTransfer->tracesInFile(), file->tracesInFile(), toTransfer->tracesInFile() + file->tracesInFile());
                *toTransfer += *file;
                
                //once we combined traces, we can discard "file"
                if (file) {
                    delete file; file = NULL;
                }
            }
            
            //final transfer
            if (toTransfer) {
                gpu->transferTrace(toTransfer);
                delete toTransfer;
                toTransfer = NULL;
            }
            
            printf("GPU %d done transfer!\n",gpu->deviceNum());
        },gpu));
    }
        
    printf("Waiting for gpu transfers to finish (%lu threads)!\n",gpuTransferThreads.size());
    while (gpuTransferThreads.size()) {
        auto t = gpuTransferThreads.front();
        gpuTransferThreads.pop();
        t->join();
        delete t;
    }

    printf("All GPU transfers finished, getting results...\n");
    std::map<std::string,std::map<std::string,CPUModelResults*>> allmodelresults;
    
    for (auto &gpu : gpus) {
        gpu->getResults(allmodelresults);
    }

    atomic<int> doneThreads{0};
    std::mutex doneThreadsEventLock;
    
    for (auto &modelresults : allmodelresults) {
        std::thread k([&modelresults,outdir,point_per_trace,&doneThreads,&doneThreadsEventLock](){
            auto trace_vals = modelresults.second.at("trace");

            gpu_float_type *censumTrace = trace_vals->censumTrace().trace;
            
            int doneKeyguesses = 0;
            int totalKeyguesses = modelresults.second.size()-1;
            for (auto &model : modelresults.second) {
                if (!isdigit(model.first.c_str()[0])) {
                    printf("skipping model=%s\n",model.first.c_str());
                    continue;
                }

                std::string outfile = outdir;
                if (outfile.back() != '/') {
                    outfile += '/';
                }
                
                int32_t guess = atoi(model.first.c_str());
                
                outfile += "CORR-";
                outfile += modelresults.first;
                outfile += "-KEYGUESS_";
                outfile += std::to_string(guess);

                printf("computing CORR - %s - Keyguess=0x%04x (0x%08x of 0x%08x)\n",modelresults.first.c_str(),guess,doneKeyguesses,totalKeyguesses);

                myfile out(outfile.c_str(),point_per_trace*sizeof(gpu_float_type));
                memset(&out[0], 0, point_per_trace*sizeof(gpu_float_type));
                
                gpu_float_type *upTrace = model.second->upperPart().trace;
                gpu_float_type hypotCensum = model.second->censumHypot().trace[0];
                
                
                for (int i=0; i<point_per_trace; i++) {
                    gpu_float_type upper = upTrace[i];
                    
                    gpu_float_type realCensum = censumTrace[i];
                    
                    gpu_float_type preLower = realCensum * hypotCensum;
                    
                    gpu_float_type lower = sqrt((double)preLower);
                    
                    out[i] = upper/lower;
                }
                doneKeyguesses++;
            }
            
            
            printf("[%s] freeing memory\n",modelresults.first.c_str());
            for (auto &r : modelresults.second) {
                free(r.second);r.second = NULL;
            }
            ++doneThreads;
            doneThreadsEventLock.unlock();
        });
        k.detach();
    }
    
    printf("waiting for corr threads to finish...\n");
    while (doneThreads < allmodelresults.size()) {
        printf("waiting for corr threads to finish. %d of %lu done!\n",(int)doneThreads,allmodelresults.size());
        doneThreadsEventLock.lock();
    }
    
    
    printf("Done!\n");

    
    return 0;
}

int r_main(int argc, const char * argv[]) {
    int ret = 0;
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outdir> {begin byte} {end byte}\n",argv[0]);
        return -1;
    }
    
    int beginKeyByte = 0;
    int endKeyByte = 0;

    const char *indir = argv[1];
    const char *outdir = argv[2];
        
    if (argc > 3) {
        beginKeyByte = atoi(argv[3]);
        printf("beginKeyByte=%d\n",beginKeyByte);
    }
    
    if (argc > 4) {
        endKeyByte = atoi(argv[4]);
        printf("endKeyByte=%d\n",endKeyByte);
    }

    ret = doCorr(indir, outdir, beginKeyByte, endKeyByte);

    printf("done!\n");
    return ret;
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

