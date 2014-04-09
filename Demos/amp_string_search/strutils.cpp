//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corp. 
//
// File: strutils.cpp
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

#include "strutils.h"

using namespace std;

mt19937& get_rand_generator()
{
    // Mersenne twister engine for random numbers
    static mt19937 gen;
    return gen;
}

// Emplace randomly generated string at destination.
void generate_string(uint length, char *dest, uint num_null_char)
{
    assert(dest != nullptr);
    assert(length > num_null_char);

    uniform_int_distribution<> distribution('a', 'z');
    for (uint i = 0; i < length - num_null_char; ++i)
    {
        dest[i] = distribution(get_rand_generator());
    }
    for (uint i = 0; i < num_null_char; ++i)
    {
        dest[length - 1 - i] = '\0'; 
    }
}

// Generate 'count' strings, each 'length' long. 
// Length includes 'num_null_char' count of null terminating characters.
void generate_strings(char *output, uint count, uint length, uint num_null_char)
{
    assert(output != nullptr);
    assert(count > 0);
    assert(length > num_null_char);

    for (uint i = 0; i < count; ++i)
    {
        generate_string(length, &output[i * length], num_null_char);
    }
}

// Inject patterns into records. Variable 'matching_level' controls the probability of pattern being copied to record.
void apply_patterns_to_records(uint num_patterns, uint pattern_length, const char *patterns, 
                               uint num_records, uint record_length, char *records, uint matching_level, uint num_null_char)
{
    assert(patterns != nullptr);
    assert(records != nullptr);
    assert(matching_level >= 0);
    assert(matching_level <= 100);
    assert(num_patterns > 0);
    assert(num_records > 0);
    assert(record_length > 0);
    assert(pattern_length > num_null_char);

    uniform_int_distribution<> match(0, 100);
    uniform_int_distribution<> pattern_distribution(0, num_patterns - 1);
    uniform_int_distribution<> offset_distribution(0, record_length - pattern_length);

    for (uint i = 0; i < num_records; ++i)
    {
        uint rand = match(get_rand_generator());
        if (rand <= matching_level)
        {
            // Pick random pattern
            int pattern_number = pattern_distribution(get_rand_generator());

            // Pick random offset for pattern within record length
            int offset = offset_distribution(get_rand_generator());

            const char *ptr_pattern = &patterns[pattern_number * pattern_length];

            // Copy pattern over to record at offset (excluding null terminating character)
            copy(ptr_pattern, ptr_pattern + pattern_length - num_null_char, &records[i * record_length + offset]);
        }
    }
}