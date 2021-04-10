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
#include "PosAligner.hpp"

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)


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
        static diriter directory_iterator(std::string);
    }
}

static diriter std::filesystem::directory_iterator(std::string dirpath){
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

int doAlign(const char *indir, const char *outdir){
    printf("start doAlign\n");

    printf("indir=%s\n",indir);
    printf("outdir=%s\n",outdir);

    vector<vector<string>> posLists;
     
    printf("reading trace\n");
    for(auto& p: filesystem::directory_iterator(indir)){
        int ypos = -1;
        int xpos = -1;
        
        std::string dirname = p.path().c_str();
        ssize_t lastSlashPos = dirname.rfind("/");
        if (lastSlashPos == std::string::npos) {
            lastSlashPos = -1;
        }
        lastSlashPos++;
        dirname = dirname.substr(lastSlashPos);
        sscanf(dirname.c_str(), "Y%d_X%d",&ypos,&xpos);
        if (xpos < 0 || ypos < 0) {
            printf("failed to parse path=%s\n",p.path().c_str());
            continue;
        }
        
        std::vector<string> tracesList;
        printf("adding pos=%s\n",dirname.c_str());
        for(auto& p: filesystem::directory_iterator(p.path())){
            if (!ends_with(p.path(), ".dat") && !ends_with(p.path(), ".dat.tar.gz")) {
                continue;
            }
            printf("\tadding to list=%s\n",p.path().c_str());
            tracesList.push_back(p.path());
        }
        
        if (tracesList.size()) {
            posLists.push_back(tracesList);
        }
    }

    cl_uint point_per_trace = 0;
    {
        printf("reading one trace for getting filesize...\n");
        LoadFile tr(posLists.at(0).at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
    }
    printf("point_per_trace=%u\n",point_per_trace);

    printf("\n");


    printf("Setting up GPUs...\n");
    // Create a program from the kernel source
#ifdef XCODE
    LoadFile kernelfile("../../../align_chip_opencl/kernels.cl");
#else
    LoadFile kernelfile("kernels.cl");
#endif

    std::string kernelsource{(char*)kernelfile.buf()};

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
    GPUDevice gpu0(0,device_ids[0],kernelsource, point_per_trace);gpus.push_back(&gpu0);
#else
    assure_(num_devices>3);

//    GPUDevice gpu0(0,device_ids[0],kernelsource, point_per_trace);gpus.push_back(&gpu0);
//    GPUDevice gpu1(1,device_ids[1],kernelsource, point_per_trace);gpus.push_back(&gpu1);
//    GPUDevice gpu2(2,device_ids[2],kernelsource, point_per_trace);gpus.push_back(&gpu2);
    GPUDevice gpu3(3,device_ids[3],kernelsource, point_per_trace);gpus.push_back(&gpu3);
#endif

    std::vector<PosAligner*> posAligners;

    std::vector<future<void>> alignerThreads;

    for (auto gpu : gpus) {
        posAligners.push_back(new PosAligner(gpu));
    }
    
    {
        int i=0;
        for (auto &pos : posLists) {
            posAligners.at(i++ % posAligners.size())->enqueuePosList(pos);
        }
    }
    
    
    {
        int i = 0;
        for (auto &pA : posAligners) {
            alignerThreads.push_back(std::async(std::launch::async,[outdir](PosAligner *aligner, int anum)->void{
                printf("Aligner %d running...\n",anum);
                aligner->run(outdir);
                printf("Aligner %d done!\n",anum);
            },pA,i++));
        }
    }

    printf("Waiting for gpu transfers to finish!\n");
    for (auto &t : alignerThreads) {
        t.wait();
    }
    return 0;
}

int r_main(int argc, const char * argv[]) {
    int ret = 0;
    if (argc < 3) {
        printf("Usage: %s <pos dir path> <outdir>\n",argv[0]);
        return -1;
    }
    

    const char *indir = argv[1];
    const char *outdir = argv[2];


    
    ret = doAlign(indir, outdir);

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

