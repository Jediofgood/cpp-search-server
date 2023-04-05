#include "remove_duplicates.h"
#include "log_duration.h"

#include <algorithm>
#include <set>

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
	std::set<int> id_to_delete;

	std::set<std::set<std::string>> words_set;

	for (int doc_id : search_server) {
		std::set< std::string> words;

		for (const auto& [word, nouseddouble] : search_server.GetWordFrequencies(doc_id)) {
			words.insert(word);
		}
		if (!words_set.emplace(words).second) {
			id_to_delete.insert(doc_id);
		}
	}

	for (int id : id_to_delete) {
		std::cout << "Found duplicate document id "s << id << std::endl;
		search_server.RemoveDocument(id);
	}
}