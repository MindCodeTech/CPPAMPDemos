//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: BinomialOptions.cpp
// 
// Implement GPU based binomial option pricing. Verify correctness with CPU 
// implementation
//----------------------------------------------------------------------------

#include <vector>
#include <assert.h>
#include <iostream>
#include <amp.h>
#include <amp_math.h>

#pragma warning (disable : 4267)

using namespace concurrency;

// Date set - small and normal
// small problem size
#define  MAX_OPTIONS    (32)
#define  NUM_STEPS      (64)
#define  TIME_STEPS     (2)
#define  CACHE_DELTA    (2 * TIME_STEPS)
#define  CACHE_SIZE     (16)
#define  CACHE_STEP     (CACHE_SIZE - CACHE_DELTA)

//// normal problem size
//#define  MAX_OPTIONS    (512)
//#define  NUM_STEPS      (2048)
//#define  TIME_STEPS     (16)
//#define  CACHE_DELTA    (2 * TIME_STEPS)
//#define  CACHE_SIZE     (256)
//#define  CACHE_STEP     (CACHE_SIZE - CACHE_DELTA)

typedef struct _option_data 
{
    float s;
    float x;
    float t;
    float r;
    float v;
} t_option_data;

#if NUM_STEPS % CACHE_DELTA
    #error Bad constants
#endif

//----------------------------------------------------------------------------
// Generate random data between specified values
//----------------------------------------------------------------------------
float random_data(float lo, float hi) 
{
    int len = (int) ::ceil(hi - lo);
    return lo + ::rand() % len;
}

//----------------------------------------------------------------------------
// GPU/CPU implementation - Call value at period t : V(t) = S(t) - X
//----------------------------------------------------------------------------
float expiry_call_value(float s, float x, float vdt, int t) restrict(amp, cpu) 
{
    float d = s * fast_math::exp(vdt * (2.0f * t - NUM_STEPS)) - x;
    return (d > 0) ? d : 0;
}

//----------------------------------------------------------------------------
// GPU implementation of binomial options tree walking to calculate option pricing
// Refer README.txt for more details on algorithm
//----------------------------------------------------------------------------
void binomial_options_kernel(tiled_index<CACHE_SIZE> &tidx,
                           array_view<const float, 1> s, array_view<const float, 1> x, 
                           array_view<const float, 1> vdt, array_view<const float, 1> pu_by_df, 
                           array_view<const float, 1> pd_by_df, array_view<float, 1> call_value, 
                           array<float, 1> &call_buffer) restrict(amp) 
{
    index<1> tile_idx = tidx.tile;
    index<1> local_idx = tidx.local;

    tile_static float call_a[CACHE_SIZE+1];
    tile_static float call_b[CACHE_SIZE+1];

    //Global memory frame for current option (thread group)
    int tid = local_idx[0];
    int i;

    // CACHE_SIZE number of thread are operating, hence steping by CACHE_SIZE
    // below for loop is similar to first inner loop of binomial_options_cpu
    //Compute values at expiry date
    for(i = tid; i <= NUM_STEPS; i += CACHE_SIZE) 
	{
		index<1> idx(tile_idx[0] * (NUM_STEPS + 16) + (i));
        call_buffer[idx] = expiry_call_value(s[tile_idx], x[tile_idx], vdt[tile_idx], i);
	}

    // Walk down binomial tree - equivalent to 2nd inner loop of binomial_options_cpu
    //                              Additional boundary checking 
    // So double-buffer and synchronize to avoid read-after-write hazards.
    for(i = NUM_STEPS; i > 0; i -= CACHE_DELTA)
	{
        for(int c_base = 0; c_base < i; c_base += CACHE_STEP)
		{
            //Start and end positions within shared memory cache
            int c_start = min(CACHE_SIZE - 1, i - c_base);
            int c_end   = c_start - CACHE_DELTA;

            //Read data(with apron) to shared memory
            tidx.barrier.wait();
            if(tid <= c_start)
			{
				index<1> idx(tile_idx[0] * (NUM_STEPS + 16) + (c_base + tid));
                call_a[tid] = call_buffer[idx];
			}

            //Calculations within shared memory
            for(int k = c_start - 1; k >= c_end;)
			{
                //Compute discounted expected value
                tidx.barrier.wait();
                call_b[tid] = pu_by_df[tile_idx] * call_a[tid + 1] + pd_by_df[tile_idx] * call_a[tid];
                k--;

                //Compute discounted expected value
                tidx.barrier.wait();
                call_a[tid] = pu_by_df[tile_idx] * call_b[tid + 1] + pd_by_df[tile_idx] * call_b[tid];
                k--;
            }

            //Flush shared memory cache
            tidx.barrier.wait();
            if(tid <= c_end)
			{
				index<1> idx(tile_idx[0] * (NUM_STEPS + 16) + (c_base + tid));
                call_buffer[idx] = call_a[tid];
			}
        }
	}

    //Write the value at the top of the tree to destination buffer
    if (tid == 0) 
        call_value[tile_idx] = call_a[0];
}

//----------------------------------------------------------------------------
// Sequential(CPU) binomial option calculation 
//----------------------------------------------------------------------------
void binomial_options_cpu(std::vector<float>& v_s, std::vector<float>& v_x,
								std::vector<float>& v_vdt, std::vector<float>& v_pu_by_df,
								std::vector<float>& v_pd_by_df,
								std::vector<float>& call_value) 
{
    const unsigned data_size = v_s.size();
    // all data size should be same
    assert(data_size == MAX_OPTIONS); // kernel uses this macro directly
    assert((v_s.size() == data_size) && (v_x.size() == data_size) &&  (v_vdt.size() == data_size) && 
        (v_pu_by_df.size() == data_size) && (v_pd_by_df.size() == data_size) && (call_value.size() == data_size));

    // this is like GPU kernel - where we have the meat
    for (unsigned i = 0; i < data_size; i++)
    {
        float call[NUM_STEPS + 1];

        // Compute values at expiration date:
        // call option value at period end is V(T) = S(T) - X
        // if S(T) is greater than X, or zero otherwise.
        // The computation is similar for put options.
        for(int j = 0; j <= NUM_STEPS; j++)
            call[j] = expiry_call_value(v_s[i], v_x[i], v_vdt[i], j);

        // Walk backwards up binomial tree
        for(int j = NUM_STEPS; j > 0; j--)
            for(int k = 0; k <= j - 1; k++)
                call[k] = v_pu_by_df[i] * call[k + 1] + v_pd_by_df[i] * call[k];

        call_value[i] = call[0];
    }
}

//----------------------------------------------------------------------------
// Wrapper offloading computation to GPU
// This function calls C++ AMP binomial option kernel
//----------------------------------------------------------------------------
void binomial_options_gpu(std::vector<float>& v_s, std::vector<float>& v_x,
								std::vector<float>& v_vdt, std::vector<float>& v_pu_by_df,
								std::vector<float>& v_pd_by_df,
								std::vector<float>& call_value)
{
    const unsigned data_size = v_s.size();
    // all data size should be same
    assert(data_size == MAX_OPTIONS); // kernel uses this macro directly
    assert((v_s.size() == data_size) && (v_x.size() == data_size) &&  (v_vdt.size() == data_size) && 
        (v_pu_by_df.size() == data_size) && (v_pd_by_df.size() == data_size) && (call_value.size() == data_size));

	extent<1> e(data_size);
	array_view<float, 1> av_call_value(e, call_value);
	av_call_value.discard_data();

	array_view<const float, 1> av_s(e, v_s);
	array_view<const float, 1> av_x(e, v_x);
	array_view<const float, 1> av_vdt(e, v_vdt);
	array_view<const float, 1> av_pu_by_df(e, v_pu_by_df);
	array_view<const float, 1> av_pd_by_df(e, v_pd_by_df);

    // Used as temporary buffer on GPU
    extent<1> ebuf(MAX_OPTIONS*(NUM_STEPS + 16));
	array<float, 1> a_call_buffer(ebuf);

    // Totally we have 
    // number of tiles = (CACHE_SIZE * MAX_OPTIONS)/CACHE_SIZE
    // number of threads/tile = CACHE_SIZE;
	extent<1> compute_extent(CACHE_SIZE * MAX_OPTIONS);
	parallel_for_each(compute_extent.tile<CACHE_SIZE>(),  [=, &a_call_buffer](tiled_index<CACHE_SIZE> ti) restrict(amp) 
	{
		binomial_options_kernel(ti, av_s, av_x, av_vdt, av_pu_by_df, av_pd_by_df, av_call_value, a_call_buffer);
	});
	av_call_value.synchronize();
}

//----------------------------------------------------------------------------
// Generated random data is normalized to conform for Binomial calculation
//----------------------------------------------------------------------------
void normalize_data(t_option_data* option_data, int num_options, 
            std::vector<float>& v_s, std::vector<float>& v_x, std::vector<float>& v_vdt, 
            std::vector<float>& v_pu_by_df, std::vector<float>& v_pd_by_df)
{
    for (int i = 0; i < num_options; i++)
    {
        v_s[i] = option_data[i].s;
        v_x[i] = option_data[i].x;

        float t = option_data[i].t;
        float r = option_data[i].r;
        float v = option_data[i].v;

        float dt = t / (float)NUM_STEPS;
        v_vdt[i] = v * ::sqrt(dt);
        float rdt = r * dt;

        //Per-step interest and discount factors
        float iff = ::exp(rdt);
        float df = ::exp(-rdt);

        //Values and pseudoprobabilities of upward and downward moves
        float u = ::exp(v_vdt[i]);
        float d = ::exp(-v_vdt[i]);
        float pu = (iff - d) / (u - d);
        float pd = 1.0f - pu;
        v_pu_by_df[i] = pu * df;
        v_pd_by_df[i] = pd * df;
    }
}


int main()
{
	accelerator default_device;
	std::wcout << L"Using device : " << default_device.get_description() << std::endl;
	if (default_device == accelerator(accelerator::direct3d_ref))
		std::cout << "WARNING!! Running on very slow emulator! Only use this accelerator for debugging." << std::endl;

    int num_options = MAX_OPTIONS;

    std::vector<float> call_value_gpu(num_options);
    std::vector<float> call_value_cpu(num_options);
    t_option_data option_data[MAX_OPTIONS];

    //Generate options set
    srand(123);
    for(int i = 0; i < num_options; i++) 
    {

        option_data[i].s = random_data(5.0f, 30.0f);
        option_data[i].x = random_data(1.0f, 100.0f);
        option_data[i].t = random_data(0.25f, 10.0f);
        option_data[i].r = 0.06f;
        option_data[i].v = 0.10f;
    }

    std::vector<float> v_s(num_options);
    std::vector<float> v_x(num_options);
    std::vector<float> v_vdt(num_options);
    std::vector<float> v_pu_by_df(num_options);
    std::vector<float> v_pd_by_df(num_options);

    normalize_data(option_data, num_options, v_s, v_x, v_vdt, v_pu_by_df, v_pd_by_df);

	printf ("CPU computation...");
	binomial_options_cpu(v_s, v_x, v_vdt, v_pu_by_df, v_pd_by_df, call_value_cpu);
	printf("done\n");

	printf ("GPU computation...");
    binomial_options_gpu(v_s, v_x, v_vdt, v_pu_by_df, v_pd_by_df, call_value_gpu);
	printf("done\n");

    double sum_delta = 0, sum_ref = 0, error_val = 0;
    printf("\nCPU binomial vs. GPU binomial\n");
    for (int i = 0; i < num_options; i++)
    {
        sum_delta += ::fabs(call_value_gpu[i] - call_value_cpu[i]);
        sum_ref += call_value_gpu[i];
    }
    if(sum_ref > 1E-5)
        printf("L1 norm: %E\n", error_val = sum_delta / sum_ref);
    else
        printf("Avg. diff: %E\n", error_val = sum_delta / (double)num_options);
	printf((error_val < 5e-4) ? "Data match\n" : "Data doesn't match\n"); 

	printf ("Press ENTER to exit this program\n");
	getchar();
}


