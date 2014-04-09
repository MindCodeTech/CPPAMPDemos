//RUN: false
//XFAIL: *
/*----------------------------------------------------------------------------
* Copyright (c) Microsoft Corp.
*
* Licensed under the Apache License, Version 2.0 (the "License"); you may not 
* use this file except in compliance with the License.  You may obtain a copy 
* of the License at http://www.apache.org/licenses/LICENSE-2.0  
* 
* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED 
* WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, 
* MERCHANTABLITY OR NON-INFRINGEMENT. 
*
* See the Apache Version 2.0 License for specific language governing 
* permissions and limitations under the License.
*---------------------------------------------------------------------------
* 
* C++ AMP measure launch overhead perf test.
*
*---------------------------------------------------------------------------*/

#include "LaunchOverhead.h"

LaunchOverhead_Simple1Array_AMP::LaunchOverhead_Simple1Array_AMP()    
    , m_array_1(1, ampAccelerator->default_view)
{
}

void LaunchOverhead_Simple1Array_AMP::SetupOnce()
{    
    // Warm-up
    Test();
}

void LaunchOverhead_Simple1Array_AMP::Test()
{
    auto& array_1 = m_array_1;
    int zero = 0;
    
    ampAccelerator->default_view.wait();
    StartTimer();
    
    for(int i = 0; i < 10000; i++)
    {
        parallel_for_each(ampAccelerator->default_view, array_1.extent, [zero, &array_1](index<1> idx) restrict(amp)
        {
            if(zero)
            {
                array_1[idx] = idx[0];
            }
        });
    }
    
    ampAccelerator->default_view.wait();
    StopTimer();
}

LaunchOverhead_SimpleMultipleArrays_AMP::LaunchOverhead_SimpleMultipleArrays_AMP()    
    , m_array_1(1, ampAccelerator->default_view)
    , m_array_2(1, ampAccelerator->default_view)
    , m_array_3(1, ampAccelerator->default_view)
    , m_array_4(1, ampAccelerator->default_view)
    , m_array_5(1, ampAccelerator->default_view)
    , m_array_6(1, ampAccelerator->default_view)
    , m_array_7(1, ampAccelerator->default_view)
    , m_array_8(1, ampAccelerator->default_view)
{
}

void LaunchOverhead_SimpleMultipleArrays_AMP::SetupOnce()
{    
    // Warm-up
    Test();
}

void LaunchOverhead_SimpleMultipleArrays_AMP::Test()
{
    auto& array_1 = m_array_1;
    auto& array_2 = m_array_2;
    auto& array_3 = m_array_3;
    auto& array_4 = m_array_4;
    const auto& array_5 = m_array_5;
    const auto& array_6 = m_array_6;
    const auto& array_7 = m_array_7;
    const auto& array_8 = m_array_8;
    int zero = 0;
    
    ampAccelerator->default_view.wait();
    StartTimer();
    
    for(int i = 0; i < 10000; i++)
    {
        parallel_for_each(ampAccelerator->default_view, array_1.extent,
            [zero,
             &array_1, &array_2, &array_3, &array_4, &array_5, &array_6, &array_7, &array_8
             ](index<1> idx) restrict(amp)
        {
            if(zero)
            {
                array_1[idx] = array_5[idx];
                array_2[idx] = array_6[idx]; 
                array_3[idx] = array_7[idx]; 
                array_4[idx] = array_8[idx];
            }
        });
    }
    
    ampAccelerator->default_view.wait();
    StopTimer();
}

template <unsigned N>
LaunchOverhead_CaptureN_AMP<N>::LaunchOverhead_CaptureN_AMP()    
    , m_array_1(1, ampAccelerator->default_view)
{
}

template <unsigned N>
void LaunchOverhead_CaptureN_AMP<N>::SetupOnce()
{    
    // Warm-up
    Test();
}

template <unsigned N>
void LaunchOverhead_CaptureN_AMP<N>::Test()
{
    auto& array_1 = m_array_1;
    NBytesData nBytesData;
    int zero = 0;
    
    ampAccelerator->default_view.wait();
    StartTimer();
    
    for(int i = 0; i < 10000; i++)
    {
        parallel_for_each(ampAccelerator->default_view, array_1.extent, [zero, &array_1, nBytesData](index<1> idx) restrict(amp)
        {
            if(zero)
            {
                array_1[idx] = 0;
                for(unsigned i = 0; i < data_num_elems; i++)
                {
                    array_1[idx] += nBytesData.data[i];
                }
            }
        });
    }
    
    ampAccelerator->default_view.wait();
    StopTimer();
}

//Do the test for the following parameter
REGISTER_TEST_EX(LaunchOverhead_CaptureN_AMP<15360>, LaunchOverhead_Capture15360_AMP) // 15 KB

LaunchOverhead_Tiled1Array_AMP::LaunchOverhead_Tiled1Array_AMP()    
    , m_array_1(1, ampAccelerator->default_view)
{
}

void LaunchOverhead_Tiled1Array_AMP::SetupOnce()
{    
    // Warm-up
    Test();
}

void LaunchOverhead_Tiled1Array_AMP::Test()
{
    auto& array_1 = m_array_1;
    int zero = 0;
    
    ampAccelerator->default_view.wait();
    StartTimer();
    
    for(int i = 0; i < 10000; i++)
    {
        parallel_for_each(ampAccelerator->default_view, array_1.extent.tile<1>(), [zero, &array_1](tiled_index<1> idx) restrict(amp)
        {
            if(zero)
            {
                array_1[idx.global] = idx.global[0];
            }
        });
    }
    
    ampAccelerator->default_view.wait();
    StopTimer();
}

LaunchOverhead_LogicalAddressing_AMP::LaunchOverhead_LogicalAddressing_AMP()    
    , m_array_1(1, ampAccelerator->default_view)
    , m_tiled(false)
    , m_domain(4096, 4096)
{
}

void LaunchOverhead_LogicalAddressing_AMP::SetupOnce()
{
	//LaunchOverhead_LogicalAddressing.tiled
	m_tiled = true;
	//LaunchOverhead_LogicalAddressing.domainX
	m_domain[0] = 1024;
	//LaunchOverhead_LogicalAddressing.domainY
	m_domain[1] = 1024

    // Warm-up
    Test();
}

void LaunchOverhead_LogicalAddressing_AMP::Test()
{
    auto& array_1 = m_array_1;
    int zero = 0;

    if(m_tiled)
    {
        ampAccelerator->default_view.wait();
        StartTimer();
    
        for(int i = 0; i < 10000; i++)
        {
            // Note: For results of this benchmark to be actionable, the tile size specified here must exactly match the tile size used
            // internally for the simple p_f_e dispatch.
            parallel_for_each(ampAccelerator->default_view, m_domain.tile<8, 32>(), [zero, &array_1](tiled_index<8, 32> idx) restrict(amp)
            {
                if(idx.global[0] == zero && idx.global[1] == zero)
                {
                    array_1[0] = 1;
                }
            });
        }
    
        ampAccelerator->default_view.wait();
        StopTimer();
    }
    else
    {
        ampAccelerator->default_view.wait();
        StartTimer();
    
        for(int i = 0; i < 10000; i++)
        {
            parallel_for_each(ampAccelerator->default_view, m_domain, [zero, &array_1](index<2> idx) restrict(amp)
            {
                if(idx[0] == zero && idx[1] == zero)
                {
                    array_1[0] = 1;
                }
            });
        }
    
        ampAccelerator->default_view.wait();
        StopTimer();
    }
}
