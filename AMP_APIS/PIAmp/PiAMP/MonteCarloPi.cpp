#include "stdafx.h"
#include "MonteCarloPi.h"

using std::random_device;
using std::uniform_real_distribution;

MonteCarloPi::MonteCarloPi(int trials)
	: trials_(trials)
{ }

int MonteCarloPi::SimulateInCircle(void)
{
	int inTotal = 0;
	uniform_real_distribution<double> uniform;
	random_device rnd;
	for (int i = 0; i < trials_; i++)
	{
		double x = uniform(rnd);
		double y = uniform(rnd);
		auto distance = std::sqrt(x*x + y*y);
		if (distance <= 1) inTotal++;
	}

	return inTotal;
}

mpf_class MonteCarloPi::ComputePiWithAreas(void)
{
	auto inTotal = SimulateInCircle();
	mpf_class pi = (mpf_class(inTotal) / mpf_class(trials_)) * 4;
	return pi;
}

int MonteCarloPi::SimulateOverLine(const double length, const double distance)
{
	double halfDistance = distance / 2;
	double halfLength = length / 2;

	uniform_real_distribution<double> uniformThrow(0, halfDistance);
	uniform_real_distribution<double> uniformAngle(0, std::_Pi/2);
	random_device rnd;

	int overTotal = 0;
	for (int i = 0; i < trials_; i++)
	{
		double throwPoint = uniformThrow(rnd);
		double distanceFromLine = halfLength - throwPoint;
		if (distanceFromLine < 0) continue;

		double angle = uniformAngle(rnd);
		double effectiveLength = halfLength * sin(angle);
		if (effectiveLength >= distanceFromLine) overTotal++;
	}

	return overTotal;
}

mpf_class MonteCarloPi::ComputePiWithNeedles(const double length, const double distance)
{
	auto inTotal = SimulateOverLine(length, distance);
	mpf_class pi = mpf_class(2 * length * trials_) / mpf_class(distance * inTotal);
	return pi;
}
