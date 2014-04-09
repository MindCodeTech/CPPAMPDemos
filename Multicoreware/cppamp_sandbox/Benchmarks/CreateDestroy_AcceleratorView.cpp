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
* C++ AMP Create/Destroy AV perf test.
*
*---------------------------------------------------------------------------*/
#include <amp.h>
using namespace Concurrency;
class CreateDestroy_AcceleratorView_AMP {
public:
    void SetupOnce();
    void Test();
private:
enum Counters
{
    CntCreate = 0,
    CntDestroy,
    CntNum
};

};

void CreateDestroy_AcceleratorView_AMP::Test()
{
    __int64_t timerSum[CntNum] = {0};
    __int64_t timerStart;
    __int64_t timerStop;

    std::aligned_storage<sizeof(accelerator_view), std::alignment_of<accelerator_view>::value>::type storage;
    accelerator_view* addr = reinterpret_cast<accelerator_view*>(&storage);
    accelerator ampAccelerator;
    for(int i = 0; i < 100; i++)
    {
        ampAccelerator.get_default_view().wait();

        {
            // Measure CPU time it takes to create the accelerator_view.
            //SampleTimer(timerStart);
            auto t1 = std::chrono::high_resolution_clock::now();
            auto accView = ampAccelerator.create_view();
            auto t2 = std::chrono::high_resolution_clock::now();
            timerSum[CntCreate] += std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();

            new(addr) accelerator_view(accView);

            // Note accView is destroyed at this point, the *addr object is the last outstanding copy.
        }

        ampAccelerator.get_default_view().wait();

        // Measure CPU time it takes to destroy the accelerator_view.
        auto t3 = std::chrono::high_resolution_clock::now();
        addr->~accelerator_view();
        auto t4 = std::chrono::high_resolution_clock::now();
        timerSum[CntDestroy] += std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t3).count();
    }

    //SetElapsedTime(timerSum[CntCreate], CntCreate);
    std::cout << "time createAcceleratorView100 " << timerSum[CntCreate]/1.0e9 << "\n";
    //SetElapsedTime(timerSum[CntDestroy], CntDestroy);
    std::cout << "time destroyAcceleratorView100 " << timerSum[CntDestroy]/1.0e9 << "\n";
}

int main()
{
     CreateDestroy_AcceleratorView_AMP *caa = new CreateDestroy_AcceleratorView_AMP();
     caa->Test();
     delete caa;
    return 0;
}
