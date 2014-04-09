//--------------------------------------------------------------------------------------
// File: nbody.cpp
//
// C++ AMP implementation of n-body gravity computation
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"

#include <string.h>
#include <math.h>
#include <ppl.h>
#include <concrtrm.h>
#include <amprt.h>
#include <amp_math.h>
#include <assert.h>

#include "nbody.h"

#define TILE_SIZE                            128
#define softeningSquared                    0.0000015625f
#define fG                                    (6.67300e-11f*10000.0f)
#define fParticleMass                        (fG*10000.0f*10000.0f)
#define deltaTime                            0.1f 
#define dampening                            1.0f

extern ID3D11Buffer*                        g_pParticlePosVelo0;
extern ID3D11Buffer*                        g_pParticlePosVelo1;
extern ID3D11ShaderResourceView*            g_pParticlePosVeloRV0;
extern ID3D11ShaderResourceView*            g_pParticlePosVeloRV1;
extern ID3D11UnorderedAccessView*            g_pParticlePosVeloUAV0;
extern ID3D11UnorderedAccessView*            g_pParticlePosVeloUAV1;

// GPU based functions
void nbody::bodybody_interaction(float_3 &acc, const float_4 &pos, const float_4 &A) restrict(amp)
{
    float_3 r = A.xyz - pos.xyz;
    
    float distSqr = r.x * r.x + r.y * r.y + r.z * r.z;
    distSqr += softeningSquared;
    
    float invDist = fast_math::rsqrt(distSqr);
    float invDistCube =  invDist * invDist * invDist;
    
    float s = fParticleMass * invDistCube;
    
    acc += r*s;
}

// This helper function loads a group of particles
void nbody::load_particles(particle* pparticles, float_3 center, float_4 velocity, float spread, int num_particles)
{
    for( int i = 0; i < num_particles; i++ )
    {
        float_3 delta( spread, spread, spread );

        while( vec3_len_sqr(delta) > spread * spread )
        {
            delta.x = signed_uint_rand() * spread;
            delta.y = signed_uint_rand() * spread;
            delta.z = signed_uint_rand() * spread;
        }

        pparticles[i].pos.x = center.x + delta.x;
        pparticles[i].pos.y = center.y + delta.y;
        pparticles[i].pos.z = center.z + delta.z;
        pparticles[i].pos.w = 10000.0f * 10000.0f;

        pparticles[i].vel = velocity;
    }    
}

void nbody::CPU_kernel(const particle *data_in, particle &data_out, int num_bodies)
{
    float_4 pos = data_out.pos;

    float_4 vel = data_out.vel;
    
    float_3 acc(0.0f);

    for (int j = 0; j < num_bodies; j++)
    {  
        float_3 r;

        r.xyz = data_in[j].pos.xyz - pos.xyz;

        float dist_sqr = r.x*r.x + r.y*r.y + r.z*r.z;
        dist_sqr += softeningSquared;

        float inv_dist = 1.0f / sqrt(dist_sqr);
        float inv_dist_cube =  inv_dist * inv_dist * inv_dist;
        
        float s = fParticleMass * inv_dist_cube;

        acc.xyz += r.xyz * s;
    }
    
    vel.xyz += acc.xyz*deltaTime;

    vel.xyz *= dampening;

    pos.xyz += vel.x*deltaTime;

    data_out.pos = pos;
    data_out.vel = vel;
}

void nbody::SSE_kernel(const particle *data_in, particle &data_out, int num_bodies)
{
    const float softening_squared0 = softeningSquared;    
    const float paramf[2] = { deltaTime, 1.0f };
    const float OOOZ[4] = { 1.0f, 1.0f, 1.f, 0.0f };
    
    __m128 SSEOOOZ = _mm_loadu_ps(OOOZ);

    {
        //D3DXVECTOR3& pos = *(D3DXVECTOR3*)&B[i].pos;
        //D3DXVECTOR3& vel = *(D3DXVECTOR3*)&B[i].vel;
        //D3DXVECTOR3 acc(0,0,0);

        __m128 SSEpos = _mm_loadu_ps((float*)&data_out.pos);
        __m128 SSEvel = _mm_loadu_ps((float*)&data_out.vel);
        __m128 SSEacc = _mm_sub_ps(SSEpos, SSEpos);
        float myMass = fParticleMass;

        for (int j = 0; j < num_bodies; j++)
        {
            //D3DXVECTOR3& pos1 = *(D3DXVECTOR3*)&A[j].pos;
            __m128 SSEpos1 = _mm_loadu_ps((float*)&data_in[j].pos);

            {
                //D3DXVECTOR3 r = pos1 - pos;
                __m128 SSEr = _mm_sub_ps(SSEpos1, SSEpos);

                //float distSqr = r.x * r.x + r.y * r.y + r.z * r.z;
                // dot product
                __m128 SSEr2 = _mm_mul_ps(SSEr, SSEr);    //x    y    z    ?
                __m128 SSErshuf = _mm_shuffle_ps(SSEr2, SSEr2, _MM_SHUFFLE(0,3,2,1));
                SSEr2 = _mm_add_ps(SSEr2, SSErshuf);  //x+y, y+z, z+?, ?+x
                SSErshuf = _mm_shuffle_ps(SSEr2, SSEr2, _MM_SHUFFLE(1,0,3,2));
                SSEr2 = _mm_add_ps(SSErshuf, SSEr2);  //x+y+z+0, y+z+0+X, z+0+x+y, 0+x+y+z

                //distSqr += softeningSquared;
                __m128 SSEsoft = _mm_load1_ps( &softening_squared0 );
                SSEr2 = _mm_add_ps(SSEr2, SSEsoft); 

                //float inv_dist = 1.0f / sqrt(distSqr);
                //float inv_distCube =  inv_dist * inv_dist * inv_dist;
                __m128 SSEinvDistSqr = _mm_rsqrt_ps(SSEr2);
                __m128 SSEinvDistCube = _mm_mul_ps(_mm_mul_ps(SSEinvDistSqr,SSEinvDistSqr), SSEinvDistSqr);
                
                //float s = A[j].pos.w * invDistCube;
                //__m128 SSEmass = _mm_shuffle_ps(SSEpos1, SSEpos1, _MM_SHUFFLE(3,3,3,3));
                __m128 SSEmass = _mm_load1_ps( &myMass );
                __m128 SSEs = _mm_mul_ps(SSEmass, SSEinvDistCube); 

                //acc += r * s;
                SSEacc = _mm_add_ps( _mm_mul_ps(SSEr, SSEs), SSEacc ); 
            }
        }

        //vel += acc * paramf[0];      //deltaTime;
        __m128 SSEpf0 = _mm_load1_ps( &paramf[0] );
        SSEvel = _mm_add_ps( _mm_mul_ps(SSEacc, SSEpf0), SSEvel ); 
        //vel *= paramf[1];            //damping;
        //pos += vel * paramf[0];      //deltaTime;    
        SSEpos = _mm_add_ps( _mm_mul_ps(SSEvel, SSEpf0), SSEpos ); 
        SSEpos = _mm_mul_ps(SSEOOOZ, SSEpos); 
        
        _mm_storeu_ps((float*)&data_out.pos, SSEpos);
        _mm_storeu_ps((float*)&data_out.vel, SSEvel);
    }
}

void nbody::SSE4_kernel(const particle *data_in, particle &data_out, int num_bodies)
{
    const float softening_squared0 = softeningSquared;
    const float paramf[2] = { deltaTime, 1.0f };

    {
        //D3DXVECTOR3& pos = *(D3DXVECTOR3*)&B[i].pos;
        //D3DXVECTOR3& vel = *(D3DXVECTOR3*)&B[i].vel;
        //D3DXVECTOR3 acc(0,0,0);

        //pos = *(D3DXVECTOR3*)&A[i].pos;
        //vel = *(D3DXVECTOR3*)&A[i].vel;
        __m128 SSEpos = _mm_loadu_ps((float*)&data_out.pos);
        __m128 SSEvel = _mm_loadu_ps((float*)&data_out.vel);
        __m128 SSEacc = _mm_sub_ps(SSEpos, SSEpos);
        float myMass = fParticleMass;//B.pos.w;

        for (int j = 0; j < num_bodies; j++)
        {
            //D3DXVECTOR3& pos1 = *(D3DXVECTOR3*)&A[j].pos;
            __m128 SSEpos1 = _mm_loadu_ps((float*)&data_in[j].pos);

            {
                //D3DXVECTOR3 r = pos1 - pos;
                __m128 SSEr = _mm_sub_ps(SSEpos1, SSEpos);

                //float distSqr = r.x * r.x + r.y * r.y + r.z * r.z;
                __m128 SSEr2 = _mm_mul_ps(SSEr, SSEr);    //x    y    z    ?
                __m128 SSErshuf = _mm_shuffle_ps(SSEr2, SSEr2, _MM_SHUFFLE(0,3,2,1));
                SSEr2 = _mm_add_ps(SSEr2, SSErshuf);  //x+y, y+z, z+?, ?+x
                SSErshuf = _mm_shuffle_ps(SSEr2, SSEr2, _MM_SHUFFLE(1,0,3,2));
                SSEr2 = _mm_add_ps(SSErshuf, SSEr2);  //x+y+z+0, y+z+0+X, z+0+x+y, 0+x+y+z

                //distSqr += softeningSquared;
                __m128 SSEsoft = _mm_load1_ps( &softening_squared0 );
                SSEr2 = _mm_add_ps(SSEr2, SSEsoft); 

                //float invDist = 1.0f / sqrt(distSqr);
                //float invDistCube =  invDist * invDist * invDist;
                __m128 SSEinvDistSqr = _mm_rsqrt_ps(SSEr2);
                __m128 SSEinvDistCube = _mm_mul_ps(_mm_mul_ps(SSEinvDistSqr,SSEinvDistSqr), SSEinvDistSqr);
                
                //float s = A[j].pos.w * invDistCube;
                //__m128 SSEmass = _mm_shuffle_ps(SSEpos1, SSEpos1, _MM_SHUFFLE(3,3,3,3));
                __m128 SSEmass = _mm_load1_ps( &myMass );
                __m128 SSEs = _mm_mul_ps(SSEmass, SSEinvDistCube); 

                //acc += r * s;
                SSEacc = _mm_add_ps( _mm_mul_ps(SSEr, SSEs), SSEacc ); 
            }
        }

        //vel += acc * paramf[0];      //deltaTime;
        __m128 SSEpf0 = _mm_load1_ps( &paramf[0] );
        SSEvel = _mm_add_ps( _mm_mul_ps(SSEacc, SSEpf0), SSEvel ); 
        //vel *= paramf[1];            //damping;
        //pos += vel * paramf[0];      //deltaTime;    
        SSEpos = _mm_add_ps( _mm_mul_ps(SSEvel, SSEpf0), SSEpos ); 
        
        _mm_storeu_ps((float*)&data_out.pos, SSEpos);
        _mm_storeu_ps((float*)&data_out.vel, SSEvel);
    }
}

void nbody::cpu_single_core(const particle *data_in, particle *data_out, int num_bodies)
{
    for (int i = 0; i < num_bodies; i++)
    {
        data_out[i].pos = data_in[i].pos;
        data_out[i].vel = data_in[i].vel;
        
        _pfn_CPU_kernel(data_in, data_out[i], num_bodies);
    }
}

void nbody::cpu_ppl(const particle *data_in, particle *data_out, int num_bodies)
{
    const int num_tasks = ::GetProcessorCount();
    parallel_for((int)0, num_tasks, [&](int task_num)
    {    
        int base_index = task_num * (num_bodies/num_tasks);
        for (int k = 0; k < (num_bodies/num_tasks); ++k)
        {
            int i = base_index + k;
            data_out[i].pos = data_in[i].pos;
            data_out[i].vel = data_in[i].vel;
            
            _pfn_CPU_kernel(data_in, data_out[i], num_bodies);
        }
    });
}

static float length(float_3 & in) restrict(amp)
{
    return fast_math::sqrt(in.x * in.x + in.y * in.y + in.z * in.z);
}

void nbody::simple_implementation(const array<particle, 1>& data_in, array<particle, 1>& data_out, int num_bodies)
{
    extent<1> compute_domain(num_bodies);

    parallel_for_each(compute_domain, [=, &data_in, &data_out] (index<1> idx) restrict(amp)
        {
            float_4 pos = data_in[idx].pos;
            float_3 acc(0.0f);

            // Update current particle using all other particles
            int j = 0;
            do
            {
                bodybody_interaction(acc, pos, data_in[j].pos);
                j++;
            } while (j < num_bodies);

            float_4 vel = data_in[idx].vel;

            vel.xyz += acc * deltaTime;
            vel.xyz *= dampening;
            pos.xyz += vel.xyz * deltaTime;

            data_out[idx].pos = pos;
            data_out[idx].vel = float_4(vel.x, vel.y, vel.z, length(acc));
    });
}

void nbody::tiling_implementation(const array<particle, 1>& data_in, array<particle, 1>& data_out, int offset, int size, int num_bodies)
{
    static_assert(TILE_SIZE%8 == 0, "Tile size should be multiple of 8");
    assert(num_bodies % TILE_SIZE == 0);
    assert(size % TILE_SIZE == 0);
    
    int cnt = num_bodies / TILE_SIZE;
    extent<1> compute_domain(size);

    parallel_for_each(compute_domain.tile<TILE_SIZE>(), [=, &data_in, &data_out] (tiled_index<TILE_SIZE> ti) restrict(amp)
        {
    
            int idx_local = ti.local[0];
            int idx_global = ti.global[0] + offset;
    
            float_4 pos = data_in[idx_global].pos;
            float_3 acc(0.0f);
    
            // Update current particle using all other particles
            int idx_temp = idx_local;
            for (int tile = 0; tile < cnt; tile++, idx_temp += TILE_SIZE)
            {
                tile_static float_4 tile_mem[TILE_SIZE];

                // Cache a tile of particles into shared memory to increase IO efficiency
                tile_mem[idx_local] = data_in[idx_temp].pos;

                ti.barrier.wait_with_tile_static_memory_fence();

                // Unroll size should be multile of TILE_SIZE
                for (int j = 0; j < TILE_SIZE; j += 8 )
                {
                    bodybody_interaction(acc, pos, tile_mem[j+0]);
                    bodybody_interaction(acc, pos, tile_mem[j+1]);
                    bodybody_interaction(acc, pos, tile_mem[j+2]);
                    bodybody_interaction(acc, pos, tile_mem[j+3]);
                    bodybody_interaction(acc, pos, tile_mem[j+4]);
                    bodybody_interaction(acc, pos, tile_mem[j+5]);
                    bodybody_interaction(acc, pos, tile_mem[j+6]);
                    bodybody_interaction(acc, pos, tile_mem[j+7]);
                }
                ti.barrier.wait_with_tile_static_memory_fence();
            }

            float_3 vel = data_in[idx_global].vel.xyz;
            vel += acc * deltaTime;
            vel *= dampening;
            pos.xyz += vel * deltaTime;

            data_out[idx_global].pos = pos;
            data_out[idx_global].vel = float_4(vel.x, vel.y, vel.z, length(acc));
        });
}

void nbody::verify_SSE_implementation()
{
    int CPUInfo[4] = {-1};
    __cpuid(CPUInfo, 1);
    bool bSSE1 = CPUInfo[3] >> 24 && 0x1;
    bool bSSE4 = CPUInfo[2] >> 19 && 0x1;

    if (bSSE4)
    {
        _pfn_CPU_kernel = &nbody::SSE4_kernel;
    }
    else if (bSSE1)
    {
        _pfn_CPU_kernel = &nbody::SSE_kernel;
    }
    else
    {
        _pfn_CPU_kernel = &nbody::CPU_kernel;
    }
}

void nbody::create_compute_buffers(particle *data, int num_bodies)
{
    _pold[0] = new array<particle, 1>(num_bodies, data, _accl_view);
    _pnew[0] = new array<particle, 1>(num_bodies, _accl_view);

    HRESULT hr = S_OK;
    hr = (concurrency::direct3d::get_buffer( *_pold[0] ))->QueryInterface(__uuidof(ID3D11Buffer), (LPVOID*)&g_pParticlePosVelo0);
    assert(hr == S_OK);
    hr = (concurrency::direct3d::get_buffer( *_pnew[0] ))->QueryInterface(__uuidof(ID3D11Buffer), (LPVOID*)&g_pParticlePosVelo1);
    assert(hr == S_OK);

    for(int i = 1; i < _ndevices; i++)
    {
        // create fields of size num_bodies for position and velocity vectors
        _pold[i] = new array<particle, 1>(num_bodies, data, _accls[i].default_view);
        _pnew[i] = new array<particle, 1>(num_bodies, _accls[i].default_view);
    }
}

// AMP nbody functions for DirectX sample integration
void nbody::create_accelerators(int &num_devices)
{
    std::vector<accelerator> devices = accelerator::get_all();
    _ndevices = static_cast<int>(devices.size());

    if (_ndevices == 0)
    {
        accelerator dRef = accelerator(accelerator::direct3d_ref);
        num_devices = _ndevices = 1;
        _accls[0] = dRef;
    }
    else
    {
        num_devices = 0;
        int j = 1;
        // assign accelerators (except for the one used for rendering) to _accls[1], accls[2], ...
        for (int i = 0; i < _ndevices; i++)
        {
            if ((!devices[i].is_emulated) && (devices[i].device_path != accelerator::cpu_accelerator))
            {
                if (devices[i] != _accl_view.accelerator)
                {
                    _accls[j++] = devices[i];
                }
                num_devices++;
            }
        }
        _ndevices = num_devices;
    }
}

void nbody::amp_single_gpu(int num_bodies, bool tiled)
{
    int dev_index = 0;

    if (tiled)
    {
        tiling_implementation((*_pold[dev_index]), (*_pnew[dev_index]), 0, num_bodies, num_bodies);
    }
    else
    {
        simple_implementation((*_pold[dev_index]), (*_pnew[dev_index]), num_bodies);
    }

    std::swap(_pold[dev_index], _pnew[dev_index]);
    std::swap( g_pParticlePosVelo0, g_pParticlePosVelo1 );
    std::swap( g_pParticlePosVeloRV0, g_pParticlePosVeloRV1 );
    std::swap( g_pParticlePosVeloUAV0, g_pParticlePosVeloUAV1 );
}

void nbody::amp_multi_gpu(particle *render_data, int num_bodies)
{
    // If system has only one GPU, call 1 GPU optimized version
    if (_ndevices == 1)
    {
        amp_single_gpu(num_bodies, true);
        return;
    }
    
    assert(num_bodies % _ndevices == 0);

    int size = num_bodies / _ndevices;

    std::vector<std::shared_future<void>> futures(_ndevices);
    parallel_for(0, _ndevices, [&] (int i)
    {
        int offset = i * size;
        tiling_implementation((*_pold[i]), (*_pnew[i]), offset, size, num_bodies);
        array_view<const particle, 1> src = (*_pnew[i]).section(offset, size);
        futures[i] = copy_async(src, stdext::make_checked_array_iterator<particle *>(render_data + offset, size));
    });

    parallel_for(0, _ndevices, [&] (int i)
    {
        futures[i].get();
    });

    parallel_for(0, _ndevices, [&] (int i)
    {
        futures[i] = copy_async(render_data, *(_pold[i]));
    });
    
    parallel_for(0, _ndevices, [&] (int i)
    {
        futures[i].get();
    });
}

void nbody::release()
{
    for(int i = 0; i<_ndevices; i++)
    {
        if (_pold[i])
        {
            delete _pold[i];
            _pold[i] = NULL;
        }

        if (_pnew[i])
        {
            delete _pnew[i];
            _pnew[i] = NULL;
        }
    }

    if (g_pParticlePosVelo0)
        g_pParticlePosVelo0->Release();
    if (g_pParticlePosVelo1)
        g_pParticlePosVelo1->Release();
}
