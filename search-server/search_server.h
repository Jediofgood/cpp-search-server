#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <execution>
#include <list>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

const size_t CONURRENT_MAP_TORRENTS = 10;

using namespace std::literals;

using vector_of_matched = std::tuple<std::vector<std::string_view>, DocumentStatus>;

class SearchServer {
public:
    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words);

    explicit SearchServer(const std::string& text) :SearchServer(std::string_view(text))
    {}

    explicit SearchServer(const std::string_view text) :SearchServer(SplitIntoWords(text))
    {}

    auto begin() const {
        return index_.begin();
    }

    auto end() const {
        return index_.end();
    }

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status1 = DocumentStatus::ACTUAL) const;

    template <typename KeyMapper>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, KeyMapper keymapper) const;

    template <typename KeyMapper>
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy exec, const std::string_view raw_query, KeyMapper keymapper) const;

    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy exec, const std::string_view raw_query, DocumentStatus status1 = DocumentStatus::ACTUAL) const;

    template <typename KeyMapper>
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy exec, const std::string_view raw_query, KeyMapper keymapper) const;

    std::vector<Document> FindTopDocuments(std::execution::parallel_policy exec, const std::string_view raw_query, DocumentStatus status1 = DocumentStatus::ACTUAL) const;

    size_t GetDocumentCount() const;

    vector_of_matched MatchDocument(const std::string_view raw_query, int document_id) const;

    vector_of_matched MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const;

    vector_of_matched MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    template<class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);


private:
    //Структуры
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    //Переменные.

    std::set<std::string, std::less<>> stop_words_;

    //map<слово, map << документ id, freqs>> 
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;

    //
    std::map<int, std::map<std::string_view, double>> documents_word_freqs_;

    std::map<int, DocumentData> documents_;
    std::set<int> index_;

    std::set<std::string, std::less<>> string_set_; //Хранилище всех не-стоп слов.

    //Функции

    bool IsStopWord(const std::string_view word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text);

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;

    Query ParseQuery(const std::string_view text, bool is_for_par = false) const;

    static bool IsValidWord(const std::string_view word);

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename KeyMapper>
    std::vector<Document> FindAllDocuments(const Query& query, KeyMapper keymapper) const;

    template <typename KeyMapper>
    std::vector<Document> FindAllDocuments(std::execution::sequenced_policy exec, const Query& query, KeyMapper keymapper) const;

    template <typename KeyMapper>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy exec,  const Query& query, KeyMapper keymapper) const;
};

//======================= 

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words) {
    for (const std::string_view word : stop_words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("wrong word in StringCollection"s);
        }
        if (!word.empty())
        {
            stop_words_.insert(std::string{ word.begin(), word.end()});
        }
    }
}


template <typename KeyMapper>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, KeyMapper keymapper) const {
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
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy exec, const std::string_view raw_query, KeyMapper keymapper) const {
    return FindTopDocuments(raw_query, keymapper);
}

template <typename KeyMapper>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, KeyMapper keymapper) const {
    std::map<int, double> document_to_relevance;

    for (const std::string_view word : query.plus_words) {
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

    for (const std::string_view word : query.minus_words) {
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

template<class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    if (documents_word_freqs_.count(document_id)) {
        documents_.erase(document_id);
        index_.erase(std::find(policy, index_.begin(), index_.end(), document_id));

        std::vector<std::string*> word_remove(documents_word_freqs_[document_id].size());
        //word_remove.reserve(documents_word_freqs_[document_id].size());

        std::transform(policy,
            documents_word_freqs_[document_id].begin(), documents_word_freqs_[document_id].end(),
            word_remove.begin(),
            [](auto& word_freq) {
                return const_cast<std::string*>(&word_freq.first); });



        std::for_each(policy,
            word_remove.begin(), word_remove.end(),
            [&](std::string* word) {
                word_to_document_freqs_[*word].erase(document_id);
        //if (!word_to_document_freqs_[*word].size()) {
        //    word_to_document_freqs_.erase(*word);
        //}
            });

        /*
        for (const auto& [word, freq] : documents_word_freqs_[document_id]) {
            word_to_document_freqs_[word].erase(document_id);

            if (!word_to_document_freqs_[word].size()) {
                word_to_document_freqs_.erase(word);
            }
        }
        */
        documents_word_freqs_.erase(document_id);
    }
}

template <typename KeyMapper>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy exec, const std::string_view raw_query, KeyMapper keymapper) const {
    Query query = ParseQuery(raw_query, true);

    std::sort(std::execution::par, query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(std::unique(std::execution::par, query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());

    //std::sort(query.minus_words.begin(), query.minus_words.begin());
    //query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());

    auto matched_documents = FindAllDocuments(std::execution::par, query, keymapper);

    sort(std::execution::par,  matched_documents.begin(), matched_documents.end(),
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
std::vector<Document> SearchServer::FindAllDocuments(std::execution::sequenced_policy exec, const Query& query, KeyMapper keymapper) const {
    return FindAllDocuments(query, keymapper);
}

template <typename KeyMapper>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy exec, const Query& query, KeyMapper keymapper) const {
    //std::map<int, double> document_to_relevance;
    ConcurrentMap<int, double> document_to_relevance(CONURRENT_MAP_TORRENTS);

    std::for_each(std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        [this, &keymapper, &document_to_relevance](std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    if (keymapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        }
    );


    std::for_each(std::execution::par,
        query.minus_words.begin(), query.minus_words.end(),
        [this, &document_to_relevance](std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        }
    );

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}

