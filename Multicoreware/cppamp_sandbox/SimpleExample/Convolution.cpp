// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
/*
 * The implementation of the cuda code 
 * taught in ECE408/CS483 in UIUC by
 * Prof Hwu.
 * http://ece408.hwu.crhc.illinois.edu/SitePages/Home.aspx
 */

#include <amp.h>
#include <vector>
#include <iostream>
using namespace concurrency;
#define TILE_SIZE 4
#define Width 8
#define Mask_Width 5
    template <typename T>
void convolution1DV1(std::vector<T> &N, std::vector<T> &M, std::vector<T> &P)
{
    array_view<const T, 1> n(Width, N);
    array_view<const T, 1> m(Mask_Width, M);
    array_view<T, 1> p(Width, P);

    extent<1> compute_domain(Width);
    parallel_for_each(compute_domain, [=] (index<1> idx) restrict (amp)
                      {
                        T Pvalue = 0;
                        int N_start_point = idx[0] - (Mask_Width / 2);
                        for (int i = 0; i < Mask_Width; i++) {
                            if (N_start_point + i >= 0 && N_start_point + i < Width)
                            Pvalue += n[N_start_point + i] * m[i];
                        }
                        p[idx] = Pvalue;
                      });

}
    template <typename T>
void convolution1DV2(std::vector<T> &cn, std::vector<T> &cm, std::vector<T> &cp)
{
    array_view<const T, 1> N(Width, cn);
    array_view<const T, 1> M(Mask_Width, cm);
    array_view<T, 1> P(Width, cp);

    parallel_for_each(extent<1>(Width).tile<TILE_SIZE>(), [=] (tiled_index<TILE_SIZE> tidx) restrict (amp)
                      {
                        index<1> globalIdx = tidx.global;
                        index<1> localIdx = tidx.local;
                        tile_static T n_ds[TILE_SIZE + Mask_Width - 1];

                        int n = Mask_Width / 2;

                        int halo_index_left = globalIdx[0] - TILE_SIZE;
                        if (localIdx[0] >= TILE_SIZE - n)
                            n_ds[localIdx[0] - (TILE_SIZE - n)] = 
                                (halo_index_left < 0) ? 0 : N[halo_index_left];

                        n_ds[n + localIdx[0]] = N[globalIdx[0]];

                        int halo_index_right = globalIdx[0] + TILE_SIZE;
                        if (localIdx[0] < n) 
                            n_ds[n + TILE_SIZE + localIdx[0]] = 
                                (halo_index_right >= Width) ? 0 : N[halo_index_right];

                        tidx.barrier.wait();
                        T Pvalue = 0;
                        for (int i = 0; i < Mask_Width; i++)
                            Pvalue += n_ds[localIdx[0] + i] * M[i];
                        P[globalIdx[0]] = Pvalue;
                      });

}

    template <typename T>
void convolution1DV3(std::vector<T> &cn, std::vector<T> &cm, std::vector<T> &cp)
{
    array_view<const T, 1> N(Width, cn);
    array_view<const T, 1> M(Mask_Width, cm);
    array_view<T, 1> P(Width, cp);

    parallel_for_each(extent<1>(Width).tile<TILE_SIZE>(), [=] (tiled_index<TILE_SIZE> tidx) restrict (amp)
                      {
                        const int globalIdx = tidx.global[0];
                        const int localIdx = tidx.local[0];
                        tile_static T N_ds[TILE_SIZE];

                        N_ds[localIdx] = N[globalIdx];

                        tidx.barrier.wait();
                        const int This_tile_start_point = tidx.tile_origin[0];
                        const int Next_tile_start_point = tidx.tile_origin[0] + TILE_SIZE;
                        const int N_start_point = globalIdx - (Mask_Width / 2);
                        T Pvalue = 0;
                        for (int i = 0; i < Mask_Width; i++) {
                            int N_index = N_start_point + i;
                            if (N_index >= 0 && N_index < Width) {
                                if ((N_index >= This_tile_start_point) && 
                                    (N_index < Next_tile_start_point))
                                    Pvalue += N_ds[localIdx + i - (Mask_Width / 2)] * M[i];
                                else
                                    Pvalue += N[N_index] * M[i];
                            }
                        }
                        P[globalIdx] = Pvalue;
                      });

}

template <typename T>
void print(std::vector<T>& vec)
{
    std::cout << std::endl << "---------output----------" << std::endl;
    for (int i = 0; i < vec.size(); i++)
        std::cout << vec[i] << " ";
    std::cout << std::endl << "-------------------------" << std::endl;
}

int main(void)
{
    std::vector<int> N{1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<int> M{3, 4, 5, 4, 3};
    std::vector<int> P(Width, 0);
    convolution1DV1(N, M, P);
    print(P);
    convolution1DV2(N, M, P);
    print(P);
    convolution1DV3(N, M, P);
    print(P);
    return 0;
}

