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
* C++ AMP NBody perf test.
*
*---------------------------------------------------------------------------*/
// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll 
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl 
// RUN: pushd %t 
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
#include <amp.h>
#include <amp_math.h>

#define TS (128)
#define SIZE (65536)
#define softeningSquared (0.0000015625f)
#define fG               (6.67300e-11f*10000.0f)
#define fParticleMass    (fG*10000.0f*10000.0f)
#define deltaTime        (0.1f)
#define dampening        (1.0f)

//using namespace Concurrency::graphics::direct3d;  Please use something similar to float4
using namespace Concurrency; 
using namespace Concurrency::fast_math;


struct NBody_NAIVE_AMP
{
    void SetupOnce();
    void Setup();
    void Test();
    void Cleanup();
    void CleanupOnce();
    int size;
    float* data;
};

typedef struct
{
  float pos[4];
  float vel[4];
} particle;

inline float vec3_len_sqr(float v[3])
{
    return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}
inline float signed_uint_rand()
{
    float ret = ( float )( ( rand() % 10000 ) - 5000 );
    return ret / 5000.0f;
}

//please used something equivalent to float4
void bodybody_interaction_NAIVE_AMP(const cl_float4 pos, const cl_float4 A, cl_float4 &r) restrict(amp)
{
    r.s[0] = A.s[0] - pos.s[0];
    r.s[1] = A.s[1] - pos.s[1];
    r.s[3] = A.s[3] - pos.s[3];
    float distSqr = r.s[0]*r.s[0] + r.s[1]*r.s[1] + r.s[2]*r.s[2];
    distSqr += softeningSquared;
    float invDist = 1.0f / sqrt(distSqr);
    float invDistCube =  invDist*invDist*invDist;
    float s = fParticleMass*invDistCube;
    r.s[0] *= s;
    r.s[1] *= s;
    r.s[2] *= s;
}

float nbody_length3_NAIVE_AMP(const cl_float3 f) restrict(amp)
{
    return sqrt(f.s[0]*f.s[0] + f.s[1]*f.s[1] + f.s[2]*f.s[2]);
}

void NBodyKernel_NAIVE_AMP_Kernel(
    int size,
    const array<float,1> &indata,
    array<float,1> &outdata,
    int P1)
{
    parallel_for_each(
        extent<1>(P1*TS).tile<TS>(),
        [=, &indata, &outdata] (tiled_index<TS> t_idx) restrict(amp)
        {
            int idx = t_idx.local[0];
            particle p;
            p.pos[0] = indata[idx * 8];
            p.pos[1] = indata[idx * 8 + 1];
            p.pos[2] = indata[idx * 8 + 2];
            p.pos[3] = indata[idx * 8 + 3];
            p.vel[0] = indata[idx * 8 + 4];
            p.vel[1] = indata[idx * 8 + 5];
            p.vel[2] = indata[idx * 8 + 6];
            p.vel[3] = indata[idx * 8 + 7];
            cl_float3 acc, res;
            acc.s[0] = acc.s[1] = acc.s[2] = 0.0f;
            int j = 0;
            do
            {
                cl_float4 pos, p_pos;
                pos.s[0] = indata[j * 8];
                pos.s[1] = indata[j * 8 + 1];
                pos.s[2] = indata[j * 8 + 2];
                pos.s[3] = indata[j * 8 + 3];
                p_pos.s[0] = p.pos[0];
                p_pos.s[1] = p.pos[1];
                p_pos.s[2] = p.pos[2];
                p_pos.s[3] = p.pos[3];
                bodybody_interaction_NAIVE_AMP(p_pos, pos, res);
                acc.s[0] += res.s[0];
                acc.s[1] += res.s[1];
                acc.s[2] += res.s[2]; 
                j++;
            } while (j < SIZE);
            p.vel[0] += acc.s[0] * deltaTime;
            p.vel[1] += acc.s[1] * deltaTime;
            p.vel[2] += acc.s[2] * deltaTime;
            p.vel[0] *= dampening;
            p.vel[1] *= dampening;
            p.vel[2] *= dampening;
            p.pos[0] += p.vel[0] * deltaTime;
            p.pos[1] += p.vel[1] * deltaTime;
            p.pos[2] += p.vel[2] * deltaTime;
            outdata[idx * 8] = p.pos[0];
            outdata[idx * 8 + 1] = p.pos[1];
            outdata[idx * 8 + 2] = p.pos[2];
            outdata[idx * 8 + 3] = p.pos[3];
            outdata[idx * 8 + 4] = p.vel[0];
            outdata[idx * 8 + 5] = p.vel[1];
            outdata[idx * 8 + 6] = p.vel[2];
            outdata[idx * 8 + 7] = nbody_length3_NAIVE_AMP(acc);
        });
}

void NBody_NAIVE_AMP::SetupOnce()
{
    size = 65536;
}

void NBody_NAIVE_AMP::Setup()
{
    data = new float[size*8];
    float spread = 400.0f;
    float center[3] = {200.0f, 200.0f, 200.0f};
    for(int i = 0; i < size; i++)
    {
        float delta[3] = {spread, spread, spread};
        while( vec3_len_sqr(delta) > spread * spread )
        {
            delta[0] = signed_uint_rand() * spread;
            delta[1] = signed_uint_rand() * spread;
            delta[2] = signed_uint_rand() * spread;
        }
        data[i * 8] = center[0] + delta[0]; // position
        data[i * 8 + 1] = center[1] + delta[1];
        data[i * 8 + 2] = center[2] + delta[2];
        data[i * 8 + 3] = 10000.0f * 10000.0f;
        data[i * 8 + 4] = 200.0f; //velocity
        data[i * 8 + 5] = 0.0f;
        data[i * 8 + 6] = 0.0f;
        data[i * 8 + 7] = 0.0f;
    }
}

void NBody_NAIVE_AMP::CleanupOnce()
{
}

void NBody_NAIVE_AMP::Cleanup()
{
    delete[] data;
}

void NBody_NAIVE_AMP::Test()
{
    // Allocate buffer indata
    array<float,1> indata(size*8);
    // Allocate buffer outdata
    array<float,1> outdata(size*8);
    // Begin @GEN_PRE_RUN_KERNEL(NBodyKernel);
    NBodyKernel_NAIVE_AMP_Kernel(
        size,
        indata,
        outdata,
        size/TS);
    // End @GEN_PRE_RUN_KERNEL(NBodyKernel);
    std::cout << "Starting timer...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    // Run kernel NBodyKernel_NAIVE
    NBodyKernel_NAIVE_AMP_Kernel(
        size,
        indata,
        outdata,
        size/TS);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "time NBodyNaive " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1.0e6 << "\n";
    //copy(outdata, data);
}

int main (int argc, char**argv) {

  NBody_NAIVE_AMP* nbody = new NBody_NAIVE_AMP();
  nbody->SetupOnce();
  nbody->Setup();
  nbody->Test();
  nbody->Cleanup();
  nbody->CleanupOnce();
  delete nbody;
  return 0;
}
