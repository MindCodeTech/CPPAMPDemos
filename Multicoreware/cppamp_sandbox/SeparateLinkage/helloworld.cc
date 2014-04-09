#include <iostream>
#include <amp.h>
extern int myinc(int i) restrict(amp);

using namespace concurrency;
int main() {
  int v[11] = {'G', 'd', 'k', 'k', 'n', 31, 'v', 'n', 'q', 'k', 'c'};

  array_view<int> av(11, v); 
  parallel_for_each(av.extent, [=](index<1> idx) restrict(amp) {
    av[idx] = myinc(av[idx]);
  });

  for(unsigned int i = 0; i < av.extent.size(); i++)
    std::cout << static_cast<char>(av(i));
}
