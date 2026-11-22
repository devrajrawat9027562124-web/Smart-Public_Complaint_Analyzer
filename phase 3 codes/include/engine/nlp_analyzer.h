/*
 * nlp_analyzer.h
 * NLP-based severity analysis, category suggestion, and spam detection.
 * Uses hash tables for keyword lookups and tokenization for text processing.
 */

#ifndef NLP_ANALYZER_H
#define NLP_ANALYZER_H

#include "complaint_types.h"
#include "hash_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NLP Analyzer context — holds all keyword dictionaries */
typedef struct {
    HashTable keyword_dict;         /* severity keywords (word → weight) */
    HashTable negation_dict;        /* negation words */
    HashTable amplifier_dict;       /* amplifier words */
    HashTable category_water;       /* water supply keywords */
    HashTable category_electricity; /* electricity keywords */
    HashTable category_roads;       /* roads/transport keywords */
    HashTable category_sanitation;  /* sanitation keywords */
    HashTable category_safety;      /* public safety keywords */
    int initialized;
} NLPAnalyzer;

/* One matched severity keyword, for explaining a score */
#define KW_PLAIN      0
#define KW_AMPLIFIED  1
#define KW_NEGATED    2
typedef struct {
    char word[MAX_TOKEN_LEN];
    int weight;         /* dictionary weight */
    double applied;     /* weight after amplifier/negation */
    int modifier;       /* KW_PLAIN, or KW_AMPLIFIED | KW_NEGATED bits */
} KeywordHit;

#define NLP_NUM_CATEGORIES 5
extern const char* const NLP_CATEGORIES[NLP_NUM_CATEGORIES];

/* ─── Operations ─── */

/* Initialize the NLP analyzer (loads all keyword dictionaries) */
void nlp_init(NLPAnalyzer* analyzer);

/* Analyze the severity of a complaint (returns 1-10) */
int analyze_severity(NLPAnalyzer* analyzer, const char* title, const char* description);

/* Same as analyze_severity, and also reports each keyword hit and the
 * hit-count bonus. hits/hit_count/hit_bonus may be NULL. *hit_count must be
 * initialised by the caller (normally to 0). */
int analyze_severity_explain(NLPAnalyzer* analyzer, const char* title,
                             const char* description, KeywordHit* hits,
                             int max_hits, int* hit_count, double* hit_bonus);

/* Keyword counts per category, in NLP_CATEGORIES order */
void category_scores(NLPAnalyzer* analyzer, const char* title,
                     const char* description, int scores[NLP_NUM_CATEGORIES]);

/* Suggest a category based on complaint text */
void suggest_category(NLPAnalyzer* analyzer, const char* title,
                      const char* description, char* category_out);

/* Detect if complaint text is irrelevant/spam (returns 1 if irrelevant) */
int detect_irrelevant(const char* title, const char* description);

/* Tokenize text into lowercase words */
int tokenize(const char* text, char tokens[][MAX_TOKEN_LEN], int max_tokens);

/* Free all NLP analyzer resources */
void nlp_free(NLPAnalyzer* analyzer);

#ifdef __cplusplus
}
#endif

#endif /* NLP_ANALYZER_H */
