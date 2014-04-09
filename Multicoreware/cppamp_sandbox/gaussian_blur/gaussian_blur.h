//----------------------------------------------------------------------------
// File: gaussian_blur.h
// 
// Refer README.txt
//----------------------------------------------------------------------------

#pragma once

#include <amp.h>
#include <vector>
#include <amp_math.h>

using namespace Concurrency;

#define DEFAULT_NUM_OF_ELEMENTS  1024

#define TILE_SIZE  16

#define BLUR_MTX_DIM  5
#define BLUR_MTX_SIZE  (BLUR_MTX_DIM*BLUR_MTX_DIM)
#define BLUR_MTX_VALUES  2, 4, 5, 4, 2, 4, 9, 12, 9, 4, 5, 12, 15, 12, 5, 4, 9, 12, 9, 4, 2, 4, 5, 4, 2

class gaussian_blur {
public:
  gaussian_blur(int _size = DEFAULT_NUM_OF_ELEMENTS);
  ~gaussian_blur();
  void execute();
  bool verify();

  static void gaussian_blur_simple_amp_kernel(int size, const array<float,2> &input, array<float,2> &output);
private:
  int size;
  std::vector<float> data;
  std::vector<float> amp_result;
};
