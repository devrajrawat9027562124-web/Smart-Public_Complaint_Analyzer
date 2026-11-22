/
 * duplicate_detector.c
 * Duplicate complaint detection using Levenshtein distance and Jaccard similarity.
 * Space-optimized Levenshtein using only two rows.
 *
 * SMART DUPLICATE LOGIC (solves the "AC not working" vs "Fan not working" problem):
 *
 *   1. STOP-WORD FILTERING: Generic words ("not", "working", "problem", "please",
 *      "issue", "area", "colony") are excluded from Jaccard similarity so only
 *      content-bearing words contribute. Without this, "AC not working" and
 *      "Fan not working" share "not" and "working" giving false high similarity.
 *
 *   2. TITLE SUBJECT MATCHING: The KEY subject nouns in the title ("AC" vs "Fan",
 *      "pipe" vs "wire") are compared separately. If the title subjects don't
 *      overlap, a penalty (0.5x) is applied — this catches that the complaints
 *      are fundamentally about DIFFERENT things even if phrased similarly.
 *
 *   3. USER-AWARE THRESHOLDING:
 *      - Same user re-submitting  → threshold = 0.75 (likely a true re-submit)
 *      - Different user, same loc → threshold = 0.82 (could be same issue)
 *      - Different user, diff loc → threshold = 0.88 (very unlikely duplicate)
 *
 *   EXAMPLE:
 *     User A @ Clock Tower: "AC is not working in my office"
 *     User B @ Rajpur Road: "Fan is not working in my house"
 *
 *     OLD system: Jaccard("not","working","office") vs ("not","working","house")
 *                 → high overlap → FALSE DUPLICATE flagged ✗
 *
 *     NEW system: Stop words "not","working" filtered out.
 *                 Content words: {"office"} vs {"fan","house"}
 *                 Jaccard = 0/3 = 0.0
 *                 Title subjects: "AC" vs "Fan" → 0.0 overlap → penalty 0.5x
 *                 Different user + different location → threshold = 0.88
 *                 Final score ≈ 0.15 << 0.88 → NOT DUPLICATE ✓
 */

#include "../../include/engine/duplicate_detector.h"
#include "../../include/engine/nlp_analyzer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ═══════════════════════════════════════════════════════════════════
 *  STOP WORDS — too generic to be meaningful for similarity.
 *  These words appear in almost every complaint and inflate
 *  similarity scores without indicating actual duplicate content.
 * ═══════════════════════════════════════════════════════════════════ */
static const char* STOP_WORDS[] = {
    /* English grammar words */
    "the", "is", "are", "was", "were", "be", "been", "being",
    "have", "has", "had", "do", "does", "did", "will", "would",
    "shall", "should", "may", "might", "must", "can", "could",
    "a", "an", "and", "but", "or", "nor", "for", "yet", "so",
    "in", "on", "at", "to", "from", "by", "with", "about",
    "of", "up", "out", "off", "over", "under", "into", "through",
    "this", "that", "these", "those", "it", "its",
    "not", "no", "very", "much", "too", "also", "just", "only",
    "my", "our", "your", "his", "her", "their", "we", "they",
    "me", "him", "them", "who", "which", "what", "how",
    "if", "when", "where", "while", "as", "than", "then",
    "here", "there", "all", "any", "each", "every", "some",
    "such", "own", "same", "other", "more", "most", "many",
    /* Common complaint filler words (appear in almost every complaint) */
    "please", "kindly", "request", "complaint", "problem", "issue",
    "facing", "since", "need", "needed", "help", "urgent",
    "immediately", "sir", "madam", "dear", "regarding", "respected",
    "working", "work", "properly", "still", "getting",
    "area", "near", "side", "colony", "lane", "sector",
    "come", "coming", "going", "make", "take", "give", "put",
    "day", "days", "week", "weeks", "month", "months", "time",
    "one", "two", "three", "four", "five", "many", "several",
    /* Two-letter words with no subject meaning */
    "am", "pm", "he", "us", "ok", "go", "hi", "oh", "ie", "eg", "etc",
    NULL
};

/* Check if a word is a stop word — O(n) scan, but the list is small */
static int is_stop_word(const char* word) {
    for (int i = 0; STOP_WORDS[i] != NULL; i++) {
        if (strcmp(word, STOP_WORDS[i]) == 0) return 1;
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 *  LEVENSHTEIN DISTANCE (space-optimized with 2-row DP)
 * ═══════════════════════════════════════════════════════════════════ */
int levenshtein_distance(const char* s1, const char* s2) {
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    /* Use only two rows for O(n) space instead of O(m×n) */
    int* prev_row = (int*)malloc((len2 + 1) * sizeof(int));
    int* curr_row = (int*)malloc((len2 + 1) * sizeof(int));

    if (!prev_row || !curr_row) {
        free(prev_row);
        free(curr_row);
        return -1;
    }

    for (int j = 0; j <= len2; j++) {
        prev_row[j] = j;
    }

    for (int i = 1; i <= len1; i++) {
        curr_row[0] = i;
        for (int j = 1; j <= len2; j++) {
            int cost = (tolower((unsigned char)s1[i - 1]) ==
                       tolower((unsigned char)s2[j - 1])) ? 0 : 1;

            int deletion     = prev_row[j] + 1;
            int insertion    = curr_row[j - 1] + 1;
            int substitution = prev_row[j - 1] + cost;

            int min_val = deletion;
            if (insertion < min_val)    min_val = insertion;
            if (substitution < min_val) min_val = substitution;

            curr_row[j] = min_val;
        }
        /* Swap rows */
        int* temp = prev_row;
        prev_row = curr_row;
        curr_row = temp;
    }

    int result = prev_row[len2];
    free(prev_row);
    free(curr_row);
    return result;
}

/* Normalized Levenshtein Similarity (0.0 to 1.0) */
double levenshtein_similarity(const char* s1, const char* s2) {
    int distance = levenshtein_distance(s1, s2);
    if (distance < 0) return 0.0;

    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);
    int max_len = (len1 > len2) ? len1 : len2;

    if (max_len == 0) return 1.0;
    return 1.0 - ((double)distance / max_len);
}

/* ═══════════════════════════════════════════════════════════════════
 *  CONTENT WORD SETS
 *  Content words = tokens of 2+ characters that are not stop words.
 *  Each word is kept once, so the measures below are true set measures.
 * ═══════════════════════════════════════════════════════════════════ */
static int content_word_set(const char* text, char out[][MAX_TOKEN_LEN], int max_out) {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int count = tokenize(text, tokens, MAX_TOKENS);
    int n = 0;

    for (int i = 0; i < count && n < max_out; i++) {
        /* Two-letter words count: subjects like "AC" or "TV" matter. */
        if (strlen(tokens[i]) < 2 || is_stop_word(tokens[i])) continue;
        int seen = 0;
        for (int j = 0; j < n; j++) {
            if (strcmp(out[j], tokens[i]) == 0) { seen = 1; break; }
        }
        if (!seen) strcpy(out[n++], tokens[i]);
    }
    return n;
}

/* |A ∩ B| / |A ∪ B| for two word sets */
static double set_jaccard(char a[][MAX_TOKEN_LEN], int na, char b[][MAX_TOKEN_LEN], int nb) {
    int inter = 0;
    for (int i = 0; i < na; i++) {
        for (int j = 0; j < nb; j++) {
            if (strcmp(a[i], b[j]) == 0) { inter++; break; }
        }
    }
    int uni = na + nb - inter;
    return (uni == 0) ? 0.0 : (double)inter / uni;
}

/* ═══════════════════════════════════════════════════════════════════
 *  JACCARD SIMILARITY (with stop-word filtering)
 *
 *  "AC not working" vs "Fan not working": "not" and "working" are stop
 *  words, so the content sets are {} and {fan} → Jaccard = 0.0.
 *
 *  Each distinct word counts once. (An earlier version counted repeated
 *  words separately, which could push the score above 1.0.)
 * ═══════════════════════════════════════════════════════════════════ */
double jaccard_similarity(const char* text1, const char* text2) {
    char set1[MAX_TOKENS][MAX_TOKEN_LEN];
    char set2[MAX_TOKENS][MAX_TOKEN_LEN];

    int n1 = content_word_set(text1, set1, MAX_TOKENS);
    int n2 = content_word_set(text2, set2, MAX_TOKENS);

    return set_jaccard(set1, n1, set2, n2);
}

/* ═══════════════════════════════════════════════════════════════════
 *  TITLE SUBJECT SIMILARITY
 *
 *  Compares ONLY the content-bearing (non-stop) words in titles.
 *    "AC not working"  → {"ac"}
 *    "Fan not working" → {"fan"}
 *    Overlap = 0  →  title_similarity = 0.0 (different subjects)
 *
 *    "Water pipe burst"     → {"water", "pipe", "burst"}
 *    "Water pipe leaking"   → {"water", "pipe", "leaking"}
 *    Overlap = 2 / 4 = 0.50  →  similar subjects
 * ═══════════════════════════════════════════════════════════════════ */
static double title_similarity(const char* title_a, const char* title_b) {
    char cw_a[MAX_TOKENS][MAX_TOKEN_LEN];
    char cw_b[MAX_TOKENS][MAX_TOKEN_LEN];

    int na = content_word_set(title_a, cw_a, MAX_TOKENS);
    int nb = content_word_set(title_b, cw_b, MAX_TOKENS);

    if (na == 0 && nb == 0) return 1.0;
    if (na == 0 || nb == 0) return 0.0;
    return set_jaccard(cw_a, na, cw_b, nb);
}

/* Title penalty: subjects that clearly differ cut the combined score */
static double title_penalty_for(double title_sim) {
    if (title_sim < 0.2) return 0.5;   /* subjects clearly differ  */
    if (title_sim < 0.4) return 0.7;   /* subjects partially differ */
    return 1.0;                        /* subjects likely the same  */
}

/* ═══════════════════════════════════════════════════════════════════
 *  COMBINED SIMILARITY SCORE (with title-mismatch penalty)
 *
 *  combined = (0.4·Jaccard + 0.3·Levenshtein + 0.2·category + 0.1·location)
 *             × title_penalty
 * ═══════════════════════════════════════════════════════════════════ */
double compute_similarity_ex(const Complaint* a, const Complaint* b,
                             SimilarityBreakdown* out) {
    char text_a[MAX_TITLE_LEN + MAX_DESC_LEN + 2];
    char text_b[MAX_TITLE_LEN + MAX_DESC_LEN + 2];

    snprintf(text_a, sizeof(text_a), "%s %s", a->title, a->description);
    snprintf(text_b, sizeof(text_b), "%s %s", b->title, b->description);

    SimilarityBreakdown s;
    s.jaccard       = jaccard_similarity(text_a, text_b);
    s.levenshtein   = levenshtein_similarity(text_a, text_b);
    s.title         = title_similarity(a->title, b->title);
    s.category      = (strcmp(a->category, b->category) == 0) ? 1.0 : 0.0;
    s.location      = (a->location_id == b->location_id) ? 1.0 : 0.0;
    s.title_penalty = title_penalty_for(s.title);

    s.combined = ((JACCARD_WEIGHT * s.jaccard)          /* 0.4 */
                + (LEVENSHTEIN_WEIGHT * s.levenshtein)  /* 0.3 */
                + (CATEGORY_WEIGHT * s.category)        /* 0.2 */
                + (LOCATION_WEIGHT * s.location))       /* 0.1 */
               * s.title_penalty;

    if (out) *out = s;
    return s.combined;
}

double compute_similarity(const Complaint* a, const Complaint* b) {
    return compute_similarity_ex(a, b, NULL);
}

/* ═══════════════════════════════════════════════════════════════════
 *  USER/LOCATION-AWARE THRESHOLD
 *
 *  ┌─────────────────────────────────┬───────────┬─────────────────┐
 *  │ Scenario                        │ Threshold │ Why             │
 *  ├─────────────────────────────────┼───────────┼─────────────────┤
 *  │ Same user re-submitting         │   0.75    │ Likely re-post  │
 *  │ Different user, SAME location   │   0.82    │ Maybe same issue│
 *  │ Different user, DIFF location   │   0.88    │ Unlikely dup    │
 *  └─────────────────────────────────┴───────────┴─────────────────┘
 * ═══════════════════════════════════════════════════════════════════ */
double duplicate_threshold(const Complaint* complaint, const Complaint* existing) {
    if (existing->user_id == complaint->user_id) return DUPLICATE_THRESHOLD;          /* 0.75 */
    if (existing->location_id == complaint->location_id) return DUPLICATE_THRESHOLD + 0.07; /* 0.82 */
    return DUPLICATE_THRESHOLD + 0.13;                                                /* 0.88 */
}

/* ═══════════════════════════════════════════════════════════════════
 *  CHECK FOR DUPLICATE
 *
 *  Every earlier, non-duplicate, relevant complaint is compared, and each
 *  comparison uses its own threshold (above). The match with the highest
 *  similarity among those that pass their threshold wins. (Previously only
 *  the single most similar complaint was tested, so a same-user re-post with
 *  a slightly lower score than an unrelated near-match could be missed.)
 *
 *  Speed-up: the Levenshtein term is at most 1, so if the score cannot reach
 *  the threshold even with Levenshtein = 1, the O(n·m) edit distance is
 *  skipped. This never changes the result.
 * ═══════════════════════════════════════════════════════════════════ */
int check_duplicate_ex(Complaint* complaint, Complaint* existing, int existing_count,
                       double* best_similarity, double* threshold_used) {
    char text_c[MAX_TITLE_LEN + MAX_DESC_LEN + 2];
    char set_c[MAX_TOKENS][MAX_TOKEN_LEN];
    char title_c[MAX_TOKENS][MAX_TOKEN_LEN];
    snprintf(text_c, sizeof(text_c), "%s %s", complaint->title, complaint->description);
    int n_set_c = content_word_set(text_c, set_c, MAX_TOKENS);
    int n_title_c = content_word_set(complaint->title, title_c, MAX_TOKENS);

    double best = -1.0, best_threshold = 0.0;
    int duplicate_id = -1;

    for (int i = 0; i < existing_count; i++) {
        const Complaint* e = &existing[i];
        if (e->id == complaint->id) continue;
        if (e->is_duplicate) continue;
        if (e->is_irrelevant) continue;
        /* A complaint already resolved before this one was made is not
         * "the same open issue": the problem has come back and needs new
         * work, so it must not be filed as a duplicate of a closed ticket.
         * (No submission date yet = a draft being written now.) Dates are
         * "YYYY-MM-DD HH:MM", so string order is time order. */
        if (strcmp(e->status, "COMPLETED") == 0 && strcmp(e->completed_date, "N/A") != 0 &&
            (complaint->submitted_date[0] == '\0' ||
             strcmp(e->completed_date, complaint->submitted_date) <= 0)) continue;

        double threshold = duplicate_threshold(complaint, e);

        /* Cheap components first */
        char text_e[MAX_TITLE_LEN + MAX_DESC_LEN + 2];
        char set_e[MAX_TOKENS][MAX_TOKEN_LEN];
        char title_e[MAX_TOKENS][MAX_TOKEN_LEN];
        snprintf(text_e, sizeof(text_e), "%s %s", e->title, e->description);
        int n_set_e = content_word_set(text_e, set_e, MAX_TOKENS);
        int n_title_e = content_word_set(e->title, title_e, MAX_TOKENS);

        double jac = set_jaccard(set_c, n_set_c, set_e, n_set_e);
        double tsim;
        if (n_title_c == 0 && n_title_e == 0) tsim = 1.0;
        else if (n_title_c == 0 || n_title_e == 0) tsim = 0.0;
        else tsim = set_jaccard(title_c, n_title_c, title_e, n_title_e);
        double cat = (strcmp(complaint->category, e->category) == 0) ? 1.0 : 0.0;
        double loc = (complaint->location_id == e->location_id) ? 1.0 : 0.0;
        double penalty = title_penalty_for(tsim);

        double upper = (JACCARD_WEIGHT * jac + LEVENSHTEIN_WEIGHT * 1.0 +
                        CATEGORY_WEIGHT * cat + LOCATION_WEIGHT * loc) * penalty;
        if (upper < threshold) continue;

        double lev = levenshtein_similarity(text_c, text_e);
        double similarity = (JACCARD_WEIGHT * jac + LEVENSHTEIN_WEIGHT * lev +
                             CATEGORY_WEIGHT * cat + LOCATION_WEIGHT * loc) * penalty;

        if (similarity >= threshold && similarity > best) {
            best = similarity;
            best_threshold = threshold;
            duplicate_id = e->id;
        }
    }

    if (best_similarity) *best_similarity = (duplicate_id >= 0) ? best : 0.0;
    if (threshold_used) *threshold_used = (duplicate_id >= 0) ? best_threshold : 0.0;

    if (duplicate_id >= 0) {
        complaint->is_duplicate = 1;
        complaint->duplicate_of = duplicate_id;
        return duplicate_id;
    }

    complaint->is_duplicate = 0;
    complaint->duplicate_of = -1;
    return -1;
}

int check_duplicate(Complaint* complaint, Complaint* existing, int existing_count) {
    return check_duplicate_ex(complaint, existing, existing_count, NULL, NULL);
}
