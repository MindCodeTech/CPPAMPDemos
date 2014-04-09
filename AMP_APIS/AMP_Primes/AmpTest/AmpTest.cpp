// AmpTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include "stdafx.h"
#include "amp.h"

using namespace std;
using namespace concurrency;

void inspect_accelerators()
{
    auto accelerators = accelerator::get_all();
    for_each(begin(accelerators), end(accelerators),[=](accelerator acc){ 
        wcout << "New accelerator: " << acc.description << '\n';
        wcout << "is_debug = " << acc.is_debug << '\n';
        wcout << "is_emulated = " << acc.is_emulated << '\n';
        wcout << "dedicated_memory = " << acc.dedicated_memory << '\n';
		wcout << "double precision: " << acc.supports_limited_double_precision << '\n';
        wcout << "device_path = " << acc.device_path << '\n';
        wcout << "has_display = " << acc.has_display << '\n';                
        wcout << "version = " << (acc.version >> 16) << '.' << (acc.version & 0xFFFF) << '\n';
        wcout << endl;
    });
}



int wmain(int argc, wchar_t* argv[])
{
	inspect_accelerators();

	return 0;
}

