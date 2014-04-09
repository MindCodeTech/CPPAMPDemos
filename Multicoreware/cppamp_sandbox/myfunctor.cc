#include "amp.h"
#include <cassert>
#include <iostream>
class myVecAdd {
 public:
  // CPU-side constructor. Written by the user
  myVecAdd(Concurrency::array_view<float>& a,
    Concurrency::array_view<float> &b,
    Concurrency::array_view<float> &c):
    a_(a), b_(b), c_(c) {
  }
  void operator() (Concurrency::index<1> idx) restrict(amp) {
    c_[idx] = a_[idx]+b_[idx];
  }
 private:
  Concurrency::array_view<float> a_, b_, c_;
};

int main(int argc, char** argv) {
  const int vecSize = 16*1024*1024;

  // Alloc & init input data
  Concurrency::extent<1> e(vecSize);
  Concurrency::array<float, 1> a(vecSize);
  Concurrency::array<float, 1> b(vecSize);
  Concurrency::array<float, 1> c(vecSize);
  Concurrency::array_view<float> ga(a);
  Concurrency::array_view<float> gb(b);
  Concurrency::array_view<float> gc(c);

  double sum = 0.f;
  for (Concurrency::index<1> i; i[0] < vecSize; i++) {
    ga[i] = 1.0f * rand() / RAND_MAX;
    gb[i] = 1.0f * rand() / RAND_MAX;
    sum += ga[i] + gb[i];
  }

  myVecAdd mf(ga, gb, gc);
  Concurrency::parallel_for_each(e, mf);

  double error = 0.f;
  double check = 0.f;
  for(unsigned i = 0; i < vecSize; i++) {
    error += gc[i] - (ga[i] + gb[i]);
    check += gc[i];
  }
  assert(check - sum < 0.001);
  std::cout << "PASS\n";
  return 0;
}
