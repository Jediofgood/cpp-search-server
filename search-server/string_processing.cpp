#include "string_processing.h"


std::vector<std::string> SplitIntoWords_old(const std::string text) {
    std::vector<std::string> words;
    std::string word;

    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size())); // удаляем пробелы 

    const int64_t pos_end = str.npos;

    while (str.size() != 0) {
        int64_t space = str.find(' ');
        result.push_back(space == pos_end ? str.substr(0, pos_end) : (str.substr(0, space)));
        str.remove_prefix(std::min(static_cast<size_t>(space), str.size()));
        str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    }
    return result;
}