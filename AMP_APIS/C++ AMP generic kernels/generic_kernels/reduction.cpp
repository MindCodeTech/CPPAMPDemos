//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 Arnaud Faucher
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "reduction.h"

#include "private_util.h"

#include <amp.h>

using namespace concurrency;

template <class T, unsigned int tileSize, bool nIsPow2, class op>
void reduce_amp(const tiled_index<tileSize>& t_idx,
				const array_view<const T>& idata,
				const array_view<writeonly<T>>& odata,
				unsigned int size,
				unsigned int numTiles) restrict(amp)
{
	op f;
	tile_static T sdata[tileSize];

	// Perform the first levels of reduction and write to tile static memory
	unsigned int tid = t_idx.local[0];
	unsigned int i = t_idx.tile[0]*tileSize*2U + tid;
	unsigned int gridSize = tileSize*2U*numTiles;

	T res = f.identity();

	while (i < size)
	{
		f(res, idata[i]);
		if (nIsPow2 || i + tileSize < size)
			f(res, idata[i + tileSize]);
		i += gridSize;
	}

	// each thread puts its local result into tile static memory 
	sdata[tid] = res;
	t_idx.barrier.wait();

	// perform reduction in tile static memory
	if (tileSize >= 512) { if (tid < 256) { f(sdata[tid], sdata[tid + 256]); } t_idx.barrier.wait(); }
	if (tileSize >= 256) { if (tid < 128) { f(sdata[tid], sdata[tid + 128]); } t_idx.barrier.wait(); }
	if (tileSize >= 128) { if (tid < 64) { f(sdata[tid], sdata[tid +  64]); } t_idx.barrier.wait(); }
	if (tileSize >= 64) { if (tid < 32) { f(sdata[tid], sdata[tid +  32]); } t_idx.barrier.wait(); }
	if (tileSize >= 32) { if (tid < 16) { f(sdata[tid], sdata[tid +  16]); } t_idx.barrier.wait(); }
	if (tileSize >= 16) { if (tid < 8) { f(sdata[tid], sdata[tid +  8]); } t_idx.barrier.wait(); }
	if (tileSize >= 8) { if (tid < 4) { f(sdata[tid], sdata[tid +  4]); } t_idx.barrier.wait(); }
	if (tileSize >= 4) { if (tid < 2) { f(sdata[tid], sdata[tid +  2]); } t_idx.barrier.wait(); }
	if (tileSize >= 2) { if (tid < 1) { f(sdata[tid], sdata[tid +  1]); } }

	// write result for this tile to global memory
	if (tid == 0)
		odata[t_idx.tile[0]] = sdata[0];
}

namespace generic_kernels
{
	template <class T, class op>
	reduction<T,op>::reduction()
		: _odata(_maxTiles)
		, _acc(accelerator(accelerator::default_accelerator).default_view)
	{
	}

	template <class T, class op>
	reduction<T,op>::reduction(accelerator_view acc)
		: _odata(_maxTiles)
		, _acc(acc)
	{
	}

	template <class T, class op>
	T reduction<T,op>::reduce(concurrency::array_view<const T> idata)
	{
		op f;

		unsigned int size = idata.extent[0];
		unsigned int threads = (size < _maxThreads*2) ? nextPow2((size + 1) / 2) : _maxThreads;

		unsigned int numTiles = (size + (threads * 2 - 1)) / (threads * 2);
		if (numTiles > _maxTiles)
			numTiles = _maxTiles;

		grid<1> g(extent<1>(numTiles*threads));

		array_view<writeonly<T>> odata(numTiles, _odata);

		if (isPow2(size))
		{
			switch (threads)
			{
			case 512:
				parallel_for_each(_acc, g.tile<512>(), [=](tiled_index<512> t_idx) restrict(amp)
				{
					reduce_amp<T, 512, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 256:
				parallel_for_each(_acc, g.tile<256>(), [=](tiled_index<256> t_idx) restrict(amp)
				{
					reduce_amp<T, 256, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 128:
				parallel_for_each(_acc, g.tile<128>(), [=](tiled_index<128> t_idx) restrict(amp)
				{
					reduce_amp<T, 128, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 64:
				parallel_for_each(_acc, g.tile<64>(), [=](tiled_index<64> t_idx) restrict(amp)
				{
					reduce_amp<T, 64, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 32:
				parallel_for_each(_acc, g.tile<32>(), [=](tiled_index<32> t_idx) restrict(amp)
				{
					reduce_amp<T, 32, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 16:
				parallel_for_each(_acc, g.tile<16>(), [=](tiled_index<16> t_idx) restrict(amp)
				{
					reduce_amp<T, 16, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  8:
				parallel_for_each(_acc, g.tile<8>(), [=](tiled_index<8> t_idx) restrict(amp)
				{
					reduce_amp<T, 8, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  4:
				parallel_for_each(_acc, g.tile<4>(), [=](tiled_index<4> t_idx) restrict(amp)
				{
					reduce_amp<T, 4, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  2:
				parallel_for_each(_acc, g.tile<2>(), [=](tiled_index<2> t_idx) restrict(amp)
				{
					reduce_amp<T, 2, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  1:
				parallel_for_each(_acc, g.tile<1>(), [=](tiled_index<1> t_idx) restrict(amp)
				{
					reduce_amp<T, 1, true, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			}
		}
		else
		{
			switch (threads)
			{
			case 512:
				parallel_for_each(_acc, g.tile<512>(), [=](tiled_index<512> t_idx) restrict(amp)
				{
					reduce_amp<T, 512, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 256:
				parallel_for_each(_acc, g.tile<256>(), [=](tiled_index<256> t_idx) restrict(amp)
				{
					reduce_amp<T, 256, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 128:
				parallel_for_each(_acc, g.tile<128>(), [=](tiled_index<128> t_idx) restrict(amp)
				{
					reduce_amp<T, 128, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 64:
				parallel_for_each(_acc, g.tile<64>(), [=](tiled_index<64> t_idx) restrict(amp)
				{
					reduce_amp<T, 64, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 32:
				parallel_for_each(_acc, g.tile<32>(), [=](tiled_index<32> t_idx) restrict(amp)
				{
					reduce_amp<T, 32, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case 16:
				parallel_for_each(_acc, g.tile<16>(), [=](tiled_index<16> t_idx) restrict(amp)
				{
					reduce_amp<T, 16, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  8:
				parallel_for_each(_acc, g.tile<8>(), [=](tiled_index<8> t_idx) restrict(amp)
				{
					reduce_amp<T, 8, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  4:
				parallel_for_each(_acc, g.tile<4>(), [=](tiled_index<4> t_idx) restrict(amp)
				{
					reduce_amp<T, 4, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  2:
				parallel_for_each(_acc, g.tile<2>(), [=](tiled_index<2> t_idx) restrict(amp)
				{
					reduce_amp<T, 2, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			case  1:
				parallel_for_each(_acc, g.tile<1>(), [=](tiled_index<1> t_idx) restrict(amp)
				{
					reduce_amp<T, 1, false, op>(t_idx, idata, odata, size, numTiles);
				});
				break;
			}
		}

		// Perform last step of reduction on the CPU
		odata.synchronize();

		T res = f.identity();
		for (size_t i = 0; i < numTiles; ++i)
			f(res, _odata[i]);


		return res;
	}

	// Instanciate the reduction class
	template class reduction<int, add_op<int>>;
	template class reduction<unsigned int, add_op<unsigned int>>;
	template class reduction<float, add_op<float>>;
	template class reduction<float_3, add_op<float_3>>;
	template class reduction<float_4, add_op<float_4>>;
	template class reduction<double, add_op<double>>;
	
	template class reduction<int, mul_op<int>>;
	template class reduction<unsigned int, mul_op<unsigned int>>;
	template class reduction<float, mul_op<float>>;
	template class reduction<float_3, mul_op<float_3>>;
	template class reduction<float_4, mul_op<float_4>>;
	template class reduction<double, mul_op<double>>;
	
	template class reduction<int, max_op<int>>;
	template class reduction<unsigned int, max_op<unsigned int>>;
	template class reduction<float, max_op<float>>;
	template class reduction<double, max_op<double>>;

	template class reduction<int, min_op<int>>;
	template class reduction<unsigned int, min_op<unsigned int>>;
	template class reduction<float, min_op<float>>;
	template class reduction<double, min_op<double>>;
}
