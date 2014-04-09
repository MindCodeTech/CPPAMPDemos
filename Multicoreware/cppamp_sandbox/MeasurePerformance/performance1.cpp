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
    for(int i=0; i<mA.extent[1]; ++i) {
      index<2> idxA(idx[0], i);
      index<2> idxB(i, idx[1]);

      result += mA[idxA] * mB[idxB];
    }

    mC[idx] = result;
  });
}

int main() {
  for (int input=0; input<10; ++input) {
    const int M = 300 + 100 * input;
    const int N = 500 + 100 * input;
    const int W = 400 + 100 * input;

    vector<float> vA(M * N);
    vector<float> vB(N * W);
    vector<float> vC(M * W);
    Fill(vA);
    Fill(vB);

#if _WIN32
    Timer tAll;
    tAll.Start();
#elif __linux__
    struct timeval start,end;
    gettimeofday( &start, NULL );
#endif

    extent<2> eA(M, N), eB(N, W), eC(M, W);
    array<float, 2> mA(eA), mB(eB), mC(eC);

    copy(vA.begin(), mA);
    copy(vB.begin(), mB);
    ComputeMatrixMult(mC, mA, mB);
    copy(mC, vC.begin());

#if _WIN32
    tAll.Stop();

    std::cout << std::fixed << std::setprecision(2)
      << tAll.Elapsed() << " ms" << std::endl;
#elif __linux__   
    gettimeofday( &end, NULL );

    std::cout << std::fixed << std::setprecision(2)
      << 1000 * ( end.tv_sec - start.tv_sec ) +
         (end.tv_usec - start.tv_usec) / 1000.0f << " ms"
      << std::endl;
#endif
  }
  return 0;
}
