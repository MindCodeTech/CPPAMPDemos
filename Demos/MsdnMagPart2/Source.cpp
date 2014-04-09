#include <amp.h>
using namespace concurrency;
using std::vector;

static const int TS = 16; //16; //2
void MatMul_tiled_step2(vector<int>& vC, const vector<int>& vA, 
                        const vector<int>& vB, int M, int N, int W )
{
    array_view<const int,2> a(M, W, vA), b(W, N, vB);
    array_view<int,2> c(M, N, vC);   
    c.discard_data();

    parallel_for_each(c.extent.tile<TS, TS>(),
        [=](tiled_index<TS, TS> t_idx) restrict(amp)  
    {
        int row = t_idx.local[0]; int col = t_idx.local[1];
        tile_static int locA[TS][TS], locB[TS][TS];
        int sum = 0;
        for (int i = 0; i < a.extent[1]; i += TS) {
            locA[row][col] = a(t_idx.global[0], col + i);
            locB[row][col] = b(row + i, t_idx.global[1]);
            t_idx.barrier.wait();

            for (int k = 0; k < TS; k++)
                sum += locA[row][k] * locB[k][col];            
            t_idx.barrier.wait();
        }
        c[t_idx.global] = sum;
    });
    c.synchronize();
}

void MatMul_tiled_step1(vector<int>& vC, const vector<int>& vA, 
                        const vector<int>& vB, int M, int N, int W )
{
    array_view<const int, 2> a(M, W, vA), b(W, N, vB);
    array_view<int, 2> c(M, N, vC);
    c.discard_data();
    parallel_for_each(c.extent.tile<TS, TS>(), 
        [=](tiled_index<TS, TS> t_idx) restrict(amp) 
    {
        int row = t_idx.global[0]; int col = t_idx.global[1];
        int sum = 0;
        for(int i = 0; i < b.extent[0]; i++) 
            sum += a(row, i) * b(i, col);
        c[t_idx.global] = sum;
    });
    c.synchronize();
}

void MatMul(vector<int>& vC, const vector<int>& vA, 
            const vector<int>& vB, int M, int N, int W )
{
    array_view<const int, 2> a(M, W, vA), b(W, N, vB);
    array_view<int, 2> c(M, N, vC);
    c.discard_data();
    parallel_for_each(c.extent, [=](index<2> idx) restrict(amp) 
    {
        int row = idx[0]; int col = idx[1];
        int sum = 0;
        for(int i = 0; i < b.extent[0]; i++) 
            sum += a(row, i) * b(i, col);
        c[idx] = sum;
    });
    c.synchronize();
}

int horrible_tile_static_intro(vector<int> my_vector)
{
    static const int TS = 2;
    array_view<int, 2> av(2, 6, my_vector);
    parallel_for_each(av.extent.tile<TS,TS>(), 
        [=](tiled_index<TS,TS> t_idx) restrict(amp) 
    {        
        tile_static int t[TS][TS];    
        t[t_idx.local[0]][t_idx.local[1]] = av[t_idx.global];
        t_idx.barrier.wait();
        if (t_idx.local == index<2>(0,0)) {
            t[0][0] = t[0][0] + t[0][1] + t[1][0] + t[1][1];               
            av[t_idx.tile_origin] = t[0][0];
        }
    });
    int sum = av(0,0) + av(0,2) + av(0,4); //the three tile_origins
    return sum;
}

void floating_code()
{
    extent<1> e(12);	 //12 threads in a single dimension
    extent<2> ee(2,6); //12 threads in two-dimensional space

    tiled_extent<6> t_e = e.tile<6>(); 	  //e.rank==t_e.rank
    tiled_extent<2,2> t_ee = ee.tile<2, 2>(); //ee.rank==t_ee.rank

    _ASSERT(e.rank==t_e.rank);
    _ASSERT(e[0] == 12 && t_e.tile_extent[0] == 6); 
    _ASSERT(e[0] % t_e.tile_extent[0] == 0);

    _ASSERT(ee.rank==t_ee.rank);
    _ASSERT(ee[0] == 2 && t_ee.tile_extent[0] == 2);
    _ASSERT(ee[1] == 6 && t_ee.tile_extent[1] == 2);
    _ASSERT(ee[0] % t_ee.tile_extent[0] == 0);
    _ASSERT(ee[1] % t_ee.tile_extent[1] == 0);

    extent<2> my_extent(4,2);
    vector<int> v(4 * 2);
    array_view<int, 2> my_array_view(my_extent, v);
    parallel_for_each(my_extent,[=](index<2> idx) restrict(amp){
        my_array_view[idx] = 42;
    });

    parallel_for_each(my_extent.tile<2,2>(),[=](tiled_index<2,2> t_idx) restrict(amp){
        my_array_view[t_idx.global] = 42;
    });

}

void call_mat_mul()
{
    const int M = 1024; //1024; // 2;
    const int N = 1024; //1024; // 6;
    const int W = 1024; //1024; // 4;

    // 3 vectors
    vector<int> A(M*W);
    vector<int> B(W*N);
    vector<int> C(M*N);

    // populate vector A
    for (unsigned int i = 0; i < A.size(); i++)
        A[i] = i + 1;

    // populate vector B
    for (unsigned int i = 0; i < B.size(); i++)
        B[i] = i + 1;

    MatMul(C, A, B, M, N, W);
    MatMul_tiled_step1(C, A, B, M, N, W);
    MatMul_tiled_step2(C, A, B, M, N, W);
}

void main()
{
    floating_code();

    vector<int> v(2*6, 2);
    int sum = horrible_tile_static_intro(v);
    _ASSERT(sum == 24);

    call_mat_mul();
}