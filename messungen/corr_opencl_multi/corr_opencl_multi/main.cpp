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

int doCorr(const char *indir, const char *outdir, long maxTraces){
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

        if (maxTraces && toNormalizeList.size() >= maxTraces) {
            printf("limiting list to %ld files\n",maxTraces);
            break;
        }
    }

    cl_uint point_per_trace = 0;
    {
        printf("reading one trace for getting filesize at=%s\n",toNormalizeList.at(0).c_str());
        LoadFile tr(toNormalizeList.at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
    }
    printf("point_per_trace=%u\n",point_per_trace);

    printf("\n");

    std::reverse(toNormalizeList.begin(),toNormalizeList.end());

    // Create a program from the kernel source
#ifdef DEBUG
    LoadFile kernelfile("../../../corr_opencl_multi/kernels.cl");
#else
    LoadFile kernelfile("kernels.cl");
#endif
    
    std::string kernelsource{(char*)kernelfile.buf()};

    std::vector<std::string> powermodels;

//    powermodels.push_back("powerModel_DEC_INPUT");
//    powermodels.push_back("powerModel_DEC_INPUT_HW");
//    powermodels.push_back("powerModel_DEC_INPUT_HW4");
//    powermodels.push_back("powerModel_DEC_INPUT_KEY_HW");
//    powermodels.push_back("powerModel_SBOX");
//    powermodels.push_back("powerModel_SBOX_HW");
//    powermodels.push_back("powerModel_ZEROVALUE");
//    powermodels.push_back("powerModel_DEC_OUTPUT");
//    powermodels.push_back("powerModel_DEC_OUTPUT_HW");
//    powermodels.push_back("powerModel_DEC_OUTPUT_HW4");
//    powermodels.push_back("powerModel_TTABLET0_HW4");
//    powermodels.push_back("powerModel_TTABLET1_HW4");
//    powermodels.push_back("powerModel_TTABLET2_HW4");
//    powermodels.push_back("powerModel_TTABLET3_HW4");
//    powermodels.push_back("powerModel_TTABLET0T1_HW4");
//    powermodels.push_back("powerModel_TTABLET0T1T2_HW4");
//    powermodels.push_back("powerModel_TTABLET0T1T2T3_HW4");
//    powermodels.push_back("powerModel_TTABLET0P0_HW4");
//    powermodels.push_back("powerModel_TTABLET0P0K0_HW4");
//    powermodels.push_back("powerModel_P0K0_HW4");

    
    
    std::vector<std::string> roundModels;

//    roundModels.push_back("powerModel_ROUND_HW");
//    roundModels.push_back("powerModel_ROUND_HW4");
//    roundModels.push_back("powerModel_ROUND_VALUE");
//    roundModels.push_back("powerModel_ROUND_SBOX");
//    roundModels.push_back("powerModel_ROUND_SBOX_HW");
//    roundModels.push_back("powerModel_ROUND_MC");
//    roundModels.push_back("powerModel_ROUND_MC_HW");
    
//    roundModels.push_back("powerModel_ROUND_HW128");
//    roundModels.push_back("powerModel_ROUND_MC_HW128");
//    roundModels.push_back("powerModel_ROUND_SBOX_HW128");
        
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW128");
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_MC_HW128");
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_SBOX_HW128");


//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW128");
//
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW64_0");
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW64_1");
//
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW32_0");
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW32_1");
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW32_2");
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW32_3");
//
//
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW16_0");
//    roundModels.push_back("powerModel_ROUND_XOR_NEXT_HW16_1");

    
//    for (auto &rm : roundModels){
//        for (int i=10; i<=14; i++) {
//            constexpr const char replStr[] = "_ROUND_";
//            ssize_t roundStr = rm.find(replStr);
//            assure_(roundStr != std::string::npos);
//            std::string newStr{"_ROUND"};
//            newStr += to_string(i);
//            newStr += "_";
//            std::string insertStr = rm;
//            powermodels.push_back(insertStr.replace(roundStr, strlen(replStr), newStr));
//        }
//    }
//
//
//    /* These are just Plaintext ^ Key */
//    powermodels.push_back("powerModel_ROUND1_HW128");
//
//    powermodels.push_back("powerModel_ROUND1_HW64_0");
//
//    powermodels.push_back("powerModel_ROUND1_HW32_0");
//    powermodels.push_back("powerModel_ROUND1_HW32_1");
//
//    powermodels.push_back("powerModel_ROUND1_HW16_0");
//    powermodels.push_back("powerModel_ROUND1_HW16_1");

    
    
    for (int i=0; i<16; i++){
        powermodels.push_back("powerModel_ROUND14_XOR_NEXT_HW8_"+to_string(i));
        powermodels.push_back("powerModel_ROUND13_XOR_NEXT_HW8_"+to_string(i));
//        powermodels.push_back("powerModel_ROUND_XOR_NEXT_zeroval_"+to_string(i));
    }

//    for (int i=0; i<8; i++) {
//        powermodels.push_back("powerModel_ROUND_XOR_NEXT_byte0_HW1_"+to_string(i));
//    }
    
    
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
    
    for (auto &gpu : gpus) {
        gpuTransferThreads.push(new std::thread([&cloader](GPUDevice *gpu)->void{
            while (LoadFile *file = cloader.dequeue()) {
                gpu->transferTrace(file);
                delete file;
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
    std::map<std::string,CPUModelResults*> modelresults;
    
    for (auto &gpu : gpus) {
        gpu->getResults(modelresults);
    }

    for (auto &model : modelresults) {
        std::string outfile = outdir;
        if (outfile.back() != '/') {
            outfile += '/';
        }
        outfile += "CORR-";
        outfile += model.first;

        printf("computing CORR - %s\n",model.first.c_str());

        myfile out(outfile.c_str(),point_per_trace*sizeof(gpu_float_type));
        memset(&out[0], 0, point_per_trace*sizeof(gpu_float_type));
        
        gpu_float_type *upTrace = model.second->upperPart().trace;
        gpu_float_type hypotCensum = model.second->censumHypot().trace[0];
        
        gpu_float_type *censumTrace = model.second->censumTrace().trace;
        
        for (int i=0; i<point_per_trace; i++) {
            gpu_float_type upper = upTrace[i];
            
            gpu_float_type realCensum = censumTrace[i];
            
            gpu_float_type preLower = realCensum * hypotCensum;
            
            gpu_float_type lower = sqrt((double)preLower);
            
            out[i] = upper/lower;
        }
    }
    
    
    printf("Main: freeing memory\n");
    for (auto &r : modelresults) {
        free(r.second);r.second = NULL;
    }
    
    return 0;
}

int r_main(int argc, const char * argv[]) {
    int ret = 0;
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outdir> {maxTraces}\n",argv[0]);
        return -1;
    }
    
    long maxTraces = 0;

    const char *indir = argv[1];
    const char *outdir = argv[2];
        
    if (argc > 3) {
        maxTraces = atol(argv[3]);
        printf("maxTraces=%ld\n",maxTraces);
    }
    
    ret = doCorr(indir, outdir, maxTraces);

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

