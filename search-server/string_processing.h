//
// Created by rustam on 06.03.2022.
//
#pragma once
#include <vector>
#include <set>
#include <string>
#include <string_view>


std::vector<std::string> SplitIntoWords(const std::string& text);

std::vector<std::string_view> SplitIntoWordsStrView(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeStringContainerNotEmpty(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}