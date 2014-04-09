Summary:
 Samples for MCW C++AMP, mostly ported/revised from Microsoft C++AMP samples. 
 This file includes build instructions for binary packages of MCW C++AMP

Prerequisites:
 Ubuntu 12.04 or later
 Install the binary .deb packages:
   * clamp-<version>.deb and libcxxamp-<version>.deb, available from:
   https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads
 
 Have a working OpenCL environment. Binary package assumes either NVIDIA
 OpenCL or AMD APP SDK
 installed at default path (i.e. at either /usr/ for NV or /opt/AMDAPP for AMD)
 Have a 32-bit compilation environment even if you have a 64-bit
 environment. On Ubuntu 12.04, this can be achieved by:
	sudo apt-get install gcc-multilib libc6-i386 libc6-dev-i386
To compile:
 1. Run
    ./buildme.binary myfunctor.cc at the cxxamp_sandbox directory
    This produces the CPU path binary at myfunctor.cc.out
    Intermediate files include: 
       kernel.cl (generated GPU code in OpenCL)
       myfunctor.cc.ll (the LLVM IR code for GPU kernel)       
 3. Run the executable myfunctor.cc.out
    If it works, you will see "PASS". If it fails, there will be
    an assertion fault (means the kernel compiles, but no correct output
    is produced), or
    OpenCL compiler error meesage (means the kernel does not compile).

For each samples, please change into their respective directory and
build using the same buildme script. e.g.  to build/run HelloWorld:
  cd HelloWorld
  ../buildme.binary helloworld.cpp
  ./helloworld.cpp.out

Samples  
 1. Black-Scholes (directory: BlackScholes)
 2. BitonicSort (directory: BitonicSort)
 3. Binomial options (directory: Binomialoptions)
 4. Gaussian Blur (directory: gaussian_blur)
 5. Hello World (directory: HelloWorld)
 6. Matrix Multiplication (directory: MatrixMultiplication)
 7. Performance Measurement (directory: MeasurePerformance)
 8. Prefix Sum (directory: Scan)
 9. Transitive Closure (directory: TransitiveClosure)
 10. Matrix Transpose (directory: transpose)
 11. Histogramming (directory: histogram)
 12. Random Number Generator (directory: MersenneTwister)
