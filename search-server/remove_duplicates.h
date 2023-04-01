#pragma once

#include "search_server.h"
#include <map>

void RemoveDuplicates(SearchServer& search_server);

bool map_key_cheker(const std::map <std::string, double>& map1, const std::map <std::string, double>& map2);
