// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
// INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.
/// <tags>P1</tags>
/// <summary>Negative tests for tile() member function on incompatible extent (different rank) for 3d tiled_extent</summary>
//#Expects: Error: error C2338

#include "./../../tile.h"

template<typename _type>
bool test_tile()
{
    return test_tile_3d_negative_incorrect_template_param<_type, 2, 2, 2>();
}

runall_result test_main() 
{
	bool result = test_tile<extent<4>>() && test_tile<extent<1>>() && test_tile<extent<2>>();
	return runall_fail;
}
