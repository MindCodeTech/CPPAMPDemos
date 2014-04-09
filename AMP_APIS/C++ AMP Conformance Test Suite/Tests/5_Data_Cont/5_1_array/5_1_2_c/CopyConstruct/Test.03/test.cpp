// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
// INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.
/// <tags>P1</tags>
/// <summary>move constructor is not invoked when const src is used</summary>

#include "./../../constructor.h"
#include <amptest_main.h>

template<typename _type, int _rank>
bool test_feature()
{
    const int rank = _rank;

    int *edata = new int[rank];
    for (int i = 0; i < rank; i++)
        edata[i] = 3;
    extent<rank> e1(edata);

    {
        const array<_type, rank> src(e1);

        array<_type, rank> dst = std::move(src);

        if (dst.get_extent() != e1)
        {
            return false;
        }

        // since move src is const, src should not be modified
        if (src.get_extent() != e1)
        {
            return false;
        }
    }

    return true;
}

runall_result test_main()
{
	accelerator::set_default(require_device().get_device_path());

	runall_result result;

	result &= REPORT_RESULT((test_feature<int, 1>()));
	result &= REPORT_RESULT((test_feature<int, 2>()));
	result &= REPORT_RESULT((test_feature<int, 5>()));
    result &= REPORT_RESULT((test_feature<float, 1>()));
	result &= REPORT_RESULT((test_feature<float, 2>()));
	result &= REPORT_RESULT((test_feature<float, 5>()));
    
    return result;
}

