Description
-----------

This is a semi-practical application which calculates antenna radiation patterns
for simple wire antennas of varying lengths. The length of the antenna is
increased and decreased, leading to varying patterns. Each line in the output
display represents a different length antenna; tracing across a line represents
the radiation pattern of the antenna as swept over a 180 degree observation arc
at a distance from the antenna. Brighter colors represent stronger radiation.

A lightweight version of the Method of Moments (MoM) is employed for the
calculation. In this method, the length of the antenna is subdivided into small
pieces. The antenna is excited by a signal and the goal is to obtain the
distribution of currents on the antenna, from which the radiation pattern can
ready be calculated. Each segment of the antenna affects each other segment, and
so there are N x N interactions. Mathematically, a matrix can be constructed
where each line represents all of the interactions for a single element. This
matrix is thus N x N in size, and must be solved.

The solution of the matrix is a large portion of the computation, and is
performed with the LAPACK method GETRF for LU Factorization. This is then
followed by a series of other calls from the BLAS and LAPACK libraries (TRSM and
LASWP) to obtain the final current distribution.

The code presented contains code paths for both the CPU and C++ AMP. All steps
of the process are performed either with C++ AMP parallel statements, or with
OpenMP parallel CPU statements. 

Technical Details
-----------------

The code uses a variety of C++ AMP and C++11 features to perform its work:
* DirectX interoperability for use of textures in computation and display
* C++11 Threading, Smart Pointers, and Synchronization
* Interoperation with C++ AMP libraries

Compilation Instructions
------------------------

AMPLAPACK and AMPBLAS are required for compilation, as is a host LAPACK/BLAS
library. Injection of the host library is performed through environment
variables, as follows:

%LAPACK_LIB_FILES_64% - Semicolon separated list of filenames to link for 64-bit builds
%LAPACK_LIB_PATH_64% - Semicolon separated list of paths to search for the above files
%LAPACK_LIB_FILES_32% - Semicolon separated list of filenames to link for 32-bit builds
%LAPACK_LIB_PATH_32% - Semicolon separated list of paths to search for the above files

AMPLAPACK and AMPBLAS are available at the following URLs:

AMPLAPACK: http://amplapack.codeplex.com/
AMPBLAS: http://ampblas.codeplex.com/

Any LAPACK and BLAS implementation for the CPU host should be adequate for this
example, though it is recommended to use a fully threaded and optimized library.

Tuning and Configuration Instructions
-------------------------------------

The source file "window.cpp" contains at the top a series of tuning parameters,
as follows:

* width/height: Describe the initial size of the window and the number of samples
                which are maintained

* complexity: A number which alters the precision of a critical calculation;
              increasing this parameter dramatically increases the memory and
              amount of computation required per output line

A suggested course of action would be to set width and height to a pleasing
window size and to set complexity as large as possible without causing an
out-of-memory condition or slow scrolling.

Usage Instructions
------------------

Pressing any keyboard button while the example is running will toggle the
present solver between CPU and GPU. The current solver (CPU or GPU) and the
performance in the number of lines calculated per second are shown in the
titlebar.

