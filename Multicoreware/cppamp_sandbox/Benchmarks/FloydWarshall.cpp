// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll 
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl 
// RUN: pushd %t 
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
/*----------------------------------------------------------------------------
* Copyright (c) Microsoft Corp.
*
* Licensed under the Apache License, Version 2.0 (the "License"); you may not 
* use this file except in compliance with the License.  You may obtain a copy 
* of the License at http://www.apache.org/licenses/LICENSE-2.0  
* 
* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED 
* WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, 
* MERCHANTABLITY OR NON-INFRINGEMENT. 
*
* See the Apache Version 2.0 License for specific language governing 
* permissions and limitations under the License.
*---------------------------------------------------------------------------
* 
* C++ AMP FloydWarshall perf test.
*
*---------------------------------------------------------------------------*/

#include <vector>
#include <random>
#include <assert.h>
#include <amp.h>

using namespace Concurrency;

#define FLD_NUM_NODES               (1024)
#define FLD_MAX_DISTANCE            (200)
#define FLD_TILE_WIDTH              (16)
#define FLD_TILE_THREADS            (FLD_TILE_WIDTH * FLD_TILE_WIDTH)

#if (FLD_TILE_THREADS > 1024)
#error "Number of threads per tile exceeds maximum allowed (1024)."
#endif //(FLD_TILE_THREADS > 1024)

#if (FLD_NUM_NODES % FLD_TILE_WIDTH != 0)
#error "Number of nodes must be divisable by tile width."
#endif //(FLD_NUM_NODES % FLD_TILE_WIDTH != 0)



class FloydWarshall
{
protected:
    std::mt19937 mersenne_twister_engine;
                    
    template<typename T>
    void FillFloatingPoint(T *arr, size_t size, T min, T max)
    {
        std::uniform_real_distribution<T> uni(min, max);
            
        for(size_t i = 0; i < size; ++i)
        {
            arr[i] = uni(mersenne_twister_engine);
        }
    }

    template<typename T>
    void FillIntegral(T *arr, size_t size, T min, T max)
    {
        std::uniform_int_distribution<T> uni(min, max);
            
        for(size_t i = 0; i < size; ++i)
        {
            arr[i] = uni(mersenne_twister_engine);
        }
    }

    void SetupFloydWarshall()
    {
        numNodes = FLD_NUM_NODES;
        maxDistance = FLD_MAX_DISTANCE;

        height = width = numNodes;

         // allocate and init memory used by host 
        unsigned int matrixSize = width * height;

        pathDistanceMatrix = new int[matrixSize];
        assert(pathDistanceMatrix != nullptr);

        pathMatrix = new int[matrixSize];
        assert(pathMatrix != nullptr);

        // random initialisation of input 

        //
        // pathMatrix is the intermediate node from which the path passes
        // pathMatrix(i,j) = k means the shortest path from i to j
        // passes through an intermediate node k
        // Initialized such that pathMatrix(i,j) = i
        //
        FillIntegral(pathDistanceMatrix, matrixSize, 0, maxDistance);

        for(unsigned int i = 0; i < height; ++i)
        {
            unsigned int iXWidth = i * width;
            pathDistanceMatrix[iXWidth + i] = 0;
        }

        //
        // pathMatrix is the intermediate node from which the path passes
        // pathMatrix(i,j) = k means the shortest path from i to j
        // passes through an intermediate node k
        // Initialized such that pathMatrix(i,j) = i
        //
        for(unsigned int i = 0; i < height; ++i)
        {
            for(unsigned int j = 0; j < i; ++j)
            {
                pathMatrix[i*width + j] = i;
                pathMatrix[j*width + i] = j;
            }
            pathMatrix[i*width + i] = i;
        }

        verificationPathDistanceMatrix = new int[matrixSize];
        assert(verificationPathDistanceMatrix != nullptr);
    
        verificationPathMatrix = new int[matrixSize];
        assert(verificationPathMatrix != nullptr);

        memcpy(verificationPathDistanceMatrix, pathDistanceMatrix, matrixSize*sizeof(int));
        memcpy(verificationPathMatrix, pathMatrix, matrixSize*sizeof(int));
    }

    virtual void CleanupFloydWarshall()
    {
        /* release program resources (input memory etc.) */
        delete[] pathDistanceMatrix;
        delete[] pathMatrix;

        delete[] verificationPathDistanceMatrix;
        delete[] verificationPathMatrix;
    }

    void FloydWarshallCPUReference(int * pathDistanceMatrix, int * pathMatrix, const unsigned int numNodes) 
    {
        int distanceYtoX, distanceYtoK, distanceKtoX, indirectDistance;
    
        // pathDistanceMatrix is the adjacency matrix(square) with
        // the dimension equal to the number of nodes in the graph.
        int width = numNodes;
        int yXwidth;
    
        // for each intermediate node k in the graph find the shortest distance between
        // the nodes i and j and update as
        //
        // ShortestPath(i,j,k) = min(ShortestPath(i,j,k-1), ShortestPath(i,k,k-1) + ShortestPath(k,j,k-1))
        //
        for(unsigned int k = 0; k < numNodes; ++k)
        {
            for(unsigned int y = 0; y < numNodes; ++y)
            {
                yXwidth =  y*numNodes;
                for(unsigned int x = 0; x < numNodes; ++x)
                {
                    distanceYtoX = pathDistanceMatrix[yXwidth + x];
                    distanceYtoK = pathDistanceMatrix[yXwidth + k];
                    distanceKtoX = pathDistanceMatrix[k*width + x];

                    indirectDistance = distanceYtoK + distanceKtoX;

                    if(indirectDistance < distanceYtoX)
                    {
                        pathDistanceMatrix[yXwidth + x] = indirectDistance;
                        pathMatrix[yXwidth + x]         = k;
                    }
                }
            }
        }
    }

    virtual void VerifyResult()
    {
        // reference implementation
        // it overwrites the input array with the output
        FloydWarshallCPUReference(verificationPathDistanceMatrix, verificationPathMatrix, width);

        // compare the results and see if they match //
        if(memcmp(pathDistanceMatrix, verificationPathDistanceMatrix, height*width*sizeof(int)) != 0)
        {
            std::cout <<"fail FloydWarshall";
        }
    }

    unsigned int           numNodes;  /**< Number of nodes in the graph */
    int         *pathDistanceMatrix;  /**< path distance array */
    int                 *pathMatrix;  /**< path arry */
    int  *verificationPathDistanceMatrix;/**< path distance array for reference implementation */
    int     *verificationPathMatrix; /**< path array for reference implementation */
    unsigned int              width; /**< width of the adjacency matrix */
    unsigned int             height; /**< height of the adjacency matrix */
    int                 maxDistance; /**< maximum distance between two nodes in the graph */
};

class FloydWarshall_AMP : protected FloydWarshall {
public:
    void Setup();
    void Test();
    void Verify();
    void Cleanup();
};


void FloydWarshall_AMP::Setup()
{
    SetupFloydWarshall();
}


void FloydWarshall_AMP::Test()
{
    extent<2> ext(height, width);

    // Copy data to the device
    //
    array_view<int, 2> pathDistanceBuffer(ext, pathDistanceMatrix);
    array_view<int, 2> pathBuffer(ext, pathMatrix);

    // Only timing the kernel 
    std::cout << "Starting timer...\n";
    auto t1 = std::chrono::high_resolution_clock::now();

    unsigned int numBlocks = width/FLD_TILE_WIDTH;
    for (unsigned int block_k = 0; block_k < width; block_k+=FLD_TILE_WIDTH)
    {
        //Sub-pass 1
        parallel_for_each(extent<2>(FLD_TILE_WIDTH,FLD_TILE_WIDTH).tile<FLD_TILE_WIDTH,FLD_TILE_WIDTH>(), 
                          [pathDistanceBuffer, pathBuffer, block_k](tiled_index<FLD_TILE_WIDTH, FLD_TILE_WIDTH> tidx) restrict(amp) {
            for (int k=block_k; k<block_k+FLD_TILE_WIDTH; ++k) {
                index<2> ij(block_k+tidx.local[0], block_k+tidx.local[1]);
                index<2> ik(block_k+tidx.local[0], k);
                index<2> kj(k, block_k+tidx.local[1]);
                int indirectDistance = pathDistanceBuffer[ik] + pathDistanceBuffer[kj];
                if (indirectDistance < pathDistanceBuffer[ij])
                {
                    pathDistanceBuffer[ij] = indirectDistance;
                    pathBuffer[ij] = k;
                }
                tidx.barrier.wait();
            }
                
        });

        //Sub-pass 2
        parallel_for_each(extent<2>(FLD_TILE_WIDTH*(numBlocks-1),FLD_TILE_WIDTH*2).tile<FLD_TILE_WIDTH,FLD_TILE_WIDTH>(),
                          [pathDistanceBuffer, pathBuffer, block_k](tiled_index<FLD_TILE_WIDTH, FLD_TILE_WIDTH> tidx) restrict(amp) {
            int extra = 0;
            if (tidx.global[0] >= block_k) {
                extra = FLD_TILE_WIDTH;
            }
            for (int k=block_k; k<block_k+FLD_TILE_WIDTH; ++k) {
                int i,j;
                if (tidx.global[1] < FLD_TILE_WIDTH) {
                    i = tidx.global[0] + extra;
                    j = block_k + tidx.local[1];
                } else {
                    i = block_k + tidx.local[1];
                    j = tidx.global[0] + extra;
                }
                index<2> ij(i, j);
                index<2> ik(i, k);
                index<2> kj(k, j);
                int indirectDistance = pathDistanceBuffer[ik] + pathDistanceBuffer[kj];
                if (indirectDistance < pathDistanceBuffer[ij])
                {
                    pathDistanceBuffer[ij] = indirectDistance;
                    pathBuffer[ij] = k;
                }
                tidx.barrier.wait();
            }
        });

        //Sub-pass 3
        parallel_for_each(extent<2>(FLD_TILE_WIDTH*(numBlocks-1),FLD_TILE_WIDTH*(numBlocks-1)).tile<FLD_TILE_WIDTH,FLD_TILE_WIDTH>(),
                          [pathDistanceBuffer, pathBuffer, block_k](tiled_index<FLD_TILE_WIDTH, FLD_TILE_WIDTH> tidx) restrict(amp) {
            int extra_i = 0;
            if (tidx.global[0] >= block_k) {
                extra_i = FLD_TILE_WIDTH;
            }
            int extra_j = 0;
            if (tidx.global[1] >= block_k) {
                extra_j = FLD_TILE_WIDTH;
            }
            for (int k=block_k; k<block_k+FLD_TILE_WIDTH; ++k) {
                int i = tidx.global[0] + extra_i;
                int j = tidx.global[1] + extra_j;
                index<2> ij(i, j);
                index<2> ik(i, k);
                index<2> kj(k, j);
                int indirectDistance = pathDistanceBuffer[ik] + pathDistanceBuffer[kj];
                if (indirectDistance < pathDistanceBuffer[ij])
                {
                    pathDistanceBuffer[ij] = indirectDistance;
                    pathBuffer[ij] = k;
                }
                tidx.barrier.wait();
            }
        });
                    
    }
    pathDistanceBuffer.synchronize();

    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "time FloydWarshall " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1e6f << "\n";

   //Copy data out and verify results    
}

void FloydWarshall_AMP::Verify()
{
    VerifyResult();
}

void FloydWarshall_AMP::Cleanup()
{
    CleanupFloydWarshall();
}

int main (int argc, char**argv) {
    FloydWarshall_AMP* fw = new FloydWarshall_AMP();
    fw->Setup();
    fw->Test();
    fw->Verify();
    fw->Cleanup();
    delete fw;
}
