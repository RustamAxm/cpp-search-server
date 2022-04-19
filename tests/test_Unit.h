//
// Created by rustam on 10.04.2022.
//
// макросы
#pragma once

#include "search_server.h"


using namespace std;

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
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id ,  doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty()); // Тут так для того чтобы использоавть все варианты ASSERT
    }
}

//Тест. Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска
void MinusWordsExlude(){
    const int doc_id = 42;
    const string content = "cat in the -city"s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT(server.FindTopDocuments("city"s).empty());
}
//Тест. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса ???

// Тест. Сортировка найденных документов по релевантности.
void TestRelevantSorting(){
    SearchServer server("и в на"s);

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
    SearchServer server("и в на"s);

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
    SearchServer server("и в на"s);

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
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    vector <double> relevance;
    vector <double> relevance_test = {0.866434, 0.173287, 0.173287};
    for (int i =0 ; i< found_docs.size(); ++i){
        ASSERT_HINT(abs(found_docs[i].relevance -  relevance_test[i]) < EPSILON, "Relevance counting ERROR"s);
    }
}

// Тест для проверки статуса документа

void TestDocumentsStatus() {
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::IRRELEVANT, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::REMOVED, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    ASSERT_EQUAL_HINT(search_server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL).size(),  1, "Document status test ERROR for ACTUAL"s);
    ASSERT_EQUAL_HINT(search_server.FindTopDocuments("хвост"s, DocumentStatus::IRRELEVANT).size(),  1, "Document status test ERROR for IRRELEVANT"s);
    ASSERT_EQUAL_HINT(search_server.FindTopDocuments("пес"s, DocumentStatus::REMOVED).size(),  0, "Document status test ERROR for REMOVED"s);
    ASSERT_EQUAL_HINT(search_server.FindTopDocuments("скворец"s, DocumentStatus::BANNED).size(),  1, "Document status test ERROR for BANNED"s);
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

