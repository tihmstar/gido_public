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
#include <iostream>
#include <signal.h>

#include <archive.h>
#include <archive_entry.h>
#include <string.h>


#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN Traces.cpp ON LINE=%d for trace=%s\n",__LINE__,_path.c_str()); raise(SIGABRT); exit(-1);}}while (0)


using namespace std;

#pragma mark TracesIterator

Traces::TracesIterator::TracesIterator(const MeasurementStructure *ms, const MeasurementStructure *end)
: _cur(ms), _end(end)
{
    //
}

Traces::TracesIterator &Traces::TracesIterator::operator++(){
    _cur = (MeasurementStructure*)( ((uint8_t*)_cur)+_cur->NumberOfPoints + sizeof(MeasurementStructure));
    return *this;
}

bool Traces::TracesIterator::operator!=(const TracesIterator &e){
   return _cur != e._cur;
}

const Traces::MeasurementStructure *Traces::TracesIterator::operator*() const{
    return _cur;
}


#pragma mark Traces
               
Traces::Traces(const char* path, bool doLoad)
 : _fmem(NULL), _path(path)
{
    struct stat sb;
               
    if (_path.size() > sizeof(".tar.gz") && _path.substr(_path.size()-sizeof(".tar.gz")+1) == ".tar.gz"){
       _path = _path.substr(0,_path.size()-sizeof(".tar.gz")+1);
    }
    
    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    
    _realFileSize = _fsize = sb.st_size;
    
    //page align for mmap
    _fsize &= ~0x3fff;
    _fsize += 0x4000;

    if (doLoad) {
        load();
    }
}

void Traces::load(){
    if (!_fmem) {
        //mmap, but don't map the file
        assure((_fmem = (int8_t*)mmap(NULL, _fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)) != (void*)-1);
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

                    la_ssize_t s = archive_read_data(archive, archiveMem, archiveEntrySize);
                    assure(s == archiveEntrySize);
                    
                    
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

Traces::~Traces(){
    if (_fmem) {
        munmap(_fmem, _fsize);
    }
    if (_fd > 0){
        close(_fd);
    }
}

Traces::TracesIterator Traces::begin(){
    return {(*this)[0],(*this)[size()]};
}

Traces::TracesIterator Traces::end(){
    return {(*this)[size()],(*this)[size()]};
}

uint32_t Traces::size() const{
    return *(uint32_t*)_fmem;
}

const Traces::MeasurementStructure *Traces::operator[](unsigned i){
    return (Traces::MeasurementStructure *)(&_fmem[4+i*(sizeof(MeasurementStructure) + *(uint32_t*)&_fmem[4])]);
}

