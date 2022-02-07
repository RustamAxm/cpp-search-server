#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <numeric>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
//Считывание из консоли
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
// Разделение на слова
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}
// Структура документа
struct Document {
    int id;
    double relevance;
    int rating;
};
// Возможные статусы документов
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
// Сам класс поисковика
class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }
    template <typename DocumentPred>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPred document_pred) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_pred);
        double epsilon = 1e-6;
        sort(matched_documents.begin(), matched_documents.end(),
             [epsilon](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < epsilon) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus new_status,  int rating) { return new_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const{
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
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
        return {matched_words, documents_.at(document_id).status};
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
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPred>
    vector<Document> FindAllDocuments(const Query& query, DocumentPred document_pred) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_pred(document_id, document_data.status, document_data.rating)) {
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
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating
                                        });
        }
        return matched_documents;
    }
};

// макросы

void AssertPrint(const bool& flag, const string& str, const string& func_name, const string& file_name, int line_number, const string& hint) {
    if(flag == false){
        cout << file_name << "("s << line_number << "): "s;
        cout << func_name << ": "s ;
        cout << "ASSERT("s << str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

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

template <typename Type>
void RunTestImpl(const Type& func, const string& str) {
    func();
    cerr << str << " OK" << endl;
}

// For cheking and printing containers

template<typename First, typename Second>
ostream &operator<<(ostream &out, const pair<First, Second> &p) {
    return out << p.first << ": "s << p.second;
}

template<typename Document>
void Print(ostream &out, const Document &container) {
    bool first = true;
    for (const auto &element: container) {
        if (first) {
            out << element;
            first = false;
        } else {
            out << ", "s << element;
        }
    }
}

template<typename Element>
ostream &operator<<(ostream &out, const vector<Element> &container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template<typename Element>
ostream &operator<<(ostream &out, const set<Element> &container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template<typename Key, typename Value>
ostream &operator<<(ostream &out, const map<Key, Value> &container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}


#define ASSERT(expr) AssertPrint((expr), #expr, __FUNCTION__, __FILE__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertPrint((expr), #expr, __FUNCTION__, __FILE__, __LINE__, (hint))

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(expr)  RunTestImpl((expr), #expr)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id ,  doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty()); // Тут так для того чтобы использоавть все варианты ASSERT
    }
}

//Тест. Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска
void MinusWordsExlude(){
    const int doc_id = 42;
    const string content = "cat in the -city"s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT(server.FindTopDocuments("city"s).empty());
}
//Тест. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса ???

// Тест. Сортировка найденных документов по релевантности.
void TestRelevantSorting(){
    SearchServer server;
    server.SetStopWords("и в на"s);

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    auto tmp = found_docs[0].relevance;
    bool flag = true;
    for (int i = 1; i < found_docs.size(); ++i){
        if (tmp >= found_docs[i].relevance) {
            flag = true;
            tmp = found_docs[i].relevance;
        }else {
            flag = false;
            break;
        }
    }
    ASSERT_HINT(flag, "Relevant sorting ERROR"s );

}

//Тест. Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestRating(){
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(20, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {-5, -12, -2, -1});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(21, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, 12, 2, 1});

    const auto found_docs =  server.FindTopDocuments("ухоженный"s);
    vector<int> rating_test;
    vector<int> rating =  {5, -1, -5};
    for (const auto & found_doc : found_docs){
        rating_test.push_back(found_doc.rating); // могу как то сразу достать вектор из структуры типа Document?
    }
    ASSERT_EQUAL(rating_test, rating);
}

// Тест. Фильтрация результатов поиска с использованием предиката, задаваемого пользователем
void TestPredicate(){
    SearchServer server;
    server.SetStopWords("и в на"s);

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {1, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {1, 1});

    //cout << "ACTUAL by default:"s << endl;
    for (const Document& document : server.FindTopDocuments("пушистый ухоженный кот"s)) {
        ASSERT_EQUAL_HINT(document.id, 0, "Predicate test ERROR for ACTUAL"s);
    }

    //cout << "BANNED:"s << endl;
    for (const Document& document : server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        ASSERT_EQUAL_HINT(document.id, 3, "Predicate test ERROR for BANNED"s);
    }
}

//Тест. Корректная реливантность
void TestCorrectRelevance(){
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    vector <double> relevance;
    vector <double> relevance_test = {0.866434, 0.173287, 0.173287};
    double epsilon = 1e-6;
    for (int i =0 ; i< found_docs.size(); ++i){
        ASSERT_HINT(abs(found_docs[i].relevance -  relevance_test[i]) < epsilon, "Relevance counting ERROR"s);
    }
}

// Тест для проверки статуса документа

void TestDocumentsStatus() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    const DocumentStatus status = DocumentStatus::ACTUAL;
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});

    const auto found_docs = search_server.FindTopDocuments("кот"s, status);
    ASSERT_EQUAL_HINT(found_docs.size(),  1, "Document status test ERROR"s);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(MinusWordsExlude);
    RUN_TEST(TestRelevantSorting);
    RUN_TEST(TestRating);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestCorrectRelevance);
    RUN_TEST(TestDocumentsStatus);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
