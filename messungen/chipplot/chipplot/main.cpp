//
//  main.cpp
//  chipplot
//
//  Created by tihmstar on 23.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "ResArray.hpp"

#include <forge.h>
#define USE_FORGE_CPU_COPY_HELPERS
#include <ComputeCopy.h>
#include <complex>
#include <cmath>
#include <vector>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "CPULoader.hpp"


#define USEC_PER_SEC 1000000

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); raise(SIGABRT); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

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

void increase_file_limit() {
    struct rlimit rl = {};
    int error = getrlimit(RLIMIT_NOFILE, &rl);
    assert(error == 0);
    rl.rlim_cur = 10240;
    rl.rlim_max = rl.rlim_cur;
    error = setrlimit(RLIMIT_NOFILE, &rl);
    if (error != 0) {
        printf("could not increase file limit\n");
    }
    error = getrlimit(RLIMIT_NOFILE, &rl);
    assert(error == 0);
    if (rl.rlim_cur != 10240) {
        printf("file limit is %llu\n", rl.rlim_cur);
    }
}

diriter std::filesystem::directory_iterator(std::string dirpath){
    DIR *dir = NULL;
    struct dirent *ent = NULL;
    diriter ret;
    
    assure_(dir = opendir(dirpath.c_str()));
    
    while ((ent = readdir (dir)) != NULL) {
        if (ent->d_type != DT_REG && ent->d_type != DT_DIR)
            continue;
        if (ent->d_name[0] == '.') {
            continue;
        }
        ret._file.push_back({dirpath + "/" + ent->d_name});
    }
    
    if (dir) closedir(dir);
    return ret;
}
#endif

struct myData{
    int xpos;
    int ypos;
    ResArray *tr;
};


struct maxXYZT{
    int x;
    int y;
    float z;
    uint32_t t;
};

using namespace std;

std::tuple<int, int, float, float, float> genSurface(std::vector<float> &vec, vector<myData> &values, uint32_t t, int &dimX, int &dimY){
    int xMax = 0;
    int yMax = 0;

    float zMax = 0;
    float zMin = 0;
    float zAvg = 0;
    int avgCnt = 0;

    vec.clear();
    
    for (auto &d : values) {
        vec.push_back(d.xpos);
        vec.push_back(d.ypos);
        
        float z = (*(d.tr))[t];
        if (dimX < d.xpos) dimX = d.xpos;
        if (dimY < d.ypos) dimY = d.ypos;
        if (zMax < z){
            zMax = z;
            xMax = d.xpos;
            yMax = d.ypos;
        }
        if (zMin > z) zMin = z;

        zAvg += (z - zAvg)/(++avgCnt);

        vec.push_back(z);
    }
    return {xMax, yMax, zMin, zMax, zAvg};
}

std::tuple<int, int, float, float, float> genSurfaceMax(std::vector<float> &vec, vector<myData> &values, int &dimX, int &dimY, std::vector<maxXYZT> &maxspots){
    int xMax = 0;
    int yMax = 0;

    float zMax = 0;
    float zMin = 0;
    float zAvg = 0;
    int avgCnt = 0;

    vec.clear();
    
    for (auto &d : values) {
        vec.push_back(d.xpos);
        vec.push_back(d.ypos);
        
        float z = 0;
        uint32_t maxT = 0;
        for (uint32_t t = 0; t<d.tr->size(); t++) {
            float v = (*(d.tr))[t];
            v = fabs(v);
                        
            if (z < v){
                z = v;
                maxT = t;
            }
        }
                
        if (dimX < d.xpos) dimX = d.xpos;
        if (dimY < d.ypos) dimY = d.ypos;
        if (zMax < z){
            zMax = z;
            xMax = d.xpos;
            yMax = d.ypos;
        }
        if (zMin > z) zMin = z;

        zAvg += (z - zAvg)/(++avgCnt);

        
        
        maxspots.push_back({d.xpos,d.ypos,z,maxT});
        vec.push_back(z);
    }
    return {xMax, yMax, zMin, zMax, zAvg};
}



int doplot(vector<myData> &values, int time, const char *model){

    /*
     * First Forge call should be a window creation call
     * so that necessary OpenGL context is created for any
     * other forge::* object to be created successfully
     */
    forge::Window wnd(1024, 768, "3d Surface Demo");
    wnd.makeCurrent();
    
    forge::Chart chart(FG_CHART_3D);


    //generate a surface
    std::vector<float> function;
    int xMax = 0;
    int yMax = 0;
    int dimX = 0;
    int dimY = 0;
    float zMin = 0;
    float zMax = 0;
    
    float zAvg = 0;
    int avgCnt = 0;
    
    uint32_t maxTime = 0;
    std::vector<maxXYZT> maxSpots;
    
    if (time >= 0) {
        uint32_t t = time;
        function.clear();
        auto maxVals = genSurface(function, values, t, dimX, dimY);
        xMax = std::get<0>(maxVals);
        yMax = std::get<1>(maxVals);
        zMin = std::get<2>(maxVals);
        zMax = std::get<3>(maxVals);
        zAvg = std::get<4>(maxVals);
        
        for (auto &v : values) {
            maxSpots.push_back({v.xpos,v.ypos, (float)(*(v.tr))[t],t});
        }
                
    }else{
        function.clear();
        auto maxVals = genSurfaceMax(function, values, dimX, dimY,maxSpots);
        xMax = std::get<0>(maxVals);
        yMax = std::get<1>(maxVals);
        zMin = std::get<2>(maxVals);
        zMax = std::get<3>(maxVals);
        zAvg = std::get<4>(maxVals);
    }

    if (time == -1) {
        time = maxTime;
    }
    
    float myZMax = zMax;
    if (myZMax > 50) {
        myZMax = 50;
    }else{
        if (myZMax < 0.01)
          myZMax = 0.01;
    }

    
    chart.setAxesLimits(0, dimX, 0, dimY, zMin, myZMax);
//    chart.setAxesTitles("x-axis", "y-axis", "Correlation (10^-3)");
    chart.setAxesTitles("", "", "");
//    chart.setAxesLabelFormat("%4.0f","%4.0f","%4.2f");
    chart.setAxesLabelFormat("","","");


    forge::Surface surf = chart.surface(dimX+1, dimY+1, forge::f32);
    surf.setColor(FG_YELLOW);

    GfxHandle* handle;
    createGLBuffer(&handle, surf.vertices(), FORGE_VERTEX_BUFFER);
    
    
    std::sort(maxSpots.begin(), maxSpots.end(), [](const maxXYZT &a, const maxXYZT &b){
        return a.z>b.z;
    });
    
#define TOP_X 20
    printf("Top %d: model=%s\n",TOP_X,model);
    for (int i=0; i<TOP_X && i<maxSpots.size(); i++) {
        auto &c = maxSpots.at(i);
        printf("Y%2d_X%2d time=%5u z=%f\n",c.y,c.x,c.t,c.z);
    }
    
    maxSpots.clear();
    uint32_t t = 0;
    
//    {
//        function.clear();
//        genSurfaceMax(function, values, dimX, dimY, maxSpots);
//        FILE *f = fopen("data.txt", "w");
//        int i = 0;
//        for (float v : function) {
////            fwrite(&v, 1, sizeof(float), f);
//            fprintf(f, "%15.15f",v);
//            if (i++ == 2) {
//                i=0;
//                fprintf(f, "\n");
//            }else{
//                fprintf(f, ",");
//            }
//        }
//        fclose(f);
//        exit(1);
//    }

    
    if (time >= 0) {
        t = time;
        printf("\nyMax=%d xMax=%d zMax=%f zAvg=%f time=%u model=%s\n",yMax,xMax,zMax,zAvg,time, model);
        function.clear();
        genSurfaceMax(function, values, dimX, dimY, maxSpots);
        copyToGLBuffer(handle, (ComputeResourceHandle)function.data(), surf.verticesSize());
        t = time;
        std::string title{"max "};
        title += model;
        wnd.setTitle(title.c_str());

        do {
            wnd.draw(chart);
            usleep(USEC_PER_SEC*0.3);
        } while(!wnd.close());
    }else{
        t = (t+1) % values.at(0).tr->size();
        function.clear();
        genSurface(function, values, t, dimX, dimY);
        copyToGLBuffer(handle, (ComputeResourceHandle)function.data(), surf.verticesSize());

        std::string title{"t="};
        title += to_string(t);
        title += " ";
        title += model;
        wnd.setTitle(title.c_str());
        do {
            wnd.draw(chart);
            usleep(USEC_PER_SEC*0.3);
        } while(!wnd.close());
    }


    releaseGLBuffer(handle);

    return 0;
}

namespace forge {
    namespace opengl{
        extern int CHART2D_FONT_SIZE;
    }
}


int main(int argc, const char **argv){
    printf("start\n");

    if (argc < 3) {
        printf("Usage: %s <results dir path> <model> {time} {blacklist...}\n",argv[0]);
        return -1;
    }
    increase_file_limit();
    
    
    forge::opengl::CHART2D_FONT_SIZE = 40;

    const char *indir = argv[1];
    const char *model = argv[2];
    int time = -1;
    
    std::vector<std::string> blacklist;
    
    printf("indir=%s\n",indir);
    printf("model=%s\n",model);
    
    if (argc > 3) {
        time = atoi(argv[3]);
        printf("time=%d\n",time);
    }
    
    for (int i=4; i<argc; i++) {
        blacklist.push_back(argv[i]);
    }

    vector<std::tuple<int,int,std::string>> toLoadPaths;
    
    vector<myData> positions;

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
        

        if (std::find(blacklist.begin(), blacklist.end(), dirname) != blacklist.end()) {
            printf("ignoring blacklisted pos=%s\n",dirname.c_str());
//            positions.push_back({xpos,ypos, new ResArray(positions.at(0).tr->size())});
            continue;
        }
        
        std::string resultsPath = p.path();
        resultsPath += "/";
        resultsPath += model;
        printf("reading=%s\n",resultsPath.c_str());
        toLoadPaths.push_back({xpos,ypos,resultsPath});
    }

    CPULoader loader(toLoadPaths,6);
    
    
    while (true) {
        auto a = loader.dequeue();
        if (!std::get<2>(a)) break;
        
        positions.push_back({std::get<0>(a),std::get<1>(a),std::get<2>(a)});
    }

    std::sort(positions.begin(), positions.end(), [](const myData &a, const myData &b){
        if (a.xpos == b.xpos) {
            return a.ypos<b.ypos;
        }
        return a.xpos<b.xpos;
    });
    

    printf("\n");

    doplot(positions, time, model);
    
    
    printf("done\n");
    return 0;
}
