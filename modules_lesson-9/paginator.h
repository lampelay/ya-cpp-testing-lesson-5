#pragma once
#include <algorithm>
#include <vector>
#include <ostream>

// IteratorRange
// API

template <class It>
class IteratorRange
{
public:
    IteratorRange(It begin, It end)
        : begin_(begin), end_(end), size_(std::distance(begin, end)) {}

    It begin() const;
    It end() const;
    size_t size() const;

private:
    It begin_;
    It end_;
    size_t size_;
};

// IMPL

template <class It>
It IteratorRange<It>::begin() const
{
    return begin_;
}

template <typename It>
It IteratorRange<It>::end() const
{
    return end_;
}

template <typename It>
size_t IteratorRange<It>::size() const
{
    return size_;
}

template <class It>
std::ostream &operator<<(std::ostream &os, IteratorRange<It> range)
{
    for (It it = range.begin(), end = range.end(); it != end; ++it)
    {
        os << *it;
    }
    return os;
}

// ----------------------

// Paginator
// API

template <typename It>
class Paginator
{
public:
    Paginator(It begin, It end, size_t page_size);

    auto begin() const;

    auto end() const;

private:
    std::vector<IteratorRange<It>> pages_;
};

// IMPL

template <class It>
Paginator<It>::Paginator(It begin, It end, size_t page_size)
{
    while (begin != end)
    {
        It temp = begin;
        if (std::distance(begin, end) < page_size)
        {
            begin = end;
        }
        else
        {
            std::advance(begin, page_size);
        }
        pages_.push_back({temp, begin});
    }
}

template <class It>
auto Paginator<It>::begin() const
{
    return pages_.begin();
}

template <class It>
auto Paginator<It>::end() const
{
    return pages_.end();
}