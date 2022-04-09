//
// Created by rustam on 06.03.2022.
//
#pragma once

#include "string_processing.h"
#include "document.h"

#include <map>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <execution>


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;


class SearchServer {
public:
    // You can refer to this constant as SearchServer::INVALID_DOCUMENT_ID
    //inline static constexpr int INVALID_DOCUMENT_ID = -1
    //Конструктор класса
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy,
                                                                       const std::string& raw_query, int document_id) const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy,
                                                                       const std::string& raw_query, int document_id) const;

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::parallel_policy& policy,int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string, double>> document_id_to_word_freqs_;

    static bool IsValidWord(const std::string& word);

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string& text) const;
    QueryWord ParseQueryWord2(const std::string& text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;
    Query ParseQuery(const std::execution::parallel_policy& policy, const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeStringContainerNotEmpty(stop_words)){
    for(const std::string& word : stop_words_){
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Invalid Stop word: " + word);
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
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
        matched_documents.push_back({
                                            document_id,
                                            relevance,
                                            documents_.at(document_id).rating
                                    });
    }
    return matched_documents;
}