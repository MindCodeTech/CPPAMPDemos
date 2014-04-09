// Primes.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Primes.h"
#include <windows.h>
#include <iostream>


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
		return 0;

	wchar_t* stopStr(0);
	unsigned long limit = wcstoul(argv[1],&stopStr,10);

	std::cout << "Calculating; Wait a moment" << std::endl;
	DWORD t0 = ::GetTickCount();
	
	unsigned long numPrimes(0);
	unsigned long candidate(3);
	for ( ; candidate <= limit; candidate++)
	{
		if (isPrime(candidate))
		{
			numPrimes++;
		}
	}
	DWORD dt = GetTickCount() - t0;
	std::cout << "Found " << numPrimes << " prime numbers in " << ((double)dt)/1000 << " seconds" << std::endl;
	return 0;
}

