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
* C++ AMP BinomialOptions perf test.
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

#include <iostream>

using namespace Concurrency;


struct BinomialOptions_256_AMP
{
    void SetupOnce();
    void Setup();
    void Test();
    void Cleanup();
    int numOptions;
    float *h_S;
    float* h_X;
    float* h_vDt;
    float* h_puByDf;
    float* h_pdByDf;
    float* h_callValue;
	float* h_callBuffer;
};


#define  BNO_NUM_STEPS     (2048)
#define  BNO_TIME_STEPS    (16)
#define  BNO_TILE_DELTA    (2 * BNO_TIME_STEPS)
#define  BNO_TILE_SIZE     (256)
#define  BNO_TILE_STEP     (BNO_TILE_SIZE - BNO_TILE_DELTA)

struct TOptionData {
    float S;
    float X;
    float T;
    float R;
    float V;
};

static TOptionData* optionsData = NULL;
inline float randData(float lo, float hi) 
{
   int len = (int) ::ceil(hi - lo);
   return lo + ::rand() % len;
}

float expiryCallValue_256_AMP(float S, float X, float vDt, int i) restrict(amp)
{
    float d = S * fast_math::expf(vDt * (2.0f * i - BNO_NUM_STEPS)) - X;
    return (d > 0) ? d : 0;
}

void BinomialOptionsKernel_256_AMP_Kernel(
    int numOptions,
    const array_view<float,1> d_S,
    const array_view<float,1> d_X,
    const array_view<float,1> d_vDt,
    const array_view<float,1> d_puByDf,
    const array_view<float,1> d_pdByDf,
    array_view<float,1> d_callValue,
    array_view<float,1> d_callBuffer,
    int P1)
{
    parallel_for_each(
        extent<1>(P1*BNO_TILE_SIZE).tile<BNO_TILE_SIZE>(),
        [=] (tiled_index<BNO_TILE_SIZE> t_idx) restrict(amp)
        {
            tile_static float s_callA[BNO_TILE_SIZE+1];
            tile_static float s_callB[BNO_TILE_SIZE+1];
            int tid = t_idx.local[0];
        #define d_Call(arg) d_callBuffer[t_idx.tile[0] * (BNO_NUM_STEPS + 16) + (arg)]
            for(int i = tid; i <= BNO_NUM_STEPS; i += BNO_TILE_SIZE)
            {
                d_Call(i) = expiryCallValue_256_AMP(d_S[t_idx.tile[0]], d_X[t_idx.tile[0]], d_vDt[t_idx.tile[0]], i);
            }
            for(int i = BNO_NUM_STEPS; i > 0; i -= BNO_TILE_DELTA)
            {
                for(int c_base = 0; c_base < i; c_base += BNO_TILE_STEP)
                {
                    int c_start = (BNO_TILE_SIZE - 1 < i - c_base) ? BNO_TILE_SIZE - 1 : i - c_base;
                    int c_end   = c_start - BNO_TILE_DELTA;
                    t_idx.barrier.wait();
                    if(tid <= c_start)
                    {
                        s_callA[tid] = d_Call(c_base + tid);
                    }
                    for(int k = c_start - 1; k >= c_end; k -= 2)
                    {
                        t_idx.barrier.wait();
                        s_callB[tid] = d_puByDf[t_idx.tile[0]] * s_callA[tid + 1] + d_pdByDf[t_idx.tile[0]] * s_callA[tid];
                        t_idx.barrier.wait();
                        s_callA[tid] = d_puByDf[t_idx.tile[0]] * s_callB[tid + 1] + d_pdByDf[t_idx.tile[0]] * s_callB[tid];
                    }
                    t_idx.barrier.wait();
                    if(tid <= c_end)
                    {
                        d_Call(c_base + tid) = s_callA[tid];
                    }
                }
            }
            if (tid == 0) 
            {
                d_callValue[t_idx.tile[0]] = s_callA[0];
            }
        });
}

void BinomialOptions_256_AMP::SetupOnce()
{    
    numOptions = 2048;    
}

void BinomialOptions_256_AMP::Setup()
{
    optionsData = new TOptionData[numOptions];
    h_S = new float[numOptions];
    h_X = new float[numOptions];
    h_vDt = new float[numOptions];
    h_puByDf = new float[numOptions];
    h_pdByDf = new float[numOptions];
    h_callValue = new float[numOptions];
	h_callBuffer = new float[numOptions*(BNO_NUM_STEPS + 16)];
    srand(123);
    for(int i = 0; i < numOptions; i++) 
    {
        optionsData[i].S = randData(5.0f, 30.0f);
        optionsData[i].X = randData(1.0f, 100.0f);
        optionsData[i].T = randData(0.25f, 10.0f);
        optionsData[i].R = 0.06f;
        optionsData[i].V = 0.10f;
        h_S[i] =      optionsData[i].S;
        h_X[i] =      optionsData[i].X;
        const float      T = optionsData[i].T;
        const float      R = optionsData[i].R;
        const float      V = optionsData[i].V;
        const float   dt = T / BNO_NUM_STEPS;
        h_vDt[i] = V * sqrt(dt);
        const float   rDt = R * dt;
        const float   If = exp(rDt);
        const float   Df = exp(-rDt);
        const float   u = exp(h_vDt[i]);
        const float   d = exp(-h_vDt[i]);
        const float   pu = (If - d) / (u - d);
        const float   pd = 1.0f - pu;
        h_puByDf[i] = pu * Df;
        h_pdByDf[i] = pd * Df;
    }    
    //assert(BNO_NUM_STEPS % BNO_TILE_DELTA == 0);
}

void BinomialOptions_256_AMP::Cleanup()
{
    delete[]  h_S;
    delete[]  h_X;
    delete[]  h_vDt;
    delete[]  h_puByDf;
    delete[]  h_pdByDf;
    delete[]  h_callValue;
	delete[]  h_callBuffer;
    delete[]  optionsData;
}

void BinomialOptions_256_AMP::Test()
{
    const int numRuns = 16;
    // Allocate buffer d_S
    array_view<float,1> d_S(extent<1>(numOptions),h_S);
    // Allocate buffer d_X
    array_view<float,1> d_X(extent<1>(numOptions),h_X);
    // Allocate buffer d_vDt
    array_view<float,1> d_vDt(extent<1>(numOptions),h_vDt);
    // Allocate buffer d_puByDf
    array_view<float,1> d_puByDf(extent<1>(numOptions),h_puByDf);
    // Allocate buffer d_pdByDf
    array_view<float,1> d_pdByDf(extent<1>(numOptions),h_pdByDf);
    // Allocate buffer d_callValue
    array_view<float,1> d_callValue(extent<1>(numOptions),h_callValue);
    // Allocate buffer d_callBuffer
    array_view<float,1> d_callBuffer(extent<1>(numOptions*(BNO_NUM_STEPS + 16)),h_callBuffer);
    // Begin @GEN_PRE_RUN_KERNEL(BinomialOptionsKernel);
    BinomialOptionsKernel_256_AMP_Kernel(
        numOptions,
        d_S,
        d_X,
        d_vDt,
        d_puByDf,
        d_pdByDf,
        d_callValue,
        d_callBuffer,
        1);
    // End @GEN_PRE_RUN_KERNEL(BinomialOptionsKernel);
    std::cout << "Starting timer...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < numRuns; iter++)
    {
        // Run kernel BinomialOptionsKernel_256
        BinomialOptionsKernel_256_AMP_Kernel(
            numOptions,
            d_S,
            d_X,
            d_vDt,
            d_puByDf,
            d_pdByDf,
            d_callValue,
            d_callBuffer,
            numOptions);
    }
    //Synchronize array_views to force kernels to execute
    d_callValue.synchronize();
    d_callBuffer.synchronize();

    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "time BinomialOptions " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1e6f << "\n";
    //copy(d_callValue, h_callValue);
}

int main (int argc, char**argv) {
  long int num_runs = 1;

  if (argc > 1) {
    char* end;
    num_runs = strtol(argv[1], &end, 10);
    if (end == argv[1]) {
      printf("Expected integer number of runs as first argument.\n");
      exit(-1);
    }
  }

  BinomialOptions_256_AMP* binOpt = new BinomialOptions_256_AMP();
  binOpt->SetupOnce();
  for (int i=0; i<num_runs; ++i) {
    binOpt->Setup();
    binOpt->Test();
    binOpt->Cleanup();
  }
  delete binOpt;
  return 0;
}
