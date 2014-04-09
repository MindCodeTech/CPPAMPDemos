#pragma once

inline bool isPrime (unsigned long candidate)
{
	if (candidate < 2) return false;

	unsigned long maxDiv = candidate / 2;
	unsigned long i = 2;
	for ( ; i <= maxDiv; ++i)
	{
		if (0 == (candidate % i)) // divisor found - not prime
		{
			break;
		}
	}
	return (i > maxDiv) ? true : false;
}

