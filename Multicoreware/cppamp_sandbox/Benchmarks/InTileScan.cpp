// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll 
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl 
// RUN: pushd %t 
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
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
* C++ AMP In tile scan FLOAT perf test.
*
*---------------------------------------------------------------------------*/
#include <amp.h>

#define TS 256
using namespace Concurrency;

struct Scan_FLOAT_AMP // : AMPTestBase
{
    //Scan_FLOAT_AMP(TestContext &ctx, const wchar_t *category, const wchar_t *name)
    //    : AMPTestBase(ctx, category, name, true) { }
    void SetupOnce();
    void Setup();
    void Test();
    void Cleanup();
    void CleanupOnce();
    int size;
    float *Values;
    float *Result;
};


void InTileScanKernel_FLOAT_AMP_Kernel(
    uint size,
    array<float,1> &values,
    array<float,1> &result,
    int P1)
{
    parallel_for_each(
        extent<1>(P1*TS).tile<TS>(),
        [=, &values, &result] (tiled_index<TS> t_idx) restrict(amp)
        {
            tile_static float s_Data[2*TS];
            uint gindex = t_idx.global[0];
            uint lindex = t_idx.local[0];
            float input = values[gindex];
            float temp = 0;
            uint lpos = 2 * lindex - (lindex & (TS - 1));
            s_Data[lpos] = 0;
            lpos += TS;
            s_Data[lpos] = input;
        #if 1 
            for (uint offset = 1; offset < TS; offset = offset << 1)
            {
                //k_barrier;
                //barrier;
                temp = s_Data[lpos] + s_Data[lpos - offset];
                //k_barrier;
                //barrier;
                s_Data[lpos] = temp;
            }
        #endif
        #if 0  // uncomment this if want to test manual loop unrolling
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 1];
            k_barrier;
            s_Data[lpos] = temp;
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 2];
            k_barrier;
            s_Data[lpos] = temp;
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 4];
            k_barrier;
            s_Data[lpos] = temp;
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 8];
            k_barrier;
            s_Data[lpos] = temp;
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 16];
            k_barrier;
            s_Data[lpos] = temp;
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 32];
            k_barrier;
            s_Data[lpos] = temp;
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 64];
            k_barrier;
            s_Data[lpos] = temp;
            k_barrier;
            temp = s_Data[lpos] + s_Data[lpos - 128];
            k_barrier;
            s_Data[lpos] = temp;      
        #endif
            result[gindex] = s_Data[lpos];
        });
}

void Scan_FLOAT_AMP::SetupOnce()
{    
    size = 2097152; 
}

void Scan_FLOAT_AMP::Setup()
{
    assert(size % TS == 0);
    Values = new float[size];
    Result = new float[size];
    for (uint i = 0; i < size; i++)
    {
        Values[i] = (float)((i + 1) % 127);
        Result[i] = 0;
    }
}

void Scan_FLOAT_AMP::CleanupOnce()
{
}

void Scan_FLOAT_AMP::Cleanup()
{
    delete[] Values;
    delete[] Result;
}

void Scan_FLOAT_AMP::Test()
{
    accelerator ampAccelerator;
    // Allocate buffer values
    array<float,1> values(size, Values, ampAccelerator.get_default_view());
    // Allocate buffer result
    array<float,1> result(size, Result, ampAccelerator.get_default_view());
    // Begin @GEN_PRE_RUN_KERNEL(InTileScanKernel);
    InTileScanKernel_FLOAT_AMP_Kernel(
        size,
        values,
        result,
        1);
    // End @GEN_PRE_RUN_KERNEL(InTileScanKernel);
    std::cout << "Starting timer...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 1000; i++)
    {
        // Run kernel InTileScanKernel_FLOAT
        InTileScanKernel_FLOAT_AMP_Kernel(
            size,
            values,
            result,
            size/TS);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "time InTileScan " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1.0e6 << "\n";
    //Copy resutls back
}

int main (int argc, char**argv) {

  Scan_FLOAT_AMP *sfa = new Scan_FLOAT_AMP();
  sfa->SetupOnce();
  sfa->Setup();
  sfa->Test();
  sfa->Cleanup();
  sfa->CleanupOnce();
  delete sfa;
  return 0;
}
