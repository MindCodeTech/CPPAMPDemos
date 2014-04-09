Summary:
 Samples for MCW C++AMP, mostly ported/revised from Microsoft C++AMP
 samples. This file includes build instructions for build from
 MCW C++AMP source. If you downloaded binary packages, please look at
 README.BINARY.txt

Prerequisites:
 Have cppamp-driver-ng compiled 
 Have a working OpenCL environment with 64-bit FP capability (cl_khr_fp64)
 Have a 32-bit compilation environment even if you have a 64-bit
 environment. On Ubuntu 12.04, this can be achieved by:
	sudo apt-get install gcc-multilib libc6-i386 libc6-dev-i386
To compile:
 1. Edit the shell script "buildme" for your cppamp-driver source and
    build. It assumes an AMD APP SDK installed at default path or
    /opt/AMDAPP 
 2. Run
    ./buildme myfunctor.cc at the cxxamp_sandbox directory
    This produces the CPU path binary at myfunctor.cc.out
    Intermediate files include: 
       kernel.cl (generated GPU code in either OpenCL-C or SPIR)
       myfunctor.cc.ll (the LLVM IR code for GPU kernel)
 3. Run the executable myfunctor.cc.out
    If it works, you will see "PASS". If it fails, there will be an
    assertion fault (means the kernel compiles, but no correct output
    is produced), or OpenCL compiler error meesage (means the kernel
    does not compile).

For each samples, please change into their respective directory and
build using the same buildme script. e.g. to build/run BlackScholes:
  cd BlackScholes
  ../buildme BlackScholes.cpp
  ./BlackScholes.cpp.out

Samples  
 1. Black-Scholes (directory: BlackScholes)
 2. BitonicSort (directory: BitonicSort)
 3. Binomial options (directory: Binomialoptions)
 4. Gaussian Blur (directory: gaussian_blur)
 5. Hello World (directory: HelloWorld)
 6. Matrix Multiplication (directory: MatrixMultiplication)
 7. Performance Measurement (directory: MeasurePerformance)
 8. Prefix Sum (directory: Sacn)
 9. Transitive Closure (directory: TransitiveClosure)
 10. Matrix Transpose (directory: transpose)
 11. Histogramming (directory: histogram)
 12. Random Number Generator (directory: MersenneTwister)
