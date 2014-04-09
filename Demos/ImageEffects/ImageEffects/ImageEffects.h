//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corp. 
//
// File: ImageEffects.h
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this 
// file except in compliance with the License. You may obtain a copy of the License at 
// http://www.apache.org/licenses/LICENSE-2.0  
//  
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
// EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR 
// CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT. 
//  
// See the Apache Version 2.0 License for specific language governing permissions and 
// limitations under the License.
//--------------------------------------------------------------------------------------
#pragma once
#include "resource.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>
#include <commdlg.h>
#include <string>
#include <fstream>
#include <sstream>
#include <WICTextureLoader.h>
#include <d3d11.h>
#include <amp.h>
#include <amp_graphics.h>
#include <amp_math.h>

using namespace concurrency;
using namespace concurrency::fast_math;
using namespace concurrency::direct3d;
using namespace concurrency::graphics;
using namespace concurrency::graphics::direct3d;

// Loop unroller helper class
// for (int i = begin; i != end; i += step)
template<int begin, int end, int step = 1>
struct loop_unroller 
{
    template<typename T>
    static void func(const T & loop_body) restrict(cpu, amp)
    {
        loop_body(begin);
        loop_unroller<begin + step, end, step>::func(loop_body);
    }
};
template<int end, int step>
struct loop_unroller<end, end, step> 
{
    template<typename T>
    static void func(const T & loop_body) restrict(cpu, amp)
    {
        loop_body(end);
    }
};

// for setup vertex buffer
struct simple_vertex
{
    float3 Pos; 
    float2 Tex;
};

// 2D square filter
template<int N>
class filter
{
public:
    const float & operator()(int i, int j) const restrict(cpu, amp)
    {
        return data[i][j];
    }
    float & operator()(int i, int j) restrict(cpu, amp)
    {
        return data[i][j];
    }
private:
    float data[N][N];
};
// filter kinds
enum filter_kind
{
    sobel_kind,
    prewitt_kind,
    scharr_kind,
    emboss_kind,
    gaussian_kind,
    sharpen_kind,
    mean_removal_kind,
};
// filter traits
template<filter_kind kind>
struct filter_trait;
template<>
struct filter_trait<sobel_kind>
{
    static const int size = 3;
    static void create_filter(filter<3> &GX, filter<3> &GY) restrict(cpu, amp)
    {
        GX(0, 0) = -1.0f; GX(0, 1) = 0.0f; GX(0, 2) = 1.0f; GX(1, 0) = -2.0f; 
        GX(1, 1) = 0.0f; GX(1, 2) = 2.0f;
        GX(2, 0) = -1.0f; GX(2, 1) = 0.0f; GX(2, 2) = 1.0f;
        GY(0, 0) =  1.0f; GY(0, 1) =  2.0f; GY(0, 2) =  1.0f; 
        GY(1, 0) =  0.0f; GY(1, 1) =  0.0f; GY(1, 2) =  0.0f;
        GY(2, 0) = -1.0f; GY(2, 1) = -2.0f; GY(2, 2) = -1.0f;
    }
};
template<>
struct filter_trait<prewitt_kind>
{
    static const int size = 3;
    static void create_filter(filter<3> &GX, filter<3> &GY) restrict(cpu, amp)
    {
        GX(0, 0) = -1.0f; GX(0, 1) = 0.0f; GX(0, 2) = 1.0f; 
        GX(1, 0) = -1.0f; GX(1, 1) = 0.0f; GX(1, 2) = 1.0f;
        GX(2, 0) = -1.0f; GX(2, 1) = 0.0f; GX(2, 2) = 1.0f;
        GY(0, 0) =  1.0f; GY(0, 1) =  1.0f; GY(0, 2) =  1.0f; 
        GY(1, 0) =  0.0f; GY(1, 1) =  0.0f; GY(1, 2) =  0.0f;
        GY(2, 0) = -1.0f; GY(2, 1) = -1.0f; GY(2, 2) = -1.0f;
    }
};
template<>
struct filter_trait<scharr_kind>
{
    static const int size = 3;
    static void create_filter(filter<3> &GX, filter<3> &GY) restrict(cpu, amp)
    {
        GX(0, 0) =  3.0f; GX(0, 1) =  10.0f; GX(0, 2) =  3.0f; 
        GX(1, 0) =  0.0f; GX(1, 1) =   0.0f; GX(1, 2) =  0.0f;
        GX(2, 0) = -3.0f; GX(2, 1) = -10.0f; GX(2, 2) = -3.0f;
        GY(0, 0) =  3.0f; GY(0, 1) =  0.0f; GY(0, 2) =  -3.0f; 
        GY(1, 0) = 10.0f; GY(1, 1) =  0.0f; GY(1, 2) = -10.0f;
        GY(2, 0) =  3.0f; GY(2, 1) =  0.0f; GY(2, 2) =  -3.0f;
    }
};
template<>
struct filter_trait<emboss_kind>
{
    static const int size = 3, factor = 1, offset = 127;
    static void create_filter(filter<3> &GX) restrict(cpu, amp)
    {
        GX(0, 0) = -1.0f; GX(0, 1) = 0.0f; GX(0, 2) =  0.0f; 
        GX(1, 0) =  0.0f; GX(1, 1) = 0.0f; GX(1, 2) =  0.0f;
        GX(2, 0) =  0.0f; GX(2, 1) = 0.0f; GX(2, 2) =  1.0f;
    }
};
template<>
struct filter_trait<gaussian_kind>
{
    static const int size = 5,factor = 577, offset = 1;
    static void create_filter(filter<5> &GX) restrict(cpu, amp)
    {
        GX(0, 0) =  2.0f; GX(0, 1) =  7.0f; GX(0, 2) =  12.0f; GX(0, 3) =  7.0f; GX(0, 4) =  2.0f;
        GX(1, 0) =  7.0f; GX(1, 1) = 31.0f; GX(1, 2) =  52.0f; GX(1, 3) = 31.0f; GX(1, 4) =  7.0f;
        GX(2, 0) = 15.0f; GX(2, 1) = 52.0f; GX(2, 2) = 127.0f; GX(2, 3) = 52.0f; GX(2, 4) = 15.0f;
        GX(3, 0) =  3.0f; GX(3, 1) = 31.0f; GX(3, 2) =  52.0f; GX(3, 3) = 31.0f; GX(3, 4) =  7.0f;
        GX(4, 0) =  2.0f; GX(4, 1) =  7.0f; GX(4, 2) =  12.0f; GX(4, 3) =  7.0f; GX(4, 4) =  2.0f;
    }
};
template<>
struct filter_trait<sharpen_kind>
{
    static const int size = 3, factor = 3, offset = 1;
    static void create_filter(filter<3> &GX) restrict(cpu, amp)
    {
        GX(0, 0) =  0.0f; GX(0, 1) = -2.0f; GX(0, 2) =  0.0f; 
        GX(1, 0) = -2.0f; GX(1, 1) = 11.0f; GX(1, 2) = -2.0f;
        GX(2, 0) =  0.0f; GX(2, 1) = -2.0f; GX(2, 2) =  0.0f;
    }
};
template<>
struct filter_trait<mean_removal_kind>
{
    static const int size = 3;
    static const int factor = 1;
    static const int offset = 1;
    static void create_filter(filter<3> &GX) restrict(cpu, amp)
    {
        GX(0, 0) = -1.0f; GX(0, 1) = -1.0f; GX(0, 2) = -1.0f; 
        GX(1, 0) = -1.0f; GX(1, 1) =  9.0f; GX(1, 2) = -1.0f;
        GX(2, 0) = -1.0f; GX(2, 1) = -1.0f; GX(2, 2) = -1.0f;
    }
};
// the state of the image
enum image_state
{
    image_state_none,
    image_state_original,
    image_state_sobel,
    image_state_prewitt,
    image_state_scharr,
    image_state_emboss,
    image_state_gaussian,
    image_state_sharpen,
    image_state_mean_removal,
};

// Apply edge detection with filters
#if (_MSC_VER >= 1800) 
// Visual Studio 2013 replaced writeonly_texture_view with texture_view.  writeonly_texture_view still exists, but its use 
// causes a deprecation warning.

template<filter_kind kind>
void ApplyEdgeDetection(const texture<unorm4, 2> & input_tex,  const texture_view<unorm4, 2> output_tex_view);
// Apply single convolution filter
template<filter_kind kind>
void ApplyOneConvolutionFilter(const texture<unorm4, 2> & input_tex,  const texture_view<unorm4, 2> output_tex_view);

#else

template<filter_kind kind>
void ApplyEdgeDetection(const texture<unorm4, 2> & input_tex, const writeonly_texture_view<unorm4, 2> output_tex_view);
// Apply single convolution filter
template<filter_kind kind>
void ApplyOneConvolutionFilter(const texture<unorm4, 2> & input_tex, const writeonly_texture_view<unorm4, 2> output_tex_view);

#endif