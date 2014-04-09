// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: BlackScholes.cpp
//
// Implements Black Scholes sample in C++ AMP
// Refer README.txt
//----------------------------------------------------------------------------

#include "BlackScholes.h"
#ifndef __APPLE__
#include <math.h>
#endif
#include <amp_math.h>
#include <iostream>
#include <assert.h>



float blackscholes::cnd_calc(float d) restrict (amp, cpu)
{
    const float a1 = 0.31938153f, a2 = -0.356563782f, a3 = 1.781477937f, a4 = -1.821255978f, a5 = 1.330274429f;
    const float isqrt2pi = 0.39894228040143267793994605993438f;
    float x = 1.0f / (1.0f + 0.2316419f * fast_math::fabsf(d));

    float cnd = isqrt2pi * fast_math::expf(- 0.5f * d * d) * (x * (a1 + x * (a2 + x * (a3 + x * (a4 + x * a5)))));
    return (d > 0) ? 1.0f - cnd : cnd;
}

blackscholes::blackscholes(float _volatility, float _riskfreerate, int _size)
{
    data_size = _size;
    riskfreerate = _riskfreerate;
    volatility = _volatility;

    stock_price.resize(data_size);
    option_strike.resize(data_size);
    option_years.resize(data_size);
    call_result_amp.resize(data_size);
    put_result_amp.resize(data_size);

    srand(2012);

    for (int i = 0; i < data_size; i++)
    {
        stock_price[i] = 100.0f * (((float)rand()) / RAND_MAX);
        option_strike[i] = stock_price[i] * ((float)rand()) / RAND_MAX;
        option_years[i] = 20.0f * ((float)rand()) / RAND_MAX;
        call_result_amp[i] = 0;
        put_result_amp[i] = 0;
    }
}

blackscholes::~blackscholes()
{

}

void blackscholes::execute()
{
    const array<float,1> a_stock_price(data_size, stock_price.begin());
    const array<float,1> a_option_strike(data_size, option_strike.begin());
    const array<float,1> a_option_years(data_size, option_years.begin());
    array<float,1> a_call_result(data_size);
    array<float,1> a_put_result(data_size);

    float R = riskfreerate;
    float V = volatility;

    std::vector<float> farr(8);
    farr[0] = 0.31938153f;
    farr[1] = -0.356563782f;
    farr[2] = 1.781477937f;
    farr[3] = -1.821255978f;
    farr[4] = 1.330274429f;
    farr[5] = 0.39894228040143267793994605993438f;
    farr[6] = 0.2316419f;
    array<float, 1> arr(8, farr.begin());

#if _WIN32
    assert((data_size%BSCHOLES_TILE_SIZE) == 0, "Tile size should be multiple of data size");
#else
    assert((data_size%BSCHOLES_TILE_SIZE) == 0 && "Tile size should be multiple of data size");
#endif

    parallel_for_each(extent<1>(data_size).tile<BSCHOLES_TILE_SIZE>(),
    [=, &a_stock_price, &a_option_strike, &a_option_years, &a_call_result, &a_put_result, &arr] (tiled_index<BSCHOLES_TILE_SIZE> tidx) restrict(amp)
    {
        float S = a_stock_price(tidx.global[0]);
        float X = a_option_strike(tidx.global[0]);
        float T = a_option_years(tidx.global[0]);

        float sqrtT = fast_math::sqrtf(T);
        float d1 = (fast_math::logf(S / X) + (R + 0.5f * V * V) * T) / (V * sqrtT);
        float d2 = d1 - V * sqrtT;

        float cndd1 = cnd_calc(d1);
        float cndd2 = cnd_calc(d2);
        float exp_RT = fast_math::expf(- R * T);
        a_call_result[tidx.global[0]] = S * cndd1 - X * exp_RT * cndd2;
        a_put_result[tidx.global[0]]  = X * exp_RT * (1.0f - cndd2) - S * (1.0f - cndd1);
    });

    copy(a_call_result, call_result_amp.begin());
    copy(a_put_result, put_result_amp.begin());
}

bool blackscholes::verify()
{
    std::vector<float> call_result_cpu(data_size);
    std::vector<float> put_result_cpu(data_size);

    for(unsigned i=0; i < stock_price.size(); i++)
    {
        blackscholes_CPU(i, call_result_cpu, put_result_cpu);
    }

    if (!sequence_equal(call_result_amp, call_result_cpu) | !sequence_equal(put_result_amp, put_result_cpu)) 
    {
        std::cout << "**BlackScholes verification FAILED.**" << std::endl;
	return 1;
    }
    else
    {
        std::cout << "**BlackScholes verification passed.**" << std::endl;
	return 0;
    }
}

bool blackscholes::sequence_equal(std::vector<float>& ref_data, std::vector<float>& compute_data) 
{
    const float EPS = 1e-4f;
    for(unsigned i=0; i < ref_data.size(); i++)
    {
        float a = ref_data[i], b = compute_data[i];

        // Is absolute or relative error less than EPS?
#if _WIN32
        float m = __max(1, __max(abs(a), abs(b)));
#else
        float m = max(1, max(abs(a), abs(b)));
#endif
        if (abs(a - b) / m > EPS) 
        {
            std::cout << "FAIL: "<< a <<',' << b << std::endl;
            return false;
        }
    }

    return true;
}


void blackscholes::blackscholes_CPU(unsigned i, std::vector<float>& out_call_result, std::vector<float>& out_put_result)
{
    float S = stock_price[i];
    float X = option_strike[i];
    float T = option_years[i];
    float R = riskfreerate;
    float V = volatility;

    float sqrtT = sqrt(T);
    float d1 = (log(S / X) + (R + 0.5f * V * V) * T) / (V * sqrtT);
    float d2 = d1 - V * sqrtT;

    std::vector<float> farr(8);
    farr[0] = 0.31938153f;
    farr[1] = -0.356563782f;
    farr[2] = 1.781477937f;
    farr[3] = -1.821255978f;
    farr[4] = 1.330274429f;
    farr[5] = 0.39894228040143267793994605993438f;
    farr[6] = 0.2316419f;
    array<float, 1> arr(8, farr.begin());

    float cndd1 = cnd_calc(d1);
    float cndd2 = cnd_calc(d2);

    float exp_RT = exp(- R * T);
    out_call_result[i] = S * cndd1 - X * exp_RT * cndd2;
    out_put_result[i]  = X * exp_RT * (1.0f - cndd2) - S * (1.0f - cndd1);
}

int main()
{
    accelerator default_device;
    std::wcout << L"Using device : " << default_device.get_description() << std::endl;

    std::cout << "Begin...\n";
    blackscholes _bs_float;
    _bs_float.execute();
    return _bs_float.verify();
}

