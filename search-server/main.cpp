#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cassert>
#include <optional>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
public:
    Document(int _id = 0, double _relevance = 0.0, int _rating = 0)
        :id(_id), relevance(_relevance), rating(_rating)
    {
    }
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    explicit SearchServer(const string& text)
    {       
        if (!IsValidWord(text)) {
            throw invalid_argument("wrong symbol in string"s);
        }
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words) {
        for (const string& word : stop_words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("wrong word in vector"s);
            }
            if (!word.empty())
            {
                stop_words_.insert(word);
            }
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        //if (document_id < 0 or documents_.count(document_id)) {
            //throw invalid_argument("wrong id"s);
        //}
        if (document_id < 0) {
            throw invalid_argument("wrong id"s);
        }
        if (documents_.count(document_id)) {
            throw invalid_argument("already exist id"s);
        }
        if (!IsValidWord(document)) {
            throw invalid_argument("wrong document"s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status1 = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status1](int document_id, DocumentStatus status, int rating) { return status == status1;});
    }
    //optional<vector<Document>> FindTopDocuments(const string& raw_query) const {
    //    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    //}

    template <typename KeyMapper>
    //vector<Document> FindTopDocuments(const string& raw_query, KeyMapper keymapper) const {
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper keymapper) const {
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("Bad query"s);
        }
        const Query query = ParseQuery(raw_query);
        if (IsWrongMinus(query.minus_words)) {
            throw invalid_argument("Wrong minus"s);
        }
        auto matched_documents = FindAllDocuments(query, keymapper);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (0 <= index and index < GetDocumentCount()) {
            return index;
        }
        else {
            throw out_of_range("if is out ofrange");
        }
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("Bad query"s);
        }

        const Query query = ParseQuery(raw_query);

        if (IsWrongMinus(query.minus_words)) {
            throw invalid_argument("Wrong minus"s);
        }

        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status };
    }



private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
            //if (text.empty()) { throw invalid_argument("пустой минус"s); }  //Так наверное быстрее, но имеет ли смысл?
            //if (text[0]==0) { throw invalid_argument("лишни минус"s); }
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    static bool IsWrongMinus(const set<string>& MinusWorld) {
        for (const string& word : MinusWorld) {
            if (word.empty() or word[0] == '-') {  //
                return true;
            }
        }
        return false;
    }

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query, KeyMapper keymapper) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                //if (documents_.at(document_id).status == status) {
                if (keymapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};





//Модульные Тесты >> v4spr.
/*
template <typename T>
ostream& operator<<(ostream& os, const vector<T>& r) {
    os << "[";
    bool flag = true;
    for (const T& i : r) {
        if (flag) {
            os << i;
            flag = false;
        }
        else {
            os << ", " << i;
        }
    }
    os << "]";
    return os;
}

template <typename T>
ostream& operator<<(ostream& os, const set<T>& r) {
    os << "{";
    bool flag = true;
    for (const T& i : r) {
        if (flag) {
            os << i;
            flag = false;
        }
        else {
            os << ", " << i;
        }
    }
    os << "}";
    return os;
}

template <typename TheKey, typename TheValue>
ostream& operator<<(ostream& os, const map<TheKey, TheValue>& r) {
    os << "{";
    bool flag = true;
    for (const auto& [key, value] : r) {
        if (flag) {
            os << key << ": " << value;
            flag = false;
        }
        else {
            os << ", " << key << ": " << value;
        }
    }
    os << "}";
    return os;
}


void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl((!!expr), #expr, __FUNCTION__, __FILE__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl((!!expr), #expr, __FUNCTION__, __FILE__, __LINE__, hint)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }

    {
        SearchServer server ("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestExcludeMinusWordsFromSearch() {

    SearchServer server("и"s);
    server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "белый пёс выразительные глаза", DocumentStatus::ACTUAL, { 1, 2, 3 });
    {
        const auto found_docs = server.FindTopDocuments("белый -пёс"s);
        ASSERT_EQUAL(found_docs[0].id, 2);
    }
    {
        const auto found_docs = server.FindTopDocuments("белый -кот"s);
        ASSERT_EQUAL(found_docs[0].id, 3);
    }
}


//Матчинг
void TestMatchedDocuments() {
    SearchServer server("и в на"s);
    server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    {
        const auto [matched_words, status] = server.MatchDocument("кот"s, 2);
        const vector<string> expected_result = { "кот"s };
        ASSERT_EQUAL(expected_result, matched_words);
    }

    {
        const auto [matched_words, status] = server.MatchDocument("ухоженный -пёс"s, 3);
        const vector<string> expected_result = {};
        ASSERT_EQUAL(expected_result, matched_words);
        ASSERT(matched_words.empty());
    }
}

//Проверка правильности поиска.
void TestAddedDocumentsByRequestWord() {
    {
        SearchServer server("и в на"s);
        ASSERT_HINT(server.GetDocumentCount() == 0, "Должен быть пуст");
        server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "Документ должен быть добавлен или добавлен только один.");

        server.AddDocument(3, "ухоженный пёс выразительный ошейник", DocumentStatus::ACTUAL, { 3, 2 ,1 });
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 2, "Документ должен быть добавлен или добавлен только один.");
        {
            const auto found_docs = server.FindTopDocuments("кот"s);
            ASSERT_EQUAL(found_docs.size(), 1);

            const Document& doc_to_compare = found_docs[0];
            ASSERT_EQUAL(doc_to_compare.id, 2);
        }
        {
            const auto found_docs = server.FindTopDocuments("ошейник -кот"s);
            ASSERT_EQUAL(found_docs.size(), 1);

            const Document& doc_to_compare = found_docs[0];
            ASSERT_EQUAL(doc_to_compare.id, 3);
        }
    }
}

//Сортировка по релевантности 
void TestSortByRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "ухоженный пёс белый"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto found_docs = server.FindTopDocuments("in the city"s);
        for (int i = 1; i < static_cast<int>(found_docs.size()); i++) {
            ASSERT(found_docs[i - 1].relevance >= found_docs[i].relevance);
        }
    }
}

//Проверка рейтинга
void TestCalculateAverageRating() {
    SearchServer server("и в на"s);
    const vector<int> ratings = { 1, 2, 3 };
    const int average = (1 + 2 + 3) / 3;
    server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, ratings);

    {
        const auto found_docs = server.FindTopDocuments("белый кот"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].rating, average);
    }
}

//Фильтрация предикатом и по статусом
void SortByPredicate() {
    SearchServer server("и в на c"s);
    server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "ухоженный пёс белый"s, DocumentStatus::ACTUAL, { -1, -2, -3 });
    server.AddDocument(4, "большой пёс с ошейником"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    {
        const auto found_docs = server.FindTopDocuments("белый кот"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
        ASSERT_EQUAL(found_docs[0].id, 2);
    }
    {
        const auto found_docs = server.FindTopDocuments("белый кот"s, [](int document_id, DocumentStatus status, int rating) { return rating < 0; });
        ASSERT_EQUAL(found_docs[0].id, 3);
    }
}


//Поиск по статусу
void TestByStatus() {
    SearchServer server("и в на"s);
    server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "ухоженный пёс белый"s, DocumentStatus::IRRELEVANT, { -1, -2, -3 });
    server.AddDocument(4, "пёс"s, DocumentStatus::BANNED, { 1, -2, 3 });
    server.AddDocument(5, "кот"s, DocumentStatus::REMOVED, { 1, 2, 3 });
    {
        const auto found_docs = server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs[0].id, 2);
    }
    {
        const auto found_docs = server.FindTopDocuments("пёс"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs[0].id, 3);
    }
    {
        const auto found_docs = server.FindTopDocuments("пёс"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs[0].id, 4);
    }
    {
        const auto found_docs = server.FindTopDocuments("кот"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs[0].id, 5);
    }

}

template <typename F>
void RunTestImpl(F func, const string& func_str) {
    func();
    cerr << func_str << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    //TestExcludeStopWordsFromAddedDocumentContent();
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);

    //TestExcludeMinusWordsFromSearch();
    RUN_TEST(TestExcludeMinusWordsFromSearch);

    //TestAddedDocumentsByRequestWord();
    RUN_TEST(TestAddedDocumentsByRequestWord);

    //TestMatchedDocuments();
    RUN_TEST(TestMatchedDocuments);

    //SortByRelevance();
    RUN_TEST(TestSortByRelevance);

    // CalculateAverageRating();
    RUN_TEST(TestCalculateAverageRating);

    //SortByPredicate();
    RUN_TEST(SortByPredicate);

    //TestByStatus();
    RUN_TEST(TestByStatus);
}


//Конец модульных тестов
*/





void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    //{
    //    TestSearchServer();
    //}
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    for (const Document& document : search_server.FindTopDocuments("ухоженный кот"s)) {
        PrintDocument(document);
    }
}