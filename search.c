#define _POSIX_C_SOURCE 200809L
#include "indexer.h"
#include "search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *strdup_lower(const char *s) {
    int n = strlen(s);
    char *d = malloc(n+1);
    for (int i = 0; i < n; ++i) d[i] = tolower((unsigned char)s[i]);
    d[n] = '\0';
    return d;
}

char **tokenize_query(const char *q, int *count) {
    char *tmp = strdup(q);
    int cap = 8; int n = 0;
    char **arr = malloc(cap * sizeof(char*));
    char *p = tmp;
    char token[256]; int tpos = 0;
    while (*p) {
        if (isalnum((unsigned char)*p)) token[tpos++] = tolower((unsigned char)*p);
        else {
            if (tpos) { token[tpos] = '\0'; if (n>=cap){cap*=2;arr=realloc(arr,cap*sizeof(char*));} arr[n++] = strdup(token); tpos = 0; }
        }
        p++;
    }
    if (tpos) { token[tpos]='\0'; if (n>=cap){cap*=2;arr=realloc(arr,cap*sizeof(char*));} arr[n++] = strdup(token); }
    free(tmp);
    *count = n;
    return arr;
}

static void free_tokens(char **toks, int n) {
    for (int i = 0; i < n; ++i) free(toks[i]);
    free(toks);
}

Hit *search_index(InvertedIndex *idx, const char *query, int *nhits) {
    int qn; char **toks = tokenize_query(query, &qn);
    if (qn == 0) { *nhits = 0; free_tokens(toks, qn); return NULL; }

    int nd = idx->ndocs;
    int *scores = calloc(nd, sizeof(int));

    for (int i = 0; i < qn; ++i) {
        TermNode *tn = index_lookup(idx, toks[i]);
        if (!tn) continue;
        for (Posting *p = tn->postings; p; p = p->next) {
            if (p->doc_id >=0 && p->doc_id < nd) scores[p->doc_id] += p->freq;
        }
    }

    int hits_cap = 16, hits_n = 0;
    Hit *hits = malloc(hits_cap * sizeof(Hit));
    for (int d = 0; d < nd; ++d) {
        if (scores[d] > 0) {
            if (hits_n >= hits_cap) { hits_cap *= 2; hits = realloc(hits, hits_cap * sizeof(Hit)); }
            hits[hits_n].doc_id = d; hits[hits_n].score = scores[d]; hits_n++;
        }
    }

    for (int i = 0; i < hits_n; ++i) {
        for (int j = i+1; j < hits_n; ++j) {
            if (hits[j].score > hits[i].score) {
                Hit t = hits[i]; hits[i] = hits[j]; hits[j] = t;
            }
        }
    }

    free(scores);
    free_tokens(toks, qn);
    *nhits = hits_n;
    return hits;
}