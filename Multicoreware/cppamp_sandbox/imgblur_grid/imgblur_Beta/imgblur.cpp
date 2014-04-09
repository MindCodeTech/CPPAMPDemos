// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
#include <amp.h>
#include <vector>
#include <random>

using namespace concurrency;
using std::vector;

const unsigned int width=17;
const unsigned int height=17;

std::mt19937 mersenne_twister_engine;

array<float,2> boxblur(array_view<float,2> img)
{
  index<2> origin(1,1);
  array<float, 2> blurimage(img);
  extent<2> result(img.get_extent() - extent<2>(2,2));

  parallel_for_each(result, [=, &blurimage](index<2> idx) 
    restrict(amp)
  {
    float r = 0.0f;
    int samples = 0;
    idx += origin;

    for (int dy = -1; dy <= 1; dy++)
    {
      for (int dx = -1; dx <= 1; dx++)
      {
        r += img[idx + index<2>(dy,dx)];
        samples++;

      }
    }

    blurimage[idx] = r/samples;
  });

  return blurimage;
}

int main()
{
  // generate dummy image data
  vector<float> data (width * height);
  vector<float> data_temp (width * height);
  std::uniform_real_distribution<float> uni(0.0f, 10.0f);

  for ( int x= 0; x < (width * height); x++)
  {
    data[x] = uni(mersenne_twister_engine);
  }

  printf("before blur .. \n");
  for (int y= 0; y < height; y++)
  {
    for (int x= 0; x < width; x++)
    {
      printf( "%0.1f ", data[x + (y*width)]);
    }
    printf("\n");
  }

  // copy data to accelerator
  array_view<float,2> img(height, width, data);

  data_temp = boxblur(img);

  // Print data first quarter out
  printf("\n\n After blur .. \n");
  for (int y= 0; y < height; y++)
  {
    for (int x= 0; x < width; x++)
    {
      printf( "%0.1f ", data_temp[x + (y*width)]);
    }
    printf("\n");
  }
  return 0;
}
