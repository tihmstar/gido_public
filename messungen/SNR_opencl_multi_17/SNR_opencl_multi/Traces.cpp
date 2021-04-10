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
#include <signal.h>
#include <archive.h>
#include <archive_entry.h>
#include <sys/errno.h>


#define assure(cond) do {if (!(cond)) {printf("ERROR: ASSURE FAILED IN Traces.cpp ON LINE=%d\n",__LINE__); raise(SIGABRT); exit(-1);}}while (0)

#ifdef DEBUG
#define dbg_assure(cond) assure(cond)
#else
#define dbg_assure(cond) //
#endif

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
               
const int8_t *Traces::buf(){
   return _fmem;
}
               
size_t Traces::bufSize(){
   return _fsize;
}

uint32_t Traces::tracesInFile() const{
    return *(uint32_t*)_fmem;
}

uint32_t Traces::pointsPerTrace() const{
    return ((uint32_t*)_fmem)[1];
}
