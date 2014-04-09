//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: TransitiveClosure.cpp
//
// Contains the implementation of algorithms which explores connectivity between 
// nodes in a graph and determine shortest path.
// This is based on paper http://www.seas.upenn.edu/~kiderj/research/papers/APSP-gh08-fin-T.pdf
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
const unsigned int num_vertices = (1 << 10); // multiple of block_size for simplicity 
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
inline bool to_display()
{
    return num_vertices <= display_threshold;
}

//----------------------------------------------------------------------------
// Generate random input graph connectivity data
//----------------------------------------------------------------------------
unsigned int *generate_input_graph()
{
    unsigned int *graph = new unsigned int[array_size];

    for (unsigned int i = 0; i < num_vertices; ++i)
    {
        for (unsigned int j = 0; j < num_vertices; ++j)
        {
            if (i == j)
            {
                graph[i*num_vertices + j] = DIRECTLY_CONNECTED;
            }
            else 
            {
                // Toss a coin to decide if we want an edge between these vertices
                if ( ((rand() % 2) == 0) && ((rand() % 2) == 0) )
                {
                    graph[i*num_vertices + j] = DIRECTLY_CONNECTED;
                }
                else
                {
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
void get_fields(const unsigned int graph_element, unsigned int &connection_field, int &path_field)
{
    connection_field = UNCONNECTED;
    if (graph_element != UNCONNECTED)
    {
        connection_field = (graph_element == DIRECTLY_CONNECTED)? DIRECTLY_CONNECTED : INDIRECTLY_CONNECTED;
    }

    path_field = -1;
    if (connection_field == INDIRECTLY_CONNECTED)
    {
        path_field = graph_element - INDIRECTLY_CONNECTED;
    }
}

//----------------------------------------------------------------------------
// Display resulting graph information - specifies each elements in terms its 
// state of connection and its cost
//----------------------------------------------------------------------------
void display_graph(const unsigned int *graph, bool show_path_field = true)
{
    for (unsigned int i = 0; i < num_vertices; ++i)
    {
        for (unsigned int j = 0; j < num_vertices; ++j)
        {
            unsigned int connection_field;
            int path_field;
            get_fields(graph[i*num_vertices + j], connection_field, path_field);

            printf ("    %d", connection_field);
            if (show_path_field)
            {
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
bool graphs_match(const unsigned int *graph1, const unsigned int *graph2)
{
    if ((graph1 == NULL) || (graph2 == NULL))
    {
        printf("One or more of the graphs being matched is NULL!\n\n");
        return false;
    }

    for (unsigned int i = 0; i < num_vertices; ++i)
    {
        for (unsigned int j = 0; j < num_vertices; ++j)
        {
            unsigned int g1_connection_field;
            int g1_path_field = -1;
            get_fields(graph1[i*num_vertices + j], g1_connection_field, g1_path_field);

            unsigned int g2_connection_field;
            int g2_path_field = -1;
            get_fields(graph2[i*num_vertices + j], g2_connection_field, g2_path_field);

            if (g1_connection_field != g2_connection_field)
            {
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
                   unsigned int maxpathlength)
{
    // validate vertices
    if ( (srcvtx >= num_vertices) || (destvtx >= num_vertices) )
    {
        return false;
    }

    if (srcvtx == destvtx)
    {
        return true;
    }

    if (maxpathlength == 0)
    {
        return false;
    }

    if (maxpathlength == 1)
    {
        // There must be a direct edge between the vertices
        return (transitive_closure_graph[srcvtx*num_vertices + destvtx] == DIRECTLY_CONNECTED);
    }

    unsigned int connection_field;
    int path_field;
    get_fields(transitive_closure_graph[srcvtx*num_vertices + destvtx], connection_field, path_field);

    switch (connection_field) 
    {
    case UNCONNECTED:
        return false;
    case DIRECTLY_CONNECTED:
        return true;
    case INDIRECTLY_CONNECTED: // 
        return ( does_path_exist(srcvtx, path_field, transitive_closure_graph, maxpathlength - 1) && 
                 does_path_exist(path_field, destvtx, transitive_closure_graph, maxpathlength - 1) );
    }

    return false;
}

//----------------------------------------------------------------------------
// This function verifies if two graphs have correct connectivity state and 
// may also verify its path
//----------------------------------------------------------------------------
bool verify_transitive_closure(const unsigned int *reference_result, 
                             const unsigned int *actual_result,
                             bool match_path_field /* = true */)
{
    if (!graphs_match(reference_result, actual_result))
    {
        return false;
    }

    if (match_path_field)
    {
        // Now lets verify if the paths in the actual Result are correct
        // For each connection marked as INDIRECTLY_CONNECTED there must be an actual path of 
        // length less than num_vertices;
        for (unsigned int i = 0; i < num_vertices; ++i)
        {
            for (unsigned int j = 0; j < num_vertices; ++j)
            {
                unsigned int connection_field;
                int path_field;
                get_fields(actual_result[i*num_vertices + j], connection_field, path_field);
                if ( (connection_field == INDIRECTLY_CONNECTED) && !does_path_exist(i, j, actual_result, num_vertices - 1) )
                {
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
void transitiveclosure_cpu_naive(unsigned int *graph)
{
    for (unsigned int k = 0; k < num_vertices; ++k)
    {
        for (unsigned int i = 0; i < num_vertices; ++i)
        {
            for (unsigned int j = 0; j < num_vertices; ++j)
            {
                if ( graph[i*num_vertices + j] == UNCONNECTED)
                {
                    if ( (graph[i*num_vertices + k] != UNCONNECTED) && (graph[k*num_vertices + j] != UNCONNECTED) )
                    {
                        graph[i*num_vertices + j] = k + INDIRECTLY_CONNECTED;
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
// Stage1 - determine connectivity between vertexs' within a TILE - primary
//----------------------------------------------------------------------------
void transitive_closure_stage1_kernel(tiled_index<TILE_SIZE, TILE_SIZE> tidx, array<unsigned int, 2> &graph, int passnum) restrict(amp)
{
    index<2> local_idx = tidx.local;
    
    // Load primary block into shared memory (primary_block_buffer)
    tile_static unsigned int primary_block_buffer[TILE_SIZE][TILE_SIZE];
    index<2> idx(passnum * TILE_SIZE + local_idx[0], passnum * TILE_SIZE + local_idx[1]);
    primary_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    tidx.barrier.wait();

    // Now perform the actual Floyd-Warshall algorithm on this block
    for (unsigned int k = 0; k < TILE_SIZE; ++k)
    {
        if ( primary_block_buffer[local_idx[0]][local_idx[1]] == UNCONNECTED)
        {
            if ( (primary_block_buffer[local_idx[0]][k] != UNCONNECTED) && (primary_block_buffer[k][local_idx[1]] != UNCONNECTED) )
            {
                primary_block_buffer[local_idx[0]][local_idx[1]] = passnum*TILE_SIZE + k + INDIRECTLY_CONNECTED;
            }
        }

        tidx.barrier.wait();
    }

    graph[idx] = primary_block_buffer[local_idx[0]][local_idx[1]];
}

//----------------------------------------------------------------------------
// Stage2 - determine connectivity between vertexs' between 2 TILE - primary 
// and current - current is along row or column of primary
//----------------------------------------------------------------------------
void transitive_closure_stage2_kernel(tiled_index<TILE_SIZE, TILE_SIZE> tidx, array<unsigned int, 2> &graph, int passnum) restrict(amp)
{
    index<2> tile_idx = tidx.tile;
    index<2> local_idx = tidx.local;
    
    // Load primary block into shared memory (primary_block_buffer)
    tile_static unsigned int primary_block_buffer[TILE_SIZE][TILE_SIZE];
    index<2> idx(passnum * TILE_SIZE + local_idx[0], passnum * TILE_SIZE + local_idx[1]);
    primary_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    // Load the current block into shared memory (curr_block_buffer)
    tile_static unsigned int curr_block_buffer[TILE_SIZE][TILE_SIZE];
    unsigned int group_id0, group_id1;
    if (tile_idx[0] == 0)
    {
        group_id0 = passnum;
        if (tile_idx[1] < passnum)
        {
            group_id1 = tile_idx[1];
        }
        else
        {
            group_id1 = tile_idx[1] + 1;
        }
    }
    else
    {
        group_id1 = passnum;
        if (tile_idx[1] < passnum)
        {
            group_id0 = tile_idx[1];
        }
        else
        {
            group_id0 = tile_idx[1] + 1;
        }
    }

    idx[0] = group_id0 * TILE_SIZE + local_idx[0];
    idx[1] = group_id1 * TILE_SIZE + local_idx[1];
    curr_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    tidx.barrier.wait();

    // Now perform the actual Floyd-Warshall algorithm on this block
    for (unsigned int k = 0; k < TILE_SIZE; ++k)
    {
        if ( curr_block_buffer[local_idx[0]][local_idx[1]] == UNCONNECTED)
        {
            if (tile_idx[0] == 0)
            {
                if ( (primary_block_buffer[local_idx[0]][k] != UNCONNECTED) && (curr_block_buffer[k][local_idx[1]] != UNCONNECTED) )
                {
                    curr_block_buffer[local_idx[0]][local_idx[1]] = passnum*TILE_SIZE + k + INDIRECTLY_CONNECTED;
                }
            }
            else
            {
                if ( (curr_block_buffer[local_idx[0]][k] != UNCONNECTED) && (primary_block_buffer[k][local_idx[1]] != UNCONNECTED) )
                {
                    curr_block_buffer[local_idx[0]][local_idx[1]] = passnum*TILE_SIZE + k + INDIRECTLY_CONNECTED;
                }
            }
        }

        tidx.barrier.wait();
    }

    graph[idx] = curr_block_buffer[local_idx[0]][local_idx[1]];
}

//----------------------------------------------------------------------------
// Stage3 - determine connectivity between vertexs' between 3 TILE 
// 1. primary block, 2. block made of row af current and column of primary 
// 3. block made of column of current and row of primary
//----------------------------------------------------------------------------
void transitive_closure_stage3_kernel(tiled_index<TILE_SIZE, TILE_SIZE> tidx, array<unsigned int, 2> &graph, int passnum) restrict(amp)
{
    index<2> tile_idx = tidx.tile;
    index<2> local_idx = tidx.local;
    
    unsigned int group_id0, group_id1;
    if (tile_idx[0] < passnum)
    {
        group_id0 = tile_idx[0];
    }
    else
    {
        group_id0 = tile_idx[0] + 1;
    }

    if (tile_idx[1] < passnum)
    {
        group_id1 = tile_idx[1];
    }
    else
    {
        group_id1 = tile_idx[1] + 1;
    }

    // Load block with same row as current block and same column as primary block into shared memory (shBuffer1)
    tile_static unsigned int shbuffer1[TILE_SIZE][TILE_SIZE];
    index<2> idx(group_id0 * TILE_SIZE + local_idx[0], passnum * TILE_SIZE + local_idx[1]);
    shbuffer1[local_idx[0]][local_idx[1]] = graph[idx];

    // Load block with same column as current block and same row as primary block into shared memory (shBuffer2)
    tile_static unsigned int shBuffer2[TILE_SIZE][TILE_SIZE];
    idx[0] = passnum * TILE_SIZE + local_idx[0];
    idx[1] = group_id1 * TILE_SIZE + local_idx[1];
    shBuffer2[local_idx[0]][local_idx[1]] = graph[idx];

    //  Load the current block into shared memory (shbuffer3)
    tile_static unsigned int curr_block_buffer[TILE_SIZE][TILE_SIZE];
    idx[0] = group_id0 * TILE_SIZE + local_idx[0];
    idx[1] = group_id1 * TILE_SIZE + local_idx[1];
    curr_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    tidx.barrier.wait();

    // Now perform the actual Floyd-Warshall algorithm on this block
    for (unsigned int k = 0; k < TILE_SIZE; ++k)
    {
        if ( curr_block_buffer[local_idx[0]][local_idx[1]] == UNCONNECTED)
        {
            if ( (shbuffer1[local_idx[0]][k] != UNCONNECTED) && (shBuffer2[k][local_idx[1]] != UNCONNECTED) )
            {
                curr_block_buffer[local_idx[0]][local_idx[1]] = passnum*TILE_SIZE + k + INDIRECTLY_CONNECTED;
            }
        }

        tidx.barrier.wait();
    }

    graph[idx] = curr_block_buffer[local_idx[0]][local_idx[1]];
}

//----------------------------------------------------------------------------
// C++ AMP (GPU) implementation entry point
//----------------------------------------------------------------------------
void transitiveclosure_amp_tiled(std::vector<unsigned int> &graph)
{
    extent<2> graph_domain(num_vertices, num_vertices);
    array<unsigned int, 2> graph_buf(graph_domain, graph.begin());

    assert((num_vertices % TILE_SIZE) == 0);
    extent<2> stage1_compute(extent<2>(TILE_SIZE, TILE_SIZE));
    assert(((num_vertices-TILE_SIZE) % TILE_SIZE) == 0);
    extent<2> stage2_compute(extent<2>(2*TILE_SIZE, num_vertices - TILE_SIZE));
    extent<2> stage3_compute(extent<2>(num_vertices - TILE_SIZE, num_vertices - TILE_SIZE));

    for (unsigned int k = 0; k < num_vertices/TILE_SIZE; ++k)
    {
        parallel_for_each(stage1_compute.tile<TILE_SIZE, TILE_SIZE>(), [k, &graph_buf] (tiled_index<TILE_SIZE, TILE_SIZE> ti) restrict(amp) {
            transitive_closure_stage1_kernel(ti, graph_buf, k); 
        });
        parallel_for_each(stage2_compute.tile<TILE_SIZE, TILE_SIZE>(), [k, &graph_buf] (tiled_index<TILE_SIZE, TILE_SIZE> ti) restrict(amp) {
            transitive_closure_stage2_kernel(ti, graph_buf, k);
        });
        parallel_for_each(stage3_compute.tile<TILE_SIZE, TILE_SIZE>(), [k, &graph_buf] (tiled_index<TILE_SIZE, TILE_SIZE> ti) restrict(amp) {
            transitive_closure_stage3_kernel(ti, graph_buf, k);
        });
    }

    graph = graph_buf;
}

int main()
{
	accelerator default_device;
	std::wcout << L"Using device : " << default_device.get_description() << std::endl;
	if (default_device == accelerator(accelerator::direct3d_ref))
		std::cout << "WARNING!! Running on very slow emulator! Only use this accelerator for debugging." << std::endl;

    srand(random_seed);
    printf ("Generating input data..");
    unsigned int *source_graph = generate_input_graph();
    printf ("Done\n");

    printf("Running the Transitive Closure Sample...\n");
    printf("Number of Vertices = %d\n", num_vertices);
    printf("Iterations = %d\n", num_iterations);
    printf("\n");

    // Now run the CPU and GPU algorithms and get performance results
    std::vector<unsigned int> cpu_transitiveclosure_graph(array_size);
    std::vector<unsigned int> gpu_transitiveclosure_graph(array_size);
    std::vector<unsigned int> gpu_datatransfer_graph(array_size); // Used only to measure time to copy data in and out of the GPU

    // Compute on CPU
    memcpy(cpu_transitiveclosure_graph.data(), source_graph, sizeof(unsigned int) * array_size);

    printf ("Computation on CPU...");
    transitiveclosure_cpu_naive(cpu_transitiveclosure_graph.data());
    printf ("Done\n");

    // Compute on GPU
    memcpy(gpu_transitiveclosure_graph.data(), source_graph, sizeof(unsigned int) * array_size);

    printf ("Computation on GPU...");
    transitiveclosure_amp_tiled(gpu_transitiveclosure_graph);
    printf ("Done\n");

    printf("\n\n");
    // Verify correctness of compute results
    bool passed = verify_transitive_closure(cpu_transitiveclosure_graph.data(), gpu_transitiveclosure_graph.data(), true);
    if (!passed)
    {
        printf("Compute Result - INCORRECT!\n\n");
    }

    if (to_display())
    {
        printf("------------------Source Graph--------------------------------------------\n");
        display_graph(source_graph);
        printf("--------------------------------------------------------------------------\n\n\n");

        printf("------------------CPU Transitive Closure Graph----------------------------\n");
        display_graph(cpu_transitiveclosure_graph.data());
        printf("--------------------------------------------------------------------------\n\n\n");

        printf("------------------GPU Transitive Closure Graph----------------------------\n");
        display_graph(gpu_transitiveclosure_graph.data());
        printf("--------------------------------------------------------------------------\n\n\n");
    }

    // cleanup
    printf("Releasing resources...\n\n");
    delete [] source_graph;
    
	printf ("Press ENTER to exit this program\n");
	getchar();
}
