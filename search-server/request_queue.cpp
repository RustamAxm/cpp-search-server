//
// Created by rustam on 06.03.2022.
//

#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) :
        search_server_(const_cast<SearchServer &>(search_server)),
        current_time_(0),
        no_results_request_(0)
{
    // напишите реализацию
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    // напишите реализацию
    const auto top_documents = search_server_.FindTopDocuments(raw_query, status);
    AddRequests(top_documents.size());
    return top_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    // напишите реализацию
    const auto top_documents = search_server_.FindTopDocuments(raw_query);
    AddRequests(top_documents.size());
    return top_documents;
}

int RequestQueue::GetNoResultRequests() const {
    // напишите реализацию
    return no_results_request_;
}

void RequestQueue::AddRequests(int top_doc_num){
    ++current_time_;
    //удаление устаревших результаов
    for (;!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp;
          requests_.pop_front()){
        if(requests_.front().results == 0) --no_results_request_;
    }
    //раз есть теперь место можно запушить
    requests_.push_back({current_time_, top_doc_num});
    if(top_doc_num == 0) ++no_results_request_;
}