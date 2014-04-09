//--------------------------------------------------------------------------------------
// File: fluidsimulation.cpp
//
// C++ AMP implementation to compute dynamics of fluid simulation
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include <sstream>
#include "fluidsimulation.h"

// Size of the tile used in the kernel
const UINT TILE_SIZE = 256;

// Dot product
static float dot2(float_2 u, float_2 v) restrict(amp)
{
	return u.x * v.x + u.y * v.y;
}

fluid_simulation::fluid_simulation(unsigned num_particles, ID3D11Device* pd3dDevice)
    : _av(concurrency::direct3d::create_accelerator_view(reinterpret_cast<IUnknown *>(pd3dDevice))),
      _particles(num_particles , _av),
      _particles_density(num_particles, _av), 
      _particles_forces(num_particles, _av)
{
    // Check preconditions
    if(num_particles % TILE_SIZE != 0)
    {
        std::stringstream ss;
        ss << "Number of particles (" << num_particles << ") must be a multiple of tile size (" << TILE_SIZE << ").";
        throw std::exception(ss.str().c_str());
    }

    // Create the initial particle positions
    const unsigned starting_width = static_cast<unsigned>( sqrt( static_cast<float>(num_particles) ) );
    const float initial_particle_spacing = 0.0045f;

    std::vector<particle> particle_data(num_particles);
    for(unsigned i = 0; i < num_particles; i++)
    {
        // Arrange the particles in a nice square
        unsigned x = i % starting_width;
        unsigned y = i / starting_width;
        particle_data[i].position.x = initial_particle_spacing * static_cast<float>(x);
        particle_data[i].position.y = initial_particle_spacing * static_cast<float>(y);
    }

    // Copy initial data to C++ AMP buffer
    copy(particle_data.begin(), _particles);
}

ID3D11Buffer* fluid_simulation::get_particles_buffer() const
{
    return reinterpret_cast<ID3D11Buffer *>(concurrency::direct3d::get_buffer(_particles));
}

ID3D11Buffer* fluid_simulation::get_particles_density_buffer() const
{
    return reinterpret_cast<ID3D11Buffer *>(concurrency::direct3d::get_buffer(_particles_density));
}

void fluid_simulation::apply_density(const params& parameters)
{
    auto& particles = _particles;
    auto& particles_density = _particles_density;

    parallel_for_each(particles.extent.tile<TILE_SIZE>(), [=, &particles, &particles_density] (tiled_index<TILE_SIZE> tidx) restrict(amp)
    {
        tile_static float_2 density_shared_pos[TILE_SIZE];

        index<1> local = tidx.local;
        index<1> global = tidx.global;

        const float h_sq = parameters.f_smooth_len * parameters.f_smooth_len;
        float_2 p_position = particles[global].position;

        float density = 0;

        // Calculate the density based on all neighbors
        for (unsigned int tile_offset = 0; tile_offset < particles.extent.size(); tile_offset += TILE_SIZE)
        {
            // Cache a tile of particles unto shared memory to increase IO efficiency
            density_shared_pos[local[0]] = particles(tile_offset + local[0]).position;
            tidx.barrier.wait_with_tile_static_memory_fence();

            for (unsigned int i = 0; i < TILE_SIZE; i++)
            {
                float_2 N_position = density_shared_pos[i];

                float_2 diff = N_position - p_position;
                float r_sq = dot2(diff, diff);
                if (r_sq < h_sq)
                {
                    density += parameters.f_density_coef * (h_sq - r_sq) * (h_sq - r_sq) * (h_sq - r_sq);
                }
            }
            tidx.barrier.wait_with_tile_static_memory_fence();
        }

        particles_density[global] = density;
    });
}

void fluid_simulation::apply_forces(const params& parameters)
{
    auto& particles = _particles;
    auto& particles_density = _particles_density;
    auto& particles_forces = _particles_forces;

    parallel_for_each(particles.extent.tile<TILE_SIZE>(), [=, &particles, &particles_density, &particles_forces] (tiled_index<TILE_SIZE> tidx) restrict(amp)
    {
        tile_static struct { float_2 position; float_2 velocity; float density; } force_shared_pos[TILE_SIZE];

        index<1> local = tidx.local;
        index<1> global = tidx.global;

        const float h_sq = parameters.f_smooth_len * parameters.f_smooth_len;
        float_2 p_position = particles[global].position;
        float_2 p_velocity = particles[global].velocity;
        float p_density = particles_density[global];
        float p_pressure = parameters.f_pressure_stiffness * fast_math::fmax(fast_math::pow(p_density / parameters.f_rest_density, 3.f) - 1, 0.f);

        float_2 acceleration;

        // Calculate the acceleration based on all neighbors
        for (unsigned int tile_offset = 0; tile_offset < particles.extent.size(); tile_offset += TILE_SIZE)
        {
            // Cache a tile of particles unto shared memory to increase IO efficiency
            int offset_idx = tile_offset + local[0];
            force_shared_pos[local[0]].position = particles(offset_idx).position;
            force_shared_pos[local[0]].velocity = particles(offset_idx).velocity;
            force_shared_pos[local[0]].density = particles_density(offset_idx);
            tidx.barrier.wait_with_tile_static_memory_fence();

            for (unsigned int i = 0; i < TILE_SIZE; i++ ) 
            {
                index<1> idx(tile_offset + i);
                float_2 N_position = force_shared_pos[i].position;

                float_2 diff = N_position - p_position;
                float r_sq = dot2(diff, diff);
                if (r_sq < h_sq && global != idx)
                {
                    float_2 N_velocity = force_shared_pos[i].velocity;
                    float N_density = force_shared_pos[i].density;

                    // Pressure Term
                    float N_pressure = parameters.f_pressure_stiffness * fast_math::fmax(fast_math::pow(N_density / parameters.f_rest_density, 3.f) - 1, 0.f);
                    float r = fast_math::sqrt(r_sq);

                    float avg_pressure = 0.5f * (N_pressure + p_pressure);
                    float grad_pressure_const = parameters.f_grad_pressure_coef * avg_pressure / N_density * (parameters.f_smooth_len - r) * (parameters.f_smooth_len - r) / r;

                    acceleration += grad_pressure_const * diff;

                    // Viscosity Term
                    float_2 vel_diff = N_velocity - p_velocity;
                    float viscosity_const = parameters.f_lap_viscosity_coef / N_density * (parameters.f_smooth_len - r);

                    acceleration += viscosity_const * vel_diff;
                }
            }
            tidx.barrier.wait();
        }

        particles_forces[global] = acceleration / p_density;
    });
}

void fluid_simulation::finalize_position(const params& parameters)
{
    auto& particles = _particles;
    auto& particles_forces = _particles_forces;

    parallel_for_each(particles.extent, [=, &particles, &particles_forces] (index<1> idx) restrict(amp) 
    {
        float_2 position = particles[idx].position;
        float_2 velocity = particles[idx].velocity;
        float_2 acceleration = particles_forces[idx];

        // Wall collisions
        for(int i = 0; i < 4; i++)
        {
            float_2 normal(parameters.v_planes[i].x, parameters.v_planes[i].y);
            float offset = parameters.v_planes[i].z;
            float dist = dot2(position, normal) + offset;
            acceleration += fast_math::fmin(dist, 0.f) * -parameters.f_wall_stiffness * normal;
        }
    
        // Apply gravity
        acceleration += parameters.v_gravity;

        // Integrate
        velocity += parameters.f_time_step * acceleration;
        position += parameters.f_time_step * velocity;

        // Update
        particles[idx].position = position;
        particles[idx].velocity = velocity;
    });
}


//All the kernels are called in this method, forming a complete single iteration for the fluid simulation
void fluid_simulation::simulate(const params& parameters)
{
    apply_density(parameters);
    apply_forces(parameters);
    finalize_position(parameters);
}
