//
//  LoadTraceFile.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "LoadTraceFile.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include <sys/errno.h>

#include <atomic>
#include <mutex>

static std::atomic<ssize_t> gmaxRamAlloc{(ssize_t)6e9};
static std::mutex gmaxRamModLock;
static std::mutex gmaxRamModEventLock;

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN LoadTraceFile.cpp ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#pragma mark TracesIterator

LoadTraceFile::TracesIterator::TracesIterator(const MeasurementStructure *ms, const MeasurementStructure *end)
: _cur(ms), _end(end)
{
    //
}

LoadTraceFile::TracesIterator &LoadTraceFile::TracesIterator::operator++(){
    _cur = (MeasurementStructure*)( ((uint8_t*)_cur)+_cur->NumberOfPoints + sizeof(MeasurementStructure));
    return *this;
}

bool LoadTraceFile::TracesIterator::operator!=(const TracesIterator &e){
   return _cur != e._cur;
}

const LoadTraceFile::MeasurementStructure *LoadTraceFile::TracesIterator::operator*() const{
    return _cur;
}


#pragma mark Traces

LoadTraceFile::LoadTraceFile(const char * path, bool doLoad)
: _fmem(NULL), _path(path)
{
    struct stat sb;
    
    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    
    _realFileSize = _fsize = sb.st_size;
    
    //page align for mmap
    _fsize &= ~0x3fff;
    _fsize += 0x4000;
    
    gmaxRamModLock.lock();
    while (gmaxRamAlloc.fetch_sub(_fsize) < _fsize) {//access happens while we have lock
        gmaxRamAlloc.fetch_add(_fsize);
        gmaxRamModLock.unlock();//release lock befor hang

        printf("[%p] hanging until free event\n",this);
        gmaxRamModEventLock.lock(); //hang here
        
        gmaxRamModLock.lock();//re-grab lock before re-read
    }
    gmaxRamModLock.unlock(); //release lock
    gmaxRamModEventLock.unlock(); //share memory update event
    
    
    if (doLoad) {
        load();
    }
}

LoadTraceFile::~LoadTraceFile(){
    if (_fmem) {
//        munlock(_fmem, _fsize);
        munmap(_fmem,_fsize);
    }
    if (_fd > 0){
        close(_fd);
    }
    gmaxRamAlloc.fetch_add(_fsize);
    gmaxRamModEventLock.unlock(); //send free event
}

void LoadTraceFile::load(){
    if (!_fmem) {
        //mmap, but don't map the file
        if ((_fmem = (int8_t*)mmap(NULL, _fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)) == (void*)-1){
            printf("failed to mmap of size=%lu errno=%d (%s)\n",_fsize,errno,strerror(errno));
            assure(0);
        }
    //    mlock(_fmem, _fsize); //don't care if this fails
        assure(read(_fd, _fmem, _realFileSize) == _realFileSize);
        
        struct archive *archive = NULL;
        struct archive_entry *entry = NULL;
        int8_t *archiveMem = NULL;
        int r = 0;
        
        archive = archive_read_new();
        archive_read_support_compression_gzip(archive);
        archive_read_support_format_tar(archive);
        
        r = archive_read_open_memory(archive, _fmem, _realFileSize);
        
        if (r == ARCHIVE_OK){
            //this is an archive
            while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
                const char *pathname = archive_entry_pathname(entry);
                size_t pathnameLen = strlen(pathname);
                if (pathnameLen >= sizeof(".dat")-1 && strstr(pathname,".dat")){//somehow fucked up the compression script so files are .dat.tar.gz inside the tar.gz o.O
                    size_t archiveEntrySize = 0;
                    
                    archiveEntrySize = archive_entry_size(entry);
                    
                    archiveMem = (int8_t*)mmap(NULL, archiveEntrySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
                    gmaxRamAlloc.fetch_sub(archiveEntrySize); //we alloced more memory, let others know

                    la_ssize_t s = archive_read_data(archive, archiveMem, archiveEntrySize);
                    assure(s == archiveEntrySize);
                    
                    gmaxRamAlloc.fetch_add(_fsize); //give back the memory we alloced before
                    
                    assure(!munmap(_fmem,_fsize));
                    _fmem = archiveMem;archiveMem = NULL;
                    _fsize = archiveEntrySize; archiveEntrySize = 0;
                    break;
                }
                archive_read_data_skip(archive);
            }
        }
        archive_read_free(archive);
    }
}

const int8_t *LoadTraceFile::buf() const{
    assure(_fmem);
    return _fmem;
}

size_t LoadTraceFile::fsize() const{
    return _fsize;
}

uint32_t LoadTraceFile::tracesInFile() const{
    return *(uint32_t*)_fmem;
}

uint32_t LoadTraceFile::pointsPerTrace() const{
    return ((uint32_t*)_fmem)[1];
}

LoadTraceFile::TracesIterator LoadTraceFile::begin(){
    return {(*this)[0],(*this)[tracesInFile()]};
}

LoadTraceFile::TracesIterator LoadTraceFile::end(){
    return {(*this)[tracesInFile()],(*this)[tracesInFile()]};
}


const LoadTraceFile::MeasurementStructure *LoadTraceFile::operator[](unsigned i){
    return (LoadTraceFile::MeasurementStructure *)(&_fmem[4+i*(sizeof(MeasurementStructure) + *(uint32_t*)&_fmem[4])]);
}
