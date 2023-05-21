#include <algorithm>
#include <execution>
#include <iterator>

#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result_to_return(queries.size());
	std::transform(
		std::execution::par,
		queries.begin(), queries.end(), //
		result_to_return.begin(),
		[&search_server](std::string str) {return search_server.FindTopDocuments(str); }
	);
	return result_to_return;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result(queries.size());
	//std::list<Document> result_to_return;

	std::transform(
		std::execution::par,
		queries.begin(), queries.end(), //
		result.begin(),
		[&search_server](std::string str) {return search_server.FindTopDocuments(str); }
	);
	{
		bool is_first = true;
		for (std::vector<Document>& docs : result) {
			if (is_first) {
				is_first = false;
			}
			else {
				result[0].insert(result[0].end(), std::move_iterator(docs.begin()), std::move_iterator(docs.end()));
			}

		}
	}

	return result[0];
}