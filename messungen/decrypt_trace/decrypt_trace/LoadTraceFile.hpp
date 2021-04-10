//
//  LoadTraceFile.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef LoadTraceFile_hpp
#define LoadTraceFile_hpp

#include <stdlib.h>
#include <stdint.h>
#include <iostream>


#define AES_BLOCKS 8

class LoadTraceFile {
public:
    struct MeasurementStructure{
        uint32_t    NumberOfPoints;
        uint8_t     Input[AES_BLOCKS*16];
        uint8_t     Output[AES_BLOCKS*16];
        int8_t      Trace[0];
    };
    
    class TracesIterator{
        const MeasurementStructure *_cur;
        const MeasurementStructure *_end;
    public:
        TracesIterator(const MeasurementStructure *ms, const MeasurementStructure *end);
        TracesIterator &operator++();
        bool operator!=(const TracesIterator &e);
        const MeasurementStructure *operator*() const;
    };
private:
    size_t _realFileSize;
    size_t _fsize;
    int _fd;
    int8_t *_fmem;
    std::string _path;

public:
    LoadTraceFile(const char * path,bool doLoad = true);
    ~LoadTraceFile();

    const int8_t *buf() const;
    size_t fsize() const;

    void load();
    
    uint32_t tracesInFile() const;
    uint32_t pointsPerTrace() const;

    std::string path() const{ return _path;}
    
    TracesIterator begin();
    TracesIterator end();
    const MeasurementStructure *operator[](unsigned i);
    
};

#endif /* LoadTraceFile_hpp */
