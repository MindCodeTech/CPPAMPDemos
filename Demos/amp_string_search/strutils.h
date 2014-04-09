//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corp. 
//
// File: strutils.h
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

#include <random>
#include <cassert>
#include <cstring>
#include <vector>

typedef unsigned int uint;

void generate_strings(char *output, uint count, uint length, uint num_null_char = 4);

void apply_patterns_to_records(uint num_patterns, uint pattern_length, const char *patterns, 
                               uint num_records, uint record_length, char *records, uint matching_level, uint num_null_char = 4);