// Primes.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Primes.h"
#include <windows.h>
#include <iostream>
#include <omp.h>
#define NUM_THREADS 4

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
		return 0;

	wchar_t* stopStr(0);
	const long limit = wcstol(argv[1],&stopStr,10);

	std::cout << "Calculating; Wait a moment" << std::endl;
	DWORD t0 = ::GetTickCount();

	unsigned long numPrimes(0);
	long candidate(2);
	long nThreads(0);

#pragma omp parallel default(none) private(candidate) shared(nThreads) num_threads(4) reduction(+ : numPrimes)
	{
#pragma omp master
		nThreads = omp_get_num_threads();
#pragma omp for schedule(static, 5000)
		for (candidate = 2 ;candidate <= limit; candidate++)
		{
			if (isPrime(candidate))
			{
				numPrimes++;
			}
		}
	}
	DWORD dt = GetTickCount() - t0;

	std::cout << "Found " << numPrimes << " prime numbers in " << ((double)dt)/1000 << " seconds" << std::endl;
	std::cout << nThreads << " threads used" << std::endl;

	return 0;
}

