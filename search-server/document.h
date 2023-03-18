#pragma once

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

