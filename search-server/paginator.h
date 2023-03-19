#pragma once

#include <vector>
#include <iterator>
#include <stdexcept>

#include "document.h"

using namespace std::literals;

template <class Iterator>
struct IteratorRange {
public:
    explicit IteratorRange(Iterator range_begin, Iterator range_end);

    Iterator begin() const;

    Iterator end() const;

private:
    Iterator begin_;
    Iterator end_;

};

template <class Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& docs);

template <typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator range_begin, Iterator range_end, size_t page_size);

    auto begin() const;

    auto end() const;

    size_t size();

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

//================

template <class Iterator>
IteratorRange<Iterator>::IteratorRange(Iterator range_begin, Iterator range_end)
    : begin_(range_begin), end_(range_end)
{
}

template <class Iterator>
Iterator IteratorRange<Iterator>::begin() const {
    return begin_;
}

template <class Iterator>
Iterator IteratorRange<Iterator>::end() const {
    return end_;
}

template <class Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& docs) {
    for (Iterator it = docs.begin(); it < docs.end(); ++it)
    {
        os << *it;
    }
    return os;
}

template <typename Iterator>
Paginator<Iterator>::Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {

    if (page_size == 0) {
        throw std::invalid_argument("page_size can be zero"s);
    }

    auto r_b = range_begin;

    for (size_t num_of_pages = distance(range_begin, range_end); num_of_pages > 0;) {

        int current_page_size = std::min(num_of_pages, page_size);
        auto temp_it = r_b + current_page_size;
        IteratorRange page(r_b, temp_it);
        pages_.push_back(page);
        r_b = temp_it;
        num_of_pages -= current_page_size;
    }
}


template <typename Iterator>
auto Paginator<Iterator>::begin() const {
    return pages_.begin();
}

template <typename Iterator>
auto Paginator<Iterator>::end() const {
    return pages_.end();
}

template <typename Iterator>
size_t Paginator<Iterator>::size() {
    return pages_.size();
}
