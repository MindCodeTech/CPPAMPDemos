#include "scan.h"

#include <assert.h>
#include <iostream>

using namespace concurrency;

// Compute prefix of prefix
template <int _tile_size, typename _type>
void prefix_scan(array_view<_type> a)
{
    array<_type> atemp(a.extent);
    scan_tiled<_tile_size>(array_view<const _type>(a), array_view<_type>(atemp));
    copy(atemp, a);
}

template <int _tile_size, typename _type>
void scan_tiled(array_view<const _type> input, array_view<_type> output)
{
    int sz = input.extent[0];
    int number_of_tiles = (sz+_tile_size-1)/_tile_size;

    // Compute tile-wise scans and reductions
    array<_type> scan_of_sums_of_tiles(number_of_tiles);
    compute_tilewise_scan<_tile_size>(array_view<const _type>(input), array_view<_type>(output), array_view<_type>(scan_of_sums_of_tiles));

    // recurse if necessary
    if (number_of_tiles >  1)
    {
        prefix_scan<_tile_size>(array_view<_type>(scan_of_sums_of_tiles));

        if (sz > 0) 
        {
            parallel_for_each(extent<1>(sz), [=, &scan_of_sums_of_tiles] (concurrency::index<1> idx) restrict (amp) 
            {
                int my_tile = idx[0]/_tile_size;
                if (my_tile == 0)
                    output[idx] = output[idx];
                else
                    output[idx] = scan_of_sums_of_tiles[my_tile-1] + output[idx];
            });
        }
    }
}

// Calculate prefix sum for a tile
template <int _tile_size, typename _type>
void compute_tilewise_scan(array_view<const _type> input, array_view<_type> tilewise_scans, array_view<_type> sums_of_tiles)
{
    int sz = input.extent[0];
    int number_of_tiles = (sz+_tile_size-1)/_tile_size;
    int number_of_threads = number_of_tiles * _tile_size;
    
    parallel_for_each(extent<1>(number_of_threads).tile<_tile_size>(), [=](tiled_index<_tile_size> tidx) restrict(amp) 
    {
        const int tid = tidx.local[0];
        const int globid = tidx.global[0];

        tile_static _type tile[2][_tile_size];
        int in = 0;
        int out = 1;
        if (globid < sz)
            tile[out][tid] = input[globid];
        tidx.barrier.wait();

        for (int offset=1; offset<_tile_size; offset *= 2) 
        {
            // For each iteration, the Y dimension index
            // specifies which index acts as input or output.
            // For each iteration these elements toggle
            in = 1 - in;
            out = 1 - out;

            if (globid < sz) 
            {
                if (tid  >= offset)
                    tile[out][tid]  = tile[in][tid-offset] + tile[in][tid];
                else 
                    tile[out][tid]  = tile[in][tid];
            }
            tidx.barrier.wait();
        }
        if (globid < sz)
            tilewise_scans[globid] = tile[out][tid];
        // update prefix sum of the tile to another array_view
        if (tid == _tile_size-1)
            sums_of_tiles[tidx.tile[0]] = tile[out][tid];
    });
}

template<typename _type, int _tile_size>
scan<_type, _tile_size>::scan(int size)
{
    assert(size%_tile_size == 0);
    
    for (unsigned i = 0; i < size; i++)
    {
        values.push_back((_type)((rand()) % 127));
        result.push_back(0);
    }
}

template<typename _type, int _tile_size>
scan<_type, _tile_size>::~scan()
{
}

template<typename _type, int _tile_size>
void scan<_type, _tile_size>::execute()
{
    array_view<const _type, 1> a_values(values.size(), values);
    array_view<_type, 1> a_result(result.size(), result);
    scan_tiled<_tile_size>(a_values, a_result);
    copy(a_result, result.begin());
}

template<typename _type, int _tile_size>
bool scan<_type, _tile_size>::verify()
{
    _type sum = (_type)0;

    for (unsigned i = 0; i < values.size(); i++)
    {
        sum += values[i];
        if (result[i] != sum)
        {
            std::cout << i << ": " << sum << " <> " << result[i] << std::endl;
            std::cout << "***SCAN VERIFICATION FAILURE***" << std::endl;
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

    scan<int, TILE_SIZE> _scan_int(NUM_OF_ELEMENTS*257);
    _scan_int.execute();
    _scan_int.verify();

    scan<float, TILE_SIZE> _scan_float(NUM_OF_ELEMENTS*4);
    _scan_float.execute();
    _scan_float.verify();

    if (default_device.supports_double_precision)
    {
        scan<double, TILE_SIZE> _scan_double(NUM_OF_ELEMENTS*2);

        _scan_double.execute();
        _scan_double.verify();
    }
}


