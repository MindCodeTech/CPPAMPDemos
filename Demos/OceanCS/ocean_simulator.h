//--------------------------------------------------------------------------------------
// File: ocean_simulator.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OCEAN_WAVE_H
#define _OCEAN_WAVE_H

#include <amp.h>
#include "CSFFT/fft_512x512.h"

using std::vector;
using namespace concurrency;

#define PAD16(n) (((n)+15)/16*16)

#define PI 3.1415926536f
#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

#define H0_LEN 0x205020
#define OMEGA_LEN 0x102810
#define RUNALL_TEST_TIME_LEN 10

struct ocean_parameter
{
    ocean_parameter () 
    {
        // The size of displacement map. In this sample, it's fixed to 512.
        dmap_dim = 512;
        // The side length (world space) of square sized patch
        patch_length  = 2000.0f;
        // Adjust this parameter to control the simulation speed
        time_scale = 0.8f;
        // A scale to control the amplitude. Not the world space height
        wave_amplitude = 0.35f;
        // 2D wind direction. No need to be normalized
        wind_dir = MHVECTOR2(0.8f, 0.6f);
        // The bigger the wind speed, the larger scale of wave crest.
        // But the wave scale can be no larger than patch_length
        wind_speed = 600.0f;
        // Damp out the components opposite to wind direction.
        // The smaller the value, the higher wind dependency
        wind_dependency = 0.07f;
        // Control the scale of horizontal movement. Higher value creates
        // pointy crests.
        choppy_scale = 1.3f;
    }
    // Must be power of 2.
    int dmap_dim;
    // Typical value is 1000 ~ 2000
    float patch_length;

    // Adjust the time interval for simulation.
    float time_scale;
    // Amplitude for transverse wave. Around 1.0
    float wave_amplitude;
    // Wind direction. Normalization not required.
    MHVECTOR2 wind_dir;
    // Around 100 ~ 1000
    float wind_speed;
    // This value damps out the waves against the wind direction.
    // Smaller value means higher wind dependency.
    float wind_dependency;
    // The amplitude for longitudinal wave. Must be positive.
    float choppy_scale;
};

struct immutable
{
    unsigned int actualdim;
    unsigned int inwidth;
    unsigned int outwidth;
    unsigned int outheight;
    unsigned int dddressoffset;
    unsigned int addressoffset;
};

struct change_per_frame
{
    float time;
    float choppyscale;
};

class ocean_simulator
{
public:
    ocean_simulator(ocean_parameter& params, ID3D11Device* pd3dDevice);
    ~ocean_simulator();

    // -------------------------- Initialization & simulation routines ------------------------

    // Update ocean wave when tick arrives.
    void update_displacement_map(float time);

    // Texture access
    ID3D11ShaderResourceView* get_direct3d_displacement_map();
    ID3D11ShaderResourceView* get_direct3d_gradient_map();

    const ocean_parameter& get_parameters();


protected:
    inline void direct3d_synchronize();

    ocean_parameter m_param;

    // ---------------------------------- GPU shading asset -----------------------------------

    // D3D objects
    ID3D11Device* m_pd3dDevice;
    ID3D11DeviceContext* m_pd3dImmediateContext;

    // accelerator_view
    accelerator_view m_av;

    // Displacement map
    ID3D11Texture2D* m_pDisplacementMap;		// (RGBA32F)
    ID3D11ShaderResourceView* m_pDisplacementSRV;
    ID3D11RenderTargetView* m_pDisplacementRTV;

    // Gradient Field
    ID3D11Texture2D* m_pGradientMap;			// (RGBA16F)
    ID3D11ShaderResourceView* m_pGradientSRV;
    ID3D11RenderTargetView* m_pGradientRTV;

    // Samplers
    ID3D11SamplerState* m_pPointSamplerState;

    // Initialize the vector Field.
    void init_height_map(ocean_parameter& params, vector<float_2> &out_h0, vector<float> &out_omega);

    void init_buffers(ocean_parameter& params, vector<float_2> &h0_data, vector<float> &omega_data, vector<float_2> &zero_data);

    void init_dx11(int hmap_dim, int output_size, UINT float2_stride);
    // ----------------------------------- CS simulation data ---------------------------------

    // Initial height array H(0) generated by phillips spectrum & gauss distribution.
    array_view<const float_2> *m_array_view_float2_h0;

    // Angular frequency
    array_view<const float> *m_array_view_float_omega;

    // Height array H(t), choppy array Dx(t) and Dy(t) in frequency domain, updated each frame.
    array_view<float_2> *m_array_view_float2_ht;

    // Height & choppy buffer in the space domain, corresponding to H(t), Dx(t) and Dy(t)
    ID3D11Buffer* m_pBuffer_Float_Dxyz;
    ID3D11UnorderedAccessView* m_pUAV_Dxyz;
    ID3D11ShaderResourceView* m_pSRV_Dxyz;
    array_view<float_2> *m_array_view_float_dxyz;

    ID3D11Buffer* m_pQuadVB;

    // Shaders, layouts and constants
    ID3D11VertexShader* m_pQuadVS;
    ID3D11PixelShader* m_pUpdateDisplacementPS;
    ID3D11PixelShader* m_pGenGradientFoldingPS;

    ID3D11InputLayout* m_pQuadLayout;

    ID3D11Buffer* m_pImmutableCB;
    immutable m_cbimmutable;
    ID3D11Buffer* m_pPerFrameCB;
    change_per_frame m_cbchange_per_frame;

    // FFT wrap-up
    csfft512x512_plan m_fft_plan;
};

#endif	// _OCEAN_WAVE_H
