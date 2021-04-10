//
//  CPUModelResults.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 17.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "CPUModelResults.hpp"
#include <string.h>
#include <netinet/in.h>

#define assure(cond) do {if (!(cond)) {printf("ERROR: CPUModelResults ASSURE FAILED ON LINE=%d\n",__LINE__); throw(__LINE__);}}while (0)


CPUModelResults::CPUModelResults(std::string modelName, cl_uint point_per_trace)
    : _modelName(modelName), _point_per_trace(point_per_trace)

,_meanTrace{}
,_censumTrace{}
,_meanHypot{}
,_censumHypot{}
,_upperPart{}
{
    for (auto &t : {&_meanTrace,&_censumTrace,&_meanHypot,&_censumHypot,&_upperPart}){
        t->trace = (double*)malloc(_point_per_trace*sizeof(double));
        t->cnt = 0;
        memset(t->trace, 0, _point_per_trace*sizeof(double));
    }
}

CPUModelResults::CPUModelResults(const CPUModelResults &cpy)
    : _modelName(cpy._modelName), _point_per_trace(cpy._point_per_trace)

,_meanTrace{}
,_censumTrace{}
,_meanHypot{}
,_censumHypot{}
,_upperPart{}
{
    for (auto &t : {&_meanTrace,&_censumTrace,&_meanHypot,&_censumHypot,&_upperPart}){
        t->trace = (double*)malloc(_point_per_trace*sizeof(double));
    }

    memcpy(_meanTrace.trace, cpy._meanTrace.trace, _point_per_trace*sizeof(double));
    _meanTrace.cnt = cpy._meanTrace.cnt;

    memcpy(_censumTrace.trace, cpy._censumTrace.trace, _point_per_trace*sizeof(double));
    _censumTrace.cnt = cpy._censumTrace.cnt;

    memcpy(_meanHypot.trace, cpy._meanHypot.trace, _point_per_trace*sizeof(double));
    _meanHypot.cnt = cpy._meanHypot.cnt;

    memcpy(_censumHypot.trace, cpy._censumHypot.trace, _point_per_trace*sizeof(double));
    _censumHypot.cnt = cpy._censumHypot.cnt;

    memcpy(_upperPart.trace, cpy._upperPart.trace, _point_per_trace*sizeof(double));
    _upperPart.cnt = cpy._upperPart.cnt;
}

CPUModelResults::CPUModelResults(void *data, size_t dataSize)
: _modelName{}, _point_per_trace(0)

,_meanTrace{}
,_censumTrace{}
,_meanHypot{}
,_censumHypot{}
,_upperPart{}
{
    uint8_t *buf = (uint8_t*)data;
    uint8_t *bufWrk = buf;

    auto traces = {&_meanTrace,&_censumTrace,&_meanHypot,&_censumHypot,&_upperPart};

    _point_per_trace = htonl(*(uint32_t*)bufWrk); bufWrk+=sizeof(_point_per_trace);

    for (auto &t : traces){
        t->trace = (double*)malloc(_point_per_trace*sizeof(double));
        double *dtmp = (double*)bufWrk;
        for (int i=0; i<_point_per_trace; i++) {
            t->trace[i] = *dtmp++;
        }
        bufWrk = (uint8_t*)dtmp;
        t->cnt = htonl(*(uint32_t*)bufWrk); bufWrk+=sizeof(t->cnt);
    }
    assure(bufWrk-buf == dataSize);
}

CPUModelResults::~CPUModelResults(){
    for (auto &t : {&_meanTrace,&_censumTrace,&_meanHypot,&_censumHypot,&_upperPart}){
        free(t->trace);t->trace=NULL;
    }
}

void CPUModelResults::serialize(void **outBuffer, size_t *outBufferSize){
    //always use double in serialization
    uint8_t *buf = NULL;
    size_t bufSize = 0;
    
    auto traces = {&_meanTrace,&_censumTrace,&_meanHypot,&_censumHypot,&_upperPart};
    
    bufSize = sizeof(_point_per_trace) + (_point_per_trace*sizeof(double) + sizeof((*traces.begin())->cnt)) * traces.size();
    buf = (uint8_t *)malloc(bufSize);
    
    uint8_t *bufWrk = buf;
    *(uint32_t*)bufWrk = htonl(_point_per_trace); bufWrk+=sizeof(_point_per_trace);
    
    for (auto &t : traces){
        double *dtmp = (double*)bufWrk;
        for (int i=0; i<_point_per_trace; i++) {
            *dtmp++ = t->trace[i];
        }
        bufWrk = (uint8_t*)dtmp;
        *(uint32_t*)bufWrk = htonl(t->cnt); bufWrk+=sizeof(t->cnt);
    }
    assure(bufWrk-buf == bufSize);
    
    *outBuffer = buf; buf = NULL;
    *outBufferSize = bufSize;
}
