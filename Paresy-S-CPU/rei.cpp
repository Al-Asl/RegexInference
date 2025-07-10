#include <rei.hpp>

#include <set>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

#define UINT128 bitmask128

#ifdef MODE
#define LOG_OP(context, cost, op_string, dif) \
        int tbc = dif; \
        if (tbc) printf("Cost %-2d | (%s) | AllREs: %-11lu | StoredREs: %-10d | ToBeChecked: %-10d \n", \
            cost, op_string.c_str() ,context.allREs, context.lastIdx, tbc);
#else
#define LOG_OP(context, cost, op_string, dif)
#endif

struct Costs
{
    int alpha;
    int question;
    int star;
    int concat;
    int or ;
    int and;
    Costs(const unsigned short* costFun) {
        alpha = costFun[0];
        question = costFun[1];
        star = costFun[2];
        concat = costFun[3];
        or = costFun[4];
        and = costFun[5];
    }
};

// ============= guide table =============

// Shortlex ordering
struct strComparison {
    bool operator () (const std::string& str1, const std::string& str2) const {
        if (str1.length() == str2.length()) return str1 < str2;
        return str1.length() < str2.length();
    }
};

// Generating the infix of a string
std::set<std::string, strComparison> infixesOf(const std::string& word) {
    std::set<std::string, strComparison> ic;
    for (int len = 0; len <= word.length(); ++len) {
        for (int index = 0; index < word.length() - len + 1; ++index) {
            ic.insert(word.substr(index, len));
        }
    }
    return ic;
}

std::set<std::string, strComparison> generatingIC(const std::vector<std::string>& pos, const std::vector<std::string>& neg) {
    // Generating infix-closure (ic) of the input strings
    std::set<std::string, strComparison> ic = {};

    for (const std::string& word : pos) {
        std::set<std::string, strComparison> set1 = infixesOf(word);
        ic.insert(set1.begin(), set1.end());
    }
    for (const std::string& word : neg) {
        std::set<std::string, strComparison> set1 = infixesOf(word);
        ic.insert(set1.begin(), set1.end());
    }
    return ic;
}

class GuideTable {
public:

    class Iterator {
        UINT128* ptr;
    public:
        Iterator(UINT128* p) : ptr(p) {}

        std::tuple<UINT128, UINT128>operator*() const { return std::make_tuple(*ptr, *(ptr + 1)); }
        Iterator& operator++() { ptr += 2; return *this; }
        bool operator!=(const Iterator& other) const { return *ptr; }
    };

    class RowIterator {
    public:
        RowIterator(const GuideTable& guideTable, int rowIndex) : guideTable(guideTable), row(rowIndex) {}
        Iterator begin() { return Iterator(guideTable.data + row * guideTable.gtColumns); }
        Iterator end() { return Iterator(guideTable.data + (row + 1) * guideTable.gtColumns); }
    private:
        const GuideTable& guideTable;
        int row;
    };

    int ICsize;
    int gtColumns;
    int alphabetSize;

    GuideTable(std::vector<std::vector<UINT128>> gt, int alphabetSize) : alphabetSize(alphabetSize)
    {
        ICsize = static_cast<int> (gt.size());
        gtColumns = static_cast<int> (gt.back().size());

        if (ICsize > 128) {
            printf("Your input needs %u bits which exceeds 128 bits ", ICsize);
            printf("(current version).\nPlease use less/shorter words and run the code again.\n");
            throw std::out_of_range("our input exceeds 128 bits!");
        }

        data = new UINT128[ICsize * gtColumns];

        for (int i = 0; i < ICsize; ++i) {
            for (int j = 0; j < gt.at(i).size(); ++j) {
                data[i * gtColumns + j] = gt.at(i).at(j);
            }
        }
    }

    GuideTable() : ICsize(0), gtColumns(0), alphabetSize(0), data(nullptr) {}

    ~GuideTable() {
        delete[] data;
    }

    RowIterator IterateRow(int rowIndex) const {
        return RowIterator(*this, rowIndex);
    }

private:
    UINT128* data;
};

void generatingGuideTable(GuideTable* guideTable, const std::set<std::string, strComparison>& ic)
{
    int alphabetSize = -1;
    for (auto& word : ic) {
        if (word.size() > 1) break;
        alphabetSize++;
    }

    std::vector<std::vector<UINT128>> gt;

    for (auto& word : ic) {
        std::vector<UINT128> row;
        for (int i = 1; i < word.length(); ++i) {

            int index1 = 0;
            for (auto& w : ic) {
                if (w == word.substr(0, i)) break;
                index1++;
            }
            int index2 = 0;
            for (auto& w : ic) {
                if (w == word.substr(i)) break;
                index2++;
            }

            row.push_back((UINT128)1 << index1);
            row.push_back((UINT128)1 << index2);
        }

        row.push_back((UINT128)0);
        gt.push_back(row);
    }

    new (guideTable) GuideTable(gt, alphabetSize);
}

// Generating of the guide table only once for the whole enumeration process
bool generatingGuideTable(GuideTable& guideTable, UINT128& posBits, UINT128& negBits,
    const std::vector<std::string>& pos, const std::vector<std::string>& neg) {

    std::set<std::string, strComparison> ic = generatingIC(pos, neg);

    generatingGuideTable(&guideTable, ic);

    for (auto& p : pos) {
        int wordIndex = distance(ic.begin(), ic.find(p));
        posBits |= ((UINT128)1 << wordIndex);
    }

    for (auto& n : neg) {
        int wordIndex = distance(ic.begin(), ic.find(n));
        negBits |= ((UINT128)1 << wordIndex);
    }

    return true;
}

// ============= operations =============

enum class Opreation { Question = 0, Star = 1, Concatenate = 2, Or = 3, And = 4, Count = 5 };

std::string to_string(Opreation op) {
    switch (op)
    {
    case Opreation::Question:
        return "Q";
    case Opreation::Star:
        return "S";
    case Opreation::Concatenate:
        return "C";
    case Opreation::Or:
        return "O";
    case Opreation::And:
        return "&";
    default:
        break;
    }
    return "";
}

UINT128 processQuestion(UINT128 CS) {
    return CS | (UINT128)1;
}

UINT128 processStar(const GuideTable& guideTable, UINT128 CS) {

    CS |= 1;
    int ix = guideTable.alphabetSize + 1;
    UINT128 c = (UINT128)1 << ix;

    while (ix < guideTable.ICsize) {
        if (!(CS & c)) {
            for (auto [left, right] : guideTable.IterateRow(ix)) {
                if ((left & CS) && (right & CS))
                {
                    CS |= c; break;
                }
            }
        }
        c <<= 1; ix++;
    }
    return CS;
}

std::tuple<UINT128, UINT128> processConcatenate(const GuideTable& guideTable, UINT128 left, UINT128 right) {

    UINT128 CS1 = (UINT128)0;
    if (left & (UINT128)1) CS1 |= right;
    if (right & (UINT128)1) CS1 |= left;
    UINT128 CS2 = CS1;

    int ix = guideTable.alphabetSize + 1;
    UINT128 c = (UINT128)1 << ix;

    while (ix < guideTable.ICsize)
    {
        // when CS have value that means one of parts contains phi, check above
        if (!(CS1 & c)) {
            for (auto [l, r] : guideTable.IterateRow(ix))
                if ((l & left) && (r & right)) { CS1 |= c; break; }
        }

        if (!(CS2 & c)) {
            for (auto [l, r] : guideTable.IterateRow(ix))
                if ((l & right) && (r & left)) { CS2 |= c; break; }
        }

        c <<= 1; ix++;
    }

    return std::make_tuple(CS1, CS2);
}

UINT128 processOr(UINT128 left, UINT128 right) {
    return left | right;
}

UINT128 processAnd(UINT128 left, UINT128 right) {
    return left & right;
}

// ============= context =============

class CostIntervals {
public:
    CostIntervals(const unsigned short maxCost) {
        opCount = static_cast<int>(Opreation::Count);
        startPoints = new int[(maxCost + 2) * opCount]();
    }
    ~CostIntervals() {
        delete[] startPoints;
    }
    const std::tuple<int, int> Interval(int cost, Opreation start = Opreation::Question, Opreation end = Opreation::And) const {
        return std::make_tuple(this->start(cost, start), this->end(cost, end));
    }
    int& start(int cost, Opreation op) {
        return startPoints[cost * opCount + static_cast<int>(op)];
    }
    const int start(int cost, Opreation op) const {
        return startPoints[cost * opCount + static_cast<int>(op)];
    }
    int& end(int cost, Opreation op) {
        return startPoints[cost * opCount + static_cast<int>(op) + 1];
    }
    const int end(int cost, Opreation op) const {
        return startPoints[cost * opCount + static_cast<int>(op) + 1];
    }
    void indexToCost(int index, int& cost, Opreation& op) const {
        int i = 0;
        while (index >= startPoints[i]) { i++; }
        i--;
        cost = i / opCount;
        op = static_cast<Opreation>(i % opCount);
    }
private:
    int* startPoints;
    int opCount;
};

class ParesyContext {
public:
    ParesyContext(int cache_capacity, UINT128 posBits, UINT128 negBits)
        : cache_capacity(cache_capacity), posBits(posBits), negBits(negBits) {

        cache = new UINT128[cache_capacity + 1];
        leftRightIdx = new int[2 * (cache_capacity + 1)];
        lastIdx = 0;
        isFound = false;
        allREs = 0;
        onTheFly = false;
    }

    ~ParesyContext() {
        delete[] cache;
        delete[] leftRightIdx;
    }

    // Checking empty, epsilon, and the alphabet
    bool intialCheck(int alphaCost, const std::vector<std::string>& pos, const std::vector<std::string>& neg, std::string& RE)
    {
        // Initialisation of the alphabet

        for (auto& word : pos) for (auto ch : word) alphabet.insert(ch);
        for (auto& word : neg) for (auto ch : word) alphabet.insert(ch);

        LOG_OP((*this), alphaCost, std::string("A"), static_cast<int>(alphabet.size()) + 2)

            // Checking empty
            allREs++;
        if (pos.empty()) { RE = "Empty"; return true; }
        visited.insert((UINT128)0); // int(CS of empty) is 0

        // Checking epsilon
        allREs++;
        if ((pos.size() == 1) && (pos.at(0).empty())) { RE = "eps"; return true; }
        visited.insert((UINT128)1); // int(CS of empty) is 1

        // Checking the alphabet
        UINT128 idx = (UINT128)2; // Pointing to the position of the first char of the alphabet (idx 1 is for epsilon)
        auto alphabetSize = static_cast<int> (alphabet.size());

        for (int i = 0; lastIdx < alphabetSize; ++i) {

            cache[i] = idx;

            allREs++;

            std::string s(1, *next(alphabet.begin(), i));
            if ((pos.size() == 1) && (pos.at(0) == s)) { RE = s; return true; }

            visited.insert(idx);

            idx <<= 1;
            lastIdx++;
        }

        return false;
    }

    bool insertAndCheck(UINT128 CS, int index, Opreation op, std::string& RE) {
        return insertAndCheck(CS, index, -1, op, RE);
    }

    bool insertAndCheck(UINT128 CS, int lIndex, int rIndex, Opreation op, std::string& RE)
    {
        allREs++;
        if (onTheFly) {
            if ((CS & posBits) == posBits && (~CS & negBits) == negBits) {
                leftRightIdx[lastIdx << 1] = lIndex;
                if (rIndex > -1)
                    leftRightIdx[(lastIdx << 1) + 1] = rIndex;
                isFound = true;
                return isFound;
            }
        }
        else if (visited.insert(CS).second)
        {
            leftRightIdx[lastIdx << 1] = lIndex;
            if (rIndex > -1)
                leftRightIdx[(lastIdx << 1) + 1] = rIndex;
            if ((CS & posBits) == posBits && (~CS & negBits) == negBits) {
                isFound = true;
                return isFound;
            }
            cache[lastIdx++] = CS;
            if (lastIdx == cache_capacity) onTheFly = true;
        }
        return false;
    }

    std::set<char> alphabet;

    int cache_capacity;

    UINT128* cache;
    int* leftRightIdx;
    std::unordered_set<UINT128> visited;

    unsigned long allREs;
    // Index of the last free position in the language cache
    int lastIdx;
    bool isFound;
    bool onTheFly;
    UINT128 posBits, negBits;
};

// Adding parentheses if needed
std::string bracket(std::string s) {
    int p = 0;
    for (int i = 0; i < s.length(); i++) {
        if (s[i] == '(') p++;
        else if (s[i] == ')') p--;
        else if (s[i] == '+' && p <= 0) return "(" + s + ")";
    }
    return s;
}

// Generating the final RE string recursively
std::string toString(int index, const std::set<char>& alphabet, const int* leftRightIdx, const CostIntervals& intervals) {

    if (index == -2) return "eps"; // Epsilon
    if (index < alphabet.size()) { std::string s(1, *next(alphabet.begin(), index)); return s; }

    int cost; Opreation op;
    intervals.indexToCost(index, cost, op);

    if (op == Opreation::Question) {
        std::string res = toString(leftRightIdx[index << 1], alphabet, leftRightIdx, intervals);
        if (res.length() > 1) return "(" + res + ")?";
        return res + "?";
    }

    if (op == Opreation::Star) {
        std::string res = toString(leftRightIdx[index << 1], alphabet, leftRightIdx, intervals);
        if (res.length() > 1) return "(" + res + ")*";
        return res + "*";
    }

    if (op == Opreation::Concatenate) {
        std::string left = toString(leftRightIdx[index << 1], alphabet, leftRightIdx, intervals);
        std::string right = toString(leftRightIdx[(index << 1) + 1], alphabet, leftRightIdx, intervals);
        return bracket(left) + bracket(right);
    }

    if (op == Opreation::Or)
    {
        std::string left = toString(leftRightIdx[index << 1], alphabet, leftRightIdx, intervals);
        std::string right = toString(leftRightIdx[(index << 1) + 1], alphabet, leftRightIdx, intervals);
        return left + "+" + right;
    }

    if (op == Opreation::And)
    {
        std::string left = toString(leftRightIdx[index << 1], alphabet, leftRightIdx, intervals);
        std::string right = toString(leftRightIdx[(index << 1) + 1], alphabet, leftRightIdx, intervals);
        return left + "&" + right;
    }
}

Result REI(const unsigned short* costFun, const unsigned short maxCost, const std::vector<std::string>& pos, const std::vector<std::string>& neg) {

    Costs costs(costFun);

    GuideTable guideTable;
    UINT128 posBits{}, negBits{};
    generatingGuideTable(guideTable, posBits, negBits, pos, neg);

    const int cache_capacity = 200000000;

    ParesyContext context(cache_capacity, posBits, negBits);
    CostIntervals intervals(maxCost);

    std::string RE;

    if (context.intialCheck(costs.alpha, pos, neg, RE)) return Result(RE, 0, context.allREs, guideTable.ICsize);

    intervals.end(costs.alpha, Opreation::Concatenate) = context.lastIdx;
    intervals.end(costs.alpha, Opreation::Or) = context.lastIdx;
    intervals.end(costs.alpha, Opreation::And) = context.lastIdx;

    int shortageCost = -1; bool lastRound = false;

    int cost{};
    bool useQuestionOverOr = costs.alpha + costs.or >= costs.question;

    for (cost = costs.alpha + 1; cost <= maxCost; ++cost) {

        // Once it uses a previous cost that is not fully stored, it should continue as the last round
        if (context.onTheFly) {
            int dif = cost - shortageCost;
            if (dif == costs.question || dif == costs.star || dif == costs.alpha + costs.concat || dif == costs.alpha + costs. or || dif == costs.alpha + costs.and ) lastRound = true;
        }

        //Question Mark
        if (cost >= costs.alpha + costs.question && useQuestionOverOr) {

            // ignore results from (*) and (?)
            auto [start, end] = intervals.Interval(cost - costs.question, static_cast<Opreation>(2));
            LOG_OP(context, cost, to_string(Opreation::Question), end - start)
                for (auto i = start; i < end; i++)
                {
                    UINT128 CS = context.cache[i];
                    if (!(CS & (UINT128)1)) {
                        CS = processQuestion(CS);
                        if (context.insertAndCheck(CS, i, Opreation::Question, RE))
                        {
                            intervals.end(cost, Opreation::Question) = INT_MAX; goto exitEnumeration;
                        }
                    }
                }
        }
        intervals.end(cost, Opreation::Question) = context.lastIdx;

        //Star
        if (cost >= costs.alpha + costs.star) {
            // ignore results from (*) and (?)
            auto [start, end] = intervals.Interval(cost - costs.star, static_cast<Opreation>(2));
            LOG_OP(context, cost, to_string(Opreation::Star), end - start)
                for (auto i = start; i < end; i++)
                {
                    UINT128 CS = context.cache[i];
                    CS = processStar(guideTable, CS);
                    if (context.insertAndCheck(CS, i, Opreation::Star, RE))
                    {
                        intervals.end(cost, Opreation::Star) = INT_MAX; goto exitEnumeration;
                    }
                }
        }
        intervals.end(cost, Opreation::Star) = context.lastIdx;

        //Concat
        for (int i = costs.alpha; 2 * i <= cost - costs.concat; ++i) {

            auto [lstart, lend] = intervals.Interval(i);
            auto [rstart, rend] = intervals.Interval(cost - i - costs.concat);
            LOG_OP(context, cost, to_string(Opreation::Concatenate), 2 * (rend - rstart) * (lend - lstart))

            for (int l = lstart; l < lend; ++l) {
                UINT128 left = context.cache[l];
                for (int r = rstart; r < rend; ++r) {

                    auto [leftRight, rightLeft] = processConcatenate(guideTable, left, context.cache[r]);

                    if (context.insertAndCheck(leftRight, l, r, Opreation::Concatenate, RE))
                    {
                        intervals.end(cost, Opreation::Concatenate) = INT_MAX; goto exitEnumeration;
                    }

                    if (context.insertAndCheck(rightLeft, r, l, Opreation::Concatenate, RE))
                    {
                        intervals.end(cost, Opreation::Concatenate) = INT_MAX; goto exitEnumeration;
                    }
                }
            }

        }
        intervals.end(cost, Opreation::Concatenate) = context.lastIdx;

        //Or
        if (!useQuestionOverOr && cost >= 2 * costs.alpha + costs. or ) {

            auto [rstart, rend] = intervals.Interval(cost - costs.alpha - costs. or );
            LOG_OP(context, cost, to_string(Opreation::Or), rend - rstart)

            for (int r = rstart; r < rend; ++r) {

                UINT128 CS = processOr((UINT128)1, context.cache[r]);

                if (context.insertAndCheck(CS, -2, r, Opreation::Or, RE))
                {
                    intervals.end(cost, Opreation::Or) = INT_MAX; goto exitEnumeration;
                }
            }
        }
        for (int i = costs.alpha; 2 * i <= cost - costs. or ; ++i) {

            auto [lstart, lend] = intervals.Interval(i);
            auto [rstart, rend] = intervals.Interval(cost - i - costs. or );
            LOG_OP(context, cost, to_string(Opreation::Or), (rend - rstart) * (lend - lstart))

            for (int l = lstart; l < lend; ++l) {
                UINT128 left = context.cache[l];
                for (int r = rstart; r < rend; ++r) {

                    UINT128 CS = processOr(left, context.cache[r]);

                    if (context.insertAndCheck(CS, l, r, Opreation::Or, RE))
                    {
                        intervals.end(cost, Opreation::Or) = INT_MAX; goto exitEnumeration;
                    }
                }
            }
        }
        intervals.end(cost, Opreation::Or) = context.lastIdx;

        //And
        for (int i = costs.alpha; 2 * i <= cost - costs. and; ++i) {

            auto [lstart, lend] = intervals.Interval(i);
            auto [rstart, rend] = intervals.Interval(cost - i - costs. and);
            LOG_OP(context, cost, to_string(Opreation::And), (rend - rstart) * (lend - lstart))

                for (int l = lstart; l < lend; ++l) {
                    UINT128 left = context.cache[l];
                    for (int r = rstart; r < rend; ++r) {

                        UINT128 CS = processAnd(left, context.cache[r]);

                        if (context.insertAndCheck(CS, l, r, Opreation::And, RE))
                        {
                            intervals.end(cost, Opreation::And) = INT_MAX; goto exitEnumeration;
                        }
                    }
                }
        }
        intervals.end(cost, Opreation::And) = context.lastIdx;

        if (lastRound) break;
        if (context.onTheFly && shortageCost == -1) shortageCost = cost;
    }

exitEnumeration:

    if (context.isFound)
    {
        RE = toString(context.lastIdx, context.alphabet, context.leftRightIdx, intervals);
        return Result(RE, cost, context.allREs, guideTable.ICsize);
    }

    if (cost == maxCost + 1) cost--;

    return Result("not_found", cost, context.allREs, guideTable.ICsize);
}