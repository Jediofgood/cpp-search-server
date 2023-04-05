#include "remove_duplicates.h"
#include "log_duration.h"

#include <algorithm>
#include <set>

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
	std::set<int> id_to_delete;

	std::map< std::set< std::string>, std::set<int>> wordsset_idset;

	std::map< int, std::set< std::string>> id_wordsset;


	for (int doc_id : search_server) { //O(N)
		std::set< std::string> words;

		for (const auto& [word, nouseddouble] : search_server.GetWordFrequencies(doc_id)) {	//O(w)
			words.insert(word);	//log(w)
		}

		wordsset_idset[words].insert(doc_id); //Log(N)
		id_wordsset[doc_id] = words; //Log(N)
	}//O(N (Log (w) + Log(N))

	//сама реализация
	for (int doc_id : search_server) {	//O(N)
		if (id_to_delete.count(doc_id)) {
			continue;
		}
		else {
			const std::set<int>& cheked_id = wordsset_idset[id_wordsset.at(doc_id)];
			if (cheked_id.size() != 1) {  //O(1)
				id_to_delete.insert(next(cheked_id.begin(), 1), cheked_id.end()); //В наихудшем случае с учётом цикла - O(N);
			}
		}
	}


	for (int id : id_to_delete) {
		std::cout << "Found duplicate document id "s << id << std::endl;
		search_server.RemoveDocument(id);
	}
	//
	//std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
}



