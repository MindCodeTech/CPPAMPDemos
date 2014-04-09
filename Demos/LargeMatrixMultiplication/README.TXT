Large Matrix Multiplication

-Overview:
This sample Implements C++ AMP based matrix multiplication for large square matrices which cannot fit in the accelerator memory. 
It compares four different chunking strategies. Each stragey has different overhead to stream the data to the accelerator.
 
   1) Strategy 1: Chunking the matrices along the dimensions I, J and K. A, B, and C all have the same chunk size;
                  Streaming the matrices in the loop order ijk.
   2) Strategy 2: Chunking the matrices along the dimensions I and J. A and B have the same chunk size. 
                  Streaming A, B and C in the loop order ij.
   3) Strategy 3: A modification of the Strategy 2. It streams a large chunk of C to reduce the number of streamings
                  for C with the tradeoff not fully utilizing the accelerator memory allocated for C during each iteration
                  of the computation.
   4) Strategy 4: A modification of the Strategy 2. It streams A, B, and C in the loop order of ji, not ij. This is 
                  to utilize the memory layout of the matrices on the CPU assuming they are all row-major. 
                  This will avoid the overhead to pack the chunk of A before it is streamed to the accelerator.

-Hardware requirement:
This sample requires DirectX 11 capable card, if none detected, sample will use DirectX 11 Emulator.

-Software requirement:
Install Visual Studio Dev11 Beta from http://msdn.microsoft.com

-Running sample:
This sample implements chunking strategies for large matrix multiplication. 

-References:
http://en.wikipedia.org/wiki/Matrix_multiplication

