//
//  CPUModelResults.hpp
//  SNR_opencl_multi
//
//  Created by tihmstar on 27.11.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef CPUModelResults_hpp
#define CPUModelResults_hpp

#include <vector>
#include "numerics.hpp"

class CPUModelResults {
    std::string _modelName;
    cl_uint _modelSize;
    cl_uint _point_per_trace;
    std::vector<combtrace> _groupsMean;
    std::vector<combtrace> _groupsCensum;
public:
    CPUModelResults(std::string modelName, cl_uint modelSize, cl_uint point_per_trace);
    CPUModelResults(const CPUModelResults &cpy) = delete;
    ~CPUModelResults();

    void combineResults(std::vector<combtrace> &retgroupsMean,std::vector<combtrace> &retgroupsCensum);
    
    std::vector<combtrace> &groupsMean();
    std::vector<combtrace> &groupsCensum();

};

#endif /* CPUModelResults_hpp */
