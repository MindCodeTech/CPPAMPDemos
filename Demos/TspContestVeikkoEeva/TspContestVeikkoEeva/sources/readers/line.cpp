
#include "line.h"

#include <istream>
#include <string>

using utilities::line;
using std::getline;
using std::istream;
using std::string;


istream& utilities::operator>>(istream& stream, line& line)
{
	getline(stream, line.data_);
	
	return stream;
}
	

line::operator string() const
{ 
	return data_;
}
