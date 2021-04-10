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
#include "CLTraces.hpp"
#include <string.h>
#include <iostream>
#include <future>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <exception>
#include <stdlib.h>
#include "numerics.hpp"
#include "helpers.hpp"

#define assure_(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); exit(1);}}while (0)
#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d err=%d\n",__LINE__,clret); exit(1);}}while (0)

#define MIN(a,b) ((a)<(b) ? (a) : (b))


#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#define debug(a ...) printf(a)
#else
#define dbg_assure(cond) //
#define debug(a ...) //
#endif


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
    cl_double *_mem;
    size_t _size;
public:
    myfile(const char *fname, size_t size) : _fd(-1),_size(size){
        assure_((_fd = open(fname, O_RDWR | O_CREAT, 0644))>0);
        lseek(_fd, _size-1, SEEK_SET);
        write(_fd, "", 1);
        assure_(_mem = (cl_double*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
    }
    ~myfile(){
        munmap(_mem, _size);
        if (_fd > 0) close(_fd);
    }
    cl_double &operator[](int at){
        return _mem[at];
    }
};

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int doAttack(const char *indir, const char *outfile, int maxTraces){
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
        printf("reading one trace for getting filesize...\n");
        Traces tr(toNormalizeList.at(0).c_str());
        point_per_trace = tr.pointsPerTrace();
        tracefileSize = tr.bufSize();
    }
    printf("point_per_trace=%u\n",point_per_trace);
    size_t filesCnt = toNormalizeList.size();

    printf("\n");
    

    //BEGIN DEVICE WORKER
    auto deviceWorker = [point_per_trace,tracefileSize, &listLock, &toNormalizeList, &filesCnt](cl_device_id device_id, int deviceWorkerNum, numberedTrace *gMean, numberedTrace *gCensum, numberedHypot *gMeanHypot, numberedHypot *gCensumHypot, numberedTrace *gUpperPart)->int{
        cl_int clret = 0;

        cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &clret);assure(!clret);
        
        // Create a program from the kernel source
        cl_program program = NULL;
        {
            //hack to use mmap inside the Trace class
#ifdef DEBUG
            Traces kernelfile("../../../attack_opencl/kernels.cl");
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
            free(log);
            assure(0);
        }
        assure(!clret);
        
        cl_device_fp_config fpconfig = 0;
        clret = clGetDeviceInfo(device_id, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(fpconfig), &fpconfig, NULL);assure(!clret);

        if(fpconfig == 0){
            printf("Error: No cl_double precision support.\n");
#ifndef DEBUG
            return -1;
#endif
        }
        
        size_t retBytesCnt = 0;
        
        printf("[D-%d] device started\n",deviceWorkerNum);

        cl_ulong deviceMemSize = 0;
        cl_ulong deviceMemAlloced = 0;
        clret = clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(deviceMemSize), &deviceMemSize, &retBytesCnt);assure(!clret);
        printf("[D-%d] total deviceMemSize      =%16llu\n",deviceWorkerNum,deviceMemSize);

        cl_ulong deviceWorkgroupSize = 0;
        clret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(deviceWorkgroupSize), &deviceWorkgroupSize, &retBytesCnt);assure(!clret);

        cl_uint deviceAddressBits = 0;
        clret = clGetDeviceInfo(device_id, CL_DEVICE_ADDRESS_BITS, sizeof(deviceAddressBits), &deviceAddressBits, &retBytesCnt);assure(!clret);
        
        deviceMemAlloced += 256*2;//SBOX, INV_SBOX
        
        // Create a command queue
        cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &clret);assure(!clret);
        uint8_t zero = 0;

        
        //BEGIN ALLOC DEVICE MEMORY
        
        /* --- BEGIN DeviceGlobalMemory --- */
        /*
         cl_double gmean[point_per_trace];
         cl_double gcensum[point_per_trace];
        */

        cl_mem gDeviceMeanTrace = clCreateBuffer(context, CL_MEM_READ_WRITE, point_per_trace * sizeof(cl_double), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceMeanTrace, &zero, 1, 0, point_per_trace * sizeof(cl_double), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += point_per_trace * sizeof(cl_double);

                
        cl_mem gDeviceCensumTrace = clCreateBuffer(context, CL_MEM_READ_WRITE, point_per_trace * sizeof(cl_double), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceCensumTrace, &zero, 1, 0, point_per_trace * sizeof(cl_double), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += point_per_trace * sizeof(cl_double);

        /*
         uint64_t gcntMean = 0;
         uint64_t gcntcensum = 0;
         */
        cl_mem gDeviceMeanTraceCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceMeanTraceCnt, &zero, 1, 0, sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += sizeof(cl_ulong);

        cl_mem gDeviceCensumTraceCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceCensumTraceCnt, &zero, 1, 0, sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += sizeof(cl_ulong);

        /*
        cl_double ghypotMeanGuessVals[0x100] = {};
        cl_double ghypotCensumGuessVals[0x100] = {};
        */

        cl_mem gDeviceHypotMeans = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_double), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceHypotMeans, &zero, 1, 0, 0x100 * sizeof(cl_double), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_double);

        cl_mem gDeviceHypotCensums = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_double), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceHypotCensums, &zero, 1, 0, 0x100 * sizeof(cl_double), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_double);


        /*
         uint64_t ghypotMeanGuessCnt[0x100] = {};
         uint64_t ghypotCensumGuessCnt[0x100] = {};
         */
        cl_mem gDeviceHypotMeansCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceHypotMeansCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);

        cl_mem gDeviceHypotCensumsCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceHypotCensumsCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);
        
        /*
         numberedTrace *gupperPart = (numberedTrace *)malloc(0x100*sizeof(numberedTrace));
         for (int k = 0; k<0x100; k++) {
             gupperPart[k].trace = (cl_double*)malloc(point_per_trace * sizeof(cl_double));
             memset(gupperPart[k].trace, 0, point_per_trace * sizeof(cl_double));
         }
         */
        cl_mem gDeviceUpperPart = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * point_per_trace * sizeof(cl_double), NULL, &clret);assure(!clret);
        clret = clEnqueueFillBuffer(command_queue, gDeviceUpperPart, &zero, 1, 0, 0x100 * point_per_trace * sizeof(cl_double), 0, NULL, NULL);assure(!clret);
        deviceMemAlloced += 0x100 * point_per_trace * sizeof(cl_double);


        /* --- END DeviceGlobalMemory --- */

        /* --- BEGIN Device Trace Memory --- */

        
        /*
         cl_double curmean[point_per_trace];
         cl_double curcensum[point_per_trace];
        */

        cl_mem curMeanTrace = clCreateBuffer(context, CL_MEM_READ_WRITE, point_per_trace * sizeof(cl_double), NULL, &clret);assure(!clret);
        deviceMemAlloced += point_per_trace * sizeof(cl_double);

                
        cl_mem curCensumTrace = clCreateBuffer(context, CL_MEM_READ_WRITE, point_per_trace * sizeof(cl_double), NULL, &clret);assure(!clret);
        deviceMemAlloced += point_per_trace * sizeof(cl_double);

        /*
         uint64_t curcntMean = 0;
         uint64_t curcntcensum = 0;
         */
        cl_mem curMeanTraceCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
        deviceMemAlloced += sizeof(cl_ulong);

        cl_mem curCensumTraceCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &clret);assure(!clret);
        deviceMemAlloced += sizeof(cl_ulong);

        /*
        cl_double curhypotMeanGuessVals[0x100] = {};
        cl_double curhypotCensumGuessVals[0x100] = {};
        */

        cl_mem curHypotMeans = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_double), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_double);

        cl_mem curHypotCensums = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_double), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_double);


        /*
         uint64_t curhypotMeanGuessCnt[0x100] = {};
         uint64_t curhypotCensumGuessCnt[0x100] = {};
         */
        cl_mem curHypotMeansCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);

        cl_mem curHypotCensumsCnt = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * sizeof(cl_ulong), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * sizeof(cl_ulong);
        
        /*
         numberedTrace curgupperPart = (numberedTrace *)malloc(0x100*sizeof(numberedTrace));
         for (int k = 0; k<0x100; k++) {
             curgupperPart[k].cnt = 0;
             curgupperPart[k].trace = (cl_double*)malloc(point_per_trace * sizeof(cl_double));
             memset(curgupperPart[k].trace, 0, point_per_trace * sizeof(cl_double));
         }
         */
        cl_mem curUpperPart = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x100 * point_per_trace * sizeof(cl_double), NULL, &clret);assure(!clret);
        deviceMemAlloced += 0x100 * point_per_trace * sizeof(cl_double);
        
        /* --- END Device Trace Memory --- */


        //END ALLOC WORKER MEMORY
        
        clret = clFinish(command_queue);assure(!clret);

        
        vector<future<void>> transferWorker;
        size_t freeMemForTrace = deviceMemSize-deviceMemAlloced;
        printf("[D-%d] deviceMemAlloced         =%16llu\n",deviceWorkerNum,deviceMemAlloced);
        printf("[D-%d] available deviceMemSize  =%16lu\n",deviceWorkerNum,freeMemForTrace);

        size_t transferThreadsCnt = freeMemForTrace/tracefileSize;

        if (transferThreadsCnt > 1)
            transferThreadsCnt--; //lets not push memory to the limits
        
        
        uint64_t cpuloadersCnt = sysconf(_SC_NPROCESSORS_ONLN);
        if (transferThreadsCnt > cpuloadersCnt) {
            transferThreadsCnt = cpuloadersCnt;
        }

//#define LIMIT 10
//        if (transferThreadsCnt > LIMIT)
//            transferThreadsCnt = LIMIT; 
            
        
        printf("[D-%d] transferThreadsCnt       =%16lu\n",deviceWorkerNum,transferThreadsCnt);
        printf("[D-%d] free mem after full stall=%16lu\n",deviceWorkerNum,freeMemForTrace - transferThreadsCnt*tracefileSize);

        if (transferThreadsCnt == 0) {
            printf("ERROR: device doesn't have enough memory to hold a single tracefile!\n");
            return -1;
        }
        
        mutex tranferlock;
        cl_event prevWaitEvents[3] = {};
        int prevWaitEventsCnt = sizeof(prevWaitEvents)/sizeof(*prevWaitEvents);
        
        auto cputransferWorker = [&](int deviceWorkerNum, int transferWorkerNum)->void{
            
            // BEGIN WORKER CREATE KERNELS
            cl_kernel kernel_computeMean = clCreateKernel(program, "computeMean", &clret);assure(!clret);
            cl_kernel kernel_computeMeanHypot = clCreateKernel(program, "computeMeanHypot", &clret);assure(!clret);

            cl_kernel kernel_computeCenteredSum = clCreateKernel(program, "computeCenteredSum", &clret);assure(!clret);
            cl_kernel kernel_computeUpper = clCreateKernel(program, "computeUpper", &clret);assure(!clret);
            cl_kernel kernel_computeCenteredSumHypot = clCreateKernel(program, "computeCenteredSumHypot", &clret);assure(!clret);

            cl_kernel kernel_combineMeanAndCenteredSum = clCreateKernel(program, "combineMeanAndCenteredSum", &clret);assure(!clret);
            cl_kernel kernel_mergeCoutnersReal = clCreateKernel(program, "mergeCoutners", &clret);assure(!clret);

            cl_kernel kernel_combineMeanAndCenteredSumHypot = clCreateKernel(program, "combineMeanAndCenteredSum", &clret);assure(!clret);
            cl_kernel kernel_mergeCoutnersHypot = clCreateKernel(program, "mergeCoutners", &clret);assure(!clret);

            cl_kernel kernel_combineUpper = clCreateKernel(program, "kernelAddTrace", &clret);assure(!clret);
            // END WORKER CREATE KERNELS

            
            // BEGIN WORKER SET KERNEL PARAMS

            //trace 0 param is set later
            clret = clSetKernelArg(kernel_computeMean, 1, sizeof(cl_mem), &curMeanTrace);assure(!clret);
            clret = clSetKernelArg(kernel_computeMean, 2, sizeof(cl_mem), &curMeanTraceCnt);assure(!clret);

            //trace 0 param is set later
            clret = clSetKernelArg(kernel_computeMeanHypot, 1, sizeof(cl_mem), &curHypotMeans);assure(!clret);
            clret = clSetKernelArg(kernel_computeMeanHypot, 2, sizeof(cl_mem), &curHypotMeansCnt);assure(!clret);

            //trace 0 param is set later
            clret = clSetKernelArg(kernel_computeCenteredSum, 1, sizeof(cl_mem), &curMeanTrace);assure(!clret);
            clret = clSetKernelArg(kernel_computeCenteredSum, 2, sizeof(cl_mem), &curHypotMeans);assure(!clret);
            clret = clSetKernelArg(kernel_computeCenteredSum, 3, sizeof(cl_mem), &curCensumTrace);assure(!clret);
            clret = clSetKernelArg(kernel_computeCenteredSum, 4, sizeof(cl_mem), &curCensumTraceCnt);assure(!clret);

            //trace 0 param is set later
            clret = clSetKernelArg(kernel_computeUpper, 1, sizeof(cl_mem), &curMeanTrace);assure(!clret);
            clret = clSetKernelArg(kernel_computeUpper, 2, sizeof(cl_mem), &curHypotMeans);assure(!clret);
            clret = clSetKernelArg(kernel_computeUpper, 3, sizeof(cl_mem), &curUpperPart);assure(!clret);

            //trace 0 param is set later
            clret = clSetKernelArg(kernel_computeCenteredSumHypot, 1, sizeof(cl_mem), &curHypotMeans);assure(!clret);
            clret = clSetKernelArg(kernel_computeCenteredSumHypot, 2, sizeof(cl_mem), &curHypotCensums);assure(!clret);
            clret = clSetKernelArg(kernel_computeCenteredSumHypot, 3, sizeof(cl_mem), &curHypotCensumsCnt);assure(!clret);

            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 0, sizeof(cl_mem), &gDeviceCensumTrace);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 1, sizeof(cl_mem), &gDeviceCensumTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 2, sizeof(cl_mem), &curCensumTrace);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 3, sizeof(cl_mem), &curCensumTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 4, sizeof(cl_mem), &gDeviceMeanTrace);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 5, sizeof(cl_mem), &gDeviceMeanTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 6, sizeof(cl_mem), &curMeanTrace);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 7, sizeof(cl_mem), &curMeanTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSum, 8, sizeof(cl_uint), &point_per_trace);assure(!clret);

            cl_uint one = 1;
            clret = clSetKernelArg(kernel_mergeCoutnersReal, 0, sizeof(cl_mem), &gDeviceCensumTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersReal, 1, sizeof(cl_mem), &curCensumTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersReal, 2, sizeof(cl_mem), &gDeviceMeanTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersReal, 3, sizeof(cl_mem), &curMeanTraceCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersReal, 4, sizeof(cl_uint), &one);assure(!clret);

            cl_uint val256 = 256;
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 0, sizeof(cl_mem), &gDeviceHypotCensums);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 1, sizeof(cl_mem), &gDeviceHypotCensumsCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 2, sizeof(cl_mem), &curHypotCensums);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 3, sizeof(cl_mem), &curHypotCensumsCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 4, sizeof(cl_mem), &gDeviceHypotMeans);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 5, sizeof(cl_mem), &gDeviceHypotMeansCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 6, sizeof(cl_mem), &curHypotMeans);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 7, sizeof(cl_mem), &curHypotMeansCnt);assure(!clret);
            clret = clSetKernelArg(kernel_combineMeanAndCenteredSumHypot, 8, sizeof(cl_uint), &val256);assure(!clret);

            clret = clSetKernelArg(kernel_mergeCoutnersHypot, 0, sizeof(cl_mem), &gDeviceHypotCensumsCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersHypot, 1, sizeof(cl_mem), &curHypotCensumsCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersHypot, 2, sizeof(cl_mem), &gDeviceHypotMeansCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersHypot, 3, sizeof(cl_mem), &curHypotMeansCnt);assure(!clret);
            clret = clSetKernelArg(kernel_mergeCoutnersHypot, 4, sizeof(cl_uint), &val256);assure(!clret);

            
            cl_uint valUpperTotal = 0x100 * point_per_trace;
            clret = clSetKernelArg(kernel_combineUpper, 0, sizeof(cl_mem), &gDeviceUpperPart);assure(!clret);
            clret = clSetKernelArg(kernel_combineUpper, 1, sizeof(cl_mem), &curUpperPart);assure(!clret);
            clret = clSetKernelArg(kernel_combineUpper, 2, sizeof(cl_uint), &valUpperTotal);assure(!clret);
            
            // END WORKER SET KERNEL PARAMS
            
            size_t local_work_size[2] = {};
            local_work_size[0] = deviceWorkgroupSize;
            local_work_size[1] = 1;

            size_t global_work_size_trace[2] = {};
            global_work_size_trace[0] = deviceWorkgroupSize;
            global_work_size_trace[1] = (point_per_trace / deviceWorkgroupSize)+1;

            size_t global_work_size_key[2] = {};
            global_work_size_key[0] = deviceWorkgroupSize;
            global_work_size_key[1] = (0x100 / deviceWorkgroupSize);
            if (0x100 % deviceWorkgroupSize != 0) global_work_size_key[1] ++;
                        
            size_t global_work_size_upper[2] = {};
            global_work_size_upper[0] = deviceWorkgroupSize;
            global_work_size_upper[1] = ((point_per_trace*0x100) / deviceWorkgroupSize);
            if ((point_per_trace*0x100) % deviceWorkgroupSize != 0) global_work_size_upper[1] ++;

            cl_mem cl_trace = NULL;
            size_t cl_traceSize = 0;
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


                    {
                        cl_event copyEvent = NULL;
                        printf("[D-%d][T-%02d] loading file\n",deviceWorkerNum,transferWorkerNum);
                        CLTraces curtrace(tpath.c_str(),context,device_id);
                        if (curtrace.size() > cl_traceSize) {
                            printf("[D-%d][T-%02d] realloc required...\n",deviceWorkerNum,transferWorkerNum);
                            if (cl_trace) {
                                clret = clReleaseMemObject(cl_trace);cl_trace = NULL;cl_traceSize = 0;assure(!clret);
                            }
                            cl_trace = clCreateBuffer(context, CL_MEM_READ_ONLY, cl_traceSize = curtrace.size(), NULL, &clret);assure(!clret);
                        }
                        printf("[D-%d][T-%02d] copying file\n",deviceWorkerNum,transferWorkerNum);

                        int sleepsec = 0;
                        do {
                            clret = clEnqueueCopyBuffer(command_queue, curtrace.getHostMem(), cl_trace, 0, 0, curtrace.size(), 0, NULL, &copyEvent);assure(!clret);
                            if (sleepsec++) {
                                printf("[D-%d][T-%02d] clEnqueueCopyBuffer failed, retrying (%d)...\n",deviceWorkerNum,transferWorkerNum,sleepsec);
                                sleep(sleepsec);
                            }
                        }while ((clret = clWaitForEvents(1, &copyEvent)));
                        assure(!clret);
                        clret = clReleaseEvent(copyEvent);assure(!clret);
                        printf("[D-%d][T-%02d] done copy\n",deviceWorkerNum,transferWorkerNum);
                    }
                    
                    // BEGIN WORKER UPDATE KERNEL PARAMS
                    clret = clSetKernelArg(kernel_computeMean, 0, sizeof(cl_mem), &cl_trace);assure(!clret);
                    clret = clSetKernelArg(kernel_computeMeanHypot, 0, sizeof(cl_mem), &cl_trace);assure(!clret);

                    clret = clSetKernelArg(kernel_computeCenteredSum, 0, sizeof(cl_mem), &cl_trace);assure(!clret);
                    clret = clSetKernelArg(kernel_computeUpper, 0, sizeof(cl_mem), &cl_trace);assure(!clret);
                    clret = clSetKernelArg(kernel_computeCenteredSumHypot, 0, sizeof(cl_mem), &cl_trace);assure(!clret);
                    // END WORKER UPDATE KERNEL PARAMS


                    //clear memory
                    cl_event clearMemoryEvent[9] = {};
                    int clearMemoryEventCnt = sizeof(clearMemoryEvent)/sizeof(*clearMemoryEvent);

                                        
                    tranferlock.lock();

                    if (prevWaitEvents[0]) {
                        clret = clEnqueueFillBuffer(command_queue, curUpperPart, &zero, 1, 0, 0x100 * point_per_trace * sizeof(cl_double), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[0]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotCensumsCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[1]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotMeansCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[2]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotMeans, &zero, 1, 0, 0x100 * sizeof(cl_double), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[3]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curMeanTrace, &zero, 1, 0, point_per_trace * sizeof(cl_double), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[4]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curCensumTrace, &zero, 1, 0, point_per_trace * sizeof(cl_double), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[5]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curMeanTraceCnt, &zero, 1, 0, sizeof(cl_ulong), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[6]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curCensumTraceCnt, &zero, 1, 0, sizeof(cl_ulong), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[7]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotCensums, &zero, 1, 0, 0x100 * sizeof(cl_double), prevWaitEventsCnt, prevWaitEvents, &clearMemoryEvent[8]);assure(!clret);
                        for (int i=0; i<prevWaitEventsCnt; i++) {
                            clret = clReleaseEvent(prevWaitEvents[i]);assure(!clret); prevWaitEvents[i] = NULL;
                        }
                    }else{
                        clret = clEnqueueFillBuffer(command_queue, curUpperPart, &zero, 1, 0, 0x100 * point_per_trace * sizeof(cl_double), 0, NULL, &clearMemoryEvent[0]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotCensumsCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[1]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotMeansCnt, &zero, 1, 0, 0x100 * sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[2]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotMeans, &zero, 1, 0, 0x100 * sizeof(cl_double), 0, NULL, &clearMemoryEvent[3]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curMeanTrace, &zero, 1, 0, point_per_trace * sizeof(cl_double), 0, NULL, &clearMemoryEvent[4]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curCensumTrace, &zero, 1, 0, point_per_trace * sizeof(cl_double), 0, NULL, &clearMemoryEvent[5]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curMeanTraceCnt, &zero, 1, 0, sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[6]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curCensumTraceCnt, &zero, 1, 0, sizeof(cl_ulong), 0, NULL, &clearMemoryEvent[7]);assure(!clret);
                        clret = clEnqueueFillBuffer(command_queue, curHypotCensums, &zero, 1, 0, 0x100 * sizeof(cl_double), 0, NULL, &clearMemoryEvent[8]);assure(!clret);
                    }

                    //compute mean
                    cl_event computeMeanEvents[2] = {};

                    clret = clEnqueueNDRangeKernel(command_queue, kernel_computeMean, 2, NULL, global_work_size_trace, local_work_size, clearMemoryEventCnt, clearMemoryEvent, &computeMeanEvents[0]);assure(!clret);
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_computeMeanHypot, 2, NULL, global_work_size_key, local_work_size, clearMemoryEventCnt, clearMemoryEvent, &computeMeanEvents[1]);assure(!clret);

                    for (int i = 0; i<sizeof(clearMemoryEvent)/sizeof(*clearMemoryEvent); i++) {
                        clret = clReleaseEvent(clearMemoryEvent[i]);assure(!clret);clearMemoryEvent[i] = NULL;
                    }
                                        
                    
                    //compute censum
                    cl_event computeCensumEvents[3] = {};
                    
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_computeCenteredSum, 2, NULL, global_work_size_trace, local_work_size, 2, computeMeanEvents, &computeCensumEvents[0]);assure(!clret);
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_computeCenteredSumHypot, 2, NULL, global_work_size_key, local_work_size, 1, &computeMeanEvents[1], &computeCensumEvents[1]);assure(!clret);
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_computeUpper, 2, NULL, global_work_size_trace, local_work_size, 2, computeMeanEvents, &computeCensumEvents[2]);assure(!clret);

                    
                    clret = clReleaseEvent(computeMeanEvents[0]);assure(!clret); computeMeanEvents[0] = NULL;
                    clret = clReleaseEvent(computeMeanEvents[1]);assure(!clret); computeMeanEvents[1] = NULL;
                                      
                    constexpr int mergeEventsCnt = 3;
                    cl_event *mergeEvents = new cl_event[mergeEventsCnt];
                    static_assert(mergeEventsCnt*sizeof(*mergeEvents) == sizeof(prevWaitEvents), "prevEvents size mismatch");
                    
                    
                    //combine mean and censum hypot
                    cl_event combineMeanAndCensumHypotEvent = 0;
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_combineMeanAndCenteredSumHypot, 2, NULL, global_work_size_key, local_work_size, 1, &computeCensumEvents[1], &combineMeanAndCensumHypotEvent);assure(!clret);
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_mergeCoutnersHypot, 2, NULL, global_work_size_key, local_work_size, 1, &combineMeanAndCensumHypotEvent, &mergeEvents[0]);assure(!clret);

                    //combine mean and censum
                    cl_event combineMeanAndCensumEvent = 0;
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_combineMeanAndCenteredSum, 2, NULL, global_work_size_trace, local_work_size, 1, &computeCensumEvents[0], &combineMeanAndCensumEvent);assure(!clret);
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_mergeCoutnersReal, 2, NULL, global_work_size_key, local_work_size, 1, &combineMeanAndCensumEvent, &mergeEvents[1]);assure(!clret);

                    //combine upper
                    clret = clEnqueueNDRangeKernel(command_queue, kernel_combineUpper, 2, NULL, global_work_size_upper, local_work_size, 1, &computeCensumEvents[2], &mergeEvents[2]);assure(!clret);
                    
                    
                    clret = clReleaseEvent(computeCensumEvents[0]);assure(!clret); computeCensumEvents[0] = NULL;
                    clret = clReleaseEvent(computeCensumEvents[1]);assure(!clret); computeCensumEvents[1] = NULL;
                    clret = clReleaseEvent(computeCensumEvents[2]);assure(!clret); computeCensumEvents[2] = NULL;

                    clret = clReleaseEvent(combineMeanAndCensumEvent);assure(!clret); combineMeanAndCensumEvent = NULL;
                    clret = clReleaseEvent(combineMeanAndCensumHypotEvent);assure(!clret); combineMeanAndCensumHypotEvent = NULL;
                    
                    
                    //share last event
                    
                    for (int i=0; i<prevWaitEventsCnt; i++) {
                        clret = clRetainEvent(mergeEvents[i]);assure(!clret); //increment ref for prevWaitEvents
                        prevWaitEvents[i] = mergeEvents[i];
                    }
                    clret = clFlush(command_queue); assure(!clret);
                    tranferlock.unlock();
                    
                    printf("[D-%d][T-%02d] waiting for clWaitForEvents\n",deviceWorkerNum,transferWorkerNum);
                    //block thread until done

                    relaxedWaitForEvent(prevWaitEventsCnt, mergeEvents);
//                    clret = clWaitForEvents(prevWaitEventsCnt, mergeEvents); assure(!clret);
                    printf("[D-%d][T-%02d] done clWaitForEvents\n",deviceWorkerNum,transferWorkerNum);

                    std::thread delevents([mergeEvents,prevWaitEventsCnt,deviceWorkerNum,transferWorkerNum]{
                        cl_int clret = 0;
                        for (int i=0; i<prevWaitEventsCnt; i++) {
                            clret = clReleaseEvent(mergeEvents[i]);assure(!clret); //decrement ref for mergeEvents
                        }
                        delete [] mergeEvents;
                        printf("[D-%d][T-%02d] done all\n",deviceWorkerNum,transferWorkerNum);
                    });
                    delevents.detach();

                    //END WORK
                }else{
                    listLock.unlock();
                    printf("[D-%d][T-%02d] no more traces available\n",deviceWorkerNum,transferWorkerNum);
                    break;
                }
            }
            //release tracefile memory on device
            if (cl_trace) {
                clret = clReleaseMemObject(cl_trace);cl_trace = NULL;cl_traceSize = 0;assure(!clret);
            }


            //BEGIN TRANFER WORKER RELEASE MEMORY
            clret = clReleaseKernel(kernel_computeMean);assure(!clret);
            clret = clReleaseKernel(kernel_computeMeanHypot);assure(!clret);

            clret = clReleaseKernel(kernel_computeCenteredSum);assure(!clret);
            clret = clReleaseKernel(kernel_computeUpper);assure(!clret);
            clret = clReleaseKernel(kernel_computeCenteredSumHypot);assure(!clret);

            clret = clReleaseKernel(kernel_combineMeanAndCenteredSum);assure(!clret);
            clret = clReleaseKernel(kernel_mergeCoutnersReal);assure(!clret);

            clret = clReleaseKernel(kernel_combineMeanAndCenteredSumHypot);assure(!clret);
            clret = clReleaseKernel(kernel_mergeCoutnersHypot);assure(!clret);

            clret = clReleaseKernel(kernel_combineUpper);assure(!clret);
            //END TRANFER WORKER RELEASE MEMORY
        };
        
        printf("[D-%d] spinning up %lu cpu-transfer workers\n",deviceWorkerNum,transferThreadsCnt);

        for (int i=0; i<transferThreadsCnt; i++){
            transferWorker.push_back(std::async(std::launch::async,cputransferWorker,deviceWorkerNum,i));
            sleep(1);
        }

        printf("[D-%d] waiting for cpu-transfer workers to finish...\n",deviceWorkerNum);
        for (auto &w : transferWorker){
            w.wait();
        }
        printf("[D-%d] all cpu-transfer workers finished!\n",deviceWorkerNum);
        for (int i=0; i<prevWaitEventsCnt; i++) {
            if (prevWaitEvents[i]) {
                clret = clReleaseEvent(prevWaitEvents[i]);assure(!clret); prevWaitEvents[i] = NULL;
            }
        }
        

        //BEGIN TRANSFER DEVICE TO HOST
        
        clret = clEnqueueReadBuffer(command_queue, gDeviceMeanTrace, false, 0, point_per_trace * sizeof(cl_double), gMean->trace, 0, NULL, NULL); assure(!clret);
        clret = clEnqueueReadBuffer(command_queue, gDeviceMeanTraceCnt, false, 0, sizeof(cl_ulong), &gMean->cnt, 0, NULL, NULL); assure(!clret);

        clret = clEnqueueReadBuffer(command_queue, gDeviceCensumTrace, false, 0, point_per_trace * sizeof(cl_double), gCensum->trace, 0, NULL, NULL); assure(!clret);
        clret = clEnqueueReadBuffer(command_queue, gDeviceCensumTraceCnt, false, 0, sizeof(cl_ulong), &gCensum->cnt, 0, NULL, NULL); assure(!clret);

        for (int k=0; k<0x100; k++) {
            clret = clEnqueueReadBuffer(command_queue, gDeviceHypotMeans, false, k*sizeof(cl_double), sizeof(cl_double), &gMeanHypot[k].trace, 0, NULL, NULL); assure(!clret);
            clret = clEnqueueReadBuffer(command_queue, gDeviceHypotMeansCnt, false, k*sizeof(cl_ulong), sizeof(cl_ulong), &gMeanHypot[k].cnt, 0, NULL, NULL); assure(!clret);

            clret = clEnqueueReadBuffer(command_queue, gDeviceHypotCensums, false, k*sizeof(cl_double), sizeof(cl_double), &gCensumHypot[k].trace, 0, NULL, NULL); assure(!clret);
            clret = clEnqueueReadBuffer(command_queue, gDeviceHypotCensumsCnt, false, k*sizeof(cl_ulong), sizeof(cl_ulong), &gCensumHypot[k].cnt, 0, NULL, NULL); assure(!clret);
            
            clret = clEnqueueReadBuffer(command_queue, gDeviceUpperPart, false, k * point_per_trace * sizeof(cl_double), point_per_trace * sizeof(cl_double), gUpperPart[k].trace, 0, NULL, NULL); assure(!clret);
        }
        
        clret = clFinish(command_queue); assure(!clret);

        //END TRANSFER DEVICE TO HOST


        //BEGIN FREE DEVICE MEMORY
        clret = clReleaseMemObject(gDeviceMeanTrace);assure(!clret);
        clret = clReleaseMemObject(gDeviceMeanTraceCnt);assure(!clret);
        clret = clReleaseMemObject(gDeviceCensumTrace);assure(!clret);
        clret = clReleaseMemObject(gDeviceCensumTraceCnt);assure(!clret);
        clret = clReleaseMemObject(gDeviceHypotMeans);assure(!clret);
        clret = clReleaseMemObject(gDeviceHypotMeansCnt);assure(!clret);
        clret = clReleaseMemObject(gDeviceHypotCensums);assure(!clret);
        clret = clReleaseMemObject(gDeviceHypotCensumsCnt);assure(!clret);
        clret = clReleaseMemObject(gDeviceUpperPart);assure(!clret);
        
        clret = clReleaseMemObject(curMeanTrace);assure(!clret);
        clret = clReleaseMemObject(curMeanTraceCnt);assure(!clret);
        clret = clReleaseMemObject(curCensumTrace);assure(!clret);
        clret = clReleaseMemObject(curCensumTraceCnt);assure(!clret);
        clret = clReleaseMemObject(curHypotMeans);assure(!clret);
        clret = clReleaseMemObject(curHypotMeansCnt);assure(!clret);
        clret = clReleaseMemObject(curHypotCensums);assure(!clret);
        clret = clReleaseMemObject(curHypotCensumsCnt);assure(!clret);
        clret = clReleaseMemObject(curUpperPart);assure(!clret);

        clret = clReleaseCommandQueue(command_queue);assure(!clret);
        clret = clReleaseProgram(program);assure(!clret);
        clret = clReleaseContext(context); assure(!clret);
        //END FREE DEVICE MEMORY
        
        
        //END DEVICE WORKER
        return 0;
    };
    
    //BEGIN GLOBAL MEMORY ALLOC
    numberedTrace gMean = {};
    numberedTrace gCensum = {};

    gMean.trace = (cl_double*)malloc(point_per_trace * sizeof(cl_double));
    gCensum.trace = (cl_double*)malloc(point_per_trace * sizeof(cl_double));

    memset(gMean.trace, 0, point_per_trace * sizeof(cl_double));
    memset(gCensum.trace, 0, point_per_trace * sizeof(cl_double));


    numberedHypot gMeanHypot[0x100] = {};
    numberedHypot gCensumHypot[0x100] = {};

    
    numberedTrace *gUpperPart = (numberedTrace *)malloc(0x100*sizeof(numberedTrace));
    for (int k = 0; k<0x100; k++) {
        gUpperPart[k].cnt = 0;
        gUpperPart[k].trace = (cl_double*)malloc(point_per_trace * sizeof(cl_double));
        memset(gUpperPart[k].trace, 0, point_per_trace * sizeof(cl_double));
    }
    //END GLOBAL MEMORY ALLOC

    
    // Get platform and device information
    cl_int clret = 0;

    cl_platform_id platform_id = NULL;
    cl_uint num_platforms = 0;
    clret = clGetPlatformIDs(1, &platform_id, &num_platforms);assure(!clret);
    cl_uint num_devices = 0;
    cl_device_id device_ids[4] = {};
    clret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 4, device_ids, &num_devices);assure(!clret);
    
    assure(num_devices>3);

    
    assure(!deviceWorker(device_ids[3], 3, &gMean, &gCensum, gMeanHypot, gCensumHypot, gUpperPart));
    
    printf("all GPUs finished!\n");

    printf("Computing correlation...\n");
    
    for (int k=0; k<0x100; k++) {
        std::string filename = outfile;
        filename += '_';
        filename += to_string(k);
        printf("generating file=%s\n",filename.c_str());
        myfile out(filename.c_str(),point_per_trace*sizeof(cl_double));
        memset(&out[0], 0, point_per_trace*sizeof(cl_double));
        
        cl_double *upTrace = gUpperPart[k].trace;
        cl_double hypotCensum = gCensumHypot[k].trace;
        for (int i=0; i<point_per_trace; i++) {
            cl_double upper = upTrace[i];
            
            cl_double realCensum = gCensum.trace[i];
            
            cl_double preLower = realCensum * hypotCensum;
            
            cl_double lower = sqrt(preLower);
            
            out[i] = upper/lower;
        }
        
    }
    
    
    
    //BEGIN GLOBAL MEMORY FREE

    for (int k = 0; k<0x100; k++) {
        free(gUpperPart[k].trace);
    }
    free(gUpperPart);
    
    free(gMean.trace);
    free(gCensum.trace);
    

    //END GLOBAL MEMORY FREE
    
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
    
    return doAttack(indir, outfile, maxTraces);
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

