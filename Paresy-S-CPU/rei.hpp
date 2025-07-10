#ifndef REI_HPP
#define REI_HPP

#include <string>
#include <vector>
#include <bitmask128.hpp>

#define UINT128 bitmask128

struct Result
{
    std::string     RE;
    int             REcost;
    unsigned long   allREs;
    int             ICsize;

    Result(const std::string& RE, int REcost, unsigned long allREs, int ICsize)
        : RE(RE), REcost(REcost), allREs(allREs), ICsize(ICsize) {
    }
};

Result REI(const unsigned short* costFun, const unsigned short maxCost, const std::vector<std::string>& pos, const std::vector<std::string>& neg);

#endif // REI_HPP