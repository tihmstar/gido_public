//
//  LoadFile.hpp
//  corr_opencl_multi
//
//  Created by tihmstar on 13.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef LoadFile_hpp
#define LoadFile_hpp

#include <stdlib.h>
#include <stdint.h>
#include <iostream>

#define AES_BLOCKS_TOTAL 4 //4*16 = 64bytes

class LoadFile {
public:
    struct MeasurementStructure{
        uint32_t    NumberOfPoints;
        uint8_t   Plaintext[AES_BLOCKS_TOTAL*16];
        uint8_t   Ciphertext[AES_BLOCKS_TOTAL*16];
        int8_t    Trace[0];
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
    LoadFile(const char * path, size_t minAllocSize = 0, bool doLoad = true);
    ~LoadFile();

    const int8_t *buf() const;
    size_t size() const;

    void load();
    
    std::string path();
    
    uint32_t tracesInFile() const;
    uint32_t pointsPerTrace() const;

    size_t numTraces() const;
    TracesIterator begin();
    TracesIterator end();
    const MeasurementStructure *operator[](unsigned i);    
};

#endif /* LoadFile_hpp */
