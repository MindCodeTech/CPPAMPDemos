#include "stdafx.h"
#include <amp.h>
#include <amp_math.h>
#include <tchar.h>
#include <iostream>
#include <numeric>


bool isPrime (unsigned int candidate) restrict(amp)
{
	if (candidate < 2) return false;

	unsigned int maxDiv = candidate / 2;
    unsigned int i = 2;
    for ( ; i <= maxDiv; ++i)
    {
        if (0 == (candidate % i)) // divisor found - not prime
        {
            break;
        }
    }
    return (i > maxDiv) ? true : false;
}


int primesCount(unsigned int upperLimit) {

	const unsigned int numThreads = upperLimit;
    std::vector<int> numbers(numThreads);
	unsigned int sum(0);

	concurrency::array_view<int, 1> nums(numThreads, numbers);

	concurrency::parallel_for_each( concurrency::extent<1>(numThreads) , [numThreads, nums] ( concurrency::index <1> idx) restrict(amp) 
	{
		unsigned int candidate = idx[0];
		if (isPrime(candidate)) nums[idx] = 1;
		else nums[idx] = 0;
	});
	nums.synchronize();

	sum = std::accumulate(numbers.begin(),numbers.end(),0);
	return sum;
}


int _tmain(int argc, _TCHAR* argv[])
{
    if (argc < 2)
        return 0;

	wchar_t* stopStr = {0};
    const unsigned long limit = wcstol(argv[1],&stopStr,10);

    std::cout << "Calculating; Wait a moment" << std::endl;
    DWORD t0 = ::GetTickCount();

	int numPrimes = {0};
	numPrimes = primesCount(limit);

    DWORD dt = GetTickCount() - t0;
    std::cout << "Found " << numPrimes << " prime numbers in " << ((double)dt)/1000 << " seconds" << std::endl;
    return 0;
}