//
// Created by rustam on 06.04.2022.
//

#include "process_queries.h"


std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> documents_lists;
    documents_lists.reserve(queries.size());

    std::transform( std::execution::par,
                    queries.begin(), queries.end(),
                   documents_lists.begin(),
                   [&search_server](const std::string& query){
                    return search_server.FindTopDocuments(query); });
//    for (const std::string& query : queries) {
//        documents_lists.push_back(search_server.FindTopDocuments(query));
//    }
    return documents_lists;
}
