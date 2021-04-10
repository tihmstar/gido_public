//
//  myfile.hpp
//  corr_server
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef myfile_hpp
#define myfile_hpp

#include <stdint.h>
#include <unistd.h>

class myfile{
    int _fd;
    double *_mem;
    size_t _size;
public:
    myfile(const char *fname, size_t size);
    ~myfile();
    double &operator[](int at);
};

#endif /* myfile_hpp */
