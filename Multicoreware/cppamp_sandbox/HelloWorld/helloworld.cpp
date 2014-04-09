// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
#include <iostream>
#include <amp.h>
using namespace concurrency;
int main() {
  int v[11] = {'G', 'd', 'k', 'k', 'n', 31, 'v', 'n', 'q', 'k', 'c'};

  array_view<int> av(11, v); 
  parallel_for_each(av.get_extent(), [=](index<1> idx) restrict(amp) {
    av[idx] += 1;
  });

  for(unsigned int i = 0; i < av.get_extent().size(); i++)
    std::cout << static_cast<char>(av(i));
  return 0;
}
