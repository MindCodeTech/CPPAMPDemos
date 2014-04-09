#include "stdafx.h"
#include "AmpExamples.h"

using namespace Bisque;
using namespace std;
using namespace concurrency;
using namespace concurrency::direct3d;
using namespace concurrency::graphics;
using namespace chrono;

AmpExamples::AmpExamples(void)
{
}


AmpExamples::~AmpExamples(void)
{
}

// Given an accelerator object, you can access its description, version, device path, size of dedicated memory in KB, 
// whether it is some kind of emulator, whether it has a display attached, whether it supports double precision, 
// and whether it was created with the debugging layer enabled for extensive error reporting.
//
// Below is example code that accesses some of the properties; in your real code you'd probably be checking one or more of them 
// in order to pick an accelerator (or check that the default one is good enough for your specific workload):
//
// You can also create accelerator directly by passing to the constructor a system-wide unique path to a device 
// if you know it (i.e. the “Device Instance Path” property for the device in Device Manager), 
// e.g. accelerator a(L"PCI\VEN_10DE&DEV_06DA&SUBSYS_1520103C&REV_A3\4&ADCCE93&0&0018");
//
// Here's the algorithm for picking a default accelerator: 
//		1. If you are performing GPU debugging then we pick the accelerator you choose in the project properties:
//		   a) by default the emulator accelerator for debugging (device_path="direct3d/ref" and description=“Software Adapter”); 
//		   b) otherwise the environment variable CPPAMP_DEFAULT_ACCELERATOR is examined for a device path. 
//		2. Failing any of those conditions being true, the default is set to a non-emulated device if one exists. 
//		   a). If there is more than one, the device with the largest dedicated_memory is chosen. 
//		   b). If there is more than one with equal memory, then the one where has_display is false becomes the default one.
//
// You can change the runtime’s default accelerator programmatically only once per process activation, 
// i.e. after you set a default yourself, it cannot be changed again until the process exits (= runtime exits). 
// You can only set a default before you perform any operation that results in the runtime using the default that it picked.
// You set the default by calling accelerator::set_default(). Typically you would enumerate all accelerators on the system 
// and then choose the one you want as default.
//
// If during the process lifetime you want to change which accelerator you use, or you want different parts of your code 
// to use different accelerators (e.g. to take advantage of multiple accelerators simultaneously), then you need to get 
// an accelerator_view and pass that to the respective API, i.e. array constructor or parallel_for_each dispatch. 
// This is very easy through the default_view property on the accelerator, e.g.
// accelerator_view av = accelerator(accelerator::direct3d_warp).default_view;

/*
Accelerators and their properties.
   -- Description:               NVIDIA Quadro 5000M
      Device path:               PCI\VEN_10DE&DEV_06DA&SUBSYS_1520103C&REV_A3\4&ADCCE93&0&0018
      Version:                   11.0
      Dedicated memory:          2047424 KB
      Supports double precision: true
      Limited double precision:  true
      Has display:               true
      Is emulated:               false
      Is debug:                  true
								 
   -- Description:               Microsoft Basic Render Driver
      Device path:               direct3d\warp
      Version:                   11.1
      Dedicated memory:          0 KB
      Supports double precision: false
      Limited double precision:  false
      Has display:               false
      Is emulated:               true
      Is debug:                  true
								 
   -- Description:               Software Adapter
      Device path:               direct3d\ref
      Version:                   11.1
      Dedicated memory:          0 KB
      Supports double precision: true
      Limited double precision:  true
      Has display:               true
      Is emulated:               true
      Is debug:                  true
								 
   -- Description:               CPU accelerator
      Device path:               cpu
      Version:                   0.1
      Dedicated memory:          16702848 KB
      Supports double precision: false
      Limited double precision:  false
      Has display:               false
      Is emulated:               true
      Is debug:                  false
*/
void AmpExamples::AcceleratorProperties()
{
	cout << "\nAccelerators and their properties.\n";

	vector<accelerator> list = accelerator::get_all();

	for_each(list.begin(), list.end(), [](const accelerator a) {
		wcout << "   -- Description:               " << a.description << endl;
		wcout << "      Device path:               " << a.device_path << endl;
		wcout << "      Version:                   " << (a.version >> 16) << '.' << (a.version & 0xFFFF) << endl;
		wcout << "      Dedicated memory:          " << a.dedicated_memory << " KB" << endl;
		wcout << "      Supports double precision: " << ((a.supports_double_precision) ? "true" : "false") << endl;				// Note that full double precision is required by the concurrency::precise_math functions in <amp_math.h>
		wcout << "      Limited double precision:  " << ((a.supports_limited_double_precision) ? "true" : "false") << endl;
		wcout << "      Has display:               " << ((a.has_display) ? "true" : "false") << endl;
		wcout << "      Is emulated:               " << ((a.is_emulated) ? "true" : "false") << endl;
		wcout << "      Is debug:                  " << ((a.is_debug) ? "true" : "false") << endl;
		wcout << endl;
	});

#ifndef _DEBUG
	// Requires ref accelerator for debugging on GPU!!!
	bool r = PickAccelerator();
#endif

	// Now that we have a GPU accelerator, we can create views to other accelerators
	accelerator_view warp = accelerator(accelerator::direct3d_warp).default_view;
	wcout << L"\n   Aquired another accelerator: " << warp.accelerator.description << endl;

	// While default view is on the gpu
	accelerator_view gpu = accelerator().default_view;
	wcout << L"   Default view:                " << gpu.accelerator.description << endl;
}

bool AmpExamples::PickAccelerator()
{
	bool success = false;

	vector<accelerator> list = accelerator::get_all();

	auto result = find_if(list.begin(), list.end(), [](const accelerator& a) {
		return	!a.is_emulated
				&& a.supports_double_precision 
				//&& !a.has_display
				;
	});

	if (result != list.end())
	{
		accelerator gpu = *result;
		success = accelerator::set_default(gpu.device_path);

		if (success)
		{
			wcout << "\n   Accelerator for the process: " << gpu.description << endl;
			return true;
		}
	}

	accelerator warp(L"direct3d\\warp");
	success = accelerator::set_default(warp.device_path);

	wcout << "\n   Accelerator for the process: " << warp.description << endl;	

	return success;
}

// When you create an accelerator_view via the create_view method of the accelerator, 
// you pass in an option of queuing_mode_immediate or queuing_mode_automatic, 
// which are the two members of the queuing_mode enum. At any point you can access 
// this value from the queuing_mode property of the accelerator_view. 
//
// When the queuing_mode value is queuing_mode_automatic (which is the default), 
// any commands sent to the device such as kernel invocations and data transfers 
// (e.g. parallel_for_each and copy), will get submitted as soon as the runtime sees fit. 
//
// When the value of queuing_mode is queuing_mode_immediate, the commands will be submitted/flushed immediately. 
//
// To send all buffered commands to the device for execution, there is a non-blocking flush method. 
// If you wish to block until all the commands have been sent, there is a wait method (which also flushes). 
/*
Accelerator Views and their properties.
   -- Description:               NVIDIA Quadro 5000M
      Version:                   11.0
      Is debug:                  true
      Queing mode:               automatic

   -- Description:               Microsoft Basic Render Driver
      Version:                   11.1
      Is debug:                  true
      Queing mode:               automatic

   -- Description:               Software Adapter
      Version:                   11.1
      Is debug:                  true
      Queing mode:               automatic

   -- Description:               CPU accelerator
      Version:                   0.1
      Is debug:                  false
      Queing mode:               automatic
*/
void AmpExamples::AcceleratorViewProperties()
{
	cout << "\nAccelerator Views and their properties.\n";

	vector<accelerator> list = accelerator::get_all();

	for_each(list.begin(), list.end(), [](accelerator a) {
		accelerator_view av = a.create_view();

		wcout << "   -- Description:               " << av.accelerator.description << endl;
		wcout << "      Version:                   " << (av.version >> 16) << '.' << (av.version & 0xFFFF) << endl;
		wcout << "      Is debug:                  " << ((av.is_debug) ? "true" : "false") << endl;
		wcout << "      Queing mode:               " << ((av.queuing_mode == queuing_mode::queuing_mode_automatic) ? "automatic" : "immediate") << endl;
		wcout << endl;
	});
}

// The following two examples illustrate the primary components of C++ AMP. 
// Assume that you want to add the corresponding elements of two one-dimensional arrays. 
// For example, you might want to add {1, 2, 3, 4, 5} and {6, 7, 8, 9, 10} to obtain {7, 9, 11, 13, 15}. 
// Without using C++ AMP, you might write the following code to add the numbers and display the results.
void AmpExamples::StandardAdd()
{
	cout << "Standard addition:\n";

	int a[] = { 1, 2, 3, 4, 5 };
	int b[] = { 6, 7, 8, 9, 10 };
	int t[ARRAYSIZE(a)];

	for (int i = 0; i < ARRAYSIZE(a); ++i)
	{
		t[i] = a[i] + b[i];
	}

	for (int i = 0; i < ARRAYSIZE(a); ++i)
	{
		cout << "t[" << i << "] = " << t[i] << endl;
	}
}

// The important parts of the code are as follows:
//   Data: The data consists of three arrays. All have the same rank (one) and length (five).
//   Iteration: The first for loop provides a mechanism for iterating through the elements in the arrays. The code that you want to execute to compute the sums is contained in the first for block.
//   Index: The idx variable accesses the individual elements of the arrays.
// Using C++ AMP, you might write the following code instead.

void AmpExamples::AmpAdd()
{
	cout << "\nAMP addition:\n";
	int a[] = { 1, 2, 3, 4, 5 };
	int b[] = { 6, 7, 8, 9, 10 };

	const int size = ARRAYSIZE(a);
	int t[size];

	Timer timer;
	timer.Start();

	// Create C++ AMP objects
	array_view<const int, 1> x(size, a);

	timer.Stop();
	cout << "array_view init time = " << timer.Elapsed() << endl;

	timer.Start();

	array_view<const int, 1> y(size, b);
	array_view<int, 1> z(size, t);

	timer.Stop();
	cout << "after that = " << timer.Elapsed() << endl;

	z.discard_data();

	parallel_for_each(z.extent, [=](index<1> idx) restrict(amp){
		z[idx] = x[idx] + y[idx];
	});

	for (int i = 0; i < size; ++i)
	{
		cout << "t[" << i << "] = " << z[i] << endl;
	}
}

// Creates a one-dimensional index that specifies the third element in a one-dimensional array_view object. 
// The index is used to print the third element in the array_view object. The output is 3.
void AmpExamples::CreateIndex1D()
{
	cout << "\nCreate Index:\n";

	int a[] = { 1, 2, 3, 4, 5 };

	array_view<int, 1> x(ARRAYSIZE(a), a);

	index<1> idx(2);

	cout << "a[2] = " << x[idx] << endl;		// 3
}

// Creates a two-dimensional index that specifies the element where the row = 1 and the column = 2 
// in a two-dimensional array_view object. The first parameter in the index constructor is the row component, 
// and the second parameter is the column component. The output is 6.
void AmpExamples::CreateIndex2D()
{
	int a[] = {
		1, 2, 3,
		4, 5, 6 
	};

	// 2 rows, 3 colums
	array_view<int, 2> x(2, 3, a);

	index<2> idx(1, 2);

	cout << "a[1,2] = " << x[idx] << endl;		// 6
}

// The following example creates a three-dimensional index that specifies the element 
// where the depth = 0, the row = 1, and the column = 3 in a three-dimensional array_view object. 
// Notice that the first parameter is the depth component, the second parameter is the row component, 
// and the third parameter is the column component. The output is 8.
void AmpExamples::CreateIndex3D()
{
	// 24 elements
	int a[] = 
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
	};
	
	/*
		2 rows, 4 colums:

		1, 2, 3, 4, 
		5, 6, 7, 8, 
		9, 10, 11, 12, 
		21, 22, 23, 24, 
		25, 26, 27, 28, 
		29, 30, 31, 32

		2 depth, 3 rows, 4 columns:

		z[1] (depth 1)
		1, 2, 3, 4, 
		5, 6, 7, 8, 
		9, 10, 11, 12, 

		z[2] (depth 2)
		21, 22, 23, 24, 
		25, 26, 27, 28, 
		29, 30, 31, 32

		2 depth, 4 rows, 3 columns:

		z[1] (depth 1)
		1, 2, 3, 
		4, 5, 6, 
		7, 8, 9, 
		10, 11, 12, 

		z[2] (depth 2)
		21, 22, 23, 
		24, 25, 26, 
		27, 28, 29, 
		30, 31, 32
	*/

	// 2 depth, 3 rows, 4 columns: size = 2 * 3 * 4 = 24
	array_view<int, 3> x(2, 3, 4, a);

	concurrency::extent<3> e(2, 4, 3);
	array_view<int, 3> y(e, a);

	index<3> idx(1, 1, 2);

	cout << "x[0,1,3] = " << x[idx] << endl;		// 27
	cout << "y[0,1,3] = " << y[idx] << endl;		// 26
}

// The extent Class (C++ AMP) specifies the length of the data in each dimension of the array or array_view object. 
// You can create an extent and use it to create an array or array_view object. You can also retrieve the extent 
// of an existing array or array_view object. The following example prints the length of the extent in each dimension of an array_view object.
void AmpExamples::CreateExtent()
{
	// 24 elements
	int a[] = 
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
	};

	// 2 x 3 x 4 (depth x rows x columns)
	array_view<int, 3> x(2, 3, 4, a);
	
	concurrency::extent<3> e(2, 3, 4);
	array_view<int, 3> y(e, a);

	cout << "\nCreate Extent.\n";
	cout << "  Columns: " << x.extent[2] << endl;
	cout << "  Rows:    " << x.extent[1] << endl;
	cout << "  Depth:   " << x.extent[0] << endl;
}

// When we capture an object by reference, it means we are using the original object in the body of the lambda. 
// The only types C++ AMP allows to be captured by reference are concurrency::array and concurrency::texture. 
// Memory for these data containers is directly allocated in the address space of the specified accelerator 
// and this allows the original object to be accessed inside a parallel_for_each.
//
// Here is a small example where we have added ‘&arr’ to the capture clause to specify that the body of the lambda 
// will access the array variable ‘arr’ by reference. Since the array resides on the accelerator memory, 
// updates to the data made are not visible on the host automatically. To get back updates, we need to explicitly transfer data 
// from the accelerator to host memory after the call to parallel_for_each.
//
// Allocation of data for concurrency::array takes place on an accelerator at array construction time.
void AmpExamples::CaptureByReference()
{
	cout << "\nCapture by reference.\n";

	const int size = 100000;
	vector<int> data(size, 5);

	accelerator_view av = accelerator().default_view;

	// array is allocated on accelerator and data is copied in
	array<int, 1> arr(size, data.begin(), av);

	// array has been copied to the device and must be captured by reference
	parallel_for_each(arr.extent, [&arr](index<1> idx) restrict(amp) {
		arr[idx] = arr[idx] + 1;
	});

	// Explicitly copy updates back to the host memory
	copy(arr, data.begin());

	for (int i = 0; i < 10; ++i)
	{
		cout << "   data[" << i << "] = " << data[i] << endl;
	}
}

// The other method of capturing variables is by value. As the name indicates, this capture method uses a copy 
// of the original object in the body of the lambda. Since types other than array or texture are allocated 
// in the address space of the host, a copy of the object needs to be created on the accelerator for use in the 
// parallel_for_each. The C++ AMP runtime automatically takes care of this for us by marshaling over copies 
// of all variables that are captured by value.
//
// Here is an example where we have added ‘=’ to the capture clause to specify that the body of the lambda can access 
// all captured variables by value unless explicitly opted out. (In this case, the array ‘arr’ is opted out of the capture by value)
void AmpExamples::CaptureByValue()
{
	cout << "\nCapture by value.\n";

	const int size = 100000;
	vector<int> data(size, 5);

	accelerator_view av = accelerator().default_view;
	array<int, 1> arr(size, data.begin(), av);

	int a = 10;		// must be copied by value

	parallel_for_each(arr.extent, [=, &arr](index<1> idx) restrict(amp) {
		arr[idx] = arr[idx] + a;
	});

	// Explicitly copy updates back to the host memory
	copy(arr, data.begin());

	for (int i = 0; i < 10; ++i)
	{
		cout << "   data[" << i << "] = " << data[i] << endl;
	}
}

// The rule to capture all other variables by value extends to the C++ AMP types concurrency::array_view 
// and concurrency::graphics::writeonly_texture_view as well. Instances of these types can only be captured by value.
//
// These types represent views of data and can be thought of as pointers to the actual underlying data. 
// If you declare an array_view or writeonly_texture_view object outside the parallel_for_each (in host code), 
// the object essentially wraps a reference to the data which it is a view of. Thus, like any other regular C++ object, 
// capturing the array_view by value in a parallel_for_each results in a copy of the object being marshaled to the accelerator. 
// The copied object continues pointing to its underlying data source.
//
// The example below captures the array_view ‘arr_view’ by value by adding a ‘=’ to the capture clause. 
// To get back updates to the data, we can either explicitly copy the array_view back to the host or take advantage 
// of its automatic synchronization on first use.
void AmpExamples::CaptureArrayViewOrTextureView()
{
	cout << "\nCapture array_view or writeonly_texture_view.\n";

	const int size = 100000;
	vector<int> data(size, 5);

	// Will be captured by value. Underlying data is copied to the accelerator at the time of capture
	array_view<int, 1> view(size, data);

	parallel_for_each( view.extent, [=](index<1> idx) restrict(amp) {
		view[idx]++;
	});

	// array_view is automatically synchronized on first access after parallel_for_each
	// but must synchronize it explicitely if working on underlying data
	view.synchronize();

	for (int i = 0; i < 10; ++i)
	{
		cout << "   data[" << i << "] = " << data[i] << endl;
	}
}

// When an array object is constructed, a deep copy of the data is created on the accelerator if you use a constructor 
// that includes a pointer to the data set. The kernel function modifies the copy on the accelerator. 
// When the execution of the kernel function is finished, you must copy the data back to the source data structure. 
// The following example multiplies each element in a vector by 10. After the kernel function is finished, 
// the vector conversion operator is used to copy the data back into the vector object.
void AmpExamples::AmpArrayAdd()
{
	vector<int> v(5);

	for (int i = 0; i < 5; ++i)
	{
		v[i] = i;
	}

	array<int, 1> a(static_cast<int>(v.size()), v.begin(), v.end());

	parallel_for_each(
		a.extent,
		[=, &a](index<1> idx) restrict(amp)
		{
			a[idx] = a[idx] * 10;
		}
	);

	v = a;

	cout << "\nAdd using array.\n";

	for (int i = 0; i < v.size(); ++i)
	{
		cout << "  v[" << i << "] = " << v[i] << endl;
	}
}

/*
The parallel_for_each method takes two arguments, a compute domain and a lambda expression.

The compute domain is an extent object or a tiled_extent object that defines the set of threads 
to create for parallel execution. One thread is generated for each element in the compute domain. 
If the extent object is one-dimensional and has five elements (array has 5 elements), five threads are started.

The lambda expression defines the code to run on each thread. The capture clause, [=], specifies 
that the body of the lambda expression accesses all captured variables by value. 

The lambda expression can include the code to execute or it can call a separate kernel function. 
The kernel function must include the restrict(amp) modifier.
*/
void AmpExamples::AmpArrayAddWithFunction()
{
	int a[] = { 1, 2, 3, 4, 5 };
	int b[] = { 6, 7, 8, 9, 10 };

	const int size = ARRAYSIZE(a);
	int s[size];
	ZeroMemory(s, sizeof(s));

	array_view<const int, 1> x(size, a);
	array_view<const int, 1> y(size, b);
	array_view<int, 1> sum(size, s);

	accelerator_view av = accelerator().create_view();

	Timer t;

	av.wait();
	t.Start();

	parallel_for_each(sum.extent, [=](index<1> idx) restrict(amp) {
		AddElementsInternal(idx, sum, x, y);
	});

	av.wait();
	t.Stop();

	cout << "\nAdd using function in lambda.\n";
	cout << "AddElementsInternal call time: " << t.Elapsed() << " ms" << endl;

	for (int i = 0; i < size; ++i)
	{
		cout << x[i] << " + " << y[i] << " = " << sum[i] << endl;
	}
}

// Simple matrix multiplication on CPU
/*
Will produce the following result:
  60 45
  49 43
  141 92
*/
void AmpExamples::CPUMatrixMultiplication()
{
	const int r1 = 3;
	const int c1 = 3;

	const int r2 = 3;
	const int c2 = 2;

	float m1[r1][c1] =
	{
		{ 6, 3, 0 },
		{ 2, 5, 1 },
		{ 9, 8, 6 }
	};

	float m2[r2][c2] =
	{
		{ 7, 4 },
		{ 6, 7 },
		{ 5, 0 }
	};

	assert(c1 == r2);

	float r[r1][c2];
	ZeroMemory(r, sizeof(r));

	for (int z = 0; z < c2; ++z)					// 2 colums of matrix #2
	{
		for (int x = 0; x < r1; ++x)				// 3 rows of matrix #1
		{
			float sum = 0;

			for (int y = 0; y < c1; ++y)			// 3 colums of matrix #1
			{
				sum += m1[x][y] * m2[y][z];
			}

			r[x][z] = sum;
		}
	}

	cout << "\nMatrix multiplication on CPU.\n";

	for (int x = 0; x < r1; ++x)
	{
		cout << "   ";

		for (int y = 0; y < c2; ++y)
		{
			cout << r[x][y] << " ";
		}

		cout << endl;
	}
}

// Simple AMP matrix multiplication
void AmpExamples::AmpMatrixMultiplication()
{
	const int r1 = 3;
	const int c1 = 3;

	const int r2 = 3;
	const int c2 = 2;

	float m1[r1 * c1] =
	{
		6, 3, 0,
		2, 5, 1,
		9, 8, 6
	};

	float m2[r2 * c2] =
	{
		7, 4,
		6, 7,
		5, 0
	};
	
	assert(c1 == r2);

	const vector<const float> v1(m1, m1 + ARRAYSIZE(m1));
	const vector<const float> v2(m2, m2 + ARRAYSIZE(m2));

	vector<float> v3(r1 * c2);

	concurrency::extent<2> e1(r1, c1);
	concurrency::extent<2> e2(r2, c2);
	concurrency::extent<2> e3(r1, c2);		// to store result

	array_view<const float, 2> a(e1, v1);
	array_view<const float, 2> b(e2, v2);

	array_view<float, 2> c(e3, v3);
	c.discard_data();									// Do not copy to the device
	
	parallel_for_each(c.extent, [=](index<2> idx) restrict(amp) {
		int x = idx[0];
		int z = idx[1];

		float sum = 0;

		for (int y = 0; y < c1; ++y)
		{
			sum += a(x, y) * b(y, z);
		}

		c[idx] = sum;
	});

	c.synchronize();

	cout << "\nMatrix multiplication on GPU.\n";

	for (int x = 0; x < r1; ++x)
	{
		cout << "   ";

		for (int y = 0; y < c2; ++y)
		{
			index<2> idx(x, y);
			cout << c(idx) << " ";
		}

		cout << endl;
	}
}

//
// Within an amp-restricted function, the target of a function-like invocation 
// (e.g., functions, member functions, object constructors & destructors, operators) 
// must be amp-restricted too. Following the amp type restrictions, we know that 
// it cannot be a virtual function or a function pointer/pointer to member function either. 
// In addition, due to the lack of hardware stack and function call support, 
// it is not allowed for a function to recursively invoke itself directly or via other functions indirectly. 
//
// All context variables must be captured by value except for concurrency::array 
// and concurrency::graphics::texture objects, which are only allowed to be captured by reference;
void AmpExamples::AddElementsInternal(
	index<1>						idx, 
	array_view<int, 1>				sum, 
	const array_view<const int, 1>	a, 
	const array_view<const int, 1>	b
	) restrict(amp)
{
	sum[idx] = a[idx] + b[idx];
}

// The code replaces each value in the tile by the average of the values in the tile.
// It then stores results in an ARRAY OF THE SAME SIZE.
//
// For today’s hardware, starting with a tile size of a multiple of 64 and ensuring that conditionals and loops would not diverge threads at the 64 boundary, 
// is going to result in better thread utilization.
//
void AmpExamples::CalculateTileAverages()
{
	cout << "\nCalculate tile averages.\n";

	// Sample data:
	int d[] = {
		2, 2, 9, 7, 1, 4,
		4, 4, 8, 8, 3, 4,
		1, 5, 1, 2, 5, 2,
		6, 8, 3, 2, 7, 2};

	// The tiles:
	// tile 1
	// 2 2    9 7    1 4
	// 4 4    8 8    3 4
	//
	// tile 2
	// 1 5    1 2    5 2
	// 6 8    3 2    7 2

	const int size = ARRAYSIZE(d);		// 24 elements

	// We'll create 6 tiles
	int avg[size];						// 24 elements
	ZeroMemory(avg, sizeof(avg));

	// Averqages
	// 3 8 3
	// 5 2 4

	// Create views
	array_view<const int, 2> x(4, 6, d);			// 4 rows, 6 columns
	array_view<int, 2>		 w(4, 6, avg);			

	parallel_for_each(
		x.extent.tile<2, 2>(),						// Create threads for x.extent and divide the extent into 2x2 tiles.
		[=](tiled_index<2, 2> idx) restrict(amp)
		{
			// Create a 2 x 2 array to hold the values in this tile
			tile_static int nums[2][2];

			// Copy the values for the tile into the 2x2 array
			nums[idx.local[1]][idx.local[0]] = x[idx.global];

			// When all the threads have executed and the 2x2 array is complete, find the average
			idx.barrier.wait();

			int sum = nums[0][0] + nums[0][1] + nums[1][0] + nums[1][1];

			// Copy the average into the array_view
			index<2> x = idx.global;
			w[idx.global] = sum / 4;
		}
	);

	// Output:
	// 3 3 8 8 3 3
	// 3 3 8 8 3 3
	// 5 5 2 2 4 4
	// 5 5 2 2 4 4
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 6; ++j)
		{
			cout << w(i,j) << " ";
		}
		cout << endl;
	}
}

// The code replaces each value in the tile by the average of the values in the tile.
// It then stores results in a REDUCED array.
void AmpExamples::CalculateTileAverages2()
{
	cout << "\nCalculate tile averages 2.\n";

	// Sample data:
	int d[] = {
		2, 2, 9, 7, 1, 4,
		4, 4, 8, 8, 3, 4,
		1, 5, 1, 2, 5, 2,
		6, 8, 3, 2, 7, 2};

	// The tiles:
	// tile 1
	// 2 2    9 7    1 4
	// 4 4    8 8    3 4
	//
	// tile 2
	// 1 5    1 2    5 2
	// 6 8    3 2    7 2

	const int size = ARRAYSIZE(d);		// 24 elements

	// We'll create 6 tiles
	int avg[size / 4];					// 6 elements
	ZeroMemory(avg, sizeof(avg));

	// Averqages
	// 3 8 3
	// 5 2 4

	// Create views
	array_view<const int, 2> x(4, 6, d);			// 4 rows, 6 columns
	array_view<int, 2>		 z(2, 3, avg);			// 2 rows, 3 columns

	parallel_for_each(
		x.extent.tile<2, 2>(),						// Create threads for x.extent and divide the extent into 2x2 tiles.
		[=](tiled_index<2, 2> idx) restrict(amp)
		{
			// Create a 2 x 2 array to hold the values in this tile
			tile_static int nums[2][2];

			// Copy the values for the tile into the 2x2 array
			nums[idx.local[1]][idx.local[0]] = x[idx.global];

			// When all the threads have executed and the 2x2 array is complete, find the average
			idx.barrier.wait();

			int sum = nums[0][0] + nums[0][1] + nums[1][0] + nums[1][1];

			// Copy the average into the array_view
			if (idx.local[0] == 0 && idx.local[1] == 0)
			{
				//index<2> x = idx.tile;
				z[idx.tile] = sum / 4;
			}
		}
	);

	// Output:
	// 3 8 3
	// 5 2 4
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			cout << z(i,j) << " ";
		}
		cout << endl;
	}
}

// C++ AMP includes two math libraries. The double-precision library in the Concurrency::precise_math Namespace 
// provides support for double-precision functions. It also provides support for single-precision functions, 
// although double-precision support on the hardware is still required. It complies with the C99 Specification (ISO/IEC 9899). 
// The accelerator must support full double precision. You can determine whether it does by checking the value 
// of the accelerator::supports_double_precision Data Member. The fast math library, in the Concurrency::fast_math Namespace, 
// contains another set of math functions. These functions, which support only float operands, execute more quickly 
// but aren’t as precise as those in the double-precision math library. The functions are contained in the <amp_math.h> header file 
// and all are declared with restrict(amp). The functions in the <cmath> header file are imported into both the fast_math 
// and precise_math namespaces. The restrict keyword is used to distinguish the <cmath> version and the C++ AMP version.
// The following code calculates the base-10 logarithm, using the fast method, of each value that is in the compute domain.
void AmpExamples::CalculateLog()
{
	cout << "\nCalculate log10.\n";

	if (!accelerator().supports_double_precision)
	{
		throw std::exception("Accelerator does not support double precision.");
	}

	double n[] = { 1.0, 10.0, 60.0, 100.0, 600.0, 1000.0 };
	const int size = ARRAYSIZE(n);

	double r[size];

	Timer t;
	accelerator_view av = accelerator().create_view();

	array_view<const double, 1> num(size, n);
	array_view<double, 1> log(size, r);

	t.Start();
	av.wait();

	parallel_for_each(
		num.extent,
		[=](index<1> idx) restrict(amp)
		{
			//log[idx] = fast_math::log10(num[idx]);
			log[idx] = precise_math::log10(num[idx]);
		}
	);

	av.wait();
	t.Stop();

	cout << "log10 call time: " << t.Elapsed() << " ms" << endl;

	for (int i = 0; i < size; ++i)
	{
		cout << "log10(" << num(i) << ") = " << log(i) << endl;
	}
}

// Shows some basic operations that you can do with the array_view
// Here we use array_view<const ...>
void AmpExamples::ArrayViewOps()
{
	cout << "\narray_view legal ops.\n";

	const int size = 10;
	int* p1 = new int[size];
	int p2[] = { 1, 2, 3, 4, 5, 6, 7 };

	array_view<const int, 1> a(size, p1);

	// Now we can only read from a
	int x = a[0];

	//a[0] = 6;		// Error: IntelliSense: expression must be a modifiable lvalue

	// Re-assign a to point to a different underlying data source
	a = array_view</*const */int, 1>(ARRAYSIZE(p2), p2);

	array_view<int, 1> b(ARRAYSIZE(p2), p2);
	b[0] = 11;

	cout << "a[0] = " << a[0] << endl;

	delete p1;
}

// Here we show const array_view<...>
void AmpExamples::ArrayViewOps2()
{
	cout << "\narray_view legal ops 2.\n";

	int p1[] = { 1, 2, 3, 4, 5 };
	int p2[] = { 6, 7, 8, 9, 10 };
	const size_t size = ARRAYSIZE(p1);

	const array_view<int, 1> a(size, p1);

	// we can read and write into a
	a[0] = a[1] + 20;

	cout << "a[0] = " << a[0] << endl;

	//a = array_view<int, 1>(size, p2);		// Error: IntelliSense: no operator "=" matches these operands operand types are: const array_view<int, 1> = array_view<int, 1>
}

// Some more restrictions on read-only access to the source
void AmpExamples::ArrayViewOps3()
{
	const int size = 10;
	int*	  p1 = new int[size];
	const int p2[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	for (int i = 0; i < 10; ++i)
	{
		p1[i] = i;
	}

	// Create read-only wrapper to p1
	array_view<const int, 1> a1(size, p1);

	// Read-write array view
	array_view<int, 1> a2(size, p1);

	// Construct read-only array-view from a2
	array_view<const int, 1> a3 = a2;

	parallel_for_each(a1.extent, [=](index<1> idx) restrict(amp)
		{
			// a1 is of type <const int> and can only read data. It will nto be copied back onto the CPU
			int x = a1[idx];

			//a1[idx] = x + 3;				// Error: expression must be a modifiable lvalue
		}
	);

	// Cannot construct writable view from read-only
	//array_view<int, 1> a4(size, p2);		// Error: error C2338: container element type and array view element type must match

	//  Same as above
	// 	Error: no instance of constructor "Concurrency::array_view<_Value_type, _Rank>::array_view [with _Value_type=int, _Rank=1]" matches the argument list
	//         argument types are: (Concurrency::array_view<const int, 1>)
	// array_view<int, 1> a5(a1);			// Error

	delete p1;
}

// Allowed and not operations on array<>
void AmpExamples::ArrayOps()
{
	int p1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	const int size = ARRAYSIZE(p1);

	// array denotes a data container and the contents of the source p1 are copied
	// to the newly allocated memory.
	array<int, 1> a1(size, p1);

	// const array<T> denotes a read-only array
	const array<int, 1> a2(size, p1);

	// Can also create a const reference to a rw array
	const array<int, 1> & a3 = a1;

	parallel_for_each(a3.extent, [&](index<1> idx) restrict(amp)
		{
			//a3[idx] = a2[idx] + 5;		// Error: expression must be a modifiable lvalue
		}
	);

	//array<int, 1> & a4 = a3;				// Error: qualifiers dropped in binding reference of type "Concurrency::array<int, 1> &" to initializer of type "const Concurrency::array<int, 1>"
}

// Executes operation on the array_view concurrently in two different kernels
void AmpExamples::ConcurrentArrayViewOp()
{
	int p[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	const int size = ARRAYSIZE(p);

	int r1[size];

	array_view<const int, 1> a1(size, p);
	array_view<int, 1> o1(size, r1);

	parallel_for_each(o1.extent, [=](index<1> idx) restrict(amp)
		{
			o1[idx] = a1[idx] << 2;
		}
	);

	int r2[size];
	array_view<const int, 1> a2(o1);
	array_view<int, 1> o2(size, r2);

	parallel_for_each(o2.extent, [=](index<1> idx) restrict(amp)
		{
			// Output of the first kernel is captured as read-only in the 2nd kernel
			o2[idx] = a2[idx] * a2[idx];
		}
	);

	// Synchronizing a2 content to the CPU can be executed concurrently
	// with the kernel above since both the copy operation and teh kernel
	// are accessing the data in a read-only mode.
	o1.synchronize();

	cout << "\nConcurrently execute read-only array_view in 2 kernels.\n";

	for (int i = 0; i < size; ++i)
	{
		cout << "p[" << i << "] = " << p[i] << " ; o1 = " << o1(i) << " ; o2 = " << o2[i] << endl;
	}
}

// Demonstrates allowed and disallowed casts
// Since pointers in C++ AMP are actually emulated on top of the DirectX 11 platform which does not support pointers, 
// to prevent patterns that may result in extremely inefficient emulated code, we disallow casting a pointer 
// to an integral type, nor an integral type to a pointer. This restriction applies to reinterpret_cast 
// as well as to C-style casts. Note that the implicit casting from constant 0 to any pointer type 
// and from any pointer type to bool is still allowed, and is treated as the null pointer just like regular C++. For example,
void AmpExamples::CastRestrictions()
{
	int p[] = { 1, 2, 3, 4, 5 };
	const int size = ARRAYSIZE(p);

	int r[size];

	array_view<const int, 1> a(size, p);
	array_view<int, 1> z(size, r);

	parallel_for_each(a.extent, [=](index<1> idx) restrict(amp)
		{
			int n1 = a[idx];
			int* p1 = 0;				// legal: 0 -> int*
			//int* p2 = reinterpret_cast<int*>(n1);		// error: illegal cast in amp-restricted function
			//int n2 = (int)p1;							// error: illegal cast in amp-restricted function
			void* p3 = p1;				// legal: int* -> void*
	
			if (p1 == nullptr)			// legal
			{
				n1 = 5;
				p1 = &n1;
			}

			if (p3)						// legal: int* -> bool
			{
				n1 = 6;
			}

			z[idx] = a[idx] * n1;
		}
	);
}

// Demonstrates restrictions on const qualifier
// Casting away constness from a pointer or reference will result in a compiler warning 
// and/or undefined behavior. This is because we rely on constness to decide what kind 
// of underlying GPU memory access mode (read-only or writable) to use and try to exploit 
// readonliness as much as possible to achieve better performance. Casting away constness 
// may result in writing into a read-only GPU buffer, which is not allowed. For example, 
void AmpExamples::ConstRestrictions()
{
	concurrency::extent<1> e(10);
	array<int, 1> a(e);
	array<int, 1> b(e);

	const array<int, 1> & c = a;

	parallel_for_each(a.extent, [&](index<1> idx) restrict(amp)
		{
			b[idx] = idx[0];

			// c[idx] returns a 'const int&'
			// cast &c[idx] to int triggers error
			//int* p = reinterpret_cast<int*>(&c[idx]);		// error: reinterpret_cast cannot cast away const or other type qualifiers
			//*p = idx[0];									// error: cannot write into read-only buffer
		}
	);
	
	//cout << "\nConst restrictions.\n";
	/*
	for (int i = 0; i < e.size(); ++i)
	{
		cout << "b[" << i << "] = " << b[i] << endl;
	}
	*/
}

// Alignment explanation
// Pointer arithmetic is not allowed to be performed on pointers to bool values 
// since that will result in invalid pointers pointing to an unaligned data region.
void AmpExamples::PointerRestrictions()
{
	int p[] = { 1, 2, 3, 4, 5 };
	const int size = ARRAYSIZE(p);

	array_view<int, 1> a(size, p);

	parallel_for_each(a.extent, [=](index<1> idx) restrict(amp)
		{
			struct A
			{
				bool flag;
				int  data;
			};

			A a;

			bool* p1 = &(a.flag);
			//bool* p2 = p1++;				// error C3599: '++' : cannot perform pointer arithmetic on pointer to bool in amp restricted code

			//bool b = *(p2);

			// Compiler Error:
			// base class, data member or array element must be at least 4-byte aligned for amp-restricted function 
			//
			/*
			struct B
			{
				bool flag;
				bool data;
			};

			B b;
			*/

			// Solution to B is to allign the struct
			struct C
			{
				bool flag;
				__declspec(align(4)) bool data;			// Note that the alignment is only applied to the field
			};

			C c;

			// To align a structure
			typedef __declspec(align(4)) 
			struct D 
			{
				bool flag;
			} 
			ALIGNED_BOOL;

			ALIGNED_BOOL d[10];							// Now we can create an array of aligned fields
		}
	);
}

// 
void AmpExamples::LambdaExample()
{
	struct Foo
	{
		int z;
	};

	Foo ambientVariable;
	auto functor = [ambientVariable](int y) restrict(amp)->int { return y + ambientVariable.z; };
}

//
// Limits on the number of data containers (array_view) in the AMP kernel.
// Can only work on 8 views at a time.
//

// error C2248: 'Concurrency::array<_Value_type,_Rank>::array' : cannot access private member 
//              declared in class 'Concurrency::array<_Value_type,_Rank>'
/*
struct MyPoint
{
	array<float, 2> x;
};
*/

void AmpExamples::ArrayAMPLimit()
{
	cout << "\narray_view AMP limitations.\n";

	const unsigned int width = 1920;
	const unsigned int height = 1080;

	accelerator_view av = accelerator().create_view();

	concurrency::extent<2> ext(width, height);
	const int r = 2;

	// Intersection
	array<float, r> ix(ext, av);		array_view<float, r> ivx(ix);
	array<float, r> iy(ext, av);		array_view<float, r> ivy(iy);
	array<float, r> iz(ext, av);		array_view<float, r> ivz(iz);

	// Normal
	array<float, r> nx(ext, av);		array_view<float, r> nvx(nx);
	array<float, r> ny(ext, av);		array_view<float, r> nvy(ny);
	array<float, r> nz(ext, av);		array_view<float, r> nvz(nz);

	// Color
	array<UINT, r> cr(ext, av);			array_view<UINT, r> cvr(cr);
	array<UINT, r> cg(ext, av);			array_view<UINT, r> cvg(cg);
	array<UINT, r> cb(ext, av);			array_view<UINT, r> cvb(cb);

	// Ray direction
	array<float, r> rx(ext, av);		array_view<float, r> rvx(rx);
	array<float, r> ry(ext, av);		array_view<float, r> rvy(ry);
	array<float, r> rz(ext, av);		array_view<float, r> rvz(rz);

	ivx.discard_data();
	ivy.discard_data();
	ivz.discard_data();
	nvx.discard_data();
	nvy.discard_data();
	nvz.discard_data();
	cvr.discard_data();
	cvg.discard_data();
	cvb.discard_data();
	rvx.discard_data();
	rvy.discard_data();
	rvz.discard_data();

	// Measure time
	//av.wait();
	time_point<system_clock> start = system_clock::now();

	// Kernel
	parallel_for_each(av, ext, [=](index<2> idx) restrict(amp) {
		ivx[idx] = 0.1f;
		ivy[idx] = 0.2f;
		ivz[idx] = 0.3f;

		nvx[idx] = 0.4f;
		nvy[idx] = 0.5f;
		nvz[idx] = 0.6f;

		//cvr[idx] = 7;
		//cvg[idx] = 8;
		//cvb[idx] = 9;

		//rvx[idx] = 1.0f;
		//rvy[idx] = 1.1f;
		//rvz[idx] = 1.2f;
	});
	
	parallel_for_each(av, ext, [=](index<2> idx) restrict(amp) {
		//ivx[idx] = 0.1f;
		//ivy[idx] = 0.2f;
		//ivz[idx] = 0.3f;

		//nvx[idx] = 0.4f;
		//nvy[idx] = 0.5f;
		//nvz[idx] = 0.6f;

		cvr[idx] = 7;
		cvg[idx] = 8;
		cvb[idx] = 9;

		rvx[idx] = 1.0f;
		rvy[idx] = 1.1f;
		rvz[idx] = 1.2f;
	});
	
	//av.wait();			// It will wait here perpetually if you have this line
	time_point<system_clock> stop = system_clock::now();

	// Executed in 127041us (127ms)
	long long ms = duration_cast<milliseconds>(stop - start).count();
	long long us = duration_cast<microseconds>(stop - start).count();
	cout << "Executed in " << us << "us (" << ms << "ms)" <<  endl;
	/*
	start = system_clock::now();
	
	ivx.synchronize();
	ivy.synchronize();
	ivz.synchronize();

	nvx.synchronize();
	nvy.synchronize();
	nvz.synchronize();
	
	cvr.synchronize();
	cvg.synchronize();
	cvb.synchronize();

	rvx.synchronize();
	rvy.synchronize();
	rvz.synchronize();
	
	stop = system_clock::now();
	ms = duration_cast<milliseconds>(stop - start).count();
	us = duration_cast<microseconds>(stop - start).count();
	cout << "Sync in " << us << "us (" << ms << "ms)" <<  endl;
	*/
	index<2> idx(0,0);
	cout << "Point  x=" << ivx[idx] << " y=" << ivy[idx] << " z=" << ivz[idx] << endl;
	cout << "Normal x=" << nvx[idx] << " y=" << nvy[idx] << " z=" << nvz[idx] << endl;
	cout << "Color  r=" << cvr[idx] << " g=" << cvg[idx] << " b=" << cvb[idx] << endl;
	cout << "Ray    x=" << rvx[idx] << " y=" << rvy[idx] << " z=" << rvz[idx] << endl;
}	   

//
// THe same example as above, but using encapsulated point
//

struct MyPoint
{
    static const int height = 1920;
    static const int width = 1024;

    // The array_view data member must be constructed in the 
    // member initialization list since array_view does not 
    // have a default constructor
    MyPoint(const accelerator_view &av) 
        : x(array<float, 2>(height, width, av))
		, y(array<float, 2>(height, width, av))
		, z(array<float, 2>(height, width, av))
    {
    }

    array_view<float, 2> x;
    array_view<float, 2> y;
    array_view<float, 2> z;
};

struct MyColor
{
    static const int height = 1920;
    static const int width = 1024;

    MyColor(const accelerator_view &av) 
        : r(array<UINT, 2>(height, width, av))
		, g(array<UINT, 2>(height, width, av))
		, b(array<UINT, 2>(height, width, av))
    {
    }

    array_view<UINT, 2> r;
    array_view<UINT, 2> g;
    array_view<UINT, 2> b;
};

/*
array_view AMP limitations.
Executed in 343024us (343ms)
Point  x=0.1 y=0.2 z=0.3
Normal x=0.4 y=0.5 z=0.6
Color  r=7 g=8 b=9
Ray    x=1 y=1.1 z=1.2

array_view AMP limitations with encapsulated containers.
Executed in 305042us (305ms)
Point  x=0.1 y=0.2 z=0.3
Normal x=0.4 y=0.5 z=0.6
Color  r=7 g=8 b=9
Ray    x=1 y=1.1 z=1.2
*/
void AmpExamples::ArrayAMPLimit2()
{
	cout << "\narray_view AMP limitations with encapsulated containers.\n";

	accelerator_view		av = accelerator().create_view();
	concurrency::extent<2>	ext(MyPoint::width, MyPoint::height);

	MyPoint vertex(av);
	MyPoint normal(av);
	MyPoint ray(av);
	MyColor color(av);

	vertex.x.discard_data();
	vertex.y.discard_data();
	vertex.z.discard_data();
	normal.x.discard_data();
	normal.y.discard_data();
	normal.z.discard_data();
	ray.x.discard_data();
	ray.y.discard_data();
	ray.z.discard_data();
	color.b.discard_data();
	color.g.discard_data();
	color.r.discard_data();

	time_point<system_clock> start = system_clock::now();

	// Kernels
	// Important: must capture by value!
	// error C3590: 'vertex': by-reference capture or 'this' capture is unsupported 
	// if the lambda is amp restricted
	parallel_for_each(av, ext, [=](index<2> idx) restrict(amp) {
		vertex.x[idx] = 0.1f;
		vertex.y[idx] = 0.2f;
		vertex.z[idx] = 0.3f;

		normal.x[idx] = 0.4f;
		normal.y[idx] = 0.5f;
		normal.z[idx] = 0.6f;
	});

	parallel_for_each(av, ext, [=](index<2> idx) restrict(amp) {
		color.r[idx] = 7;
		color.g[idx] = 8;
		color.b[idx] = 9;

		ray.x[idx] = 1.0f;
		ray.y[idx] = 1.1f;
		ray.z[idx] = 1.2f;
	});

	time_point<system_clock> stop = system_clock::now();

	long long ms = duration_cast<milliseconds>(stop - start).count();
	long long us = duration_cast<microseconds>(stop - start).count();
	cout << "Executed in " << us << "us (" << ms << "ms)" <<  endl;

	index<2> idx(0,0);
	cout << "Point  x=" << vertex.x[idx] << " y=" << vertex.y[idx] << " z=" << vertex.z[idx] << endl;
	cout << "Normal x=" << normal.x[idx] << " y=" << normal.y[idx] << " z=" << normal.z[idx] << endl;
	cout << "Color  r=" << color.r[idx]  << " g=" << color.g[idx]  << " b=" << color.b[idx] << endl;
	cout << "Ray    x=" << ray.x[idx]    << " y=" << ray.y[idx]    << " z=" << ray.z[idx] << endl;
}

// Shows how to use staging arrays to optimize data transfers between the host and a C++ AMP accelerator_view.
// When you use the new concurrency::array data container, you are in charge of the data transfer 
// between host memory (e.g. STL data containers) and the GPU memory (e.g. arrays on GPU accelerator_view’s). 
// Below is a simple example that contains copy between a std::vector and a device array.
void AmpExamples::StagingArrays()
{
	cout << "\nStaging array.\n";

	const int size = 100000;
	vector<int> data(size);

	// Initialize data
	int i = 0;
	
	generate(data.begin(), data.end(), [&i](){
		return ++i;
	});

	// allocate array on an accelerator_view
	time_point<system_clock> start = system_clock::now();
	
	accelerator_view av = accelerator().create_view();
	array<int, 1> a(size, av);

	// Note that the construction of array and the copying in data into array 
	// could also be combined into a single statement with a constructor like:
	array<int, 1> a2(size, data.begin(), data.end(), av);

	// Copy from the host vector to the device array
	copy(data.begin(), data.end(), a);

	// Process array on device
	
	// Copy from the device array to the host buffer
	copy(a, data.begin());

	// Work with data on the host

	time_point<system_clock> stop = system_clock::now();
	long long us = duration_cast<microseconds>(stop - start).count();

	// Copied to and from in: 2020us
	cout << "Copied to and from array in: " << us << "us" << endl;

	//
	// Staging array
	//

	// A staging array is constructed with a second accelerator_view.
	// The second accelerator_view parameter is called associated accelerator_view, 
	// it is a hint to the runtime that you will often transfer data between this array 
	// and other arrays on acclView2, therefore, the implementation should be optimized 
	// for such data transfers.
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476259%28v=vs.85%29.aspx

	// Allocate on host
	accelerator cpuAccelerator = accelerator(accelerator::cpu_accelerator);
	array<int, 1> staging(size, cpuAccelerator.default_view, av);

	// Polulate staging array on host
	i = 0;

	generate(staging.data(), staging.data() + size, [&i]() {
		return ++i;
	});

	start = system_clock::now();

	// Allocate on device
	array<int, 1> device(size, av);

	// Copy from the staging array to the device array
	copy(staging, device);

	// Process on device

	// Copy from device to staging
	copy(device, staging);

	// Process on host

	// Copied to and from staging in: 5977us
	stop = system_clock::now();
	us = duration_cast<microseconds>(stop - start).count();
	cout << "Copied to and from staging in: " << us << "us" << endl;

	//
	// Recommendation is that you use concurrency::array_view, and let the runtime take care 
	// of the copy operations implicitly. This way, you can get the benefit of improved 
	// copy performance by wrapping a staging array with an array_view, 
	// and use the array_view afterwards.
	//

	start = system_clock::now();

	// Create an array_view over the staging array located on the CPU
	array_view<int, 1> stagingAv(staging);

	// Use staging array_view, runtime manages copy to the device accelerator
	parallel_for_each(av, stagingAv.extent, [=](index<1> idx) restrict(amp) {
		stagingAv[idx] = idx[0];
	});

	// Access staging array_view on the CPU, runtime manages copy out
	const int * p = stagingAv.data();

	// Copied to and from array_view in: 21003us
	stop = system_clock::now();
	us = duration_cast<microseconds>(stop - start).count();
	cout << "Copied to and from array_view in: " << us << "us" << endl;

	for (int i = 0; i < 10; ++i)
	{
		cout << "   staging[" << i << "] = " << p[i] << endl;
	}

	// Accelerator description
	// Accelerator info: NVIDIA Quadro 5000M
	wcout << L"\nAccelerator info: " << accelerator().description << endl;
}

// You should be careful on caching pointers obtained via the array::data() method or &stagingArray[idx]. 
// These pointers are not guaranteed to be valid once a copy operation or parallel_for_each involving 
// the staging array or its array_view starts. Using these pointers can cause undefined behavior. 
// Therefore, these pointers must not be cached for use across such intervening operations.  
// Otherwise, you will get undefined behavior (e.g. access violation). 
//
// In general, you should consider using staging array only if the data transfer performance is critical for your application.
void AmpExamples::RelianceOnUnderlyingPointers()
{
	//cout << "\nUnderlying pointers.\n";

	// Create staging array
	const int size = 100000;

	accelerator_view deviceAv = accelerator().create_view();
	accelerator_view cpuAv    = accelerator(accelerator::cpu_accelerator).default_view;

	array<int, 1> staging(size, cpuAv, deviceAv);
	array<int, 1> device(size, deviceAv);

	generate(staging.data(), staging.data() + size, [=]() {
		return rand() % size;
	});

	int *		p1 =  staging.data();
	const int * p2 = &staging[0];

	qsort(p1, staging.extent.size(), sizeof(int), [](const void * a, const void * b) -> int {
		return *(int*)a - *(int*)b;
	});

	int x = *p2; // OK

	copy(staging, device);

	int y = staging[0];		// OK, using [] operator is safe

	staging[0] = 15;		// OK, using [] operator is safe

	*p1 = 0;				// Undefined, do not use cached pointer across copy which is using staging array

	int z = *p2;			// Undefined, do not use cached pointer across copy

	array_view<int, 1> deviceView(device);
	int * p3 = &deviceView[0];
	int   w  = *p3;			// OK

	const int * p4 = &staging[0];

	// Use device array
	parallel_for_each(deviceAv, deviceView.extent, [=](index<1> idx) restrict(amp) {
		deviceView[idx] = idx[0];
	});

	int r = *p3;			// Undefined, do not use cached pointer across parallel_for_each

	r += *p4;				// Undefined, do not use cached pointer across parallel_for_each
}

// Demonstraits ability to chain the dependent tasks using “continuations”. 
// A continuation is an asynchronous task that is invoked by another task, which is known as the antecedent, 
// when the antecedent completes. Continuations provide the ability of “wait-free” composition of tasks. 
void AmpExamples::ContinuationChaining()
{
	cout << "\nAsynchronous chained synchronization.\n";

	array<int, 1> a(512);
	array_view<int, 1> v(a);

	// Kick-off calculation in kernel
	parallel_for_each(v.extent, [=](index<1> idx) restrict(amp) {
		v[idx] = 10;
	});

	// Issue asynchronous synchronization of the results and chain the dependent task
	v.synchronize_async().then([]() {
		// Perform dependent task here
		cout << "   Dependent task executed." << endl;
	});

	// Continue performing other useful work on current CPU thread
	Sleep(1000);

	index<1> idx;
	for (int i = 0; i < 10; ++i)
	{
		cout << "      v[" << i << "] = " << v[idx] << endl;
	}

	cout << "   Completed chaining!" << endl;
}

// The moral here is: do not overlap views!
// An array_view or any other array_view overlapping it cannot be concurrently accessed 
// if any of the accesses is a write access. Such concurrent accesses constitute a data race 
// and have undefined behavior. Concurrent accesses can be made from multiple CPU threads 
// or on a single thread through any of the async APIs, and should be properly synchronized for correctness.
void AmpExamples::Synchronization()
{
	const int size = 10;
	vector<int> data(size, 2);			// 10 integers with value 2

	array_view<int, 1> v1(size, data);

	array_view<int, 1> s1 = v1.section(0, size / 2 + 1);
	array_view<int, 1> s2 = v1.section(size / 2 - 1, size / 2 + 1);			// Overlaps s1

	task<void> asyncCPUTask([&]()
		{
			s1[0]++;
		}
	);

	// Undefined behavior: Accessing s1 while another thread accesses s2 by asyncCPUTask above.
	//					   Illegal to do so even if non-overlapped elements are accessed.
	parallel_for_each(s2.extent, [=](index<1> idx) restrict(amp) {
		s2[1]++;
	});

	asyncCPUTask.get();

	// OK to access s1 after the concurrent accesses are synchronized
	s1[0]++;

	//
	vector<int> out(size);

	auto f = copy_async(s2, out.begin());

	// Incorrect: Illegal to access s1 since an asynchronous copy operation
	//            on an overlapping array_view s2 is in flight.
	parallel_for_each(s1.extent, [=](index<1> idx) restrict(amp) {
		s1[0]++;
	});

	// OK to access read-only view when the concurrent copy operation
	// is accessing the overlapping array_view s2
	array_view<const int, 1> rv1(s1);

	int value = rv1[0];
}

// When accessing underlying data after kernel execution, synchronization is required
void AmpExamples::SynchronizingUnderlyingData()
{
	cout << "\nUnderlying data synchronization.\n";

	const int			size = 100000;
	vector<int>			data(size, 5);
	array_view<int, 1>	view(size, data);

	accelerator_view av = accelerator().create_view();

	// Modify array_view on device
	parallel_for_each(av, view.extent, [=](index<1> idx) restrict(amp) {
		view[idx] = view[idx] + 2;
	});

	// Data is still as before the kernel
	// UNDEFINED BEHAVIOR!!!
	cout << "   After the kernel, data[0] = " << data[0] << endl;			// Not guaranteed and should not be done!

	//
	// A) Calling synchronize or synchronize_async on the array_view will copy the data from the device to the host. 
	//    synchronize_async will block current thread while waiting for synchronization to complete. 
	//
	view.synchronize();
	cout << "   After synchronization, data[0] = " << data[0] << endl;

	//
	//    Or, synchronize asynchronously
	//
	parallel_for_each(av, view.extent, [=](index<1> idx) restrict(amp) {
		view[idx] = view[idx] + 2;
	});

	completion_future future = view.synchronize_async();

	//    Do some other operation here

	//    Wait for synchronization to complete
	future.wait();
	cout << "   After asynch synchronization, data[0] = " << data[0] << endl;

	//
	// B) Accessing the array_view through indexing either as an L-value or R-value.
	//
	parallel_for_each(av, view.extent, [=](index<1> idx) restrict(amp) {
		view[idx] = view[idx] + 2;
	});

	index<1> idx(0);
	int x = view[idx];																	// Blocks until all data has been copied from the device
	cout << "   After accessing array_view, data[0] = " << data[0] << endl;

	//
	// C) Calling data() member function only available on one-dimensional array_views.
	//
	parallel_for_each(av, view.extent, [=](index<1> idx) restrict(amp) {
		view[idx] = view[idx] + 2;
	});

	int * p = view.data();
	cout << "   After array_view.data(), p[0] = " << p[0] << endl;

	//
	// D) A given data can be wrapped by multiple array_view objects. 
	//    When the last copy of array_view wrapping the underlying data is destroyed 
	//    (either when it goes out of scope or when the destructor is explicitly called), 
	//    data on the accelerator memory will be copied out to update the host data. 
	//    Relying on this is discouraged, since any exceptions thrown at the 
	//    synchronization point would not be observed by user code.
	//
}

//
// Demonstrates recommended explicit synchronization of data from the device to the CPU
//

template<typename T>
void AddVectors(const float* const a, const float* const b, float* result, int size)
{
	// Guideline A: Explicitly specify read-onliness by creating array_view<const>
	//				to ensure that the data is not copied from the device to the CPU

	//array_view<T, 1> va(size, a);					// Wrong
	array_view<const T, 1> va(size, a);				// OK

	//array_view<T, 1> vb(size, b);					// Wrong
	array_view<const T, 1> vb(size, b);				// OK

	array_view<T, 1> vr(size, result);				// OK: we'll be writing result here
	vr.discard_data();								//     do not copy data from CPU to the device

	parallel_for_each(vr.extent, [=](index<1> idx) restrict(amp) {
		vr[idx] = va[idx] + vb[idx];
	});

	// Guideline B: Explicitely synchronize.
	//				Implicit synchronization on destructions will swallow exceptions..
	vr.synchronize();

	// Return pointer to the underlying data
	result = vr.data();
}

void AmpExamples::SynchronizationBestPractices()
{
	cout << "\nSynchronization best practices.\n";

	float a[] = { 1, 2, 3, 4, 5 };
	float b[] = { 0, 6, 7, 8, 9 };
	
	const int size = ARRAYSIZE(a);

	float* r = new float[size];

	AddVectors<float>(a, b, r, size);

	for (int i = 0; i < size; ++i)
	{
		cout << "   r[" << i << "] : " << a[i] << " + " << b[i] << " = " << r[i] << endl;
	}

	delete r;
}

//
// Best practices for discard_data
//

template<typename F>
float ReduceRow(const array_view<const float>& matrix, const array_view<const float>& row, const F& func , int numColumns)
{
	vector<float>	  data(row.extent[0] / 2);
	array_view<float> view(data.size(), data);

	// Guideline A: Call discard_data on an array_view which will be used purely as an output
	//				to avoid unnecessary copying to the device
	view.discard_data();

	parallel_for_each(vew.extent, [=](index<1> idx) restrict(amp) {
		float a = matrix(row, idx[0]);
		float b = matrix(row, idx[0] + (numColumns / 2));

		view[idx] = func(a, b);
	});

	for (int stride = view.extent[0] / 2; stride > 0; stride /= 2)
	{
		parallel_for_each(extent<1>(stride), [=](index<1> idx) restrict(amp) {
			float a = view(idx);
			float b = view(idx[0] + stride);

			view[idx] = func(a, b);
		});
	}

	float result;
	copy(view.section(0, 1), &result);

	// Guideline B: Call discard_data on the temporary view before it goes out of scope
	//				to avoid the view from being synchronized to the CPU on desrtuction
	view.discard_data();

	return result;
}

// Algorithm will calculate sum of all floats in the matrix.
// Note that the algorithm is not the most efficient one and only serves to demonstrate sections.
// Demonstrates how to offset origin point in order to operate on a smaller section of data.
// Sample constraint is that width and height equal power of 2
void AmpExamples::Subsections()
{
	cout << "\nSubsection Example.\n";

	int width = 1024;														// Must be power of 2 for this sample

	vector<float> data(width * width, 1.f);

	// Wrap data
	array_view<float, 2> view(width, width, data);

	// Repeat reduction: works like creating mip-maps.
	// We are splitting matrix into quarters, then again a quarter into smaller quarters, and so on...

	time_point<system_clock> start = system_clock::now();

	while (width > 1)
	{
		width /= 2;

		concurrency::extent<2> ext(width, width);

		// Split data into 4 quarters and create array_view with offset to each quarter
		const array_view<const float, 2> s1 = 
			view.section(
				index<2>(0, 0),							// Origin
				ext										// Extent
			);

		const array_view<const float, 2> s2 = view.section(index<2>(width, 0), ext);
		const array_view<const float, 2> s3 = view.section(index<2>(0, width), ext);
		const array_view<const float, 2> s4 = view.section(index<2>(width, width), ext);

		array<float, 2> quarter(ext);

		// Execute kernel to accumulate all quarters into the first quarter
		parallel_for_each(ext, [=, &quarter](index<2> idx) restrict(amp) {
			quarter[idx] = s1[idx] + s2[idx] + s3[idx] + s4[idx];
		});

		// Set quarter array as input array for the next loop
		// NOTE: data is not synchronized from GPU to the host
		view = quarter;
	}

	time_point<system_clock> stop = system_clock::now();
	long long us = duration_cast<microseconds>(stop - start).count();
	long long ms = duration_cast<milliseconds>(stop - start).count();

	// Result should be 1048576: 1024 * 1024
	index<2> idx(0,0);
	cout.setf(ios::fixed, ios::floatfield);
	cout.setf(ios::showpoint);
	cout << "   Sum = " << setprecision(2) << view[idx] << endl;

	// ~60ms
	cout << "   Executed in: " << us << "us (" << ms << "ms)" << endl;
}

// Sometimes you might need to reshape data to adapt to an existing interface or to use data generated 
// by a factory library in your algorithms. For example, you might want to take advantage of a library 
// that only provides APIs for linear containers although your data sits in N-dimensional arrays or array_views.
//
// It is available for arrays of any rank, but only available for array_views of rank 1. 
// This is because contiguous memory is necessary for well-defined and efficient data reshaping and in C++ AMP 
// we only guarantee memory contiguity for arrays and the least-significant-dimension of array_views.
//
// Real world example would be to adapt 2D image data to a histogram calculator, which is usually a 1D algorithm.

void AmpExamples::RandomFill(array_view<float, 1> & view)
{
	parallel_for_each(view.extent, [=](index<1> idx) restrict(amp) {
		view[idx] = 1.f;
	});
}

void AmpExamples::ViewAs()
{
	cout << "\nview_as example.\n";

	const unsigned int width = 1920;
	const unsigned int height = 1080;

	concurrency::extent<2>	ext(width, height);
	array<float, 2>			image(ext);

	time_point<system_clock> start = system_clock::now();

	// Fill 
	array_view<float, 1> tmpView = image.view_as<1>(concurrency::extent<1>(image.extent.size()));
	RandomFill(tmpView);

	//tmpView.synchronize();
	float x = tmpView[0];

	time_point<system_clock> stop = system_clock::now();

	long long us = duration_cast<microseconds>(stop - start).count();
	cout << "   Executed in: " << us << "us" << endl;

	//cout << "   tmpView(0,0) = " << tmpView(0) << endl;

	array_view<float, 2> view(image);
	index<2> idx(10, 15);
	cout << "   image(10,15)   = " << view[idx] << endl;
}

// Just like the view_as function, reinterpret_as is only available for array_views of rank 1 
// and arrays of any rank, and it preserves the constness of the original container.
// In addition, the return type of reinterpret_as is always array_view of rank 1, because, 
// it disregards the shape of the data and works on the linearized or flattened data directly. 
// If you want to work with higher dimensional views of the data, you can always use the view_as operation 
// on top of the returned array_view to do that. Note that, the total element number of the array_view object 
// returned by reinterpret_as may be different from that of the original array or array_view object 
// if the sizes of the from and to element types are different.
//
// 

// Changes given float data to its absolute value
void AmpExamples::MyFabs(array_view<float, 1> & view)
{
	array_view<unsigned int, 1> tmp = view.reinterpret_as<unsigned int>();

	parallel_for_each(tmp.extent, [=](index<1> idx) restrict(amp) {
		tmp[idx] &= 0x7FFFFFFF;
	});
}

void AmpExamples::ReinterpretAs()
{
	cout << "\nreinterpret_as example.\n";

	const int size = 100;
	vector<float_2> position(size);
	array_view<float_2, 1> view(size, position);

	time_point<system_clock> start = system_clock::now();

	// Fill 
	array_view<float, 1> tmpView = view.reinterpret_as<float>();
	RandomFill(tmpView);

	tmpView.synchronize();

	time_point<system_clock> stop = system_clock::now();

	long long us = duration_cast<microseconds>(stop - start).count();
	cout << "   Executed in: " << us << "us" << endl;

	index<1> idx(12);
	cout << "   position[12]: x = " << position[12].x << "; y = " << position[12].y << endl;
	cout << "   view[12]:     x = " << view[idx].x    << "; y = " << view[idx].y    << endl;

	//
	// Another usage of reinterpret_as is to do bits manipulation.
	// MyFabs function changes given float data to its absolute value.
	//

	vector<float> data(size);

	generate(data.begin(), data.end(), []() { 
		return -(static_cast<float>(rand()) / RAND_MAX); 
	});

	start = system_clock::now();

	array_view<float, 1> view2(size, data);

	MyFabs(view2);

	view2.synchronize();

	stop = system_clock::now();

	us = duration_cast<microseconds>(stop - start).count();
	cout << "\n   fabs finished in: " << us << "us" << endl;

	for (int i = 0; i < 10; ++i)
	{
		cout << "      data[" << i << "] = " << data[i] << endl;
	}

	//
	// One thing to watch out for reinterpret_as is that the size of the reinterpreted element type
	// must evenly divide into the total size of the original array or array_view.
	//
	array<int_2> a(size);

	try
	{
		// Exception: Element type of reinterpret_as does not evenly divide into extent
		array_view<int_3> view3 = a.reinterpret_as<int_3>();
	}
	catch(const runtime_exception & ex)
	{
		cout << "\n   exception: " << ex.what() << endl;
	}
}

// A projection of an array or array_view of rank N is an array_view of rank N-1 
// consisting of all of the elements with the most significant index equal to the single integer value.  
// Since the result is an array_view, no data is copied.  For example, the result of projecting a 2D array 
// with integer i is the ith row (a 1D array_view). 
void AmpExamples::Projection()
{
	cout << "\nProjection: matrix x vector.\n";

	const int width = 1900;
	const int height = 1080;

	accelerator_view av = accelerator().create_view();

	// 6x6 matrix
	float m[] =
	{
		1, 2, 3, 4, 5, 6,
		1, 2, 3, 4, 5, 6,
		1, 2, 3, 4, 5, 6,
		1, 2, 3, 4, 5, 6,
		1, 2, 3, 4, 5, 6,
		1, 2, 3, 4, 5, 6,
	};

	array_view<const float, 2> matrix(6, 6, m);

	// 6-Vector
	float v[] = { 3, 1, 4, 5, 8, 7 };

	array_view<const float, 1> vec(6, v);

	// Result
	vector<float> r(6);
	array_view<float, 1> result(6, r);

	result.discard_data();						// Do not copy contents to the device

	time_point<system_clock> start = system_clock::now();

	parallel_for_each(av, vec.extent, [=](index<1> idx) restrict(amp) {
		int   x = idx[0];
		float z = 0.f;		// result

		for (int y = 0; y < vec.extent[0]; ++y)
		{
			z += matrix(x, y) * vec(y);
		}

		result[idx] = z;
	});

	result.synchronize();

	time_point<system_clock> stop = system_clock::now();
	long long us = duration_cast<microseconds>(stop - start).count();

	cout << "\n   finished in: " << us << "us" << endl;

	for (int i = 0; i < 6; ++i)
	{
		cout << "      r[" << i << "] = " << r[i] << endl;
	}

	//
	// Now use a projection
	//

	result.discard_data();

	start = system_clock::now();

	parallel_for_each(av, vec.extent, [=](index<1> idx) restrict(amp) {
		int   x = idx[0];
		float z = 0.f;									// result
		array_view<const float, 1> row = matrix[x];		// projection

		for (int y = 0; y < row.extent[0]; ++y)
		{
			z += row(y) * vec(y);
		}

		result[idx] = z;
	});

	result.synchronize();

	stop = system_clock::now();
	us = duration_cast<microseconds>(stop - start).count();

	cout << "\n   finished projection in: " << us << "us" << endl;

	for (int i = 0; i < 6; ++i)
	{
		cout << "      r[" << i << "] = " << r[i] << endl;
	}
}

// Suppose you want to count the number of threads that run into an exceptional occurrence of some kind. 
// This is similar to a reduction, but because there are presumably going to be very few occurrences, 
// it may be better to simply use the atomic increment function.
//
// NOTE: Note the use of an array_view to write the number of exceptional occurrences to memory.  
//		 You must use either an array or an array_view to communicate results to the CPU, 
//       even when the results consist of just one item like in the example above.
void AmpExamples::AtomicOperations()
{
	int numOccurences = 0;								// Counter of all occurences of something
	array_view<int, 1> total(1, &numOccurences);

	int p[] = { 1,2,3,4,5,6,7,8,9,10 };
	array_view<int, 1> view(ARRAYSIZE(p), p);

	parallel_for_each(view.extent, [=](index<1> idx) restrict(amp) {
		atomic_fetch_inc(&total(0));
	});

	total.synchronize();

	cout << "\nAtomic operation example.\n";

	cout << "   After atomic inc: " << numOccurences << endl;
}

// TDR or Timeout Detection and Recovery is a mechanism for maintaining GPU responsiveness.
// Windows will reset display driver when a task appears to be hanging or running longer 
// than the permitted quantum time (default is 2 seconds). The display driver is restarted 
// to free up the GPU for display and other waiting apps. Users will notice a momentary screen flicker 
// with a message in the task bar like “Display driver has stopped responding and has successfully recovered”.
//
// C++ AMP apps can observe a TDR during intensive computations which execute longer than the permitted quantum time. 
// All executing commands on the offending accelerator_view will be cancelled and any state stored on the accelerator_view 
// will be lost. Here are some examples of situations where TDR can occur:
//		a) The app has a parallel_for_each which takes longer than 2 seconds to complete
//		b) An irrecoverable out of memory exception is encountered during copying data to the accelerator.
//		c) The accelerator_view has queuing mode set to automatic and a batch of copy and parallel_for_each operations 
//		   are sent to the accelerator as a single DMA buffer. This batch of operations takes longer than 2 seconds to execute.
//		d) Another app causes a TDR which is broadcast to all apps using the same accelerator.
//		e) The device is physically removed from the system.
//
// The accelerator_view_exception can have a message like the ones shown below:
//		a) Failed to wait for D3D marker event. Error code: 887A0005
//		b) Failed to map staging buffer. Error code: 887A0005
//
// You can get additional diagnostics about the failure by compiling with the /MTD or /MDd compiler switches which causes 
// debug version of the AMP runtime to be used. Here are problem descriptions when runin debug:
//
//		TDR exception received: Failed to wait for D3D marker event.
//		ID3D11Device::RemoveDevice: Device removal has been triggered for the following reason
//		(DXGI_ERROR_DEVICE_HUNG: The Device took an unreasonable amount of time to execute its commands, 
//		or the hardware crashed/hung. As a result, the TDR (Timeout Detection and Recovery) mechanism has been triggered. 
//		The current Device Context was executing commands when the hang occurred. The application may want to respawn 
//		and fallback to less aggressive use of the display hardware).
//
// Here's another example where exception message was received during large copy operations. 
// Note that this TDR occurred because of an Out of Memory condition.
//
//		ID3D11Device::RemoveDevice: Device removal has been triggered for the following reason 
//		(E_OUTOFMEMORY: The application tried to use more adapter memory than the Device can simultaneously accommodate. 
//		As a result, the TDR (Timeout Detection and Recovery) mechanism has been triggered. The current Device Context 
//		was executing commands when exhaustion occurred. The application needs to make less aggressive use of the display memory, 
//		perhaps by leveraging ClearState to ensure large Resources do not stay bound to the pipeline, 
//		or by leveraging information, such as DXGI_ADAPTER_DESC::DedicatedVideoMemory)
//
//
// On my machine the setting is 7 seconds!
//
// PS C:\WINDOWS\system32> cd hklm:
/*
PS HKLM:\> dir

    Hive: HKEY_LOCAL_MACHINE

Name                           Property                                                                                                                                                 
----                           --------                                                                                                                                                 
BCD00000000                                                                                                                                                                             
HARDWARE                                                                                                                                                                                
SAM                                                                                                                                                                                     
dir : Requested registry access is not allowed.
At line:1 char:1
+ dir
+ ~~~
    + CategoryInfo          : PermissionDenied: (HKEY_LOCAL_MACHINE\SECURITY:String) [Get-ChildItem], SecurityException
    + FullyQualifiedErrorId : System.Security.SecurityException,Microsoft.PowerShell.Commands.GetChildItemCommand
 
SOFTWARE                                                                                                                                                                                
SYSTEM                                                                                                                                                                                  

PS HKLM:\> cd System\CurrentControlSet\Control\GraphicsDrivers

PS HKLM:\System\CurrentControlSet\Control\GraphicsDrivers> dir

    Hive: HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\GraphicsDrivers

Name                           Property                                                                                                                                                 
----                           --------                                                                                                                                                 
AdditionalModeLists                                                                                                                                                                     
Configuration                                                                                                                                                                           
Connectivity                                                                                                                                                                            
DCI                            Timeout : 7                                                                                                                                              
UseNewKey                                         
*/

// Long running kernel
void AmpExamples::LongRunningKernel(const vector<int>& data, vector<int>& result, queuing_mode queuingMode)
{
	accelerator_view av = accelerator().create_view(queuingMode);

	array<int> a(static_cast<int>(data.size()), data.begin(), av);

	parallel_for_each(a.extent, [&](index<1> idx) restrict(amp) {
		// Long-running operation
	});
	
	copy(a, result.begin());
}

void AmpExamples::TimoutException()
{
	cout << "\nTimeout Exception (TDR) example.\n";

	const int size = 10;
	vector<int> data(size, 5);
	vector<int> result(size);

	try
	{
		LongRunningKernel(data, result);
	}
	catch(accelerator_view_removed& ex)
	{
		cout << "   TDR exception: " << ex.what() << endl;
		cout << "   Error code:    " << ex.get_error_code() << endl;
		cout << "   Reson:         " << ex.get_view_removed_reason() << endl;
	}
}

// To recover and continue after the TDR, you have to create a new accelerator_view, reallocate memory resources and populate them afresh on the new accelerator_view.
// Note that if your app uses the default accelerator_view, it will not be able to recover unless you switch to using a different accelerator, or restart the app. 
// It is better to use a non-default accelerator view as it can be discarded and a new one created (still corresponding to the same accelerator).
// If your individual commands are not long running but collectively result in a long running execution, you can try again with queuing_mode set to immediate. 
// Flushing regularly or using immediate queuing mode can help avoid TDRs.

void AmpExamples::RecoverFromTDR()
{
	cout << "\nRecovering from TDR.\n";

	const int size = 10;
	vector<int> data(size, 5);
	vector<int> result(size);

	queuing_mode queuingMode = queuing_mode_automatic;

	for (int i = 1; i <= 2; ++i)
	{
		try
		{
			cout << "   Long Running Kernel, attempt # " << i << endl;
			LongRunningKernel(data, result, queuingMode);
			break;
		}
		catch(accelerator_view_removed& ex)
		{
			cout << "   TDR exception: " << ex.what() << endl;
			cout << "   Error code:    " << ex.get_error_code() << endl;
			cout << "   Reson:         " << ex.get_view_removed_reason() << endl << endl;
		}

		queuingMode = queuing_mode_immediate;
	}
}

// If your application needs to execute long running commands on the GPU, on Windows 8 you can create a Direct3D 11 device 
// with GPU timeout disabled using D3D11CreateDevice method, and subsequently create a C++ AMP accelerator_view using 
// C++ AMP DirectX interoperability API method concurrency::direct3d::create_accelerator_view. On accelerator_views 
// created through this mechanism, commands are allowed to execute beyond the TDR timeout limit as long as the OS 
// or other processes are not simultaneously contending for the same GPU accelerator.
//
// 1)	This feature is only available on Windows8.
// 2)	Disabling GPU timeout on devices prevents TDR occurrence only if the OS or other processes are not simultaneously contending 
//		for that GPU. If Windows detects contention for the GPU from the Desktop Windows manager or other processes, it will initiate TDR 
//		to reset the accelerator_view where a long running command is executing, irrespective of the disablement of GPU timeout on that device. 
//		Hence for this technique to be effective in preventing your long running C++ AMP computations from causing TDR, you must pick 
//		a dedicated GPU accelerator which is not connected to display and is neither concurrently used by other processes 
//		thus eliminating any chances of contention.
//
// Following is a code snippet illustrating creation of a C++ AMP accelerator_view which is not subject to TDR timeout.

void AmpExamples::DisableTDR()
{
	cout << "\nDisable TDR.\n";

	unsigned int flags = D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;			// DISABLE TDR!!!
	
#if _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ID3D11Device*			device  = nullptr;
	ID3D11DeviceContext*	context = nullptr;

	D3D_DRIVER_TYPE driverTypes[] = 
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	D3D_FEATURE_LEVEL		feature;

	// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476877(v=vs.85).aspx
	//IDXGIAdapter* adapter = nullptr;

	HRESULT hr = S_OK;

	for (UINT i = 0; i < ARRAYSIZE(driverTypes); ++i)
	{
		D3D_DRIVER_TYPE driverType = driverTypes[i];

		hr = D3D11CreateDevice(
			nullptr,						// dxgi adapter
			driverType,						// driver type
			nullptr,						// software rasterizer
			flags,							// flags
			featureLevels,					// feture levels
			ARRAYSIZE(featureLevels),		// feature levels
			D3D11_SDK_VERSION,				// sdk version
			&device,
			&feature,
			&context
			);

		if (SUCCEEDED(hr))
		{
			break;
		}
	}

	if (FAILED(hr) || 
		((feature != D3D_FEATURE_LEVEL_11_1) && (feature != D3D_FEATURE_LEVEL_11_0))
		)
	{
		cerr << "   Failed to create Direct3D 11 device." << endl;
		return;
	}

	// This accelerator_view will not time-out
	accelerator_view av = create_accelerator_view(device);

	wcout << "   -- Description:               " << av.accelerator.description << endl;
	wcout << "      Version:                   " << (av.version >> 16) << '.' << (av.version & 0xFFFF) << endl;
	wcout << "      Is debug:                  " << ((av.is_debug) ? "true" : "false") << endl;
	wcout << "      Queing mode:               " << ((av.queuing_mode == queuing_mode::queuing_mode_automatic) ? "automatic" : "immediate") << endl;
	wcout << endl;
}

// void direct3d_abort() restrict(amp)
// This function aborts the execution of a kernel. When the abort is detected by the runtime, 
// it raises a runtime_exception on the host with the error message, “Reference Rasterizer: Shader abort instruction hit”.
//
// D3D11 MESSAGE: Reference Rasterizer:    view[0,0] = 2 [ SHADER MESSAGE #2097410: SHADER_MESSAGE]
// D3D11 MESSAGE: Reference Rasterizer:    view[0,1] = 3 [ SHADER MESSAGE #2097410: SHADER_MESSAGE]
// D3D11 MESSAGE: Reference Rasterizer:    view[1,0] = 4 [ SHADER MESSAGE #2097410: SHADER_MESSAGE]
// D3D11 MESSAGE: Reference Rasterizer:    view[1,1] = 5 [ SHADER MESSAGE #2097410: SHADER_MESSAGE]
//
//
// void direct3d_printf(const char *_Format_string, …) restrict(amp)
// (Parameters)_Format_string: The format string; ...: An optional list of parameters of variable count.
// This function accepts a format string and an optional list of parameters of variable count. 
// It prints formatted output from a kernel to the Visual Studio output window.
//
// D3D11 ERROR: Reference Rasterizer:    errorf: av[idx] = 2 [ SHADER ERROR #2097411: SHADER_ERROR]
// D3D11 ERROR: Reference Rasterizer:    errorf: av[idx] = 3 [ SHADER ERROR #2097411: SHADER_ERROR]
// D3D11 ERROR: Reference Rasterizer:    errorf: av[idx] = 4 [ SHADER ERROR #2097411: SHADER_ERROR]
// D3D11 ERROR: Reference Rasterizer:    errorf: av[idx] = 5 [ SHADER ERROR #2097411: SHADER_ERROR]
//
//
// void direct3d_errorf(char *_Format_string, …) restrict(amp)
// This function has identical characteristics and usage to the direct3d_printf function, 
// in that a message is printed to the output window. Additionally the C++ AMP runtime 
// will raise a runtime_exception on the host with the same error message passed to the direct3d_errof call.
//
// D3D11 ERROR: Reference Rasterizer: Shader abort instruction hit at IP 462 [ EXECUTION ERROR #2097409: SHADER_ABORT]
// D3D11 ERROR: Reference Rasterizer: Shader abort instruction hit at IP 462 [ EXECUTION ERROR #2097409: SHADER_ABORT]
// D3D11 ERROR: Reference Rasterizer: Shader abort instruction hit at IP 462 [ EXECUTION ERROR #2097409: SHADER_ABORT]
// D3D11 ERROR: Reference Rasterizer: Shader abort instruction hit at IP 462 [ EXECUTION ERROR #2097409: SHADER_ABORT]
// //
void AmpExamples::DebugHelpers()
{
	cout << "\nDebugging Support in AMP.\n";

	const int width = 2;
	const int height = 2;
	const int size = width * height;

	vector<int> data(size);

	int i = 0;
	generate(data.begin(), data.end(), [&i]{ return ++i; });

	// In DEBUG mode with GPU ony selected, av will be ref!
	// In other build configurations, these helpers will be replaced with NOOP.
	//accelerator_view av = accelerator().create_view();
	accelerator_view av = accelerator(accelerator::direct3d_ref).default_view;

	wcout << L"\n   device: " << av.get_accelerator().description << endl;

	concurrency::extent<2> ext(width, height);
	array_view<int, 2> view(ext, data);

	// printf
	parallel_for_each(av, ext, [=](index<2> idx) restrict(amp) {
		view[idx]++;
		direct3d_printf("   view[%d,%d] = %d\n", idx[0], idx[1], view[idx]);				// Limit is 7 parameters, will throw exception in RELEASE
	});

	view.synchronize();

	// errorf
	try
	{
		parallel_for_each(av, ext, [=](index<2> idx) restrict(amp) {
			direct3d_errorf("   errorf: av[idx] = %d\n", view[idx]);
			view[idx] *= 10;
		});

		view.synchronize();
	}
	catch(runtime_exception& e)
	{
		cout << "\n   errorf caused runtime exception: " << e.what() << endl;
	}

	// abort
	try
	{
		parallel_for_each(av, ext, [=](index<2> idx) restrict(amp) {
			view[idx] *= 10;
			direct3d_abort();							// This will terminate the program when debugging in GPU only mode
		});

		view.synchronize();
	}
	catch(runtime_exception& e)
	{
		cout << "\n   aborted: " << e.what() << endl;
	}
}

//
// Measuring kernel performance
//

void AmpExamples::RandomFill(vector<float>& data, float min, float max)
{
	mt19937 mersenne;
	uniform_real_distribution<float> distribution(min, max);

	for (size_t i = 0; i < data.size(); ++i)
	{
		data[i] = distribution(mersenne);
	}
}

void AmpExamples::MultiplyMatrices(accelerator_view& view, array<float, 2>& result, const array<float, 2>& a, const array<float, 2>& b)
{
	parallel_for_each(view, result.extent, [&](index<2> idx) restrict(amp) {
		float r = 0;

		for (int i = 0; i < a.extent[1]; ++i)
		{
			index<2> x(idx[0], i);
			index<2> y(i, idx[1]);

			r += a(x) * b(y);
		}

		result(idx) = r;
	});
}

/*
Measuring Performance 1. (This is in release mode.)

   device: NVIDIA Quadro 5000M

   0 executed in  22006us ( 22ms)
   1 executed in  21002us ( 21ms)
   2 executed in  33978us ( 33ms)
   3 executed in  50009us ( 50ms)
   4 executed in  72046us ( 72ms)
   5 executed in  99018us ( 99ms)
   6 executed in 145017us (145ms)
   7 executed in 193025us (193ms)
   8 executed in 229014us (229ms)
   9 executed in 280023us (280ms)
*/
void AmpExamples::MeasurePerformance1()
{
	cout << "\nMeasuring Performance 1.\n\n";

	time_point<system_clock> start = system_clock::now();
	time_point<system_clock> stop = system_clock::now();

	accelerator_view device = accelerator().default_view;
	wcout << L"   device: " << device.accelerator.description << endl << endl;

	// 10 samples increasing amount of data by i in each loop
	for (int i = 0; i < 10; ++i)
	{
		// NUmber of rows and columns for both matrices
		const int r1 = 300 + 100 * i;
		const int c1 = 500 + 100 * i;
		const int r2 = c1;
		const int c2 = 400 + 100 * i;

		assert(c1 == r2);					// columns in m1 == rows in m2

		vector<float> va(r1 * c1);
		vector<float> vb(r2 * c2);
		vector<float> vc(r1 * c2);			// resultant matrix

		RandomFill(va);
		RandomFill(vb);

		// Start timer
		start = system_clock::now();

		concurrency::extent<2> ea(r1, c1);
		concurrency::extent<2> eb(r2, c2);
		concurrency::extent<2> ec(r1, c2);

		// Using arrays only to measure performance. If using arrqay_view, we have 
		// to manually force synchronization because parallel_for_each is async.
		// When parallel_for_each returns, then computation is only scheduled on the device. 
		// To force execution you need to call wait() on accelerator_view. 
		array<float, 2> a(ea);
		array<float, 2> b(eb);
		array<float, 2> c(ec);

		// Copy underlying data to the device
		copy(va.begin(), a);
		copy(vb.begin(), b);

		// Run kernel
		MultiplyMatrices(device, c, a, b);

		// Copy data back to the host
		copy(c, vc.begin());

		// Stop timer
		stop = system_clock::now();

		long long ms = duration_cast<milliseconds>(stop - start).count();
		long long us = duration_cast<microseconds>(stop - start).count();
		cout << "   " << i << " executed in " << us << "us (" << ms << "ms)" <<  endl;
	}
}

// Here we will measure performance of copying data to and from the device

void AmpExamples::WarmUp(accelerator_view& view)
{
	concurrency::extent<2> ext(1, 1);

	array<float, 2> a(ext);
	array<float, 2> b(ext);
	array<float, 2> c(ext);

	MultiplyMatrices(view, c, a, b);
}

/*
Measuring Performance 2. (In release mode)

   device: NVIDIA Quadro 5000M

   0 executed in  66991us ( 66ms) : copy-in    0us, kernel  66033us, copy 2  958us
   1 executed in  19012us ( 19ms) : copy-in  994us, kernel  17023us, copy 2  994us
   2 executed in  30010us ( 30ms) : copy-in    0us, kernel  29039us, copy 2  970us
   3 executed in  48035us ( 48ms) : copy-in 1003us, kernel  45059us, copy 2 1972us
   4 executed in  68055us ( 68ms) : copy-in  996us, kernel  66019us, copy 2 1039us
   5 executed in  96026us ( 96ms) : copy-in  999us, kernel  93051us, copy 2 1975us
   6 executed in 140032us (140ms) : copy-in 2008us, kernel 136021us, copy 2 2002us
   7 executed in 192019us (192ms) : copy-in 1997us, kernel 187037us, copy 2 2984us
   8 executed in 230028us (230ms) : copy-in 2996us, kernel 224043us, copy 2 2989us
   9 executed in 281015us (281ms) : copy-in 3000us, kernel 275015us, copy 2 2998us

Measuring Performance 2. (One more run)

   device: NVIDIA Quadro 5000M

   0 executed in  10997us ( 10ms) : copy-in  975us, kernel   9048us, copy 2  973us
   1 executed in  18967us ( 18ms) : copy-in 1003us, kernel  16987us, copy 2  976us
   2 executed in  31041us ( 31ms) : copy-in  999us, kernel  29009us, copy 2 1033us
   3 executed in  48008us ( 48ms) : copy-in    0us, kernel  45996us, copy 2 2011us
   4 executed in  68004us ( 68ms) : copy-in    0us, kernel  67023us, copy 2  980us
   5 executed in  96010us ( 96ms) : copy-in 1000us, kernel  92998us, copy 2 2011us
   6 executed in 140985us (140ms) : copy-in 2002us, kernel 137009us, copy 2 1973us
   7 executed in 194028us (194ms) : copy-in 2003us, kernel 190041us, copy 2 1984us
   8 executed in 228027us (228ms) : copy-in 3025us, kernel 223019us, copy 2 1983us
   9 executed in 280027us (280ms) : copy-in 2996us, kernel 274053us, copy 2 2977us
*/
void AmpExamples::MeasurePerformance2()
{
	cout << "\nMeasuring Performance 2.\n\n";

	time_point<system_clock> start = system_clock::now();
	time_point<system_clock> stop = system_clock::now();
	time_point<system_clock> tmStart = system_clock::now();

	accelerator_view device = accelerator().default_view;
	wcout << L"   device: " << device.accelerator.description << endl << endl;

	WarmUp(device);

	// 10 samples increasing amount of data by i in each loop
	for (int i = 0; i < 10; ++i)
	{
		// NUmber of rows and columns for both matrices
		const int r1 = 300 + 100 * i;
		const int c1 = 500 + 100 * i;
		const int r2 = c1;
		const int c2 = 400 + 100 * i;

		assert(c1 == r2);					// columns in m1 == rows in m2

		vector<float> va(r1 * c1);
		vector<float> vb(r2 * c2);
		vector<float> vc(r1 * c2);			// resultant matrix

		RandomFill(va);
		RandomFill(vb);

		concurrency::extent<2> ea(r1, c1);
		concurrency::extent<2> eb(r2, c2);
		concurrency::extent<2> ec(r1, c2);

		// Using arrays only to measure performance. If using arrqay_view, we have 
		// to manually force synchronization because parallel_for_each is async.
		// When parallel_for_each returns, then computation is only scheduled on the device. 
		// To force execution you need to call wait() on accelerator_view. 
		array<float, 2> a(ea);
		array<float, 2> b(eb);
		array<float, 2> c(ec);

		// Copy underlying data to the device
		tmStart = system_clock::now();
		start = system_clock::now();

		copy(va.begin(), a);
		copy(vb.begin(), b);

		stop = system_clock::now();
		long long tmCopy1 = duration_cast<microseconds>(stop - start).count();

		// Run kernel
		start = system_clock::now();

		MultiplyMatrices(device, c, a, b);

		device.wait();					// Ensure that kernel completed execution!!!

		stop = system_clock::now();
		long long tmKernel = duration_cast<microseconds>(stop - start).count();

		// Copy data back to the host
		start = system_clock::now();

		copy(c, vc.begin());

		stop = system_clock::now();
		long long tmCopy2 = duration_cast<microseconds>(stop - start).count();

		long long ms = duration_cast<milliseconds>(stop - tmStart).count();
		long long us = duration_cast<microseconds>(stop - tmStart).count();
		cout << "   " << i << " executed in " << us << "us (" << ms << "ms)" 
			 << " : copy-in " << tmCopy1 
			 << "us, kernel " << tmKernel
			 << "us, copy 2 " << tmCopy2 
			 << "us" <<  endl;
	}
}
