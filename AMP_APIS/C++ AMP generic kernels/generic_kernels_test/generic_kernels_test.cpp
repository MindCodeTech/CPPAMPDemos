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
#include "generic_kernels.h"

#include <iostream>

#include "compat_types.h"

using namespace std;

template <class T, class op>
T cpu_reduce(const vector<T> &arr, int n)
{
	op f;
    T res = f.identity();
    for (size_t i = 0; i < arr.size(); i++)
	{
        f(res, arr[i]);
    }
    return res;
}

template <class T>
vector<T> rand_vector(unsigned int size)
{
    srand(time(NULL));
    
    vector<T> vec(size);
    for (size_t i = 0; i < size; i++)
	{
		vec[i] = generic_kernels::type_spec<T>::make_rand();
    }
    return vec;
}

template <class T>
void test_getvalue(unsigned int size)
{
    vector<T> ivec = rand_vector<T>(size);
	concurrency::array<T> d_idata(size, ivec);

	cout << "Getting values in a concurrency::array of " << size << " elements" << endl << endl;

	int N0 = 10000;
	cout << "CPU (" << N0 << " times)... ";
	T meth1;
	DWORD tickCount0 = ::GetTickCount();
	for (int i = N0-1; i >= 0; --i)
	{
		meth1 = ivec[i];
	}
	DWORD elapsed0 = ::GetTickCount() - tickCount0;
	cout << "Done. Value = " << meth1 << ", CPU time = " << (double)elapsed0 / N0 << " ms (" << sizeof(T)*1.0E-9*size*N0/(0.001*elapsed0) << " GB/s)" << endl;

	int N = 10000;
	cout << "GPU (" << N << " times)... ";
	T meth2;
	DWORD tickCount = ::GetTickCount();
	for (int i = N-1; i >= 0; --i)
	{
		meth2 = getvalue(d_idata, concurrency::index<1>(i));
	}
	DWORD elapsed = ::GetTickCount() - tickCount;
	cout << "Done. Value = " << meth2 << ", GPU time = " << (double)elapsed / N << " ms (" << sizeof(T)*1.0E-9*size*N/(0.001*elapsed) << " GB/s)" << endl << endl;

    bool passed = (meth1 == meth2);
	cout << "Result: " << (passed ? "PASSED" : "FAILED") << endl << endl;
}

template <class T>
void test_reduction(unsigned int size)
{
    vector<T> ivec = rand_vector<T>(size);

	cout << "Reducing " << size << " elements" << endl << endl;

	int N0 = 100;
	cout << "CPU (" << N0 << " times)... ";
	T expected;
	DWORD tickCount0 = ::GetTickCount();
	for (int i = 0; i < N0; ++i)
	{
		expected = cpu_reduce<T, generic_kernels::add_op<T>>(ivec, i);
	}
	DWORD elapsed0 = ::GetTickCount() - tickCount0;
	cout << "Done. Value = " << expected << ", CPU time = " << (double)elapsed0 / N0 << " ms (" << sizeof(T)*1.0E-9*size*N0/(0.001*elapsed0) << " GB/s)" << endl;

	int N = 100;
	cout << "GPU (" << N << " times)... ";
	T actual;
	concurrency::array<T> d_idata(size, ivec);
	generic_kernels::reduction<T, generic_kernels::add_op<T>> reduction;
	DWORD tickCount = ::GetTickCount();
	for (int i = 0; i < N; ++i)
	{
		actual = reduction.reduce(d_idata);
	}
	DWORD elapsed = ::GetTickCount() - tickCount;
	cout << "Done. Value = " << actual << ", GPU time = " << (double)elapsed / N << " ms (" << sizeof(T)*1.0E-9*size*N/(0.001*elapsed) << " GB/s)" << endl << endl;

    bool passed = (expected == actual);
	cout << "Result: " << (passed ? "PASSED" : "FAILED") << endl << endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	test_getvalue<uint>(16777216);
	test_reduction<uint>(16777216);

	cout << "Press any key to exit >";
	getchar();
	return 0;
}
