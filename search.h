#ifndef SEARCH_H
#define SEARCH_H

#include "indexer.h"

typedef struct Hit {
    int doc_id;
    int score;
} Hit;

Hit *search_index(InvertedIndex *idx, const char *query, int *nhits);

#endif