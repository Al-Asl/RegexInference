#ifndef REI_UTIL_HPP
#define REI_UTIL_HPP

#include <vector>
#include <sstream>
#include <iostream>

//#include "rei.hpp"

bool readStream(std::istream& stream, std::vector<std::string>& pos, std::vector<std::string>& neg);

// Reading the input file
bool readFile(const std::string& fileName, std::vector<std::string>& pos, std::vector<std::string>& neg);

//std::vector<std::string> CSToStrings(const std::set<std::string, strComparison>& ic, UINT128 cs);

//UINT128 stringsToCS(const std::set<std::string, strComparison>& ic, const std::string* begin, const std::string* end);

//void print(const std::string* begin, const std::string* end);

#endif //end REI_UTIL_HPP