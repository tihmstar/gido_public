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
    size_t _fsize;
    int _fd;
    int8_t *_fmem;
public:
    Traces(const char *path, size_t minSize = 0);
    ~Traces();
    
    const int8_t *buf();
    size_t bufSize();

    uint32_t tracesInFile() const;
    uint32_t pointsPerTrace() const;
};

#endif /* Traces_hpp */
