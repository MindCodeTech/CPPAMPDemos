// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.
/// <tags>P2</tags>
/// <summary>Condition of barrier is based on global thread id. 3d</summary>

#include <iostream>
#include <amptest.h>
#include <amptest_main.h>
using namespace std;
using namespace Concurrency;
using namespace Concurrency::Test;

const int XGroupSize = 8;
const int YGroupSize = 8;
const int ZGroupSize = 16;

const int NumXGroups = 32;           
const int NumYGroups = 32;           
const int NumZGroups = 63;           
const int NumGroups  =  NumXGroups * NumYGroups * NumZGroups;

const int XSize      = XGroupSize * NumXGroups;
const int YSize      = YGroupSize * NumYGroups;
const int ZSize     = ZGroupSize * NumZGroups;
const int Size = XSize * YSize * ZSize;

template<typename ElementType>
void CalculateGroupSum(tiled_index<ZGroupSize, YGroupSize, XGroupSize> idx, const array<ElementType, 3> & fA, array<ElementType, 3> & fB) __GPU
{
    // error: cause 3561
    if ((idx.global[0] %2 == 0) && (idx.global[1] %2 == 0) && (idx.global[2] %2 == 0))
    {
        tile_static ElementType shared[ZGroupSize][YGroupSize][XGroupSize];
        shared[idx.local[0]][idx.local[1]][idx.local[2]] = fA[idx.global];
        idx.barrier.wait();
        fB[idx.global] = shared[idx.local[0] % 2][idx.local[1] % 2][idx.local[2] % 2];
    }
}

template <typename ElementType>
void kernel(tiled_index<ZGroupSize, YGroupSize, XGroupSize> idx, const array<ElementType, 3> & fA, array<ElementType, 3> & fB, int x) __GPU
{
    do { if(x <= 1)  break; do { if(x <= 2)  break; do { if(x <= 3)  break; do { if(x <= 4)  break; do { if(x <= 5)  break; 
    for(;x > 6;)   { for(;x > 7;)  { for(;x > 8;)  { for(;x > 9;)  { for(;x > 10;) {
        if(x > 11) if(x > 12) if(x > 13) if(x > 14) if(x > 15)
        {
            switch(x > 16? 1:0) { case 0: break; case 1: 
                switch(x > 17? 1:0) { case 0: break; case 1: switch(x > 18? 1:0) { case 0: break; case 1: 
                switch(x > 19? 1:0) { case 0: break; case 1: switch(x > 20? 1:0) { case 0: break; case 1: 
            {
                while(x > 21) { while(x > 22) { while(x > 23) { while(x > 24) { while(x > 25){ 

                    CalculateGroupSum(idx, fA, fB);

                    break;} break;} break;} break;} break;}   

            }
            }}}}}
        }
        break;} break;} break;} break;} break;}
    break;} while(true); break;} while(true); break;} while(true); break;} while(true); break;} while(true);    
}

template <typename ElementType>
runall_result test()
{
    srand(2012);
    bool passed = true;

    //ElementType A[Size]; // data
    ElementType *A = new ElementType[Size];
    ElementType *B = new ElementType[NumGroups];   // holds the grouped sum of data

    ElementType *refB1 = new ElementType[NumGroups]; // Expected value if conditions are satisfied; sum of elements in each group
    ElementType *refB2 = new ElementType[NumGroups]; // Expected value if the conditions are not satisfied. Some fixed values

    for(int g = 0; g < NumGroups; g++)
    {
        refB2[g] = 100; // Init to fixed value
    }

    accelerator_view rv =  require_device(Device::ALL_DEVICES).default_view;

    Concurrency::extent<3> extentA(ZSize, YSize, XSize), extentB(NumZGroups, NumYGroups, NumXGroups);
    array<ElementType, 3> fA(extentA, rv), fB(extentB, rv);

    //forall where conditions are met
    copy(A, fA);

    int x = 26;
    parallel_for_each(fA.extent.template tile<ZGroupSize, YGroupSize, XGroupSize>(), [&, x] (tiled_index<ZGroupSize, YGroupSize, XGroupSize> idx) __GPU_ONLY {
        kernel<ElementType>(idx, fA, fB, x);
    });

    copy(fB, B);

    if(!Verify<ElementType>(B, refB1, NumGroups))
    {
        passed = false;
        cout << "Test1: failed" << endl;
    }    
    else
    {
        cout << "Test1: passed" << endl;
    }

    delete []A;
    delete []B;
    delete []refB1;
    delete []refB2;
    return passed;
}

runall_result test_main()
{
    runall_result result;

    cout << "Test shared memory with \'int\'" << endl;
    result = test<int>();


    return runall_fail;
}

//#Expects: Error: \(38\) : .+ C3561
//#Expects: Error: \(38\) : .+ C3561
