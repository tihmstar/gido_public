//
//  ResArray.hpp
//  chipplot
//
//  Created by tihmstar on 23.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef ResArray_hpp
#define ResArray_hpp

#include <stdint.h>
#include <unistd.h>

class ResArray{
    size_t _fsize;
    int _fd;
    double *_fmem;

public:
    ResArray(const char *path);
    ResArray(size_t nop);
    ~ResArray();

    double operator[](unsigned i) const;

    size_t size();
    
};

#endif /* ResArray_hpp */
