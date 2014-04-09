// source: pattern_matcher.hpp
// use:	   Defines simple regexp based functions for pattern matching, matrix generation et al.
//		   Not very general purpose, relies on some assumptions about the patterns.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _PATTERN_MATCHER_HPP_BUMPTZI
#define _PATTERN_MATCHER_HPP_BUMPTZI

#include <amp_short_vectors.h> // For concurrency::graphics::short_vector.
#include <cmath>			   // For std::abs, std::ceil, std::cos and M_PI.
#include <exception>		   // For std::logic_error.
#include <functional>		   // For std::function.
#include <iostream>			   // For std::cout.
#include <iterator>			   // For std::istream_iterator.
#include <ppl.h>			   // For concurrency::parallel_for_each.
#include <string>			   // For std::string.
#include <sstream>			   // For std::istringstream.
#include <utility>			   // For std::pair.
#include <vector>			   // For std::vector.

namespace pbl { struct Tsp_container; } // Forward declaration.

namespace ptm
{
	// Used to index into dist_funcs based on problem type.
	const enum Dist_fn_idx { Euc, Man, Max, Geo, Att, Ceil, Xray1, Xray2 };
	
	template<typename T> class Pattern_matcher {
	public:
		// Placeholder typedefs, this can eventually be made more flexible.
		typedef typename concurrency::graphics::short_vector<T, 3>::type Vec_t;
		typedef std::pair<unsigned int, Vec_t> Node_3d;	
		typedef std::pair<std::function<Vec_t (Vec_t, Vec_t)>, std::function<T (Vec_t)>> dist_cost_fns;
		// Constructor used for initializing invariants.
		Pattern_matcher(void); 

		std::vector<T> make_adj_mtx(const std::string& data_str, const pbl::Tsp_container& tsp_params);
		std::vector<T> read_tri_mtx(std::istream_iterator<T>& data_it, const pbl::Tsp_container& params);
		std::vector<T> read_func_data(std::istream_iterator<T>& data_it, const pbl::Tsp_container& params);
		template<typename U> 
		std::vector<T> calc_edg_wgt(const std::vector<U>& node_data, const pbl::Tsp_container& params);
	private:
		std::vector<dist_cost_fns> dist_funcs;
	};
	
	// Definitions for function templates.
	template<typename T> Pattern_matcher<T>::Pattern_matcher(void)
	{
		// Helpers.
		const auto vabs = [ ](const Vec_t& vec) { return Vec_t(std::abs(vec.x), std::abs(vec.y), std::abs(vec.z)); };
		const auto vceil = [ ](const Vec_t& vec) { return Vec_t(std::ceil(vec.x), std::ceil(vec.y), std::ceil(vec.z)); };
		const auto vcos = [ ](const Vec_t& vec) { return Vec_t(std::cos(vec.x), std::cos(vec.y), std::cos(vec.z)); };
		const auto add_vec_com = [ ](const Vec_t& vec) { return (vec.x + vec.y + vec.z); };
		
		// Distance functions.
		const auto clc_dst = [ ](const Vec_t& lhs, const Vec_t& rhs) { return lhs - rhs; };
		const auto clc_abs_dst = [vabs](const Vec_t& lhs, const Vec_t& rhs) { return vabs(lhs - rhs); };
		const auto clc_lat_lng = [vceil](const Vec_t& node) -> Vec_t { // Used for computing latitude and longitude.			
			const auto degs = vceil(node);
			const auto mins = node - degs;

			return T(M_PI) * (degs + T(5) * mins / T(3)) / T(180);
		};
		const auto clc_geo_dst = [vcos, clc_lat_lng](const Vec_t& lhs, const Vec_t& rhs) -> Vec_t { 
			const auto lhs_geo = clc_lat_lng(lhs);
			const auto rhs_geo = clc_lat_lng(rhs);

			return vcos(Vec_t((lhs_geo - rhs_geo).y, (lhs_geo - rhs_geo).x, (lhs_geo + rhs_geo).x)); 
		};

		// Populate vector of distance_fn - edge_weight_fn pairs.
		dist_funcs.emplace(std::begin(dist_funcs) + Euc, clc_dst, [add_vec_com](const Vec_t& dist) { 
			return std::sqrt(add_vec_com(dist * dist)); 
		});

		dist_funcs.emplace(std::begin(dist_funcs) + Man, clc_abs_dst, [add_vec_com](const Vec_t& dist) { 
			return add_vec_com(dist); 
		});

		dist_funcs.emplace(std::begin(dist_funcs) + Max, clc_abs_dst, [ ](const Vec_t& dist) { 
			return std::max((dist.x, dist.y), (dist.z)); 
		});

		dist_funcs.emplace(std::begin(dist_funcs) + Geo, clc_geo_dst, [ ](const Vec_t& dist) -> T { 
			static const T radius(6378.388); // Radius of idealized earth sphere.
			return radius * std::acos(T(0.5) * ((T(1) + dist.x) * dist.y - (T(1) - dist.x) * dist.z)) + T(1); 
		});

		dist_funcs.emplace(std::begin(dist_funcs) + Att, clc_dst, [this](const Vec_t& dist) { 
			return dist_funcs.at(Euc).second(dist / std::sqrt(T(10))); 
		}); // Not entirely true to the spec.

		dist_funcs.emplace(std::begin(dist_funcs) + Ceil, clc_dst, [this](const Vec_t& dist) { 
			return std::ceil(dist_funcs.at(Euc).second(dist)); 
		});
	}

	template<typename T> 
	std::vector<T> Pattern_matcher<T>::make_adj_mtx(const std::string& data_str, const pbl::Tsp_container& tsp_params)
	{
		std::vector<T> res_adj_mtx; 
		std::istringstream data_sstr(data_str); // For reading convenience.
		std::istream_iterator<T> data_start(data_sstr);
		std::istream_iterator<T> data_end;
						
		const auto is_explicit = (tsp_params.edg_wgt_typ == "EXPLICIT");
		if (is_explicit) {
			const auto is_full_mtx = (tsp_params.edg_wgt_fmt == "FULL_MATRIX");
			if (is_full_mtx) {
				copy(data_start, data_end, back_inserter(res_adj_mtx));
			}
			else { 
				res_adj_mtx = read_tri_mtx(data_start, tsp_params);
			}
		}
		else { // We're dealing with a non-explicit problem specification.
			res_adj_mtx = read_func_data(data_start, tsp_params);
		}

		return res_adj_mtx;
	}

	template<typename T> 
	std::vector<T> Pattern_matcher<T>::read_tri_mtx(std::istream_iterator<T>& data_it, const pbl::Tsp_container& params)
	{
		const auto is_row_majr = (params.edg_wgt_fmt.find("ROW") != std::string::npos);
		const auto is_uppr_mtx = (params.edg_wgt_fmt.find("UPPER") != std::string::npos);
		const auto is_diag_inc = (params.edg_wgt_fmt.find("DIAG") != std::string::npos); 
		const auto diag_sentry = T(-1);
		bool continue_iter = true;
					
		std::vector<T> result(params.problem_dim * params.problem_dim);
		for (auto i = 0u; i < params.problem_dim; ++i) {
			auto j = (is_uppr_mtx == true) ? i : 0;
			do {
				if (i != j) {
					const auto first_idx = (is_row_majr == true) ? (i * params.problem_dim + j) : (j * params.problem_dim + i);
					const auto other_idx = (is_row_majr == true) ? (j * params.problem_dim + i) : (i * params.problem_dim + j);
		
					result.at(first_idx) = *data_it;
					result.at(other_idx) = result.at(first_idx);
		
					++data_it;
				}
				else {
					result.at(i * params.problem_dim + i) = diag_sentry;
					if (is_diag_inc == true) {
						++data_it; // Make sure that iterator doesn't get incremented spuriously.
					}
				}
				++j;
				continue_iter = (is_uppr_mtx == true) ? (j < params.problem_dim) : (j <= i);
			} while (continue_iter == true);
		}
		return result;
	}

	template<typename T> 
	std::vector<T> Pattern_matcher<T>::read_func_data(std::istream_iterator<T>& data_it, const pbl::Tsp_container& params)
	{		
		std::istream_iterator<T> end_it;
		std::vector<Node_3d> temp_data; // For convenient component-wise ops.

		const auto is_3d = (params.edg_wgt_typ.find("3D") != std::string::npos);
		while (data_it != end_it) {
			const auto node_id = static_cast<unsigned int>(*data_it++) - 1; // TSPLIB indexing is 1-based, so we adjust.
			const auto x_coord = *data_it++;
			const auto y_coord = *data_it++;
			const auto z_coord = (is_3d) ? *data_it++ : T(0);

			temp_data.emplace_back(node_id, Vec_t(x_coord, y_coord, z_coord));
		}
		return calc_edg_wgt(temp_data, params);
	}

	template<typename T> 
	template<typename U> 
	std::vector<T> Pattern_matcher<T>::calc_edg_wgt(const std::vector<U>& node_data, const pbl::Tsp_container& params)
	{
		typedef typename U::second_type Vec_t;
		// Fill a vector with keywords determining function type; XRAYs unimplemented for now.
		std::vector<std::string> edg_wgt_typ;
		edg_wgt_typ.emplace(std::begin(edg_wgt_typ) + Euc, "EUC");
		edg_wgt_typ.emplace(std::begin(edg_wgt_typ) + Man, "MAN");
		edg_wgt_typ.emplace(std::begin(edg_wgt_typ) + Max, "MAX");
		edg_wgt_typ.emplace(std::begin(edg_wgt_typ) + Geo, "GEO");
		edg_wgt_typ.emplace(std::begin(edg_wgt_typ) + Att, "ATT");
		edg_wgt_typ.emplace(std::begin(edg_wgt_typ) + Ceil, "CEIL");
			 
		std::vector<T> result(params.problem_dim * params.problem_dim);	
		
		const auto fn_it = std::find_if(std::begin(edg_wgt_typ), std::end(edg_wgt_typ), [&params](const string& type) {
			return params.edg_wgt_typ.find(type) != std::string::npos; 
		});
		if (fn_it == std::end(edg_wgt_typ)) {
			throw std::logic_error("Ill-formed TSP used as input to ptm::calc_edg_wgt()");
		}

		const auto fns = dist_funcs[std::distance(std::begin(edg_wgt_typ), fn_it)];
		// Parallelise across nodes, each treated as "owner" of a row.
		concurrency::parallel_for_each(std::begin(node_data), std::end(node_data), [node_data, params, fns, &result](const U& node) {
			const auto node_idx = node.first;
			const auto node_dta = node.second;
			const T diag_sentry(-1);

			for (auto i = 0u; i != params.problem_dim; ++i) {
				if (i == node_idx) {
					result.at(node_idx * params.problem_dim + node_idx) = diag_sentry;
					continue;
				}
				else {
					const auto dist = fns.first(node_data.at(i).second, node_dta);
					result.at(node_idx * params.problem_dim + i) =  fns.second(dist);
				}
			}
		});
		return result;
	}
} // Namespace ptm.
#endif // _PATTERN_MATCHER_HPP_BUMPTZI