//
// Created by rustam on 06.03.2022.
//

#include "search_server.h"


SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(std::string_view(stop_words_text)) {
}

SearchServer::SearchServer(std::string_view stop_words_text)
        : SearchServer(SplitIntoWordsStrView(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id,
                               std::string_view document,
                               DocumentStatus status,
                               const std::vector<int>& ratings) {
    if (document_id < 0 || documents_.count(document_id)) {
        throw std::invalid_argument("id is less zero or id is present, ID: " + std::to_string(document_id));
    }

    const int computed_rating = ComputeAverageRating(ratings);
    documents_.emplace(document_id,
                       DocumentData{computed_rating,
                                    status,
                                    std::string (document)});

    const std::vector<std::string_view> words = SplitIntoWordsNoStop(documents_[document_id].document);

    const double inv_word_count = 1.0 / static_cast<double>(words.size());

    for (std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_id_to_word_freqs_[document_id][word] += inv_word_count;
    }

    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
                                                     std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query,
                                          [status](int document_id, DocumentStatus new_status,  int rating)
                                          { return new_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
                                                     std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(std::execution::par, raw_query,
                                          [status](int document_id, DocumentStatus new_status,  int rating)
                                          { return new_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query,
                                          [status](int document_id, DocumentStatus new_status,  int rating)
                                          { return new_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
                                                     std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
                                                     std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {

    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::sequenced_policy& policy,
                            const std::string_view raw_query, int document_id) const {
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id ");
    }

    const auto query = ParseQuery(std::execution::seq, raw_query);

    const auto status = documents_.at(document_id).status;

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            std::vector<std::string_view> tmp = {};
            return { tmp, status };
        }
    }

    std::vector<std::string_view> matched_words;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            auto it = word_to_document_freqs_.find(word);
            if (it->first == word)
                matched_words.push_back(it->first);
        }
    }
    return { matched_words, status };

}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::parallel_policy& policy,
                            const std::string_view raw_query, int document_id) const {

    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id ");
    }

    const auto query = ParseQuery(std::execution::par, raw_query);

    const auto status = documents_.at(document_id).status;

    const auto word_checker =
            [this, document_id](std::string_view word) {
                const auto it = word_to_document_freqs_.find(word);
                return it != word_to_document_freqs_.end() && it->second.count(document_id);
            };

    if (std::any_of(std::execution::par,
                    query.minus_words.begin(),
                    query.minus_words.end(),
                    word_checker)) {
        std::vector<std::string_view> tmp = {};
        return { tmp, status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto words_end = std::copy_if(std::execution::par,
                                            query.plus_words.begin(), query.plus_words.end(),
                                            matched_words.begin(),
                                            word_checker
                                            );
    std::sort(std::execution::par, matched_words.begin(), words_end);
    words_end = std::unique(std::execution::par, matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return { matched_words, status };
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> words_frequencies;
    if (!words_frequencies.empty()){
        words_frequencies.clear();
    }
    if (document_id < 0 || !documents_.count(document_id)) {
        return words_frequencies;
    }
    words_frequencies = document_id_to_word_freqs_.at(document_id);
    return words_frequencies;
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    const std::map<std::string_view , double>& words_freqs(document_id_to_word_freqs_.at(document_id));
    if(!words_freqs.empty()) {
        std::vector<std::string_view> words{words_freqs.size()};

        transform(policy,
                  words_freqs.begin(), words_freqs.end(),
                  words.begin(),
                  [](const auto& wf){
                      return wf.first;
                  });

        std::for_each(policy,
                      words.begin(), words.end(),
                      [this, document_id](std::string_view item){
                          word_to_document_freqs_[item].erase(document_id);
                      });

        documents_.erase(document_id);
        document_ids_.erase(document_id);
        document_id_to_word_freqs_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    const std::map<std::string_view, double>& words_freqs(document_id_to_word_freqs_.at(document_id));
    if(!words_freqs.empty()) {
        std::vector<std::string_view> words{words_freqs.size()};

        transform(policy,
                  words_freqs.begin(), words_freqs.end(),
                  words.begin(),
                  [](const auto& wf){
                      return wf.first;
                  });

        std::for_each(policy,
                      words.begin(), words.end(),
                      [this, document_id](std::string_view item) {
                        word_to_document_freqs_[item].erase(document_id);
                        });

        documents_.erase(document_id);
        document_ids_.erase(document_id);
        document_id_to_word_freqs_.erase(document_id);
    }
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWordsStrView(text)) {
        if(!IsValidWord(word)){
            throw std::invalid_argument("Word is invalid: " + std::string(word));
        }
        if (!IsStopWord(word)) {
            words.emplace_back(word);
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word ) const {

    if (word.empty()) {
        throw std::invalid_argument("Query word is empty");
    }

    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }

    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

std::vector<std::string_view> SearchServer::SortUniq(const std::execution::sequenced_policy& policy,
                                                     std::vector<std::string_view>& container) const {

    std::sort(policy, container.begin(), container.end());
    auto words_end = std::unique(policy, container.begin(), container.end());
    container.erase(words_end, container.end());

    return container;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy& policy,
                                             std::string_view text) const {
    Query result;
    auto words = SplitIntoWordsStrView(text);

    for (const std::string_view word : words) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    result.minus_words = SortUniq(policy, result.minus_words);
    result.plus_words = SortUniq(policy, result.plus_words);

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy& policy,
                                             std::string_view text) const {

    Query result;
    auto words = SplitIntoWordsStrView(text);

    for (const std::string_view word : words) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(SearchServer::GetDocumentCount() * 1.0 / static_cast<double> (word_to_document_freqs_.at(word).size()));
}



