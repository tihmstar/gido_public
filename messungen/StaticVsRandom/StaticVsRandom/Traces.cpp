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
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>


#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN Traces.cpp ON LINE=%d\n",__LINE__); throw __LINE__;}}while (0)

#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#else
#define dbg_assure(cond) //
#endif

uint8_t roundkeys[11][16] =
{
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
    {0xd6, 0xaa, 0x74, 0xfd, 0xd2, 0xaf, 0x72, 0xfa, 0xda, 0xa6, 0x78, 0xf1, 0xd6, 0xab, 0x76, 0xfe},
    {0xb6, 0x92, 0xcf, 0x0b, 0x64, 0x3d, 0xbd, 0xf1, 0xbe, 0x9b, 0xc5, 0x00, 0x68, 0x30, 0xb3, 0xfe},
    {0xb6, 0xff, 0x74, 0x4e, 0xd2, 0xc2, 0xc9, 0xbf, 0x6c, 0x59, 0x0c, 0xbf, 0x04, 0x69, 0xbf, 0x41},
    {0x47, 0xf7, 0xf7, 0xbc, 0x95, 0x35, 0x3e, 0x03, 0xf9, 0x6c, 0x32, 0xbc, 0xfd, 0x05, 0x8d, 0xfd},
    {0x3c, 0xaa, 0xa3, 0xe8, 0xa9, 0x9f, 0x9d, 0xeb, 0x50, 0xf3, 0xaf, 0x57, 0xad, 0xf6, 0x22, 0xaa},
    {0x5e, 0x39, 0x0f, 0x7d, 0xf7, 0xa6, 0x92, 0x96, 0xa7, 0x55, 0x3d, 0xc1, 0x0a, 0xa3, 0x1f, 0x6b},
    {0x14, 0xf9, 0x70, 0x1a, 0xe3, 0x5f, 0xe2, 0x8c, 0x44, 0x0a, 0xdf, 0x4d, 0x4e, 0xa9, 0xc0, 0x26},
    {0x47, 0x43, 0x87, 0x35, 0xa4, 0x1c, 0x65, 0xb9, 0xe0, 0x16, 0xba, 0xf4, 0xae, 0xbf, 0x7a, 0xd2},
    {0x54, 0x99, 0x32, 0xd1, 0xf0, 0x85, 0x57, 0x68, 0x10, 0x93, 0xed, 0x9c, 0xbe, 0x2c, 0x97, 0x4e},
    {0x13, 0x11, 0x1d, 0x7f, 0xe3, 0x94, 0x4a, 0x17, 0xf3, 0x07, 0xa7, 0x8b, 0x4d, 0x2b, 0x30, 0xc5}
};

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

Traces::Traces(const char * path)
: _fd(-1),_fmem(NULL)
{
    struct stat sb;

    assure((_fd = open(path, O_RDONLY)) > 0);
    assure(!fstat(_fd, &sb));
    _realFileSize = _fsize = sb.st_size;
    
    
    //page align for mmap
    _fsize &= ~0x3fff;
    _fsize += 0x4000;
    
    load();
}

Traces::~Traces(){
    if (_fmem) {
        munmap(_fmem,_fsize);
    }
    if (_fd > 0){
        close(_fd);
    }
}

void Traces::load(){
    if (!_fmem) {
        //mmap, but don't map the file
        if ((_fmem = (int8_t*)mmap(NULL, _fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)) == (void*)-1){
            printf("failed to mmap of size=%lu errno=%d (%s)\n",_fsize,errno,strerror(errno));
            assure(0);
        }

        {
            assure(read(_fd, _fmem, _realFileSize) == _realFileSize);
        }
        
        
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

                    la_ssize_t s = archive_read_data(archive, archiveMem, archiveEntrySize);
                    assure(s == archiveEntrySize);
                    
                    assure(!munmap(_fmem,_fsize));

                    _fmem = archiveMem;archiveMem = NULL;
                    _realFileSize = _fsize = archiveEntrySize;
                    archiveEntrySize = 0;
                    break;
                }
                archive_read_data_skip(archive);
            }
        }
        archive_read_free(archive);
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
    return memcmp(ms->Input, roundkeys, sizeof(ms->Input)) == 0;
}
