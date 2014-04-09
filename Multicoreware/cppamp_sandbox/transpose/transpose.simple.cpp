// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
//----------------------------------------------------------------------------
// File: transpose.cpp
// 
// Implement C++ AMP version of matrix transpose
//----------------------------------------------------------------------------

#include <amp.h>
#include <cmath>
#include <assert.h>
#include <iostream>
#include <sstream>

using namespace concurrency;

//-----------------------------------------------------------------------------
// Using simple model transpose
//-----------------------------------------------------------------------------
template <typename _value_type>
void transpose_simple(const array_view<_value_type, 2>& data, 
                      const array_view<_value_type, 2>& data_transpose) {
  parallel_for_each(data.get_extent(), [=] (index<2> idx) restrict(amp) {
    index<2> idx2(idx[1],idx[0]); 
    data_transpose[idx2] = data[idx];
  });
}

//-----------------------------------------------------------------------------
// Test driver
//-----------------------------------------------------------------------------
typedef void traspose_func(const array_view<float, 2>& data,
                           const array_view<float, 2>& data_transpose);

void test_transpose_func(int m, int n,
         traspose_func *user_func, std::string func_name) {
  std::cout << "Testing implementation " << func_name << std::endl;
  std::vector<float> v_data(m * n);
  std::vector<float> v_data_transpose(n * m);

  bool passed = true;

  for (int ir = 0; ir < m; ir++) {
    for (int ic = 0; ic < n; ic++) {
      v_data[ir * n + ic] = ir * 37.0f + ic * 7.0f;
      v_data_transpose[ir * n + ic] = -1.0f;
    }
  }

  array_view<float, 2> data_av(m, n, v_data);
  array_view<float, 2> data_transpose_av(n, m, v_data_transpose);

  user_func(data_av, data_transpose_av);

  data_transpose_av.synchronize();

  for (int ir = 0; ir < m; ir++) {
    for (int ic = 0; ic < n; ic++) {
      if (v_data[ir * n + ic] != v_data_transpose[ic * m + ir]) {
        std::cout << "Mismatch at (" << ir << "," << ic 
                  << ") data=" << v_data[ir * n + ic] << " transpose="
                  << v_data_transpose[ic * m + ir] << std::endl;
        passed = false;
      }
    }
  }
  
  std::cout << "Test "
            << static_cast<const char *>(passed ? "passed" : "failed")
            << std::endl;
}

int main() {
  std::cout << "Running test transpose_simple" << std::endl; 
  test_transpose_func(999, 666, transpose_simple<float>, "transpose_simple");
  return 0; 
}

