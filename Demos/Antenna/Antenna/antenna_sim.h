/*----------------------------------------------------------------------------
 * Copyright © Microsoft Corporation. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <mutex>
#include <atomic>

#include <amp.h>
#include <amp_math.h>
#include <amp_graphics.h>

#include "amp_addendum.h"

// ampblas requirements
#include "ampblas_complex.h"
#include "detail/gemm.h"
#include "detail/trsm.h"

// amplapack requirements
#include "detail/getrf.h"
#include "amplapack_addendum.h"

#include "basic_math.h"

using namespace concurrency;

// forward declares
template<typename T> class antenna_sim_cpu;
template<typename T> class antenna_sim_amp;
template<typename COMPUTE_TYPE> class antenna_sim;

class timer
{
public:
    timer()
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        frequency = double(f.QuadPart);
    }
    void init() 
    {
        QueryPerformanceCounter(&start_time);
    }
    double get_time()
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return (double(t.QuadPart - start_time.QuadPart) / frequency);
    }

private:
    LARGE_INTEGER start_time;
    double frequency;
};
//
// Base class for concrete solvers (C++ AMP and CPU)
//
template<typename T>
class compute_element
{
public:
    compute_element(antenna_sim<T>& p) : parent(p) {}

    virtual ~compute_element() {}

    virtual void fill_matrix() = 0;
    virtual void solve() = 0;
    virtual void prep_angles() = 0;

protected:
    antenna_sim<T>& parent;
};

template<typename COMPUTE_TYPE>
class antenna_sim
{
public:
    antenna_sim(int n, int samples, int history, std::mutex &m, std::unique_ptr<graphics::texture<float,2>>& tex, std::unique_ptr<graphics::texture<float,2>>& tex2, std::atomic<bool> &render_switch) :
        // these default lambda values can be changed
        lmin(0.125),
        lmax(20.0),     // controls how many layers of variation are present in the output
        ldelta(0.0625), // controls amount of variation row to row. best set to a power of 2.
        lambda(lmin),
        grow(true),
        runs(),
        n(n),
        samples(samples),
        history(history),
        thread_m(m),
        tex(tex),
        tex2(tex2),
        render_switch(render_switch),
        screen_line(samples,tex->accelerator_view),
        device(L"none"),
        amp_solver(*this),
        cpu_solver(*this)
    {
        toggle();
    }

    // thread entry point, this is a runnable class
    // loops antennas of various lengths until a quit()
    void operator() ()
    {
        looping = true;
        while(looping)
        {
            runs++;

            // "sample" current_solver so that toggles during this sequence allow us to keep using
            // the same solver until next pass
            compute_element<COMPUTE_TYPE>* c_save = current_solver;
            c_save->fill_matrix();
            c_save->solve();
            c_save->prep_angles();
            data_to_screen();

            lambda = grow ? lambda+ldelta : lambda-ldelta;

            if( lambda > lmax ) 
            { 
                grow = false;  
                lambda = lmax;
            }
            
            if( lambda < lmin )
            { 
                grow = true;
                lambda = lmin;
            }
        }
    }

    // quit the main solve loop, terminate the thread
    void quit() { looping=false; }

    // toggle between CPU and GPU solvers
    void toggle()
    {
        // do all internal state updates in one go
        std::lock_guard<std::mutex> lg(thread_m);

        runs = 0;
        timer.init();

        if( device == L"AMP" )
        {
            current_solver = &cpu_solver;
            device = L"CPU";
        }
        else
        {
            current_solver = &amp_solver;
            device = L"AMP";
        }
    }

    std::unique_ptr<graphics::texture<float,2>>& get_tex()
    {
        return tex;
    }

    std::wstring get_device()
    {
        return device;
    }

    // calculate a measure of performance
    float get_fps()
    {
        // lock needed because it uses state affected by toggling
        std::lock_guard<std::mutex> lg(thread_m);

        // wait a few cycles for the time to stabilize
        if( runs < 4 ) return 0.0;

        double time = timer.get_time();
        return float((runs-1)/time);
    }

    void update_tex(std::unique_ptr<graphics::texture<float,2>>& tex, std::unique_ptr<graphics::texture<float,2>>& tex2)
    {
        // local captures for amp's needs
        int h = history;
        const auto& line = screen_line;

        auto& rtex = *tex.get();
        auto& rtex2 = *tex2.get();

        parallel_for_each(
            tex->accelerator_view,
            extent<2>(history - 1, samples),
            [=,&rtex, &rtex2, &line](index<2> idx) restrict(amp)
        {
            index<2> old_idx(idx[0] + 1, idx[1]);
            auto val= rtex.get(old_idx);
            rtex2.set(idx, val);

            if (idx[0] == 0)
            {
                auto val = static_cast<float>(line[idx[1]]);
                rtex2.set(index<2>(h - 1, idx[1]), val);
            }
        });
    }

    // take the generated signature and add it to the texture, shifting all other lines by 1
    void data_to_screen()
    {
        std::lock_guard<std::mutex> lg(thread_m);

        // Use ping pong buffer for rendering.

        if (render_switch)
        {
            update_tex(tex, tex2);
        }
        else
        {
            update_tex(tex2, tex);
        }
        render_switch = !render_switch;
    }

    const COMPUTE_TYPE lmin;
    const COMPUTE_TYPE lmax;
    const COMPUTE_TYPE ldelta;
    COMPUTE_TYPE lambda;
    bool grow;

    std::mutex& thread_m;
    std::atomic<bool> &render_switch;
    std::atomic<bool> looping;

    std::wstring device;

    // display and computation sizes
    const int history; // corresponds to texture height (and initial window height)
    const int samples; // corresponds to texture width (and initial window width)
    const int n;       // corresponds to "complexity" config parameter

    array<COMPUTE_TYPE> screen_line;

private:
    // timer requirements
    timer timer;
    std::atomic<int> runs;

    std::unique_ptr<graphics::texture<float,2>>& tex;
    std::unique_ptr<graphics::texture<float,2>>& tex2;
    std::atomic<compute_element<COMPUTE_TYPE>*> current_solver;

    antenna_sim_amp<COMPUTE_TYPE> amp_solver;
    antenna_sim_cpu<COMPUTE_TYPE> cpu_solver;
};

template<typename T>
class antenna_sim_cpu : public compute_element<T>
{
public:
    antenna_sim_cpu(antenna_sim<T>& p) : compute_element(p), mat(parent.n*parent.n), vec(parent.n), ipiv_v(parent.n), screen_line(parent.samples,0) {}

private:
    typedef ampblas::complex<T> complex;
    std::vector<complex> mat;
    std::vector<complex> vec;
    std::vector<int> ipiv_v;
    std::vector<float> screen_line;

    virtual void solve()
    {
        int info;

        complex *a = &mat[0];
        complex *b = &vec[0];
        int *ipiv = &ipiv_v[0];

        // LU factorize with host lapack - very handy wrapper from amplapack
        amplapack::_detail::lapack::getrf(parent.n,parent.n,a,parent.n,ipiv,info);
        assert(info==0);

        // backsolve with host blas
        laswp(1,b,parent.n,1,parent.n,ipiv,1);
        trsm('L','L','N','U',parent.n,1,complex(1.0),a,parent.n,b,parent.n);
        trsm('L','U','N','N',parent.n,1,complex(1.0),a,parent.n,b,parent.n);
    }

    virtual void prep_angles()
    {
        const T pi = acos(T(0))*2;
        T l = parent.lambda;
        T dz = l*T(0.5)/(parent.n-1);
        float max_pow = 0;
        critical_section cs;

        parallel_for(0, parent.samples, [&](size_t x)
        {
            float local_max = 0;
            T theta = T(x)/(parent.samples-1) * pi;
            T sint = sin(theta);
            T cost = cos(theta);
            T arg = pi*dz*cost;
            T ft = (abs(arg)<T(0.001)) ? 1 : sin(arg)/arg;
            complex crt;
            for( int i=0; i<parent.n-1; ++i )
            {
                complex ek = vec[i]*ft;
                T argp = pi*(-l+i*2*dz+dz)*cost;
                crt += e_i(argp)*ek;
                argp = pi*(-l+(parent.n-1+i)*2*dz+dz)*cost;
                crt += e_i(argp)*ek;
            }
            screen_line[x] = float(abs(crt)*sint*sint);
            local_max = std::max(local_max,screen_line[x]);
            cs.lock();
            max_pow = std::max(max_pow,local_max);
            cs.unlock();
        });

        parallel_for(0, parent.samples, [&](size_t i)
        {
            screen_line[i] /= max_pow;
        });

        std::lock_guard<std::mutex> lg(parent.thread_m);
        copy(screen_line.begin(),parent.screen_line);
    }

    virtual void fill_matrix()
    {
        auto l = parent.lambda;

        const T pi = acos(T(0))*2;
        const T dz1 = l*T(0.5)/parent.n;
        const T eta = T(1)/(120*pi*2);
        const T one_4_pi = 1/(4*pi);

        std::vector<complex> ktab(2*parent.n,complex());

        // initialize "source" portion of matrix
        parallel_for(0, parent.n, [&](size_t i)
        {
            auto z=(2*(i+1)-1)*dz1*T(0.5);
            mat[i+(parent.n-1)*parent.n]=complex(cos(2*pi*z),0);
            vec[i]=complex(0,-sin(2*pi*z)*eta);
        });

        // build a lookup table of integrals
        T del = dz1/2;

        parallel_for(0, 2*parent.n, [&](size_t i) // loop whole length of antenna
        {
            T x = (2*(i+1)-1)*dz1*T(0.5) + del; // cell centroid
            ktab[i] += del*(one_4_pi/x)*e_i(-2*pi*x);
        });

        // fill the matrix using the integral tables
        parallel_for(0, parent.n-1, [&](size_t j)
        {
            for( int i=0; i<parent.n; ++i )
            {
                mat[i+j*parent.n]=ktab[abs(i-(int)j)]+ktab[abs(i+(int)j)];
            }
        });
    }

};

// C++ AMP implementation of the antenna solver
// There are frequent locks and unlocks of the mutex; this allows
// the other thread (display) to refresh the display between
// parallel work invocations
template<typename T>
class antenna_sim_amp : public compute_element<T>
{
public:
    antenna_sim_amp(antenna_sim<T>& p) :
        compute_element(p),
        av(p.get_tex()->accelerator_view),
        mat_storage(p.n,p.n,av),
        mat(mat_storage),
        vec_storage(1,p.n,av),
        vec(vec_storage),
        ipiv_storage(p.n,av),
        ipiv(ipiv_storage),
        ktab(2*p.n,av)
    {
    }

private:
    typedef ampblas::complex<T> complex;
    accelerator_view av;
    array<complex,2> mat_storage;
    array_view<complex,2> mat;
    array<complex,2> vec_storage;
    array_view<complex,2> vec;
    array<int,1> ipiv_storage;
    array_view<int,1> ipiv;
    array<complex,1> ktab;

    virtual void solve()
    {
        using amplapack::ordering;
        using ampblas::side;
        using ampblas::uplo;
        using ampblas::diag;
        using ampblas::transpose;

        std::lock_guard<std::mutex> lg(parent.thread_m);

        // LU factorize with "device" interface to amplapack routine
        amplapack::getrf<ordering::column_major>(av,mat,ipiv);

        // backsolve with host blas (also using wrapper from amplapack)
        // 3-step process of LASWP->TRSM->TRSM

        amplapack::_detail::laswp<ordering::column_major>(av,vec,1,parent.n,ipiv);

        array_view<const complex,2> mat_const(mat_storage);

        ampblas::trsm(av, side::left, uplo::lower, transpose::no_trans, diag::unit, complex(1.0), mat_const, vec);

        ampblas::trsm(av, side::left, uplo::upper, transpose::no_trans, diag::non_unit, complex(1.0), mat_const, vec);
    }

    virtual void prep_angles()
    {
        T l = parent.lambda;
        T samples = T(parent.samples);
        int n = parent.n;
        auto& vec_capture = vec;
        auto& out = parent.screen_line;

        const T pi = static_cast<T>(acos(0.0)*2);
        T dz = l*T(0.5)/(n-1);

        std::unique_lock<std::mutex> device_lock(parent.thread_m);
        parallel_for_each(
            av,
            extent<1>(parent.samples),
            [=,&out](index<1> idx) restrict(amp)
        {
            const int x = idx[0];
            T theta = T(x)/(samples-1) * pi;
            float sint, cost;
            fast_math::sincos(float(theta),&sint,&cost);
            float arg = float(pi*dz*cost);
            T ft = (fast_math::fabsf(arg)<T(0.001)) ? T(1.0) : fast_math::sin(arg)/arg;
            complex crt;
            for( int i=0; i<n-1; ++i )
            {
                complex ek = vec_capture(0, i)*ft;
                T argp = pi*(-l+i*2*dz+dz)*cost;
                crt += e_i(argp)*ek;
                argp = pi*(-l+(n-1+i)*2*dz+dz)*cost;
                crt += e_i(argp)*ek;
            }
            out[idx] = abs(crt)*sint*sint;
        }
        );
        device_lock.unlock();

        // normalize the vector to 1
        static const int tile_size_normalize = 32;
        auto e = extent<1>(parent.samples).tile<tile_size_normalize>().pad();
        device_lock.lock();
        parallel_for_each(
            av,
            e,
            [=,&out](tiled_index<tile_size_normalize> idx) restrict(amp)
        {
            tile_static T cache[tile_size_normalize];
            int local_i = idx.local[0];

            T max_el = 0.0;
            for( int k=0; k<samples; k+=tile_size_normalize )
            {
                if( k+local_i < samples ) cache[local_i] = out[index<1>(k+local_i)];
                idx.barrier.wait_with_tile_static_memory_fence();
                
                // This is a typical parallel reduction application. When the compute domain size is large, 
                // we can use parallel reduction at: http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx

                for( int i=0; i<tile_size_normalize; ++i ) 
                    if( cache[i] > max_el )
                        max_el = cache[i];

                idx.barrier.wait_with_tile_static_memory_fence();
            }
            out[idx] = out[idx]/max_el;
        }
        );
    }
    virtual void fill_matrix()
    {
        auto l = parent.lambda;

        const T pi = acos(T(0))*2;
        const T dz1 = l*T(0.5)/parent.n;
        const T eta = T(1)/(120*pi*2);
        const T one_4_pi = 1/(4*pi);

        // local captures
        int n = parent.n;
        auto& mat_capture = mat;
        auto& vec_capture = vec;
        auto& ktab_capture = ktab;

        std::unique_lock<std::mutex> device_lock(parent.thread_m);

        parallel_for_each(
            av,
            extent<1>(parent.n),
            [=](index<1> idx) restrict(cpu,amp)
        {
            auto i = idx[0];
            auto z=(2*(i+1)-1)*dz1*T(0.5);
            mat_capture(n-1,i)=complex(fast_math::cosf(float(2*pi*z)),0);
            T imag = -fast_math::sinf(float(2*pi*z))*eta;
            vec_capture(0,i)=complex(0,imag);
        });

        T del = dz1/2;

        parallel_for_each(
            av,
            ktab.extent,
            [=,&ktab_capture](index<1> idx) restrict(cpu,amp)
        {
            auto i = idx[0];
            complex out;
            T x = (2*(i+1)-1)*dz1*T(0.5) + del; // cell centroid
            out += del*(one_4_pi/x)*e_i(-2*pi*x);
            ktab_capture[idx] = out;
        });

        parallel_for_each(
            av,
            mat.extent,
            [=,&ktab_capture](index<2> idx) restrict(cpu,amp)
        {
            auto i=idx[1];
            auto j=idx[0];
            if( j < n-1 )
                mat_capture[idx] = ktab_capture[index<1>(abs(i-j))]+ktab_capture[index<1>(abs(i+j))];
        });
    }
};

