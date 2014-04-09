// code example	for blog: Use extent instead of grid class - Sample 2
//                 created by: Tamer Afify              Date:1/1/2012

//This sample shows how to replace grid with extent in the 
//previously illustrated image blur solution.
//For code porting process follow those three simple steps;
//1. Wherever grid type or array/aray_view value is used replace with extent
//2. If array is constructed with a grid origin index value, then whenever 
//   this array is used add the origin index to its index value.
//3. If the compute domain grid - for parallel_for_each – is constructed with origin, 
//   add this origin to every index use in the kernel.

// Note: to compile this code you need to use Visual Studio 2011 Beta Release

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
  extent<2> result(img.extent - extent<2>(2,2));

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

void main()
{
  // generate dummy image data
  vector<float> data (width * height);
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

  data = boxblur(img);

  // Print data first quarter out
  printf("\n\n After blur .. \n");
  for (int y= 0; y < height; y++)
  {
    for (int x= 0; x < width; x++)
    {
      printf( "%0.1f ", data[x + (y*width)]);
    }
    printf("\n");
  }

}