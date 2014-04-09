// source: helper_functions.cpp
// use:	   Defines useful helpers for UI processing.
//		   Tailored for ACO_in_AMP sample, not very general purpose.
// author: Alex Voicu
// date:   29/06/2012

#include <amp_graphics.h>		// For concurrency::direct3d::get_device.
#include <cctype>				// For toupper.
#include <conio.h>				// For _kbhit and _getch.
#include <d3d11.h>				// For ID3D11Device.
#include <dxgi.h>				// For IDXGIDevice and IDXGIAdapter.
#include <utility>				// For std::pair and std::make_pair.
#include "helper_functions.hpp"
#include "pattern_matcher.hpp"
#include "problem.hpp"

using namespace concurrency;
using namespace hlp;
using namespace pbl;
using namespace ptm;
using namespace std;
using namespace std::tr2::sys;

In_key hlp::get_kbd_in(void)
{
	do {
		if (_kbhit() != 0) {
			const char input = static_cast<char>(toupper(_getch()));
			switch (input) {
			case 'S':
				return In_key::Down;
			case 'W':
				return In_key::Up;
			case '\r':
				return In_key::Enter;
			default:
				continue;
			}
		}
	} while (true); // Infinite loop for grabbing input.
}

unsigned int hlp::move_cursor(In_key cmd, unsigned int max, unsigned int cur_pos)
{
	auto new_cur_pos = cur_pos;

	switch (cmd) {
		case (In_key::Down):
			(new_cur_pos < max) ? (++new_cur_pos) : 0;
			break;
		case (In_key::Up):
			(new_cur_pos > 0) ? (--new_cur_pos) : 0;
			break;
		default:
			throw std::logic_error("Case fell through default in hlp::move_cursor()");
	}
	return new_cur_pos;
}

vector<directory_entry> hlp::parse_dir(const path& dir_path)
{
	const directory_iterator beg_it(dir_path);
	const directory_iterator end_it; // Default constructor points to end.
	// Helper lambdas.
	const auto not_tsp = [ ](const path& in_path) { return (in_path.extension() != ".tsp"); };
	const auto not_atsp = [ ](const path& in_path) { return (in_path.extension() != ".atsp"); };
	const auto not_dir = [ ](const path& in_path) { return (is_directory(in_path) == false); };
	const auto not_valid = [=](const path& in_path) { return (not_tsp(in_path) && not_atsp(in_path) && not_dir(in_path)); };
	
	vector<directory_entry> dir_contents(beg_it, end_it);
	dir_contents.erase(remove_if(begin(dir_contents), end(dir_contents), not_valid), end(dir_contents)); // Keep only directories, .tsp and .atsp files.
	if (dir_path.has_branch_path() == true) { // Insert parent or root_sentry.
		dir_contents.emplace(begin(dir_contents), dir_path.branch_path());
	}
	else {
		dir_contents.emplace(begin(dir_contents), path(""));
	}
	return dir_contents;
}

void hlp::print_dir(const vector<directory_entry>& dir_contents, unsigned int cur_choice)
{
	// This is going to be quite ugly for dirs that containt lots of files. TODO fix.
	clear_scr();

	const auto is_choice = [=](unsigned int pos) { return ((pos == cur_choice) ? "-->" : "   "); };
	for (auto i = 0u; i != dir_contents.size(); ++i) {
		cout << is_choice(i);
		if (i == 0) {
			cout << "..\\\n";
		}
		else {
			cout << dir_contents.at(i).path().filename() << '\n';
		}
	}			
	cout << is_choice(static_cast<unsigned int>(dir_contents.size())) << "Return to previous menu.\n"; 
}

concurrency::accelerator hlp::disable_tdr(const concurrency::accelerator& in_acc)
{
	ID3D11Device* device_ptr = nullptr;
	IDXGIDevice* dxgi_dev_ptr = nullptr;
	IDXGIAdapter* adapter_ptr = nullptr;
	ID3D11DeviceContext* dev_ctx_ptr = nullptr;
	
	direct3d::get_device(in_acc.default_view)->QueryInterface(&device_ptr);
	device_ptr->QueryInterface(&dxgi_dev_ptr);
	dxgi_dev_ptr->GetAdapter(&adapter_ptr);

	auto flags = device_ptr->GetCreationFlags();
	flags |= D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
	D3D11CreateDevice(adapter_ptr, D3D_DRIVER_TYPE_UNKNOWN, HMODULE(0), flags, nullptr, 0, D3D11_SDK_VERSION, &device_ptr, nullptr, &dev_ctx_ptr);

	return direct3d::create_accelerator_view(device_ptr).get_accelerator();
}

