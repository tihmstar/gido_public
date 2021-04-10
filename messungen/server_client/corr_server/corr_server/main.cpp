//
//  main.cpp
//  corr_server
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
#include <future>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "CorrServer.hpp"
#include <mutex>
#include <sys/stat.h>
#include <signal.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: MAIN ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: MAIN ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)


//#warning custom point_per_trace
//#define NUMBER_OF_POINTS_OVERWRITE 5000


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

CorrServer *gserver = NULL;

#ifdef SIGINFO
void sig_handler(int signo){
    if (signo == SIGINFO){
        printf("received SIGUSR1\n");
        if (gserver) {
            gserver->notifyMannualFilesUpdate();
        }
    }
}
#endif


int doServer(const char *indir, const char *outdir, int modelsCnt, const char **models){
    printf("start doServer\n");
    
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
    
    printf("indir=%s\n",indir);
    printf("outdir=%s\n",outdir);
  
#ifdef SIGINFO
    if (signal(SIGINFO, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGINFO\n");
#endif

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

    std::queue<std::string> fileQueue;
    for (auto e : toNormalizeList) {
        ssize_t lastSlash = e.rfind("/");
        if (lastSlash != std::string::npos) {
            e = e.substr(lastSlash+1);
        }
        fileQueue.push(e);
    }

    
    std::vector<std::string> powermodels;

    
    for (int i=0; i<modelsCnt; i++) {
        powermodels.push_back(models[i]);
    }

    for (auto &m : powermodels) {
        printf("Model=%s\n",m.c_str());
    }
    
    printf("Got %3lu powerModels!\n",powermodels.size());
    
    gserver = new CorrServer(8452, point_per_trace, powermodels, fileQueue);
    
    std::thread updateThread([]{
        char buf[500];
        while (true) {
            read(0, buf, sizeof(buf));
            if (gserver) {
                if (strncmp(buf, "info", strlen("info")) == 0) {
                    gserver->printClientStats();
                }else if (strncmp(buf, "kill", strlen("kill")) == 0) {
                    gserver->killClient(atoi(buf+strlen("kill ")));
                }
                gserver->notifyMannualFilesUpdate();
            }
        }
    });
    updateThread.detach();
    
    size_t lastCntSave = 0;
#define SAVE_CORR_STEPS 200

    
#ifdef DEBUG
#   undef SAVE_CORR_STEPS
#define SAVE_CORR_STEPS 1
#endif

    
    while (gserver->remainingFilesCnt() || gserver->clientsCnt() > 0) {
        size_t didProcess = fileQueue.size() - gserver->remainingFilesCnt();

        printf("[Server] (%2d clients) [processed files=%lu] [remaining files=%lu] (%lu queued, %lu pending)\n",gserver->clientsCnt(),didProcess,gserver->remainingFilesCnt(), gserver->queuedFilesCnt(), gserver->unqueuedFilesCnt());
        
#ifdef SAVE_CORR_STEPS
        if (didProcess-lastCntSave >= SAVE_CORR_STEPS) {
            std::string outsavepath = outdir;
            if (outsavepath.back() != '/') {
                outsavepath += '/';
            }
            outsavepath += to_string(didProcess) + "/";
            
            printf("[Server] saving midcorr (%lu) at=%s\n",didProcess,outsavepath.c_str());
            mkdir(outsavepath.c_str(), 0655);
            try {
                gserver->saveCorrToDir(outsavepath);
                lastCntSave = didProcess;
                printf("[Server] done saving midcorr (%lu) at=%s\n",didProcess,outsavepath.c_str());
            } catch (...) {
                //ignore midsave failures
                printf("[Server] saving midcorr (%lu) failed!\n",didProcess);
            }
        }
#endif
        gserver->hangUntilFilesUpdate();
    }

    printf("[Server] saving final corr...\n");
    gserver->saveCorrToDir(outdir);
    
    printf("[Server] done saving final corr!\n");


    {
        CorrServer *lserver = gserver; gserver = NULL;
        delete lserver;
    }

    printf("doServer done!\n");

    return 0;
}

int r_main(int argc, const char * argv[]) {
    int ret = 0;
    if (argc < 3) {
        printf("Usage: %s <traces dir path> <outdir> {powermodels ...}\n",argv[0]);
        return -1;
    }
    
    const char *indir = argv[1];
    const char *outdir = argv[2];
        

    ret = doServer(indir, outdir, argc-3, &argv[3]);

    printf("main done!\n");
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
        return -1;
    }
#endif //DEBUG
}
#endif //NOMAIN

