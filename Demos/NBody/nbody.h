//--------------------------------------------------------------------------------------
// File: nbody.cpp
//
// C++ AMP implementation n-body gravity computation
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <amp.h>
#include <amp_graphics.h>

using namespace concurrency;
using namespace concurrency::graphics;

struct particle
{
    particle() restrict(amp,cpu) {}

    particle(const particle& a) restrict(amp,cpu) 
    {
        pos = a.pos;
        vel = a.vel;
    }

    particle operator= (const particle& a) restrict(amp,cpu) 
    {
        pos = a.pos;
        vel = a.vel;
        return *this;
    }

    float_4 pos;
    float_4 vel;
};

using namespace concurrency;

#define MAX_DEVICE_COUNT 4 


inline float vec3_len_sqr(float_3 v3)
{
	return v3.x*v3.x + v3.y*v3.y + v3.z*v3.z;
}

class nbody
{
public:
	nbody(ID3D11Device* pd3dDevice) :
		_accl_view(concurrency::direct3d::create_accelerator_view(reinterpret_cast<IUnknown *>(pd3dDevice)))
	{

	}

	~nbody()
	{
        release();
	}

	// Functions called by UI framework
	void verify_SSE_implementation();

	void create_accelerators(int &num_devices);
	void create_compute_buffers(particle *data, int num_bodies);

	void cpu_single_core(const particle *data_in, particle *data_out, int num_bodies);
	void cpu_ppl(const particle *data_in, particle *data_out, int num_bodies);

	void amp_single_gpu(int num_bodies, bool tiled);
	void amp_multi_gpu(particle *render_data, int num_bodies);

	void release();

	void load_particles(particle* pparticles, float_3 center, float_4 velocity, float spread, int num_particles);


private:
	// function pointer to switch to SSE nbody implementations
	void (*_pfn_CPU_kernel)(const particle *data_in, particle &data_out, int num_bodies);

	// All CPU based function
	inline float signed_uint_rand()
	{
		float ret = ( float )( ( rand() % 10000 ) - 5000 );
		return ret / 5000.0f;
	}

	static void CPU_kernel(const particle *data_in, particle &data_out, int num_bodies);
	static void SSE_kernel(const particle *data_in, particle &data_out, int num_bodies);
	static void SSE4_kernel(const particle *data_in, particle &data_out, int num_bodies);

	//// GPU based functions
	static void bodybody_interaction(float_3 &acc, const float_4 &pos, const float_4 &A) restrict(amp);
	
	// Different implementation of NBody
	void simple_implementation(const array<particle, 1>& data_in, array<particle, 1>& data_out, int num_bodies);
	void tiling_implementation(const array<particle, 1>& data_in, array<particle, 1>& data_out, int offset, int size, int num_bodies);

	// Data
	array<particle, 1> *_pold[MAX_DEVICE_COUNT];
	array<particle, 1> *_pnew[MAX_DEVICE_COUNT];
	accelerator			_accls[MAX_DEVICE_COUNT];
	accelerator_view	_accl_view;
	int					_ndevices;
};