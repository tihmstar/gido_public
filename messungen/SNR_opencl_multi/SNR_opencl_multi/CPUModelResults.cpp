//
//  CPUModelResults.cpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 27.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CPUModelResults.hpp"
#include "numerics.hpp"
#include <sys/mman.h>
#include <string.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: CPUModelResults ASSURE FAILED ON LINE=%d with err=%d\n",__LINE__,clret); exit(-1);}}while (0)
#define assure_(cond) do {if (!(cond)) {printf("ERROR: CPUModelResults ASSURE FAILED ON LINE=%d\n",__LINE__); exit(-1);}}while (0)


CPUModelResults::CPUModelResults(std::string modelName, cl_uint modelSize, cl_uint point_per_trace)
    : _modelName(modelName), _modelSize(modelSize), _point_per_trace(point_per_trace)
{
    for (int i=0; i<_modelSize; i++) {
        _groupsMean.push_back({0,0});
        _groupsCensum.push_back({0,0});
    }
    
    //alloc memory
    for (auto &cmbtr : _groupsMean){
        cmbtr.trace = (gpu_float_type*)malloc(_point_per_trace*sizeof(gpu_float_type));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, _point_per_trace*sizeof(gpu_float_type));
    }
    for (auto &cmbtr : _groupsCensum){
        cmbtr.trace = (gpu_float_type*)malloc(_point_per_trace*sizeof(gpu_float_type));
        cmbtr.cnt = 0;
        memset(cmbtr.trace, 0, _point_per_trace*sizeof(gpu_float_type));
    }
}

CPUModelResults::~CPUModelResults(){
    for (int i=0; i<_modelSize; i++) {
        free(_groupsMean.at(i).trace);_groupsMean.at(i).trace = NULL;
        free(_groupsCensum.at(i).trace);_groupsCensum.at(i).trace = NULL;
    }
}

void CPUModelResults::combineResults(std::vector<combtrace> &retgroupsMean,std::vector<combtrace> &retgroupsCensum){
    assure_(retgroupsMean.size() == _modelSize);
    assure_(retgroupsCensum.size() == _modelSize);
    for (size_t i=0; i<_modelSize; i++) {
        
        combineMeans(_groupsMean.at(i), retgroupsMean.at(i), _point_per_trace);
        combineCenteredSum(_groupsCensum.at(i), _groupsMean.at(i), retgroupsCensum.at(i), retgroupsMean.at(i), _point_per_trace);
    }
}

std::vector<combtrace> &CPUModelResults::groupsMean(){
    return _groupsMean;
}

std::vector<combtrace> &CPUModelResults::groupsCensum(){
    return _groupsCensum;
}
