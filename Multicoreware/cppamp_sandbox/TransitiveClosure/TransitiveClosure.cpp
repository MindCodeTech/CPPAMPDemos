// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
//----------------------------------------------------------------------------
// File: TransitiveClosure.cpp
//
// Contains the implementation of algorithms which explores connectivity between 
// nodes in a graph and determine shortest path.
// This is based on paper http://www.seas.upenn.edu/~kiderj/research/papers/
//APSP-gh08-fin-T.pdf
//----------------------------------------------------------------------------

#include <amp.h>
#include <assert.h>
#include <iostream>

// Seed used to control random input data
const unsigned int random_seed = 42;
// Aids in debugging smaller graph - controls criteria to print out graph 
const unsigned int display_threshold = 16;
// Number of iterations to run - kernel is run specified number of time
const unsigned int num_iterations = 1;

// Constants used in the Graph representations
const unsigned int num_vertices = (1 << 4); // multiple of block_size for
                                            // simplicity 
// graph dimension
const unsigned int array_size = (num_vertices * num_vertices); 

// Constants - specifies tile size
#define TILE_SIZE (1 << 3)

// State of connection
#define UNCONNECTED 0
#define DIRECTLY_CONNECTED 1
#define INDIRECTLY_CONNECTED 2

using namespace concurrency;

//----------------------------------------------------------------------------
// Wrapper to control displaying/outputing graph result
//----------------------------------------------------------------------------
inline bool to_display() {
  return num_vertices <= display_threshold;
}

void copy(std::vector<unsigned int> &v,
          Concurrency::array_view<unsigned int, 1> &av) {
  for(size_t i = 0; i < v.size(); ++i) {
    av[i] = v[i];
  }
}

void copy(Concurrency::array_view<unsigned int, 1> &av,
          std::vector<unsigned int> &v) {
  for(size_t i = 0; i < v.size(); ++i) {
    v[i] = av[i];
  }
}

//----------------------------------------------------------------------------
// Generate random input graph connectivity data
//----------------------------------------------------------------------------
unsigned int *generate_input_graph() {
  unsigned int *graph = new unsigned int[array_size];

  for (unsigned int i = 0; i < num_vertices; ++i) {
    for (unsigned int j = 0; j < num_vertices; ++j) {
      if (i == j) {
        graph[i*num_vertices + j] = DIRECTLY_CONNECTED;
      }
      else {
        // Toss a coin to decide if we want an edge between these vertices
        if ( ((rand() % 2) == 0) && ((rand() % 2) == 0) ) {
          graph[i*num_vertices + j] = DIRECTLY_CONNECTED;
        }
        else {
          // No edge between these vertices
          graph[i*num_vertices + j] = UNCONNECTED;
        }
      }
    }
  }

  return graph;
}

//----------------------------------------------------------------------------
// Get graph vertex information - state of connection and the indirect path node
//----------------------------------------------------------------------------
void get_fields(const unsigned int graph_element,
                unsigned int &connection_field, int &path_field) {
  connection_field = UNCONNECTED;
  if (graph_element != UNCONNECTED) {
    connection_field = (graph_element == DIRECTLY_CONNECTED)?
                       DIRECTLY_CONNECTED : INDIRECTLY_CONNECTED;
  }

  path_field = -1;
  if (connection_field == INDIRECTLY_CONNECTED) {
    path_field = graph_element - INDIRECTLY_CONNECTED;
  }
}

//----------------------------------------------------------------------------
// Display resulting graph information - specifies each elements in terms its 
// state of connection and its cost
//----------------------------------------------------------------------------
void display_graph(const unsigned int *graph, bool show_path_field = true) {
  for (unsigned int i = 0; i < num_vertices; ++i) {
    for (unsigned int j = 0; j < num_vertices; ++j) {
      unsigned int connection_field;
      int path_field;
      get_fields(graph[i*num_vertices + j], connection_field, path_field);

      printf (" %d", connection_field);
      if (show_path_field) {
        printf(", %d", path_field);
      }
    }

    printf("\n");
  }
}

//----------------------------------------------------------------------------
// This function help verify correctness or inconsistency between CPU and GPU 
// version of implementation. 
// Verify only vertices connectivity state
//----------------------------------------------------------------------------
bool graphs_match(const unsigned int *graph1, const unsigned int *graph2) {
  if ((graph1 == NULL) || (graph2 == NULL)) {
    printf("One or more of the graphs being matched is NULL!\n\n");
    return false;
  }

  for (unsigned int i = 0; i < num_vertices; ++i) {
    for (unsigned int j = 0; j < num_vertices; ++j) {
      unsigned int g1_connection_field;
      int g1_path_field = -1;
      get_fields(graph1[i*num_vertices + j], g1_connection_field,
                 g1_path_field);

      unsigned int g2_connection_field;
      int g2_path_field = -1;
      get_fields(graph2[i*num_vertices + j], g2_connection_field,
                 g2_path_field);

      if (g1_connection_field != g2_connection_field) {
        return false;
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
// This function determines connectivity between source and destination vertex 
//----------------------------------------------------------------------------
bool does_path_exist(unsigned int srcvtx, unsigned int destvtx,
                     const unsigned int *transitive_closure_graph,
                     unsigned int maxpathlength) {
  // validate vertices
  if ( (srcvtx >= num_vertices) || (destvtx >= num_vertices) ) {
    return false;
  }

  if (srcvtx == destvtx) {
    return true;
  }

  if (maxpathlength == 0) {
    return false;
  }

  if (maxpathlength == 1) {
    // There must be a direct edge between the vertices
    return (transitive_closure_graph[srcvtx*num_vertices + destvtx] ==
            DIRECTLY_CONNECTED);
  }

  unsigned int connection_field;
  int path_field;
  get_fields(transitive_closure_graph[srcvtx*num_vertices + destvtx],
             connection_field, path_field);

  switch (connection_field) {
    case UNCONNECTED:
      return false;
    case DIRECTLY_CONNECTED:
      return true;
    case INDIRECTLY_CONNECTED: // 
      return ( does_path_exist(srcvtx, path_field, transitive_closure_graph,
               maxpathlength - 1) && does_path_exist(path_field, destvtx,
               transitive_closure_graph, maxpathlength - 1) );
  }

  return false;
}

//----------------------------------------------------------------------------
// This function verifies if two graphs have correct connectivity state and 
// may also verify its path
//----------------------------------------------------------------------------
bool verify_transitive_closure(const unsigned int *reference_result, 
                               const unsigned int *actual_result,
                               bool match_path_field /* = true */) {
  if (!graphs_match(reference_result, actual_result)) {
    return false;
  }

  if (match_path_field) {
    // Now lets verify if the paths in the actual Result are correct
    // For each connection marked as INDIRECTLY_CONNECTED there must
    // be an actual path of 
    // length less than num_vertices;
    for (unsigned int i = 0; i < num_vertices; ++i) {
      for (unsigned int j = 0; j < num_vertices; ++j) {
        unsigned int connection_field;
        int path_field;
        get_fields(actual_result[i*num_vertices + j],
                   connection_field, path_field);
        if ( (connection_field == INDIRECTLY_CONNECTED) && !does_path_exist(i,
             j, actual_result, num_vertices - 1) ) {
          return false;
        }
      }
    }
  }

  return true;
}


//----------------------------------------------------------------------------
// Naive CPU implementation of Transitive Closure algorithm
//----------------------------------------------------------------------------
void transitiveclosure_cpu_naive(unsigned int *graph) {
  for (unsigned int k = 0; k < num_vertices; ++k) {
    for (unsigned int i = 0; i < num_vertices; ++i) {
      for (unsigned int j = 0; j < num_vertices; ++j) {
        if ( graph[i*num_vertices + j] == UNCONNECTED) {
          if ( (graph[i*num_vertices + k] != UNCONNECTED) &&
               (graph[k*num_vertices + j] != UNCONNECTED) ) {
            graph[i*num_vertices + j] = k + INDIRECTLY_CONNECTED;
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// C++ AMP (GPU) implementation entry pointunsigned int, 2
//----------------------------------------------------------------------------
void transitiveclosure_amp_tiled(std::vector<unsigned int> &graph) {
  Concurrency::extent<1> e(num_vertices), eA(array_size);
  Concurrency::array<unsigned int, 1> mA(eA);
  Concurrency::array_view<unsigned int> ga(mA);
  
  copy(graph, ga);
  
  for (unsigned int k = 0; k < num_vertices; ++k) {
    for (unsigned int i = 0; i < num_vertices; ++i) {
      Concurrency::parallel_for_each(e,
          [=](Concurrency::index<1> idx) restrict(amp) {
        unsigned int j = idx[0];
        if ( ga[i*num_vertices + j] == UNCONNECTED) {
          if ( (ga[i*num_vertices + k] != UNCONNECTED) &&
               (ga[k*num_vertices + j] != UNCONNECTED) ) {
            ga[i*num_vertices + j] = k + INDIRECTLY_CONNECTED;
          }
        }
      });
    }
  }
  copy(ga, graph);
}

int main() {
  srand(random_seed);
  printf("Generating input data..");
  unsigned int *source_graph = generate_input_graph();
  printf("Done\n");

  printf("Running the Transitive Closure Sample...\n");
  printf("Number of Vertices = %d\n", num_vertices);
  printf("Iterations = %d\n", num_iterations);
  printf("\n");

  // Now run the CPU and GPU algorithms and get performance results
  std::vector<unsigned int> cpu_transitiveclosure_graph(array_size);
  std::vector<unsigned int> gpu_transitiveclosure_graph(array_size);
  std::vector<unsigned int> gpu_datatransfer_graph(array_size); 

  // Compute on CPU
  memcpy(cpu_transitiveclosure_graph.data(), source_graph, 
         sizeof(unsigned int) * array_size);

  printf("Computation on CPU...");
  transitiveclosure_cpu_naive(cpu_transitiveclosure_graph.data());
  printf("Done\n");

  // Compute on GPU
  memcpy(gpu_transitiveclosure_graph.data(), source_graph,
         sizeof(unsigned int) * array_size);

  printf("Computation on GPU...");
  transitiveclosure_amp_tiled(gpu_transitiveclosure_graph);
  printf("Done\n");

  printf("\n\n");
  // Verify correctness of compute results
  bool passed = verify_transitive_closure(cpu_transitiveclosure_graph.data(),
           gpu_transitiveclosure_graph.data(), true);
  if (!passed) {
    printf("Compute Result - INCORRECT!\n\n");
  }

  if (to_display()) {
    printf("------------------Source Graph--------------------------------\n");
    display_graph(source_graph);
    printf("----------------------------------------------------------\n\n\n");

    printf("------------------CPU Transitive Closure Graph----------------\n");
    display_graph(cpu_transitiveclosure_graph.data());
    printf("----------------------------------------------------------\n\n\n");

    printf("------------------GPU Transitive Closure Graph----------------\n");
    display_graph(gpu_transitiveclosure_graph.data());
    printf("----------------------------------------------------------\n\n\n");
  }

  // cleanup
  printf("Releasing resources...\n\n");
  delete [] source_graph;
  return 0;
}
