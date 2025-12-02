#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "indexer.h"
#include "search.h"

static void index_dir(InvertedIndex *idx, const char *path) {
    DIR *d = opendir(path);
    if (!d) { perror("opendir"); return; }
    struct dirent *ent;
    char full[1024];
    while ((ent = readdir(d)) != NULL) {
        // REMOVED "ent->d_type == DT_REG" check for Windows compatibility
        const char *name = ent->d_name;
        
        // Skip . and ..
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        size_t L = strlen(name);
        if (L > 4 && strcmp(name + L - 4, ".txt") == 0) {
            snprintf(full, sizeof(full), "%s/%s", path, name);
            int id = index_add_document(idx, full);
            if (id >= 0) printf("Indexed [%d] %s\n", id, full);
        }
    }
    closedir(d);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <docs_dir>\n", argv[0]);
        return 1;
    }
    const char *docs_dir = argv[1];
    InvertedIndex *idx = index_create(10007);
    index_dir(idx, docs_dir);

    char line[512];
    while (1) {
        printf("\nsearch> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        
        char *p = line; while (*p && *p!='\n' && *p!='\r') p++; *p='\0';
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
        int nh;
        Hit *hits = search_index(idx, line, &nh);
        if (nh == 0) { printf("No results.\n"); free(hits); continue; }
        printf("Results (%d):\n", nh);
        for (int i = 0; i < nh; ++i) {
            int id = hits[i].doc_id;
            printf("[%d] %s (score=%d)\n", id, idx->doc_names[id], hits[i].score);
        }
        free(hits);
    }

    index_destroy(idx);
    return 0;
}