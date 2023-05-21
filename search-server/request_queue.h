#pragma once

#include <stack>
#include <vector>
#include <string>
#include <iostream>

#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);


    int GetNoResultRequests() const;



private:
    struct QueryResult {
        bool isempty_;
        uint64_t timestamp;
    };
    std::deque<QueryResult> requests_;

    const static int min_in_day_ = 1440;

    const SearchServer& search_server_;
    size_t clock_ = 0;
    int empty_count_ = 0;

    bool IsNewDay();

    void NewRequest(const std::vector<Document>& doc);

};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> result = search_server_.FindTopDocuments<DocumentPredicate>(raw_query, document_predicate);
    NewRequest(result);
    return result;
}
