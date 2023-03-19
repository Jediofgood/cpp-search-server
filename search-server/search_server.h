#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

using namespace std::literals;

class SearchServer {
public:
    explicit SearchServer(const std::string& text) :SearchServer(SplitIntoWords(text))
    {
    }

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words);


    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status1 = DocumentStatus::ACTUAL) const;

    template <typename KeyMapper>                                                             
    std::vector<Document> FindTopDocuments(const std::string& raw_query, KeyMapper keymapper) const;

    size_t GetDocumentCount() const;

    int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
    //Структуры
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    //Переменные.

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> index_;

    //Функции

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string text) const;

    Query ParseQuery(const std::string& text) const;

    static bool IsValidWord(const std::string& word);

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename KeyMapper>
    std::vector<Document> FindAllDocuments(const Query& query, KeyMapper keymapper) const;
};

//======================= 

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words) {
    for (const std::string& word : stop_words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("wrong word in StringCollection"s);
        }
        if (!word.empty())
        {
            stop_words_.insert(word);
        }
    }
}

template <typename KeyMapper>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, KeyMapper keymapper) const {
    const Query query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, keymapper);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename KeyMapper>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, KeyMapper keymapper) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            if (keymapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}
