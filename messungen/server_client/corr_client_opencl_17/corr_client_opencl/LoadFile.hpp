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

class LoadFile {
    size_t _realFileSize;
    size_t _fsize;
    int _fd;
    int8_t *_fmem;
    std::string _path;
    

public:
    LoadFile(const char * path,bool doLoad = true);
    ~LoadFile();

    const int8_t *buf() const;
    size_t size() const;

    void load();
    
    uint32_t tracesInFile() const;
    uint32_t pointsPerTrace() const;

    LoadFile &operator+=(const LoadFile &toAdd);
    
    
    friend size_t CurlDownloadCallback(void *contents, size_t size, size_t nmemb, void *userp);
};

#endif /* LoadFile_hpp */
