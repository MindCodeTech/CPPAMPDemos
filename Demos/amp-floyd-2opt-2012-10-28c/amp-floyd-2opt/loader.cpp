// source: loader.cpp
// use:	   Implements Simple_loader class.
// author: Alex Voicu
// date:   29/06/2012

#include <cstdlib>          // For std::toupper.
#include <exception>		// For std::logic_error.
#include <iosfwd>			// For std::streamsize.
#include <limits>			// For std::numeric_limits.
#include "loader.hpp"
#include "init_structs.hpp"

using namespace clm;
using namespace hlp;
using namespace ini;
using namespace ldr;
using namespace pbl;
using namespace std;
using namespace std::tr2::sys;

Simple_loader::Simple_loader(const Ldr_menu_init& init)
	: Loader(), top_menu(init.top_menu_txt, init.top_menu_sz)
{
}

void Simple_loader::init_menu(Option& return_to)
{
	// Define lambda wrappers for members.
	const auto load_one = [this]( ) { single_load(); };
	const auto load_multiple = [this]( ) { multi_load(); };
	const auto unload_all = [this]( ) { pbl_to_load.clear(); };
	const auto print_loaded = [this]( ) { for (const auto& pbl : pbl_to_load) cout << '\n' << pbl.first << '\t'; };
	const auto print_count = [this]( ) { cout << "There are: " << pbl_to_load.size() << "TSPs in the queue.\n"; cin.get(); clear_scr(); };
	// Set options for top_menu.
	top_menu.set_option(unique_ptr<Option>(new Action("Load single TSP", load_one, &top_menu)));
	top_menu.set_option(unique_ptr<Option>(new Action("Load multiple TSPs", load_multiple, &top_menu)));
	top_menu.set_option(unique_ptr<Option>(new Action("Unload all TSPs", unload_all, &top_menu)));
	top_menu.set_option(unique_ptr<Option>(new Action("Show loaded TSPs", print_loaded, &top_menu)));
	top_menu.set_option(unique_ptr<Option>(new Action("Return to previous menu", print_count, &return_to)));
}

void Simple_loader::single_load(void)
{
	auto parsed_dir = parse_dir(current_path<path>());
	unsigned int choice = 0;
	bool choice_made = false;
	// Start traversal loop. 
	do {					
		const unsigned int max_choice = static_cast<unsigned int>(parsed_dir.size());
		print_dir(parsed_dir, choice);

		auto cmd = get_kbd_in();
		if (cmd != In_key::Enter) { // Wait for selection.
			choice = move_cursor(cmd, max_choice, choice);
			choice_made = false;
		}
		else if (choice == max_choice) { // Get back to prior menu.
			choice_made = true; 
		}
		else if (is_directory(parsed_dir.at(choice).status())) { // Browse dir.
			current_path(parsed_dir.at(choice).path());
			parsed_dir = parse_dir(current_path<path>()); 
			choice = 0;
			choice_made = false;
		}
		else { // Process candidate TSP.
			choice_made = process_choice(parsed_dir.at(choice).path());
		}
	} while (choice_made == false);
}

void Simple_loader::multi_load(void)
{
	bool load_another = true;
	char choice = 'X';

	do {
		single_load();

		cout << "Load another file? Y/N: ";
		while (cin >> choice) {
			choice = toupper(choice);
			if ((choice == 'Y') || (choice == 'N')) {
				break;
			}
		}

		switch (choice) {
		case 'Y':
			continue;
		case 'N':
			load_another = false;
			break;
		default:
			throw std::logic_error("Fell through default case in ldr::Simple_loader::multi_load()");
		}
	} while (load_another == true);
}

bool ldr::Simple_loader::process_choice(path chosen_path)
{
	ifstream in_file(chosen_path);
	if(!in_file) {
		cout << "\n File " << chosen_path << " could not be opened!\n";
		return false;
	}
	const auto work_str = file_to_str(chosen_path); // Inefficient.
	
	const auto has_name = [ ](const string& file_str) { return (file_str.find("NAME") != string::npos); };
	const auto has_dim = [ ](const string& file_str) { return (file_str.find("DIMENSION") != string::npos); };
		
	const auto is_valid = has_name(work_str) && has_dim(work_str);
	if (is_valid == true) {
		const auto already_loaded = pbl_to_load.find(chosen_path.filename()) != end(pbl_to_load);

		if (already_loaded == false) {
			pbl_to_load.emplace(chosen_path.filename(), chosen_path);
			sg_solve_tgt = chosen_path.filename();
			
			cout << "Added " << sg_solve_tgt << " as target problem.\n";
			cin.get();

			return true;
		}
		else {
			sg_solve_tgt = chosen_path.filename();

			cout << chosen_path.filename() << " is already in the target list. Setting as solve target.\n";
			cin.get();

			return true;
		}
	}
	else {
		cout << chosen_path.filename() << " does not contain a valid TSP.\n";
		cin.get();

		return false;
	}
}

vector<Tsp_container> Simple_loader::get_multi_tsp(void) const
{
	vector<Tsp_container> result;

	for (const auto& pblm : pbl_to_load) {
		result.push_back(parse_tsp(file_to_str(pblm.second)));
	}
	return result;
}