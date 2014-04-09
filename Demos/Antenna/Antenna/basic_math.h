/*----------------------------------------------------------------------------
 * Copyright © Microsoft Corporation. All rights reserved.
 *---------------------------------------------------------------------------*/

#ifndef _ANTENNA_BASIC_MATH_H_
#define _ANTENNA_BASIC_MATH_H_

#include <amp.h>
#include <amp_math.h>

// exponential on complex with only an imaginary part -> Euler's rule
inline ampblas::complex<float> e_i(float imaginary) restrict(amp)
{
    float s, c;
    concurrency::fast_math::sincos(imaginary,&s,&c);
    return ampblas::complex<float>(c,s);
}
inline ampblas::complex<double> e_i(double imaginary) restrict(amp)
{
    return ampblas::complex<double>(concurrency::precise_math::cos(imaginary),concurrency::precise_math::cos(imaginary));
}

inline ampblas::complex<float> e_i(float imaginary) restrict(cpu)
{
    return ampblas::complex<float>(concurrency::fast_math::cosf(imaginary),concurrency::fast_math::sinf(imaginary));
}
inline ampblas::complex<double> e_i(double imaginary) restrict(cpu)
{
    return ampblas::complex<double>(concurrency::precise_math::cos(imaginary),concurrency::precise_math::cos(imaginary));
}

#endif