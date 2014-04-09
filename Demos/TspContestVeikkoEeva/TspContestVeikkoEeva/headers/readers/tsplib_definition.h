/***********************************************************************************************************
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

#pragma once

#include "matrix_utilities.h"
#include "problem_definition.h"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>


namespace tsp
{	
	enum class instance_type
	{
		none	= 0,
		tsp		= 1,
		atsp	= 2,
		sop		= 3,
		hcp		= 4,
		cvr		= 5,
		tour	= 6
	};
			

	enum class edge_weight_type
	{
		none	= 0,
		expl	= 1,
		euc_2d	= 2,
		euc_3d	= 3,
		max_2d	= 4,
		max_3d	= 5,
		man_2d	= 6,
		man_3d	= 7,
		ceil_2d	= 8,
		geo		= 9,
		att		= 10,
		xray1	= 11,
		xray2	= 12,
		special	= 13
	};
			

	enum class edge_weight_format_type
	{
		none			= 0,
		function		= 1,
		full_matrix		= 2,
		upper_row		= 3,
		lower_row		= 4,
		upper_diag_row	= 5,
		lower_diag_row	= 6,
		upper_col		= 7,
		lower_col		= 8,
		upper_diag_col	= 9,
		lower_diag_col	= 10
	};
				

	enum class edge_data_format_type
	{
		none		= 0,
		edge_list	= 1,
		adj_list	= 2
	};
	

	enum class node_coord_type
	{
		none			= 0,
		two_coords		= 1,
		three_coords	= 2,
		no_coords		= 3,
		default_value	= no_coords
	};
	

	enum class display_data_type
	{
		none			= 0,
		coord_display	= 1,
		twod_display	= 2,
		no_display		= 3,
		default_value	= no_display
	};


	template<typename numeric_type>
	class tsplib_definition: public problem_definition<numeric_type>
	{
		public:
			tsplib_definition():
				dimension_(0),
				shortest_known_tour_((std::numeric_limits<long int>::max)()),
				instance_type_(tsp::instance_type::none),
				edge_weight_type_(tsp::edge_weight_type::none),
				edge_weight_format_type_(tsp::edge_weight_format_type::none),
				edge_data_format_type_(tsp::edge_data_format_type::none),
				node_coord_type_(tsp::node_coord_type::none),
				display_data_type_(tsp::display_data_type::none)
			{
				static_assert(std::is_floating_point<numeric_type>::value == true, "The template \"numeric_type\" needs to be a floating point type.");
			};

			virtual ~tsplib_definition() {};
						
			void set_origin_file(std::string const& file) { origin_file_ = file; };
			std::string get_origin_file() const { return origin_file_; };

			void set_shortest_known_tour(unsigned long int shortest_known_tour) { shortest_known_tour_ = shortest_known_tour; }
			unsigned long int get_shortest_known_tour() const { return shortest_known_tour_; }
			
			//void add_element(T element) { elements_.push_back(element); };
			//const std::vector<T> get_elements() const { return elements_; };

			void set_comment(std::string const& comment) { comment_ = comment; };
			std::string get_comment() const { return comment_; };

			void set_dimension(int dimension) { dimension_ = dimension; };
			int get_dimension() const { return dimension_; };

			void set_name(std::string const& name) { name_ = name; };
			std::string get_name() const { return name_; };

			void set_instance_type(instance_type type) { instance_type_ = type; };
			instance_type get_instance_type() const { return instance_type_; };

			void set_edge_weight_type(edge_weight_type edge_weight_type) { edge_weight_type_ = edge_weight_type; };
			edge_weight_type get_edge_weight_type() const { return edge_weight_type_; };
		
			void set_edge_weight_format_type(edge_weight_format_type edge_weight_format_type) { edge_weight_format_type_ = edge_weight_format_type; };
			edge_weight_format_type get_edge_weight_format_type() const { return edge_weight_format_type_; };

			void set_edge_data_format_type(edge_data_format_type edge_data_format_type) { edge_data_format_type_ = edge_data_format_type; };
			edge_data_format_type get_edge_data_format_type() const { return edge_data_format_type_; };
			
			void set_node_coord_type(node_coord_type node_coord_type) { node_coord_type_ = node_coord_type; };
			node_coord_type get_node_coord_type() const { return node_coord_type_; };

			void set_display_data_type(display_data_type display_data_type) { display_data_type_ = display_data_type; };
			display_data_type get_display_data_type() const { return display_data_type_; };
						
		private:
			std::string origin_file_;
			std::string comment_;
			int dimension_;
			long int shortest_known_tour_;
			std::string name_;
			tsp::instance_type instance_type_;
			tsp::edge_weight_type edge_weight_type_;
			tsp::edge_weight_format_type edge_weight_format_type_;
			tsp::edge_data_format_type edge_data_format_type_;
			tsp::node_coord_type node_coord_type_;
			tsp::display_data_type display_data_type_;
			//std::vector<T> elements_;
	};
		
	//TODO: These could be in an .incl file or somesuch.
	namespace
	{
		namespace
		{
			//This is a utility function to convert between strongly typed enums and integer values.
			template<typename E>
			typename std::underlying_type<E>::type underlying_type(E e)
			{
				return static_cast<typename std::underlying_type<E>::type>(e);
			}
			
			//This type will hold strings matching each of the enumerations with their respective specializations.
			//If a suitable conversion isn't found, a compile-time error will be issued.
			template<typename T>
			struct enum_strings
			{
				static char const* data[];
			};

			//These arrays provide a charater array presentation of their respective enumerations.
			//They must enumerated in a sequence matching the enumerations in order to make the enum and string values match.
			template<> char const* enum_strings<tsp::instance_type>::data[] = { "N/A", "TSP", "ATSP", "SOP", "HCP", "CVR" };
			template<> char const* enum_strings<tsp::edge_weight_type>::data[] = { "N/A", "EXPLICIT", "EUC_2D", "EUC_3D", "MAX_2D", "MAX_3D", "MAN_2D", "MAN_3D", "CEIL_2D", "GEO", "ATT", "XRAY1", "XRAY2", "SPECIAL" };
			template<> char const* enum_strings<tsp::edge_weight_format_type>::data[] = { "N/A", "FUNCTION","FULL_MATRIX", "UPPER_ROW", "LOWER_ROW", "UPPER_DIAG_ROW", "LOWER_DIAG_ROW", "UPPER_COL", "LOWER_COL", "UPPER_DIAG_COL", "LOWER_DIAG_ROW" };  
			template<> char const* enum_strings<tsp::edge_data_format_type>::data[] = {"N/A", "EDGE_LIST", "ADJ_LIST" };
			template<> char const* enum_strings<tsp::node_coord_type>::data[] = { "N/A", "TWO_COORDS", "THREE_COORDS", "NO_COORDS" };
			template<> char const* enum_strings<tsp::display_data_type>::data[] = { "N/A", "COORD_DISPLAY", "TWOD_DISPLAY", "NO_DISPLAY", "NO_DISPLAY" };


			//TODO: Pointers etc. should be checked properly here.
			template<typename T>
			int nint(T const& value)
			{
				return static_cast<int>(value + 0.5);
			}


			template<typename T>
			T euclidean_2d_distance(T const& dx, T const& dy)
			{
				return static_cast<T>(nint(std::sqrt(dx * dx + dy * dy)));
			}


			template<typename T>
			T euclidean_2d_distance_ceil(T const& dx, T const& dy)
			{
				return static_cast<T>(std::ceil(std::sqrt(dx * dx + dy * dy)));
			}


			template<typename T>
			T euclidean_pseudo_distance(T const& dx, T const& dy)
			{				
				const T rij = static_cast<T>(std::sqrt((dx * dx + dy * dy) / 10.0));
				const int tij = nint(rij);
				const T dij = static_cast<T>(tij < rij ? tij + 1 : tij); 
				
				return dij;
			}
		}
			

		template<typename T>
		T to_enum(std::string const& value)
		{
			const static auto begin = std::begin(enum_strings<T>::data);
			const static auto end = std::end(enum_strings<T>::data);
			const auto find = std::find(begin, end, value);
			if(find != end)
			{   
				return static_cast<T>(std::distance(begin, find));
			}

			//TODO: Perhaps this should throw instead.
			return static_cast<T>(0);
		}


		template<typename T>
		std::string to_string(T const& token)
		{
			return enum_strings<T>::data[underlying_type(token)];
		}

		
		std::vector<float> transform_to_row_major_matrix(std::vector<std::vector<float>> const& matrix_to_transform, int dimension)
		{
			std::vector<float> transformed_matrix(dimension * dimension);
			for(int i = 0; i < dimension; ++i)
			{
				for(int j = 0; j < dimension; ++j)
				{
					transformed_matrix[i * dimension + j] = matrix_to_transform[j][i];
				}
			}

			return transformed_matrix;
		}
						
		
		//TODO: This should be refactored to separate functions.
		//TODO: It would be rather much smarter to wrap this to a proper matrix abstraction, save space when accessing symmetric matrix etc.
		template<typename T>
		std::vector<T> provide_cost_matrix(std::unique_ptr<tsp::tsplib_definition<T>> const& problem)
		{	
			auto const edge_weight_type = problem->get_edge_weight_type();
			assert(
				edge_weight_type == tsp::edge_weight_type::euc_2d || 
				edge_weight_type == tsp::edge_weight_type::ceil_2d ||
				edge_weight_type == tsp::edge_weight_type::att ||
				edge_weight_type == tsp::edge_weight_type::expl);

			const int dimension = problem->get_dimension();
			std::vector<T> cost_matrix(dimension * dimension);
			if(edge_weight_type == tsp::edge_weight_type::euc_2d ||
				edge_weight_type == tsp::edge_weight_type::ceil_2d ||
				edge_weight_type == tsp::edge_weight_type::att)
			{				
				auto const& elements = problem->get_elements();
				
				for(int i = 0; i < dimension * 2; i += 2)
				{
					for(int j = i; j < dimension * 2; j += 2)
					{
						const T dx = elements[i] - elements[j];
						const T dy = elements[i + 1] - elements[j + 1];
						T cost(0);
						if(edge_weight_type == tsp::edge_weight_type::euc_2d)
						{
							cost = euclidean_2d_distance(dx, dy);
						}
						else if(edge_weight_type == tsp::edge_weight_type::ceil_2d)
						{
							cost = euclidean_2d_distance_ceil(dx, dy);
						}
						else if(edge_weight_type == tsp::edge_weight_type::att)
						{
							cost = euclidean_pseudo_distance(dx, dy);
						}
												
						//Here a symmetric matrix is filled. This ought to be refactored (better cache locality, less space, more vectorizing),
						//but later, because in the contest 2D matrices are used (by default, but it'd be an optimization point)
						//and there's some serious time pressure...
						cost_matrix[utilities::to_row_major_index_from_column_major_indices(i / 2, j / 2, dimension)] = cost;
						cost_matrix[utilities::to_row_major_index_from_column_major_indices(j / 2, i / 2, dimension)] = cost;
					}
				}	
			}
			else if(problem->get_edge_weight_type() == tsp::edge_weight_type::expl)
			{
				if(problem->get_edge_weight_format_type() == tsp::edge_weight_format_type::full_matrix)
				{
					cost_matrix = problem->get_elements();
				}
			}
			
			return cost_matrix;
		}
	}
}
