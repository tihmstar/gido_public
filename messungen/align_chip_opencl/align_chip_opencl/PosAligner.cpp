//
//  PosAligner.cpp
//  align_chip_opencl
//
//  Created by tihmstar on 10.01.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "PosAligner.hpp"
#include <thread>
#include "GPUWorker.hpp"
#include <signal.h>
#include <sys/stat.h>
#include "TraceAligner.hpp"
#include <future>
#include "CPULoader.hpp"
#include <algorithm>


#define assure(cond) do {if (!(cond)) {printf("ERROR: PosAligner ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); raise(SIGABRT); exit(-1);}}while (0)


PosAligner::PosAligner(GPUDevice *gpu)
    : _gpu(gpu)
{
    
}


void PosAligner::enqueuePosList(const std::vector<std::string> posList){
    _doMeanQueue.push(posList);
    assure(_doMeanQueue.back().size());
}

void PosAligner::run(const char *outdir){
    _gpuWorkersEventLock.lock();
    
    std::thread doMeans([&]{
        int wnum=0;
        while (_doMeanQueue.size()) {
            std::vector<std::string> p = _doMeanQueue.front();
            assure(p.size());
            _doMeanQueue.pop();
            
            assure(p.size());
            
            GPUWorker *worker = new GPUWorker(wnum++,_gpu);
            
            CPULoader loader(p, 5e9, 4);

            while (LoadFile *f = loader.dequeue()) {
                GPUMem *mem = _gpu->transferTrace(f);
                worker->enqueue(mem);
                mem->release();
                worker->cputraces().push(f);
            }
                        
            
            //push worker in queue
            _gpuWorkersLock.lock();
            _gpuWorkers.push(worker);
            _gpuWorkersEventLock.unlock();
            _gpuWorkersLock.unlock();            
        }
        _gpuWorkersEventLock.unlock(); //send empty event
    });
    
    auto meanContinue = [&](int workernum){
        while (true) {
            GPUWorker *worker = NULL;
            if (!_gpuWorkers.size()){
                _gpuWorkersEventLock.lock(); //wait for event
            }

            if (!_gpuWorkers.size()){
                printf("[%02d] worker reached end of work\n",workernum);
                _gpuWorkersEventLock.unlock();
                break; //this was the empty event to signal end of work
            }
            
            _gpuWorkersLock.lock();
            worker = _gpuWorkers.front();
            _gpuWorkers.pop();
            _gpuWorkersLock.unlock();
            
            combtrace mean = worker->getResults();

#ifdef DEBUG
#warning DEBUG
            {
                FILE *f = fopen("mean.dat", "wb");
                fwrite(mean.trace, 1, sizeof(gpu_float_type)*_gpu->_point_per_trace, f);
                fclose(f);
            }
#endif
            std::vector<uint32_t> peaks;
            
            double lastPoint = 0;
            int direction = 0; //2 up, 1 down
            lastPoint = mean.trace[0];
            
            double meanVal = 0;
            
            for (int t=0; t<_gpu->_point_per_trace; t++) {
                if (mean.trace[t] > lastPoint) {
                   if (direction && direction != 2){ //check if we change directions
                       peaks.push_back(t-1);
                   }
                   direction = 2; //up
                }else if (mean.trace[t] < lastPoint){
                   if (direction && direction != 1){ //check if we change directions
                       peaks.push_back(t-1);
                   }
                   direction = 1;
                }
                lastPoint = mean.trace[t];
                meanVal += mean.trace[t]/_gpu->_point_per_trace;
            }


            double upMean = 0;
            double downMean = 0;
            int upCnt = 0;
            int downCnt = 0;
                        
            for (auto &p : peaks) {
                double v = mean.trace[p];
                if (v > meanVal) {
                    upMean += v;
                    upCnt++;
                }else if (v < meanVal){
                    downMean += v;
                    downCnt++;
                }
            }
            upMean /= upCnt;
            downMean /= downCnt;

#ifdef DEBUG
#warning DEBUG
            {
                FILE *f=fopen("peak.txt", "w");
                for (auto &p : peaks) {
                    double v = mean.trace[p];
                    if (v > downMean && v < upMean)
                        continue;
                    fprintf(f,"%u\n",p);
                }
                fclose(f);
            }
#endif
#ifdef DEBUG
#warning DEBUG
            {
                FILE *f=fopen("means.txt", "w");
                fprintf(f, "%f\n",meanVal);
                fprintf(f, "%f\n",upMean);
                fprintf(f, "%f\n",downMean);
                fclose(f);
            }
#endif
            
#define WINDOW_SIZE 3000
#define MOVE_SIZE 500
            
            std::vector<std::pair<uint32_t, double>> wins;
            
//            printf("searching window...\n");
            for (int t=MOVE_SIZE; t<_gpu->_point_per_trace-WINDOW_SIZE-MOVE_SIZE; t++) {
                double wval = 0;
                
                for (auto &p : peaks) {
                    double v = mean.trace[p];

                    if (p < t || p > t + WINDOW_SIZE) continue; //check if peak in window
                    if (v > downMean && v < upMean) continue; //check if peak isn't noise
                    v = v-meanVal;
                    if (v < 0) v *= -1;
                    wval +=v;
                }
                wins.push_back({t,wval});
            }
            
                        
            std::sort(wins.begin(), wins.end(), [](const std::pair<uint32_t, double> &a, const std::pair<uint32_t, double> &b){
                return a.second > b.second;
            });
            
//            printf("b=%d e=%d f=%f\n",wins.at(0).first,wins.at(0).first+WINDOW_SIZE,wins.at(0).second);
//            printf("b=%d e=%d f=%f\n",wins.at(wins.size()-1).first,wins.at(wins.size()-1).first+WINDOW_SIZE,wins.at(wins.size()-1).second);

            uint32_t align_window_start = wins.at(0).first;

#ifdef DEBUG
#warning DEBUG
            {
                FILE *f=fopen("vlines.txt", "w");
                fprintf(f, "%d_%d\n",align_window_start,align_window_start+WINDOW_SIZE);
                fclose(f);
            }
#endif
            
            std::string posdir = worker->cputraces().front()->path();
            ssize_t lastSlash = posdir.rfind("/");
            assure(lastSlash != std::string::npos);
            posdir = posdir.substr(0,lastSlash);
            lastSlash = posdir.rfind("/");
            assure(lastSlash != std::string::npos);
            posdir = posdir.substr(lastSlash+1);
            
            std::string outdirpath = outdir;
            if (outdirpath.back() != '/') outdirpath += '/';
            outdirpath += posdir;
            if (outdirpath.back() != '/') outdirpath += '/';

            mkdir(outdirpath.c_str(), 0755);
            
            doNormalize(worker->cputraces(), outdirpath.c_str(), 4, _gpu->_point_per_trace, align_window_start, WINDOW_SIZE, MOVE_SIZE);
            delete worker;
        }
    };
    
    
    int meanContinueThreadsCnt = 4;
    std::vector<std::future<void>> workers;

    printf("spinning up %d meanContinue worker threads\n",meanContinueThreadsCnt);
    
    for (int i=0; i<meanContinueThreadsCnt; i++){
        workers.push_back(std::async(std::launch::async,meanContinue,i));
    }

    printf("waiting for workers to finish...\n");
    for (int i=0; i<meanContinueThreadsCnt; i++){
        workers[i].wait();
    }
    printf("all workers finished!\n");


    
    doMeans.join();
}
