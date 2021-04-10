//
//  Traces.hpp
//  normalize
//
//  Created by tihmstar on 31.10.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef Traces_hpp
#define Traces_hpp

#include <stdint.h>
#include <unistd.h>
#include <iostream>

#define AES_BLOCKS 8

class Traces {
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
    Traces(const char *path, bool doLoad = true);
    ~Traces();

    TracesIterator begin();
    TracesIterator end();

    void load();
    
    uint32_t size() const;
    const MeasurementStructure *operator[](unsigned i);
    
};

#endif /* Traces_hpp */
