#pragma once

#include <set>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

#include "string_processing.h"
#include "document.h"

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer &strings)
{
    std::set<std::string> non_empty_strings;
    for (const std::string &str : strings)
    {
        if (!str.empty())
        {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

int ComputeAverageRating(const std::vector<int> &ratings);

class SearchServer
{
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(const std::string &stop_words_text);

    explicit SearchServer();

    void AddDocument(int document_id, const std::string &document,
                     DocumentStatus status, const std::vector<int> &ratings);

    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string &raw_query,
                                           const Predicate predicate) const;

    std::vector<Document> FindTopDocuments(const std::string &raw_query,
                                           const DocumentStatus expected_status) const;

    std::vector<Document> FindTopDocuments(const std::string &raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
        const std::string &raw_query, int document_id) const;

    int GetDocumentId(int index) const;

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    bool IsStopWord(const std::string &word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string &text) const;

    struct QueryWord
    {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    struct Query
    {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string &text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string &word) const
    {
        return log(GetDocumentCount() * 1.0 /
                   word_to_document_freqs_.at(word).size());
    }

    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query &query,
                                           const Predicate predicate) const
    {
        std::map<int, double> document_to_relevance;
        for (const std::string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] :
                 word_to_document_freqs_.at(word))
            {
                const auto &doc = documents_.at(document_id);
                if (predicate(document_id, doc.status, doc.rating))
                {
                    document_to_relevance[document_id] +=
                        term_freq * inverse_document_freq;
                }
            }
        }

        for (const std::string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

    static bool IsValidWord(const std::string &word)
    {
        return !word.empty() && word.at(0) != '-' &&
               word.at(word.size() - 1) != '-' &&
               std::none_of(word.begin(), word.end(),
                            [](const char c)
                            { return '\0' <= c && c < ' '; });
    }
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    for (const std::string &word : stop_words_)
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument(word + " is not valid stop-word"s);
        }
    }
}

template <typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query,
                                                     const Predicate predicate) const
{
    const Query query = ParseQuery(raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(query, predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs)
         {
             if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
             {
                 return lhs.rating > rhs.rating;
             }
             else
             {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}