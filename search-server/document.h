//
// Created by rustam on 06.03.2022.
//
#pragma once

// Структура документа
struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id)
            , relevance(relevance)
            , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};
// Возможные статусы документов
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

