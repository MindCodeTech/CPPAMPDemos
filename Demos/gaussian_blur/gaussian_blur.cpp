//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: gaussian_blur.cpp
// 
// Implements a simple gaussian filter using in C++ AMP
//----------------------------------------------------------------------------

#include "gaussian_blur.h"

#include <iostream>
#include <assert.h>

void gaussian_blur::gaussian_blur_simple_amp_kernel(int size, const array<float,2> &input, array<float,2> &output)
{
    // Sample assumes the filter matrix to be a square matrix
    static_assert(BLUR_MTX_SIZE == (BLUR_MTX_DIM*BLUR_MTX_DIM), "Sample assumes filter matrix to be a square matrix");
    parallel_for_each(extent<2>(size, size), [=, &input, &output] (index<2> idx) restrict(amp)
        {
            float value = 0.0f;
            float total = 0.0f;
            const float gaussian_blur_matrix[BLUR_MTX_DIM][BLUR_MTX_DIM] = { BLUR_MTX_VALUES };
            for(int i=0; i<BLUR_MTX_DIM; i++) 
            {
                for(int j=0; j<BLUR_MTX_DIM; j++) 
                {
                    int x = idx[1] + i - (BLUR_MTX_DIM / 2);
                    int y = idx[0] + j - (BLUR_MTX_DIM / 2);
                    if ((x >= 0) & (y >= 0) & (x < size) & (y < size))
                    {
                        float coef = gaussian_blur_matrix[i][j];
                        total += coef;
                        value += coef * input(y, x);
                    }
                }
            }
            output[idx] = value / total;
        });
}

gaussian_blur::gaussian_blur(int _size)
{
    size = _size;
    assert((size&(size-1)) == 0);

    srand(2012);
    for (int i = 0; i < size*size; i++)
    {
        data.push_back(((float)rand()) / ((float)RAND_MAX));
        amp_result.push_back(0);
    }
}

gaussian_blur::~gaussian_blur()
{

}

void gaussian_blur::execute()
{
    // Allocate data buffer 
    array<float, 2> a_data(size, size, data.begin());
    // Allocate result buffer
    array<float, 2> a_amp_result(size, size);

    gaussian_blur_simple_amp_kernel(size, a_data, a_amp_result);

    copy(a_amp_result, amp_result.begin());
}

bool gaussian_blur::verify()
{
    std::cout << "Comparing results ";
    bool passed = true;
    const float blur_mtx[BLUR_MTX_DIM][BLUR_MTX_DIM] = { BLUR_MTX_VALUES };
    for(int yi=0; yi<size && passed; yi++) for(int xi=0; xi<size && passed; xi++)
    {
        float total = 0.0f;
        float value = 0.0f;
        for(int i=0; i<BLUR_MTX_DIM; i++) 
        {
            for(int j=0; j<BLUR_MTX_DIM; j++) 
            {
                int x = xi + i - (BLUR_MTX_DIM / 2);
                int y = yi + j - (BLUR_MTX_DIM / 2);
                if ((x >= 0) & (y >= 0) & (x < size) & (y < size))
                {
                    float coef = blur_mtx[i][j];
                    total += coef;
                    value += coef * data[size*y + x];
                }
            }
        }
        float expectedValue = value / total;
        float gotValue = amp_result[size*yi + xi];
        if (abs(gotValue-expectedValue) / max(1.0f, max(gotValue, expectedValue)) > 1e-4f)
        {
            passed = false;
            std::cout << "***GAUSSIANBLUR_UNTILED VERIFICATION FAILURE***" << std::endl;
            break;
        }
    }

    std::cout << "done. Verification Pass" << std::endl;
    return passed;
}

int main()
{
    accelerator default_device;
    std::wcout << L"Using device : " << default_device.get_description() << std::endl;
    if (default_device == accelerator(accelerator::direct3d_ref))
        std::cout << "WARNING!! Running on very slow emulator! Only use this accelerator for debugging." << std::endl;

    std::cout << "Applying Gaussian filter using non-tiled version of kernel " << std::endl;
    gaussian_blur _blur_simple;

    _blur_simple.execute();
    _blur_simple.verify();

}
