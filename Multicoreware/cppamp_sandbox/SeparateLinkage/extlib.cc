#include<amp.h>
#ifdef __GPU__
int myinc(int i) restrict(amp) {
  return i+1;
}
#else
int myinc(int i) restrict(cpu) {
  return 0;
}
#endif
