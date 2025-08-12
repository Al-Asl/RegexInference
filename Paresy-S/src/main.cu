#include <string>
#include <vector>
#include <chrono>
#include <cassert>
#include <cmath>

#include <regex_match.hpp>
#include "rei_dc.hpp"
#include "rei_util.hpp"

int calculateCost(const std::string& pattren, unsigned short* costFun) {
    auto counts = paresy_s::countOpreations(pattren);
    int count = 0;
    count += counts.alpha * costFun[0];
    count += counts.question * costFun[1];
    count += counts.star * costFun[2];
    count += counts.concat * costFun[3];
    count += counts.alternation * costFun[4];
    count += counts.intersection * costFun[5];
    return count;
}

int main(int argc, char* argv[]) {

#ifndef EVALUATION_MODE
// -----------------
// Reading the input
// -----------------

#ifndef HARD_CODED_INPUT
    if (argc != 12) {
        printf("Arguments should be in the form of\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s <file_address> <dc_type> <window_size> <max_time> <c1> <c2> <c3> <c4> <c5> <c6> <max_cost>\n", argv[0]);
        printf("-----------------------------------------------------------------\n");
        printf("\nFor example\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s ./input 1 12 60 1 1 1 1 1 1 500\n", argv[0]);
        printf("-----------------------------------------------------------------\n");
        return 0;
    }

    bool argError = false;
    for (int i = 2; i < argc; ++i) {
        if (std::atoi(argv[i]) <= 0 || std::atoi(argv[i]) > SHRT_MAX) {
            printf("Argument number %d, \"%s\", should be a positive short integer.\n", i, argv[i]);
            argError = true;
        }
    }
    if (argError) return 0;

    std::string fileName = argv[1];
    std::vector<std::string> pos, neg;
    if (!paresy_s::readFile(fileName, pos, neg)) return 0;

    unsigned short dc_type = std::atoi(argv[2]);
    unsigned short window_size = std::atoi(argv[3]);
    unsigned short max_time = std::atoi(argv[4]);

    unsigned short costFun[6];
    for (int i = 0; i < 6; i++)
        costFun[i] = std::atoi(argv[i + 5]);
    unsigned short maxCost = std::atoi(argv[11]);
#else

    std::string text = R"(
++
"00"
"1101"
"0001"
"0111"
"001"
"1"
"10"
"1100"
"111"
"1010"
--
""
"0"
"0000"
"0011"
"01"
"010"
"011"
"100"
"1000"
"1001"
"11"
"1110"
)";

    unsigned short* costFun = new unsigned short[5];
    costFun[0] = 1;
    costFun[1] = 1;
    costFun[2] = 1;
    costFun[3] = 1;
    costFun[4] = 1;
    costFun[5] = 1;
    const unsigned short maxCost = 500;
    unsigned short dc_type = 1;
    const unsigned short window_size = 30;
    double max_time = 120;

    std::vector<std::string> pos, neg;
    auto stream = std::istringstream(text);
    if (!paresy_s::readStream(stream, pos, neg)) return 0;

#endif


    // ----------------------------------
    // Regular Expression Inference (REI)
    // ----------------------------------

    paresy_s::RecursiveProfileInfo profileInfo;

    auto start = std::chrono::high_resolution_clock::now();

    std::string result;
    if (dc_type == 1)
        result = paresy_s::randSplit(window_size, costFun, maxCost, pos, neg, max_time, profileInfo);
    else
        result = paresy_s::detSplit(window_size, costFun, maxCost, pos, neg, max_time, profileInfo);

    auto stop = std::chrono::high_resolution_clock::now();

    // -------------------
    // Printing the output
    // -------------------


    printf("\nPositive: "); for (const auto& p : pos) printf("\"%s\" ", p.c_str());
    printf("\nNegative: "); for (const auto& n : neg) printf("\"%s\" ", n.c_str());
    printf("\nCost Function: \"a\"=%u, \"?\"=%u, \"*\"=%u, \".\"=%u, \"+\"=%u",
        costFun[0], costFun[1], costFun[2], costFun[3], costFun[4]);
    auto finalCost = calculateCost(result, costFun);
    printf("\nFinal Cost: %u", finalCost);
    printf("\nCall count: %d, Max depth: %d\n", profileInfo.callCount, profileInfo.maxDepth);
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    printf("\nRunning Time: %f s", (double)duration * 0.000001);
    printf("\n\nRE: \"%s\"\n", result.c_str());

    return 0;

#else

// -----------------
// Reading the input
// -----------------

if (argc != 13) {
    printf("Arguments should be in the form of\n");
    printf("-----------------------------------------------------------------\n");
    printf("%s <file_address> <dc_type> <window_size> <max_time> <train_ratio> <c1> <c2> <c3> <c4> <c5> <c6> <max_cost>\n", argv[0]);
    printf("-----------------------------------------------------------------\n");
    printf("\nFor example\n");
    printf("-----------------------------------------------------------------\n");
    printf("%s ./input 1 12 60 50 1 1 1 1 1 1 500\n", argv[0]);
    printf("-----------------------------------------------------------------\n");
    return 0;
}

bool argError = false;
for (int i = 3; i < argc; ++i) {
    if (std::atoi(argv[i]) <= 0 || std::atoi(argv[i]) > SHRT_MAX) {
        printf("Argument number %d, \"%s\", should be a positive short integer.\n", i, argv[i]);
        argError = true;
    }
}
if (argError) return 0;

std::string fileName = argv[1];
std::vector<std::string> pos, neg;
if (!paresy_s::readFile(fileName, pos, neg)) return 0;

unsigned short dc_type = std::atoi(argv[2]);
unsigned short window_size = std::atoi(argv[3]);
unsigned short max_time = std::atoi(argv[4]);
unsigned short train_ratio = std::atoi(argv[5]);

unsigned short costFun[6];
for (int i = 0; i < 6; i++)
    costFun[i] = std::atoi(argv[i + 6]);
unsigned short maxCost = std::atoi(argv[12]);

// ----------------------------------
// Regular Expression Inference (REI)
// ----------------------------------

auto midPos = pos.begin() + std::roundl(pos.size() * train_ratio / 100.0);
std::vector<std::string> pos_train(pos.begin(), midPos);
std::vector<std::string> pos_test(midPos, pos.end());

midPos = neg.begin() + std::roundl(neg.size() * train_ratio / 100.0);
std::vector<std::string> neg_train(neg.begin(), midPos);
std::vector<std::string> neg_test(midPos, neg.end());

paresy_s::RecursiveProfileInfo profileInfo;

auto start = std::chrono::high_resolution_clock::now();

std::string result;
if (dc_type == 1)
result = paresy_s::randSplit(window_size, costFun, maxCost, pos_train, neg_train, max_time, profileInfo);
else
result = paresy_s::detSplit(window_size, costFun, maxCost, pos_train, neg_train, max_time, profileInfo);

auto stop = std::chrono::high_resolution_clock::now();

int tp = 0, fp = 0;
for (const auto& p : pos_test) {
    if (match(result, p))
    {
        tp++;
    }
    else
    {
        fp++;
    }
}

int tn = 0, fn = 0;
for (const auto& n : neg_test) {
    if (match(result, n))
    {
        fn++;
    }
    else
    {
        tn++;
    }
}

float accuracy = tp + tn + fp + fn == 0 ? 0 : (tn + tp) / static_cast<float>(tp + tn + fp + fn);
float precision = tp + fp == 0 ? 0 : tp / static_cast<float>(tp + fp);
float recall = tp + fn == 0 ? 0 : tp / static_cast<float>(tp + fn);
float f1 = precision + recall == 0 ? 0 : 2 * (precision * recall) / (precision + recall);

// -------------------
// Printing the output
// -------------------

printf("\nPositive: "); for (const auto& p : pos_train) printf("\"%s\" ", p.c_str());
printf("\nNegative: "); for (const auto& n : neg_train) printf("\"%s\" ", n.c_str());
printf("\nCost Function: \"a\"=%u, \"?\"=%u, \"*\"=%u, \".\"=%u, \"+\"=%u",
    costFun[0], costFun[1], costFun[2], costFun[3], costFun[4]);
auto finalCost = calculateCost(result, costFun);
printf("\nFinal Cost: %u", finalCost);
printf("\nTruePositive=%u, FalsePositive=%u, TrueNegative=%u, FalseNegative=%u", tp, fp, tn, fn);
printf("\nAccuracy=%f, Precision=%f, Recall=%f, F1-score=%f", accuracy, precision, recall, f1);
printf("\nCall count: %d, Max depth: %d\n", profileInfo.callCount, profileInfo.maxDepth);
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
printf("\nRunning Time: %f s", (double)duration * 0.000001);
printf("\n\nRE: \"%s\"\n", result.c_str());

return 0;

#endif
}