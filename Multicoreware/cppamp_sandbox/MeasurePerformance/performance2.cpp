//RUN: false
//XFAIL: *
#include <amp.h>
#include <vector>
#include <random>
#include <iostream>
#include <iomanip>

#if _WIN32
#include "timer.h"
#elif __linux__
#include <sys/time.h>
#endif

using namespace concurrency;
using std::vector;

void Fill(vector<float> &v,
          float min = -1 << 15,
          float max = 1 << 15) {
  static std::mt19937 mersenne_twister_engine;
  std::uniform_real_distribution<float> uni(min, max);

  for(size_t i = 0; i < v.size(); ++i) {
    v[i] = uni(mersenne_twister_engine);
  }
}

void ComputeMatrixMult(array<float, 2> &mC,
                        array<float, 2> &mA,
                        array<float, 2> &mB) {
  parallel_for_each(mC.extent,
                    [&](index<2> idx) restrict(amp) {
    float result = 0.0f;
    for(int i = 0; i < mA.extent[1]; ++i) {
      index<2> idxA(idx[0], i);
      index<2> idxB(i, idx[1]);

      result += mA[idxA] * mB[idxB];
    }

    mC[idx] = result;
  });
}

void WarmUp() {
  extent<2> eA(1, 1), eB(1, 1), eC(1, 1);
  array<float, 2> mA(eA), mB(eB), mC(eC);
  ComputeMatrixMult(mC, mA, mB);
}

int main() {
  WarmUp(); // warm up runtime and JIT our kernel

  for(int input = 0; input < 10; ++input) {
    const int M = 300 + 100 * input;
    const int N = 500 + 100 * input;
    const int W = 400 + 100 * input;

    vector<float> vA(M * N);
    vector<float> vB(N * W);
    vector<float> vC(M * W);
    Fill(vA);
    Fill(vB);

#if _WIN32
    Timer tAll, tCopyIn, tCompute, tCopyOut;
    tAll.Start();
#elif __linux__
    struct timeval tAllStart, tAllEnd, tCopyInStart,tCopyInEnd,
                   tComputeStart,tComputeEnd, tCopyOutStart, tCopyOutEnd;
    gettimeofday( &tAllStart, NULL );
#endif

    extent<2> eA(M, N), eB(N, W), eC(M, W);
    array<float, 2> mA(eA), mB(eB), mC(eC);

#if _WIN32
    tCopyIn.Start();
#elif __linux__
    gettimeofday( &tCopyInStart, NULL );
#endif
    copy(vA.begin(), mA);
    copy(vB.begin(), mB);

#if _WIN32
    tCopyIn.Stop();
    tCompute.Start();
#elif __linux__
    gettimeofday( &tCopyInEnd, NULL );
    gettimeofday( &tComputeStart, NULL );
#endif

    ComputeMatrixMult(mC, mA, mB);
    mC.get_accelerator_view().wait();
#if _WIN32
    tCompute.Stop();
    tCopyOut.Start();
#elif __linux__
    gettimeofday( &tComputeEnd, NULL );
    gettimeofday( &tCopyOutStart, NULL );
#endif

    copy(mC, vC.begin());
#if _WIN32
    tCopyOut.Stop();
    tAll.Stop();
#elif __linux__
    gettimeofday( &tCopyOutEnd, NULL );
    gettimeofday( &tAllEnd, NULL );
#endif
#if _WIN32 
    std::cout <<  std::fixed << std::setprecision(2)
        << "copy-in=" << tCopyIn.Elapsed() << " ms, "
        << "compute=" << tCompute.Elapsed() << " ms, "
        << "copy-out=" << tCopyOut.Elapsed() << " ms, "
        << "total=" << tAll.Elapsed() << " ms" << std::endl;
#elif __linux__
    std::cout <<  std::fixed << std::setprecision(2)
        << "copy-in=" << 1000 * ( tCopyInEnd.tv_sec - tCopyInStart.tv_sec ) +
        (tCopyInEnd.tv_usec - tCopyInStart.tv_usec) / 1000.0f << " ms, "
        << "compute=" << 1000 * ( tComputeEnd.tv_sec - tComputeStart.tv_sec ) +
        (tComputeEnd.tv_usec - tComputeStart.tv_usec)  / 1000.0f << " ms, "
        << "copy-out=" << 1000 * ( tCopyOutEnd.tv_sec - tCopyOutStart.tv_sec ) +
        (tCopyOutEnd.tv_usec - tCopyOutStart.tv_usec)  / 1000.0f << " ms, "
        << "total=" <<1000 * ( tAllEnd.tv_sec - tAllStart.tv_sec ) +
        (tAllEnd.tv_usec - tAllStart.tv_usec)  / 1000.0f << " ms"
        << std::endl;
#endif

  }
  return 0;
}
