#pragma once

#include <vector>
#include <string>
#include <string_view>

std::vector<std::string> SplitIntoWords_old(const std::string text);

std::vector<std::string_view> SplitIntoWords(const std::string_view text);