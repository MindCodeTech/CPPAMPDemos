// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
#include <amp.h>
#include <vector>
using namespace concurrency;
#define N 16
#define BLOCK_SIZE (N / 2)

template <typename T>
void PrefixSumV1(std::vector<T> &Input, std::vector<T> &Output)
{
    array_view<const T, 1> input(N, Input);
    array_view<T, 1> output(N, Output);

    parallel_for_each(extent<1>(N).tile<N>(), [=] (tiled_index<N> tidx) restrict (amp)
                      {
                        // localIdx == globalIdx
                        // index<1> localIdx = tidx.local;
                        index<1> globalIdx = tidx.global;
                        tile_static T temp[N];
                        unsigned stride = 1;

                        temp[globalIdx[0]] = input(globalIdx);

                        tidx.barrier.wait();

                        for (unsigned offset = 1; offset < N; offset *= 2) {
                            if (globalIdx[0] >= offset)
                                temp[globalIdx[0]] += temp[globalIdx[0] - stride]; 
                            tidx.barrier.wait();
                            stride *= 2;
                        }
                        output(globalIdx) = temp[globalIdx[0]];
                      });
}

// Reduction + Inclusive Post Scan
template <typename T>
void PrefixSumV2(std::vector<T> &Input, std::vector<T> &Output)
{
    array_view<const T, 1> input(N, Input);
    array_view<T, 1> output(N, Output);

    parallel_for_each(extent<1>(N).tile<N>(), [=] (tiled_index<N> tidx) restrict (amp)
                      {
                        const unsigned globalIdx = tidx.global[0];
                        const unsigned localIdx = tidx.local[0];
                        tile_static T temp[N];

                        temp[localIdx] = input[globalIdx];

                        tidx.barrier.wait();

                        // reduction step
                        unsigned stride = 1;
                        while (stride <= BLOCK_SIZE) {
                            unsigned index = (localIdx + 1) * stride * 2 - 1;
                            if (index < N)
                                temp[index] += temp[index - stride];
                            stride *= 2;
                            tidx.barrier.wait();
                        }

                        stride = BLOCK_SIZE / 2;
                        while (stride) {
                            unsigned index = (localIdx + 1) * stride * 2 - 1;
                            if (index < N)
                                temp[index + stride] += temp[index];
                            stride /= 2;
                            tidx.barrier.wait();
                        }

                        output[globalIdx] = temp[localIdx];
                      });

}

template <typename T>
void ExclusivePrefixSum(std::vector<T> &Input, std::vector<T> &Output)
{
    array_view<const T, 1> input(N, Input);
    array_view<T, 1> output(N, Output);

    extent<1> compute_domain(N);

    parallel_for_each(compute_domain.tile<N>(), [=] (tiled_index<N> tidx) restrict (amp)
                      {
                        index<1> globalIdx = tidx.global;
                        tile_static T temp[N];

                        temp[globalIdx[0]] = input[globalIdx[0]];

                        tidx.barrier.wait();

                        // reduction step
                        unsigned stride = 1;
                        while (stride <= BLOCK_SIZE) {
                            unsigned index = (globalIdx[0] + 1) * stride * 2 - 1;
                            if (index < N)
                                temp[index] += temp[index - stride];
                            stride *= 2;
                            tidx.barrier.wait();
                        }

                        if (globalIdx[0] == 0)
                            temp[N - 1] = 0;

                        stride = BLOCK_SIZE;
                        while (stride) {
                            unsigned index = (globalIdx[0] + 1) * stride * 2 - 1;
                            if (index < N) {
                                T t = temp[index];
                                temp[index] += temp[index - stride];
                                temp[index - stride] = t;
                            }
                            stride /= 2;
                            tidx.barrier.wait();
                        }

                        output[globalIdx[0]] = temp[globalIdx[0]];
                      });
}

int main(void)
{
    std::vector<int> input(N), output(N, 0);
    for (int i = 0; i < N; i++)
        input[i] = i + 1;
    for (int i = 0; i < N; i++) 
        printf("%3d%c", input[i], (i + 1) & 15 ? ' ' : '\n');
    printf("\n --------------------\n");
    PrefixSumV1(input, output);
    for (int i = 0; i < N; i++) 
        printf("%3d%c", output[i], (i + 1) & 15 ? ' ' : '\n');
    for (int i = 0; i < N; i++)
        output[i] = 0;
    printf("\n --------------------\n");
    PrefixSumV2(input, output);
    for (int i = 0; i < N; i++) 
        printf("%3d%c", output[i], (i + 1) & 15 ? ' ' : '\n');
    for (int i = 0; i < N; i++)
        output[i] = 0;
    printf("\n --------------------\n");
    ExclusivePrefixSum(input, output);
    for (int i = 0; i < N; i++) 
        printf("%3d%c", output[i], (i + 1) & 15 ? ' ' : '\n');

    return 0;
}

