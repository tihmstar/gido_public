//
//  LoadFile.cpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "LoadFile.hpp"
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

#include <atomic>
#include <mutex>

static std::atomic<ssize_t> gmaxRamAlloc{(ssize_t)20e9};
static std::mutex gmaxRamModLock;
static std::mutex gmaxRamModEventLock;


inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN LoadFile.cpp ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#pragma mark TracesIterator

LoadFile::TracesIterator::TracesIterator(const MeasurementStructure *ms, const MeasurementStructure *end)
: _cur(ms), _end(end)
{
    //
}

LoadFile::TracesIterator &LoadFile::TracesIterator::operator++(){
    _cur = (MeasurementStructure*)( ((uint8_t*)_cur)+_cur->NumberOfPoints + sizeof(MeasurementStructure));
    return *this;
}

bool LoadFile::TracesIterator::operator!=(const TracesIterator &e){
   return _cur != e._cur;
}

const LoadFile::MeasurementStructure *LoadFile::TracesIterator::operator*() const{
    return _cur;
}


#pragma mark LoadFile

LoadFile::LoadFile(const char * path, size_t minAllocSize, bool doLoad)
: _fmem(NULL), _path(path)
{
    struct stat sb;
    
    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    
    _realFileSize = _fsize = sb.st_size;
    
    if (minAllocSize < 0x4000) {
        minAllocSize = 0x4000; //16k page size
    }
    
    if (_fsize < minAllocSize) { //min alloc size
        _fsize = minAllocSize;
    }
    
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

LoadFile::~LoadFile(){
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

void LoadFile::load(){
    if (!_fmem) {
        //mmap, but don't map the file
        assure((_fmem = (int8_t*)mmap(NULL, _fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)) != (void*)-1);
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
                if (pathnameLen >= sizeof(".dat")-1 && strcmp(pathname+pathnameLen-sizeof(".dat")+1, ".dat") == 0){
                    size_t archiveEntrySize = 0;
                    
                    archiveEntrySize = archive_entry_size(entry);
                    
                    archiveMem = (int8_t*)mmap(NULL, archiveEntrySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
                    gmaxRamAlloc.fetch_sub(archiveEntrySize); //we alloced more memory, let others know

                    la_ssize_t s = archive_read_data(archive, archiveMem, archiveEntrySize);
                    assure(s == archiveEntrySize);
                    
                    gmaxRamAlloc.fetch_add(_fsize); //give back the memory we alloced before
                    
                    munmap(_fmem,_fsize);
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

std::string LoadFile::path(){
    if (ends_with(_path, ".tar.gz")){
        return _path.substr(0,_path.size()-sizeof(".tar.gz")+1);
    }
    return _path;
}

const int8_t *LoadFile::buf() const{
    assure(_fmem);
    return _fmem;
}

size_t LoadFile::size() const{
    return _fsize;
}

uint32_t LoadFile::tracesInFile() const{
    return *(uint32_t*)_fmem;
}

uint32_t LoadFile::pointsPerTrace() const{
    return ((uint32_t*)_fmem)[1];
}

LoadFile::TracesIterator LoadFile::begin(){
    return {(*this)[0],(*this)[tracesInFile()]};
}

LoadFile::TracesIterator LoadFile::end(){
    return {(*this)[tracesInFile()],(*this)[tracesInFile()]};
}

const LoadFile::MeasurementStructure *LoadFile::operator[](unsigned i){
    return (LoadFile::MeasurementStructure *)(&_fmem[4+i*(sizeof(MeasurementStructure) + *(uint32_t*)&_fmem[4])]);
}
