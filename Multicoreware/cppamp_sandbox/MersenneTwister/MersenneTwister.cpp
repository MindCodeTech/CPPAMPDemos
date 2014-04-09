// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %cxxamp %link %t/kernel.o %s -o %t.out && %t.out
#include <math.h>
#include <fstream>
#include <iostream>
#include <amp.h>
#include <amp_math.h>


#pragma warning (disable : 4267)

using namespace concurrency;

//----------------------------------------------------------------------------
// Common host and device function 
//----------------------------------------------------------------------------
inline int div_up(int a, int b) {
  return (a+b-1) / b;
}

// Align a to nearest higher multiple of b
inline int align_up(int a, int b) {
  return ((a+b-1) / b) * b;
}

typedef struct {
  unsigned int matrix_a;
  unsigned int mask_b;
  unsigned int mask_c;
  unsigned int seed;
} mt_struct;

#define   MT_RNG_COUNT 4096
#define          MT_MM 9
#define          MT_NN 19
#define       MT_WMASK 0xFFFFFFFFU
#define       MT_UMASK 0xFFFFFFFEU
#define       MT_LMASK 0x1U
#define      MT_SHIFT0 12
#define      MT_SHIFTB 7
#define      MT_SHIFTC 15
#define      MT_SHIFT1 18

//----------------------------------------------------------------------------
// Data configuration
//----------------------------------------------------------------------------
const int g_path_n = 256;
const int g_n_per_RNG = align_up(div_up(g_path_n, MT_RNG_COUNT), 2);
const int g_rand_n = MT_RNG_COUNT * g_n_per_RNG; // how many #'s

const unsigned int g_seed = 777;

// Load twister configurations
void load_MT(const char *fname, mt_struct* data, int elements) {
  FILE *fd;
#if _WIN32
  if(0 != fopen_s(&fd, fname, "rb")) {
#else
  if (!(fd = fopen(fname, "r"))) {
#endif
    printf("load_MT(): failed to open %s\n", fname);
    printf("TEST FAILED\n");
    exit(0);
  }
  if (fread(data, elements * sizeof(mt_struct), 1, fd) ==
      elements * sizeof(mt_struct)) {
    printf("load_MT(): failed to load %s\n", fname);
    printf("TEST FAILED\n");
    exit(0);
  }
  fclose(fd);
}

// Initialize/seed twister
void seed_MT(unsigned int seed0, mt_struct* data,
             std::vector<unsigned int>& matrix,
             std::vector<unsigned int>& mask_b,
             std::vector<unsigned int>& mask_c, 
             std::vector<unsigned int>& seed) {
  for (int i = 0; i < MT_RNG_COUNT; i++) {
    matrix[i] = data[i].matrix_a;
    mask_b[i] = data[i].mask_b;
    mask_c[i] = data[i].mask_c;
    seed[i] = seed0;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Write MT_RNG_COUNT vertical lanes of n_per_RNG random numbers to random_nums.
// For coalesced global writes MT_RNG_COUNT should be a multiple of hardware scehduling unit size.
// Hardware scheduling unit is called warp or wave or wavefront
// Initial states for each generator are the same, since the states are
// initialized from the global seed. In order to improve distribution properties
// on small n_per_RNG supply dedicated (local) seed to each twister.
// The local seeds, in their turn, can be extracted from global seed
// by means of any simple random number generator, like LCG.
////////////////////////////////////////////////////////////////////////////////
void rand_MT_kernel(index<1> idx,
                    array<float, 2>& random_nums,
                    const unsigned int matrix_a,
                    const unsigned int mask_b,
                    const unsigned int mask_c,
                    const unsigned int seed,
                    const int n_per_RNG,
                    array<float, 1>& cstF) restrict(amp) {
  int state_1;
  int state_M;
  unsigned int mti, mti_M, x;
  unsigned int mti_1, mt[MT_NN];

  // Bit-vector Mersenne Twister parameters are in matrix_a, mask_b, mask_c, seed
  // Initialize current state
  mt[0] = seed;
  for (int state = 1; state < MT_NN; state++) {
    mt[state] = (1812433253U * (mt[state - 1] ^ (mt[state - 1] >> 30))
                             + state) & MT_WMASK;
  }

  mti_1 = mt[0];
  for (int out = 0, state = 0; out < n_per_RNG; out++) {
    state_1 = state + 1;
    state_M = state + MT_MM;
    if (state_1 >= MT_NN) state_1 -= MT_NN;
    if (state_M >= MT_NN) state_M -= MT_NN;
    mti = mti_1;
    mti_1 = mt[state_1];
    mti_M = mt[state_M];

    x = (mti & MT_UMASK) | (mti_1 & MT_LMASK);
    x = mti_M ^ (x >> 1) ^ ((x & 1) ? matrix_a : 0);
    mt[state] = x;
    state = state_1;

    // Tempering transformation
    x ^= (x >> MT_SHIFT0);
    x ^= (x << MT_SHIFTB) & mask_b;
    x ^= (x << MT_SHIFTC) & mask_c;
    x ^= (x >> MT_SHIFT1);

    // Convert to (0, 1) float and write to global memory
    // Using unsigned int max, to convert a uniform number in uint range to a
    // uniform range over [-1 ... 1] 
    random_nums[index<2>(out, idx[0])] = ((float)x + 1.0f) / cstF[0];
  }
}

////////////////////////////////////////////////////////////////////////////////
// Transform each of MT_RNG_COUNT lanes of n_per_RNG uniformly distributed 
// random samples, produced by rand_MT_amp(), to normally distributed lanes
// using Cartesian form of Box-Muller transformation.
// n_per_RNG must be even.
////////////////////////////////////////////////////////////////////////////////
void box_muller_transform(float& u1, float& u2, float& cstF) restrict(amp) {
  float r = fast_math::sqrt(-2.0f * fast_math::log(u1));
  float phi = 2.0f * cstF * u2;
  u1 = r * fast_math::cos(phi);
  u2 = r * fast_math::sin(phi);
}

void box_muller_kernel(index<1> idx, array<float, 2>& random_nums,
                       array<float, 2>& normalized_random_nums,
                       int n_per_RNG, array<float, 1>& cstF) restrict(amp) {
  for (int out = 0; out < n_per_RNG; out += 2) {
    index<2> zero(out, idx[0]);
    index<2> one(out + 1, idx[0]);
    float f0 = random_nums[zero];
    float f1 = random_nums[one];
    float pad = cstF[0];
    box_muller_transform(f0, f1, pad);
    normalized_random_nums[zero] = f0;
    normalized_random_nums[one] = f1;
  }
}

void generate_rand_on_amp(std::vector<unsigned int>& v_matrix_a,
                          std::vector<unsigned int>& v_mask_b,
                          std::vector<unsigned int>& v_mask_c,
                          std::vector<unsigned int>& v_seed,
                          std::vector<float>& v_random_nums,
                          bool apply_transform = true) {
  extent<1> e_c(v_matrix_a.size());
  int n_per_RNG = g_n_per_RNG;
  extent<2> rn(g_n_per_RNG, MT_RNG_COUNT);

  array<float, 2> random_nums(rn); 
  array<float, 2> normalized_random_nums(rn);

  // Copy to GPU
  array<unsigned int, 1> matrix_a(e_c, v_matrix_a.begin());
  array<unsigned int, 1> seed(e_c, v_seed.begin());
  array<unsigned int, 1> mask_b(e_c, v_mask_b.begin());
  array<unsigned int, 1> mask_c(e_c, v_mask_c.begin());

  std::vector<float> farr(4);
  farr[0] = 4294967296.0f;
  array<float, 1> cstF(4, farr.begin());

  std::vector<float> farr2(4);
  farr2[0] = 3.14159265358979f;
  array<float, 1> cstF2(4, farr2.begin());

  // generate random numbers
  parallel_for_each(e_c,
                    [=, &random_nums, &matrix_a, &mask_b, &mask_c, &seed, &cstF]
                        (index<1> idx) restrict(amp) {
    rand_MT_kernel(idx, random_nums, matrix_a[idx], mask_b[idx], mask_c[idx],
                   seed[idx], n_per_RNG, cstF);
  });

  if (apply_transform) {
    parallel_for_each(e_c,
                      [=, &random_nums, &normalized_random_nums, &cstF2]
                          (index<1> idx) restrict(amp) {
        box_muller_kernel(idx, random_nums, normalized_random_nums, n_per_RNG,
                          cstF2);
    });
    copy(normalized_random_nums, v_random_nums.begin());
  }
  else
    copy(random_nums, v_random_nums.begin());
}

//----------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------
int main(int argc, char** argv) {
  accelerator default_device;
  std::wcout << L"Using device : " << default_device.get_description()
             << std::endl;

  std::vector<unsigned int> v_matrix(MT_RNG_COUNT), v_mask_b(MT_RNG_COUNT),
                            v_mask_c(MT_RNG_COUNT), v_seed(MT_RNG_COUNT);

  printf("Loading GPU twisters parameters...\n");

  mt_struct data_in_MT[MT_RNG_COUNT];

  // Load precomputed Mersenne twister config parameter
#if _WIN32
  load_MT(".\\MersenneTwister.dat", data_in_MT, MT_RNG_COUNT);
#else
  std::string str(argv[0]);
  int pos = str.rfind("/");
  std::string path(str, 0, pos+1);
  std::string input_file = path + "MersenneTwister.dat";
  load_MT(input_file.c_str(), data_in_MT, MT_RNG_COUNT);
#endif

  seed_MT(g_seed, data_in_MT, v_matrix, v_mask_b, v_mask_c, v_seed);
  
  printf("Generating random numbers on accelerator...\n");

  std::vector<float> random_nums_amp(g_rand_n);
  generate_rand_on_amp(v_matrix, v_mask_b, v_mask_c, v_seed, random_nums_amp);

  printf("Completed generating random numbers.\n  Dumping to file...\n");
#if _WIN32
  std::fstream log("data.txt", std::fstream::out);
#else
  std::string output_file = path + "data.txt";
  std::fstream log(output_file.c_str(), std::fstream::out);
#endif
  for (unsigned i = 0; i < random_nums_amp.size(); i++)
    log << random_nums_amp[i] << " ";
  log.close();
  std::cout<< "Finshed dumping to file.\n";
  return 0;
}
