#include "stdafx.h"
#include "montecarlopi.h"
#include "IterativePi.h"
#include "StopWatch.h"

using namespace std;
using namespace concurrency;

mpf_class ComputePiWithAreas()
{
	MonteCarloPi mcPi(999999);
	auto experimentalPi = mcPi.ComputePiWithAreas();
	return experimentalPi;
}

mpf_class ComputePiWithNeedles()
{
	MonteCarloPi mcPi(9999999);
	auto experimentalPi = mcPi.ComputePiWithNeedles(5, 6);
	return experimentalPi;
}

mpf_class ComputePiWithGaussLegendre()
{
	IterativePi itPi;
	auto experimentalPi = itPi.ComputePiWithGaussLegendre();
	return experimentalPi;
}

int main(int argc, char* argv[])
{
	mpf_set_default_prec(1000000);

	cout << "Calculating the first 1,000,000 digits of pi" << std::endl << std::endl;

	StopWatch sw;
	sw.Start();
	task<mpf_class> glTask([]() -> mpf_class { return ComputePiWithGaussLegendre(); });
	task<mpf_class> needleTask([]() -> mpf_class { return ComputePiWithNeedles(); });
	task<mpf_class> areaTask([]() -> mpf_class { return ComputePiWithAreas(); });

	auto calculated = glTask.get();
	cout << setw(13) << left << "Calculated:" << setiosflags(ios::fixed) << setprecision(64) << calculated << std::endl;
	cout << "Duration: " << setprecision(2) << sw.GetDuration() << "(s) (displaying 64 of 1,000,000 digits)" << std::endl << std::endl;

	auto needleExperimental = needleTask.get();
	auto areaExperimental = areaTask.get();

	auto avgTask = create_task(
		[&needleExperimental, &areaExperimental]() -> mpf_class { return (needleExperimental + areaExperimental) / 2; })
		.then(
			[&calculated](mpf_class& average) -> mpf_class
			{
				cout << setw(13) << left << "Average: " << setprecision(64) << average << std::endl;
				return abs(calculated - average) / calculated;
			});

	avgTask.wait();
	cout << setprecision(2) << "Duration: " << sw.GetDuration() << "(s)" << std::endl << std::endl;

	cout << setw(13) << left << setprecision(10) << "Error:" << avgTask.get() << std::endl;
	char c;
	cin >> setw(1) >> c;
}
