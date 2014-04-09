Transitive closure

-Overview:
What is transitive closure?
Consider a directed graph G=(V,E), where V is the set of vertices and E is the set of edges. The transitive closure of G is a graph G+ = (V,E+) such that for all v,w in V there is an edge (v,w) in E+ if and only if there is a non-null path from v to w in G.

Why and where is it needed?
Finding the transitive closure of a directed graph is an important problem in many computational tasks. It is required, for instance, in the reachability analysis of transition networks representing distributed and parallel systems and in the construction of parsing automata in compiler construction. Recently, efficient transitive closure computation has been recognized as a significant subproblem in evaluating recursive database queries, since almost all practical recursive queries are transitive.

-Hardware requirement:
This sample requires DirectX 11 capable card, if none detected sample will use DirectX 11 Reference Emulator.

-Software requirement:
Install Visual Studio 2012 from http://msdn.microsoft.com

-Running sample:
This sample explore and determines if graph vertexs' are connected. 
This samples also demonstrates a typical use of array i.e. use array when algorithm/application works in stages or phases and we dont have to bring down data after stages. If needed we have to use copy API explicit.

-References:
http://www.seas.upenn.edu/~kiderj/research/papers/APSP-gh08-fin-T.pdf
You can more information on transitive closure at http://en.wikipedia.org/wiki/Transitive_closure#Graph_theory. 
C++ AMP implementation is using Floyd–Warshall algorithm to explore space - http://en.wikipedia.org/wiki/Floyd%E2%80%93Warshall_algorithm                              
