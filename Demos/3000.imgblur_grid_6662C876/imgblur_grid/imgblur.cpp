// code example	for blog: Use extent instead of grid class - Sample 1
//                 created by: Tamer Afify              Date:1/1/2012

//This is a sample function for using the grid class to do an image blur
//The pixel blur can be performed by changing every pixel color RBG band to 
//the arithmetic average of this pixel value with all its 8 neighbors’ pixels.

//The grid offset feature can be of great benefit when the compute domain origin 
//is different from the data origin. In other words, (0,0) for the data is not 
//matching the (0,0) starting point for computation.

//In this sample we will use this feature to blur the inner image box without the 
//boarder pixels as they don’t have 8 neighbors pixel. So the compute domain origin 
//is (1, 1) in the data index. And also compute domain extent is smaller than data 
//extent by 2 rows and 2 columns.

// Note: to compile this code you need to use C++ AMP Developer Preview destributed
// During the TAP progrm.

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
  grid<2> result(origin, img.grid.extent - extent<2>(2,2));

  parallel_for_each(result, [=, &blurimage](index<2> idx) 
    restrict(direct3d)
  {
    float r = 0.0f;
    int samples = 0;

    for (int dy = -1; dy <= 1; dy++)
    {
      for (int dx = -1; dx <= 1; dx++)
      {
        index<2> i(idx + index<2>(dy,dx));

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

