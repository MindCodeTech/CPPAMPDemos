//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: BlackScholes.h
// 
// Refer README.txt
//----------------------------------------------------------------------------

#pragma once

#include <vector>
#include <amp.h>

using namespace concurrency;

#define DEFAULT_SIZE                (512*512)
#define DEFAULT_RISK_FREE_RATE      (-0.01f)
#define DEFAULT_VOLATILITY          (0.99f)

#define BSCHOLES_TILE_SIZE          256

class blackscholes
{
public:
    blackscholes(float _volatility = DEFAULT_VOLATILITY, float _riskfreerate = DEFAULT_RISK_FREE_RATE, int _size = DEFAULT_SIZE);
    void execute();
    ~blackscholes();
    bool verify();

private:
    bool sequence_equal(std::vector<float>& ref_data, std::vector<float>& compute_data);

    void blackscholes_CPU(unsigned i, std::vector<float>& out_call_result, std::vector<float>& out_put_result);

    std::vector<float> stock_price, option_strike, option_years, call_result_amp, put_result_amp;
    int data_size;
    float riskfreerate;
    float volatility;
    static float cnd_calc(float d) restrict (amp, cpu);
};

