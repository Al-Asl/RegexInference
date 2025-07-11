#include "rei_dc.hpp"

#include <set>
#include <algorithm>
#include <random>
#include <rei.h>
#include <regex_match.hpp>

using std::vector;
using std::set;
using std::string;
using std::tuple;

 tuple<vector<string>, vector<string>> midSplit(const vector<string>& vec) {
    auto midPos = vec.begin() + vec.size() / 2;
    vector<string> p1(vec.begin(), midPos);
    vector<string> p2(midPos, vec.end());
    return { p1, p2 };
}

 bool matchesAll(const vector<string>& examples, const string& pattern) {
    auto matchResult = match(examples, pattern);
    return all_of(matchResult.begin(), matchResult.end(), [](bool val) { return val; });
}

 bool matchesNone(const vector<string>& examples, const string& pattern) {
    auto matchResult = match(examples, pattern);
    return none_of(matchResult.begin(), matchResult.end(), [](bool val) { return val; });
}

 vector<string> select(const vector<string>& vec, const vector<bool>& filter) {
    vector<string> res;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (filter[i]) {
            res.push_back(vec[i]);
        }
    }
    return res;
}

 vector<string> selectInverse(const vector<string>& vec, const vector<bool>& filter) {
    vector<string> res;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (!filter[i]) {
            res.push_back(vec[i]);
        }
    }
    return res;
}

 vector<string> subtract(vector<string> a, vector<string> b) {
    set<string> bSet(b.begin(), b.end());
    vector<string> res;
    for (const auto& n : a) {
        if (bSet.find(n) == bSet.end()) {
            res.push_back(n);
        }
    }
    return res;
}

 string paresy_s::detSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
    const vector<string>& pos, const vector<string>& neg, double maxTime, paresy_s::RecursiveProfileInfo& profileInfo) {

    profileInfo.enter();

#if LOG_LEVEL >= 1
    printf("split at depth: %u, call count: %u, pos: %u, neg: %u\n", profileInfo.maxDepth, profileInfo.callCount, (int)pos.size(), (int)neg.size());
#endif

    if (pos.size() + neg.size() <= static_cast<size_t>(window)) {
        string output = paresy_s::REI(costFun, maxCost, pos, neg, maxTime).RE;
#if LOG_LEVEL >= 1
        printf("paresy output: %s\n", output.c_str());
#endif
        if (output != "not_found") return output;
    }

    auto [p1, p2] = midSplit(pos);
    auto [n1, n2] = midSplit(neg);

    string r11 = detSplit(window, costFun, maxCost, p1, n1, maxTime, profileInfo);
    profileInfo.exit();

    auto r11FilterOnP2 = match(p2, r11);
    auto r11FilterOnN2 = match(n2, r11);

    bool r11AcceptsTheWholeP2 = all_of(r11FilterOnP2.begin(), r11FilterOnP2.end(), [](bool val) { return val; });
    bool r11RejectsTheWholeN2 = none_of(r11FilterOnN2.begin(), r11FilterOnN2.end(), [](bool val) { return val; });

    if (r11AcceptsTheWholeP2 && r11RejectsTheWholeN2) return r11;

    string left;
    if (r11RejectsTheWholeN2) {
        left = r11;
    }
    else {
        vector<string> n2Andr11 = select(n2, r11FilterOnN2);
        string r12 = detSplit(window, costFun, maxCost, p1, n2Andr11, maxTime, profileInfo);
        profileInfo.exit();

        vector<string> negMinusN2Andr11 = subtract(neg, n2Andr11);

        if (matchesNone(negMinusN2Andr11, r12))
            left = r12;
        else
            left = "(" + r11 + ")&(" + r12 + ")";

        if (matchesAll(p2, left)) return left;
    }

    auto leftFilterOnP2 = match(p2, left);
    vector<string> p2MinusLeft = selectInverse(p2, leftFilterOnP2);

    string r21 = detSplit(window, costFun, maxCost, p2MinusLeft, n1, maxTime, profileInfo);
    profileInfo.exit();

    vector<string> p1MinusP2MinusLeft = subtract(pos, p2MinusLeft);

    auto r21FilterOnP1 = match(p1MinusP2MinusLeft, r21);
    auto r21FilterOnN2 = match(n2, r21);

    bool r21AcceptsTheWholeP1 = all_of(r21FilterOnP1.begin(), r21FilterOnP1.end(), [](bool val) { return val; });
    bool r21RejectsTheWholeN2 = none_of(r21FilterOnN2.begin(), r21FilterOnN2.end(), [](bool val) { return val; });

    if (r21AcceptsTheWholeP1 && r21RejectsTheWholeN2) return r21;

    string right;
    if (r21RejectsTheWholeN2) {
        right = r21;
    }
    else {
        vector<string> n2Andr21 = select(n2, r21FilterOnN2);
        string r22 = detSplit(window, costFun, maxCost, p2MinusLeft, n2Andr21, maxTime, profileInfo);
        profileInfo.exit();

        vector<string> negMinusN2Andr21 = subtract(neg, n2Andr21);

        if (matchesNone(negMinusN2Andr21, r22)) {
            right = r22;
        }
        else {
            right = "(" + r21 + ")&(" + r22 + ")";
        }

        vector<string> posMinusP2MinusLeft = subtract(pos, p2MinusLeft);
        if (matchesAll(posMinusP2MinusLeft, right)) return right;
    }

    return "(" + left + ")+(" + right + ")";
}

 vector<string> randomSample(const vector<string>& input, size_t sampleSize, unsigned int seed = 0) {
     vector<string> result;

     if (sampleSize >= input.size()) {
         return input;
     }

     static std::mt19937 rng(seed);
     result.reserve(sampleSize);
     sample(input.begin(), input.end(), back_inserter(result), sampleSize, rng);
     return result;
 }

 string paresy_s::randSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
     const vector<string>& pos, const vector<string>& neg, double maxTime, paresy_s::RecursiveProfileInfo& profileInfo) {

    profileInfo.enter();

#if LOG_LEVEL >= 1
    printf("split at depth: %u, call count: %u, pos: %u, neg: %u\n", profileInfo.maxDepth, profileInfo.callCount, (int)pos.size(), (int)neg.size());
#endif

    int seed = 0;
    int win = window;

    string r11;

    while (true) {
        vector<string> p1, n1;
        if (pos.size() + neg.size() <= static_cast<size_t>(win)) {
            p1 = pos;
            n1 = neg;
        }
        else {
            if (pos.size() <= win / 2) {
                p1 = pos;
                n1 = randomSample(neg, win - p1.size(), seed);
            }
            else if (neg.size() <= win / 2) {
                n1 = neg;
                p1 = randomSample(pos, win - n1.size(), seed);
            }
            else {
                p1 = randomSample(pos, win / 2, seed);
                n1 = randomSample(neg, win - p1.size(), seed);
            }
        }

        string output = paresy_s::REI(costFun, maxCost, p1, n1, maxTime).RE;
    #if LOG_LEVEL >= 1
        printf("paresy output: %s\n", output.c_str());
    #endif

        if (output != "not_found")
        { r11 = output; break; }
        else
        { win /= 2; }
    }

    auto r11FilterOnP = match(pos, r11);
    auto r11FilterOnN = match(neg, r11);

    auto p2 = selectInverse(pos, r11FilterOnP);
    auto n2 = select(neg, r11FilterOnN);

    auto p1 = subtract(pos, p2);
    auto n1 = subtract(neg, n2);

    if (p2.size() == 0 && n2.size() == 0)
        return r11;

    string left;
    if (n2.size() == 0)
        left = r11;
    else
    {
        auto r12 = randSplit(window, costFun, maxCost, p1, n2, maxTime, profileInfo);
        profileInfo.exit();

        if (matchesNone(n1, r12))
            left = r12;
        else {
            left = "(" + r11 + ")&(" + r12 + ")";
            if (matchesAll(p2, left))
                return left;

        }
    }

    auto r21 = randSplit(window, costFun, maxCost, p2, n1, maxTime, profileInfo);
    profileInfo.exit();

    bool r21AcceptsTheWholeP1 = matchesAll(p1, r21);
    bool r21RejectsTheWholeN2 = matchesNone(n2, r21);

    if (r21AcceptsTheWholeP1 && r21RejectsTheWholeN2)
        return r21;

    string right;
    if (r21RejectsTheWholeN2)
        right = r21;
    else
    {
        auto r22 = randSplit(window, costFun, maxCost, p2, n2, maxTime, profileInfo);
        profileInfo.exit();

        if (matchesNone(n1, r22))
            right = r22;
        else
        {
            right = "(" + r21 + ")&(" + r22 + ")";
            if (matchesAll(p1, right))
                return right;
        }
    }
    return "(" + left + ")|(" + right + ")";
 }