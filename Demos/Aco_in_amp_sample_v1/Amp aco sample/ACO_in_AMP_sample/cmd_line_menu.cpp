// source: cmd_line_menu.cpp
// use:    Implements menu classes; based on Chapter 12 from D. M. Capper
//		   "Introducing C++ for Scientists, Engineers and Mathematicians 2nd edition"
// author: Alex Voicu
// date:   29/06/2012

#include <limits>				 // For std::numeric_limits.
#include "cmd_line_menu.hpp"
#include "exception_classes.hpp"

using namespace exc;
using namespace std;

// class Menu declarations
clm::Menu::Menu(const string& menu_label, int max_no_of_opt)
	: Option(menu_label), max_options(max_no_of_opt)
{
	if (max_no_of_opt <= 0) {
		throw Menu_sz_ex("clm::Menu::Menu(const string&, int)", max_no_of_opt);
	}
	else {
		option_list.reserve(max_options);
	}
	options = 0;
}

void clm::Menu::set_option(unique_ptr<Option>&& opt)
{
	if (options < max_options) {
		option_list.emplace_back(move(opt));
		++options;
	}
	else {
		throw Menu_set_ex("clm::Menu::set_option(unique_ptr<Option>&&)", options + 1, max_options);
	}
}

void clm::Menu::activate(void) const
{
	const auto max_str_sz = numeric_limits<streamsize>::max();
	unsigned int choice = options + 1;

	do {
		cout << '\n';
		print_label();
		cout << '\n';
		for (auto i = 0u; i != options; ++i) {
			cout << i << '\t';
			option_list.at(i)->print_label();
			cout << '\n';
		}
		cout << "Select option: ";
		cin >> choice;
		if (cin == false) { 
			cin.clear();
		}
		cin.ignore(max_str_sz, '\n'); // In case there's something other than the choice stuck.
	} while (choice >= options);

	option_list.at(choice)->activate();
}
//class Action declarations
clm::Action::Action(const string& action_label, function<void (void)> fct, clm::Option* op_ptr)
	: Option(action_label), func(fct), after_opt(op_ptr) 
{ 
}