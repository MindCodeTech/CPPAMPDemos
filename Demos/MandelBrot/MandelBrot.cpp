//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: MandelBrot.cpp
// 
// Implements Mandel Brot sample in C++ AMP
//----------------------------------------------------------------------------

#include "mandelbrot.h"
#include <math.h>
#include <assert.h>
#include <iostream>

mandelbrot::mandelbrot()
{
    int size_1d = (int)sqrt(DEFAULT_DATA_SIZE);

    assert((DEFAULT_DATA_SIZE == size_1d*size_1d), "Data size should be a square number");
    data.resize(DEFAULT_DATA_SIZE);

    iterations = DEFAULT_ITERATIONS;
    num_of_tiles = DEFAULT_NUM_TILES;
}

mandelbrot::mandelbrot(int _data_size, int _iterations, int _num_tiles)
{
    int size_1d = (int)sqrt(_data_size);

    assert((_data_size == size_1d*size_1d), "Data size should be a square number");
    data.resize(_data_size);

    iterations = _iterations;
    num_of_tiles = _num_tiles;
}

mandelbrot::~mandelbrot()
{

}

// Core mandelbrot function
unsigned mandelbrot::mandelbrot_calc(int iterations, float y0, float x0) restrict(cpu,amp)
{
    float y = 0.0f;
    float x = 0.0f;
    float yy = 0.0f;
    float xx = 0.0f;
    int iteration = 0;
    while ((xx + yy <= 4.0f) & (iteration < iterations))
    {
        y = 2.0f * x * y + y0;
        x = xx - yy + x0;
        yy = y * y;
        xx = x * x;
        iteration += 1;
    }
    // This also means for a point (x0, y0), color can be derived from "iteration" value
    return (unsigned)iteration;
}

void mandelbrot::execute()
{
    int iters = iterations;
    int tiles = num_of_tiles;

    int size_1d = (int)sqrt(data.size());

    array<int, 1> count(1);
    array<unsigned, 2> a_data(size_1d, size_1d);
    int zero = 0;

    int max_chunks = (size_1d * size_1d) / (TILE_SIZE * TILE_SIZE);

    float yscale = YSIZE / (float)size_1d;
    float xscale = XSIZE / (float)size_1d;


    parallel_for_each(extent<2>(TILE_SIZE, tiles*TILE_SIZE).tile<TILE_SIZE, TILE_SIZE>(),
        [=, &a_data, &count] (tiled_index<TILE_SIZE, TILE_SIZE> tidx) restrict(amp)
        {
            tile_static int chunk_id;
            tile_static int global_y;
            tile_static int global_x;

            // Here each tile will process a chuck of data and pick next block to process
            // This is like load balancing computation, a tile will pick next available chunk to process
            // "chunk_id" value in a tile will determine which chunk of data is being processed by this tile
            while (1)
            {
                // All threads from previous iteration sync here
                tidx.barrier.wait();
                if (tidx.local[1] == 0 && tidx.local[0] == 0)
                {
                    // Sync-ing chuck to be processed between tiles
                    // get chunk to process for this tile
                    chunk_id = atomic_fetch_add(&count[0], 1);
                    global_y = chunk_id / (size_1d / TILE_SIZE) * TILE_SIZE;
                    global_x = chunk_id % (size_1d / TILE_SIZE) * TILE_SIZE;
                }
                // Sync within a tile.
                // Now threads have tile specific chunk_id, global_y, and global_x
                tidx.barrier.wait();

                if (chunk_id >= max_chunks) break; 

                // calculate Mandelbrot for scaled coordinate of pixel 
                float y0 = (global_y + tidx.local[0]) * yscale + YLOW;
                float x0 = (global_x + tidx.local[1]) * xscale + XLOW;
                a_data(global_y + tidx.local[0], global_x + tidx.local[1]) =
                    mandelbrot_calc(iters, y0, x0);
            }
        });

    copy(a_data, data.begin());
}

bool mandelbrot::verify()
{
    unsigned size_1d = (unsigned)sqrt(data.size());

    for (unsigned i = 0; i < size_1d; i++)
    {
        float y0 = i * (YSIZE / size_1d) + YLOW;
        for (unsigned j = 0; j < size_1d; j++)
        {
            float x0 = j * (XSIZE / size_1d) + XLOW;
            if (data[i*size_1d+j] != mandelbrot_calc(iterations, y0, x0))
            {
                std::cout << "***Mandelbrot VERIFICATION FAILURE***" << std::endl;
                return false;
            }
        }
    }

    return true;
}

int main()
{
    accelerator default_device;
    std::wcout << L"Using device : " << default_device.get_description() << std::endl;
    if (default_device == accelerator(accelerator::direct3d_ref))
        std::cout << "WARNING!! Running on very slow emulator! Only use this accelerator for debugging." << std::endl;

    mandelbrot mb_def_demo;
    mb_def_demo.execute();
    mb_def_demo.verify();

    return 0;
}

