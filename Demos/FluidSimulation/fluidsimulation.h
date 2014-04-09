//--------------------------------------------------------------------------------------
// File: fluidsimulation.h
//
// C++ AMP implementation to compute dynamics of fluid simulation
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <amp.h>
#include <amp_math.h>
#include <amp_short_vectors.h>
using namespace concurrency;
using namespace concurrency::graphics;

class fluid_simulation
{
public:
    struct params
    {
        float f_time_step;
        float f_smooth_len;
        float f_pressure_stiffness;
        float f_rest_density;
        float f_density_coef;
        float f_grad_pressure_coef;
        float f_lap_viscosity_coef;
        float f_wall_stiffness;
        float_2 v_gravity;
        float_3 v_planes[4];
    };

    struct particle
    {
        float_2 position;
        float_2 velocity;
    };

    // Creates the simulation, allocating data on the provided device
    fluid_simulation(unsigned num_particles, ID3D11Device* pd3dDevice);

    // Simulates the simulation step using the provided parameters
    void simulate(const params& parameters);

    // Returns buffer underlying C++ AMP objects
    ID3D11Buffer* get_particles_buffer() const;
    ID3D11Buffer* get_particles_density_buffer() const;

private:
    // Density kernel - calculates the density using the technique of tiling the particle data in shared memory of the GPU
    void apply_density(const params& parameters);

    // Force kernel - calculates the force on each particle using a straightforward N^2 algorithm,
    // using the technique of tiling the particle position and density data into shared memory of the GPU
    void apply_forces(const params& parameters);

    // Integration kernel - the calculated density and the forces are used together to calculate the final position of each
    // particle after the end of one iteration
    void finalize_position(const params& parameters);

    accelerator_view   _av;
    array<particle, 1> _particles;
    array<float, 1>    _particles_density;
    array<float_2, 1>  _particles_forces;
};
