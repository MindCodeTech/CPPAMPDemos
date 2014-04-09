// source: cmd_line_menu.hpp
// use:	   Defines a commandline menu inheritance hierarchy; based on Chapter 12 from 
//   	   D. M. Capper "Introducing C++ for Scientists, Engineers and Mathematicians 2nd edition"
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _CMD_LINE_MENU_HPP_BUMPTZI
#define _CMD_LINE_MENU_HPP_BUMPTZI

#include <functional> // For std::function.
#include <iostream>   // For std::cout and std::cin.
#include <memory>	  // For std::unique_ptr.
#include <string>     // For std::string.
#include <vector>	  // For std::vector.

namespace clm
{
	class Option {
	public:
		Option(const std::string& option_label)
			: label(option_label) { };
		virtual ~Option(void) { };

		virtual void activate(void) const = 0;
		void print_label(void) const { std::cout << label; };
	private:
		std::string label;
	};

	class Menu : public Option {
	public:
		explicit Menu(const std::string& menu_label, int max_no_of_opt);
		virtual ~Menu(void) { };

		void set_option(std::unique_ptr<Option>&& op_ptr);
		void activate(void) const override;
	private:
		Menu(void);                         // No default construction
		Menu(const Menu& other);            // No copy construction
		Menu& operator=(const Menu& other); // No copy assignment
		
		std::vector<std::unique_ptr<Option>> option_list;
		unsigned int options;
		unsigned int max_options;
	};

	class Action : public Option {
	public:
		Action(const std::string& action_label, std::function<void (void)> fct, Option* op_ptr = nullptr); // A bit dirty wrt nullptr usage (TODO fix)

		void activate(void) const override;
	private:
		Action(void);                           // No default construction
		Action(const Action& other);            // No copy construction
		Action& operator=(const Action& other); // No copy assignment
		
		std::function<void (void)> func;
		Option* after_opt;
	};
	
	inline void Action::activate(void) const
	{
		func();
		if (after_opt != nullptr) {
			after_opt->activate();
		}
	}
} // Namespace clm.
#endif // _CMD_LINE_MENU_HPP_BUMPTZI