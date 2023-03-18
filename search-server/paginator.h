#pragma once
#include <vector>
#include <iterator>

#include "string_processing.h"

template <class Iterator>
struct IteratorRange {
public:
    explicit IteratorRange(Iterator range_begin, Iterator range_end)
        : begin_(range_begin), end_(range_end)
    {
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

private:
    Iterator begin_;
    Iterator end_;

};

template <class Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& docs) {
    for (Iterator it = docs.begin(); it < docs.end(); ++it)
    {
        os << *it;
    }
    return os;
}

template <typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {

        auto r_b = range_begin;

        for (size_t last_page = distance(range_begin, range_end) / page_size - 1; last_page > 0; --last_page) {
            auto temp_it = r_b + page_size;
            IteratorRange page(r_b, temp_it);
            pages_.push_back(page);
            r_b = temp_it;
        }
        IteratorRange page(r_b, range_end);
        pages_.push_back(page);
    }


    //template <class It> // Iterator
    auto begin() const {
        return pages_.begin();
    }

    //template <typename It>
    auto end() const {
        return pages_.end();
    }

    size_t size() {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}