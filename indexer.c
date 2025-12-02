#define _POSIX_C_SOURCE 200809L
#include "indexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_DOC_CAP 16

static unsigned long hash_str(const char *s) {
    unsigned long h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)(*s++);
    return h;
}

InvertedIndex *index_create(size_t nbuckets) {
    InvertedIndex *idx = calloc(1, sizeof(*idx));
    idx->nbuckets = nbuckets;
    idx->buckets = calloc(nbuckets, sizeof(TermNode*));
    idx->doc_names = calloc(INITIAL_DOC_CAP, sizeof(char*));
    idx->ndocs = 0;
    return idx;
}

static void ensure_doc_capacity(InvertedIndex *idx) {
    static int cap = INITIAL_DOC_CAP;
    if (idx->ndocs >= cap) {
        cap *= 2;
        idx->doc_names = realloc(idx->doc_names, cap * sizeof(char*));
    }
}

static void add_posting(TermNode *tn, int doc_id) {
    Posting *p = tn->postings;
    while (p) {
        if (p->doc_id == doc_id) { p->freq++; return; }
        p = p->next;
    }
    Posting *np = malloc(sizeof(*np));
    np->doc_id = doc_id; np->freq = 1; np->next = tn->postings; tn->postings = np;
}

TermNode *index_lookup(InvertedIndex *idx, const char *term) {
    unsigned long h = hash_str(term) % idx->nbuckets;
    TermNode *tn = idx->buckets[h];
    while (tn) {
        if (strcmp(tn->term, term) == 0) return tn;
        tn = tn->next;
    }
    return NULL;
}

static TermNode *insert_termnode(InvertedIndex *idx, const char *term) {
    unsigned long h = hash_str(term) % idx->nbuckets;
    TermNode *tn = malloc(sizeof(*tn));
    tn->term = strdup(term);
    tn->postings = NULL;
    tn->next = idx->buckets[h];
    idx->buckets[h] = tn;
    return tn;
}

static void free_postings(Posting *p) {
    while (p) {
        Posting *t = p->next; free(p); p = t;
    }
}

void index_destroy(InvertedIndex *idx) {
    if (!idx) return;
    for (size_t i = 0; i < idx->nbuckets; ++i) {
        TermNode *tn = idx->buckets[i];
        while (tn) {
            TermNode *t2 = tn->next;
            free(tn->term);
            free_postings(tn->postings);
            free(tn);
            tn = t2;
        }
    }
    for (int i = 0; i < idx->ndocs; ++i) free(idx->doc_names[i]);
    free(idx->doc_names);
    free(idx->buckets);
    free(idx);
}

static void index_file_contents(InvertedIndex *idx, const char *buf, int doc_id) {
    const char *p = buf;
    char token[256];
    int tpos = 0;
    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (isalnum(c)) {
            token[tpos++] = tolower(c);
            if (tpos >= (int)sizeof(token)-1) tpos = sizeof(token)-2;
        } else {
            if (tpos > 0) {
                token[tpos] = '\0';
                TermNode *tn = index_lookup(idx, token);
                if (!tn) tn = insert_termnode(idx, token);
                add_posting(tn, doc_id);
                tpos = 0;
            }
        }
        p++;
    }
    if (tpos > 0) {
        token[tpos] = '\0';
        TermNode *tn = index_lookup(idx, token);
        if (!tn) tn = insert_termnode(idx, token);
        add_posting(tn, doc_id);
    }
}

int index_add_document(InvertedIndex *idx, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);

    ensure_doc_capacity(idx);
    idx->doc_names[idx->ndocs] = strdup(path);
    int doc_id = idx->ndocs;
    idx->ndocs++;

    index_file_contents(idx, buf, doc_id);
    free(buf);
    return doc_id;
}