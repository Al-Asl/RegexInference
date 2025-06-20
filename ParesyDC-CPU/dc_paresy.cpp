#include "dc_paresy.hpp"

#include <set>
#include <algorithm>
#include <gpu126.h>
#include <regex_match.hpp>

using namespace std;

 vector<bool> match(const vector<string>& examples, const string& pattern) {
    vector<bool> res(examples.size());
    for (int i = 0; i < examples.size(); i++)
        res[i] = match(pattern, examples[i]);
    return res;
}

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

 string detSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
    const vector<string>& pos, const vector<string>& neg, RecursiveProfileInfo& profileInfo) {

     profileInfo.enter();

    if (pos.size() + neg.size() <= static_cast<size_t>(window)) {
        string output = REI(costFun, maxCost, pos, neg).RE;
        if (output != "not_found") return output;
    }

    auto [p1, p2] = midSplit(pos);
    auto [n1, n2] = midSplit(neg);

    string r11 = detSplit(window, costFun, maxCost, p1, n1, profileInfo);
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
        string r12 = detSplit(window, costFun, maxCost, p1, n2Andr11, profileInfo);
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

    string r21 = detSplit(window, costFun, maxCost, p2MinusLeft, n1, profileInfo);
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
        string r22 = detSplit(window, costFun, maxCost, p2MinusLeft, n2Andr21, profileInfo);
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