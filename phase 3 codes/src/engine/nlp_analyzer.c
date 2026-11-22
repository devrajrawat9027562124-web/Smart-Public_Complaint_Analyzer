/*
 * nlp_analyzer.c
 * NLP-based severity analysis with 120+ keywords, tokenization,
 * amplifier/negation handling, category suggestion, and spam detection.
 */

#include "../../include/engine/nlp_analyzer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* ─── Tokenizer ───
 * Splits on non-alphanumeric characters and lowercases each word.
 * An apostrophe inside a word is dropped, so "isn't" becomes "isnt" and
 * matches the negation dictionary instead of splitting into "isn" + "t".
 * Words of MAX_TOKEN_LEN characters or more are skipped. */
int tokenize(const char* text, char tokens[][MAX_TOKEN_LEN], int max_tokens) {
    int count = 0;
    int i = 0;
    int len = (int)strlen(text);

    while (i < len && count < max_tokens) {
        /* Skip non-alphanumeric characters */
        while (i < len && !isalnum((unsigned char)text[i])) i++;
        if (i >= len) break;

        /* Extract token */
        int tok_len = 0;
        int too_long = 0;
        while (i < len) {
            unsigned char ch = (unsigned char)text[i];
            if (isalnum(ch)) {
                if (tok_len < MAX_TOKEN_LEN - 1) {
                    tokens[count][tok_len++] = (char)tolower(ch);
                } else {
                    too_long = 1;
                }
                i++;
            } else if (ch == '\'' && i + 1 < len &&
                       isalnum((unsigned char)text[i + 1]) && tok_len > 0) {
                i++;  /* apostrophe inside a word: skip it */
            } else if (ch == 0xE2 && i + 3 < len && (unsigned char)text[i + 1] == 0x80 &&
                       (unsigned char)text[i + 2] == 0x99 &&
                       isalnum((unsigned char)text[i + 3]) && tok_len > 0) {
                i += 3;  /* typographic apostrophe (U+2019, phone keyboards): "don’t" = "dont" */
            } else {
                break;
            }
        }

        if (tok_len > 0 && !too_long) {
            tokens[count][tok_len] = '\0';
            count++;
        }
    }
    return count;
}

/* ─── Load keyword dictionaries ─── */
static void load_severity_keywords(HashTable* ht) {
    /* Critical (8-10) */
    ht_insert(ht, "fire", 10);
    ht_insert(ht, "explosion", 10);
    ht_insert(ht, "collapse", 10);
    ht_insert(ht, "collapsed", 10);
    ht_insert(ht, "collapsing", 10);
    ht_insert(ht, "electrocution", 10);
    ht_insert(ht, "electrocuted", 10);
    ht_insert(ht, "death", 10);
    ht_insert(ht, "dead", 10);
    ht_insert(ht, "fatal", 10);
    ht_insert(ht, "flood", 9);
    ht_insert(ht, "flooding", 9);
    ht_insert(ht, "earthquake", 9);
    ht_insert(ht, "landslide", 9);
    ht_insert(ht, "emergency", 9);
    ht_insert(ht, "hazardous", 9);
    ht_insert(ht, "toxic", 9);
    ht_insert(ht, "poisonous", 9);
    ht_insert(ht, "gas", 8);
    ht_insert(ht, "burning", 8);
    ht_insert(ht, "sparking", 8);
    ht_insert(ht, "electrified", 8);
    ht_insert(ht, "dangerous", 8);
    ht_insert(ht, "life", 8);
    ht_insert(ht, "threatening", 8);
    ht_insert(ht, "critical", 8);
    ht_insert(ht, "severe", 8);
    ht_insert(ht, "accident", 8);
    ht_insert(ht, "injured", 8);
    ht_insert(ht, "injury", 8);
    /* Added: common words for serious problems that matched nothing, so
     * e.g. "Child kidnapping attempt near school" scored 1/10 */
    ht_insert(ht, "murder", 10);
    ht_insert(ht, "drowning", 10);
    ht_insert(ht, "drowned", 10);
    ht_insert(ht, "kidnapping", 9);
    ht_insert(ht, "kidnap", 9);
    ht_insert(ht, "abduction", 9);
    ht_insert(ht, "trapped", 9);
    ht_insert(ht, "unconscious", 9);
    ht_insert(ht, "epidemic", 9);
    ht_insert(ht, "outbreak", 8);
    ht_insert(ht, "cholera", 8);
    ht_insert(ht, "danger", 8);
    ht_insert(ht, "hazard", 8);
    ht_insert(ht, "shock", 8);
    ht_insert(ht, "molestation", 8);

    /* Urgent (5-7) */
    ht_insert(ht, "broken", 6);
    ht_insert(ht, "burst", 7);
    ht_insert(ht, "sewage", 7);
    ht_insert(ht, "overflow", 7);
    ht_insert(ht, "overflowing", 7);
    ht_insert(ht, "blackout", 7);
    ht_insert(ht, "outage", 7);
    ht_insert(ht, "blocked", 6);
    ht_insert(ht, "clogged", 6);
    ht_insert(ht, "contaminated", 7);
    ht_insert(ht, "contamination", 7);
    ht_insert(ht, "exposed", 6);
    ht_insert(ht, "wires", 5);
    ht_insert(ht, "cable", 5);
    ht_insert(ht, "fallen", 6);
    ht_insert(ht, "sinkhole", 7);
    ht_insert(ht, "crack", 5);
    ht_insert(ht, "cracked", 5);
    ht_insert(ht, "unsafe", 6);
    ht_insert(ht, "unstable", 6);
    ht_insert(ht, "flooded", 7);
    ht_insert(ht, "submerged", 7);
    ht_insert(ht, "stagnant", 5);
    ht_insert(ht, "shortage", 6);
    ht_insert(ht, "disruption", 6);
    ht_insert(ht, "disrupted", 6);
    ht_insert(ht, "malfunction", 5);
    ht_insert(ht, "malfunctioning", 5);
    ht_insert(ht, "failure", 7);
    ht_insert(ht, "threat", 7);
    ht_insert(ht, "disease", 7);
    ht_insert(ht, "illness", 7);
    ht_insert(ht, "dengue", 7);
    ht_insert(ht, "malaria", 7);
    ht_insert(ht, "diarrhea", 7);
    ht_insert(ht, "stranded", 7);
    ht_insert(ht, "sick", 6);
    ht_insert(ht, "vomiting", 6);
    ht_insert(ht, "infection", 6);
    ht_insert(ht, "risk", 6);
    ht_insert(ht, "breakdown", 6);
    ht_insert(ht, "desperate", 6);
    ht_insert(ht, "unbearable", 5);
    ht_insert(ht, "chaos", 5);
    ht_insert(ht, "defunct", 5);
    ht_insert(ht, "nonfunctional", 5);
    ht_insert(ht, "congestion", 4);
    ht_insert(ht, "theft", 6);
    ht_insert(ht, "robbery", 7);
    ht_insert(ht, "violence", 7);
    ht_insert(ht, "assault", 7);
    ht_insert(ht, "attack", 7);
    ht_insert(ht, "crime", 6);
    ht_insert(ht, "criminal", 6);
    ht_insert(ht, "vandalism", 5);
    ht_insert(ht, "harassment", 6);
    
    /* Moderate (3-5) */
    ht_insert(ht, "damaged", 5);
    ht_insert(ht, "damage", 5);
    ht_insert(ht, "leaking", 4);
    ht_insert(ht, "leak", 4);
    ht_insert(ht, "leakage", 4);
    ht_insert(ht, "pothole", 4);
    ht_insert(ht, "potholes", 4);
    ht_insert(ht, "garbage", 4);
    ht_insert(ht, "trash", 4);
    ht_insert(ht, "rubbish", 4);
    ht_insert(ht, "noise", 3);
    ht_insert(ht, "noisy", 3);
    ht_insert(ht, "dirty", 4);
    ht_insert(ht, "filthy", 5);
    ht_insert(ht, "smell", 4);
    ht_insert(ht, "smelly", 4);
    ht_insert(ht, "stink", 4);
    ht_insert(ht, "stinking", 4);
    ht_insert(ht, "mosquito", 4);
    ht_insert(ht, "mosquitoes", 4);
    ht_insert(ht, "rats", 4);
    ht_insert(ht, "insects", 3);
    ht_insert(ht, "pests", 3);
    ht_insert(ht, "waterlogging", 5);
    ht_insert(ht, "erosion", 4);
    ht_insert(ht, "rust", 3);
    ht_insert(ht, "rusted", 3);
    ht_insert(ht, "corroded", 4);
    ht_insert(ht, "flickering", 3);
    ht_insert(ht, "dim", 3);
    ht_insert(ht, "irregular", 3);
    ht_insert(ht, "intermittent", 3);
    ht_insert(ht, "delay", 3);
    ht_insert(ht, "slow", 3);
    ht_insert(ht, "weak", 3);
    ht_insert(ht, "poor", 3);
    ht_insert(ht, "inadequate", 4);
    ht_insert(ht, "insufficient", 4);
    ht_insert(ht, "encroachment", 4);
    ht_insert(ht, "illegal", 5);
    ht_insert(ht, "unauthorized", 5);
    ht_insert(ht, "abandoned", 4);
    ht_insert(ht, "stray", 3);
    ht_insert(ht, "dogs", 3);
    ht_insert(ht, "animals", 3);
    
    /* Low (1-2) */
    ht_insert(ht, "request", 1);
    ht_insert(ht, "suggestion", 1);
    ht_insert(ht, "minor", 2);
    ht_insert(ht, "cosmetic", 1);
    ht_insert(ht, "paint", 2);
    ht_insert(ht, "faded", 2);
    ht_insert(ht, "worn", 2);
    ht_insert(ht, "old", 1);
    ht_insert(ht, "replace", 2);
    ht_insert(ht, "upgrade", 2);
    ht_insert(ht, "improve", 2);
    ht_insert(ht, "clean", 2);
    ht_insert(ht, "maintenance", 2);
    ht_insert(ht, "repair", 3);
    ht_insert(ht, "fix", 3);
    ht_insert(ht, "install", 2);
    ht_insert(ht, "installation", 2);
    ht_insert(ht, "missing", 3);
    ht_insert(ht, "needed", 2);
    ht_insert(ht, "required", 2);
    ht_insert(ht, "new", 1);
    ht_insert(ht, "additional", 1);
    ht_insert(ht, "streetlight", 3);
    ht_insert(ht, "signboard", 2);
    ht_insert(ht, "bench", 1);
    ht_insert(ht, "park", 1);
}

static void load_negation_words(HashTable* ht) {
    ht_insert(ht, "not", 1);
    ht_insert(ht, "no", 1);
    ht_insert(ht, "never", 1);
    ht_insert(ht, "none", 1);
    ht_insert(ht, "nothing", 1);
    ht_insert(ht, "neither", 1);
    ht_insert(ht, "nor", 1);
    ht_insert(ht, "without", 1);
    ht_insert(ht, "hardly", 1);
    ht_insert(ht, "barely", 1);
    ht_insert(ht, "isn't", 1);
    ht_insert(ht, "wasn't", 1);
    ht_insert(ht, "aren't", 1);
    ht_insert(ht, "doesn't", 1);
    ht_insert(ht, "don't", 1);
    ht_insert(ht, "didn't", 1);
    ht_insert(ht, "won't", 1);
    ht_insert(ht, "can't", 1);
    ht_insert(ht, "isnt", 1);
    ht_insert(ht, "wasnt", 1);
    ht_insert(ht, "arent", 1);
    ht_insert(ht, "doesnt", 1);
    ht_insert(ht, "dont", 1);
    ht_insert(ht, "didnt", 1);
    ht_insert(ht, "wont", 1);
    ht_insert(ht, "cant", 1);
}

static void load_amplifier_words(HashTable* ht) {
    ht_insert(ht, "very", 1);
    ht_insert(ht, "extremely", 1);
    ht_insert(ht, "severely", 1);
    ht_insert(ht, "highly", 1);
    ht_insert(ht, "seriously", 1);
    ht_insert(ht, "terribly", 1);
    ht_insert(ht, "horribly", 1);
    ht_insert(ht, "awfully", 1);
    ht_insert(ht, "absolutely", 1);
    ht_insert(ht, "completely", 1);
    ht_insert(ht, "totally", 1);
    ht_insert(ht, "utterly", 1);
    ht_insert(ht, "quite", 1);
    ht_insert(ht, "really", 1);
    ht_insert(ht, "incredibly", 1);
    ht_insert(ht, "exceptionally", 1);
    ht_insert(ht, "massively", 1);
    ht_insert(ht, "enormously", 1);
    ht_insert(ht, "desperately", 1);
    ht_insert(ht, "urgently", 1);
    ht_insert(ht, "immediately", 1);
    ht_insert(ht, "rapidly", 1);
    ht_insert(ht, "major", 1);
    ht_insert(ht, "huge", 1);
    ht_insert(ht, "massive", 1);
    ht_insert(ht, "enormous", 1);
    ht_insert(ht, "terrible", 1);
    ht_insert(ht, "horrible", 1);
    ht_insert(ht, "awful", 1);
    ht_insert(ht, "worst", 1);
}

static void load_category_water(HashTable* ht) {
    ht_insert(ht, "water", 1);
    ht_insert(ht, "pipe", 1);
    ht_insert(ht, "pipes", 1);
    ht_insert(ht, "pipeline", 1);
    ht_insert(ht, "supply", 1);
    ht_insert(ht, "tap", 1);
    ht_insert(ht, "tank", 1);
    ht_insert(ht, "borewell", 1);
    ht_insert(ht, "well", 1);
    ht_insert(ht, "drain", 1);
    ht_insert(ht, "drainage", 1);
    ht_insert(ht, "sewage", 1);
    ht_insert(ht, "sewer", 1);
    ht_insert(ht, "sewerage", 1);
    ht_insert(ht, "drinking", 1);
    ht_insert(ht, "contaminated", 1);
    ht_insert(ht, "dirty", 1);
    ht_insert(ht, "muddy", 1);
    ht_insert(ht, "waterlogging", 1);
    ht_insert(ht, "flood", 1);
    ht_insert(ht, "flooding", 1);
    ht_insert(ht, "leaking", 1);
    ht_insert(ht, "leak", 1);
    ht_insert(ht, "burst", 1);
    ht_insert(ht, "overflow", 1);
    ht_insert(ht, "overflowing", 1);
    ht_insert(ht, "stagnant", 1);
    ht_insert(ht, "rainwater", 1);
    ht_insert(ht, "clogged", 1);
}

static void load_category_electricity(HashTable* ht) {
    ht_insert(ht, "electricity", 1);
    ht_insert(ht, "electric", 1);
    ht_insert(ht, "electrical", 1);
    ht_insert(ht, "power", 1);
    ht_insert(ht, "voltage", 1);
    ht_insert(ht, "transformer", 1);
    ht_insert(ht, "wire", 1);
    ht_insert(ht, "wires", 1);
    ht_insert(ht, "cable", 1);
    ht_insert(ht, "cables", 1);
    ht_insert(ht, "pole", 1);
    ht_insert(ht, "poles", 1);
    ht_insert(ht, "meter", 1);
    ht_insert(ht, "blackout", 1);
    ht_insert(ht, "outage", 1);
    ht_insert(ht, "shortcircuit", 1);
    ht_insert(ht, "sparking", 1);
    ht_insert(ht, "spark", 1);
    ht_insert(ht, "electrocution", 1);
    ht_insert(ht, "electrocuted", 1);
    ht_insert(ht, "flickering", 1);
    ht_insert(ht, "streetlight", 1);
    ht_insert(ht, "streetlights", 1);
    ht_insert(ht, "lamp", 1);
    ht_insert(ht, "bulb", 1);
    ht_insert(ht, "light", 1);
    ht_insert(ht, "lights", 1);
    ht_insert(ht, "inverter", 1);
    ht_insert(ht, "generator", 1);
    ht_insert(ht, "current", 1);
}

static void load_category_roads(HashTable* ht) {
    ht_insert(ht, "road", 1);
    ht_insert(ht, "roads", 1);
    ht_insert(ht, "highway", 1);
    ht_insert(ht, "street", 1);
    ht_insert(ht, "lane", 1);
    ht_insert(ht, "pothole", 1);
    ht_insert(ht, "potholes", 1);
    ht_insert(ht, "crack", 1);
    ht_insert(ht, "cracked", 1);
    ht_insert(ht, "bridge", 1);
    ht_insert(ht, "footpath", 1);
    ht_insert(ht, "sidewalk", 1);
    ht_insert(ht, "pavement", 1);
    ht_insert(ht, "divider", 1);
    ht_insert(ht, "signal", 1);
    ht_insert(ht, "traffic", 1);
    ht_insert(ht, "sign", 1);
    ht_insert(ht, "signboard", 1);
    ht_insert(ht, "speed", 1);
    ht_insert(ht, "breaker", 1);
    ht_insert(ht, "construction", 1);
    ht_insert(ht, "asphalt", 1);
    ht_insert(ht, "tar", 1);
    ht_insert(ht, "sinkhole", 1);
    ht_insert(ht, "manhole", 1);
    ht_insert(ht, "flyover", 1);
    ht_insert(ht, "crossing", 1);
    ht_insert(ht, "junction", 1);
    ht_insert(ht, "barricade", 1);
    ht_insert(ht, "railing", 1);
}

static void load_category_sanitation(HashTable* ht) {
    ht_insert(ht, "garbage", 1);
    ht_insert(ht, "trash", 1);
    ht_insert(ht, "waste", 1);
    ht_insert(ht, "rubbish", 1);
    ht_insert(ht, "litter", 1);
    ht_insert(ht, "dump", 1);
    ht_insert(ht, "dumping", 1);
    ht_insert(ht, "dustbin", 1);
    ht_insert(ht, "bin", 1);
    ht_insert(ht, "sanitation", 1);
    ht_insert(ht, "hygiene", 1);
    ht_insert(ht, "cleanliness", 1);
    ht_insert(ht, "clean", 1);
    ht_insert(ht, "dirty", 1);
    ht_insert(ht, "filth", 1);
    ht_insert(ht, "filthy", 1);
    ht_insert(ht, "smell", 1);
    ht_insert(ht, "smelly", 1);
    ht_insert(ht, "stink", 1);
    ht_insert(ht, "stinking", 1);
    ht_insert(ht, "mosquito", 1);
    ht_insert(ht, "mosquitoes", 1);
    ht_insert(ht, "rats", 1);
    ht_insert(ht, "pests", 1);
    ht_insert(ht, "insects", 1);
    ht_insert(ht, "toilet", 1);
    ht_insert(ht, "latrine", 1);
    ht_insert(ht, "urinal", 1);
    ht_insert(ht, "defecation", 1);
    ht_insert(ht, "compost", 1);
    ht_insert(ht, "recycling", 1);
    ht_insert(ht, "sweeper", 1);
}

static void load_category_safety(HashTable* ht) {
    ht_insert(ht, "safety", 1);
    ht_insert(ht, "safe", 1);
    ht_insert(ht, "unsafe", 1);
    ht_insert(ht, "security", 1);
    ht_insert(ht, "police", 1);
    ht_insert(ht, "crime", 1);
    ht_insert(ht, "criminal", 1);
    ht_insert(ht, "theft", 1);
    ht_insert(ht, "robbery", 1);
    ht_insert(ht, "assault", 1);
    ht_insert(ht, "attack", 1);
    ht_insert(ht, "violence", 1);
    ht_insert(ht, "violent", 1);
    ht_insert(ht, "harassment", 1);
    ht_insert(ht, "vandalism", 1);
    ht_insert(ht, "threatening", 1);
    ht_insert(ht, "suspicious", 1);
    ht_insert(ht, "gang", 1);
    ht_insert(ht, "drugs", 1);
    ht_insert(ht, "drunk", 1);
    ht_insert(ht, "gambling", 1);
    ht_insert(ht, "nuisance", 1);
    ht_insert(ht, "fire", 1);
    ht_insert(ht, "accident", 1);
    ht_insert(ht, "danger", 1);
    ht_insert(ht, "dangerous", 1);
    ht_insert(ht, "hazard", 1);
    ht_insert(ht, "hazardous", 1);
    ht_insert(ht, "emergency", 1);
    ht_insert(ht, "alarm", 1);
    ht_insert(ht, "cctv", 1);
    ht_insert(ht, "camera", 1);
    ht_insert(ht, "patrol", 1);
    ht_insert(ht, "guard", 1);
}

/* ─── Initialize the NLP Analyzer ─── */
void nlp_init(NLPAnalyzer* analyzer) {
    ht_init(&analyzer->keyword_dict, HT_DEFAULT_SIZE);
    ht_init(&analyzer->negation_dict, HT_DEFAULT_SIZE);
    ht_init(&analyzer->amplifier_dict, HT_DEFAULT_SIZE);
    ht_init(&analyzer->category_water, HT_DEFAULT_SIZE);
    ht_init(&analyzer->category_electricity, HT_DEFAULT_SIZE);
    ht_init(&analyzer->category_roads, HT_DEFAULT_SIZE);
    ht_init(&analyzer->category_sanitation, HT_DEFAULT_SIZE);
    ht_init(&analyzer->category_safety, HT_DEFAULT_SIZE);
    
    load_severity_keywords(&analyzer->keyword_dict);
    load_negation_words(&analyzer->negation_dict);
    load_amplifier_words(&analyzer->amplifier_dict);
    load_category_water(&analyzer->category_water);
    load_category_electricity(&analyzer->category_electricity);
    load_category_roads(&analyzer->category_roads);
    load_category_sanitation(&analyzer->category_sanitation);
    load_category_safety(&analyzer->category_safety);
    
    analyzer->initialized = 1;
}

/* ─── Dictionary lookup with simple inflections ───
 * The dictionaries hold base forms, so "accidents", "injuries", "damaged"
 * and "outages" used to match nothing. If the exact word is absent, try
 * removing a plural or past-tense ending. Returns the stored value, or 0. */
static int lookup_inflected(HashTable* ht, const char* word) {
    char stem[MAX_TOKEN_LEN];
    size_t len = strlen(word);
    int v = ht_search(ht, word);
    if (v > 0 || len < 4) return v > 0 ? v : 0;

    struct { const char* ending; const char* replacement; } rules[] = {
        {"ies", "y"}, {"es", ""}, {"s", ""}, {"ed", ""}, {"ed", "e"}, {"d", ""},
        {"ing", ""}, {"ing", "e"},
    };
    for (size_t r = 0; r < sizeof(rules) / sizeof(rules[0]); r++) {
        size_t el = strlen(rules[r].ending);
        if (len <= el + 2 || strcmp(word + len - el, rules[r].ending) != 0) continue;
        if (strcmp(rules[r].ending, "s") == 0 && word[len - 2] == 's') continue;  /* "glass" */
        snprintf(stem, sizeof(stem), "%.*s%s", (int)(len - el), word, rules[r].replacement);
        v = ht_search(ht, stem);
        if (v > 0) return v;
    }
    return 0;
}

/* Words that may stand between a modifier and its keyword: in "not a fire"
 * the "a" used to cancel the negation (scored 10/10) */
static int is_article(const char* w) {
    return strcmp(w, "a") == 0 || strcmp(w, "an") == 0 || strcmp(w, "the") == 0 ||
           strcmp(w, "any") == 0;
}

/* ─── Analyze Severity (with explanation) ───
 * Score = average weight of matched severity keywords + a bonus for the
 * number of hits. An amplifier ("very", "major") before a keyword multiplies
 * its weight by AMPLIFIER_MULT; a negation ("not", "no") multiplies it by
 * NEGATION_MULT. A modifier applies to the next word, skipping articles
 * ("not a fire").
 *
 * Scoring by the strongest keyword instead of the average was tried and
 * measured against the 150 hand-labelled templates in generate_dataset.py:
 * it overestimates (mean error 1.83 vs 1.41, Spearman 0.67 vs 0.75), so the
 * average is kept. */
int analyze_severity_explain(NLPAnalyzer* analyzer, const char* title,
                             const char* description, KeywordHit* hits,
                             int max_hits, int* hit_count, double* hit_bonus_out) {
    char combined[MAX_TITLE_LEN + MAX_DESC_LEN + 2];
    snprintf(combined, sizeof(combined), "%s %s", title, description);

    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int num_tokens = tokenize(combined, tokens, MAX_TOKENS);

    double total_score = 0.0;
    int keyword_hits = 0;
    int negate_next = 0;
    int amplify_next = 0;

    for (int i = 0; i < num_tokens; i++) {
        /* Check for negation words */
        if (ht_contains(&analyzer->negation_dict, tokens[i])) {
            negate_next = 1;
            continue;
        }

        /* Check for amplifier words */
        if (ht_contains(&analyzer->amplifier_dict, tokens[i])) {
            amplify_next = 1;
            continue;
        }

        /* An article keeps a pending modifier for the word after it */
        if (is_article(tokens[i])) continue;

        /* Check for severity keywords */
        int weight = lookup_inflected(&analyzer->keyword_dict, tokens[i]);
        if (weight > 0) {
            double adjusted_weight = (double)weight;
            int modifier = KW_PLAIN;

            if (amplify_next) {
                adjusted_weight *= AMPLIFIER_MULT;
                amplify_next = 0;
                modifier |= KW_AMPLIFIED;
            }

            if (negate_next) {
                adjusted_weight *= NEGATION_MULT;
                negate_next = 0;
                modifier |= KW_NEGATED;
            }

            if (hits && hit_count && *hit_count < max_hits) {
                KeywordHit* h = &hits[(*hit_count)++];
                strncpy(h->word, tokens[i], MAX_TOKEN_LEN - 1);
                h->word[MAX_TOKEN_LEN - 1] = '\0';
                h->weight = weight;
                h->applied = adjusted_weight;
                h->modifier = modifier;
            }

            total_score += adjusted_weight;
            keyword_hits++;
        } else {
            /* Reset modifiers if no keyword follows */
            negate_next = 0;
            amplify_next = 0;
        }
    }

    if (hit_bonus_out) *hit_bonus_out = 0.0;
    if (keyword_hits == 0) return MIN_SEVERITY;

    /* Compute average score */
    double avg_score = total_score / keyword_hits;

    /* Add bonus for multiple keyword hits */
    double hit_bonus = 0.0;
    if (keyword_hits >= 5) hit_bonus = 2.0;
    else if (keyword_hits >= 3) hit_bonus = 1.0;
    else if (keyword_hits >= 2) hit_bonus = 0.5;
    if (hit_bonus_out) *hit_bonus_out = hit_bonus;

    double final_score = avg_score + hit_bonus;

    /* Clamp to [1, 10] */
    int severity = (int)(final_score + 0.5);  /* Round */
    if (severity < MIN_SEVERITY) severity = MIN_SEVERITY;
    if (severity > MAX_SEVERITY) severity = MAX_SEVERITY;

    return severity;
}

int analyze_severity(NLPAnalyzer* analyzer, const char* title, const char* description) {
    return analyze_severity_explain(analyzer, title, description, NULL, 0, NULL, NULL);
}

/* ─── Category keyword scores ─── */
const char* const NLP_CATEGORIES[NLP_NUM_CATEGORIES] = {
    "Water Supply", "Electricity", "Roads & Transport",
    "Sanitation", "Public Safety"
};

void category_scores(NLPAnalyzer* analyzer, const char* title,
                     const char* description, int scores[NLP_NUM_CATEGORIES]) {
    char combined[MAX_TITLE_LEN + MAX_DESC_LEN + 2];
    snprintf(combined, sizeof(combined), "%s %s", title, description);

    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int num_tokens = tokenize(combined, tokens, MAX_TOKENS);

    for (int c = 0; c < NLP_NUM_CATEGORIES; c++) scores[c] = 0;
    /* 0=Water, 1=Electricity, 2=Roads, 3=Sanitation, 4=Safety */

    for (int i = 0; i < num_tokens; i++) {
        if (lookup_inflected(&analyzer->category_water, tokens[i]))       scores[0]++;
        if (lookup_inflected(&analyzer->category_electricity, tokens[i])) scores[1]++;
        if (lookup_inflected(&analyzer->category_roads, tokens[i]))       scores[2]++;
        if (lookup_inflected(&analyzer->category_sanitation, tokens[i]))  scores[3]++;
        if (lookup_inflected(&analyzer->category_safety, tokens[i]))      scores[4]++;
    }
}

/* ─── Suggest Category ─── */
void suggest_category(NLPAnalyzer* analyzer, const char* title,
                      const char* description, char* category_out) {
    int scores[NLP_NUM_CATEGORIES];
    category_scores(analyzer, title, description, scores);

    int max_idx = 0;
    for (int i = 1; i < NLP_NUM_CATEGORIES; i++) {
        if (scores[i] > scores[max_idx]) {
            max_idx = i;
        }
    }

    if (scores[max_idx] > 0) {
        strcpy(category_out, NLP_CATEGORIES[max_idx]);
    } else {
        strcpy(category_out, "General");
    }
}

/* ─── Detect Irrelevant / Spam ─── */
int detect_irrelevant(const char* title, const char* description) {
    int title_len = (int)strlen(title);
    int desc_len = (int)strlen(description);
    
    /* Check 1: Too short */
    if (desc_len < 10 && title_len < 10) {
        return 1;
    }
    
    /* Combine text for further checks */
    char combined[MAX_TITLE_LEN + MAX_DESC_LEN + 2];
    snprintf(combined, sizeof(combined), "%s %s", title, description);
    int combined_len = (int)strlen(combined);
    
    /* Check 2: Gibberish — more than 6 consecutive consonants ('y' counts
     * as a vowel: "heavy rhythms" used to be flagged as spam) */
    int consecutive_consonants = 0;
    for (int i = 0; i < combined_len; i++) {
        char c = (char)tolower((unsigned char)combined[i]);
        if (isalpha((unsigned char)c)) {
            if (c != 'a' && c != 'e' && c != 'i' && c != 'o' && c != 'u' && c != 'y') {
                consecutive_consonants++;
                if (consecutive_consonants > 6) return 1;
            } else {
                consecutive_consonants = 0;
            }
        } else {
            consecutive_consonants = 0;
        }
    }
    
    /* Check 3: Low vowel ratio */
    int vowels = 0, alphabetic = 0;
    for (int i = 0; i < combined_len; i++) {
        char c = (char)tolower((unsigned char)combined[i]);
        if (isalpha((unsigned char)c)) {
            alphabetic++;
            if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y') {
                vowels++;
            }
        }
    }
    if (alphabetic > 10 && (double)vowels / alphabetic < 0.20) {
        return 1;
    }
    
    /* Check 4: the same letter 5+ times in a row ("aaaaa"). Letters only:
     * a phone number (9999999999), an amount (Rs 1000000) or "!!!!!!" used
     * to flag a real, urgent complaint as spam. */
    int repeated = 1;
    for (int i = 1; i < combined_len; i++) {
        unsigned char c = (unsigned char)combined[i];
        if (isalpha(c) && tolower(c) == tolower((unsigned char)combined[i - 1])) {
            repeated++;
            if (repeated >= 5) return 1;
        } else {
            repeated = 1;
        }
    }
    
    /* Check 5: Too few words (< 3) */
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int num_tokens = tokenize(combined, tokens, MAX_TOKENS);
    int meaningful = 0;
    for (int i = 0; i < num_tokens; i++) {
        if (strlen(tokens[i]) > 2) meaningful++;
    }
    if (meaningful < 3) {
        return 1;
    }
    
    return 0;  /* Not irrelevant */
}

/* ─── Free NLP Analyzer ─── */
void nlp_free(NLPAnalyzer* analyzer) {
    if (!analyzer->initialized) return;
    
    ht_free(&analyzer->keyword_dict);
    ht_free(&analyzer->negation_dict);
    ht_free(&analyzer->amplifier_dict);
    ht_free(&analyzer->category_water);
    ht_free(&analyzer->category_electricity);
    ht_free(&analyzer->category_roads);
    ht_free(&analyzer->category_sanitation);
    ht_free(&analyzer->category_safety);
    
    analyzer->initialized = 0;
}
