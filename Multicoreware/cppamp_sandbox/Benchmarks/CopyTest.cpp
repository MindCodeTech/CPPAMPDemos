// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll 
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl 
// RUN: pushd %t 
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
/*----------------------------------------------------------------------------
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
* C++ AMP measure copy perf test.
*
*---------------------------------------------------------------------------*/
#include <amp.h>
using namespace Concurrency;
class Copy_AMP
{
public:
    void SetupOnce();
    void Setup();
    void CleanupOnce();
    void Cleanup();
    void Test();
private:
unsigned int m_data_size;
std::vector<float> m_data;
};

void Copy_AMP::SetupOnce()
{
    m_data_size = 2048;
    m_data.resize(m_data_size);
}

void Copy_AMP::Setup()
{
    std::fill(m_data.begin(), m_data.end(), 12.0f);
}

void Copy_AMP::CleanupOnce()
{
}

void Copy_AMP::Cleanup()
{
}

void Copy_AMP::Test()
{
   std::vector<float> data_src(m_data_size);
    //StartTimer(copy_in);
    std::cout << "Starting timer...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    array_view<float, 1> srcView(m_data_size, m_data);
    array_view<float, 1> destView(m_data_size, data_src);
    accelerator ampAccelerator;
    accelerator_view av = ampAccelerator.get_default_view();
    parallel_for_each(av, extent<1>(1),  [=] (index<1> i) restrict(amp)
    {
        destView[i] = srcView[i];
    });

    destView.synchronize();

    //StopTimer(copy_in);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "time CopyTest " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1.0e6 << "\n";
}

int main (int argc, char**argv) {

  Copy_AMP *copy_amp = new Copy_AMP();
  copy_amp->SetupOnce();
  copy_amp->Setup();
  copy_amp->Test();
  copy_amp->Cleanup();
  copy_amp->CleanupOnce();
  delete copy_amp;
  return 0;
}
