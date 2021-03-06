// Copyright (c) Microsoft Corp. 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0  
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT. 
//
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

#include <amp.h>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace concurrency;

// Iteration 0
// Original version
struct record_v_1
{
	float cost_1;
	float cost_2;
	float total_cost;
	char* label;
};

// Iteration 1
// No pointer version
struct record_v_2
{
	float cost_1;
	float cost_2;
	float total_cost;
#if defined(_M_IX86)
	static_assert(sizeof(int) == sizeof(char*), "Cannot represent char* as int!");
	int label;
#elif defined(_M_AMD64)
	static_assert(sizeof(__int64) == sizeof(char*), "Cannot represent char* as __int64!");
	__int64 label;
#else
#error "Unknown architecture!"
#endif
};

// Iteration 2
// C++ AMP compatible version
struct record_v_3
{
	float cost_1;
	float cost_2;
	float total_cost;
	int label[(sizeof(char*) + sizeof(int) - 1) / sizeof(int)]; // warning: char* in a disguise
};

// Final solution
// Generic solution
template <typename Type>
class pointer_holder
{
public:
	typedef Type element_type;
	typedef Type* pointer;

	pointer_holder() restrict(cpu,amp)
	{
	}

	pointer_holder(pointer ptr) restrict(cpu)
	{
		reset(ptr);
	}

	pointer_holder& operator=(pointer ptr) restrict(cpu)
	{
		reset(ptr);
		return *this;
	}

	void reset(pointer ptr) restrict(cpu)
	{
		*reinterpret_cast<pointer UNALIGNED *>(data) = ptr;
	}

	operator pointer() const restrict(cpu)
	{
		return *reinterpret_cast<const pointer UNALIGNED *>(data);
	}

	element_type& operator*() const restrict(cpu)
	{
		return *static_cast<pointer>(*this);
	}

	pointer operator->() const restrict(cpu)
	{
		return static_cast<pointer>(*this);
	}

private:
	int data[(sizeof(pointer) + sizeof(int) - 1) / sizeof(int)];
};

struct record
{
	float cost_1;
	float cost_2;
	float total_cost;
	pointer_holder<char> label;
};

int main()
{
	std::vector<record> records;

	// ... fill the records vector from an external source ...
	// (But for the sake of the sample, let's just do it in place!)
	{
		record r;
		r.cost_1 = 2.0f;
		r.cost_2 = 3.6f;
		r.label = "Trey Research";
		records.push_back(r);
		r.cost_1 = 3.0f;
		r.cost_2 = 1.4f;
		r.label = "A. Datum Corporation";
		records.push_back(r);
	}

	float factor_1 = 100.0f;
	float factor_2 = 10.0f;

	array_view<record, 1> records_view(static_cast<int>(records.size()), records);
	parallel_for_each(records_view.extent,
		[=](index<1> idx) restrict(amp)
		{
			record& r = records_view[idx];
			r.total_cost =
				  factor_1 * r.cost_1
				+ factor_2 * r.cost_2;
		}
	);
	records_view.synchronize();

	for(const auto& r : records)
	{
		std::cout << std::setw(20) << r.label << "\t" << r.total_cost << std::endl;
	}
}