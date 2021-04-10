//
//  TraceAligner.hpp
//  align_chip_opencl
//
//  Created by tihmstar on 11.01.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef TraceAligner_hpp
#define TraceAligner_hpp

#include <queue>
#include <iostream>
#include <stdint.h>
#include "LoadFile.hpp"

int doNormalize(std::queue<LoadFile *> &intraces, const char *outdir, int threadsCnt, uint32_t point_per_trace, uint32_t align_window_start, uint32_t align_window_size, int move_radius);

#endif /* TraceAligner_hpp */
