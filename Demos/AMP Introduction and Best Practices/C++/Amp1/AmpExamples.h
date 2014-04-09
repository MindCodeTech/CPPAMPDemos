#pragma once

namespace Bisque
{
	using concurrency::accelerator_view;
	using concurrency::array;
	using concurrency::array_view;
	using concurrency::index;
	using concurrency::queuing_mode;
	using concurrency::queuing_mode_automatic;
	using std::vector;

	class AmpExamples
	{
	public:
		AmpExamples(void);
		~AmpExamples(void);

		void AcceleratorProperties();
		void AcceleratorViewProperties();
		void StandardAdd();
		void AmpAdd();
		void CreateIndex1D();
		void CreateIndex2D();
		void CreateIndex3D();
		void CreateExtent();
		void CaptureByReference();
		void CaptureByValue();
		void CaptureArrayViewOrTextureView();
		void AmpArrayAdd();
		void AmpArrayAddWithFunction();
		void CPUMatrixMultiplication();
		void AmpMatrixMultiplication();
		void CalculateTileAverages();
		void CalculateTileAverages2();
		void CalculateLog();
		void ArrayViewOps();
		void ArrayViewOps2();
		void ArrayViewOps3();
		void ArrayOps();
		void ConcurrentArrayViewOp();
		void CastRestrictions();
		void ConstRestrictions();
		void PointerRestrictions();
		void LambdaExample();
		void ArrayAMPLimit();
		void ArrayAMPLimit2();
		void StagingArrays();
		void RelianceOnUnderlyingPointers();
		void ContinuationChaining();
		void Synchronization();
		void SynchronizingUnderlyingData();
		void SynchronizationBestPractices();
		void Subsections();
		void ViewAs();
		void ReinterpretAs();
		void Projection();
		void AtomicOperations();
		void TimoutException();
		void RecoverFromTDR();
		void DisableTDR();
		void DebugHelpers();
		void MeasurePerformance1();
		void MeasurePerformance2();

	private:
		static void AddElementsInternal(index<1> idx, array_view<int, 1> sum, const array_view<const int, 1> a, const array_view<const int, 1> b) restrict(amp);

		void LongRunningKernel	(const vector<int>& data, vector<int>& result, queuing_mode queuingMode = queuing_mode_automatic);		// For TimeoutException (TDR) example
		void MultiplyMatrices	(accelerator_view& view, array<float, 2>& result, const array<float, 2>& a, const array<float, 2>& b);	// For measuring performance
		void MyFabs				(array_view<float, 1> & view);																			// For reinterpret_as example
		bool PickAccelerator	();																										// Selects accelerator for the process
		void RandomFill			(array_view<float, 1> & view);																			// For view_as example
		void RandomFill			(vector<float>& data, float min = -1 << 15, float max = 1 << 15);										// For measuring performance
		void WarmUp				(accelerator_view& view);																				// For measuring performance 2
	};

	//
	// Note that capturing the “this” pointer is not allowed since it will result 
	// in a pointer data member in the generated lambda class. This causes some inconvenience 
	// when using lambda expressions inside an amp restricted member function because 
	// we cannot refer to the enclosing object’s data members directly in the lambda expression. 
	// The workaround is to declare a local reference variable to the data member, 
	// and capture that local variable in the lambda expression instead. For example,
	// 
	class Scale
	{
	public:
		int Apply(array<int,1> & a, unsigned int start, unsigned int end) restrict(amp)
		{
			int & scale = m_scale;

			auto f = [scale](int n) { return n * scale; };

			for (unsigned int i = start; i < end; ++i)
			{
				a[i] = f(a[i]);
			}
		}

	private:
		int m_scale;
	};

	//
	// Demonstrates encapsulation of array_view
	//
	// Usage:
	//
	// Matrix<float> m(width, hight, av);
	//
	// The Matrix object can be captured in the parallel_for_each by value as it only contains
	// an array_view data member. 
	// parallel_for_each(m.extent, [=](index<2> idx) restrict(amp) {
	//		m(idx) = fast_math::sqrt(m(idx));
	// });
	//
	template<typename T>
	class Matrix
	{
	public:
		// The Matrix type's array_view data member can be constructed from a temporary array
		// without making the array a data member of the type. Memory underlying the array
		// will be freed when the array_view will be destructed.
		Matrix(int width, int height, accelerator_view av)
			: m_data(array<T, 2>(width, height, av))
		{
		}

		array_view<T, 2> GetRow(int rowNumber) restrict(cpu, amp)
		{
			return m_data[rowNumber];
		}
		
		T& operator()(int x, int y) restrict(cpu, amp)
		{
			return m_data(x, y);
		}
		
	private:
		array_view<T, 2> m_data;
	};
}
