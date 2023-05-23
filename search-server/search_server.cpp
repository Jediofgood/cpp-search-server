#include "search_server.h"
#include "read_input_functions.h"

#include <utility>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace std::string_literals; //


void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("wrong id"s);
    }
    if (documents_.count(document_id)) {
        throw std::invalid_argument("already exist id"s);
    }
    if (!IsValidWord(document)) {
        throw std::invalid_argument("wrong document"s);
    }

    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();

    for (const std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;

        documents_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    index_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status1) const {
    return FindTopDocuments(raw_query, [status1](int document_id, DocumentStatus status, int rating) { return status == status1; });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy exec, const std::string_view raw_query, DocumentStatus status1) const {
    return FindTopDocuments(raw_query, [status1](int document_id, DocumentStatus status, int rating) { return status == status1; });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy exec, const std::string_view raw_query, DocumentStatus status1) const {
    return FindTopDocuments(exec, raw_query, [status1](int document_id, DocumentStatus status, int rating) { return status == status1; });
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

vector_of_matched SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Wrong Words"s);
    }
    if (!index_.count(document_id)) {
        throw std::out_of_range("Document id is out of range"s);
    }

    const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
    }

    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    
    return { matched_words, documents_.at(document_id).status };
}


bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text){
    std::vector<std::string_view> words;

    for (const std::string_view word : SplitIntoWords(text)) {

        if (!IsStopWord(word)) {
            if (string_set_.count(word)) {
                words.push_back(*string_set_.find(word));
            }
            else {
                auto res = string_set_.insert(std::string{ word.begin(), word.end() });
                words.push_back(*res.first);
            }
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        //text = text.substr(1);
        text.remove_prefix(1); 
    }
    if (text.empty()) {
        throw std::invalid_argument("пустой минус"s);
    }
    if (text[0] == '-') {
        throw std::invalid_argument("лишний минус"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool is_for_par) const {
    if (!IsValidWord(text)) {
        throw std::invalid_argument("Спец символ в минус запросе"s);
    }
    Query query;

    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }

    if (!is_for_par)
    {
        std::sort(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());

        std::sort(query.minus_words.begin(), query.minus_words.begin());
        query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
    }

    return query;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    const static std::map<std::string_view, double> emptymap;
    return (documents_word_freqs_.count(document_id) ? documents_word_freqs_.at(document_id) : emptymap);
}

void SearchServer::RemoveDocument(int document_id) {

    if (documents_word_freqs_.count(document_id)) {
        documents_.erase(document_id);
        index_.erase(find(index_.begin(), index_.end(), document_id));

        for (const auto& [word, freq] : documents_word_freqs_[document_id]) {
            word_to_document_freqs_[word].erase(document_id);

            if (!word_to_document_freqs_[word].size()) {
                word_to_document_freqs_.erase(word);
            }
        }

        documents_word_freqs_.erase(document_id);

    }
}

vector_of_matched SearchServer::MatchDocument(std::execution::sequenced_policy exec, const std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}


vector_of_matched SearchServer::MatchDocument(std::execution::parallel_policy exec, const std::string_view raw_query, int document_id) const {
    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Wrong Words"s);
    }
    if (!index_.count(document_id)) {
        throw std::out_of_range("Document id is out of range"s);
    }

    Query query = ParseQuery(raw_query, true);

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [&](std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                return (word_to_document_freqs_.at(word).count(document_id) != 0);
            }
            return false;
        }))
    {
        return { std::vector<std::string_view>{}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto last = std::copy_if(std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        [&](std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                return word_to_document_freqs_.at(word).count(document_id) != 0;
            }
            return false;
        }
    );

    matched_words.erase(last, matched_words.end());

    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(std::execution::par, matched_words.begin(), matched_words.end()), matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}
