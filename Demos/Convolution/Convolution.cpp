//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: Convolution.cpp
// 
// Implement C++ AMP based simple and tiled version of Convolution filter used in 
// image processing.
//----------------------------------------------------------------------------

#include <math.h>
#include <assert.h>
#include <amp.h>
#include<iostream>

#pragma warning (disable : 4267)

#define DEFAULT_WIDTH   512
#define DEFAULT_HEIGHT  512
// TILE_SIZE should be multiple of both DEFAULT_WIDTH and DEFAULT_HEIGHT
#define TILE_SIZE		128

using namespace concurrency;

//----------------------------------------------------------------------------
// Simple implementation of convolution filter along different dimension
//----------------------------------------------------------------------------
template<int dim_to_convolve>
float convolution_dim_simple(index<2> idx, int radius, const array<float, 2> &img, const array<float, 1> &filter) restrict(amp)
{
	float sum = 0.0f;
    for (int k = -radius; k <= radius; k++)
    {
        int dim = direct3d::clamp((int)(idx[dim_to_convolve]) + k, 0, (int)(img.extent[dim_to_convolve]-1));

        index<2> aidx(idx);
		aidx[dim_to_convolve] = dim;

        index<1> kidx(k+radius);
        sum += img[aidx]*filter[kidx];
    }
	return sum;
}

//----------------------------------------------------------------------------
// Simple implementation of convolution separable filter 
//----------------------------------------------------------------------------
void convolution_simple(std::vector<float>& v_img, std::vector<float>& v_filter, std::vector<float>& v_result, size_t width, size_t height)
{
    extent<1> ek(v_filter.size());
    array<float, 1> a_filter(ek, v_filter.begin());

    extent<2> e(height, width);
    array<float, 2> a_img(e, v_img.begin()),  a_img_buffer(e), a_img_result(e);

    int radius = (static_cast<int>(a_filter.extent[0]) - 1)/2;

    parallel_for_each(a_img.extent, [=, &a_img_buffer, &a_img, &a_filter](index<2> idx) restrict(amp) 
	{
        a_img_buffer[idx] = convolution_dim_simple<1>(idx, radius, a_img, a_filter);
    });

    parallel_for_each(a_img.extent, [=, &a_img_result, &a_img_buffer, &a_filter](index<2> idx) restrict(amp) 
	{
        a_img_result[idx] = convolution_dim_simple<0>(idx, radius, a_img_buffer, a_filter);
    });

    v_result = a_img_result;
}

//----------------------------------------------------------------------------
// Tile implementation of convolution filter along different dimension
//----------------------------------------------------------------------------
template <int dim_y, int dim_x, int dim_to_convolve>
void convolution_tile_dim(tiled_index<dim_y, dim_x> &tidx, 
						  const array<float, 2> &img, const array<float, 1> &filter, 
						  int radius, array<float, 2> &result) restrict(amp)
{
    tile_static float local[TILE_SIZE];
    
    index<2> tile_idx = tidx.tile;
    index<2> local_idx = tidx.local;
        
    int idx_convolve = (tile_idx[dim_to_convolve])*(TILE_SIZE - 2 * radius) + (int)(local_idx[dim_to_convolve]) - radius;
    int max_idx_convolve = img.extent[dim_to_convolve];
    float sum = 0.0f;

    index<2> a_idx(tile_idx);
	a_idx[dim_to_convolve] = direct3d::clamp(idx_convolve, 0, max_idx_convolve-1);
    if (idx_convolve < (max_idx_convolve + radius))
    {
        local[local_idx[dim_to_convolve]] = img[a_idx];
    }
    tidx.barrier.wait();
    
    if ((int)(local_idx[dim_to_convolve]) >= radius && (int)(local_idx[dim_to_convolve]) < (TILE_SIZE - radius) && idx_convolve < max_idx_convolve)
    {
        for (int k = -radius; k <= radius; k++)
        {
            index<1> k_idx(k+radius);
            sum += local[local_idx[dim_to_convolve] + k]*filter[k_idx];
        }
        result[a_idx] = sum;
    }
}

//----------------------------------------------------------------------------
// Tile implementation of convolution separable filter
//----------------------------------------------------------------------------
void convolution_tiling(std::vector<float>& v_img, std::vector<float>& v_filter, std::vector<float>& v_result, size_t width, size_t height)
{
	assert((TILE_SIZE < DEFAULT_WIDTH) && (TILE_SIZE < DEFAULT_HEIGHT));

    extent<1> ek(v_filter.size());
    array<float, 1> a_filter(ek, v_filter.begin());

    extent<2> e(height, width);
    array<float, 2> a_img(e, v_img.begin()),  a_img_buffer(e), a_img_result(e);

    int radius = (static_cast<int>(a_filter.extent[0]) - 1)/2;
    
    extent<2> eRow((((e[0] - 1) / (TILE_SIZE - 2*radius)) + 1)*TILE_SIZE, e[1]);
    extent<2> eCol(e[0], (((e[1] - 1) / (TILE_SIZE - 2*radius)) + 1)*TILE_SIZE);
    
    parallel_for_each(extent<2>(eRow).tile<TILE_SIZE, 1>(), [=, &a_img_buffer, &a_img, &a_filter](tiled_index<TILE_SIZE, 1> ti) restrict(amp) 
	{
		convolution_tile_dim<TILE_SIZE, 1, 0>(ti, a_img, a_filter, radius, a_img_buffer);
    });

    parallel_for_each(extent<2>(eCol).tile<1, TILE_SIZE>(), [=, &a_img_buffer, &a_img_result, &a_filter](tiled_index<1, TILE_SIZE> ti) restrict(amp) 
	{
		convolution_tile_dim<1, TILE_SIZE, 1>(ti, a_img_buffer, a_filter, radius, a_img_result);
    });

    v_result = a_img_result;
}

//----------------------------------------------------------------------------
// CPU implementation of convolution separable filter
//----------------------------------------------------------------------------
void convolution_cpu(std::vector<float>& v_img, std::vector<float>& v_filter, std::vector<float>& v_result, size_t width, size_t height)
{
    size_t x, y;
    int d;
    int k;
    float sum;
    std::vector<float> temp(width*height);

	int radius = (v_filter.size() - 1)/2;
    
    // row convolution filter
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            sum = 0;
            for (k = -radius; k <= radius; k++)
            {
                d = static_cast<int>(x) + k;
                if (d < 0)
                {
                    d = 0;
                }
                if (d >= static_cast<int>(width))
                {
                    d = static_cast<int>(width) - 1;
                }
                sum += v_img[y*width + d]*v_filter[k + radius];
            }
            temp[y*width + x] = sum;
        }
    }

    // column convolution filter
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            sum = 0;
            for (k = -radius; k <= radius; k++)
            {
                d = static_cast<int>(y) + k;
                if (d < 0)
                {
                    d = 0;
                }
                if (d >= (int)height)
                {
                    d = static_cast<int>(height) - 1;
                }
                sum += temp[d*width + x]*v_filter[k + radius];
            }
            v_result[y*width + x] = sum;
        }
    }
}

void usage()
{
	printf("Usage: convolution.exe [<width> <height>]\n");
	printf("Convolution of an image\n");
	printf("\twidth - number of pixels\n");
	printf("\theight - number of pixels\n");
}

int main(int argc, char **argv)
{
    size_t width;
    size_t height;
    size_t radius = 7;
	assert(radius < TILE_SIZE);
    size_t filter_size = radius*2 + 1;
    size_t i;
    double sum_delta, sum_ref, L1_norm;

	if (argc > 3)
	{
		usage();
		exit(1);
	}
    width = (argc>1) ? atoi(argv[1]) : DEFAULT_WIDTH;
    height = (argc>2) ? atoi(argv[2]) : DEFAULT_HEIGHT;  

	accelerator default_device;
	std::wcout << L"Using device : " << default_device.get_description() << std::endl;
	if (default_device == accelerator(accelerator::direct3d_ref))
		std::cout << "WARNING!! Running on very slow emulator! Only use this accelerator for debugging." << std::endl;

	assert((width > TILE_SIZE) && (height > TILE_SIZE));

    printf("%i x %i\n", width, height);
    printf("Initializing data...\n");
    
    std::vector<float> v_filter(radius*2 + 1);
    std::vector<float> v_img(width*height);
    std::vector<float> cpu_ref(width*height);
    std::vector<float> amp_simple(width*height);
    std::vector<float> amp_tiling(width*height);

    // initialize filter values
    float filter_sum = 0;
    for (i = 0; i < filter_size; i++)
    {
        float dist = ((float)i - radius) / (float)radius;
        v_filter[i] = expf(-(dist*dist / 2));
        filter_sum += v_filter[i];
    }
    for (i = 0; i < filter_size; i++)
    {
        v_filter[i] /= filter_sum;
    }

    // initialize random data values
    srand(2011);
    for (i = 0; i < width*height; i++)
    {
        v_img[i] = (float)rand() / (float)RAND_MAX;
    }

  
    // amp simple separable convolution        
    printf("C++ AMP simple convolution...");
    convolution_simple(v_img, v_filter, amp_simple, width, height);
    printf("done.\n");

    // amp tiling separable convolution  
    printf("C++ AMP tiling convolution...");
    convolution_tiling(v_img, v_filter, amp_tiling, width, height);
    printf("done.\n");

    printf("Checking the results...\n");
    printf("CPU C++ simple convolution...");
    convolution_cpu(v_img, v_filter, cpu_ref, width, height);
	printf("done.\n");

    printf("...comparing the results.\n");
    printf("GPU simple results: ");
    sum_delta = 0;
    sum_ref   = 0;
    for (i = 0; i < width*height; i++)
    {
        sum_delta += fabs(cpu_ref[i] - amp_simple[i]);
        sum_ref   += fabs(cpu_ref[i]);
    }
    L1_norm = sum_delta / sum_ref;
    printf("L1 norm: %E    ", L1_norm);
    printf((L1_norm < 1e-6) ? "Data match\n" : "Data doesn't match\n");
    
    printf("GPU tiling results: ");
    sum_delta = 0;
    sum_ref   = 0;
    for (i = 0; i < width*height; i++)
    {
        sum_delta += fabs(cpu_ref[i] - amp_tiling[i]);
        sum_ref   += fabs(cpu_ref[i]);
    }
    L1_norm = sum_delta / sum_ref;
    printf("L1 norm: %E    ", L1_norm);
    printf((L1_norm < 1e-6) ? "Data match\n" : "Data doesn't match\n");

	printf ("Press ENTER to exit this program\n");
	getchar();
}
