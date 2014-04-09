CUDAInterOp

-Overview:
This samples demostrates how to use CUDA libraries from C++ AMP.
 > amp_cuda.h contains utilities for C++ AMP/CUDA interop via C++ AMP/D3D11 interop and CUDA/D3D11 interop.
 > Example.cpp demonstrates the use of utilities from amp_cuda.h with two examples
   + use library function curandGenerateUniform from CURAND
   + use library function cublasSgemm from CUBLAS

-Hardware requirement:
This sample requires DirectX 11 and CUDA capable card, and CUDA capable driver.

-Software requirement:
Install Visual Studio 2012 from http://msdn.microsoft.com
Install CUDA 4.2 for Windows from http://developer.nvidia.com/cuda-downloads

-Note:
In "Properties -> Configuration Properties -> VC++ Directories", the following properties needs to be set 
to correct locations for the CUDA 4.2 installation
 > Executable Directories
 > Include Directories
 > Library Directories
