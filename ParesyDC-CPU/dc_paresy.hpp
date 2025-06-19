#ifndef DC_PARESY_HPP
#define DC_PARESY_HPP

#include <vector>
#include <string>
#include <tuple>

 std::vector<bool> match(const std::vector<std::string>& examples, const std::string& pattern);

 std::tuple<std::vector<std::string>, std::vector<std::string>> midSplit(const std::vector<std::string>& vec);

 bool matchesAll(const std::vector<std::string>& examples, const std::string& pattern);

 bool matchesNone(const std::vector<std::string>& examples, const std::string& pattern);

 std::vector<std::string> select(const std::vector<std::string>& vec, const std::vector<bool>& filter);

 std::vector<std::string> selectInverse(const std::vector<std::string>& vec, const std::vector<bool>& filter);

 std::vector<std::string> subtract(std::vector<std::string> a, std::vector<std::string> b);

// Main recursive function
 std::string detSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
    const std::vector<std::string>& pos, const std::vector<std::string>& neg);


#endif //DC_PARESY_HPP