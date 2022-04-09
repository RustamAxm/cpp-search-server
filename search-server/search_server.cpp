//
// Created by rustam on 06.03.2022.
//

#include "search_server.h"
#include "log_duration.h"


SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0 || documents_.count(document_id)){
        throw std::invalid_argument("id is less zero or id is present, ID: " + std::to_string(document_id));
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    const int computed_rating = ComputeAverageRating(ratings);

    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_id_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id,
                       DocumentData{computed_rating,
                                    status
                       });

    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query,
                                          [status](int document_id, DocumentStatus new_status,  int rating)
                                          { return new_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const{
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const{
    return document_ids_.end();
}

std::tuple<std::vector<std::string>, DocumentStatus>
SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id ");
    }

    const Query query = ParseQuery( raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
//   return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::parallel_policy& policy,
                            const std::string& raw_query, int document_id) const{
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id ");
    }
//
//    const auto& words_freq = document_id_to_word_freqs_.at(document_id);
//    if (words_freq.empty()) {
//        return {{}, documents_.at(document_id).status};
//    }
//
//    const auto query = ParseQuery(raw_query);
//
//    auto it_found = std::find_if(policy,
//                                 query.minus_words.begin(),
//                                 query.minus_words.end(),
//                                 [&words_freq](const std::string& word){
//                                    return words_freq.count(word);
//                                    });
//
//    if (it_found != query.minus_words.end()) {
//        return {{}, documents_.at(document_id).status};
//    }

//    for (const std::string& word : query.plus_words) {
//        if (word_to_document_freqs_.count(word) == 0) {
//            continue;
//        }
//        if (word_to_document_freqs_.at(word).count(document_id)) {
//            matched_words.push_back(word);
//        }
//    }

//    std::for_each(
//                  query.plus_words.begin(), query.plus_words.end(),
//                  [this,&document_id,  &matched_words](auto &word){
//                    if(word_to_document_freqs_.at(word).count(document_id)){
//                        matched_words.push_back(word);
//                    }
//                    });
//    return {matched_words, documents_.at(document_id).status};

    const Query& query = ParseQuery(policy, raw_query);

    const auto& words_freqs = document_id_to_word_freqs_.at(document_id);
    if (words_freqs.empty()) {
        return {{}, documents_.at(document_id).status};
    }
    bool is_minus = std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [&](const auto &word) {
        return words_freqs.count(word) > 0;
    });
    if (is_minus) {
        return {{}, documents_.at(document_id).status};
    }

    std::vector<std::string> matched_words(query.plus_words.size());
    std::atomic_uint index = 0u;

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](const auto &word) {
        if (words_freqs.find(word) != std::end(words_freqs)) {
            matched_words[index++] = word;
        }
    });
    matched_words.resize(index);

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::sequenced_policy& policy,
                            const std::string& raw_query, int document_id) const{

//    if (documents_.count(document_id) == 0) {
//        throw std::out_of_range("No document with id ");
//    }
//
//    const auto& words_freq = document_id_to_word_freqs_.at(document_id);
//    if (words_freq.empty()) {
//        return {{}, documents_.at(document_id).status};
//    }
//
//    const auto query = ParseQuery(raw_query);
//    std::vector<std::string> matched_words;
//    auto it_found = std::find_if(policy,
//                                 query.minus_words.begin(),
//                                 query.minus_words.end(),
//                                 [&words_freq](const std::string& word){
//                                     return words_freq.count(word);
//                                 });
//
//    if (it_found != query.minus_words.end()) {
//        return {{}, documents_.at(document_id).status};
//    }
//
//    for (const std::string& word : query.plus_words) {
//        if (word_to_document_freqs_.count(word) == 0) {
//            continue;
//        }
//        if (word_to_document_freqs_.at(word).count(document_id)) {
//            matched_words.push_back(word);
//        }
//    }
//
//    return {matched_words, documents_.at(document_id).status};
    return MatchDocument(raw_query, document_id);
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const{
    static std::map<std::string, double> words_frequencies;
    if (!words_frequencies.empty()){
        words_frequencies.clear();
    }
    if (document_id < 0 || !documents_.count(document_id)){
        return words_frequencies;
    }
    words_frequencies = document_id_to_word_freqs_.at(document_id);
    return words_frequencies;
}

void SearchServer::RemoveDocument(int document_id){
    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id){
    const std::map<std::string, double>& words_freqs(document_id_to_word_freqs_.at(document_id));
    if(!words_freqs.empty()){
        std::vector<const std::string*> words{words_freqs.size(),nullptr};

        transform(policy,
                  words_freqs.begin(), words_freqs.end(),
                  words.begin(),
                  [](const auto& wf){
                      return &wf.first;
                  });

        std::for_each(policy,
                      words.begin(), words.end(),
                      [this, document_id](const std::string* item){
                        word_to_document_freqs_[*item].erase(document_id);
                        });

        documents_.erase(document_id);
        document_ids_.erase(document_id);
        document_id_to_word_freqs_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id){
    const std::map<std::string, double>& words_freqs(document_id_to_word_freqs_.at(document_id));
    if(!words_freqs.empty()){
        std::vector<const std::string*> words{words_freqs.size(),nullptr};

        transform(policy,
                  words_freqs.begin(), words_freqs.end(),
                  words.begin(),
                  [](const auto& wf){
                      return &wf.first;
                  });

        std::for_each(policy,
                      words.begin(), words.end(),
                      [this, document_id](const std::string* item){
                          word_to_document_freqs_[*item].erase(document_id);
                      });

        documents_.erase(document_id);
        document_ids_.erase(document_id);
        document_id_to_word_freqs_.erase(document_id);
    }
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if(!IsValidWord(word)){
            throw std::invalid_argument("Word is invalid: " + word );
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
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

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word " + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::QueryWord SearchServer::ParseQueryWord2(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }

    if (word.empty() || word[0] == '-' || !IsValidWord(std::string (word))) {
        throw std::invalid_argument("Query word " + text + " is invalid");
    }

    return {std::string (word), is_minus, IsStopWord(std::string (word))};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy& policy, const std::string& text) const {
    Query result;
    for (const std::string& word : SplitIntoWords2(text)) {
        const auto query_word = ParseQueryWord2(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(SearchServer::GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}



