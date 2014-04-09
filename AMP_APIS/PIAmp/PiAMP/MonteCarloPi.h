#pragma once

#include <mpirxx.h>

class MonteCarloPi
{
private:
	int trials_;
	int SimulateInCircle();
	int SimulateOverLine(const double length, const double distance);
public:
	MonteCarloPi(int trials);
	mpf_class ComputePiWithAreas();
	mpf_class ComputePiWithNeedles(const double length, const double distance);
};
