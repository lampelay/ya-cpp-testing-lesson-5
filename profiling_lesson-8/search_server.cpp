#include <numeric>
#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "search_server.h"

using namespace std::string_literals;

int ComputeAverageRating(const std::vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    const int sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return sum / static_cast<int>(ratings.size());
}

bool SearchServer::IsValidWord(const std::string &word)
{
    return !word.empty() && word.at(0) != '-' &&
           word.at(word.size() - 1) != '-' &&
           std::none_of(word.begin(), word.end(),
                        [](const char c)
                        { return '\0' <= c && c < ' '; });
}

// SearchServer

SearchServer::SearchServer(const std::string &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {}

SearchServer::SearchServer() : SearchServer(""s) {}

void SearchServer::AddDocument(int document_id, const std::string &document,
                               DocumentStatus status, const std::vector<int> &ratings)
{
    if (document_id < 0)
    {
        throw std::invalid_argument("Document ID is negative"s);
    }
    if (documents_.count(document_id))
    {
        throw std::invalid_argument(
            "Search Server already contains document with ID '"s +
            std::to_string(document_id) + "'"s);
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    for (const std::string &word : words)
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word '"s + word +
                                        "' in document is not valid"s);
        }
    }
    const double inv_word_count = 1.0 / words.size();
    for (const std::string &word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id,
                       DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(
    const std::string &raw_query,
    const DocumentStatus expected_status) const
{
    return FindTopDocuments(
        raw_query,
        [expected_status](
            const int id,
            const DocumentStatus status,
            const int rating)
        {
            return status == expected_status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const { return documents_.size(); }

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
    const std::string &raw_query, int document_id) const
{
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
        }
    }
    for (const std::string &word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.clear();
            break;
        }
    }
    return std::tuple{matched_words, documents_.at(document_id).status};
}

int SearchServer::GetDocumentId(int index) const { return document_ids_.at(index); }

bool SearchServer::IsStopWord(const std::string &word) const
{
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string &text) const
{
    std::vector<std::string> words;
    for (const std::string &word : SplitIntoWords(text))
    {
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
{
    bool is_minus = false;
    if (text.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }
    if (text[0] == '-')
    {
        is_minus = true;
        text = text.substr(1);
    }
    if (!IsValidWord(text))
    {
        throw std::invalid_argument("'"s + text + "' is not valid query word"s);
    }
    return QueryWord{text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string &text) const
{
    Query query;
    for (const std::string &word : SplitIntoWords(text))
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.insert(query_word.data);
            }
            else
            {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string &word) const
{
    return std::log(GetDocumentCount() * 1.0 /
                    word_to_document_freqs_.at(word).size());
}