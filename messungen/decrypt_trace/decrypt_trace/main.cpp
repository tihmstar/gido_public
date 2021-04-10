//
//  main.cpp
//  decrypt_trace
//
//  Created by tihmstar on 10.02.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include <iostream>
#include <libirecovery.h>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <mutex>
#include "CPULoader.hpp"
#include <archive.h>
#include <archive_entry.h>


#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define safeFree(ptr) do{ if (ptr){free(ptr);ptr = NULL;} }while(0)
#define assure(cond) do {if (!(cond)) {printf("ERROR: MAIN ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

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
    
    assure(dir = opendir(dirpath.c_str()));
    
    while ((ent = readdir (dir)) != NULL) {
        if ((ent->d_type != DT_REG && ent->d_type != DT_DIR) || ent->d_name[0] == '.')
            continue;
        ret._file.push_back({dirpath + "/" + ent->d_name});
    }
    
    if (dir) closedir(dir);
    return ret;
}

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

using namespace std;

atomic<uint16_t> wantWriteCounter{0};
mutex fileWriteEvent;


std::pair<std::string, std::string> getOutFileNameAndPath(LoadTraceFile *trace, const char *outdir){
    ssize_t lastSlashPos = trace->path().rfind("/");
    std::string outFileName = trace->path().substr(lastSlashPos+1);
    
    std::string outFilePath = outdir;
    if (outFilePath.c_str()[outFilePath.size()-1] != '/') {
        outFilePath += "/";
    }
    
    outFileName = "decrypted_" + outFileName;
    
    if (outFileName.substr(outFileName.size()-sizeof(".tar.gz")+1) == ".tar.gz") {
        outFileName = outFileName.substr(0,outFileName.size()-sizeof(".tar.gz")+1);
    }
    
    outFilePath += outFileName + ".tar.gz";
    return {outFileName,outFilePath};
}

int decryptfunc(int wnum, irecv_client_t client, LoadTraceFile *trace, mutex &deviceMutex, const char *outdir){
    uint32_t numOfTraces = trace->tracesInFile();
    uint32_t curTraceNum = 0;
    
    auto outnames = getOutFileNameAndPath(trace,outdir);
    
    std::string outFileName = outnames.first;
    std::string outFilePath = outnames.second;

    printf("[W-%d] Decrypting %s\n",wnum,trace->path().c_str());

    uint8_t mesh[16] = {};
    uint64_t *meshb = (uint64_t*)mesh;

    for (auto t: *trace){
        int r = 0;
        uint8_t output[sizeof(t->Output)+10] = {};
        char outputStr[sizeof(t->Output)*2+1] = {};

        meshb[0] ^= ((uint64_t*)t->Output)[0];
        meshb[1] ^= ((uint64_t*)t->Output)[1];
        
//        const uint8_t cin[]   = { 0x2a, 0x50, 0xb8, 0x31, 0xb0, 0x2d, 0xa3, 0x10, 0xf8, 0x55, 0x6a, 0xdb, 0xec, 0xc3, 0x74, 0xda };
//        const uint8_t cout[]  = { 0x8b, 0x71, 0x83, 0x72, 0x95, 0xef, 0x72, 0xcb, 0x82, 0xd4, 0x28, 0xd1, 0x68, 0xbd, 0x5c, 0x78 };

//        const uint8_t cin[]   = { 0xb9, 0x3a, 0x66, 0x60, 0x88, 0xe1, 0xee, 0x9c, 0xc8, 0x2a, 0x80, 0x5a, 0xea, 0xdc, 0x35, 0xc6, 0x4b };
//        const uint8_t cout[]  = //99c8021b5921fa42dcdc05493b2aac2a


        deviceMutex.lock();
        if ((r = irecv_send_buffer(client, (unsigned char*)t->Input, sizeof(t->Input), 1))){
            printf("[Error] failed to send buffer\n");
            deviceMutex.unlock();
            assure(0);
        }
        if ((r = irecv_send_command(client, "decrypt"))){
            printf("[Error] failed to decrypt command\n");
            deviceMutex.unlock();
            assure(0);
        }
        if ((r = irecv_usb_control_transfer(client, 0xC0, 0, 0, 0, (unsigned char*) outputStr, sizeof(outputStr), 500)) != sizeof(outputStr)){
            printf("[Error] failed to recv decryption\n");
            deviceMutex.unlock();
            assure(0);
        }
        deviceMutex.unlock();

        for (int i=0; i<sizeof(t->Output); i++) {//scanf write 4 bytes, but output is large enough to stay within bounds
            sscanf(outputStr+i*2, "%02x", (unsigned int*)&output[i]);
        }

        for (int i=0; i<sizeof(t->Output); i+=16) {
            meshb[0] ^= ((uint64_t*)t->Output)[i/8] = ((uint64_t*)output)[i/8];;
            meshb[1] ^= ((uint64_t*)t->Output)[i/8 + 1] = ((uint64_t*)output)[i/8 + 1];
        }

        curTraceNum++;
        
        if (curTraceNum % 1000 == 0) {
            if (meshb[0] || meshb[1]) {
                printf("[Error] decryption mesh is incorrect!\n");
                assure(0);
            }
        }
#ifdef DEBUG
        if (curTraceNum % 1000 == 0) {
            printf("[W-%d] done %04d of %04d traces\n",wnum,curTraceNum,numOfTraces);
        }
#endif
    }
    
    auto compressedwritefunc = [](LoadTraceFile *trace, string outfilepath, std::string outfilename, int workerNum)->void{
        printf("[WT-%d] compressing decrypted trace...\n",workerNum);
        
        std::string tmpoutfilepath = outfilepath;
        tmpoutfilepath += ".partial";
        
        struct archive *a = NULL;
        struct archive_entry *entry = NULL;
        a = archive_write_new();
        archive_write_add_filter_gzip(a);
        archive_write_set_format_pax_restricted(a);
        archive_write_open_filename(a, tmpoutfilepath.c_str());
        
        entry = archive_entry_new();
        archive_entry_set_pathname(entry, outfilename.c_str());
        archive_entry_set_size(entry, trace->fsize()); // Note 3
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_mtime(entry, time(NULL), 0);
        archive_write_header(a, entry);

        archive_write_data(a, trace->buf(), trace->fsize()); //TracesInFile (actual data!)
        archive_entry_free(entry);
        
        archive_write_close(a); // Note 4
        archive_write_free(a); // Note 5

        rename(tmpoutfilepath.c_str(), outfilepath.c_str());
        
        printf("[WT-%d] done saving compressed decrypted trace to %s!\n",workerNum, outfilepath.c_str());
        
//        FILE *f = NULL;
//        if ((f = fopen(outfilepath.c_str(), "rb"))) {
//            fclose(f);
//            unlink(trace->path().c_str());
//        }else{
//            printf("[WT-%d] Error: failed to creat file=%s\n",workerNum,outfilepath.c_str());
//            assure(0);
//        }
        delete trace;
        wantWriteCounter.fetch_sub(1);
        fileWriteEvent.unlock(); //send wantWriteCounter update event
    };

#define MAX_WRITE_THREADS 16
    
    while (wantWriteCounter.fetch_add(1) > MAX_WRITE_THREADS) {
        wantWriteCounter.fetch_sub(1);
        printf("[T-%d] writer limit reached, waiting for some writers to finish...\n",wnum);
        fileWriteEvent.lock();
    }
    fileWriteEvent.unlock(); //always send event when counter is modified
    
    std::thread wthread(compressedwritefunc,trace, outFilePath, outFileName, wnum);
    wthread.detach();

    return 0;
}

int doDecrypt(const char *indir, const char *outdir){
    irecv_init();
    irecv_client_t client = NULL;

    for (int i = 0; i <= 5; i++) {
        printf("Attempting to connect... \n");
        if (irecv_open_with_ecid(&client, 0) != IRECV_E_SUCCESS)
            sleep(1);
        else
            break;

        if (i == 5) {
            printf("failed to open device\n");
            return -1;
        }
    }
    
    irecv_device_t device = NULL;
    irecv_devices_get_device_by_client(client, &device);
    printf("Connected to %s, model %s, cpid 0x%04x, bdid 0x%02x\n", device->product_type, device->hardware_model, device->chip_id, device->board_id);

    vector<string> toNormalizeList;
     
    printf("reading trace\n");
    for(auto& p: filesystem::directory_iterator(indir)){
        if (!ends_with(p.path(), ".dat") && !ends_with(p.path(), ".dat.tar.gz")) {
            continue;
        }
        printf("adding to list=%s\n",p.path().c_str());
        toNormalizeList.push_back(p.path());
    }
    
    
    size_t systemRam = (size_t)4e9;

    uint64_t cpuloadersCnt = sysconf(_SC_NPROCESSORS_ONLN);
#define MAX_CPU_LOADER_THREADS 8
    if (cpuloadersCnt > MAX_CPU_LOADER_THREADS) {
        cpuloadersCnt = MAX_CPU_LOADER_THREADS;
    }
    
    CPULoader cloader(toNormalizeList, systemRam, cpuloadersCnt, [outdir](LoadTraceFile *trace)->bool{
        
        auto outnames = getOutFileNameAndPath(trace,outdir);
        
        std::string outFileName = outnames.first;
        std::string outFilePath = outnames.second;
        if (FILE *f = fopen(outFilePath.c_str(), "rb")) {
            fclose(f);
            return false; // do not load this file
        }
        
        return true; //do load this file
    });

    mutex deviceMutex;
    
#define MAX_DEVICE_WORKER_THREADS 4
    std::queue<std::thread *> deviceWorkerThreads;
    for (int i=0; i<MAX_DEVICE_WORKER_THREADS; i++) {
        deviceWorkerThreads.push(new std::thread([&cloader, &deviceMutex, &client, outdir](int wnum)->void{
            while (LoadTraceFile *file = cloader.dequeue()) {
                assure(!decryptfunc(wnum, client, file, deviceMutex, outdir));
            }
            printf("deviceworker %d done transfer!\n",wnum);
        },i));
        sleep(1); // make them be asynchron
    }
    
    printf("Waiting for deviceworker to finish (%lu threads)!\n",deviceWorkerThreads.size());
    while (deviceWorkerThreads.size()) {
        auto t = deviceWorkerThreads.front();
        deviceWorkerThreads.pop();
        t->join();
        delete t;
    }
    
    printf("waiting for writers to finish...\n");
    while (wantWriteCounter > 0) {
        //we are only waiting for a few writers here, which are about to finish
        //eventlocks lead to locking issues.
        //don't hog CPU and sping here, sleep should be good enough, we won't stay here very long
        sleep(5);
    }
    printf("all writers finished!\n");
    
    
    printf("Done!\n");
    return 0;
}


int r_main(int argc, const char * argv[]) {
    int ret = 0;
    if (argc < 3) {
        printf("Usage: %s <traces indir path> <traces outdir path>\n",argv[0]);
        return -1;
    }
    
    const char *indir = argv[1];
    const char *outdir = argv[2];

    ret = doDecrypt(indir, outdir);

    return ret;
}


#ifndef NOMAIN
int main(int argc, const char * argv[]) {
    
#ifdef DEBUG
    return r_main(argc, argv);
#else //DEBUG
    try {
        return r_main(argc, argv);
    } catch (int e) {
        printf("ERROR: died on line=%d\n",e);
    }
#endif //DEBUG
}
#endif //NOMAIN

