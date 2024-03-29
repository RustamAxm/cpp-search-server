//
// Created by rustam on 06.03.2022.
//
#pragma once

#include <deque>
#include <vector>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        // определите, что должно быть в структуре
        uint64_t timestamp;
        int results;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    SearchServer& search_server_;
    uint64_t current_time_;
    int no_results_request_;

    void AddRequests(int top_doc_num);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    // напишите реализацию
    const auto top_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddRequests(top_documents.size());
    return top_documents;
}