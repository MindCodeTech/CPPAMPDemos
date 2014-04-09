//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: Histogram.h
// 
// Refer README.txt
//----------------------------------------------------------------------------

#pragma once
#include <cmath>
#include <amp.h>
#include <vector>

using namespace concurrency;

typedef unsigned char user_data_t;

// This class member method implements C++ AMP kernels
// and its supporting routine
class histogram_amp
{
public:
    template <int _no_of_tiles>
    static void histo_kernel(unsigned data_count, array<unsigned, 1>& data, array<unsigned, 1>& partial_result);
    template <int _no_of_tiles>
    static void histo_merge_kernel(array<unsigned, 1>& partial_result, array<unsigned, 1>& histogram_amp, unsigned tile_count);
};

class histogram
{
public:
    histogram(unsigned bin_count, unsigned data_size_in_mb);
    ~histogram();
    void run();
    bool verify();
    
private:
    histogram()
    {
        data_type_size = 0;
    }
    inline unsigned log2(unsigned data)
    {
        return (unsigned)(log(data)/log(2)); 
    }

    std::vector<user_data_t> data;
    int data_type_size;
    std::vector<unsigned> histo_amp;
};

