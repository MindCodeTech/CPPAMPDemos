//RUN: false
//XFAIL: *
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
* C++ AMP matrix mulitply perf test.
*
*---------------------------------------------------------------------------*/

#include <amp.h>
using namespace Concurrency;
#define TS 16

struct MatrixMultiply_DOUBLE_NAIVE_AMP
{
    //MatrixMultiply_DOUBLE_NAIVE_AMP(TestContext &ctx, const wchar_t *category, const wchar_t *name);
    //    : AMPTestBase(ctx, category, name, true) { }
    void SetupOnce();
    void Setup();
    void Test();
    void Cleanup();
    void CleanupOnce();
    int size;
    double *A, *B, *C;
};

void MatrixMultiplyKernel_DOUBLE_NAIVE_AMP_Kernel(
    int size,
    const array<double,2> &a,
    const array<double,2> &b,
    array<double,2> &c,
    int P1,
    int P2)
{
    parallel_for_each(
        extent<2>(P1*TS, P2*TS).tile<TS, TS>(),
        [=, &a, &b, &c] (tiled_index<TS, TS> t_idx) restrict(amp)
        {
            double sum = 0;
            for (int k = 0; k < size; k++)
            {
                sum += a(t_idx.global[0], k) * b(k, t_idx.global[1]);
            }
            c(t_idx.global[0], t_idx.global[1]) = sum;
        });
}

void MatrixMultiply_DOUBLE_NAIVE_AMP::SetupOnce()
{
    size = 512;
}

void MatrixMultiply_DOUBLE_NAIVE_AMP::Setup()
{
    assert(size % 16 == 0);
    A = new double[size*size];
    B = new double[size*size];
    C = new double[size*size];
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            A[i*size+j] = (i == j) ? 1 : 0;
            B[i*size+j] = i*size+j;
            C[i*size+j] = 0;
        }
    }
}

void MatrixMultiply_DOUBLE_NAIVE_AMP::CleanupOnce()
{
}

void MatrixMultiply_DOUBLE_NAIVE_AMP::Cleanup()
{
    delete[] A;
    delete[] B;
    delete[] C;
}

void MatrixMultiply_DOUBLE_NAIVE_AMP::Test()
{
    accelerator ampAccelerator;
    // Allocate buffer a
    array<double,2> a(size, size, A, ampAccelerator.get_default_view());
    // Allocate buffer b
    array<double,2> b(size, size, B, ampAccelerator.get_default_view());
    // Allocate buffer c
    array<double,2> c(size, size, C, ampAccelerator.get_default_view());
    int num_iters = 1;
    if(size <= 256)
    {
        num_iters = 1000;
    }
    else if(size <= 1024)
    {
        num_iters = 10;
    }
    int k = 0;
    // Begin @GEN_PRE_RUN_KERNEL(MatrixMultiplyKernel);
    MatrixMultiplyKernel_DOUBLE_NAIVE_AMP_Kernel(
        size,
        a,
        b,
        c,
        1,
        1);
    // End @GEN_PRE_RUN_KERNEL(MatrixMultiplyKernel);
    std::cout << "Starting timer...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < num_iters; iter++)
    {
            // Run kernel MatrixMultiplyKernel_DOUBLE_NAIVE
            MatrixMultiplyKernel_DOUBLE_NAIVE_AMP_Kernel(
                size,
                a,
                b,
                c,
                size/TS,
                size/TS);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "time MatrixMul " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1.0e6 << "\n";
}

int main (int argc, char**argv) {
  MatrixMultiply_DOUBLE_NAIVE_AMP *mdna = new MatrixMultiply_DOUBLE_NAIVE_AMP();
  mdna->SetupOnce();
  mdna->Setup();
  mdna->Test();
  mdna->Cleanup();
  mdna->CleanupOnce();
  delete mdna;
  return 0;
}
