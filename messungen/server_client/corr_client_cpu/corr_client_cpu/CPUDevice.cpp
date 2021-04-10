//
//  GPUDevice.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CPUDevice.hpp"
#include <signal.h>
#include <thread>
#include <unistd.h>
#include "powermodel.hpp"
#include "numerics.hpp"

#define assure(cond) do {if (!(cond)) {printf("ERROR: CPUDevice ASSURE FAILED ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)
#define safeFreeCustom(ptr,func) ({if (ptr) func(ptr),ptr=NULL;})


CPUDevice::CPUDevice(uint32_t point_per_trace, std::vector<std::string> models, uint16_t threadsCnt)
: _point_per_trace(point_per_trace),_threadsCnt(threadsCnt), _wipTrace(NULL), _workerIsRunningCnt(0), _wipTraceUsageCnt(0),
    _workerSyncCounter(0)
{
    
    if (_threadsCnt == 0) {
        _threadsCnt = sysconf(_SC_NPROCESSORS_ONLN);
    }
    
    //assuming model size is 256
    for (auto &name : models) {
        for (int i=0; i<0x100; i++) {
            _gmodelresults[name][std::to_string(i)] = new CPUModelResults(std::to_string(i),_point_per_trace);
        }
        _gmodelresults[name]["trace"] = new CPUModelResults("trace",_point_per_trace);
    }
    
    //assuming model size is 256
    for (auto &name : models) {
        for (int i=0; i<0x100; i++) {
            _curmodelresults[name][std::to_string(i)] = new CPUModelResults(std::to_string(i),_point_per_trace);
        }
        _curmodelresults[name]["trace"] = new CPUModelResults("trace",_point_per_trace);
    }
        
    for (uint16_t i = 0; i<_threadsCnt; i++) {
        _workers.push(new std::thread([this](int workernum){
            _workerIsRunningCnt++;
            
            worker(workernum);
            
            _workerIsRunningCnt--;
        },i));
    }
    
    while (_workerIsRunningCnt < _threadsCnt) {
        usleep(100); //wait until we spawned all workers
    }
    
    printf("");
}

CPUDevice::~CPUDevice(){
    _workerShouldStop = true;
    _wipTraceEventLock.unlock();

    while (_workerIsRunningCnt) {
        sleep(1); //wait for all workers to finish
    }
    
    //free memory
    for (auto &category : _gmodelresults) {
        for (auto &model : category.second) {
            delete model.second;
        }
    }

    //free memory
    for (auto &category : _curmodelresults) {
        for (auto &model : category.second) {
            delete model.second;
        }
    }

}

void CPUDevice::resetCur(){
    //free memory
    for (auto &category : _curmodelresults) {
        for (auto &model : category.second) {
            delete model.second;
        }
    }
    
    //assuming model size is 256
    for (auto &category : _curmodelresults) {
        for (int i=0; i<0x100; i++) {
            _curmodelresults[category.first][std::to_string(i)] = new CPUModelResults(std::to_string(i),_point_per_trace);
        }
        _curmodelresults[category.first]["trace"] = new CPUModelResults("trace",_point_per_trace);
    }
}

void CPUDevice::resetDevice(){
    //free memory
    for (auto &category : _gmodelresults) {
        for (auto &model : category.second) {
            delete model.second;
        }
    }
    
    //assuming model size is 256
    for (auto &category : _gmodelresults) {
        for (int i=0; i<0x100; i++) {
            _gmodelresults[category.first][std::to_string(i)] = new CPUModelResults(std::to_string(i),_point_per_trace);
        }
        _gmodelresults[category.first]["trace"] = new CPUModelResults("trace",_point_per_trace);
    }
}

void CPUDevice::worker(int workernum){
    uint32_t traceWorkSize = _point_per_trace / _threadsCnt;
    
    uint32_t myWorkTraceBegin = traceWorkSize * workernum;
    uint32_t myWorkTraceEnd = traceWorkSize * (workernum+1);

    if (myWorkTraceEnd == _threadsCnt-1) {
        //if i am the last worker, finish all remaining work
        myWorkTraceEnd = _point_per_trace;
    }

    uint32_t hypotWorkSize = 0x100 / _threadsCnt;

    
    uint32_t myWorkHypotBegin = hypotWorkSize * workernum;
    uint32_t myWorkHypotEnd = hypotWorkSize * (workernum+1);

    if (myWorkHypotEnd == _threadsCnt-1) {
        //if i am the last worker, finish all remaining work
        myWorkTraceEnd = 0x100;
    }
    
    uint32_t syncPerRound = _curmodelresults.size()*3; //3 stages of sync
    uint32_t syncPerExecution = syncPerRound*_threadsCnt;
    
    
    while (!_workerShouldStop) {
        
        _wipTraceEventLock.lock();
        
        if (!_wipTrace) {
            continue; //no work to do
        }
        
        _wipTraceEventLock.unlock();
        
        Tracefile *tf = (Tracefile *)_wipTrace->buf();

        uint32_t tracesInFile = tf->tracesInFile;
        uint32_t pointsPerTrace = tf->ms->NumberOfPoints;
        uint32_t nextMsmOffset = sizeof(struct MeasurementStructure)+pointsPerTrace;

        const double factor = (1.0/tracesInFile);


        int curmodelCnt = 0;
        _workerSyncCounter += syncPerRound;
        
        for (auto &model : _curmodelresults) {
            curmodelCnt++;
            
            uint8_t key_selector = 0;
            ssize_t kspos = model.first.rfind('_');
            assure(kspos != std::string::npos);

            std::string ksstring = model.first.substr(kspos+1);
            key_selector = atoi(ksstring.c_str());
            
            CPUModelResults *curtrace = model.second["trace"];

            curtrace->_meanTrace.cnt = tracesInFile; /* warning every thread does this */
            curtrace->_censumTrace.cnt = tracesInFile; /* warning every thread does this */

            double *curmeantrace = curtrace->_meanTrace.trace;
            double *curcensumtrace = curtrace->_censumTrace.trace;

            //compute mean trace
            {
                const struct MeasurementStructure *msm = tf->ms;
                for (uint32_t j=0; j < tracesInFile; j++,msm = (const struct MeasurementStructure *)(((uint8_t*)msm)+nextMsmOffset)){

                    for (uint32_t t = myWorkTraceBegin; t<myWorkTraceEnd; t++) {
                        curmeantrace[t] = msm->Trace[t] * factor;
                    }
                }
            }
            
            
            //compute mean hypot
            for (uint32_t k = myWorkHypotBegin; k<myWorkHypotEnd; k++) {
                combtrace &curhypotmean = model.second[std::to_string(k)]->_meanHypot;
                curhypotmean.cnt = tracesInFile; /* warning every thread does this */
                double *curmeanhypot = curhypotmean.trace;
                
                const struct MeasurementStructure *msm = tf->ms;
                for (uint32_t j=0; j < tracesInFile; j++,msm = (const struct MeasurementStructure *)(((uint8_t*)msm)+nextMsmOffset)){
                    uint8_t hypot = powerModel_ATK(msm,k,key_selector);
                    curmeanhypot[k] += hypot * factor;
                }
            }

            //compute centered sum trace
            for (uint32_t t = myWorkTraceBegin; t<myWorkTraceEnd; t++) {
                double curMean = curmeantrace[t];
        
                const struct MeasurementStructure *msm = tf->ms;
                for (uint32_t j=0; j < tracesInFile; j++,msm = (const struct MeasurementStructure *)(((uint8_t*)msm)+nextMsmOffset)){
                    //normal censum
                    double tmp = msm->Trace[t] - curMean;
                    curcensumtrace[t] += tmp*tmp;
                }
            }

            
            //compute centered sum hypot
            for (uint32_t k = myWorkHypotBegin; k<myWorkHypotEnd; k++) {
                CPUModelResults *curmodel = model.second[std::to_string(k)];
                
                combtrace &curhypotmean = curmodel->_meanHypot;
                double curMeansHypot = curhypotmean.trace[k];

                combtrace &curhypotcensum= curmodel->_censumHypot;


                curhypotcensum.cnt = tracesInFile; /* warning every thread does this */
                double *curcensumhypot = curhypotcensum.trace;
                
                const struct MeasurementStructure *msm = tf->ms;
                for (uint32_t j=0; j < tracesInFile; j++,msm = (const struct MeasurementStructure *)(((uint8_t*)msm)+nextMsmOffset)){

                    //hypot
                    uint8_t hypot = powerModel_ATK(msm,k,key_selector);
                    double hypotTmp = hypot - curMeansHypot;
                    
                    curcensumhypot[k] += hypotTmp*hypotTmp;

                }
            }
            
            /* --- SYNC HERE STAGE 1 --- */

            //first we tell we are ready to sync
            --_workerSyncCounter;

            //then we announce our changes
            _workerSyncLock.unlock();

            //then we wait until everyone else is done
            while (_workerSyncCounter > syncPerExecution - (curmodelCnt*_threadsCnt + 0)) {
                _workerSyncLock.lock();
            }

            //once we received the announcement that everyone is ready, tell every one else about it
            _workerSyncLock.unlock();

            /* --- SYNC DONE STAGE 1 --- */

            
            //computer upper part
            for (uint32_t t = myWorkTraceBegin; t<myWorkTraceEnd; t++) {
                double curMean = curmeantrace[t];
                
                for (uint32_t k = 0; k<0x100; k++) {
                    CPUModelResults *curmodel = model.second[std::to_string(k)];
                    
                    combtrace &curhypotmean = curmodel->_meanHypot;
                    double curMeansHypot = curhypotmean.trace[k];

                    combtrace &curUpperPartComb = curmodel->_upperPart;
                    double *curUpperPartTrace = curUpperPartComb.trace;

                    
                    const struct MeasurementStructure *msm = tf->ms;
                    for (uint32_t j=0; j < tracesInFile; j++,msm = (const struct MeasurementStructure *)(((uint8_t*)msm)+nextMsmOffset)){

                        
                        double tmp = msm->Trace[t] - curMean;

                        uint8_t hypot = powerModel_ATK(msm,k,key_selector);
                        double hypotTmp = hypot - curMeansHypot;
                        curUpperPartTrace[k] += hypotTmp*tmp;
                        
                    }

                }
        
            }
            
            /* --- SYNC HERE STAGE 2 --- */

            //first we tell we are ready to sync
            --_workerSyncCounter;

            //then we announce our changes
            _workerSyncLock.unlock();

            //then we wait until everyone else is done
            while (_workerSyncCounter > syncPerExecution - (curmodelCnt*_threadsCnt + 1)) {
                _workerSyncLock.lock();
            }

            //once we received the announcement that everyone is ready, tell every one else about it
            _workerSyncLock.unlock();

            /* --- SYNC DONE STAGE 2 --- */
            
            
            if (workernum == 0) {
                for (auto &category : _gmodelresults) {
                    CPUModelResults *newResults = _curmodelresults[category.first]["trace"];
                    CPUModelResults *globalResults = _gmodelresults[category.first]["trace"];

                    combineMeans(globalResults->_meanTrace, newResults->_meanTrace, _point_per_trace);
                    combineCenteredSum(globalResults->_censumTrace, globalResults->_meanTrace, newResults->_censumTrace, newResults->_meanTrace, _point_per_trace);
                    
                    for (auto &model : category.second) {
                        if (model.first == "trace") {
                            continue;
                        }
                        CPUModelResults *newResults = _curmodelresults[category.first][model.first];
                        CPUModelResults *globalResults = _gmodelresults[category.first][model.first];
                        
                        combineMeans(globalResults->_meanHypot, newResults->_meanHypot, 1);
                        combineCenteredSum(globalResults->_censumHypot, globalResults->_meanHypot, newResults->_censumHypot, newResults->_meanHypot, 1);

                        for (int i=0; i<_point_per_trace; i++) {
                            globalResults->_upperPart.trace[i] += globalResults->_upperPart.trace[i];
                        }
                    }
                }
            }
            
            /* --- SYNC HERE STAGE 3 --- */

            //first we tell we are ready to sync
            --_workerSyncCounter;

            //then we announce our changes
            _workerSyncLock.unlock();

            //then we wait until everyone else is done
            while (_workerSyncCounter > syncPerExecution - (curmodelCnt*_threadsCnt + 2)) {
                _workerSyncLock.lock();
            }

            //once we received the announcement that everyone is ready, tell every one else about it
            _workerSyncLock.unlock();

            /* --- SYNC DONE STAGE 3 --- */
            
        }
        
        if(_wipTraceUsageCnt.fetch_sub(1) == 1){
            //this guy removed the last reference to _wipTrace. free it!
            delete _wipTrace;
            _wipTrace = NULL;
            
            resetCur();
            
            _wipTraceLock.unlock();
        }
    }
}

void CPUDevice::transferTrace(LoadFile *trace){
    _wipTraceLock.lock();

    assure(!_wipTrace);
    assure(!_wipTraceUsageCnt);
    _wipTraceUsageCnt = (uint16_t)_workerIsRunningCnt;
    _wipTrace = trace;
    
    _wipTraceEventLock.unlock();
}

void CPUDevice::getResults(std::map<std::string,std::map<std::string,CPUModelResults*>> &modelresults){
    _wipTraceLock.lock(); //make sure no trace is in use
    assure(!_wipTrace);
    assure(!_wipTraceUsageCnt);

    for (auto &category : _gmodelresults) {
        for (auto &model : category.second) {
            CPUModelResults *globalResults = _gmodelresults[category.first][model.first];
            modelresults[category.first][model.first] = globalResults;
            _gmodelresults[category.first][model.first] = new CPUModelResults(globalResults->modelName(), _point_per_trace);
        }
    }
    
    _wipTraceLock.unlock();
}
