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
#pragma once
#include <amp_graphics.h>

// Measure time to launch a simple p_f_e capturing one array.
struct LaunchOverhead_Simple1Array_AMP 
{
    LaunchOverhead_Simple1Array_AMP();
    void SetupOnce() override;
    void Test() override;

private:
    array<int, 1> m_array_1;
};

// Simple1Array + multiple arrays.
struct LaunchOverhead_SimpleMultipleArrays_AMP 
{
    LaunchOverhead_SimpleMultipleArrays_AMP();
    void SetupOnce() override;
    void Test() override;

private:
    array<int, 1> m_array_1;
    array<int, 1> m_array_2;
    array<int, 1> m_array_3;
    array<int, 1> m_array_4;
    array<int, 1> m_array_5;
    array<int, 1> m_array_6;
    array<int, 1> m_array_7;
    array<int, 1> m_array_8;
};

// Simple1Array + N bytes of data.
template <unsigned N>
struct LaunchOverhead_CaptureN_AMP 
{
    LaunchOverhead_CaptureN_AMP();
    void SetupOnce() override;
    void Test() override;

private:
    static const int data_num_elems = N / sizeof(int);
    struct NBytesData
    {
        int data[data_num_elems];
    };

    array<int, 1> m_array_1;
};

// Measure time to launch a tiled p_f_e capturing one array.
struct LaunchOverhead_Tiled1Array_AMP 
{
    LaunchOverhead_Tiled1Array_AMP();
    void SetupOnce() override;
    void Test() override;

private:
    array<int, 1> m_array_1;
};

// Measure time to compute logical address in the kernel.
// Parameters:
//   - LaunchOverhead_LogicalAddressing.tiled
//   - LaunchOverhead_LogicalAddressing.domainX
//   - LaunchOverhead_LogicalAddressing.domainY
struct LaunchOverhead_LogicalAddressing_AMP
{
    LaunchOverhead_LogicalAddressing_AMP();
    void SetupOnce() override;
    void Test() override;

private:
    array<int, 1> m_array_1;
    bool m_tiled;
    extent<2> m_domain;
};
