 /**********************************************************************************************************
 * Copyright (c) 2012 Veikko Eeva <veikko@... see LinkedIn etc.>										   *
 *																										   *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software		   *
 * and associated documentation files (the "Software"), to deal in the Software without restriction,	   *
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,			   *
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is		   *
 * furnished to do so, subject to the following conditions:												   *
 * 																										   *
 * The above copyright notice and this permission notice shall be included in all copies or				   *
 * substantial portions of the Software.																   *
 *																										   *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT	   *
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, *
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE	   *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.												   *
 *																										   *
 ***********************************************************************************************************/
/***********************************************************************************************************
 * This code was written to participate to a competition organised by Beyond3D forums and supported by     *
 * AMD and Microsoft. See more at http://www.beyond3d.com/content/articles/121/. My sincere thanks         *
 * for the inducement to learn many new things.                                                            *
 *                                                                                                         * 
 ***********************************************************************************************************/

#include "construction_heuristics.h"
#include "kernel_context.h"
#include "kernel_utilities.h"
#include "iterator_overloads.h"
#include "opt2_move.h"
#include "opt3kernel.h"
#include "opt4kernel.h"
#include "path_pruner.h"
#include "stopwatch.h"
#include "testing.h"
#include "tsplib_definition.h"
#include "tsplib_definition_builder.h"
#include "utilities.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <iterator>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

using kernel::opt2_move;
using kernel::double_bridge;
using kernel::solution_cost;
using timers::stopwatch;
using timers::to_seconds;
using utilities::generate_path_symbols;
using utilities::get_accelerators;

using std::bad_alloc;
using std::begin;
using std::end;
using std::endl;
using std::exception;
using std::fixed;
using std::istream;
using std::make_pair;
using std::min_element;
using std::pair;
using std::reverse;
using std::rotate;
using std::setprecision;
using std::swap;
using std::string;
using std::tie;
using std::tuple;
using std::vector;
using std::wcerr;
using std::wcout;
using std::wostringstream;
using std::wstring;

//TODO: Note that SAL 2 annotations and Microsoft commenting or Doxygen should
//be used throughout the code (excluding other matters to make the code
//"production ready".
tuple<vector<int>, float, double> tsp_solver(vector<float> const& cost_matrix, int dimension, bool is_symmetric, unsigned int max_rounds_multiplier, std::chrono::duration<long long> maximum_solving_time_in_seconds = std::chrono::duration<long long>(1040));
wstring tour_completed_information(vector<int>& path, float path_cost, double calculation_time_in_seconds, float known_path_optimum);


vector<tuple<long long, unsigned int, string>> collect_input_parameters(istream& stream)
{
	long long max_tsp_duration = 0;
	unsigned int max_rounds_multiplier = 0;
	string problem_instance_file;
	vector<tuple<long long, unsigned int, string>> parameters;
	while(stream >> max_tsp_duration >> max_rounds_multiplier >> problem_instance_file)
	{
		parameters.push_back(make_tuple(max_tsp_duration, max_rounds_multiplier, problem_instance_file));
	}	

	return parameters;
}


int main(int argc, char* argv[]) 
{
	try
	{
		//The numeric type that will be used when parsing TSPLib files.
		typedef float tsp_numeric_type;
	
		auto problem_parameters = collect_input_parameters(std::cin);
		
		//vector<tuple<long long, unsigned int, string>> problem_parameters;
	   	//problem_parameters.push_back(std::make_tuple(10, 20, "../ALL_tsp/bays29.tsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_tsp/att48.tsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_tsp/berlin52.tsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_tsp/att532.tsp"));		
		//problem_parameters.push_back(std::make_tuple(15, 20, "../ALL_tsp/kroA200.tsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_tsp/d657.tsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_tsp/dsj1000.tsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_tsp/pcb1173.tsp"));
		//problem_parameters.push_back(std::make_tuple(120, 20, "../ALL_tsp/d2103.tsp"));
		//problem_parameters.push_back(std::make_tuple(120, 20, "../ALL_tsp/fl3795.tsp"));
		//problem_parameters.push_back(std::make_tuple(120, 20, "../ALL_tsp/fnl4461.tsp"));
		//problem_parameters.push_back(std::make_tuple(200, 20, "../ALL_tsp/pla7397.tsp"));
		//problem_parameters.push_back(std::make_tuple(15, 20, "../ALL_tsp/usa13509.tsp"));		
		//problem_parameters.push_back(std::make_tuple(15, 20, "../ALL_atsp/ftv33.atsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_atsp/ftv70.atsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_atsp/kro124p.atsp"));
		//problem_parameters.push_back(std::make_tuple(20, 40, "../ALL_atsp/ftv170.atsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_atsp/rbg323.atsp"));
		//problem_parameters.push_back(std::make_tuple(40, 20, "../ALL_atsp/rbg358.atsp"));
		//problem_parameters.push_back(std::make_tuple(60, 20, "../ALL_atsp/rbg403.atsp"));
		//problem_parameters.push_back(std::make_tuple(60, 20, "../ALL_atsp/rbg443.atsp"));
				
		for(const auto& problem_parameter: problem_parameters)
		{
			try
			{
				string problem_file;
				long long maximum_processing_time = 0;
				unsigned int max_rounds_multiplier = 0;

				tie(maximum_processing_time, max_rounds_multiplier, problem_file) = problem_parameter;

				wcout << "Processing file: " << wstring(problem_file.begin(), problem_file.end()) << "..." << endl;
				auto problem = tsp::build_instance<tsp_numeric_type>(problem_file);
		
				const int dimension = problem->get_dimension();
				wcout << "Building a square matrix of dimension " << dimension << "...\n" << endl;
				auto weight_matrix = tsp::provide_cost_matrix<tsp_numeric_type>(problem);
		
				wcout << "Starting solver... " << endl;
				vector<int> path;
				float path_cost;
				double time_in_seconds;
				tie(path, path_cost, time_in_seconds) = tsp_solver(weight_matrix, dimension, problem->get_instance_type() == tsp::instance_type::tsp ? true : false, max_rounds_multiplier, std::chrono::duration<long long>(maximum_processing_time));
				wcout << tour_completed_information(path, path_cost, time_in_seconds, static_cast<float>(problem->get_shortest_known_tour())); 
				wcout << endl;

				assert(test::no_duplicates(path));
			}
			catch(bad_alloc const& ex)
			{
				wcerr << ex.what() << endl;
			}
		}
	}
	catch(exception const& ex)
	{
		wcerr << ex.what() << endl;
		return EXIT_FAILURE;
	}
	catch(...)
	{
		wcerr << L"An unexpected error, exiting..." << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


std::tuple<std::vector<int>, float, double> tsp_solver(std::vector<float> const& cost_matrix, int dimension, bool is_symmetric, unsigned int max_rounds_multiplier, std::chrono::duration<long long> maximum_solving_time_in_seconds)
{
	timers::stopwatch solver_timer;
	
	//For discussion what this means, see opt4kernel.h.
	//It looks like on longer paths require less kicks on the perturbation
	//scheme used, in which random swings on the length of the whole path aren't
	//always the best option. This is black magic...
	const int DOUBLE_BRIDGE_KICKS = dimension > 4000 ? 1 : 2;
	auto accelerators = get_accelerators();
	
	for(auto const& accelerator: accelerators)
	{
		wcout << L"Accelerators: " << accelerator.description << endl;
	}
	
	//const std::chrono::system_clock::time_point solver_time_limit = std::chrono::system_clock::now() + std::chrono::duration<long long>(360);
	//wcout << L"\tSolver hard time limit set to " << 360 << " seconds" << endl;
	const std::chrono::system_clock::time_point solver_time_limit = std::chrono::system_clock::now() + maximum_solving_time_in_seconds;
	wcout << L"\tSolver hard time limit set to " << to_seconds(maximum_solving_time_in_seconds) << " seconds (see input switches)" << endl;

	//This is the path information that is being used in indexing and tracked.
	//It's taken to a staging array directly to save time.
	auto path_staging_array = utilities::generate_staged_path_symbols<int>(accelerators[0], dimension);

	//This buffer is the one that will be used on the GPU.
	concurrency::array<int, 1> gpu_path(dimension, accelerators[0].default_view);
	auto context = kernel::kernel_context<float>(cost_matrix, dimension, is_symmetric, accelerators[0]);
	
	//Initial tour length, the one before using some fast tour construction heuristic.
	float shortest_path_cost = solution_cost<float>(cost_matrix, path_staging_array);
	wcout << L"\tInitial tour length: " << setprecision(3) << fixed << shortest_path_cost << endl;
	
	//Where should this timer be set?
	solver_timer.start();
	auto heuristic_path = kernel::nearest_neighbor(cost_matrix, path_staging_array);
	float heuritic_shortest_path_cost = solution_cost<float>(cost_matrix, heuristic_path);
	if(heuritic_shortest_path_cost < shortest_path_cost)
	{
		shortest_path_cost = heuritic_shortest_path_cost;
		concurrency::copy(begin(heuristic_path), end(heuristic_path), path_staging_array);
	}
		
	wcout << L"\tTour length after construction heuristic: " << setprecision(3) << fixed << shortest_path_cost << endl;
	auto shortest_path = path_staging_array;

	//This will check that already checked paths will not be re-checked.
	path_pruner path_pruner;
	
	//Number of failures, zeroed upon succesful path shortening.
	//int failure_count = 0;	
	
	//Perturbations done in escaping local minima in an attempt towards global minima. Could be used
	//as stopping condition.
	unsigned long int perturbations = 0;
	
	//For statistical purposes, the number of 2-opt rounds calculated.
	unsigned long int opt2_rounds = 0;
	
	//TODO: This could be made into the acceptance criteria functio. See TODO below.
	//http://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
	float accumulator = is_symmetric ? 10.0f : 300.0f;
	static const float alpha = 1.0f/1000.0f;
	static const float window_component = 1.0f - alpha;
	try
	{		
		//This is the maximum number of allowed failures without improvement in tour length.
		//As there's a random element in the double bridge (4-opt move) and to some extent the results
		//are dependent on initial route selection, this should be "sufficiently large" depending
		//on the expected problem domain (STSP/ASTP and weights). Here it's just fixed as in the contest only
		//solution quality counts if it's faster than produced by others. These "magic numbers" are
		//completely shaken from a stetson.
		//const int MAX_FAILURES = (std::min)(dimension * 20u, 100000u);
		//const int MAX_FAILURES = (std::min)(dimension * max_rounds_multiplier, 100000u);
		
		//This is a crude hack that really helps in some asymmetric cases.
		//Though done on CPU and computationally heavy, so doing in a background thread
		//repeatedly would be better... These "magic numbers" are completely shaken from a hat.
		if(!is_symmetric && dimension > 70 && dimension < 512)
		{
			kernel::reduced_opt3kernel_path_conditioner<float>(cost_matrix, path_staging_array);
			reverse(begin(path_staging_array), end(path_staging_array));
			kernel::reduced_opt3kernel_path_conditioner<float>(cost_matrix, path_staging_array);
		}		
		
		//TODO: The acceptance condition here could be a function object supplied as a parameter. In that way,
		//it would easier to make, say, even a target cost as a acceptance criterion if such is known.
		while(accumulator > 0.1f && std::chrono::system_clock::now() < solver_time_limit)
		//while(failure_count < MAX_FAILURES && std::chrono::system_clock::now() < solver_time_limit)
		{
			//NOTE: The timer limit will not be checked in this loop on purpose.
			opt2_move opt2_result;
			do
			{
				if(path_pruner.try_add(path_staging_array))
				{	
					//accelerators[0].default_view.wait();
					concurrency::copy(path_staging_array, gpu_path);
					opt2_result = context.opt2kernel_calculation(path_staging_array, gpu_path);
					if(opt2_result.minchange_ < 0)
					{
						if(opt2_result.i_ > opt2_result.j_)
						{	
							swap(opt2_result.i_, opt2_result.j_);
						}
												
						reverse(begin(path_staging_array) + opt2_result.i_, begin(path_staging_array) + opt2_result.j_);
					}
					++opt2_rounds;
				}
				else
				{						
					break;
				}
			} while(opt2_result.minchange_ < 0);
						
			//TODO: This should be calculable based on edge swap information (what was removed, what was added).
			//For asymmetric case rather straightforward already (just taking care the only the new all-time-best
			//solution is subtrated)? Needs to be worked out...
			const float current_cost = solution_cost<float>(cost_matrix, path_staging_array);
			
			const float cost_difference = shortest_path_cost - current_cost;
			accumulator = (alpha * (cost_difference < 0.0f ? 0 : cost_difference)) + window_component * accumulator;
			if(current_cost < shortest_path_cost)
			{					
				shortest_path_cost = current_cost;
				path_staging_array.copy_to(shortest_path);
				//failure_count = 0;
				
				wcout << L"\tA new optimum found: " << setprecision(3) << fixed << shortest_path_cost 
					<< L" (" << to_seconds(solver_timer.elapsed()) << L" s)" << endl;
			}
			else
			{
				//++failure_count;
				
				//The best known solution is basis for the next round too, with some
				//perturbation moves to break the possible local optimum. There's some
				//discussion regarding tour choice with regard to local optimum at
				//http://scim.brad.ac.uk/staff/pdf/picowlin/CowlingKeuthen2004.pdf
				//on page seven.
				shortest_path.copy_to(path_staging_array);
			}
			
			//kernel::random_4opt2(path, DOUBLE_BRIDGE_KICKS);
			//kernel::random_4opt(cost_matrix, path);
			//double_bridge(path, DOUBLE_BRIDGE_KICKS);
			double_bridge(path_staging_array, DOUBLE_BRIDGE_KICKS);
			++perturbations;
		}
		solver_timer.stop();
	} 
	catch(exception const& ex)
	{
		wcout << ex.what() << endl;
		throw;
	}
	
	wcout << "Number of 2-opt rounds: " << opt2_rounds << endl;
	wcout << "Number of perturbations: " << perturbations << endl;
	wcout << "Number of rejected paths: " << path_pruner.rejected_paths() << endl;
	wcout << "Number of approved paths: " << path_pruner.approved_paths() << endl;
	
	return make_tuple(vector<int>(begin(path_staging_array), end(path_staging_array)), shortest_path_cost, to_seconds(solver_timer.elapsed()));
}


wstring tour_completed_information(vector<int>& path, float path_cost, double calculation_time_in_seconds, float known_path_optimum)
{
	//TODO: collecting tour printing information and modifying the path with rotate is bad form. Better would be to give
	//the rotated path as a parameter...

	wostringstream print_stream;

	//TODO: This -1 stuff would need to be refactored.
	print_stream << L"Time elapsed (s): " << calculation_time_in_seconds << endl;		
	print_stream << L"Calculated length: " << setprecision(3) << fixed << path_cost << endl;
	print_stream << L"Path cost: " << setprecision(3) << fixed << path_cost;
	if(known_path_optimum > -1.0f)
	{
		print_stream << L" (" << setprecision(3) << fixed << path_cost * 100.f / known_path_optimum  << L" % of known optimum " << known_path_optimum << L")";
	}
	print_stream << endl;
	 
	//The tour is assumed to printed in a TSPLib compatible form, so the smallest element will be rotated
	//to first, one (1) will be added to every element to modify zero based tour indexing and finally the
	//last element will be connected to the first one (here "-1" will not be printed).
	rotate(path.begin(), min_element(path.begin(), path.end()), path.end());
	for(const auto& output: path)
	{
		print_stream << output + 1 << "-";
	}
	print_stream << path[0] + 1;
	print_stream << L"\n" << endl;

	return print_stream.str();
}
