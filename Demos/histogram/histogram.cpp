//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: Histogram.cpp
// 
// Implements histogram in C++ AMP
// Refer README.txt
//----------------------------------------------------------------------------

#include "histogram.h"
#include <assert.h>
#include <iostream>

#pragma warning (disable : 4267)

const unsigned histogram_bin_count = 256; // Bin count

const unsigned log2_thread_size = 5U;
const unsigned thread_count = 8; // number of partial histogram per tile

const unsigned histogram256_tile_size = (thread_count * (1U << log2_thread_size));
const unsigned histogram256_tile_static_memory = (thread_count * histogram_bin_count);

const unsigned merge_tile_size = histogram_bin_count; // Partial result Merge size
const unsigned partial_histogram256_count = (thread_count * (1U << log2_thread_size));

// Atomically update bucket count
// This function decodes packed byte size data to identify bucket
void histogram_amp::addword_256(unsigned *s_hist, unsigned offset, unsigned data) restrict (amp)
{
    atomic_fetch_add(&(s_hist[offset + (data >>  0) & 0xFFU]), 1);
    atomic_fetch_add(&(s_hist[offset + (data >>  8) & 0xFFU]), 1);
    atomic_fetch_add(&(s_hist[offset + (data >> 16) & 0xFFU]), 1);
    atomic_fetch_add(&(s_hist[offset + (data >> 24) & 0xFFU]), 1);
}

// This functions divides data among _no_of_tiles*histogram256_tile_size threads.
// And in each tile, threads are grouped by (1 << log2_thread_size) number of threads to update 
// partial histogram count.
template <int _no_of_tiles>
void histogram_amp::histo_kernel(unsigned data_in_uint_count, array<unsigned, 1>& data, array<unsigned, 1>& partial_result)
{
    assert((histogram256_tile_size % (1 << log2_thread_size)) == 0, "Threads in a tile should be grouped equally among bin count");
    assert(histogram256_tile_static_memory == (thread_count * histogram_bin_count), "Shared memory size should be in multiple of bin count and number of threads per tile");

    extent<1> e_compute(_no_of_tiles*histogram256_tile_size);
    parallel_for_each(e_compute.tile<histogram256_tile_size>(), 
        [=, &data, &partial_result] (tiled_index<histogram256_tile_size> tidx) restrict(amp)
        {
            tile_static unsigned s_hist[histogram256_tile_static_memory];

            // initialize shared memory - each thread will ZERO 8 locations
            unsigned per_thread_init = histogram256_tile_static_memory / histogram256_tile_size;
            for(unsigned i = 0; i < per_thread_init; i++)
                s_hist[tidx.local[0] + i * histogram256_tile_size] = 0;
            tidx.barrier.wait();

            // Each group of 32 threads(tidx.local[0] >> log2_thread_size), will update bin count in shared memory
            unsigned offset = (tidx.local[0] >> log2_thread_size) * histogram_bin_count;

            // Entire data is divided for processing between _no_of_tiles*histogram256_tile_size threads
            // There are totally _no_of_tiles tiles with histogram256_tile_size thread in each of them - this is the increment size
            // Consecutive threads do take advantage of memory coalescing
            for(unsigned pos = tidx.global[0]; pos < data_in_uint_count; pos += histogram256_tile_size * partial_histogram256_count)
            {
                unsigned datum = data[pos];
                addword_256(s_hist, offset, datum);
            }
            tidx.barrier.wait();

            for(unsigned bin = tidx.local[0]; bin < histogram_bin_count; bin += histogram256_tile_size)
            {
                unsigned sum = 0;
                // Updated each threads partial sum to partial histogram
                for(unsigned i = 0; i < thread_count; i++)
                    sum += s_hist[bin + i * histogram_bin_count];
                partial_result[tidx.tile[0] * histogram_bin_count + bin] = sum;
            }
        });
}

// This function aggregates partial results
template <int _no_of_tiles>
void histogram_amp::histo_merge_kernel(array<unsigned, 1>& partial_result, array<unsigned, 1>& histogram_amp, unsigned hist_bin_count)
{
    assert((partial_result.get_extent().size() % histogram_bin_count) == 0);
    assert((partial_result.get_extent().size() % _no_of_tiles) == 0, "Bound checking partial histogram array size");

    extent<1> e_compute(hist_bin_count*_no_of_tiles);
    parallel_for_each(e_compute.tile<_no_of_tiles>(),
        [=, &partial_result, &histogram_amp] (tiled_index<_no_of_tiles> tidx) restrict(amp)
        {
            unsigned sum = 0;
            for (unsigned i = tidx.local[0]; i < partial_result.get_extent().size(); i += _no_of_tiles)
            {
                sum += partial_result[tidx.tile[0] + i * histogram_bin_count];
            }

            tile_static unsigned s_data[_no_of_tiles];
            s_data[tidx.local[0]] = sum;

            // parallel reduce within a tile
            for (int stride = _no_of_tiles / 2; stride > 0; stride >>= 1)
            {
                tidx.barrier.wait();
                if (tidx.local[0] < stride)
                {
                    s_data[tidx.local[0]] += s_data[tidx.local[0] + stride];
                }
            }

            // tile sum is updated to result array by zero-th thread
            if (tidx.local[0] == 0)
            {
                histogram_amp[tidx.tile[0]] = s_data[0];
            }
        });
}


// This function generates random input data and initializes
// bin count
histogram::histogram(unsigned bin_count, unsigned data_size_in_mb)
{
    data_type_size = sizeof(user_data_t);
    unsigned byte_count = data_size_in_mb * (1024 * 1024);
    int item_count =  byte_count / data_type_size;

    // This algorithm is tuned to work for 256 bin count
    assert(histogram_bin_count == bin_count, "Current implementation is tuned for 256 bin count");

    data.resize(item_count);
    histo_amp.resize(bin_count);

    assert(item_count > 0, "No data to process");

    // number of relevant data bits in a random number
    unsigned data_mask = (unsigned)(pow(2, data_type_size * 8) - 1);
    srand(2012);
    for (int i = 0; i < item_count; i++)
        data[i] = rand() & data_mask;

    for (unsigned i = 0; i < bin_count; i++)
        histo_amp[i] = 0;
}

histogram::~histogram()
{

}

// This function creates copies of data on accelerator, dispatch 2 kernels, 
// 1st to calculate histogram partially and then another to merge the results.
// Finally copy result from accelerator to host memory
void histogram::run()
{
    unsigned byte_count = data.size() * data_type_size;
    assert(byte_count % sizeof(unsigned) == 0);
    unsigned data_uint_count = byte_count / sizeof(unsigned);

    const unsigned histo_count = histo_amp.size();

    array<unsigned, 1> a_histogram(histo_amp.size());
    // interpret all data as unsigned
    array<unsigned, 1> a_data(byte_count / sizeof(unsigned), (unsigned*)data.data());
    array<unsigned, 1> a_partial(partial_histogram256_count * histo_amp.size());

    // Run kernels to generate partial histogram - stage 1
    histogram_amp::histo_kernel<partial_histogram256_count>(data_uint_count, a_data, a_partial);
    // Run kernel to merge partial histograms - stage 2
    histogram_amp::histo_merge_kernel<merge_tile_size>(a_partial, a_histogram, histo_amp.size());

    copy(a_histogram, histo_amp.begin());
}

// This function computes histogram on CPU (as a baseline) and compare with 
// GPU results. If there is any error printf out the first mismatched result
bool histogram::verify()
{
    std::vector<unsigned> histo_cpu(histo_amp.size());

    for (unsigned i = 0; i < histo_cpu.size(); i++)
        histo_cpu[i] = 0;

    // Number of higher order bits used to calculate bucket
    unsigned data_shift = (data_type_size * 8) - log2(histo_amp.size());

    // histogram on cpu
    for (unsigned i = 0; i < data.size(); i++)
    {
        histo_cpu[(data[i] >> data_shift)]++;
    }

    // verify data
    for (unsigned i = 0; i < histo_cpu.size(); i++)
    {
        if (histo_amp[i] != histo_cpu[i])
        {
            std::cout << i << ": " << histo_amp[i] << " <> " << histo_cpu[i] << std::endl;
            std::cout << "***HISTOGRAM " << histo_amp.size() << " VERIFICATION FAILURE***" << std::endl;
            return false;
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

    // With larger data size there will be better performance, use data size just 
    // enough to amortize copy cost. 
    // For simplicity using 1MB of data
    const int data_size_in_mb = 1;

    histogram _histo_char(histogram_bin_count, data_size_in_mb);
    _histo_char.run();
    _histo_char.verify();
}

