//
// Created by rustam on 06.03.2022.
//

#include "document.h"

Document::Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
}

std::ostream& operator<<(std::ostream& out, const Document& doc) {
    out << "{ document_id = " << doc.id
        << ", relevance = " << doc.relevance
        << ", rating = " << doc.rating
        << " }";
    return out;
}

