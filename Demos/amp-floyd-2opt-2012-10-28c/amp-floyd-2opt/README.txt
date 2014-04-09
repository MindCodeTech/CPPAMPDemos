amp-floyd-2opt

This project explores serial and parallel implementations for
finding best known solutions to the Travelling Salesman Problem (TSP).

The programm runs in two phases:
The first phase optimizes a weight (distance) matrix using Floyd-Warshall.
The second phase selects tours from that Floyd-Warshall matrix using 
a simple Nearest-NeighBor algorithm. Once a better tour has been 
found we run a final 2-opt optimization on the selected tour.

There are serial (*_cpu) and parallel (*_gpu) versions included 
for Floyd-Warshall and all 2-opt algorithms. In addition to a traditional, 
"text-book" implementation of 2-opt I also added an interesting 2-opt 
implementation developed by Ioannis Mavroidis et al. from the Technical 
University of Crete (TUC). The two_opt_tuc_cpu and two_opt_tuc_gpu 
implementations usually produce less precize results in less time when 
compared to the traditional 2-opt methods.

It turns out that the combination of parallel Floyd-Warshall and 
serial 2-opt-TUC yielded the "best deal" for fastest calculation 
of best known solutions. For better results, which get closer to the 
optimal solution, I recommend using parallel Floyd-Warshall with 
traditional serial 2-opt optimization.

The syntax for amp-floyd-2opt is:

	amp-floyd-2opt [options] [tsp-file]

If you run amp-floyd-2opt without any parameters the program will ask 
you interactively for the parameters.

An overview over all command line arguments can be obtained via:

	amp-floyd-2opt -help

The defaults are set for fastest execution with less precize results 
(using parallel Floyd plus TUC's serial 2-opt):

	amp-floyd-2opt -use-gpu -use-two-opt-tuc-cpu

For better results but slower execution (using parallel Floyd 
plus serial text-book 2-opt) try:

	amp-floyd-2opt -use-gpu -use-two-opt-cpu

This project uses Microsoft's AMP (Accelerated Massive Parallelism) 
technology and requires Visual Studio 2012.

For license information please see MIT_LICENSE.txt.

September, 2012. 
Bernd Paradies
http://blogs.adobe.com/bparadie/

----------------------------------------------------------------------------

CONTENT

main.cpp

provides the application's main entry point. 


amp_menu.cpp, cmd_line_menu.cpp, helper_functions.cpp, 
loader.cpp, problem.cpp, timer.cpp

Those are files for readin TSP files and creating
weight matrices from the Solver Sample provided by the

Beyond3D's C++ AMP Contest 
http://www.beyond3d.com/content/articles/121/
http://rpg.sh/AMP_contest/Aco_in_amp_sample_v1.zip


TspRunner.cpp, TspSolver.cpp

glue code for the Beyond3D's C++ AMP Contest solver framework.


TspOptions.cpp

extends the Beyond3D's C++ AMP Contest solver framework with
command line arguments.


TspUtils.cpp

contains shared utility functions calculating tour lengths etc.


TspFloydSolver.cpp

contains the serial and parallel implementations of the Floyd-Warshall algorithm 
with path reconstruction enhancements. A second pass picks the shortest path 
by using a simple nearest-neighbor algorithm (see TspNearestNeighbor.cpp).
The cpu implementation is based on pseudo-code found at this Wikipedia page:
http://en.wikipedia.org/wiki/Floyd%E2%80%93Warshall_algorithm
The AMP implementation is based on: 
Transitive Closure Sample in C++ AMP,
http://blogs.msdn.com/b/nativeconcurrency/archive/2011/11/08/transitive-closure-sample-in-c-amp.aspx


TspNearestNeighbor.cpp

implements nearest-neighbor algorithm and runs 2-opt optimization on
candidate tours (see Tsp2Opt).


TspFindMinimum.cpp

contains serial and parallel implementations for finding the position of the 
minimum value within a one dimensional array.
Used by TspNearestNeighbor.

The AMP implementations are based on: 
Parallel Reduction using C++ AMP,
http://blogs.msdn.com/b/nativeconcurrency/archive/2011/11/02/matrix-multiplication-sample.aspx


Tsp2Opt.cpp

contains a serial, and parallel implementations of a text-book 2-opt and 
an alternative method developed by Ioannis Mavroidis et al. from the Technical 
University of Crete (TUC).

The serial version of the text-book 2-opt method is based on Pascal code 
published in M.M.Syslo, N.Deo, J.S.Kowalik, Discrete Optimization 
Algorithms with Pascal Programs, Prentice-Hall, Englewood 
Cliffs 1983.

Operations Research Software Exchange Program
http://optimierung.mathematik.uni-kl.de/old/ORSEP/contents.html

PASCAL Codes of Discrete Optimization Algorithms
EJOR 40/1 (1989), 120-127
ftp://www.mathematik.uni-kl.de/pub/Math/ORSEP/SYSLO.ZIP

The parallel version of the text-book 2-opt method is based on 

Accelerating 2-opt and 3-opt Local Search Using GPU in the Travelling Salesman Problem
http://www.researchgate.net/publication/229476110_Accelerating_2-opt_and_3-opt_Local_Search_Using_GPU_in_the_Travelling_Salesman_Problem

The serial and parallel implementations of 2-opt are based on:
Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
http://dl.acm.org/citation.cfm?id=1263915
http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1


TspGASolver.cpp

implements a genetic algorithm (GA) with 2-opt optimization based on 

A highly-parallel TSP solver for a GPU computing platform
http://dl.acm.org/citation.cfm?id=1945726
http://www.springerlink.com/content/n20570265p081734/fulltext.pdf

This solver is activated via -use-ga command line argument and run 
after floyd, nearest-neighbor, and 2-opt. Unfortunately TspGASolver does 
not significantly improve pre-optimized tours.

You can also run TspGASolver as a stand-alone solver with randomized tours:

	amp-floyd-2opt -use-ga -no-floyd

The default implementation uses a population size of 60 with 1000 generations
with mixed results.


