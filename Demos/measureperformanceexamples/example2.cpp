#include <amp.h>
#include <vector>
#include <random>
#include <iostream>
#include <iomanip>
#include "timer.h"

using namespace concurrency;
using std::vector;

void Fill(vector<float> &v,
          float min = -1 << 15,
          float max = 1 << 15)
{
    static std::mt19937 mersenne_twister_engine;
    std::uniform_real_distribution<float> uni(min, max);

    for(size_t i = 0; i < v.size(); ++i)
    {
        v[i] = uni(mersenne_twister_engine);
    }
}

void ComputeMatrixMult(array<float, 2> &mC,
                        array<float, 2> &mA,
                        array<float, 2> &mB)
{
    parallel_for_each(mC.extent,
                      [&](index<2> idx) restrict(amp) {
    
        float result = 0.0f;
        for(int i=0; i<mA.extent[1]; ++i)
        {
            index<2> idxA(idx[0], i);
            index<2> idxB(i, idx[1]);

            result += mA[idxA] * mB[idxB];
        }

        mC[idx] = result;
    });
}

void WarmUp()
{
    extent<2> eA(1, 1), eB(1, 1), eC(1, 1);
    array<float, 2> mA(eA), mB(eB), mC(eC);
    ComputeMatrixMult(mC, mA, mB);
}

void main()
{
    WarmUp(); // warm up runtime and JIT our kernel

    for (int input=0; input<10; ++input)
    {
        const int M = 300 + 100 * input;
        const int N = 500 + 100 * input;
        const int W = 400 + 100 * input;

        vector<float> vA(M * N);
        vector<float> vB(N * W);
        vector<float> vC(M * W);
        Fill(vA);
        Fill(vB);

        Timer tAll, tCopyIn, tCompute, tCopyOut;
        tAll.Start();

        extent<2> eA(M, N), eB(N, W), eC(M, W);
        array<float, 2> mA(eA), mB(eB), mC(eC);
  
        tCopyIn.Start();
        copy(vA.begin(), mA);
        copy(vB.begin(), mB);
        tCopyIn.Stop();
        
        tCompute.Start();
        ComputeMatrixMult(mC, mA, mB);
        mC.accelerator_view.wait();
        tCompute.Stop();

        tCopyOut.Start();
        copy(mC, vC.begin());
        tCopyOut.Stop();

        tAll.Stop();

        std::cout <<  std::fixed << std::setprecision(2)
            << "copy-in=" << tCopyIn.Elapsed() << " ms, "
            << "compute=" << tCompute.Elapsed() << " ms, "
            << "copy-out=" << tCopyOut.Elapsed() << " ms, "
            << "total=" << tAll.Elapsed() << " ms" << std::endl;
    }
}