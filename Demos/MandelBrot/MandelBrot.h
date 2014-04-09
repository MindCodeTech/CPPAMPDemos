//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: MandelBrot.h
// 
// Refer README.txt
//----------------------------------------------------------------------------

#pragma once
#include <amp.h>

#define TILE_SIZE       16
#define XLOW            (-2.5f)
#define XSIZE           (3.5f)
#define YLOW            (-1.0f)
#define YSIZE           (2.0f)

#define DEFAULT_DATA_SIZE   (1280*1280)
#define DEFAULT_ITERATIONS   1000
#define DEFAULT_NUM_TILES   512

using namespace concurrency;

class mandelbrot
{
public:
    mandelbrot();
    mandelbrot(int _data_size, int _iterations, int _num_tiles);
    void execute();
    bool verify();
    ~mandelbrot();

private:
    static unsigned mandelbrot_calc(int iterations, float y0, float x0) restrict(cpu,amp);

    std::vector<unsigned> data;
    unsigned iterations;
    unsigned num_of_tiles;
};

