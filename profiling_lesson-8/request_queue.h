#pragma once

#include <vector>
#include <deque>
#include <string>

#include "search_server.h"

class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer &search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query,
                                         const DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string &raw_query,
                                         const DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string &raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        bool is_empty;
        int timestamp;
    };
    
    std::deque<QueryResult> requests_;

    const SearchServer &search_server_;

    int no_result_requests_ = 0;
    int current_time = 0;

    /**
     * Общий метод для добавления результата поиска в очередь
     */
    void AddQueryResult(const bool empty);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(
    const std::string &raw_query, DocumentPredicate document_predicate)
{
    const std::vector<Document> documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddQueryResult({documents.empty()});
    return documents;
}