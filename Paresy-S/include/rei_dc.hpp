#ifndef REI_DC_HPP
#define REI_DC_HPP

#include <vector>
#include <string>
#include <tuple>

namespace paresy_s {

    struct RecursiveProfileInfo
    {
        int callCount = 0;
        int currentDepth = 0;
        int maxDepth = 0;

        void enter() {
            ++callCount;
            ++currentDepth;
            if (currentDepth > maxDepth)
                maxDepth = currentDepth;
        }

        void exit() {
            --currentDepth;
        }
    };

    std::string detSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
        const std::vector<std::string>& pos, const std::vector<std::string>& neg, double maxTime, RecursiveProfileInfo& profileInfo);

    std::string randSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
        const std::vector<std::string>& pos, const std::vector<std::string>& neg, double maxTime, RecursiveProfileInfo& profileInfo);
}

#endif //REI_DC