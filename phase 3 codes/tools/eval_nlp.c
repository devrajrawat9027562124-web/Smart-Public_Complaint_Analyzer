/*
 * eval_nlp.c - helper for tools/eval_nlp.py (built by `mingw32-make eval`).
 * Reads lines "title<TAB>description" on stdin and prints, for each,
 * "severity<TAB>suggested category<TAB>spam flag" from the C engine's NLP.
 */
#include <stdio.h>
#include <string.h>
#include "../include/engine/nlp_analyzer.h"

int main(void) {
    static char line[4096];
    NLPAnalyzer nlp;
    nlp_init(&nlp);
    while (fgets(line, sizeof(line), stdin)) {
        char title[MAX_TITLE_LEN] = "", desc[MAX_DESC_LEN] = "", cat[MAX_CATEGORY_LEN];
        char* tab;
        line[strcspn(line, "\r\n")] = '\0';
        tab = strchr(line, '\t');
        if (!tab) continue;
        *tab = '\0';
        strncpy(title, line, sizeof(title) - 1);
        strncpy(desc, tab + 1, sizeof(desc) - 1);
        suggest_category(&nlp, title, desc, cat);
        printf("%d\t%s\t%d\n", analyze_severity(&nlp, title, desc), cat, detect_irrelevant(title, desc));
    }
    nlp_free(&nlp);
    return 0;
}
