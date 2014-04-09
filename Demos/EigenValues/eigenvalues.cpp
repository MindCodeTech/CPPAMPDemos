//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corp. 
//
// File: EigenValues.cpp
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this 
// file except in compliance with the License. You may obtain a copy of the License at 
// http://www.apache.org/licenses/LICENSE-2.0  
//  
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
// EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR 
// CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT. 
//  
// See the Apache Version 2.0 License for specific language governing permissions and 
// limitations under the License.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Implements EigenValues sample in C++ AMP
// This sample is C++ AMP version of NVIDIA CUDA's Eigen Values Sample and it is developed
// using the algorithm provided by NVIDIA Corporation.
//
// http://docs.nvidia.com/cuda/cuda-samples/index.html#eigenvalues
// Refer README.txt
//--------------------------------------------------------------------------------------

#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <list>
#include <stack>
#include <math.h>
#include <amp.h>

using namespace concurrency;

#define EIGEN_TILE_SIZE 512
#define MATRIX_SIZE (512)

class EigenValues
{	
public:
	EigenValues(const unsigned int matrixSize, const float precision);
	~EigenValues();
	void Execute();
	bool Verify();

private:
	bool VerifyResults(const std::vector<float> &results1, const std::vector<float> &results2, const float precision);	
	void ComputeGerschgorin(const float *diagonal, const float *superdiagonal, const size_t matrixSize, float &lowerBound, float &upperBound);
	unsigned int ComputeEigenValuesCount(const std::vector<float> &diagonal, const std::vector<float> &superdiagonal, const size_t matrixSize, float midpoint);
	void Bisection(std::list<float> &results, const std::vector<float> &diagonal, const std::vector<float> &superdiagonal, const float precision, float lowerBound, float upperBound);
	void ComputeEigenvaluesCPU(std::vector<float> &eigenvalues, const std::vector<float> &diagonal, const std::vector<float> &superdiagonal, const float precision);

	static unsigned int ComputeEigenValuesCount(const array_view<const float, 1> &fDiagonal, const array_view<const float, 1> &fSuperdiagonal, const unsigned int matrixSize, float midpoint) restrict(amp);

	std::vector<float> m_diagonal;
	std::vector<float> m_superdiagonal;
	std::vector<float> m_eigenvaluesGPU;
	const unsigned int m_matrixDim;
	const float m_precision;
};

struct RangeType
{
	RangeType(float l, float r, unsigned int lC, unsigned int rC) :left(l), right(r), leftCount(lC), rightCount(rC) { }
	float left;
	float right;
	unsigned int leftCount;
	unsigned int rightCount;
};

EigenValues::EigenValues(const unsigned int matrixSize, const float precision) :m_matrixDim(matrixSize), m_precision(precision)
{
	m_diagonal.resize(matrixSize);
	m_superdiagonal.resize(matrixSize);
	m_eigenvaluesGPU.resize(matrixSize);

	srand(2009);
	for (size_t i = 0; i != matrixSize; ++i)
	{
		m_diagonal[i] = rand() / static_cast<float>(RAND_MAX);
		m_superdiagonal[i] = rand() / static_cast<float>(RAND_MAX);
	}

	// Superdiagonal is smaller by 1 than diagonal, but we want it to be aligned to the right.
	m_superdiagonal[0] = 0.0f;
}

EigenValues::~EigenValues()
{

}

void EigenValues::Execute()
{	
	extent<1> matrixDim(m_matrixDim);
	const array_view<const float, 1> fDiagonal(m_matrixDim, m_diagonal);
	const array_view<const float, 1> fSuperdiagonal(m_matrixDim, m_superdiagonal);
	array_view<float, 1> fEigenvalues(m_matrixDim, m_eigenvaluesGPU);

	float lowerBound = 0.0f, upperBound = 0.0f;

	ComputeGerschgorin(m_diagonal.data(), m_superdiagonal.data() + 1, m_matrixDim, lowerBound, upperBound);
	std::cout << "Gerschgorin boundaries: " << lowerBound << " - " << upperBound << std::endl;
	unsigned int matrixSize = m_matrixDim;
	float precision = m_precision;

	fEigenvalues.discard_data();
	parallel_for_each(matrixDim.tile<EIGEN_TILE_SIZE>(), [fDiagonal, fSuperdiagonal, matrixSize, precision, lowerBound, upperBound, fEigenvalues](tiled_index<EIGEN_TILE_SIZE> idx) restrict(amp)
	{
		int threadId = idx.local[0];

		// Interval arrays
		tile_static float leftInterval[EIGEN_TILE_SIZE];
		tile_static float rightInterval[EIGEN_TILE_SIZE];

		if (0 == threadId)
		{
			leftInterval[0] = lowerBound;
			rightInterval[0] = upperBound;
		}
		else
		{
			leftInterval[threadId] = 0.0f;
			rightInterval[threadId] = 0.0f;
		}

		// Eigenvalue count arrays
		tile_static unsigned int leftCount[EIGEN_TILE_SIZE];
		tile_static unsigned int rightCount[EIGEN_TILE_SIZE];

		if (0 == threadId)
		{
			leftCount[0] = 0;
			rightCount[0] = EIGEN_TILE_SIZE;
		}
		else
		{
			leftCount[threadId] = 0;
			rightCount[threadId] = 0;
		}

		// Eigenvalues in shared memory
		tile_static float results[EIGEN_TILE_SIZE];
		results[threadId] = 0.0f;

		tile_static bool bAllDone;
		bAllDone = true;

		// Compute until ran out of unconverged intervals 
		while (true)
		{
			// Each thread checks if he has something to compute
			bAllDone = true;

			// Synch before touching bAllDone
			idx.barrier.wait();

			bool bCompute = false;
			if (leftInterval[threadId] != rightInterval[threadId])
			{
				bCompute = true;
				bAllDone = false;
			}

			// Synch before touching leftInterval and rightInterval arrays
			idx.barrier.wait();

			if (bCompute)
			{
				float left = leftInterval[threadId];
				unsigned int eigenvaluesOnLeft = leftCount[threadId];

				float right = rightInterval[threadId];
				unsigned int eigenvaluesOnRight = rightCount[threadId];

				// Compute midpoint
				float midpoint = (left + right) / 2.0f;

				// Compute number of eigenvalues < midpoint
				unsigned int eigenvalueCount = ComputeEigenValuesCount(fDiagonal, fSuperdiagonal, matrixSize, midpoint);

				// Investigate left interval
				if (midpoint - left <= precision)
				{
					for (unsigned int i = 0; i < eigenvalueCount - eigenvaluesOnLeft; ++i)
					{
						results[eigenvaluesOnLeft + i] = midpoint;
					}
					// Mark interval as done
					leftInterval[threadId] = 0.0f;
					rightInterval[threadId] = 0.0f;
				}
				else if (eigenvalueCount - eigenvaluesOnLeft > 0)
				{
					// Update interval that needs further computation
					rightInterval[threadId] = midpoint;
					rightCount[threadId] = eigenvalueCount;

					// leftInterval and leftCount doesn't need updates
				}
				else
				{
					// This interval is empty. Mark interval as done
					leftInterval[threadId] = 0.0f;
					rightInterval[threadId] = 0.0f;
				}

				// Investigate right interval
				if (right - midpoint <= precision)
				{
					for (unsigned int i = 0; i < eigenvaluesOnRight - eigenvalueCount; ++i)
					{
						results[eigenvalueCount + i] = midpoint;
					}
					// No need to mark right interval as done
				}
				else if (eigenvaluesOnRight - eigenvalueCount > 0)
				{
					// Update interval that needs further computation
					rightInterval[eigenvalueCount] = right;
					rightCount[eigenvalueCount] = eigenvaluesOnRight;

					leftInterval[eigenvalueCount] = midpoint;
					leftCount[eigenvalueCount] = eigenvalueCount;
				}
			}

			if (bAllDone)
			{
				break;
			}

			// Synch before touching bAllDone
			idx.barrier.wait();
		}

		//fEigenvalues[index<1>(threadId)] = results[threadId];
		fEigenvalues[idx] = results[threadId];
	});
}

unsigned int EigenValues::ComputeEigenValuesCount(const array_view<const float, 1> &fDiagonal, const array_view<const float, 1> &fSuperdiagonal, const unsigned int matrixSize, float midpoint) restrict(amp)
{
	unsigned int count = 0;
	float delta = 1.0f;

	for (unsigned int i = 0; i != matrixSize; ++i)
	{
		delta = fDiagonal[index<1>(i)] - midpoint - fSuperdiagonal[index<1>(i)] * fSuperdiagonal[index<1>(i)] / delta;
		if (delta < 0)
		{
			++count;
		}
	}
	return count;
}

bool EigenValues::Verify()
{
	std::vector<float> m_eigenvaluesCPU_result(m_matrixDim, 0.0f);
	ComputeEigenvaluesCPU(m_eigenvaluesCPU_result, m_diagonal, m_superdiagonal, m_precision);
	return VerifyResults(m_eigenvaluesGPU, m_eigenvaluesCPU_result, m_precision);
}

bool EigenValues::VerifyResults(const std::vector<float> &results1, const std::vector<float> &results2, const float precision)
{
	if (results1.size() != results2.size())
	{
		std::cout << "The sizes of Result vectors are not equal : " << results1.size() << " , " << results2.size() << std::endl;
	}

	for (size_t i = 0; i != results1.size(); ++i)
	{
		float r1 = results1[i];
		float r2 = results2[i];

		r1 = (r1 >= 0) ? r1 : -r1;
		r2 = (r2 >= 0) ? r2 : -r2;

		float diff = r1 - r2;
		diff = (diff >= 0) ? diff : -diff;

		if (diff > precision)
		{
			std::cout << "results1[" << i << "] = " << results1[i] << ", result2[" << i << "] = " << results2[i] << std::endl;
			return false;
		}
	}
	return true;
}

void EigenValues::ComputeEigenvaluesCPU(std::vector<float> &eigenvalues, const std::vector<float> &diagonal, const std::vector<float> &superdiagonal, const float precision)
{
	std::cout << "Computing Eigenvalues on CPU" << std::endl;
	float lowerBound = 0.0f, upperBound = 0.0f;

	ComputeGerschgorin(static_cast<const float*>(diagonal.data()), static_cast<const float*>(superdiagonal.data() + 1), diagonal.size(), lowerBound, upperBound);
	std::cout << "Gerschgorin boundaries:" << lowerBound << " - " << upperBound << std::endl;

	std::list<float> results;
	Bisection(results, diagonal, superdiagonal, precision, lowerBound, upperBound);

	int i = 0;
	std::list<float>::const_iterator iter = results.begin();
	while (iter != results.end())
	{
		eigenvalues[i] = *iter;
		++iter;
		++i;
	}
}

void EigenValues::ComputeGerschgorin(const float *diagonal, const float *superdiagonal, const size_t matrixSize, float &lowerBound, float &upperBound)
{
	float lower, upper;

	// Special boundary case for first element
	lowerBound = diagonal[0] - fabs(superdiagonal[0]);
	upperBound = diagonal[0] + fabs(superdiagonal[0]);

	// General case for 1 to matrixSize - 1
	for (size_t i = 1; i != matrixSize - 1; ++i)
	{
		lower = diagonal[i] - fabs(superdiagonal[i]) - fabs(superdiagonal[i - 1]);
		upper = diagonal[i] + fabs(superdiagonal[i]) + fabs(superdiagonal[i - 1]);

		lowerBound = min(lower, lowerBound);
		upperBound = max(upper, upperBound);
	}

	// Special boundary case for last element
	lowerBound = min(diagonal[matrixSize - 1] - fabs(superdiagonal[matrixSize - 2]), lowerBound);
	upperBound = max(diagonal[matrixSize - 1] + fabs(superdiagonal[matrixSize - 2]), upperBound);
}

void EigenValues::Bisection(std::list<float> &results, const std::vector<float> &diagonal, const std::vector<float> &superdiagonal,
	const float precision, float lowerBound, float upperBound)
{
	std::stack<RangeType> ranges;
	ranges.push(RangeType(lowerBound, upperBound, 0, static_cast<unsigned int>(diagonal.size())));

	while (!ranges.empty())
	{
		RangeType range = ranges.top();
		ranges.pop();

		// Compute midpoint
		float midpoint = (range.left + range.right) / 2.0f;

		// Compute number of eigenvalues < midpoint
		unsigned int eigenvalueCount = ComputeEigenValuesCount(diagonal, superdiagonal, diagonal.size(), midpoint);

		// Investigate left interval
		if (midpoint - range.left <= precision)
		{
			for (size_t i = 0; i < eigenvalueCount - range.leftCount; ++i)
			{
				results.push_front(midpoint);
			}
		}
		else if (eigenvalueCount - range.leftCount > 0)
		{
			// Push intervals that need further computation
			ranges.push(RangeType(range.left, midpoint, range.leftCount, eigenvalueCount));
		}

		// Investigate right interval
		if (range.right - midpoint <= precision)
		{
			for (size_t i = 0; i < range.rightCount - eigenvalueCount; ++i)
			{
				results.push_front(midpoint);
			}
		}
		else if (range.rightCount - eigenvalueCount > 0)
		{
			// Push intervals that need further computation
			ranges.push(RangeType(midpoint, range.right, eigenvalueCount, range.rightCount));
		}
	}
}

unsigned int EigenValues::ComputeEigenValuesCount(const std::vector<float> &diagonal, const std::vector<float> &superdiagonal, const size_t matrixSize, float midpoint)
{
	unsigned int count = 0;
	float delta = 1.0f;

	for (size_t i = 0; i != matrixSize; ++i)
	{
		delta = diagonal[i] - midpoint - superdiagonal[i] * superdiagonal[i] / delta;
		if (delta < 0)
		{
			++count;
		}
	}
	return count;
}

int main()
{
    const size_t matrixSize = MATRIX_SIZE;
    const float precision = 0.0001f;

	EigenValues eig( matrixSize, precision);
	eig.Execute();
	std::cout << (eig.Verify()? "Pass" : "Fail") << std::endl;
}
