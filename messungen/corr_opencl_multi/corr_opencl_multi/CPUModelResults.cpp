//
//  CPUModelResults.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 17.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CPUModelResults.hpp"
#include <string.h>

CPUModelResults::CPUModelResults(std::string modelName, cl_uint point_per_trace)
    : _modelName(modelName), _point_per_trace(point_per_trace)

,_meanTrace{}
,_censumTrace{}
,_meanHypot{}
,_censumHypot{}
,_upperPart{}
{
    for (auto &t : {&_meanTrace,&_censumTrace,&_meanHypot,&_censumHypot,&_upperPart}){
        t->trace = (gpu_float_type*)malloc(_point_per_trace*sizeof(gpu_float_type));
        t->cnt = 0;
        memset(t->trace, 0, _point_per_trace*sizeof(gpu_float_type));
    }
}


CPUModelResults::~CPUModelResults(){
    for (auto &t : {&_meanTrace,&_censumTrace,&_meanHypot,&_censumHypot,&_upperPart}){
        free(t->trace);t->trace=NULL;
    }
}
