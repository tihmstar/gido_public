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

class Traces {
public:
    struct MeasurementStructure{
        uint32_t    NumberOfPoints;
        uint8_t     Input[128];
        uint8_t     Output[128];
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
    size_t _fsize;
    size_t _realFileSize;
    int _fd;
    int8_t *_fmem;
    
    void load();
    
public:
    Traces(const char *path);
    ~Traces();

    TracesIterator begin();
    TracesIterator end();
    

    uint32_t tracesInFile() const;
    uint32_t pointsPerTrace() const;
    const MeasurementStructure *operator[](unsigned i) const;
    
    static bool isFixed(const MeasurementStructure *ms);
    
};

#endif /* Traces_hpp */
