#pragma once
#include<iostream>

struct Document {
    int id;
    double relevance;
    int rating;
public:
    Document(int _id = 0, double _relevance = 0.0, int _rating = 0);
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& os, const Document& doc);

void PrintDocument(const Document& document);

