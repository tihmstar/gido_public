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

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); throw __LINE__;}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); throw __LINE__;}}while (0)

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

int doSNR(const char *indir, const char *outfile, int maxTraces){
    printf("start\n");

    printf("indir=%s\n",indir);
    printf("outfile=%s\n",outfile);
    
    vector<string> toNormalizeList;
    mutex listLock;

        
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

    size_t tracefileSize = 0;
    cl_uint point_per_trace = 0;
    {
        Traces tr(toNormalizeList.at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
        tracefileSize = tr.bufSize();
    }
    printf("point_per_trace=%u\n",point_per_trace);
    size_t filesCnt = toNormalizeList.size();

    printf("\n");
    
    
    

    //BEGIN DEVICE WORKER
    auto deviceWorker = [point_per_trace,tracefileSize, &listLock, &toNormalizeList, &filesCnt](cl_device_id device_id, int deviceWorkerNum, std::vector<combtrace> &retgroupsMean, std::vector<combtrace> &retgroupsCensum)->int{
        cl_int clret = 0;

        cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &clret);assure(!clret);
        
        // Create a program from the kernel source
        cl_program program = NULL;
        {
            //hack to use mmap inside the Trace class
#ifdef DEBUG
            Traces kernelfile("../../../SNR_opencl/kernels.cl");
#else
            Traces kernelfile("kernels.cl");
#endif
            const char *fileBuf = (const char *)kernelfile.buf();
            size_t fileSize = kernelfile.bufSize();
            program = clCreateProgramWithSource(context, 1, &fileBuf, &fileSize, &clret);assure(!clret);
        }

        // Build the program
        clret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
        if (clret == CL_BUILD_PROGRAM_FAILURE) {
            // Determine the size of the log
            size_t log_size;
            clret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);assure(!clret);

            // Allocate memory for the log
            char *log = (char *) malloc(log_size);

            // Get the log
            clret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);assure(!clret);

            // Print the log
            printf("%s\n", log);
            assure(0);
        }
        assure(!clret);
        
        cl_device_fp_config fpconfig = 0;
        clret = clGetDeviceInfo(device_id, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(fpconfig), &fpconfig, NULL);assure(!clret);

        if(fpconfig == 0){
            printf("Error: No double precision support.\n");
#ifdef DEBUG
#warning No double precision support
//            return -1;
#endif
        }
        
        size_t retBytesCnt = 0;
        
        printf("[D-%d] device started\n",deviceWorkerNum);

        cl_ulong deviceMemSize = 0;
        cl_ulong deviceMemAlloced = 0;
        clret = clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(deviceMemSize), &deviceMemSize, &retBytesCnt);assure(!clret);
        printf("[D-%d] total deviceMemSize      =%llu\n",deviceWorkerNum,deviceMemSize);

        cl_ulong deviceWorkgroupSize = 0;
        clret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(deviceWorkgroupSize), &deviceWorkgroupSize, &retBytesCnt);assure(!clret);
        printf("[D-%d] deviceWorkgroupSize      =%llu\n",deviceWorkerNum,deviceWorkgroupSize);

        deviceMemAlloced += 256*2 + 256*4*4; //SBOX, INV_SBOX, T-Table
        
        // Create a command queue
        cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &clret);assure(!clret);
        uint8_t zero = 0;

        
        //BEGIN ALLOC DEVICE MEMORY
        
        /*
        std::vector<combtrace> groupsMean{0x100};
        for (auto &cmbtr : groupsMean){
            cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
            cmbtr.cnt = 0;
            memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
        }
         
         std::vector<combtrace> groupsCenSum{0x100};
         for (auto &cmbtr : groupsCenSum){
             cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
             cmbtr.cnt = 0;
             memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
         }
        */

        cl_mem groupsMean = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, groupsMean, &zero, 1, 0, 0x100 * point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * point_per_trace * sizeof(gpu_float_type);

                
        cl_mem groupsCensum = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, groupsCensum, &zero, 1, 0, 0x100 * point_per_trace * sizeof(gpu_float_type), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * point_per_trace * sizeof(gpu_float_type);


        /*
         cl_ulong groupsMeanCnt[0x100] = {};
         cl_ulong groupsCensumCnt[0x100] = {};
         */
        cl_mem groupsMeanCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, groupsMeanCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);

        cl_mem groupsCensumCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, groupsCensumCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);

        /*
        std::vector<combtrace> curgroupsMean{0x100};
        for (auto &cmbtr : curgroupsMean){
            cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
            cmbtr.cnt = 0;
            memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
        }
         
         std::vector<combtrace> curgroupsCenSum{0x100};
         for (auto &cmbtr : curgroupsCenSum){
             cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
             cmbtr.cnt = 0;
             memset(cmbtr.trace, 0, cmbtr.cnt*sizeof(gpu_float_type));
         }
        */

        cl_mem curgroupsMean = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * point_per_trace * sizeof(gpu_float_type);

        cl_mem curgroupsCenSum = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * point_per_trace * sizeof(gpu_float_type), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * point_per_trace * sizeof(gpu_float_type);


        /*
         cl_ulong curgroupsMeanCnt[0x100] = {};
         cl_ulong curgroupsCenSumCnt[0x100] = {};
         */
        cl_mem curgroupsMeanCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);

        cl_mem curgroupsCenSumCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);
        //END ALLOC WORKER MEMORY
        
        clret = clFinish(command_queue);assure(!clret);

        
        vector<future<void>> transferWorker;
        size_t freeMemForTrace = deviceMemSize-deviceMemAlloced;
        printf("[D-%d] deviceMemAlloced         =%llu\n",deviceWorkerNum,deviceMemAlloced);
        printf("[D-%d] available deviceMemSize  =%lu\n",deviceWorkerNum,freeMemForTrace);

        size_t transferThreadsCnt = freeMemForTrace/tracefileSize;
                    
        if (transferThreadsCnt > 1)
            transferThreadsCnt--; //lets not push memory to the limits
        
        int cpusCnt = (int)sysconf(_SC_NPROCESSORS_ONLN);
        printf("CPUs=%d\n",cpusCnt);
        
        if (transferThreadsCnt > cpusCnt)
            transferThreadsCnt = cpusCnt; // too many threads aren't good either

        
        printf("[D-%d] transferThreadsCnt       =%lu\n",deviceWorkerNum,transferThreadsCnt);
        printf("[D-%d] free mem after full stall=%lu\n",deviceWorkerNum,freeMemForTrace - transferThreadsCnt*tracefileSize);

        if (transferThreadsCnt == 0) {
            printf("ERROR: device doesn't have enough memory to hold a single tracefile!\n");
            return -1;
        }
        
        mutex tranferlock;
        cl_event prevWaitEvent = {};
        
        auto cputransferWorker = [&](int deviceWorkerNum, int transferWorkerNum)->void{
            
            // BEGIN WORKER CREATE KERNELS
            cl_kernel kernel_computeMeans = clCreateKernel(program, "computeMeans", &clret);assure(!clret);
            cl_kernel kernel_computeCensums = clCreateKernel(program, "computeCenteredSums", &clret);assure(!clret);

            cl_kernel kernel_combineMeansAndCenteredSums = clCreateKernel(program, "combineMeanAndCenteredSums", &clret);assure(!clret);
            cl_kernel kernel_mergeCoutners = clCreateKernel(program, "mergeCoutners", &clret);assure(!clret);
            // END WORKER CREATE KERNELS

            
            // BEGIN WORKER SET KERNEL PARAMS

            //trace 0 param is set later
            clret = clSetKernelArg(kernel_computeMeans, 1, sizeof(cl_mem), &curgroupsMean);assure(!clret);
            clret = clSetKernelArg(kernel_computeMeans, 2, sizeof(cl_mem), &curgroupsMeanCnt);assure(!clret);
            
            //trace 0 param is set later
            clret = clSetKernelArg(kernel_computeCensums, 1, sizeof(cl_mem), &curgroupsMean);assure(!clret);
            clret = clSetKernelArg(kernel_computeCensums, 2, sizeof(cl_mem), &curgroupsMeanCnt);assure(!clret);
            clret = clSetKernelArg(kernel_computeCensums, 3, sizeof(cl_mem), &curgroupsCenSum);assure(!clret);
            clret = clSetKernelArg(kernel_computeCensums, 4, sizeof(cl_mem), &curgroupsCenSumCnt);assure(!clret);

            
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 0, sizeof(cl_mem), &groupsCensum);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 1, sizeof(cl_mem), &groupsCensumCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 2, sizeof(cl_mem), &curgroupsCenSum);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 3, sizeof(cl_mem), &curgroupsCenSumCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 4, sizeof(cl_mem), &groupsMean);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 5, sizeof(cl_mem), &groupsMeanCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 6, sizeof(cl_mem), &curgroupsMean);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 7, sizeof(cl_mem), &curgroupsMeanCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeansAndCenteredSums, 8, sizeof(cl_uint), &point_per_trace);assure(!clret);

            
            clret = clSetKernelArg(kernel_mergeCoutners, 0, sizeof(cl_mem), &groupsCensumCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutners, 1, sizeof(cl_mem), &curgroupsCenSumCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutners, 2, sizeof(cl_mem), &groupsMeanCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutners, 3, sizeof(cl_mem), &curgroupsMeanCnt);assure(!clret);
            // END WORKER SET KERNEL PARAMS
            
            cl_event myPrevWaitEvent = {};
            cl_mem myPrevClTrace = {};
            
            auto myWaitFunc = [&]()->void{
                if (myPrevWaitEvent){
                    //block thread until done
                    clret = clWaitForEvents(1, &myPrevWaitEvent); assure(!clret);
                    clret = clReleaseEvent(myPrevWaitEvent); assure(!clret);
                    myPrevWaitEvent = NULL;
                }
                if (myPrevClTrace) {
                    //release tracefile memory on device
                   clret = clSetKernelArg(kernel_computeMeans, 0, sizeof(cl_mem), &curgroupsMean);assure(!clret); //drop reference on cl_trace
                   clret = clSetKernelArg(kernel_computeCensums, 0, sizeof(cl_mem), &curgroupsMean);assure(!clret); //drop reference on cl_trace
                   clret = clReleaseMemObject(myPrevClTrace); assure(!clret);

                   myPrevClTrace = NULL;
                }
            };
            
            while (true) {
                printf("[D-%d][T-%d] getting tracesfile...\n",deviceWorkerNum,transferWorkerNum);
                listLock.lock();
                size_t todoCnt = toNormalizeList.size();
                if (toNormalizeList.size()) {
                    string tpath = toNormalizeList.back();
                    toNormalizeList.pop_back();
                    listLock.unlock();
                    //START WORK
                    printf("[D-%d][T-%02d] [ %3lu of %3lu] working on trace=%s\n",deviceWorkerNum,transferWorkerNum,(filesCnt+1-todoCnt),filesCnt,tpath.c_str());
                    
                    cl_mem cl_trace = {};
                    
                    {
                        Traces curtrace(tpath.c_str());
                        myWaitFunc(); //wait until my prev trace is out of queue and clean it
                        cl_trace = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, curtrace.bufSize(), (void*)curtrace.buf(), &clret);assure(!clret);
                    }
                    
                    
                    // BEGIN WORKER UPDATE KERNEL PARAMS
                    clret = clSetKernelArg(kernel_computeMeans, 0, sizeof(cl_mem), &cl_trace);assure(!clret);
                    clret = clSetKernelArg(kernel_computeCensums, 0, sizeof(cl_mem), &cl_trace);assure(!clret);
                    // END WORKER UPDATE KERNEL PARAMS

                    size_t global_work_size[2] = {};
                    global_work_size[0] = deviceWorkgroupSize;
                    global_work_size[1] = (point_per_trace / deviceWorkgroupSize)+1;

                    size_t local_work_size[2] = {};
                    local_work_size[0] = deviceWorkgroupSize;
                    local_work_size[1] = 1;


                    //clear memory
                    cl_event clearMemoryEvent[4] = {};
                    
                    tranferlock.lock();
                    
                    if (prevWaitEvent) {
                        clret = clEnqueueFillBuffer(command_queue, curgroupsMean, &zero, 1, 0, 0x100 * point_per_trace * sizeof(gpu_float_type), 1, &prevWaitEvent, &clearMemoryEvent[0]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curgroupsCenSum, &zero, 1, 0, 0x100 * point_per_trace * sizeof(gpu_float_type), 1, &prevWaitEvent, &clearMemoryEvent[1]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curgroupsMeanCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 1, &prevWaitEvent, &clearMemoryEvent[2]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curgroupsCenSumCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 1, &prevWaitEvent, &clearMemoryEvent[3]);assure(!clret);
                        clret = clReleaseEvent(prevWaitEvent);assure(!clret); prevWaitEvent = NULL;
                    }else{
                        clret = clEnqueueFillBuffer(command_queue, curgroupsMean, &zero, 1, 0, 0x100 * point_per_trace * sizeof(gpu_float_type), 0, NULL, &clearMemoryEvent[0]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curgroupsCenSum, &zero, 1, 0, 0x100 * point_per_trace * sizeof(gpu_float_type), 0, NULL, &clearMemoryEvent[1]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curgroupsMeanCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[2]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curgroupsCenSumCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[3]);assure(!clret);
                    }
     
                    //compute mean
                    cl_event computeMeanEvent = 0;
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_computeMeans, 2, NULL, global_work_size, local_work_size, 4, clearMemoryEvent, &computeMeanEvent);assure(!clret);
                    for (int i = 0; i<4; i++) {
                        clret = clReleaseEvent(clearMemoryEvent[i]);assure(!clret);clearMemoryEvent[i] = NULL;
                    }

#ifdef DEBUG
                    #warning without this wait sync the program aborts
                    clret = clWaitForEvents(1, &computeMeanEvent); assure(!clret);
#endif
                    
                    //compute censum
                    cl_event computeCensumEvent = 0;
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_computeCensums, 2, NULL, global_work_size, local_work_size, 1, &computeMeanEvent, &computeCensumEvent);assure(!clret);
                    clret = clReleaseEvent(computeMeanEvent);assure(!clret); computeMeanEvent = NULL;

                    
                    //combine mean and censum
                    cl_event combineMeanAndCensumEvent = 0;
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_combineMeansAndCenteredSums, 2, NULL, global_work_size, local_work_size, 1, &computeCensumEvent, &combineMeanAndCensumEvent);assure(!clret);
                    clret = clReleaseEvent(computeCensumEvent);assure(!clret); computeCensumEvent = NULL;

                    
                    //combine mean and censum
                    size_t merge_work_size = 0x100;
                    cl_event mergeCountersEvent = 0;
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_mergeCoutners, 1, NULL, &merge_work_size, &merge_work_size, 1, &combineMeanAndCensumEvent, &mergeCountersEvent);assure(!clret);
                    clret = clReleaseEvent(combineMeanAndCensumEvent);assure(!clret); combineMeanAndCensumEvent = NULL;
                    
                    //share last event
                    clret = clRetainEvent(mergeCountersEvent); assure(!clret);
                    prevWaitEvent = mergeCountersEvent;
                    
                    //remember my own last event
                    myPrevWaitEvent = mergeCountersEvent;
                    myPrevClTrace = cl_trace;
                    tranferlock.unlock();
                    
                    //END WORK
                }else{
                    listLock.unlock();
                    printf("[D-%d][T-%02d] no more traces available\n",deviceWorkerNum,transferWorkerNum);
                    break;
                }
            }
            
            myWaitFunc(); //wait until my prev trace is out of queue and clean it

            
            //BEGIN TRANFER WORKER RELEASE MEMORY
            clret = clReleaseKernel(kernel_computeMeans);assure(!clret);
            clret = clReleaseKernel(kernel_computeCensums);assure(!clret);
            clret = clReleaseKernel(kernel_combineMeansAndCenteredSums);assure(!clret);
            //END TRANFER WORKER RELEASE MEMORY
        };
        
        printf("[D-%d] spinning up %lu cpu-transfer workers\n",deviceWorkerNum,transferThreadsCnt);

        for (int i=0; i<transferThreadsCnt; i++){
            transferWorker.push_back(std::async(std::launch::async,cputransferWorker,deviceWorkerNum,i));
        }
        
        printf("[D-%d] waiting for cpu-transfer workers to finish...\n",deviceWorkerNum);
        for (auto &w : transferWorker){
            w.wait();
        }
        printf("[D-%d] all cpu-transfer workers finished!\n",deviceWorkerNum);
        if (prevWaitEvent) {
            clret = clReleaseEvent(prevWaitEvent);assure(!clret); prevWaitEvent = NULL;
        }
        
        //BEGIN TRANSFER DEVICE TO HOST
        
        retgroupsMean.clear();
        retgroupsCensum.clear();

        for (int i=0; i<0x100; i++) {
            retgroupsMean.push_back({0,0});
            retgroupsCensum.push_back({0,0});
        }
        
        
        //alloc memory
        for (auto &cmbtr : retgroupsMean){
            cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
            cmbtr.cnt = 0;
            memset(cmbtr.trace, 0, point_per_trace*sizeof(gpu_float_type));
        }
        for (auto &cmbtr : retgroupsCensum){
            cmbtr.trace = (gpu_float_type*)malloc(point_per_trace*sizeof(gpu_float_type));
            cmbtr.cnt = 0;
            memset(cmbtr.trace, 0, point_per_trace*sizeof(gpu_float_type));
        }
        
        for (int i=0; i<0x100; i++) {
            clret = clEnqueueReadBuffer(command_queue, groupsMean, false, i*point_per_trace * sizeof(gpu_float_type), point_per_trace * sizeof(gpu_float_type), retgroupsMean.at(i).trace, 0, NULL, NULL); assure(!clret);
            clret = clEnqueueReadBuffer(command_queue, groupsMeanCnt, false, i * sizeof(cl_ulong), sizeof(cl_ulong), &retgroupsMean.at(i).cnt, 0, NULL, NULL); assure(!clret);

            clret = clEnqueueReadBuffer(command_queue, groupsCensum, false, i*point_per_trace * sizeof(gpu_float_type), point_per_trace * sizeof(gpu_float_type), retgroupsCensum.at(i).trace, 0, NULL, NULL); assure(!clret);
            clret = clEnqueueReadBuffer(command_queue, groupsCensumCnt, false, i * sizeof(cl_ulong), sizeof(cl_ulong), &retgroupsCensum.at(i).cnt, 0, NULL, NULL); assure(!clret);
        }
        
        clret = clFinish(command_queue); assure(!clret);
        
        //END TRANSFER DEVICE TO HOST

        
        
        //BEGIN FREE DEVICE MEMORY
        clret = clReleaseMemObject(groupsMean);assure(!clret);
        clret = clReleaseMemObject(groupsCensum);assure(!clret);
        clret = clReleaseMemObject(groupsMeanCnt);assure(!clret);
        clret = clReleaseMemObject(groupsCensumCnt);assure(!clret);
        clret = clReleaseMemObject(curgroupsMean);assure(!clret);
        clret = clReleaseMemObject(curgroupsCenSum);assure(!clret);
        clret = clReleaseMemObject(curgroupsMeanCnt);assure(!clret);
        clret = clReleaseMemObject(curgroupsCenSumCnt);assure(!clret);

        clret = clReleaseCommandQueue(command_queue);assure(!clret);
        clret = clReleaseProgram(program);assure(!clret);
        clret = clReleaseContext(context); assure(!clret);
        //END FREE DEVICE MEMORY
        
        
        //BEGIN DEVICE WORKER
        return 0;
    };
    

    
    // Get platform and device information
    cl_int clret = 0;

    cl_platform_id platform_id = NULL;
    cl_uint num_platforms = 0;
    clret = clGetPlatformIDs(1, &platform_id, &num_platforms);assure(!clret);
    cl_uint num_devices = 0;
    cl_device_id device_id = NULL;
    clret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &num_devices);assure(!clret);

    
    std::vector<combtrace> groupsMean{0x100};
    std::vector<combtrace> groupsCenSum{0x100};
    
    assure(!deviceWorker(device_id,0, groupsMean, groupsCenSum));
        
    printf("computing SNRs\n");
    gpu_float_type meanOfMeans[point_per_trace];
    memset(meanOfMeans, 0, sizeof(gpu_float_type)*point_per_trace);

    gpu_float_type meanOfVariances[point_per_trace];
    memset(meanOfVariances, 0, sizeof(gpu_float_type)*point_per_trace);

    gpu_float_type variancesOfMeans[point_per_trace];
    memset(variancesOfMeans, 0, sizeof(gpu_float_type)*point_per_trace);
    
    computeMeanOfMeans(groupsMean, meanOfMeans, point_per_trace);
    computeVariancesOfMeans(groupsMean, meanOfMeans, variancesOfMeans, point_per_trace);
    computeMeanOfVariancesFromCensums(groupsCenSum, meanOfVariances, point_per_trace);
       
    
    printf("generating file=%s\n",outfile);
    auto snrTrace = myfile(outfile,point_per_trace*sizeof(gpu_float_type));
    memset(&snrTrace[0], 0, point_per_trace*sizeof(gpu_float_type));

    for (uint32_t i=0; i<point_per_trace; i++) {
        gpu_float_type tmp = variancesOfMeans[i] / meanOfVariances[i];
        snrTrace[i] = tmp;
    }
    

    printf("done!\n");
    return 0;
}

int r_main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outfile> {maxTraces}\n",argv[0]);
        return -1;
    }

    int maxTraces = 0;
    
    const char *indir = argv[1];
    const char *outfile = argv[2];
        
    if (argc > 3) {
        maxTraces = atoi(argv[3]);
    }
    
    return doSNR(indir, outfile, maxTraces);
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

