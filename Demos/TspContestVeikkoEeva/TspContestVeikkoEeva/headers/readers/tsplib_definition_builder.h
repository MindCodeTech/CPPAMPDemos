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

#include "line.h"
#include "istream_range.h"
#include "string_utilities.h"
#include "tour_cost_provider.h"
#include "tsplib_definition.h"

#include <array>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>


#include <algorithm>
#include <iostream>

namespace tsp
{
	using tsp::istream_range;
	using tsp::tsplib_definition;
	using utilities::line;
	using utilities::split;
	using utilities::string_split_options;
	using utilities::trim;
	using utilities::trim_left;

	using std::accumulate;
	using std::array;
	using std::find;
	using std::ifstream;
	using std::string;
	using std::unique_ptr;
	using std::vector;
			
	namespace
	{
		//TODO: Change these to template typedefs when the compiler supports. One interesting approach is also available
		//at http://cpp-next.com/archive/2012/09/unifying-generic-functions-and-function-objects/.
		template<typename T>
		struct command_map_type
		{
			typedef std::unordered_map<std::string, std::function<void(std::string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill)>> type;
		}; 
		        
		template<typename T>
		struct data_section_command_type
		{
			typedef std::array<typename command_map_type<T>::type::const_iterator, 7> type;
		};
				
		//TODO: Here should be selected stof or stod or stoi or something else depending on template type.		
		template<typename T>
		typename command_map_type<T>::type initialize_commands()
		{
			static typename command_map_type<T>::type commands;

			commands["EOF"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { };
			commands["NAME"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_name(token); };
			commands["COMMENT"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill)
			{ 
				auto temp_comment = tsp_instance_to_fill->get_comment();
				temp_comment += token;
				tsp_instance_to_fill->set_comment(temp_comment); 
			};
			commands["DIMENSION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_dimension(stoi(token)); };
			commands["TYPE"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_instance_type(to_enum<tsp::instance_type>(token));	};
			commands["EDGE_WEIGHT_TYPE"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_edge_weight_type(to_enum<tsp::edge_weight_type>(token)); };
			commands["EDGE_WEIGHT_FORMAT"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_edge_weight_format_type(to_enum<tsp::edge_weight_format_type>(token)); };
			commands["EDGE_DATA_FORMAT"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_edge_data_format_type(to_enum<tsp::edge_data_format_type>(token)); };
			commands["DISPLAY_DATA_TYPE"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_display_data_type(to_enum<tsp::display_data_type>(token)); };
			commands["NODE_COORD_TYPE"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->set_node_coord_type(to_enum<tsp::node_coord_type>(token)); };

			//After these splits should by space?
			commands["DEPOT_SECTION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { };
			commands["DEMAND_SECTION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { };
			commands["NODE_COORD_SECTION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->add_element(stof(token)); };
			commands["EDGE_DATA_SECTION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { };
			commands["EDGE_WEIGHT_SECTION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { tsp_instance_to_fill->add_element(stof(token)); };
			commands["DISPLAY_DATA_SECTION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { };
			commands["TOUR_SECTION"] = [](string const& token, std::unique_ptr<tsplib_definition<T>> const& tsp_instance_to_fill) { };
						
			return commands;
		}
		
		
		template<typename T>
		typename data_section_command_type<T>::type initialize_data_section_commands(typename command_map_type<T>::type const& commands)
		{
			static typename data_section_command_type<T>::type data_section_commands;

			data_section_commands[0] = commands.find("DEPOT_SECTION");
			data_section_commands[1] = commands.find("DEMAND_SECTION");
			data_section_commands[2] = commands.find("NODE_COORD_SECTION");
			data_section_commands[3] = commands.find("EDGE_DATA_SECTION");
			data_section_commands[4] = commands.find("EDGE_WEIGHT_SECTION");
			data_section_commands[5] = commands.find("DISPLAY_DATA_SECTION");
			data_section_commands[6] = commands.find("TOUR_SECTION");

			return data_section_commands;
		}
	}
		

	template<typename T>
	unique_ptr<tsplib_definition<T>> build_instance(string const& filename)
	{
		static const auto commands(initialize_commands<T>());
		static const auto data_section_commands(initialize_data_section_commands<T>(commands));

		unique_ptr<tsplib_definition<T>> tsp_instance(new tsplib_definition<T>());
		tsp_instance->set_origin_file(filename);

		ifstream tsp_file(filename);
		if(tsp_file.good())
		{
			static const auto no_command = std::end(commands);
			static const auto data_section_begin = std::begin(data_section_commands);
			static const auto data_section_end = std::end(data_section_commands);
			static const auto end_command = commands.find("EOF");
			static const auto comment_command = commands.find("COMMENT");
			static const auto node_coord_section_command = commands.find("NODE_COORD_SECTION");
			auto command = std::end(commands);

			for(string const& token: istream_range<line>(tsp_file))
			{
				if(token == "")
				{
					continue;
				}
				
				string temp_token = token;
				
				//If the string splits with colon, it means it was a line containing a command.
				//If not, it's assumed it was a line containing numerical weigh values separated by
				//whitespace.
				vector<string> tokens;
				split(temp_token, ':', back_inserter(tokens), string_split_options::remove_empty_entries);
				const auto temp_command = commands.find(trim(tokens[0]));
				if(temp_command != no_command)
				{
					command = temp_command;
				}

				if(!tokens.empty() && command == comment_command)
				{
					//The IF branch means the reserved word COMMENT has already been encountered. The field after
					//semicolon should be a new reserved word, otherwise the whole line will be marked as a comment.
					//The ELSE branch means this is the same line in which the reserved word COMMENT was encountered.
					//The line-feed is added since the comment lines should be as-is.
					//
					//Currently there isn't a check if there's a reserved word that it's possible and the parameter
					//type is correct. That is, if the first token isn't a reserved word, the whole line is
					//assumed to be a comment line.
					if(temp_command == comment_command)
					{
						//The first token is trimmed because this is the line that contains the word "COMMENT" -- which is the first word.
						(*comment_command).second(trim_left(accumulate(++tokens.begin(), tokens.end(), string("")) + "\n"), tsp_instance);
					}
					else
					{
						(*comment_command).second(token + "\n", tsp_instance);
					}

					continue;
				}

				if((find(data_section_begin, data_section_end, command) != data_section_end && temp_command != command))
				{
					//A re-split since there was no double colon as it's the weights being parsed...
					tokens.clear();
					split(token, ' ', back_inserter(tokens), string_split_options::remove_empty_entries);
					if(!tokens.empty())
					{
						auto start_token = tokens.begin();
						if(command == node_coord_section_command)
						{
							++start_token;
						}
					
						for_each(start_token, tokens.end(), [&command, &tsp_instance](string const& token) { (*command).second(token, tsp_instance); });
					}

					continue;
				}

				if(tokens.size() == 2)
				{
					(*command).second(trim(tokens[1]), tsp_instance);
				}
			
				if(command == end_command || tokens.empty())
				{
					break;
				}
			}
		}

		//TODO: These should be confidence intervals or somesuch.
		//NOTE: Currently VS 2012 warning C4244: 'argument' : conversion from 'tsp_numeric_type' to 'unsigned long', possible loss of data"
		//will be not ignored or remedied on purpose. The reason is the numeric type thing should be fixed to flow
		//more naturally everywhere in the code.
		tsp_instance->set_shortest_known_tour(tsplib::best_known_tour_cost<T>(tsp_instance->get_name()));
					

		//TODO: If the file is not found, this should throw.
		return tsp_instance;
	}
}
