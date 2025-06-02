#include <regexMatch.h>

#include <rei.h>

std::vector<bool> match(const std::vector<std::string>& examples, const std::string& pattren) {
    std::vector<bool> res(examples.size());
    for (int i = 0; i < examples.size(); i++) 
        res[i] = match(pattren, examples[i]);
    return res;
}

std::string DetSplit(int window, const unsigned short* costFun, const unsigned short maxCost,
    const std::vector<std::string>& pos, const std::vector<std::string>& neg)
{
    if (pos.size() + neg.size() <= static_cast<size_t>(window)) {

        std::string output = REI(costFun, maxCost, pos, neg).RE;
        if (output != "not_found") return output;
    }

    // Split pos and neg
    auto midPos = pos.begin() + pos.size() / 2;
    std::vector<std::string> p1(pos.begin(), midPos);
    std::vector<std::string> p2(midPos, pos.end());

    auto midNeg = neg.begin() + neg.size() / 2;
    std::vector<std::string> n1(neg.begin(), midNeg);
    std::vector<std::string> n2(midNeg, neg.end());

    std::string r11 = DetSplit(window, costFun, maxCost, p1, n1);
    auto r11FilterOnP2 = match(p2, r11);
    auto r11FilterOnN2 = match(n2, r11);

    bool r11AcceptsTheWholeP2 = std::all_of(r11FilterOnP2.begin(), r11FilterOnP2.end(), [](bool val) { return val; });
    bool r11RejectsTheWholeN2 = std::none_of(r11FilterOnN2.begin(), r11FilterOnN2.end(), [](bool val) { return val; });

    if (r11AcceptsTheWholeP2 && r11RejectsTheWholeN2) { return r11; }

    std::string left;
    if (r11RejectsTheWholeN2) {
        left = r11;
    }
    else {
        std::vector<std::string> n2Andr11;
        for (size_t i = 0; i < n2.size(); ++i) {
            if (r11FilterOnN2[i]) {
                n2Andr11.push_back(n2[i]);
            }
        }

        std::string r12 = DetSplit(window, costFun, maxCost, p1, n2Andr11);
        std::set<std::string> n2Andr11Set(n2Andr11.begin(), n2Andr11.end());

        std::vector<std::string> negMinusN2Andr11;
        for (const auto& n : neg) {
            if (n2Andr11Set.find(n) == n2Andr11Set.end()) {
                negMinusN2Andr11.push_back(n);
            }
        }

        auto r12Filter = match(negMinusN2Andr11, r12);
        bool r12CoversRemainingNeg = std::none_of(r12Filter.begin(), r12Filter.end(), [](bool val) { return val; });

        if (r12CoversRemainingNeg) {
            left = r12;
        }
        else {
            left = "(" + r11 + ")&(" + r12 + ")";
        }

        auto leftFilterOnP2 = match(p2, left);
        bool leftIsAnswer = std::all_of(leftFilterOnP2.begin(), leftFilterOnP2.end(), [](bool val) { return val; });
        if (leftIsAnswer) {
            return left;
        }
    }

    auto leftFilterOnP2 = match(p2, left);
    std::vector<std::string> P2MinusLeft;
    for (size_t i = 0; i < p2.size(); ++i) {
        if (!leftFilterOnP2[i]) {
            P2MinusLeft.push_back(p2[i]);
        }
    }

    std::string r21 = DetSplit(window, costFun, maxCost, P2MinusLeft, n1);

    std::set<std::string> P2MinusLeftSet(P2MinusLeft.begin(), P2MinusLeft.end());
    std::vector<std::string> P1MinusP2MinusLeft;
    for (const auto& t : pos) {
        if (P2MinusLeftSet.find(t) == P2MinusLeftSet.end()) {
            P1MinusP2MinusLeft.push_back(t);
        }
    }

    auto r21FilterOnP1 = match(P1MinusP2MinusLeft, r21);
    auto r21FilterOnN2 = match(n2, r21);

    bool r21AcceptsTheWholeP1 = std::all_of(r21FilterOnP1.begin(), r21FilterOnP1.end(), [](bool val) { return val; });
    bool r21RejectsTheWholeN2 = std::none_of(r21FilterOnN2.begin(), r21FilterOnN2.end(), [](bool val) { return val; });

    if (r21AcceptsTheWholeP1 && r21RejectsTheWholeN2) {
        return r21;
    }

    std::string right;
    if (r21RejectsTheWholeN2) {
        right = r21;
    }
    else {
        std::vector<std::string> n2Andr21;
        for (size_t i = 0; i < n2.size(); ++i) {
            if (r21FilterOnN2[i]) {
                n2Andr21.push_back(n2[i]);
            }
        }

        std::string r22 = DetSplit(window, costFun, maxCost, P2MinusLeft, n2Andr21);
        std::set<std::string> n2Andr21Set(n2Andr21.begin(), n2Andr21.end());

        std::vector<std::string> negMinusN2Andr21;
        for (const auto& n : neg) {
            if (n2Andr21Set.find(n) == n2Andr21Set.end()) {
                negMinusN2Andr21.push_back(n);
            }
        }

        auto r22Filter = match(negMinusN2Andr21, r22);
        bool r22CoversRemainingNeg = std::none_of(r22Filter.begin(), r22Filter.end(), [](bool val) { return val; });

        if (r22CoversRemainingNeg) {
            right = r22;
        }
        else {
            right = "(" + r21 + ")&(" + r22 + ")";
        }

        std::vector<std::string> posMinusP2MinusLeft;
        for (const auto& p : pos) {
            if (P2MinusLeftSet.find(p) == P2MinusLeftSet.end()) {
                posMinusP2MinusLeft.push_back(p);
            }
        }

        auto rightFilter = match(posMinusP2MinusLeft, right);
        bool rightIsAnswer = std::all_of(rightFilter.begin(), rightFilter.end(), [](bool val) { return val; });

        if (rightIsAnswer) {
            return right;
        }
    }

    auto res =  "(" + left + ")|(" + right + ")";

    auto res_p = match(pos, res);
    auto res_n = match(neg, res);

    return res;
}