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


    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result = search_server_.FindTopDocuments<DocumentPredicate>(raw_query, document_predicate);
        NewRequest(result);
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);


    int GetNoResultRequests() const;



private:
    struct QueryResult {
        bool isempty_;
        //string raw_query_;
        std::vector<Document> searchresult_;
    };
    std::deque<QueryResult> requests_;

    const static int min_in_day_ = 1440;
    // возможно, здесь вам понадобится что-то ещё

    const SearchServer& search_server_;
    size_t clock_ = 0;
    int empty_count_ = 0;

    bool IsNewDay();

    void NewRequest(const std::vector<Document>& doc);

};
