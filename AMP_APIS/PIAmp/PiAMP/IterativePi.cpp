#include "stdafx.h"
#include "IterativePi.h"
#include <mpirxx.h>

using std::cout;
using std::unique_ptr;

mpf_class IterativePi::ComputePiWithGaussLegendre()
{
	const mpf_class one(1.0);
	const mpf_class two(2.0);
	const mpf_class four(4.0);

	mpf_class ai(one);
	mpf_class bi(one/sqrt(two));
	mpf_class ti(one/four);
	mpf_class pi(one);

	mpf_class tmpA, tmpB;

	for (int i = 0; i < 25; i++)
	{
		tmpA = (ai + bi) / two;
		tmpB = sqrt(ai*bi);
		ti = ti - pi*(ai - tmpA)*(ai - tmpA);
		pi *= two;

		ai = tmpA;
		bi = tmpB;
	}

	mpf_class result((ai + bi)*(ai + bi) / (four*ti));

	return result;
}
