#include "document.h"

Document::Document(int _id, double _relevance, int _rating)
    :id(_id), relevance(_relevance), rating(_rating)
{
}

using namespace std::string_literals;

std::ostream& operator<<(std::ostream& os, const Document& doc) {
    os << "{ document_id = "s << doc.id << ", relevance = "s << doc.relevance << ", rating = "s << doc.rating << " }"s;
    return os;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}