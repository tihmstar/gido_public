//
//  main.cpp
//  attack_opencl
//
//  Created by tihmstar on 13.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "Traces.hpp"
#include <string.h>
#include <iostream>
#include <future>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <exception>
#include <stdlib.h>
#include <fcntl.h>
#include "numerics.hpp"
#include "GPUPowerModel.hpp"
#include <map>
#include "CPUModelResults.hpp"
#include "GPUDeviceWorker.hpp"
#include <algorithm>
#include <dirent.h>
#include <signal.h>


#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#define debug(a ...) printf(a)
#else
#define dbg_assure(cond) //
#define debug(a ...) //
#endif

//#ifndef NOMAIN
//#define HAVE_FILESYSTEM
//#endif


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

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int doSNR(const char *indir, const char *outdir, long maxTraces){
    printf("start\n");

    printf("indir=%s\n",indir);
    printf("outdir=%s\n",outdir);
    
    vector<string> toNormalizeList;
    mutex listLock;

        
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

    size_t tracefileSize = 0;
    cl_uint point_per_trace = 0;
    {
        printf("reading one trace for getting filesize...\n");
        Traces tr(toNormalizeList.at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
        tracefileSize = tr.bufSize();
    }
    printf("point_per_trace=%u\n",point_per_trace);
    size_t filesCnt = toNormalizeList.size();

    printf("\n");
    
    std::reverse(toNormalizeList.begin(),toNormalizeList.end());

    
    // Create a program from the kernel source
#ifdef DEBUG
    Traces kernelfile("../../../SNR_opencl_multi/kernels.cl");
#else
    Traces kernelfile("kernels.cl");
#endif
    
    std::string kernelsource{(char*)kernelfile.buf()};

    std::vector<std::pair<std::string,uint16_t>> powermodels;
    
    for (int i=1; i<8; i++) {
//        powermodels.push_back({to_string(i) + "_traceSelector_INPUT_HW", 9});
        powermodels.push_back({to_string(i) + "_traceSelector_OUTPUT_HW", 9});
    }

    
    // Get platform and device information
    cl_int clret = 0;

    cl_platform_id platform_id = NULL;
    cl_uint num_platforms = 0;
    clret = clGetPlatformIDs(1, &platform_id, &num_platforms);assure(!clret);
    cl_uint num_devices = 0;
    cl_device_id device_ids[4] = {};
    clret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 4, device_ids, &num_devices);assure(!clret);
    
    
    std::vector<GPUDeviceWorker*> gpus;
#ifdef DEBUG
    GPUDeviceWorker gpu0(0,device_ids[0],kernelsource, point_per_trace, tracefileSize,powermodels);gpus.push_back(&gpu0);
#else
    assure_(num_devices>3);
//    GPUDeviceWorker gpu2(2,device_ids[2],kernelsource, point_per_trace, tracefileSize,powermodels);gpus.push_back(&gpu2);
    GPUDeviceWorker gpu3(3,device_ids[3],kernelsource, point_per_trace, tracefileSize,powermodels);gpus.push_back(&gpu3);
#endif

    size_t systemRam = (size_t)20e9;

    uint64_t cpuloadersCnt = sysconf(_SC_NPROCESSORS_ONLN);

    cpuloadersCnt = 8;
        
    atomic_uint16_t activeCpuLoaderCnt{0};
    auto cpuloader = [&listLock,&toNormalizeList,&filesCnt, &gpus, &activeCpuLoaderCnt, systemRam](int transferWorkerNum)->void{
        ++activeCpuLoaderCnt;
        /*
         loaders are split into gpus.size()-groups.
         each gpu has their own set of loaders.
         each trace is assigned to exactly one gpu
         */
        int myGPU = transferWorkerNum % gpus.size();

        std::queue<std::pair<size_t,CPUMem *>>cpumems;
        
        while (true) {
            printf("[C-%02d][G-%02d] getting tracesfile...\n",transferWorkerNum,myGPU);
            listLock.lock();
            size_t todoCnt = toNormalizeList.size();
            if (toNormalizeList.size()) {
                string tpath = toNormalizeList.back();
                toNormalizeList.pop_back();
                listLock.unlock();
                //START WORK
                printf("[C-%02d][G-%02d] [ %3lu of %3lu] loading trace=%s\n",transferWorkerNum,myGPU,(filesCnt+1-todoCnt),filesCnt,tpath.c_str());

                
                CPUMem *curtrace = new CPUMem(new Traces(tpath.c_str()),systemRam);
                cpumems.push({(filesCnt+1-todoCnt),curtrace});

                gpus.at(myGPU)->enqueuTrace(curtrace);

                curtrace->release();
                //END WORK
            }else{
                listLock.unlock();
                printf("[C-%02d][G-%02d] no more traces available\n",transferWorkerNum,myGPU);
                break;
            }
            
            while (cpumems.size()) {
                auto cur = cpumems.front();
                if (cur.second->getRefcnt() == 0){ //delete if done
                    cpumems.pop();
                    delete cur.second;
                    cur.second = NULL;
                    printf("[C-%02d][G-%02d] [ %3lu of %3lu] Done!\n",transferWorkerNum,myGPU,cur.first,filesCnt);
                }else{
                    break;
                }
            }
        }
        while (cpumems.size()) {
            auto cur = cpumems.front();
            cpumems.pop();
            delete cur.second;
            cur.second = NULL; //block until done and delete
            printf("[C-%02d][G-%02d] [ %3lu of %3lu] Done!\n",transferWorkerNum,myGPU,cur.first,filesCnt);
        }
        --activeCpuLoaderCnt;
        printf("[C-%02d][G-%02d] loader finished! (%2u loaders still active)\n",transferWorkerNum,myGPU,(uint16_t)activeCpuLoaderCnt);
    };

    
    printf("spinning up %lu cpu-loader workers\n",cpuloadersCnt);
    vector<future<void>> cpuLoaderWorkers;


    for (uint64_t i=0; i<cpuloadersCnt; i++){
        cpuLoaderWorkers.push_back(std::async(std::launch::async,cpuloader,i));
    }
    
    printf("waiting for cpu-transfer workers to finish...\n");
    for (auto &w : cpuLoaderWorkers){
        w.wait();
    }
    printf("all cpu-loader workers finished!\n");
    
    
    std::map<std::string,CPUModelResults*> modelresults;
    
    printf("Getting results!\n");
    for (auto &gpu : gpus) {
        gpu->getResults(modelresults);
    }
    
    
    for (auto &model : modelresults) {
        std::string outfile = outdir;
        if (outfile.back() != '/') {
            outfile += '/';
        }
        outfile += "SNR-";
        outfile += model.first;

        printf("computing SNRs - %s\n",model.first.c_str());
        gpu_float_type *meanOfMeans = new gpu_float_type[point_per_trace];
        gpu_float_type *meanOfVariances = new gpu_float_type[point_per_trace];
        gpu_float_type *variancesOfMeans = new gpu_float_type[point_per_trace];

        memset(meanOfMeans, 0, sizeof(gpu_float_type)*point_per_trace);
        memset(meanOfVariances, 0, sizeof(gpu_float_type)*point_per_trace);
        memset(variancesOfMeans, 0, sizeof(gpu_float_type)*point_per_trace);

        computeMeanOfMeans(model.second->groupsMean(), meanOfMeans, point_per_trace);
        computeVariancesOfMeans(model.second->groupsMean(), meanOfMeans, variancesOfMeans, point_per_trace);
        computeMeanOfVariancesFromCensums(model.second->groupsCensum(), meanOfVariances, point_per_trace);


        printf("generating file=%s\n",outfile.c_str());
        auto snrTrace = myfile(outfile.c_str(),point_per_trace*sizeof(gpu_float_type));
        memset(&snrTrace[0], 0, point_per_trace*sizeof(gpu_float_type));

        for (uint32_t i=0; i<point_per_trace; i++) {
            gpu_float_type tmp = variancesOfMeans[i] / meanOfVariances[i];
            snrTrace[i] = tmp;
        }
        
        //free memory
        for (int i=0; i<model.second->groupsMean().size(); i++) {
            free(model.second->groupsMean().at(i).trace);model.second->groupsMean().at(i).trace= NULL;
            free(model.second->groupsCensum().at(i).trace);model.second->groupsCensum().at(i).trace = NULL;
        }
        delete [] meanOfMeans;
        delete [] meanOfVariances;
        delete [] variancesOfMeans;
    }
    
    printf("Main: freeing memory\n");
    for (auto &mr : modelresults){
        delete mr.second; mr.second = NULL;
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

    
    ret = doSNR(indir, outdir, maxTraces);
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

