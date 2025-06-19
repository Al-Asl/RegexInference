#include <string>
#include <vector>
#include <chrono>

#include <rei.hpp>
#include <rei_util.hpp>
#include <dc_paresy.hpp>

int main(int argc, char* argv[]) {

    // -----------------
    // Reading the input
    // -----------------

#if USER_INPUT

    if (argc != 9) {
        printf("Arguments should be in the form of\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s <input_file_address> <c1> <c2> <c3> <c4> <c5> <c6> <maxCost>\n", argv[0]);
        printf("-----------------------------------------------------------------\n");
        printf("\nFor example\n");
        printf("-----------------------------------------------------------------\n");
        printf("%s ./input.txt 1 1 1 1 1 1 500\n", argv[0]);
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
    unsigned short costFun[6];
    for (int i = 0; i < 6; i++)
        costFun[i] = std::atoi(argv[i + 2]);
    unsigned short maxCost = std::atoi(argv[8]);

#else

    std::string text = R"(
++
"0111"
"10011"
"0011"
"000"
""
"1001"
"01110"
"1101"
--
"0"
"00000"
"1"
"10"
"101"
"1010"
"10101"
"10111"
"1110"
)";

    unsigned short* costFun = new unsigned short[6];
    costFun[0] = 1;
    costFun[1] = 1;
    costFun[2] = 1;
    costFun[3] = 1;
    costFun[4] = 1;
    costFun[5] = 1;
    const unsigned short maxCost = 500;

    std::vector<std::string> pos, neg;
    if (!readStream(std::istringstream(text), pos, neg)) return 0;

#endif

    // ----------------------------------
    // Regular Expression Inference (REI)
    // ----------------------------------

#if MEASUREMENT_MODE
    auto start = std::chrono::high_resolution_clock::now();
#endif

    auto result = detSplit(8, costFun, maxCost, pos, neg);
    //auto result = REI(costFun, maxCost, pos, neg).RE;

#if MEASUREMENT_MODE
    auto stop = std::chrono::high_resolution_clock::now();
#endif

    // -------------------
    // Printing the output
    // -------------------

    printf("\nPositive: "); for (const auto& p : pos) printf("\"%s\" ", p.c_str());
    printf("\nNegative: "); for (const auto& n : neg) printf("\"%s\" ", n.c_str());
    printf("\nCost Function: \"a\"=%u, \"?\"=%u, \"*\"=%u, \".\"=%u, \"+\"=%u \"&\"=%u",
        costFun[0], costFun[1], costFun[2], costFun[3], costFun[4], costFun[5]);
#if MEASUREMENT_MODE
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    printf("\nRunning Time: %f s", (double)duration * 0.000001);
#endif
    printf("\n\nRE: \"%s\"\n", result.c_str());

    delete[] costFun;

    return 0;
}