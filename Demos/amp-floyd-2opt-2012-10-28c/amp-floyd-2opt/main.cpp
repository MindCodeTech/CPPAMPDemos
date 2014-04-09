/*
 * 
 *	Copyright (c) 2012 Bernd Paradies
 *  http://blogs.adobe.com/bparadie/
 * 
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

#include <cstdlib>			// For EXIT_FAILURE and EXIT_SUCCESS.
#include <iostream>			// For std::cerr and std::cin.
#include <exception>		// For std::exception.
#include "amp_menu.hpp"
#include "loader.hpp"
#include "problem.hpp"

#include "TspSolver.hpp"
#include "TspRunner.hpp"
#include "TspTests.hpp"

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace concurrency;
using namespace ini;
using namespace ldr;
using namespace slv;
using namespace std;
using namespace opt;
using namespace run;

//----------------------------------------------------------------------------
// main()
//----------------------------------------------------------------------------

#define TSP_NAME "Floyd-Warshall with Nearest-Neighbor and 2-opt"

int main(int argc, const char * argv[]) 
{
	// Setup init parameters
	// App menu
	App_menu_init app_init;
	app_init.top_menu_sz = 4;
	app_init.slv_menu_sz = 5;
	app_init.top_menu_txt = "***TSP solver using " TSP_NAME " implemented in C++ AMP***";
	app_init.slv_menu_txt = "***Select accelerator for solve and execute it***";
	// Loader menu
	Ldr_menu_init ldr_init;
	ldr_init.top_menu_txt = "***Choose single/multiple problems to load***";
	ldr_init.top_menu_sz = 5;

	try {

		TspSolver solver;
		if( argc > 1 )
		{
			TspOptions options;
			if( !options.parse(argc, argv) )
				return EXIT_FAILURE;

			TspRunner runner(solver, options);
			runner.run();
		}
		else
		{
			Simple_loader loader(ldr_init);
			Amp_menu app_menu(app_init, loader, solver);
			app_menu.init_menu();
		}
	}
	catch (exception& ex) {
		cerr << ex.what() << endl;
		cin.get();

		return EXIT_FAILURE;
	}
	catch (...) {
		cerr << "Some incredibly dangerous and utterly unknown thing messed up the poor app!" << endl;
		cin.get();

		return EXIT_FAILURE;
	}
 	return EXIT_SUCCESS;
}