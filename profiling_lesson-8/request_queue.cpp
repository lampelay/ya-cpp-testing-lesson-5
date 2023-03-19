#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer &search_server)
    : search_server_(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query,
                                                   const DocumentStatus status)
{
    // напишите реализацию
    const std::vector<Document> documents = search_server_.FindTopDocuments(raw_query, status);
    AddQueryResult({documents.empty()});
    return documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query)
{
    // напишите реализацию
    const std::vector<Document> documents = search_server_.FindTopDocuments(raw_query);
    AddQueryResult({documents.empty()});
    return documents;
}

int RequestQueue::GetNoResultRequests() const
{
    return no_result_requests_;
}

const int MIN_IN_DAY = 1440;

void RequestQueue::AddQueryResult(const bool empty)
{
    // Добавляем результат в очередь
    ++current_time;
    requests_.push_back({empty, current_time});

    // Если результат пустой, увеличиваем счётчик пустых результатов
    if (empty)
    {
        ++no_result_requests_;
    }

    // Удаляем все результаты, время для которых превышает допустимое
    while (!requests_.empty() && (current_time - requests_.front().timestamp) >= MIN_IN_DAY)
    {
        // Если удаляемый результат пустой, уменьшаем счётчик пустых результатов
        if (requests_.front().is_empty && no_result_requests_ > 0)
        {
            --no_result_requests_;
        }
        requests_.pop_front();
    }
}