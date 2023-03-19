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
        //В примере решения был TimeStamp
        //В теории говорилось про хранение запросов, которые привели к пустому ответу.
        //Название переменной подразумевает резьтат.
        //Подскажите, пожалуйста, что нужно в этой структуре в будущем? Или мы не вернёмся к ней в будущем?

        bool isempty_;
        uint64_t timestamp;
        //string raw_query_; 
        //std::vector<Document> search_result_;
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
