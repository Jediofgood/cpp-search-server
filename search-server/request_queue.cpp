#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    :search_server_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document>result = search_server_.FindTopDocuments(raw_query, status);
    NewRequest(result);
    return result;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    NewRequest(result);
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_count_;
}

bool RequestQueue::IsNewDay() {
    if (clock_ >= min_in_day_) {
        ++clock_;
        return true;
    }
    else {
        ++clock_;
        return false;
    }
}

void RequestQueue::NewRequest(const std::vector<Document>& doc) {
    if (!IsNewDay()) {
        RequestQueue::QueryResult toadd;
        bool flag = (doc.begin() == doc.end());
        toadd.isempty_ = flag;
        if (flag) {
            ++empty_count_;
        }
        toadd.timestamp = clock_;
        //toadd.search_result_ = doc;
        requests_.push_back(toadd);
    }
    else {
        QueryResult toadd;
        bool flag = (doc.begin() == doc.end());
        toadd.isempty_ = flag;
        if (flag) {
            ++empty_count_;
        }
        toadd.timestamp = clock_;
        //toadd.search_result_ = doc;
        if (requests_.front().isempty_) {
            --empty_count_;
        }
        requests_.pop_front();
        requests_.push_back(toadd);
    }
}