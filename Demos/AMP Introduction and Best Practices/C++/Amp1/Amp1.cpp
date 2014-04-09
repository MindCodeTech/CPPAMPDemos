// Amp1.cpp : Defines the entry point for the console application.
//
// http://msdn.microsoft.com/en-us/library/hh265136.aspx
//

#include "stdafx.h"
#include "AmpExamples.h"

using namespace Bisque;

AmpExamples g_sample;

int _tmain(int argc, _TCHAR* argv[])
{
	g_sample.AcceleratorProperties();
	g_sample.AcceleratorViewProperties();
	g_sample.StandardAdd();
	g_sample.AmpAdd();
	g_sample.CreateIndex1D();
	g_sample.CreateIndex2D();
	g_sample.CreateIndex3D();
	g_sample.CreateExtent();
	g_sample.CaptureByReference();
	g_sample.CaptureByValue();
	g_sample.CaptureArrayViewOrTextureView();
	g_sample.AmpArrayAdd();
	g_sample.AmpArrayAddWithFunction();
	g_sample.CPUMatrixMultiplication();
	g_sample.AmpMatrixMultiplication();
	g_sample.CalculateTileAverages();
	g_sample.CalculateTileAverages2();
	g_sample.CalculateLog();
	g_sample.ArrayViewOps();
	g_sample.ArrayViewOps2();
	g_sample.ArrayViewOps3();
	g_sample.ArrayOps();
	g_sample.ConcurrentArrayViewOp();
	g_sample.CastRestrictions();
	g_sample.ConstRestrictions();
	g_sample.PointerRestrictions();
	g_sample.ArrayAMPLimit();
	g_sample.ArrayAMPLimit2();
	g_sample.StagingArrays();
	g_sample.RelianceOnUnderlyingPointers();
	g_sample.ContinuationChaining();
	g_sample.Synchronization();
	g_sample.SynchronizingUnderlyingData();
	g_sample.SynchronizationBestPractices();
	g_sample.Subsections();
	g_sample.ViewAs();
	g_sample.ReinterpretAs();
	g_sample.Projection();
	g_sample.AtomicOperations();
	g_sample.TimoutException();
	g_sample.RecoverFromTDR();
	g_sample.DisableTDR();
	g_sample.DebugHelpers();
	g_sample.MeasurePerformance1();
	g_sample.MeasurePerformance2();

	std::cout << "\nPress enter key to exit.\n";
	std::cin.get();
	return 0;
}
