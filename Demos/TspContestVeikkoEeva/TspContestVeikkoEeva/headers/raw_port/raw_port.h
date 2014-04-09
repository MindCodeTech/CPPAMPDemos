/***********************************************************************************************************
 * Copyright (c) 2012 Veikko Eeva <veikko@... see LinkedIn etc.>										   *
 *																										   *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software		   *
 * and associated documentation files (the "Software"), to deal in the Software without restriction,	   *
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,			   *
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is		   *
 * furnished to do so, subject to the following conditions:												   *
 * 																										   *
 * The above copyright notice and this permission notice shall be included in all copies or				   *
 * substantial portions of the Software.																   *
 *																										   *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT	   *
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, *
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE	   *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.												   *
 *																										   *
 ***********************************************************************************************************/

/***********************************************************************************************************
 * This code was written to participate to a competition organised by Beyond3D forums and supported by     *
 * AMD and Microsoft. See more at http://www.beyond3d.com/content/articles/121/. My sincere thanks         *
 * for the inducement to learn many new things.                                                            *
 *                                                                                                         * 
 ***********************************************************************************************************/

/***********************************************************************************************************
 * See \headers\opt2kernel.h for more information.							                               *
 **********************************************************************************************************/

/*
typedef struct cc
{
	float x_;
	float y_;

	cc() restrict(cpu, amp): x_(0), y_(0) {};
	cc(float x, float y): x_(x), y_(y) {};

} city_coords;

int calculateDistance2DSimple(unsigned int i, unsigned int j, city_coords* coords)
{	float dx, dy;
 	
 	dx = coords[i].x_ - coords[j].x_;
	dy = coords[i].y_ - coords[j].y_;

	return (int)(sqrtf(dx * dx + dy * dy) + 0.5f);
}

auto elements = instance->get_elements();
		vector<city_coords> coordinates;
		coordinates.reserve(elements.size() / 2);
		for(unsigned int i = 0; i < elements.size(); i += 2)
		{			
			coordinates.emplace_back(city_coords(elements[i], elements[i + 1]));
		}
		
		vector<unsigned int> route(coordinates.size());
		iota(route.begin(), route.end(), 0);

		auto out = utilities::opt2_simple_kernel<TILE_SIZE, GPU_BLOCKS, STATIC_CACHE_SIZE, CITY_CACHE_SIZE>(coordinates_size, number_of_2opts, iterations, coordinates_view, route_view, chosen_accelerator);

		unsigned long number_of_2opts = static_cast<long int>(coordinates.size() - 3) * static_cast<long int>(coordinates.size() - 2) / 2;
	
	static const unsigned int TILE_SIZE = 1024;
	static const unsigned int THREADS = 16384;
	
	//This is the best values shared cache size. It should be the size of threads in one tile.
	static const unsigned int STATIC_CACHE_SIZE = TILE_SIZE;
	
	//This is a cache size for the cities/coordinates.
	static const unsigned int CITY_CACHE_SIZE = 1024;
	
	static const unsigned int GPU_BLOCKS = THREADS / TILE_SIZE;
					
	//unsigned int iterations = 52;
	unsigned int iterations = (number_of_2opts / (THREADS * GPU_BLOCKS)) + 1;
	
	static const int INFINITY = (std::numeric_limits<int>::max)();

	unsigned long perturbations = 0;
 	unsigned long bestLength = INFINITY;
 	unsigned long temp = 0;


inline best2_out min_change2(best2_out const& a, best2_out const& b) restrict(amp)
{
	return (a.minchange_ < b.minchange_) ? a : b;
}
 

template<unsigned int TILE_SIZE, unsigned int TILE_COUNT, unsigned int STATIC_CACHE_SIZE, unsigned int CITY_CACHE_SIZE>
best2_out opt2_simple_kernel(int coordinates_size, unsigned int number_of_2opts, unsigned int iterations,
	concurrency::array_view<const city_coords, 1> const& coordinates_view, concurrency::array_view<const ROUTE_DATA_TYPE, 1> const& route_view, concurrency::accelerator const& chosen_accelerator)
{
	//TODO: This is temporarily here...
	const int INFINITY = (std::numeric_limits<int>::max)();

	//best2_out out_struct = best2_out(0, 0, 0);
	std::array<best2_out, 16> out_structs;
	array_view<best2_out, 1> out(out_structs.size(), out_structs);
	//array_view<best2_out, 1> out(1, &out_struct);
	//out.discard_data();

	//spin_lock spinner;
	//array_view<spin_lock, 1> spin_lock(1, &spinner);
				
	//TILE_SIZE * TILE_COUNT = the number of total threads.
	auto tsp_domain = concurrency::extent<1>(TILE_SIZE * TILE_COUNT).tile<TILE_SIZE>();
	parallel_for_each(chosen_accelerator.default_view, tsp_domain, [=](tiled_index<TILE_SIZE> t_idx) restrict(amp)
	{				
		unsigned int local_id = t_idx.global[0];
		unsigned int packSize = TILE_SIZE * TILE_COUNT;//coordinates_view.extent[0];
		unsigned int max = number_of_2opts;
		unsigned int iter = iterations;
		best2_out best = best2_out(0, 0, INFINITY);
			
		//See the paper for the array lengths...
		tile_static city_coords coords[CITY_CACHE_SIZE];
		tile_static best2_out best_values[STATIC_CACHE_SIZE];
		tile_static best2_out out2[TILE_SIZE];
			
		for(int k = t_idx.local[0]; k < coordinates_size; k += t_idx.tile_dim0)
		{
			coords[k] = coordinates_view[route_view[k]];
		}
				
		for(int k = t_idx.local[0]; k < STATIC_CACHE_SIZE; k += t_idx.tile_dim0)
		{					
			best_values[k] = best;
		}
		t_idx.barrier.wait();
				

		for(unsigned int no = 0; no < iter; no++)
		{				
			unsigned int id = local_id  + no * packSize;
			if(id < max)
			{
				//a nasty formula to calculate the right cell of a triangular matrix index based on the thread id
				unsigned int j = static_cast<unsigned int>(3 + concurrency::fast_math::sqrtf(8.0f * static_cast<float>(id) + 1.0f)) / 2; //l_idx.i;		
				unsigned int i = id - (j - 2) * (j - 1) / 2 + 1; 
		
				//calculate the effect of (i,j) swap
				int change = calculateDistance2DSimple(i, j + 1,   coords) +  calculateDistance2DSimple(i - 1, j, coords) - calculateDistance2DSimple(i, i - 1, coords)   -  calculateDistance2DSimple(j + 1, j, coords);						
				//save if best than already known swap
				if(change < best.minchange_)
				{
					best.minchange_ = change;
					best.i_ = i;
					best.j_ = j + 1;

					best_values[t_idx.local[0]] = best;
				}
			}
		}
		t_idx.barrier.wait();
				
			
		//This reduction number 3 at http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx.
			
		for(int s = TILE_SIZE / 2; s > 0; s /= 2)
		{
			if(t_idx.local[0] < s)
			{
				//best_values[t_idx.local[0]] = min_change(best_values[t_idx.local[0]], best_values[t_idx.local[0] + s]);
				min_change(best_values[t_idx.local[0]], best_values[t_idx.local[0] + s]);
			}
								
			t_idx.barrier.wait();
		}
			
		//Store the tile result in the global memory.
		if(t_idx.local[0] == 0)
		{				
			auto i = t_idx.tile[0];
			out[i] = best_values[0];
		}
	});
		
	out.synchronize();
	auto min_element = *std::min_element(out_structs.begin(), out_structs.end(), [](best2_out const& a, best2_out const& b) { return(a.minchange_ < b.minchange_); });
						
	return min_element;
}
*/