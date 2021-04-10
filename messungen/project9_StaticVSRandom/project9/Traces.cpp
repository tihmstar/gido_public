//
//  Traces.cpp
//  normalize
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "Traces.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

#define assure(cond) do {if (!(cond)) throw __LINE__;}while (0)

#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#else
#define dbg_assure(cond) //
#endif

using namespace std;

uint8_t FixedText[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };


#pragma mark TracesIterator

Traces::TracesIterator::TracesIterator(const MeasurementStructure *ms, const MeasurementStructure *end)
: _cur(ms), _end(end)
{
    //
}

Traces::TracesIterator &Traces::TracesIterator::operator++(){
    _cur = (MeasurementStructure*)( ((uint8_t*)_cur)+_cur->NumberOfPoints + sizeof(MeasurementStructure));
    dbg_assure(_cur<=_end);
    return *this;
}

bool Traces::TracesIterator::operator!=(const TracesIterator &e){
   return _cur != e._cur;
}

const Traces::MeasurementStructure *Traces::TracesIterator::operator*() const{
    return _cur;
}


#pragma mark Traces

Traces::Traces(const char* path){
    struct stat sb;
    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    assure((_fmem = (int8_t*)mmap(NULL, _fsize = sb.st_size, PROT_READ, MAP_PRIVATE | MCL_CURRENT, _fd, 0)) != (void*)-1);

    mlock(_fmem, _fsize); //don't care if this fails
}

Traces::~Traces(){
    if (_fmem) {
        munlock(_fmem, _fsize);
        munmap(_fmem, _fsize);
    }
    if (_fd > 0){
        close(_fd);
    }
}

Traces::TracesIterator Traces::begin(){
    return {(*this)[0],(*this)[tracesInFile()]};
}

Traces::TracesIterator Traces::end(){
    return {(*this)[tracesInFile()],(*this)[tracesInFile()]};
}

uint32_t Traces::tracesInFile() const{
    return *(uint32_t*)_fmem;
}

uint32_t Traces::pointsPerTrace() const{
    return (*this)[0]->NumberOfPoints;
}

const Traces::MeasurementStructure *Traces::operator[](unsigned i) const{
    return (Traces::MeasurementStructure *)(&_fmem[4+i*(sizeof(MeasurementStructure) + *(uint32_t*)&_fmem[4])]);
}

#pragma mark static

bool Traces::isFixed(const MeasurementStructure *ms){
    return memcmp(&ms->Trace[ms->NumberOfPoints], FixedText, sizeof(FixedText)) == 0;
}
