//RUN: false
//XFAIL: *
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
* C++ AMP Create/Destroy container perf test.
*
*---------------------------------------------------------------------------*/
#include <amp.h>
using namespace Concurrency;
// Base class for the following test classes.
// Parameters:
//   - CreateDestroy_Container_AMP.sizeX - number of elements in the created container, dimension 0
//   - CreateDestroy_Container_AMP.sizeY - number of elements in the created container, dimension 1
struct CreateDestroy_Container_AMP
{
    CreateDestroy_Container_AMP();
    void SetupOnce() ;//override;
    void Test();

protected:
    template <typename Container>
    void TestImpl();

    extent<2> m_size;
};

// Measure time to create and destroy array.
struct CreateDestroy_Array_AMP : public CreateDestroy_Container_AMP
{
    CreateDestroy_Array_AMP();
    void Test();//override;
};

// Measure time to create and destroy texture.
struct CreateDestroy_Texture_AMP : public CreateDestroy_Container_AMP
{
    CreateDestroy_Texture_AMP();
    void Test();//override;
};

// Measure time to create and destroy array_view.
struct CreateDestroy_ArrayView_AMP //: public AMPTestBase
{
    CreateDestroy_ArrayView_AMP();
    void SetupOnce();// override;
    void Test();// override;
};

enum Counters
{
    CntCreate = 0,
    CntDestroy,
    CntCopyConstruct,
    CntCopyAssign,
    CntNum
};

CreateDestroy_Container_AMP::CreateDestroy_Container_AMP(){}

void CreateDestroy_Container_AMP::SetupOnce()
{

    // Read parameters
    //CreateDestroy_Container_AMP.sizeX
    m_size[0] = 2048;

    //CreateDestroy_Container_AMP.sizeY"
    m_size[1] = 1024;
}

template <typename Container>
void CreateDestroy_Container_AMP::TestImpl()
{
    std::cout << "Array test\n";

    __int64_t timerSum[CntNum] = {0};
    __int64_t timerStart;
    __int64_t timerStop;

    typename std::aligned_storage<sizeof(Container), std::alignment_of<Container>::value>::type storage;
    Container* addr = reinterpret_cast<Container*>(&storage);
    accelerator ampAccelerator;
    for(int i = 0; i < 10000; i++)
    {
        // Measure CPU time it takes to construct the container.
        ampAccelerator.get_default_view().wait();
        //SampleTimer(timerStart);
        auto t1 = std::chrono::high_resolution_clock::now();
        new(addr) Container(m_size, ampAccelerator.get_default_view());
        //SampleTimer(timerStop);
        auto t2 = std::chrono::high_resolution_clock::now();
        timerSum[CntCreate] += std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();

        // Measure CPU time it takes to destroy the container.
        ampAccelerator.get_default_view().wait();
        //SampleTimer(timerStart);
        auto t3 = std::chrono::high_resolution_clock::now();
        addr->~Container();
        //SampleTimer(timerStop);
        auto t4 = std::chrono::high_resolution_clock::now();
        timerSum[CntDestroy] += std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t3).count();
    }

    //SetElapsedTime(timerSum[CntCreate], CntCreate);
    //SetElapsedTime(timerSum[CntDestroy], CntDestroy);
    std::cout << "time createArray10000: " << timerSum[CntCreate]/1.0e9 << "\n";
    std::cout << "time destoryArray10000: " << timerSum[CntDestroy]/1.0e9 << "\n";
}

void CreateDestroy_Container_AMP::Test()
{
    TestImpl< array<int, 2> >();
}

CreateDestroy_ArrayView_AMP::CreateDestroy_ArrayView_AMP() {}


void CreateDestroy_ArrayView_AMP::Test()
{
    __int64_t timerSum[CntNum] = {0};
    //__int64_t timerStart;
    //__int64_t timerStop;

    typedef array_view<int, 2> Container;
    extent<2> size(256, 256);
    std::unique_ptr<int[]> cpuMemory(new int[size.size()]);

    std::aligned_storage<sizeof(Container), std::alignment_of<Container>::value>::type storage[2];
    Container* addr = reinterpret_cast<Container*>(&storage[0]);
    Container* addr_copy = reinterpret_cast<Container*>(&storage[1]);

    std::cout << "ArrayView test\n";

    for(int i = 0; i < 10000; i++)
    {
        // Measure CPU time it takes to construct the container.
        //SampleTimer(timerStart);
        auto t1 = std::chrono::high_resolution_clock::now();
        new(addr) Container(size, cpuMemory.get());
        //SampleTimer(timerStop);
        auto t2 = std::chrono::high_resolution_clock::now();
        timerSum[CntCreate] += std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();

        // Measure CPU time it take to copy construct the container.
        //SampleTimer(timerStart);
        auto t3 = std::chrono::high_resolution_clock::now();
        new(addr_copy) Container(*addr);
        //SampleTimer(timerStop);
        auto t4 = std::chrono::high_resolution_clock::now();
        timerSum[CntCopyConstruct] += std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t3).count();

        // Measure CPU time it take to copy assign the container.
        //SampleTimer(timerStart);
        auto t5 = std::chrono::high_resolution_clock::now();
        *addr_copy = *addr;
        //SampleTimer(timerStop);
        auto t6 = std::chrono::high_resolution_clock::now();
        timerSum[CntCopyAssign] += std::chrono::duration_cast<std::chrono::nanoseconds>(t6-t5).count();

        // Measure CPU time it takes to destroy the container.
        //SampleTimer(timerStart);
        auto t7 = std::chrono::high_resolution_clock::now();
        addr->~Container();
        //SampleTimer(timerStop);
        auto t8 = std::chrono::high_resolution_clock::now();
        timerSum[CntDestroy] += std::chrono::duration_cast<std::chrono::nanoseconds>(t8-t7).count();

        // Clean up
        addr_copy->~Container();
    }

    //SetElapsedTime(timerSum[CntCreate], CntCreate);
    //SetElapsedTime(timerSum[CntDestroy], CntDestroy);
    //SetElapsedTime(timerSum[CntCopyConstruct], CntCopyConstruct);
    //SetElapsedTime(timerSum[CntCopyAssign], CntCopyAssign);
    std::cout << "time createArrayView10000 " << timerSum[CntCreate]/1.0e9 << "\n";
    std::cout << "time destroyArrayView10000 " << timerSum[CntDestroy]/1.0e9 << "\n";
    std::cout << "time copyConstructArrayView10000 " << timerSum[CntCopyConstruct]/1.0e9 << "\n";
    std::cout << "time copyAssignArrayView10000 " << timerSum[CntCopyAssign]/1.0e9 << "\n";
}

int main()
{
    CreateDestroy_Container_AMP *cca = new CreateDestroy_Container_AMP();
    cca->SetupOnce();
    cca->Test();
    delete cca;
    CreateDestroy_ArrayView_AMP *caa = new CreateDestroy_ArrayView_AMP();
    caa->Test();
    delete caa;
    return 0;
}
