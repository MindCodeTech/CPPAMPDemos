//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corp. 
//
// File: ampstrstr.cpp
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
#include <amp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "strutils.h"
#include "timer.h"

using namespace concurrency;
using std::cout;
using std::endl;
using std::vector;

// --------------------------------------------------------------------------------------------------------
// CPU string search.
// Given a set of strings called records and a set of strings called patterns, 
// the algorithm checks which records contain which patterns.
// The output array 'matches' contains 'num_records' * 'num_patterns' results.
// First 'num_patterns' results belongs to 1st record, 
// next 'num_patterns' results belong to 2nd record and so on.
// The algorithm writes -1 if pattern was not found within record,
// otherwise the offset within record at which first match was found.
// --------------------------------------------------------------------------------------------------------
// Parameters:
//   num_patterns   - total number of patterns
//   pattern_length - length of individual pattern (including any null terminating characters)
//   patterns       - pointer to the array of patterns
//   num_records    - total number of records
//   record_length  - length of individual record (including any null terminating characters)
//   records        - pointer to the array of records
//   matches        - the output array of size (num_records * num_patterns)
void cpu_string_search(uint num_patterns, uint pattern_length, const char *patterns, 
                       uint num_records, uint record_length, const char *records, int *matches)
{
    // Loop over all records
    for (uint record_number = 0; record_number < num_records; ++record_number)
    {
        // Loop over all patterns
        for (uint pattern_number = 0; pattern_number < num_patterns; ++pattern_number)
        {
            // Execute string search
            const char *ptr_record = &records[record_number * record_length];
            const char *ptr_match = std::strstr(ptr_record, &patterns[pattern_number * pattern_length]);

            // If pattern was found, then calculate offset, otherwise result is -1
            if (ptr_match)
            {
                matches[record_number * num_patterns + pattern_number] = static_cast<int>(std::distance(ptr_record, ptr_match));
            }
            else
            {
                matches[record_number * num_patterns + pattern_number] = -1;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------
// Prepares the container for the cpu and executes cpu algorithm.
// --------------------------------------------------------------------------------------------------------
// Parameters:
//   num_patterns    - number of patterns inside the patterns array
//   pattern_length  - number of characters inside each pattern (including any null terminating characters)
//   patterns        - the pointer to the array of patterns
//   num_records     - number of records inside the records array
//   record_length   - number of characters inside each record (including any null terminating characters)
//   records         - the pointer to the array of records
//   time            - output parameter, it will contain the number of milliseconds for CPU computation
vector<int> benchmark_cpu_string_search(uint num_patterns, uint pattern_length, const char *patterns, 
                                        uint num_records, uint record_length, const char *records, double &time)
{
    // Prepare the container for the results
    vector<int> matches(num_records * num_patterns);

    // First run is always warmup, we print the result for second run
    for(uint run = 0; run < 2; ++run)
    {
        Timer t_total;
        t_total.Start();

        // Run the cpu algorithm
        cpu_string_search(num_patterns, pattern_length, patterns, num_records, record_length, records, matches.data());

        t_total.Stop();

        if (run > 0)
        {
            time = t_total.Elapsed();
        }
    }

    return matches;
}

template<typename type, uint num_elements>
struct constant_memory_wrapper
{
    type data[num_elements];
};

// These numbers are used by rolling hash algorithm.
// For more details see: http://en.wikipedia.org/wiki/Rolling_hash
static const int B = 17; // magic base number (small prime)
static const int M = 24036583; // magic number for modulo (large prime)

template<typename T>
T modulo(T a, T b) restrict(cpu,amp)
{
    // Handles negative numbers
    return (a % b + b) % b;
}

template<typename T>
T power_modulo(T a, T b)
{
    T result = 1;
    for (T i = 0; i<b; ++i)
    {
        result = modulo(result * a, M);
    }
    return result;
}

// --------------------------------------------------------------------------------------------------------
// Computes rolling hash. Uses modulo arithmetic to fit into int storage.
// --------------------------------------------------------------------------------------------------------
// Parameters:
//   hash - previous hash value
//   in   - new character to be added to rolling hash
//   out  - value to be removed from rolling hash
//   E    - value by which 'out' was multiplied inside 'hash' value
int rehash(int hash, uint in, uint out, int E) restrict(amp)
{
    hash = modulo(hash - modulo(static_cast<int>(out) * E, M), M);
    hash = modulo(hash * B, M);
    hash = modulo(hash + static_cast<int>(in), M);
    return hash;
}

// --------------------------------------------------------------------------------------------------------
// Computes 'num_elements' hashes of 'input' and stores them in 'dest'.
// Each hash is 'length' long and the data inside 'input' is 'data_length' long.
// --------------------------------------------------------------------------------------------------------
// Parameters:
//   dest          - the output array for hashes
//   num_elements  - number of hashes to be computed from the input
//   input         - the pointer to the array from which we compute the hash
//   length        - the length of hash to be computed
//   data_length   - the size of data inside input array
void hash_array(int *dest, uint num_elements, const char *input, uint length, uint data_length)
{
    for (uint i = 0; i < num_elements; ++i)
    {
        int hash = 0;
        for (uint j = 0; j < length; ++j)
        {
            hash = modulo(hash * B, M);
            hash = modulo(hash + input[i * data_length + j], M);
        }
        dest[i] = hash;
    }
}

// --------------------------------------------------------------------------------------------------------
// Check if hash computed on the portion of the record matches any pattern hashes.
// The loop is fully unrolled at compile time.
// --------------------------------------------------------------------------------------------------------
// Template parameters:
//   num_patterns   - number of pattern hashes inside 'pattern_hashes' array
// Parameters:
//   i              - index of the loop
//   hash           - the current hash of the record
//   pattern_hashes - the pointer to the array of pattern hashes
//   match          - output flag set to 1 if any pattern hash matches current record hash, otherwise not set
template <uint num_patterns>
void hash_check_unroller(int i, int hash, const int *pattern_hashes, uint &match) restrict(amp)
{
    hash_check_unroller<1>(i, hash, pattern_hashes, match);
    hash_check_unroller<num_patterns - 1>(i + 1, hash, pattern_hashes, match);
}

// Leaf for template meta-programming
template<>
void hash_check_unroller<1>(int i, int hash, const int *pattern_hashes, uint &match) restrict(amp)
{
    if (hash == pattern_hashes[i])
    {
        match = 1;
    }
}

// --------------------------------------------------------------------------------------------------------
// C++ AMP string search.
// Algorithm computes the hash for each pattern, then these hashes along with all records
// are implicitly copied to accelerator's memory via array_view.
// Next, Rabin-Karp algorithm is used on the accelerator.
// The C++ AMP computation checks if record matches any pattern,
// the output container from C++ AMP computation contains 1 if record matches any pattern or 0 if it does not match.
// For those records that match some pattern execute multicore std::strstr to 
// gather the information which exact patterns matched.
// Notice that even if C++ AMP computation would give us the results for individual patterns,
// the algorithm would still need verify the results as hash-based comparision might give false-positives.
// --------------------------------------------------------------------------------------------------------
// Template parameters:
//   num_patterns       - number of patterns inside the patterns array
// Parameters:
//   pattern_length     - number of characters inside each pattern (including any null terminating characters)
//   host_patterns      - the reference to the vector of patterns
//   num_records        - number of records inside the records array
//   record_length      - number of characters inside each record (including any null terminating characters)
//   host_records       - the reference to the staging array with records
//   num_null_char      - number of null terminating characters
//   host_matches       - the container for the results
//   pattern_hashes     - the container for pattern hashes
//   records            - the array_view on top of records
//   matches_any        - the array_view for results in C++ AMP computation
//   target_view        - accelerator_view of target accelerator
template<uint num_patterns>
void amp_string_search(uint pattern_length, const vector<char> &host_patterns, uint num_records, uint record_length, const array<uint> &host_records, uint num_null_char, vector<int> &host_matches,
                       constant_memory_wrapper<int, num_patterns> &pattern_hashes, array_view<const uint> &records, array_view<uint> &matches_any, accelerator_view &target_view)
{
    // Compute hashes for the patterns (exclude null terminating characters from hash value)
    hash_array(&pattern_hashes.data[0], num_patterns, host_patterns.data(), pattern_length - num_null_char, pattern_length);

    // Compute the constant value for rolling hash (it is the value by which output character is multiplied before it is removed from the hash)
    const uint E = power_modulo(B, static_cast<int>(pattern_length - num_null_char - 1));
    
    // No need to copy-in the content of matches_any to accelerator's memory
    matches_any.discard_data();

    parallel_for_each(extent<1>(num_records), [=](index<1> idx) restrict(amp) {

        uint match = 0;

        // Calculate the hash for first pattern_length characters of the record (exclude null terminating characters)
        int hash = 0;
        for (uint i = 0; i<(pattern_length - num_null_char) / 4; ++i)
        {
            uint val = records[idx[0] * record_length / 4 + i];
            for (int shift = 0; shift < 4; ++shift)
            {
                // This assumes little-endian
                uint in_char = (val >> (8 * shift)) & 0xFFU;
                hash = modulo(hash * B, M);
                hash = modulo(hash + static_cast<int>(in_char), M);
            }
        }

        // Check if that first hash matches to any pattern
        hash_check_unroller<num_patterns>(0, hash, &pattern_hashes.data[0], match);

        // Load new character, rehash, check against patterns
        for (uint i = (pattern_length - num_null_char) / 4; i< (record_length - num_null_char) / 4; ++i)
        {
            uint in_val = records[idx[0] * record_length / 4 + i];
            uint out_val = records[idx[0] * record_length / 4 + i - (pattern_length - num_null_char) / 4];

            for (int shift = 0; shift < 4; ++shift)
            {
                // This assumes little-endian
                uint in_char = (in_val >> (8 * shift)) & 0xFFU;
                uint out_char = (out_val >> (8 * shift)) & 0xFFU; 
                hash = rehash(hash, in_char, out_char, E);

                hash_check_unroller<num_patterns>(0, hash, &pattern_hashes.data[0], match);
            }
        }

        matches_any[idx] = match;
    });
    matches_any.synchronize();

    fill(host_matches.begin(), host_matches.end(), -1);
    const char *ptr_records = reinterpret_cast<const char *>(host_records.data());
    
    // Run multicore string search on those records that has some matches
    parallel_for(0, static_cast<int>(num_records), 1, [&](int i)
    {
        if (matches_any[i])
        {        
            for (int j = 0; j<num_patterns; ++j)
            {
                const char *ptr_record = &ptr_records[i * record_length];
                const char *ptr_match = strstr(ptr_record, &host_patterns[j * pattern_length]);
                if (ptr_match)
                {
                    host_matches[i * num_patterns + j] = static_cast<int>(std::distance(ptr_record, ptr_match));
                }
                else
                {
                    host_matches[i * num_patterns + j] = -1;
                }
            }
        }
    });
}

// --------------------------------------------------------------------------------------------------------
// Prepares the containers for C++ AMP and executes C++ AMP algorithm,
// function returns vector container with results for each pattern against all records.
// --------------------------------------------------------------------------------------------------------
// Template parameters:
//   num_patterns       - number of patterns inside the patterns array
// Parameters:
//   pattern_length     - number of characters inside each pattern (including any null terminating characters)
//   host_patterns      - the reference to the vector of patterns
//   num_records        - number of records inside the records array
//   record_length      - number of characters inside each record (including any null terminating characters)
//   host_records       - the reference to the staging array with records
//   num_null_char      - number of null terminating characters
//   target_view        - accelerator_view of target accelerator
//   time               - output parameter, it will contain number of milliseconds that it took for entire C++ AMP computation
template<uint num_patterns>
vector<int> benchmark_amp_string_search(uint pattern_length, const vector<char> &host_patterns, uint num_records, uint record_length, const array<uint> &host_records,
                                        uint num_null_char, accelerator_view &target_view, double &time)
{
    // Container to store the final results
    vector<int> host_matches(num_records * num_patterns);

    // Create container for pattern hashes
    constant_memory_wrapper<int, num_patterns> pattern_hashes;

    // Create array_view on top of records to make them accessible on the accelerator
    array_view<const uint> records(host_records);

    // Container to store the results from C++ AMP computation and corresponding array_view
    vector<uint> host_matches_any(num_records);
    array_view<uint> matches_any(extent<1>(num_records), host_matches_any);

    // First run is always warmup, we print the result for second run
    for(uint run = 0; run < 2; ++run)
    {
        Timer t_total;
        t_total.Start();
        // Run the C++ AMP algorithm
        amp_string_search<num_patterns>(pattern_length, host_patterns, num_records, record_length, host_records, num_null_char, host_matches, 
                                        pattern_hashes, records, matches_any, target_view);
        t_total.Stop();

        if (run > 0)
        {
            time = t_total.Elapsed();
        }
    }
    return host_matches;
}

template<typename type>
void compare_results(const type &a, const type &b, const char *cmp_identifier)
{
    assert(a.size() == b.size());

    int num_matches = 0;
    int num_errors = 0;
    for (uint i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            cout << "Incorrect result (" << cmp_identifier << "): [" << i << "] " << a[i] << " != " << b[i] << endl;
            if (++num_errors >= 10)
            {
                break;
            }
        }
        else if (a[i] >= 0)
        {
            ++num_matches;
        }
    }
    
    if (!num_errors)
    {
        cout << "Results matched, total number of patterns found: " << num_matches << endl;
    }
}

// --------------------------------------------------------------------------------------------------------
// Runs single benchmark for given patterns and records
// --------------------------------------------------------------------------------------------------------
// Template parameters:
//   num_patterns       - number of patterns inside the patterns array
// Parameters:
//   pattern_length     - number of characters inside each pattern (including any null terminating characters)
//   patterns           - the reference to the vector of patterns
//   num_records        - number of records inside the records array
//   record_length      - number of characters inside each record (including any null terminating characters)
//   records            - the reference to the staging array with records
//   num_null_char      - number of null terminating characters
//   target_view        - accelerator_view of target accelerator 
template<uint num_patterns>
void benchmark(uint pattern_length, vector<char> &patterns, uint num_records, uint record_length, array<uint> &records,
               uint num_null_char, accelerator_view &target_view)
{
    cout << "Size of records: " << num_records * record_length / (1024 * 1024) << "MB" 
         << ", size of patterns: " << num_patterns * pattern_length / (1024) << "KB" << endl;

    double cpu_time, amp_time;
    cout << "Running C++ AMP algorithm..." << std::flush;
    auto amp_results = benchmark_amp_string_search<num_patterns>(pattern_length, patterns, num_records, record_length, records,
                                                                 num_null_char, target_view, amp_time);
    cout << " done!" << endl;
    cout << "C++ AMP string search took: " << amp_time << " ms" << endl;

    cout << "Running CPU algorithm..." << std::flush;
    auto cpu_results = benchmark_cpu_string_search(num_patterns, pattern_length, patterns.data(), num_records, record_length,
                                                   reinterpret_cast<const char*>(records.data()), cpu_time);
    cout << " done!" << endl;
    cout << "CPU string search took: " << cpu_time << " ms" << endl;

    compare_results(cpu_results, amp_results, "CPU vs C++ AMP");
    cout << "Speedup (CPU/C++ AMP): " << std::setprecision(4) << cpu_time/amp_time << "X" << endl;
    cout << endl;
}

void default_benchmark()
{
    const uint num_patterns = 128; // number of patterns to search (this value has to be known at compile time)
    const uint pattern_length = 64; // length of each pattern
    const uint record_length = pattern_length * 16; // length of each record
    const uint matching_level = 1; // probability (0-100) at which a pattern is present in a record
    const uint num_null_char = 4; // the number of null terminating characters after each pattern and each record

    // Algorithm packs chars into unsigned ints, therefore the length has to be divisble by 4.
    // Note that you can always add padding to your data to meet this requirement.
    static_assert(pattern_length % 4 == 0, "Pattern length has to be divisible by 4");
    static_assert(record_length % 4 == 0, "Record length has to be divisible by 4");

    // Print the description of default accelerator
    accelerator_view target_view = accelerator().create_view();
    std::wcout << target_view.accelerator.description << endl;

    // Generate search patterns
    vector<char> patterns(num_patterns * pattern_length);
    generate_strings(patterns.data(), num_patterns, pattern_length);

    // Run the algorithms few times with increasing number of records
    for (uint num_records = 16 * 1024; num_records < 256 * 1024; num_records *= 2)
    {
        // Use staging array for the records (characters are packed into uints)
        array<uint> records(extent<1>(num_records * record_length / 4), accelerator(accelerator::cpu_accelerator).default_view, target_view);
        generate_strings(reinterpret_cast<char*>(records.data()), num_records, record_length);
        // Inject some patterns into some records
        apply_patterns_to_records(num_patterns, pattern_length, patterns.data(), num_records, record_length, reinterpret_cast<char*>(records.data()), matching_level);

        // Execute CPU and C++ AMP algorithms
        benchmark<num_patterns>(pattern_length, patterns, num_records, record_length, records, num_null_char, target_view);
    }
}

// --------------------------------------------------------------------------------------------------------
// Parses input file and writes the content to 'dest' buffer
// --------------------------------------------------------------------------------------------------------
// Parameters:
//   path_to_file  - path to input file
//   dest          - a pointer to destination buffer, has to be num * length * sizeof(char) large
//   num           - number of strings to read
//   length        - length of each string including null terminating characters
//   num_null_char - number of null terminating characters to append
void parse_input_file(char *path_to_file, char *dest, uint num, uint length, uint num_null_char)
{
    assert(path_to_file != nullptr);
    std::stringstream msg;

    std::ifstream file_stream(path_to_file);
    assert(file_stream.good());
    std::fill(dest, dest + num * length, '\0');

    // Ignore header (first line inside file)
    file_stream.ignore(256, '\n');

    std::string line;
    uint offset = 0;
    while(file_stream >> line)
    {
        assert(line.length() == length - num_null_char); // unexpected length 
        assert(offset <= (num - 1) * length); // too many strings inside file
        std::copy(line.begin(), line.end(), dest + offset);
        offset += length;
    }
    file_stream.close();

    // Check if we read expected amount of data
    assert(offset == num * length);
}

// --------------------------------------------------------------------------------------------------------
// Parses input file for number of strings and their length
// --------------------------------------------------------------------------------------------------------
// Parameters:
//   path_to_file  - path to input file
//   num           - output parameter, it will contain number of strings inside input file
//   length        - output parameter, it will contain length of each string inside input file
void parse_file_header(char *path_to_file, uint &num, uint &length)
{
    std::ifstream file_stream(path_to_file);
    assert(file_stream.good());
    std::string dummy;
    file_stream >> dummy >> num >> dummy >> length;
    assert(num > 0);
    assert(length > 0);
    file_stream.close();
}

void benchmark_on_data_from_files(char *path_to_patterns, char *path_to_records)
{
    uint num_records;
    uint record_length;
    const uint num_patterns = 128; // this has to be a compile time constant
    uint num_patterns_temp;
    uint pattern_length;
    const uint num_null_char = 4; // number of null terminating characters to be inserted after each pattern and each record

    // Extract input sizes from file headers
    parse_file_header(path_to_records, num_records, record_length);
    parse_file_header(path_to_patterns, num_patterns_temp, pattern_length);
    
    assert(num_patterns_temp == num_patterns);

    // Adjust lengths (include number null terminating characters)
    record_length += num_null_char;
    pattern_length += num_null_char;

    // Print the description of default accelerator
    accelerator_view target_view = accelerator().create_view();
    std::wcout << target_view.accelerator.description << endl;

    // Read patterns from the file to memory
    vector<char> patterns(num_patterns * pattern_length);
    parse_input_file(path_to_patterns, patterns.data(), num_patterns, pattern_length, num_null_char);

    // Read records from the file to memory
    array<uint> records(extent<1>(num_records * record_length / 4), accelerator(accelerator::cpu_accelerator).default_view, target_view);
    parse_input_file(path_to_records, reinterpret_cast<char*>(records.data()), num_records, record_length, num_null_char);

    // Run benchmark
    benchmark<num_patterns>(pattern_length, patterns, num_records, record_length, records, num_null_char, target_view);
}

enum class action_type
{
    default_benchmark,
    use_input_files,
    invalid
};

action_type parse_command_line(int argc, char **argv)
{
    switch(argc)
    {
        case 1:
            return action_type::default_benchmark;
        case 3:
            return action_type::use_input_files;
        default:
            return action_type::invalid;
    }
}

void print_usage(char *program_name)
{
    assert(program_name != nullptr);
    cout << "Runs C++ AMP string search sample" << endl << endl;
    cout << "- Program accepts two input parameters. File path to patterns and file path to records." << endl;
    cout << "- When no parameters are passed program generates string patterns and string records"
         << " then executes CPU and C++ AMP algorithms on various sizes of records." << endl << endl;
    cout << "Examples:" << endl;
    cout << program_name << endl;
    cout << program_name << " patterns.txt records.txt" << endl << endl;
}

int main(int argc, char **argv)
{
    try
    {
        auto action = parse_command_line(argc, argv);

        switch(action)
        {
            case action_type::default_benchmark:
                default_benchmark();
                break;
            case action_type::use_input_files:
                benchmark_on_data_from_files(argv[1], argv[2]);
                break;
            default:
                print_usage(argv[0]);
        }
    } 
    catch(const std::exception &e)
    {
        cout << e.what() << endl;
    }
    system("pause");
}