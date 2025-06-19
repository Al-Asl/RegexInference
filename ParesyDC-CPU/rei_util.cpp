#include <rei_util.hpp>

#include <fstream>

bool readStream(std::istream& stream, std::vector<std::string>& pos, std::vector<std::string>& neg) {
    std::string line;

    while (line != "++") {
        getline(stream, line);
        if (stream.eof()) {
            printf("Unable to find \"++\" for positive words");
            printf("\nPlease check the input file.\n");
            return false;
        }
    }

    getline(stream, line);

    while (line != "--") {
        std::string word = "";
        for (auto c : line) if (c != ' ' && c != '"') word += c;
        pos.push_back(word);
        getline(stream, line);
        if (line != "--" && stream.eof()) {
            printf("Unable to find \"--\" for negative words");
            printf("\nPlease check the input file.\n");
            return false;
        }
    }

    while (getline(stream, line)) {
        std::string word = "";
        for (auto c : line) if (c != ' ' && c != '"') word += c;
        for (auto& p : pos) {
            if (word == p) {
                printf("\"%s\" is in both Pos and Neg examples", word.c_str());
                printf("\nPlease check the input file, and remove one of those.\n");
                return false;
            }
        }
        neg.push_back(word);
    }

    return true;
}

// Reading the input file
bool readFile(const std::string& fileName, std::vector<std::string>& pos, std::vector<std::string>& neg) {

    std::ifstream textFile(fileName);

    if (textFile.is_open()) {

        if (!readStream(textFile, pos, neg)) return false;

        textFile.close();

        return true;
    }

    printf("Unable to open the file");
    return false;

}

//std::vector<std::string> CSToStrings(const std::set<std::string, strComparison>& ic, UINT128 cs) {
//    std::vector<std::string> res;
//    int i = 0;
//    while (cs > 0) {
//        if ((cs & 1) == 1)
//        {
//            auto it = ic.begin();
//            std::advance(it, i);
//            res.push_back(*it);
//        }
//        cs >>= 1; i++;
//    }
//    return res;
//}

//UINT128 stringsToCS(const std::set<std::string, strComparison>& ic, const std::string* begin, const std::string* end) {
//    UINT128 res{};
//    for (auto it = begin; it != end; ++it) {
//        auto itc = std::find(ic.begin(), ic.end(), *it);
//        if (itc != ic.end()) {
//            int index = std::distance(ic.begin(), itc);
//            res |= 1 << index;
//        }
//    }
//    return res;
//}

//void print(const std::string* begin, const std::string* end) {
//    for (auto it = begin; it != end; ++it) {
//        std::cout << *it;
//        if (std::next(it) != end) {
//            std::cout << ',';
//        }
//    }
//}