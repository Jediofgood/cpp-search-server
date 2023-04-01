#include "remove_duplicates.h"

#include <algorithm>
#include <set>

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
	//std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
	auto right = search_server.end();
	std::set<int> todel;

	for (auto left = search_server.begin(); left != right; ++left) {   // по Н. 

		if (todel.count(*left)) {
			continue;
		}

		else {
			const std::map<std::string, double> currmap = search_server.GetWordFrequencies(*left);

			for (auto left1 = next(left, 1); left1 != right; ++left1) {
				const std::map<std::string, double> tocheckmap = search_server.GetWordFrequencies(*left1);
				
				if (map_key_cheker(currmap, tocheckmap)) {
					todel.emplace(*left1);
				}
			}
		}
	}

	for (int i : todel) {
		std::cout << "Found duplicate document id "s << i << std::endl;
		search_server.RemoveDocument(i);
	}
	//std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
}

bool map_key_cheker(const std::map <std::string, double>& map1, const std::map <std::string, double>& map2) {
	return map1.size() == map2.size() && std::equal(map1.begin(), map1.end(), map2.begin(), [](auto s1, auto s2) {return s1.first == s2.first; });
}



