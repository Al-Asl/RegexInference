#include <string>
#include <vector>
#include <chrono>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

#include "rei_dc.hpp"
#include "rei_util.hpp"

int runOnDirectory(int argc, char* argv[])
{
    if (argc != 10) {
        printf("Arguments should be in the form of\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s <input_dir_address> <window_size> <max_time> <c1> <c2> <c3> <c4> <c5> <maxCost>\n", argv[0]);
        printf("-----------------------------------------------------------------\n");
        printf("\nFor example\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s ./input 12 60 1 1 1 1 1 500\n", argv[0]);
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

    unsigned short window_size = std::atoi(argv[2]);
    unsigned short max_time = std::atoi(argv[3]);

    unsigned short costFun[5];
    for (int i = 0; i < 5; i++)
        costFun[i] = std::atoi(argv[i + 4]);
    unsigned short maxCost = std::atoi(argv[9]);

    std::vector<fs::path> files;
    try {
        for (const auto& entry : fs::directory_iterator(argv[1])) {
            if (entry.is_regular_file()) {
                if (entry.path().extension() == ".txt") {
                    files.push_back(entry.path());
                }
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << '\n';
    }

    for (size_t i = 0; i < files.size(); i++)
    {
        std::vector<std::string> pos, neg;
        if (!paresy_s::readFile(files[i].string(), pos, neg)) {
            printf("\nCan't read the file: %s\n", files[i].stem().c_str());
            continue;
        }

        auto profileInfo = paresy_s::RecursiveProfileInfo();

        auto start = std::chrono::high_resolution_clock::now();
        auto result = paresy_s::detSplit(window_size, costFun, maxCost, pos, neg, max_time, profileInfo);
        auto stop = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
        printf("\nPositive: "); for (const auto& p : pos) printf("\"%s\" ", p.c_str());
        printf("\nNegative: "); for (const auto& n : neg) printf("\"%s\" ", n.c_str());
        printf("\nRunning Time: %f s", (double)duration * 0.000001);
        printf("\nCall count: %d, Max depth: %d\n", profileInfo.callCount, profileInfo.maxDepth);
        printf("\n\nRE: \"%s\"\n", result.c_str());
    }

    return 0;
}

int runOnFile(int argc, char* argv[])
{

// -----------------
// Reading the input
// -----------------

#ifndef HARD_CODED_INPUT
    if (argc != 10) {
        printf("Arguments should be in the form of\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s <file_address> <window_size> <max_time> <c1> <c2> <c3> <c4> <c5> <maxCost>\n", argv[0]);
        printf("-----------------------------------------------------------------\n");
        printf("\nFor example\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s ./input 12 60 1 1 1 1 1 500\n", argv[0]);
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

    unsigned short window_size = std::atoi(argv[2]);
    unsigned short max_time = std::atoi(argv[3]);

    unsigned short costFun[5];
    for (int i = 0; i < 5; i++)
        costFun[i] = std::atoi(argv[i + 4]);
    unsigned short maxCost = std::atoi(argv[9]);
#else

    std::string text = R"(
++
"00"
"0111"
"110"
"1100"
"000"
"0"
"1000"
"1101"
"0101"
"11"
"010"
--
""
"0000"
"0001"
"01"
"0100"
"011"
"1"
"100"
"101"
"1011"
"111"
)";

    unsigned short* costFun = new unsigned short[5];
    costFun[0] = 1;
    costFun[1] = 1;
    costFun[2] = 1;
    costFun[3] = 1;
    costFun[4] = 1;
    const unsigned short maxCost = 500;
    const unsigned short window_size = 100;
    double max_time = 60;

    std::vector<std::string> pos, neg;
    if (!paresy_s::readStream(std::istringstream(text), pos, neg)) return 0;

#endif


    // ----------------------------------
    // Regular Expression Inference (REI)
    // ----------------------------------

    auto start = std::chrono::high_resolution_clock::now();

    paresy_s::RecursiveProfileInfo profileInfo;
    auto result = paresy_s::detSplit(window_size, costFun, maxCost, pos, neg, max_time, profileInfo);

    auto stop = std::chrono::high_resolution_clock::now();

    // -------------------
    // Printing the output
    // -------------------

    printf("\nPositive: "); for (const auto& p : pos) printf("\"%s\" ", p.c_str());
    printf("\nNegative: "); for (const auto& n : neg) printf("\"%s\" ", n.c_str());
    printf("\nCost Function: \"a\"=%u, \"?\"=%u, \"*\"=%u, \".\"=%u, \"+\"=%u",
        costFun[0], costFun[1], costFun[2], costFun[3], costFun[4]);
    printf("\nCall count: %d, Max depth: %d\n", profileInfo.callCount, profileInfo.maxDepth);
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    printf("\nRunning Time: %f s", (double)duration * 0.000001);
    printf("\n\nRE: \"%s\"\n", result.c_str());

    return 0;
}

int main(int argc, char* argv[]) {

#ifdef BATCH_MODE
    return runOnDirectory(argc, argv);
#else
    return runOnFile(argc, argv);
#endif
}