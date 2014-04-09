// source: exception_classes.hpp
// use:	   Declares and defines classes derived from std::except.
//		   Used for cleaner throw - catch blocks. All definitions inline.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _EXCEPTION_CLASSES_HPP_BUMPTZI
#define _EXCEPTION_CLASSES_HPP_BUMPTZI

#include <exception> // For std::exception
#include <sstream>   // For std::ostringstream 
#include <string>	 // For std::string

namespace exc
{
	struct Menu_sz_ex : public std::exception {
		Menu_sz_ex(const std::string& where, int why)
		{
			std::ostringstream err_txt;
			err_txt << "Error in function " << where << ": max_options = " <<
				why << ". It must be greater than 0.\n";

			exception::operator=(std::exception(err_txt.str().c_str()));
		}
	};

	struct Menu_set_ex : public std::exception {
		Menu_set_ex(const std::string& where, unsigned int why, unsigned int lim)
		{
			std::ostringstream err_txt;
			err_txt << "Error in function " << where << ": trying to set " <<
				why << " menu options.\n Only " << lim <<" options allowed.\n";

			exception::operator=(std::exception(err_txt.str().c_str()));
		}
	};

	struct File_err_ex : public std::exception {
		File_err_ex(const std::string& where, const std::string& who)
		{
			std::ostringstream err_txt;
			err_txt << "Error in function " << where << ". File: " << who <<
				" could not be loaded.\n";
			
			exception::operator=(std::exception(err_txt.str().c_str()));
		}
	};
} // Namespace exc.
#endif // _EXCEPTION_CLASSES_HPP_BUMPTZI