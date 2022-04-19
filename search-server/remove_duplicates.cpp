//
// Created by rustam on 18.03.2022.
//

#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::map<std::set<std::string_view>, int> word_to_document_freqs;
    std::set<int> documents_to_delete;
    for (auto document_id : search_server) {
        std::set<std::string_view> words;
        std::map<std::string_view , double> document_freq = search_server.GetWordFrequencies(document_id);

        std::transform(document_freq.begin(), document_freq.end(),
                       std::inserter(words, std::end(words)),
                       [](const auto& x) {return x.first;});

        if (word_to_document_freqs.find(words) == word_to_document_freqs.end()) {
            word_to_document_freqs[words] = document_id;
        } else if (document_id > word_to_document_freqs[words]) {
            word_to_document_freqs[words] = document_id;
            documents_to_delete.insert(document_id);
        }
    }

    for (auto document : documents_to_delete) {
        std::cout << "Found duplicate document id " << document << std::endl;
        search_server.RemoveDocument(document);
    }
}