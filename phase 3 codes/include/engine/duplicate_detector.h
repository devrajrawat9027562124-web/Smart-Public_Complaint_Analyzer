/*
 * duplicate_detector.h
 * Duplicate complaint detection using Levenshtein distance and Jaccard similarity.
 * Weighted combination of text similarity, category match, and location match.
 */

#ifndef DUPLICATE_DETECTOR_H
#define DUPLICATE_DETECTOR_H

#include "complaint_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Every component of a similarity score (all 0.0 – 1.0) */
typedef struct {
    double jaccard;        /* content-word set overlap of title+description */
    double levenshtein;    /* normalized edit-distance similarity */
    double title;          /* content-word overlap of the titles only */
    double category;       /* 1 if same category */
    double location;       /* 1 if same location */
    double title_penalty;  /* 0.5, 0.7 or 1.0 depending on `title` */
    double combined;       /* weighted sum × title_penalty */
} SimilarityBreakdown;

/* ─── Similarity Functions ─── */

/* Compute Levenshtein edit distance between two strings */
int levenshtein_distance(const char* s1, const char* s2);

/* Compute normalized Levenshtein similarity (0.0 to 1.0) */
double levenshtein_similarity(const char* s1, const char* s2);

/* Compute Jaccard similarity between two texts (set-based word overlap) */
double jaccard_similarity(const char* text1, const char* text2);

/* ─── Duplicate Detection ─── */

/* Check if a complaint is a duplicate of any existing complaint.
 * Sets complaint->is_duplicate and complaint->duplicate_of if found.
 * Returns the ID of the duplicate original, or -1 if not a duplicate. */
int check_duplicate(Complaint* complaint, Complaint* existing,
                    int existing_count);

/* Compute combined similarity score (weighted) */
double compute_similarity(const Complaint* a, const Complaint* b);

/* Same, and fills every component into `out` (may be NULL) */
double compute_similarity_ex(const Complaint* a, const Complaint* b,
                             SimilarityBreakdown* out);

/* Threshold used when comparing `complaint` against `existing`
 * (0.75 same user, 0.82 same location, 0.88 otherwise) */
double duplicate_threshold(const Complaint* complaint, const Complaint* existing);

/* check_duplicate that also reports the winning similarity and threshold
 * (both 0.0 when no duplicate is found). Either pointer may be NULL. */
int check_duplicate_ex(Complaint* complaint, Complaint* existing, int existing_count,
                       double* best_similarity, double* threshold_used);

#ifdef __cplusplus
}
#endif

#endif /* DUPLICATE_DETECTOR_H */
