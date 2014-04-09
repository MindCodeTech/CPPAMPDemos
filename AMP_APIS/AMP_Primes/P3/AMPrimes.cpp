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

	const unsigned int numThreads = 100000;
    int numbers[numThreads];
	unsigned int sum(0);

	concurrency::array_view<int, 1> nums(numThreads, numbers);

	for (unsigned int startVal = 0; startVal < upperLimit; startVal += numThreads)
	{
		concurrency::parallel_for_each( concurrency::extent<1>(numThreads) , [numThreads, nums, startVal] ( concurrency::index <1> idx) restrict(amp) 
		{
			unsigned int candidate = idx[0] + startVal;
			if (isPrime(candidate)) nums[idx] = 1;
			else nums[idx] = 0;
		});
		nums.synchronize();
		sum += std::accumulate(numbers,numbers + numThreads,0);
	}
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

	try
	{
		numPrimes = primesCount(limit);
	}
    catch(concurrency::accelerator_view_removed& ex)
    {
		std::cout << "TDR exception received: " << ex.what() << '\n';
		std::cout << "Error code:" << std::hex << ex.get_error_code() << '\n';
		std::cout << "Removed reason:" << std::hex << ex.get_view_removed_reason() << '\n';
        return -1;
    } 

    DWORD dt = GetTickCount() - t0;
    std::cout << "Found " << numPrimes << " prime numbers in " << ((double)dt)/1000 << " seconds" << std::endl;
    return 0;
}