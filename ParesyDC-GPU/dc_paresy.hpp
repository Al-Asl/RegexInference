#ifndef DC_PARESY_HPP
#define DC_PARESY_HPP

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

    // Main recursive function
    std::string detSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
        const std::vector<std::string>& pos, const std::vector<std::string>& neg, RecursiveProfileInfo& profileInfo);

}

#endif //DC_PARESY_HPP