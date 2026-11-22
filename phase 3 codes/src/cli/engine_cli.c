/*
 * engine_cli.c
 * Command-line bridge to the C engine, used by the web server (web/server.py).
 *
 * Every command prints exactly one JSON object on stdout:
 *   success → {"ok": true, ...}            exit code 0
 *   failure → {"ok": false, "code": "...", "error": "..."}  exit code 1
 * Free text (titles, descriptions, passwords) is read from stdin, one field
 * per line, so it never passes through the shell.
 *
 * The working directory is the project root: $SPCA_ROOT if set, otherwise
 * the parent of the directory holding this executable (like the console app).
 *
 *   engine_cli graph
 *   engine_cli route <from> <to>
 *   engine_cli nlp                         stdin: title, description
 *   engine_cli analyze                     stdin: user_id, location_id, category, title, description
 *   engine_cli submit                      stdin: user_id, username, location_id, category, title, description
 *   engine_cli queue [limit]
 *   engine_cli compare <id_a> <id_b>
 *   engine_cli review <id> <admin>
 *   engine_cli escalate <id> <severity> <admin>
 *   engine_cli assign-dept <id> <department_id> <admin>
 *   engine_cli assign-worker <id> <worker_id> <officer>
 *   engine_cli start <id> <worker_id>
 *   engine_cli complete <id> <worker_id>
 *   engine_cli set-location <user_id> <location_id>
 *   engine_cli set-availability <user_id> <0|1>   (worker: busy / available)
 *   engine_cli log-signin <user_id>               (log a website sign-in)
 *   engine_cli signup                      stdin: username, password, full_name, phone, email
 *   engine_cli reanalyze [--dry-run]
 *   engine_cli refresh-departments
 */

#include "../../include/engine/complaint_types.h"
#include "../../include/engine/file_manager.h"
#include "../../include/engine/nlp_analyzer.h"
#include "../../include/engine/duplicate_detector.h"
#include "../../include/engine/priority_queue.h"
#include "../../include/engine/route_optimizer.h"
#include "../../include/engine/workflow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#define CHANNEL "web"
#define MAX_HITS 64

/* ═══════════════════════════════════════════════════════════════════
 *  JSON output
 * ═══════════════════════════════════════════════════════════════════ */

static void json_str(const char* s) {
    putchar('"');
    for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\n': fputs("\\n", stdout); break;
            case '\r': fputs("\\r", stdout); break;
            case '\t': fputs("\\t", stdout); break;
            default:
                if (*p < 0x20) printf("\\u%04x", *p);
                else putchar(*p);
        }
    }
    putchar('"');
}

static int fail(const char* code, const char* message) {
    fputs("{\"ok\":false,\"code\":", stdout);
    json_str(code);
    fputs(",\"error\":", stdout);
    json_str(message);
    fputs("}\n", stdout);
    return 1;
}

static const char* wf_code_name(int code) {
    switch (code) {
        case WF_NOT_FOUND: return "not_found";
        case WF_FORBIDDEN: return "forbidden";
        case WF_INVALID:   return "invalid";
        case WF_IO_ERROR:  return "io_error";
        default:           return "error";
    }
}

static int fail_wf(WfResult r) {
    return fail(wf_code_name(r.code), r.message);
}

static void json_route(CityGraph* graph, const RouteResult* r) {
    fputs("{\"path\":[", stdout);
    for (int i = 0; i < r->path_length; i++) printf("%s%d", i ? "," : "", r->path[i]);
    fputs("],\"names\":[", stdout);
    for (int i = 0; i < r->path_length; i++) {
        if (i) putchar(',');
        json_str(get_location_name(graph, r->path[i]));
    }
    printf("],\"distance\":%d,\"steps\":[", r->total_distance);
    for (int i = 0; i + 1 < r->path_length; i++) {
        int a = r->path[i], b = r->path[i + 1];
        printf("%s{\"from\":%d,\"to\":%d,\"km\":%d}", i ? "," : "", a, b, graph->adj_matrix[a][b]);
    }
    fputs("]}", stdout);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Input helpers
 * ═══════════════════════════════════════════════════════════════════ */

static void read_field(char* buf, size_t cap) {
    if (!fgets(buf, (int)cap, stdin)) { buf[0] = '\0'; return; }
    size_t len = strcspn(buf, "\r\n");
    if (buf[len] == '\0' && len == cap - 1) {
        /* Line longer than the field: discard the rest of it */
        int ch;
        while ((ch = getchar()) != EOF && ch != '\n') {}
    }
    buf[len] = '\0';
}

static int parse_int(const char* s, int* out) {
    char* end;
    long v;
    if (!s || !*s) return 0;
    errno = 0;
    v = strtol(s, &end, 10);
    if (*end != '\0' || errno == ERANGE || v > INT_MAX || v < INT_MIN) return 0;
    *out = (int)v;
    return 1;
}

/* 0 if SPCA_ROOT names a folder that can't be entered: carrying on would
 * quietly create a fresh data/ (with the default accounts) wherever this
 * process happened to start */
static int set_project_root(void) {
    const char* env = getenv("SPCA_ROOT");
    if (env && env[0]) return chdir(env) == 0;
#ifdef _WIN32
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) > 0) {
        char* slash = strrchr(path, '\\');
        if (!slash) slash = strrchr(path, '/');
        if (slash) {
            *slash = '\0';
            char* parent = strrchr(path, '\\');
            if (!parent) parent = strrchr(path, '/');
            if (parent) { *parent = '\0'; chdir(path); }
        }
    }
#endif
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Analysis output (shared by nlp / analyze / submit)
 * ═══════════════════════════════════════════════════════════════════ */

static void json_analysis_body(NLPAnalyzer* nlp, const Complaint* c, const char* suggested) {
    KeywordHit hits[MAX_HITS];
    int hit_count = 0;
    double bonus = 0.0;
    int scores[NLP_NUM_CATEGORIES];

    int severity = analyze_severity_explain(nlp, c->title, c->description, hits, MAX_HITS,
                                            &hit_count, &bonus);
    category_scores(nlp, c->title, c->description, scores);

    printf("\"severity\":%d,\"severity_label\":", severity);
    json_str(wf_severity_label(severity));
    fputs(",\"category\":", stdout);
    json_str(suggested);
    fputs(",\"category_scores\":{", stdout);
    for (int i = 0; i < NLP_NUM_CATEGORIES; i++) {
        if (i) putchar(',');
        json_str(NLP_CATEGORIES[i]);
        printf(":%d", scores[i]);
    }
    printf("},\"irrelevant\":%s,\"keywords\":[", c->is_irrelevant ? "true" : "false");
    for (int i = 0; i < hit_count; i++) {
        if (i) putchar(',');
        fputs("{\"word\":", stdout);
        json_str(hits[i].word);
        printf(",\"weight\":%d,\"applied\":%.2f,\"modifier\":", hits[i].weight, hits[i].applied);
        if (hits[i].modifier == (KW_AMPLIFIED | KW_NEGATED)) json_str("amplified+negated");
        else if (hits[i].modifier == KW_AMPLIFIED) json_str("amplified");
        else if (hits[i].modifier == KW_NEGATED) json_str("negated");
        else fputs("null", stdout);
        putchar('}');
    }
    printf("],\"hit_bonus\":%.1f", bonus);
}

static void json_duplicate(int dup_id, double similarity, double threshold) {
    Complaint original;
    fputs(",\"duplicate\":", stdout);
    if (dup_id < 0 || !find_complaint(dup_id, &original)) {
        fputs("null", stdout);
        return;
    }
    printf("{\"id\":%d,\"title\":", dup_id);
    json_str(original.title);
    printf(",\"similarity\":%.3f,\"threshold\":%.2f}", similarity, threshold);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Commands
 * ═══════════════════════════════════════════════════════════════════ */

static int cmd_graph(void) {
    CityGraph g;
    int first = 1;
    init_city_graph(&g);
    fputs("{\"ok\":true,\"nodes\":[", stdout);
    for (int i = 0; i < g.num_locations; i++) {
        printf("%s{\"id\":%d,\"name\":", i ? "," : "", i);
        json_str(g.locations[i].name);
        putchar('}');
    }
    fputs("],\"edges\":[", stdout);
    for (int i = 0; i < g.num_locations; i++) {
        for (int j = i + 1; j < g.num_locations; j++) {
            if (g.adj_matrix[i][j] > 0) {
                printf("%s{\"a\":%d,\"b\":%d,\"km\":%d}", first ? "" : ",", i, j, g.adj_matrix[i][j]);
                first = 0;
            }
        }
    }
    fputs("]}\n", stdout);
    return 0;
}

static int cmd_route(int argc, char** argv) {
    CityGraph g;
    RouteResult r;
    int from, to;
    if (argc < 4 || !parse_int(argv[2], &from) || !parse_int(argv[3], &to))
        return fail("invalid", "usage: route <from> <to>");
    init_city_graph(&g);
    if (from < 0 || from >= g.num_locations || to < 0 || to >= g.num_locations)
        return fail("invalid", "Unknown location ID.");
    if (!dijkstra(&g, from, to, &r)) return fail("not_found", "No route between these locations.");
    fputs("{\"ok\":true,\"route\":", stdout);
    json_route(&g, &r);
    fputs("}\n", stdout);
    return 0;
}

static int cmd_nlp(void) {
    NLPAnalyzer nlp;
    Complaint c;
    char suggested[MAX_CATEGORY_LEN];
    memset(&c, 0, sizeof(c));
    read_field(c.title, sizeof(c.title));
    read_field(c.description, sizeof(c.description));

    nlp_init(&nlp);
    wf_analyze_draft(&nlp, &c, suggested);
    fputs("{\"ok\":true,", stdout);
    json_analysis_body(&nlp, &c, suggested);
    fputs(",\"duplicate\":null}\n", stdout);
    nlp_free(&nlp);
    return 0;
}

static int cmd_analyze(void) {
    NLPAnalyzer nlp;
    Complaint c;
    CityGraph g;
    char buf[64], suggested[MAX_CATEGORY_LEN];
    double sim = 0.0, threshold = 0.0;
    int dup_id = -1;

    memset(&c, 0, sizeof(c));
    read_field(buf, sizeof(buf));
    if (!parse_int(buf, &c.user_id)) c.user_id = -1;
    read_field(buf, sizeof(buf));
    if (!parse_int(buf, &c.location_id)) c.location_id = -1;
    read_field(c.category, sizeof(c.category));
    read_field(c.title, sizeof(c.title));
    read_field(c.description, sizeof(c.description));

    nlp_init(&nlp);
    wf_analyze_draft(&nlp, &c, suggested);
    if (c.category[0] == '\0') strcpy(c.category, suggested);

    init_city_graph(&g);
    if (c.location_id >= 0 && c.location_id < g.num_locations && c.title[0]) {
        Complaint* existing = (Complaint*)malloc(sizeof(Complaint) * MAX_COMPLAINTS);
        if (existing) {
            int n = load_complaints(existing, MAX_COMPLAINTS);
            c.id = 0;
            dup_id = check_duplicate_ex(&c, existing, n, &sim, &threshold);
            free(existing);
        }
    }

    fputs("{\"ok\":true,", stdout);
    json_analysis_body(&nlp, &c, suggested);
    json_duplicate(dup_id, sim, threshold);
    fputs("}\n", stdout);
    nlp_free(&nlp);
    return 0;
}

static int cmd_submit(void) {
    NLPAnalyzer nlp;
    Complaint c;
    char buf[64], username[MAX_USERNAME_LEN], suggested[MAX_CATEGORY_LEN];
    double sim = 0.0, threshold = 0.0;

    memset(&c, 0, sizeof(c));
    read_field(buf, sizeof(buf));
    if (!parse_int(buf, &c.user_id) || c.user_id <= 0) return fail("invalid", "Missing user ID.");
    read_field(username, sizeof(username));
    read_field(buf, sizeof(buf));
    if (!parse_int(buf, &c.location_id)) return fail("invalid", "Choose a location.");
    read_field(c.category, sizeof(c.category));
    read_field(c.title, sizeof(c.title));
    read_field(c.description, sizeof(c.description));

    nlp_init(&nlp);
    wf_analyze_draft(&nlp, &c, suggested);
    if (c.category[0] == '\0') strcpy(c.category, suggested);

    WfResult r = wf_finalize_submission(&c, username, CHANNEL, &sim, &threshold);
    if (r.code != WF_OK) { nlp_free(&nlp); return fail_wf(r); }

    printf("{\"ok\":true,\"id\":%d,\"message\":", c.id);
    json_str(r.message);
    fputs(",\"analysis\":{", stdout);
    json_analysis_body(&nlp, &c, suggested);
    json_duplicate(c.is_duplicate ? c.duplicate_of : -1, sim, threshold);
    fputs("}}\n", stdout);
    nlp_free(&nlp);
    return 0;
}

static int cmd_queue(int argc, char** argv) {
    int limit = MAX_COMPLAINTS;
    if (argc >= 3 && !parse_int(argv[2], &limit)) return fail("invalid", "usage: queue [limit]");

    Complaint* all = (Complaint*)malloc(sizeof(Complaint) * MAX_COMPLAINTS);
    if (!all) return fail("io_error", "Out of memory.");
    int total = load_complaints(all, MAX_COMPLAINTS);

    PriorityQueue pq;
    pq_init(&pq, 64);
    for (int i = 0; i < total; i++) {
        if (strcmp(all[i].status, "PENDING") == 0) pq_insert(&pq, all[i]);
    }
    int pending = pq_size(&pq);

    printf("{\"ok\":true,\"total\":%d,\"ids\":[", pending);
    for (int i = 0; i < limit && !pq_is_empty(&pq); i++) {
        Complaint top = pq_extract_max(&pq);
        printf("%s%d", i ? "," : "", top.id);
    }
    fputs("]}\n", stdout);
    pq_free(&pq);
    free(all);
    return 0;
}

static int cmd_compare(int argc, char** argv) {
    int a_id, b_id;
    Complaint a, b;
    SimilarityBreakdown s;
    if (argc < 4 || !parse_int(argv[2], &a_id) || !parse_int(argv[3], &b_id))
        return fail("invalid", "usage: compare <id_a> <id_b>");
    if (!find_complaint(a_id, &a) || !find_complaint(b_id, &b))
        return fail("not_found", "Complaint not found.");
    compute_similarity_ex(&a, &b, &s);
    printf("{\"ok\":true,\"jaccard\":%.3f,\"levenshtein\":%.3f,\"title\":%.3f,"
           "\"category\":%.0f,\"location\":%.0f,\"title_penalty\":%.2f,"
           "\"combined\":%.3f,\"threshold\":%.2f,\"same_user\":%s}\n",
           s.jaccard, s.levenshtein, s.title, s.category, s.location, s.title_penalty,
           s.combined, duplicate_threshold(&a, &b), a.user_id == b.user_id ? "true" : "false");
    return 0;
}

static int print_wf(WfResult r) {
    if (r.code != WF_OK) return fail_wf(r);
    fputs("{\"ok\":true,\"message\":", stdout);
    json_str(r.message);
    fputs("}\n", stdout);
    return 0;
}

static int need_ints(int argc, char** argv, int count, int* out, const char* usage) {
    if (argc < 2 + count) return 0;
    for (int i = 0; i < count; i++) {
        if (!parse_int(argv[2 + i], &out[i])) return 0;
    }
    (void)usage;
    return 1;
}

/* Route from the assigned worker's current location to the complaint */
static void json_worker_route(int complaint_id) {
    Complaint c;
    UserRecord w;
    CityGraph g;
    RouteResult r;
    fputs(",\"route\":", stdout);
    init_city_graph(&g);
    if (find_complaint(complaint_id, &c) && c.worker_id > 0 && find_user_by_id(c.worker_id, &w) &&
        dijkstra(&g, w.current_location_id, c.location_id, &r)) {
        json_route(&g, &r);
    } else {
        fputs("null", stdout);
    }
}

/* Submission order: date, then ID for complaints made in the same minute */
static int by_submission(const void* a, const void* b) {
    const Complaint* x = (const Complaint*)a;
    const Complaint* y = (const Complaint*)b;
    int d = strcmp(x->submitted_date, y->submitted_date);
    if (d) return d;
    return (x->id > y->id) - (x->id < y->id);
}

static int by_id(const void* a, const void* b) {
    const Complaint* x = (const Complaint*)a;
    const Complaint* y = (const Complaint*)b;
    return (x->id > y->id) - (x->id < y->id);
}

static int cmd_reanalyze(int argc, char** argv) {
    int dry_run = 0;
    if (argc >= 3) {
        /* Anything but exactly --dry-run used to do the real rewrite */
        if (argc > 3 || strcmp(argv[2], "--dry-run") != 0) return fail("invalid", "usage: reanalyze [--dry-run]");
        dry_run = 1;
    }
    NLPAnalyzer nlp;
    Complaint* all = (Complaint*)malloc(sizeof(Complaint) * MAX_COMPLAINTS);
    int* order = (int*)malloc(sizeof(int) * MAX_COMPLAINTS);
    if (!all || !order) { free(all); free(order); return fail("io_error", "Out of memory."); }
    /* Hold the lock from loading to saving, so no submission made meanwhile is lost */
    if (!dry_run && !fm_lock()) { free(all); free(order); return fail("io_error", "The data files are busy. Try again."); }
    int n = load_complaints(all, MAX_COMPLAINTS);
    int changed_nlp = 0, changed_irr = 0, changed_dup = 0, dups = 0, irr = 0;

    /* Remember the file order: save_all_complaints writes records back in it */
    for (int i = 0; i < n; i++) order[i] = all[i].id;

    nlp_init(&nlp);
    /* Process in submission order (date, not ID: generated IDs are not in
     * date order, and "duplicate of an earlier complaint" then pointed at a
     * later one); each complaint is compared only with those submitted
     * before it, exactly as if they had arrived one after another. */
    qsort(all, n, sizeof(Complaint), by_submission);
    for (int i = 0; i < n; i++) {
        Complaint* c = &all[i];
        int old_nlp = c->nlp_score, old_irr = c->is_irrelevant;
        int old_dup = c->is_duplicate, old_of = c->duplicate_of;

        c->nlp_score = analyze_severity(&nlp, c->title, c->description);
        c->is_irrelevant = detect_irrelevant(c->title, c->description);
        if (c->is_irrelevant) {
            c->is_duplicate = 0;
            c->duplicate_of = -1;
        } else {
            check_duplicate(c, all, i);   /* only the i earlier complaints */
        }

        if (c->nlp_score != old_nlp) changed_nlp++;
        if (c->is_irrelevant != old_irr) changed_irr++;
        if (c->is_duplicate != old_dup || c->duplicate_of != old_of) changed_dup++;
        if (c->is_duplicate) dups++;
        if (c->is_irrelevant) irr++;
    }
    nlp_free(&nlp);

    if (!dry_run) {
        /* Put the records back in file order before writing them back */
        Complaint* by_file = (Complaint*)malloc(sizeof(Complaint) * (n > 0 ? n : 1));
        int saved = 0;
        if (by_file) {
            qsort(all, n, sizeof(Complaint), by_id);
            for (int i = 0; i < n; i++) {
                /* IDs are unique in a consistent file; find each one's record */
                Complaint key;
                key.id = order[i];
                Complaint* hit = (Complaint*)bsearch(&key, all, n, sizeof(Complaint), by_id);
                if (!hit) break;
                by_file[i] = *hit;
            }
            saved = save_all_complaints(by_file, n);
            free(by_file);
        }
        if (!saved) {
            fm_unlock();
            free(all);
            free(order);
            return fail("io_error", "Could not write complaints.txt (run tools/repair_data.py if IDs repeat).");
        }
        refresh_department_counts();
        log_activity("Dataset re-analysed with the C engine (nlp_score, spam and duplicate flags)");
        fm_unlock();
    }
    printf("{\"ok\":true,\"dry_run\":%s,\"complaints\":%d,\"duplicates\":%d,\"irrelevant\":%d,"
           "\"changed\":{\"nlp_score\":%d,\"irrelevant\":%d,\"duplicate\":%d}}\n",
           dry_run ? "true" : "false", n, dups, irr, changed_nlp, changed_irr, changed_dup);
    free(all);
    free(order);
    return 0;
}

int main(int argc, char** argv) {
    int ids[3];
    if (!set_project_root()) return fail("io_error", "SPCA_ROOT is not a folder that can be opened.");
    init_data_files();

    if (argc < 2) {
        fail("invalid", "usage: engine_cli <command> [args] (see source header)");
        return 2;
    }
    const char* cmd = argv[1];

    if (strcmp(cmd, "graph") == 0) return cmd_graph();
    if (strcmp(cmd, "route") == 0) return cmd_route(argc, argv);
    if (strcmp(cmd, "nlp") == 0) return cmd_nlp();
    if (strcmp(cmd, "analyze") == 0) return cmd_analyze();
    if (strcmp(cmd, "submit") == 0) return cmd_submit();
    if (strcmp(cmd, "queue") == 0) return cmd_queue(argc, argv);
    if (strcmp(cmd, "compare") == 0) return cmd_compare(argc, argv);
    if (strcmp(cmd, "reanalyze") == 0) return cmd_reanalyze(argc, argv);

    if (strcmp(cmd, "review") == 0) {
        if (argc < 4 || !need_ints(argc, argv, 1, ids, "")) return fail("invalid", "usage: review <id> <admin>");
        return print_wf(wf_review(ids[0], argv[3], CHANNEL));
    }
    if (strcmp(cmd, "escalate") == 0) {
        if (argc < 5 || !need_ints(argc, argv, 2, ids, "")) return fail("invalid", "usage: escalate <id> <severity> <admin>");
        return print_wf(wf_escalate(ids[0], ids[1], argv[4], CHANNEL));
    }
    if (strcmp(cmd, "assign-dept") == 0) {
        if (argc < 5 || !need_ints(argc, argv, 2, ids, "")) return fail("invalid", "usage: assign-dept <id> <department_id> <admin>");
        return print_wf(wf_assign_department(ids[0], ids[1], argv[4], CHANNEL));
    }
    if (strcmp(cmd, "assign-worker") == 0) {
        if (argc < 5 || !need_ints(argc, argv, 2, ids, "")) return fail("invalid", "usage: assign-worker <id> <worker_id> <officer>");
        WfResult r = wf_assign_worker(ids[0], ids[1], argv[4], CHANNEL);
        if (r.code != WF_OK) return fail_wf(r);
        fputs("{\"ok\":true,\"message\":", stdout);
        json_str(r.message);
        json_worker_route(ids[0]);
        fputs("}\n", stdout);
        return 0;
    }
    if (strcmp(cmd, "start") == 0) {
        RouteResult route;
        CityGraph g;
        if (!need_ints(argc, argv, 2, ids, "")) return fail("invalid", "usage: start <id> <worker_id>");
        WfResult r = wf_start_task(ids[0], ids[1], CHANNEL, &route);
        if (r.code != WF_OK) return fail_wf(r);
        init_city_graph(&g);
        fputs("{\"ok\":true,\"message\":", stdout);
        json_str(r.message);
        fputs(",\"route\":", stdout);
        json_route(&g, &route);
        fputs("}\n", stdout);
        return 0;
    }
    if (strcmp(cmd, "complete") == 0) {
        if (!need_ints(argc, argv, 2, ids, "")) return fail("invalid", "usage: complete <id> <worker_id>");
        return print_wf(wf_complete_task(ids[0], ids[1], CHANNEL));
    }
    if (strcmp(cmd, "log-signin") == 0) {
        /* Same entry as a console sign-in (MenuSystem.cpp), marked [web] */
        UserRecord u;
        char msg[256];
        if (!need_ints(argc, argv, 1, ids, "")) return fail("invalid", "usage: log-signin <user_id>");
        if (!find_user_by_id(ids[0], &u)) return fail("not_found", "User not found.");
        snprintf(msg, sizeof(msg), "User '%s' logged in (Role: %s) [" CHANNEL "]", u.username, u.role);
        log_activity(msg);
        fputs("{\"ok\":true}\n", stdout);
        return 0;
    }
    if (strcmp(cmd, "set-availability") == 0) {
        if (!need_ints(argc, argv, 2, ids, "") || (ids[1] != 0 && ids[1] != 1))
            return fail("invalid", "usage: set-availability <user_id> <0|1>");
        return print_wf(wf_set_availability(ids[0], ids[1], CHANNEL));
    }
    if (strcmp(cmd, "set-location") == 0) {
        if (!need_ints(argc, argv, 2, ids, "")) return fail("invalid", "usage: set-location <user_id> <location_id>");
        return print_wf(wf_set_location(ids[0], ids[1], CHANNEL));
    }
    if (strcmp(cmd, "signup") == 0) {
        char username[64], password[64], name[MAX_NAME_LEN], phone[32], email[MAX_EMAIL_LEN];
        UserRecord u;
        read_field(username, sizeof(username));
        read_field(password, sizeof(password));
        read_field(name, sizeof(name));
        read_field(phone, sizeof(phone));
        read_field(email, sizeof(email));
        WfResult r = wf_register_user(username, password, name, phone, email, CHANNEL, &u);
        if (r.code != WF_OK) return fail_wf(r);
        printf("{\"ok\":true,\"id\":%d,\"message\":", u.id);
        json_str(r.message);
        fputs("}\n", stdout);
        return 0;
    }
    if (strcmp(cmd, "refresh-departments") == 0) {
        if (!refresh_department_counts()) return fail("io_error", "Could not update departments.txt.");
        fputs("{\"ok\":true}\n", stdout);
        return 0;
    }

    return fail("invalid", "Unknown command.");
}
