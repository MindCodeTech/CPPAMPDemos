// source: problem.cpp
// use:	   Implements helper functions for making a pbl::Tsp_container.
// author: Alex Voicu
// date:   29/06/2012

#include <cstdlib>			   // For std::strtoul.
#include "pattern_matcher.hpp"
#include "problem.hpp"

using namespace pbl;
using namespace ptm;
using namespace std;

Tsp_container pbl::parse_tsp(const string& in_work_str)
{
	// Define the patterns for a TSPLIB. Put them in an array for convenience. Ugly-ish.
	const enum Kwd_idx { Name, Type, Dimm, Edwt, Edwf, Data };
	static vector<regex> patterns;
	patterns.emplace(begin(patterns) + Name, "NAME\\s*:\\s*(\\w+)");
	patterns.emplace(begin(patterns) + Type, "TYPE\\s*:\\s*(\\w+)");
	patterns.emplace(begin(patterns) + Dimm, "DIMENSION\\s*:\\s*(\\d+)");
	patterns.emplace(begin(patterns) + Edwt, "EDGE_WEIGHT_TYPE\\s*:\\s*(\\w+)");
	patterns.emplace(begin(patterns) + Edwf, "EDGE_WEIGHT_FORMAT\\s*:\\s*(\\w+)");
	patterns.emplace(begin(patterns) + Data, "(?:NODE_COORD_SECTION|EDGE_DATA_SECTION|EDGE_WEIGHT_SECTION)([\\d\\s\\w$-.+]+)");
	
	// Load the problem data.
	vector<string> pbl_params;
	for (auto i = 0u; i != patterns.size(); ++i) {
		pbl_params.emplace(begin(pbl_params) + i, extract_param(in_work_str, patterns.at(i)));
	}

	// Populate container
	Pattern_matcher<float> pat_mat;
	Tsp_container result;
	result.problem_nam = pbl_params[Name];
	result.problem_typ = pbl_params[Type];
	result.problem_dim = strtoul(pbl_params[Dimm].c_str(), nullptr, 10);
	result.edg_wgt_typ = pbl_params[Edwt];
	result.edg_wgt_fmt = pbl_params[Edwf];
	result.pbl_adj_mtx = pat_mat.make_adj_mtx(pbl_params[Data], result);

	return result;
}

string pbl::extract_param(const string& where, const regex& what)
{
	smatch matches;
	const auto found = regex_search(where, matches, what);
	if (found == true && (matches.size() >= 2)) { // We assume that we have the wanted data in the first sub-pattern.
		return matches[1];
	}
	else {
		return std::string(); 
	}
}