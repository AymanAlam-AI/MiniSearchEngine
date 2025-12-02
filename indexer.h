#ifndef INDEXER_H
#define INDEXER_H

#include <stddef.h>

// Simple posting: doc id and term frequency in that doc
typedef struct Posting {
    int doc_id;
    int freq;
    struct Posting *next;
} Posting;

// Hash node for a term
typedef struct TermNode {
    char *term;            // dynamically allocated
    Posting *postings;     // linked list of postings
    struct TermNode *next; // for bucket chaining
} TermNode;

// Index structure
typedef struct InvertedIndex {
    TermNode **buckets;
    size_t nbuckets;
    // document list
    char **doc_names; // array of filenames
    int ndocs;
} InvertedIndex;

// create/destroy
InvertedIndex *index_create(size_t nbuckets);
void index_destroy(InvertedIndex *idx);

// add document (path) and index its words; returns doc_id or -1 on error
int index_add_document(InvertedIndex *idx, const char *path);

// lookup postings for a term (returns pointer to TermNode or NULL)
TermNode *index_lookup(InvertedIndex *idx, const char *term);

#endif // INDEXER_H
