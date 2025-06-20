#include <string>
#include <vector>
#include <chrono>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

#include "dc_paresy.hpp"
#include "rei_util.hpp"

int runOnDirectory(int argc, char* argv[])
{
    if (argc != 9) {
        printf("Arguments should be in the form of\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s <dir_address> <window_size> <c1> <c2> <c3> <c4> <c5> <maxCost>\n", argv[0]);
        printf("-----------------------------------------------------------------\n");
        printf("\nFor example\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s ./input 12 1 1 1 1 1 500\n", argv[0]);
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

    unsigned short costFun[5];
    for (int i = 0; i < 5; i++)
        costFun[i] = std::atoi(argv[i + 3]);
    unsigned short maxCost = std::atoi(argv[8]);

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
        if (!readFile(files[i].string(), pos, neg)) {
            printf("\nCan't read the file: %s\n", files[i].stem().c_str());
            continue;
        }

        auto profileInfo = RecursiveProfileInfo();

        auto start = std::chrono::high_resolution_clock::now();
        auto result = detSplit(window_size, costFun, maxCost, pos, neg, profileInfo);
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

#if USER_INPUT
    if (argc != 9) {
        printf("Arguments should be in the form of\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s <dir_address> <window_size> <c1> <c2> <c3> <c4> <c5> <maxCost>\n", argv[0]);
        printf("-----------------------------------------------------------------\n");
        printf("\nFor example\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s ./input 12 1 1 1 1 1 500\n", argv[0]);
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
    if (!readFile(fileName, pos, neg)) return 0;

    unsigned short window_size = std::atoi(argv[2]);

    unsigned short costFun[5];
    for (int i = 0; i < 5; i++)
        costFun[i] = std::atoi(argv[i + 3]);
    unsigned short maxCost = std::atoi(argv[8]);
#else

    std::string text = R"(
++
"10"
"101"
"100"
"1010"
"1011"
"1000"
"1001"
--
""
"0"
"1"
"00"
"11"
"010"
    )";

    unsigned short* costFun = new unsigned short[5];
    costFun[0] = 1;
    costFun[1] = 1;
    costFun[2] = 1;
    costFun[3] = 1;
    costFun[4] = 1;
    const unsigned short maxCost = 500;
    const unsigned short window_size = 12;

    std::vector<std::string> pos, neg;
    if (!readStream(std::istringstream(text), pos, neg)) return 0;

#endif


    // ----------------------------------
    // Regular Expression Inference (REI)
    // ----------------------------------

#if MODE == 0
    auto start = std::chrono::high_resolution_clock::now();
#endif

    RecursiveProfileInfo profileInfo;
    auto result = detSplit(window_size, costFun, maxCost, pos, neg, profileInfo);

#if MODE == 0
    auto stop = std::chrono::high_resolution_clock::now();
#endif

    // -------------------
    // Printing the output
    // -------------------

    printf("\nPositive: "); for (const auto& p : pos) printf("\"%s\" ", p.c_str());
    printf("\nNegative: "); for (const auto& n : neg) printf("\"%s\" ", n.c_str());
    printf("\nCost Function: \"a\"=%u, \"?\"=%u, \"*\"=%u, \".\"=%u, \"+\"=%u",
        costFun[0], costFun[1], costFun[2], costFun[3], costFun[4]);
    printf("\nCall count: %d, Max depth: %d\n", profileInfo.callCount, profileInfo.maxDepth);
#if MODE == 0
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    printf("\nRunning Time: %f s", (double)duration * 0.000001);
#endif
    printf("\n\nRE: \"%s\"\n", result.c_str());

    return 0;
}

int main(int argc, char* argv[]) {

#if MODE == 2 
    return runOnDirectory(argc, argv);
#else
    return runOnFile(argc, argv);
#endif
}