//
//  main.cpp
//  project9
//
//  Created by tihmstar on 07.07.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "Traces.hpp"
#include <future>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>

using namespace std;

#define TRACES_FILE "/Users/tihmstar/dev/gido/messungen/traces_normalized.dat"

#define POINTS_PER_TRACE tr.pointsPerTrace()
#define TRACES_IN_FILE tr.tracesInFile()
#define TRACES_STEPS TRACES_IN_FILE


inline bool isBadTrace(const Traces::MeasurementStructure *msm){
    if (msm->Trace[84] < 10) {
        return true;
    }
    return false;
}


inline void computeMean(Traces &tr, double *output, uint8_t doRandom, int tracesInFile){
//    printf("mean %s\n",doRandom ? "random" : "satic");
    memset(output, 0, sizeof(double)*tr.pointsPerTrace());
    for (int i=0; i<tr.pointsPerTrace(); i++) {
        int cnt = 0;
        for (int j=0; j<tracesInFile; j++) {
            const Traces::MeasurementStructure *msm = tr[j];
            if (isBadTrace(msm)) continue;
            if (Traces::isFixed(msm) != doRandom) {
                output[i] += msm->Trace[i];
                cnt ++;
            }
        }
        output[i] /= cnt;
    }
}

inline void computeVariace(Traces &tr, double *mean, double *output, uint8_t doRandom, int* cntTrace, int tracesInFile){
//    printf("var %s\n",doRandom ? "random" : "satic");
    memset(output, 0, sizeof(double)*tr.pointsPerTrace());
    int cnt = 0;
    for (int i=0; i<tr.pointsPerTrace(); i++) {
        cnt = 0;
        for (int j=0; j<tracesInFile; j++) {
            const Traces::MeasurementStructure *msm = tr[j];
            if (isBadTrace(msm)) continue;
            if (Traces::isFixed(msm) != doRandom) {
                double tmp = msm->Trace[i] - mean[i];
                output[i] += tmp*tmp;
                cnt ++;
            }
        }
        output[i] /= cnt;
    }
    *cntTrace = cnt;
}

inline void computeMomentN(Traces &tr, double *mean, double *output, uint8_t doRandom, int n, int tracesInFile){
//    printf("var %s\n",doRandom ? "random" : "satic");
    memset(output, 0, sizeof(double)*tr.pointsPerTrace());
    int cnt = 0;
    for (int i=0; i<tr.pointsPerTrace(); i++) {
        cnt = 0;
        for (int j=0; j<tracesInFile; j++) {
            const Traces::MeasurementStructure *msm = tr[j];
            if (isBadTrace(msm)) continue;
            if (Traces::isFixed(msm) != doRandom) {
                double tmp = msm->Trace[i] - mean[i];
                output[i] += pow(tmp, n);
                cnt ++;
            }
        }
        output[i] /= cnt;
    }
}

class myfile{
    int _fd;
    double *_mem;
    size_t _size;
    size_t _increment;
public:
    myfile(const char *fname, size_t size) : _size(size),_increment(0){
        int fdT1 = open(fname, O_RDWR | O_CREAT, 0644);
        lseek(fdT1, _size-1, SEEK_SET);
        write(fdT1, "", 1);
        _mem = (double*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_SHARED, fdT1, 0);
    }
    ~myfile(){
        munmap(_mem, _size);
        close(_fd);
    }
    double &operator[](int at){
        return _mem[at+_increment];
    }
};

int main(int argc, const char * argv[]) {
    printf("starting\n");

    Traces tr(TRACES_FILE);
    
    auto t1trace = myfile("tOrder1.dat",POINTS_PER_TRACE*sizeof(double)*(TRACES_IN_FILE/TRACES_STEPS));
    auto t2trace = myfile("tOrder2.dat",POINTS_PER_TRACE*sizeof(double)*(TRACES_IN_FILE/TRACES_STEPS));
    auto t3trace = myfile("tOrder3.dat",POINTS_PER_TRACE*sizeof(double)*(TRACES_IN_FILE/TRACES_STEPS));

    
    for (int tracesInFile = TRACES_STEPS; tracesInFile<=TRACES_IN_FILE; tracesInFile+=TRACES_STEPS){
        printf("-----curMaxTraces=%d-----\n",tracesInFile);
        int correctionval = ((tracesInFile/TRACES_STEPS)-1)*POINTS_PER_TRACE;
        
        double meanStatic[POINTS_PER_TRACE];
        double meanRandom[POINTS_PER_TRACE];
        
        double meanRandom2_[POINTS_PER_TRACE];
        double meanStatic2_[POINTS_PER_TRACE];
        
        double meanRandom3[POINTS_PER_TRACE];
        double meanStatic3[POINTS_PER_TRACE];
        
        double varStatic_[POINTS_PER_TRACE];
        double varRandom_[POINTS_PER_TRACE];
        
        
        double *meanRandom2 = meanRandom2_;
        double *meanStatic2 = meanStatic2_;
        double *varStatic = varStatic_;
        double *varRandom = varRandom_;
        
        
        int cntStatic = 0;
        int cntRandom = 0;
        
        double tOrder1[POINTS_PER_TRACE];
        memset(tOrder1, 0, sizeof(double)*POINTS_PER_TRACE);
        
        std::thread tts([&]{
            computeMean(tr, meanStatic, 0,tracesInFile);
            computeVariace(tr, meanStatic, varStatic, 0, &cntStatic,tracesInFile);
        });
        
        std::thread ttr([&]{
            computeMean(tr, meanRandom, 1,tracesInFile);
            computeVariace(tr, meanRandom, varRandom, 1, &cntRandom,tracesInFile);
        });
        
        tts.join();
        ttr.join();
        
        printf("[%d] computing t1-Value\n",tracesInFile);
        for (int i=0; i<POINTS_PER_TRACE; i++) {
            double upper = meanRandom[i] - meanStatic[i];
            double lower = sqrt((varRandom[i]/cntRandom)+(varStatic[i]/cntStatic));
            t1trace[i+correctionval] = upper/lower;
        }
        
        swap(meanRandom2, varRandom);
        swap(meanStatic2, varStatic);
        
        
        std::thread tts2([&]{
            computeMomentN(tr, meanStatic, varStatic, 0, 4,tracesInFile);
            for (int i=0; i<POINTS_PER_TRACE; i++) {
                varStatic[i] -= meanStatic2[i]*meanStatic2[i];
            }
        });
        
        std::thread ttr2([&]{
            computeMomentN(tr, meanRandom, varRandom, 1, 4,tracesInFile);
            for (int i=0; i<POINTS_PER_TRACE; i++) {
                varRandom[i] -= meanRandom2[i]*meanRandom2[i];
            }
        });
        
        tts2.join();
        ttr2.join();
        
        printf("[%d] computing t2-Value\n",tracesInFile);
        for (int i=0; i<POINTS_PER_TRACE; i++) {
            double upper = meanRandom2[i] - meanStatic2[i];
            double lower = sqrt((varRandom[i]/cntRandom)+(varStatic[i]/cntStatic));
            t2trace[i+correctionval] = upper/lower;
        }
        
        std::thread tts3([&](int d){
            computeMomentN(tr, meanStatic, meanStatic3, 0, d,tracesInFile);
            
            computeMomentN(tr, meanStatic, varStatic, 0, 2*d,tracesInFile);
            for (int i=0; i<POINTS_PER_TRACE; i++) {
                varStatic[i] = (varStatic[i] - meanStatic3[i]*meanStatic3[i]) / pow(meanStatic2[i], d);
            }
            
            for (int i=0; i<POINTS_PER_TRACE; i++) {
                meanStatic3[i] /= pow(meanStatic2[i], d/2.0);
            }
            
        },3);
        
        
        std::thread ttr3([&](int d){
            computeMomentN(tr, meanRandom, meanRandom3, 1, d,tracesInFile);
            
            computeMomentN(tr, meanRandom, varRandom, 1, 2*d,tracesInFile);
            for (int i=0; i<POINTS_PER_TRACE; i++) {
                varRandom[i] = (varRandom[i] - meanRandom3[i]*meanRandom3[i]) / pow(meanRandom2[i], d);
            }
            
            for (int i=0; i<POINTS_PER_TRACE; i++) {
                meanRandom3[i] /= pow(meanRandom2[i], d/2.0);
            }
        },3);
        
        
        tts3.join();
        ttr3.join();
        
        printf("[%d] computing t3-Value\n",tracesInFile);
        for (int i=0; i<POINTS_PER_TRACE; i++) {
            double upper = meanRandom3[i] - meanStatic3[i];
            double lower = sqrt((varRandom[i]/cntRandom)+(varStatic[i]/cntStatic));
            t3trace[i+correctionval] = upper/lower;
        }
    }
    
    
    
    printf("done\n");
    return 0;
}
