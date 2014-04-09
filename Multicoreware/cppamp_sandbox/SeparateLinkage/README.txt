This directory contains an attempt to achieve separate compilation of MCW C++AMP codes

* Compile sources. Each one generates an ordinary .o and a .bc file holding kernels 

SeparateLinkage$ ./ampcc-c helloworld.cc
Compiling for GPU path; output is helloworld.cc.bc
Compiling for CPU path
SeparateLinkage$ ./ampcc-c helloworld.cc 
Compiling for GPU path; output is extlib.cc.bc
Compiling for CPU path

* Generate a kernel file out of all bitcode files
SeparateLinkage$ ./ampcc-kernelgen *.bc
Generate kernel from extlib.cc.bc helloworld.cc.bc; output C file is kernel.cl
Create embeddable kernel object kernel.o

* Link CPU and GPU objects and produce the final executable

SeparateLinkage$ ./ampcc-link helloworld.o extlib.o
Linking CPU and GPU objects

* Run it!
SeparateLinkage$ ./a.out
Hello world
