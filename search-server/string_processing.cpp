//
// Created by rustam on 06.03.2022.
//

#include "string_processing.h"

// Разделение на слова
std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string> SplitIntoWords2(const std::string& text) {
    std::string_view text2 = text;
    std::vector<std::string> words;
    for (size_t pos = 0; pos != text2.npos; text2.remove_prefix(pos + 1)) {
        pos = text2.find(' ');
        words.emplace_back(std::string (text2.substr(0, pos)));
    }
    return words;
}


