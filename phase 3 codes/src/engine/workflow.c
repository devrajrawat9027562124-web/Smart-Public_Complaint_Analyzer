/*
 * workflow.c
 * Complaint workflow rules shared by the console app and the web server.
 * See workflow.h for the contract of each operation.
 */

#include "../../include/engine/workflow.h"
#include "../../include/engine/file_manager.h"
#include "../../include/engine/duplicate_detector.h"
#include "../../include/engine/route_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ─── Helpers ─── *

static WfResult result(int code, const char* fmt, const char* a, int n) {
    WfResult r;
    r.code = code;
    if (a) snprintf(r.message, sizeof(r.message), fmt, a, n);
    else snprintf(r.message, sizeof(r.message), fmt, n);
    return r;
}

static WfResult ok_msg(const char* message) {
    WfResult r;
    r.code = WF_OK;
    snprintf(r.message, sizeof(r.message), "%s", message);
    return r;
}

/* Append " [web]" (or another channel tag) to log messages from other interfaces */
static void log_with_channel(const char* message, const char* channel) {
    char buf[512];
    if (channel && channel[0]) {
        snprintf(buf, sizeof(buf), "%s [%s]", message, channel);
        log_activity(buf);
    } else {
        log_activity(message);
    }
}

static void trim(char* s) {
    size_t len = strlen(s);
    size_t start = 0;
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start > 0) memmove(s, s + start, len - start + 1);
}

static void copy_str(char* dst, size_t cap, const char* src) {
    strncpy(dst, src ? src : "", cap - 1);
    dst[cap - 1] = '\0';
}

/* The acting account must exist and have this role. The website checks
 * roles too; this keeps the rule in one place for every interface. */
static int has_role(const char* username, const char* role) {
    UserRecord u;
    return username && find_user_by_username(username, &u) && strcmp(u.role, role) == 0;
}

/* 1 if a username equal to `name` ignoring case exists, 0 if not, -1 on a
 * read error: "Admin" must not be registrable next to "admin". */
static int username_taken_ci(const char* name) {
    int exact = username_exists(name), n, taken = 0;
    UserRecord* users;
    if (exact != 0) return exact;              /* 1 = taken, -1 = read error */
    users = (UserRecord*)malloc(sizeof(UserRecord) * MAX_USERS);
    if (!users) return -1;
    n = load_users(users, MAX_USERS);
    for (int i = 0; i < n && !taken; i++) {
        const char *a = users[i].username, *b = name;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
        taken = (*a == '\0' && *b == '\0');
    }
    free(users);
    return taken;
}

int wf_status_rank(const char* status) {
    static const char* order[] = {"PENDING", "REVIEWED", "ASSIGNED", "IN_PROGRESS", "COMPLETED"};
    for (int i = 0; i < 5; i++) {
        if (strcmp(status, order[i]) == 0) return i;
    }
    return -1;
}

const char* wf_severity_label(int severity) {
    if (severity >= 9) return "CRITICAL";
    if (severity >= 7) return "HIGH";
    if (severity >= 5) return "MEDIUM";
    if (severity >= 3) return "LOW";
    return "MINOR";
}

/* Only the five departments: "General" (what the NLP suggests when it finds
 * no category) used to be accepted, but no department handles it, so such
 * a complaint (#502 in the data) could never be assigned or resolved. */
int wf_valid_category(const char* category) {
    for (int i = 0; i < NLP_NUM_CATEGORIES; i++) {
        if (strcmp(category, NLP_CATEGORIES[i]) == 0) return 1;
    }
    return 0;
}

/* ─── Submission ─── */

void wf_analyze_draft(NLPAnalyzer* nlp, Complaint* draft, char* suggested) {
    sanitize_field(draft->title);
    sanitize_field(draft->description);
    trim(draft->title);
    trim(draft->description);

    draft->is_irrelevant = detect_irrelevant(draft->title, draft->description);
    draft->severity = analyze_severity(nlp, draft->title, draft->description);
    draft->nlp_score = draft->severity;
    if (suggested) suggest_category(nlp, draft->title, draft->description, suggested);
}

static WfResult finalize_submission_impl(Complaint* draft, const char* username, const char* channel,
                                double* dup_similarity, double* dup_threshold) {
    CityGraph graph;
    char log_msg[256];

    if (dup_similarity) *dup_similarity = 0.0;
    if (dup_threshold) *dup_threshold = 0.0;

    sanitize_field(draft->title);
    sanitize_field(draft->description);
    trim(draft->title);
    trim(draft->description);
    if (draft->title[0] == '\0') return result(WF_INVALID, "Title cannot be empty.%s", "", 0);
    if (draft->description[0] == '\0') return result(WF_INVALID, "Description cannot be empty.%s", "", 0);
    if (draft->category[0] == '\0' || strcmp(draft->category, "General") == 0)
        return result(WF_INVALID, "Choose a category: Water Supply, Electricity, Roads & Transport, "
                                  "Sanitation or Public Safety.%s", "", 0);
    if (!wf_valid_category(draft->category))
        return result(WF_INVALID, "Unknown category '%s'.", draft->category, 0);

    init_city_graph(&graph);
    if (draft->location_id < 0 || draft->location_id >= graph.num_locations)
        return result(WF_INVALID, "Location ID must be between 0 and %d.", NULL, graph.num_locations - 1);
    copy_str(draft->location, sizeof(draft->location), get_location_name(&graph, draft->location_id));
    copy_str(draft->department, sizeof(draft->department), draft->category);

    /* The submission date is set first: the duplicate check uses it to skip
     * complaints that were already completed before this one was made */
    get_current_datetime(draft->submitted_date, MAX_DATE_LEN);

    /* Duplicate check against every stored complaint (heap, not stack).
     * Spam is not checked, as in reanalyze: "spam + duplicate of #1" made
     * the two disagree. */
    draft->is_duplicate = 0;
    draft->duplicate_of = -1;
    if (!draft->is_irrelevant) {
        Complaint* existing = (Complaint*)malloc(sizeof(Complaint) * MAX_COMPLAINTS);
        if (!existing) return result(WF_IO_ERROR, "Out of memory.%s", "", 0);
        int existing_count = load_complaints(existing, MAX_COMPLAINTS);
        draft->id = 0;  /* not yet assigned: never equal to a stored ID */
        check_duplicate_ex(draft, existing, existing_count, dup_similarity, dup_threshold);
        free(existing);
    }

    draft->id = get_next_complaint_id();
    if (draft->id <= 0)
        return result(WF_IO_ERROR, "Could not read complaints.txt to number the complaint.%s", "", 0);
    strcpy(draft->status, "PENDING");
    draft->worker_id = -1;
    strcpy(draft->completed_date, "N/A");

    if (!save_complaint(draft))
        return result(WF_IO_ERROR, "Failed to save complaint #%d.", NULL, draft->id);

    snprintf(log_msg, sizeof(log_msg), "User '%s' submitted complaint #%d: %s",
             username ? username : "?", draft->id, draft->title);
    log_with_channel(log_msg, channel);
    refresh_department_counts();

    return result(WF_OK, "Complaint #%d submitted successfully.", NULL, draft->id);
}

/* ─── Admin operations ─── */

static WfResult review_impl(int complaint_id, const char* admin_username, const char* channel) {
    Complaint c;
    char log_msg[256];
    if (!has_role(admin_username, "ADMIN"))
        return result(WF_FORBIDDEN, "'%s' is not an administrator.", admin_username ? admin_username : "?", 0);
    if (!find_complaint(complaint_id, &c))
        return result(WF_NOT_FOUND, "Complaint #%d not found.", NULL, complaint_id);
    if (strcmp(c.status, "PENDING") != 0)
        return result(WF_INVALID, "Only PENDING complaints can be reviewed (complaint is %s).", c.status, 0);

    strcpy(c.status, "REVIEWED");
    if (!update_complaint(&c))
        return result(WF_IO_ERROR, "Could not update complaint #%d.", NULL, complaint_id);

    snprintf(log_msg, sizeof(log_msg), "Admin '%s' reviewed complaint #%d",
             admin_username, complaint_id);
    log_with_channel(log_msg, channel);
    return result(WF_OK, "Complaint #%d marked as REVIEWED.", NULL, complaint_id);
}

static WfResult escalate_impl(int complaint_id, int new_severity, const char* admin_username,
                     const char* channel) {
    Complaint c;
    char log_msg[256];
    if (!has_role(admin_username, "ADMIN"))
        return result(WF_FORBIDDEN, "'%s' is not an administrator.", admin_username ? admin_username : "?", 0);
    if (new_severity < MIN_SEVERITY || new_severity > MAX_SEVERITY)
        return result(WF_INVALID, "Severity must be between 1 and %d.", NULL, MAX_SEVERITY);
    if (!find_complaint(complaint_id, &c))
        return result(WF_NOT_FOUND, "Complaint #%d not found.", NULL, complaint_id);
    if (strcmp(c.status, "COMPLETED") == 0)
        return result(WF_INVALID, "Complaint #%d is already completed.", NULL, complaint_id);

    c.severity = new_severity;
    if (!update_complaint(&c))
        return result(WF_IO_ERROR, "Could not update complaint #%d.", NULL, complaint_id);

    snprintf(log_msg, sizeof(log_msg), "Admin '%s' changed severity of complaint #%d to %d",
             admin_username, complaint_id, new_severity);
    log_with_channel(log_msg, channel);
    return result(WF_OK, "Severity updated to %d.", NULL, new_severity);
}

static WfResult assign_department_impl(int complaint_id, int department_id, const char* admin_username,
                              const char* channel) {
    Complaint c;
    Department depts[MAX_DEPARTMENTS];
    const Department* dept = NULL;
    char log_msg[256];
    int count;
    if (!has_role(admin_username, "ADMIN"))
        return result(WF_FORBIDDEN, "'%s' is not an administrator.", admin_username ? admin_username : "?", 0);
    count = load_departments(depts, MAX_DEPARTMENTS);

    for (int i = 0; i < count; i++) {
        if (depts[i].id == department_id) { dept = &depts[i]; break; }
    }
    if (!dept) return result(WF_NOT_FOUND, "Department %d not found.", NULL, department_id);
    if (!find_complaint(complaint_id, &c))
        return result(WF_NOT_FOUND, "Complaint #%d not found.", NULL, complaint_id);
    if (strcmp(c.status, "COMPLETED") == 0)
        return result(WF_INVALID, "Complaint #%d is already completed.", NULL, complaint_id);

    if (strcmp(c.department, dept->name) != 0) {
        /* A worker from the previous department cannot keep the task */
        c.worker_id = -1;
        if (wf_status_rank(c.status) > 2) strcpy(c.status, "ASSIGNED");
    }
    copy_str(c.department, sizeof(c.department), dept->name);
    copy_str(c.category, sizeof(c.category), dept->name);
    if (wf_status_rank(c.status) < 2) strcpy(c.status, "ASSIGNED");

    if (!update_complaint(&c))
        return result(WF_IO_ERROR, "Could not update complaint #%d.", NULL, complaint_id);
    refresh_department_counts();

    snprintf(log_msg, sizeof(log_msg), "Admin '%s' assigned complaint #%d to department '%s'",
             admin_username, complaint_id, dept->name);
    log_with_channel(log_msg, channel);
    return result(WF_OK, "Complaint assigned to %s.", dept->name, 0);
}

/* ─── Department officer ─── */

static WfResult assign_worker_impl(int complaint_id, int worker_id, const char* officer_username,
                          const char* channel) {
    UserRecord officer, worker;
    Complaint c;
    char log_msg[256];

    if (!find_user_by_username(officer_username, &officer) ||
        strcmp(officer.role, "DEPT_OFFICER") != 0)
        return result(WF_FORBIDDEN, "'%s' is not a department officer.", officer_username, 0);
    if (!find_complaint(complaint_id, &c))
        return result(WF_NOT_FOUND, "Complaint #%d not found.", NULL, complaint_id);
    if (strcmp(c.department, officer.department) != 0)
        return result(WF_FORBIDDEN, "Complaint #%d is not in your department.", NULL, complaint_id);
    /* Only after the admin has routed it to the department (ASSIGNED):
     * officers used to be able to pick up PENDING complaints and skip review */
    if (strcmp(c.status, "ASSIGNED") != 0)
        return result(WF_INVALID, "This complaint is %s: an admin must route complaint #%d to the department first.",
                      c.status, complaint_id);
    if (c.worker_id > 0)
        return result(WF_INVALID, "Complaint #%d already has a worker.", NULL, complaint_id);
    if (!find_user_by_id(worker_id, &worker) || strcmp(worker.role, "WORKER") != 0)
        return result(WF_NOT_FOUND, "Worker %d not found.", NULL, worker_id);
    if (strcmp(worker.department, officer.department) != 0)
        return result(WF_FORBIDDEN, "%s is not in your department.", worker.full_name, 0);
    if (!worker.is_available)
        return result(WF_INVALID, "%s is not available.", worker.full_name, 0);

    c.worker_id = worker_id;
    strcpy(c.status, "ASSIGNED");
    if (!update_complaint(&c))
        return result(WF_IO_ERROR, "Could not update complaint #%d.", NULL, complaint_id);

    snprintf(log_msg, sizeof(log_msg), "Officer '%s' assigned worker '%s' (ID:%d) to complaint #%d",
             officer_username, worker.username, worker_id, complaint_id);
    log_with_channel(log_msg, channel);
    return result(WF_OK, "Worker %s assigned.", worker.full_name, 0);
}

/* ─── Worker ─── */

static WfResult load_own_task(int complaint_id, int worker_id, Complaint* c, UserRecord* worker) {
    if (!find_user_by_id(worker_id, worker) || strcmp(worker->role, "WORKER") != 0)
        return result(WF_NOT_FOUND, "Worker %d not found.", NULL, worker_id);
    if (!find_complaint(complaint_id, c))
        return result(WF_NOT_FOUND, "Complaint #%d not found.", NULL, complaint_id);
    if (c->worker_id != worker_id)
        return result(WF_FORBIDDEN, "Complaint #%d is not assigned to you.", NULL, complaint_id);
    if (strcmp(c->status, "COMPLETED") == 0)
        return result(WF_INVALID, "Complaint #%d is already completed.", NULL, complaint_id);
    return ok_msg("");
}

static WfResult start_task_impl(int complaint_id, int worker_id, const char* channel, RouteResult* route) {
    Complaint c;
    UserRecord worker;
    CityGraph graph;
    RouteResult r;
    char log_msg[256];
    WfResult check = load_own_task(complaint_id, worker_id, &c, &worker);
    if (check.code != WF_OK) return check;

    init_city_graph(&graph);
    memset(&r, 0, sizeof(r));
    if (!dijkstra(&graph, worker.current_location_id, c.location_id, &r))
        return result(WF_INVALID, "No route found to complaint #%d.", NULL, complaint_id);
    if (route) *route = r;

    /* Already started: just give the route again, without a second
     * "started" entry in the activity log */
    if (strcmp(c.status, "IN_PROGRESS") == 0)
        return ok_msg("Task already in progress - route recalculated from your current location.");

    strcpy(c.status, "IN_PROGRESS");
    if (!update_complaint(&c))
        return result(WF_IO_ERROR, "Could not update complaint #%d.", NULL, complaint_id);

    snprintf(log_msg, sizeof(log_msg), "Worker '%s' started route to complaint #%d (distance: %d km)",
             worker.username, complaint_id, r.total_distance);
    log_with_channel(log_msg, channel);
    return ok_msg("Task marked as IN_PROGRESS.");
}

static WfResult complete_task_impl(int complaint_id, int worker_id, const char* channel) {
    Complaint c;
    UserRecord worker;
    char log_msg[256];
    WfResult check = load_own_task(complaint_id, worker_id, &c, &worker);
    if (check.code != WF_OK) return check;
    /* ASSIGNED -> IN_PROGRESS -> COMPLETED: completing an unstarted task
     * used to skip the IN_PROGRESS step of the workflow */
    if (strcmp(c.status, "IN_PROGRESS") != 0)
        return result(WF_INVALID, "Start task #%d (get its route) before marking it complete.", NULL, complaint_id);

    strcpy(c.status, "COMPLETED");
    get_current_datetime(c.completed_date, MAX_DATE_LEN);
    if (!update_complaint(&c))
        return result(WF_IO_ERROR, "Could not update complaint #%d.", NULL, complaint_id);
    refresh_department_counts();

    snprintf(log_msg, sizeof(log_msg), "Worker '%s' completed complaint #%d: %s",
             worker.username, complaint_id, c.title);
    log_with_channel(log_msg, channel);
    return result(WF_OK, "Task #%d marked as COMPLETED!", NULL, complaint_id);
}

static WfResult set_location_impl(int user_id, int location_id, const char* channel) {
    UserRecord u;
    CityGraph graph;
    char log_msg[256];
    init_city_graph(&graph);
    if (location_id < 0 || location_id >= graph.num_locations)
        return result(WF_INVALID, "Location ID must be between 0 and %d.", NULL, graph.num_locations - 1);
    if (!find_user_by_id(user_id, &u))
        return result(WF_NOT_FOUND, "User %d not found.", NULL, user_id);

    u.current_location_id = location_id;
    if (!update_user(&u))
        return result(WF_IO_ERROR, "Could not update user %d.", NULL, user_id);

    snprintf(log_msg, sizeof(log_msg), "Worker '%s' updated location to '%s'",
             u.username, get_location_name(&graph, location_id));
    log_with_channel(log_msg, channel);
    return result(WF_OK, "Location updated to %s.", get_location_name(&graph, location_id), 0);
}

static WfResult set_availability_impl(int user_id, int available, const char* channel) {
    UserRecord u;
    char log_msg[256];
    if (!find_user_by_id(user_id, &u) || strcmp(u.role, "WORKER") != 0)
        return result(WF_NOT_FOUND, "Worker %d not found.", NULL, user_id);

    u.is_available = available ? 1 : 0;
    if (!update_user(&u))
        return result(WF_IO_ERROR, "Could not update user %d.", NULL, user_id);

    snprintf(log_msg, sizeof(log_msg), "Worker '%s' is now %s", u.username,
             available ? "available for new tasks" : "busy (not taking new tasks)");
    log_with_channel(log_msg, channel);
    return ok_msg(available ? "You are available: officers can assign you new tasks."
                            : "You are marked busy: officers cannot assign you new tasks until you switch back.");
}

/* ─── Registration ─── */

static int valid_username(const char* s) {
    size_t len = strlen(s);
    if (len < 3 || len > 30) return 0;
    for (; *s; s++) {
        if (!isalnum((unsigned char)*s) && *s != '_' && *s != '.' && *s != '-') return 0;
    }
    return 1;
}

static WfResult register_user_impl(const char* username, const char* password, const char* full_name,
                          const char* phone, const char* email, const char* channel,
                          UserRecord* out) {
    UserRecord record;
    char name[MAX_NAME_LEN], ph[MAX_PHONE_LEN], em[MAX_EMAIL_LEN];
    char log_msg[256];

    if (!username || !valid_username(username))
        return result(WF_INVALID, "Username must be 3-30 characters: letters, digits, '_', '.' or '-'.%s", "", 0);
    int taken = username_taken_ci(username);
    if (taken < 0) return result(WF_IO_ERROR, "Could not read the accounts. Try again.%s", "", 0);
    if (taken) return result(WF_INVALID, "Username '%s' is already taken.", username, 0);

    size_t plen = password ? strlen(password) : 0;
    if (plen < 4 || plen >= MAX_PASSWORD_LEN)
        return result(WF_INVALID, "Password must be 4-%d characters.", NULL, MAX_PASSWORD_LEN - 1);
    for (const char* p = password; *p; p++) {
        if (isspace((unsigned char)*p) || *p == '|')
            return result(WF_INVALID, "Password cannot contain spaces or '|'.%s", "", 0);
    }

    copy_str(name, sizeof(name), full_name);
    sanitize_field(name);
    trim(name);
    if (name[0] == '\0') return result(WF_INVALID, "Full name cannot be empty.%s", "", 0);

    copy_str(ph, sizeof(ph), phone);
    trim(ph);
    if (ph[0]) {
        size_t digits = 0;
        for (const char* p = ph; *p; p++) {
            if (!isdigit((unsigned char)*p))
                return result(WF_INVALID, "Phone number must contain digits only.%s", "", 0);
            digits++;
        }
        if (digits < 10 || digits > 15)
            return result(WF_INVALID, "Phone number must have 10-15 digits.%s", "", 0);
    }

    copy_str(em, sizeof(em), email);
    sanitize_field(em);
    trim(em);
    if (em[0]) {
        const char* at = strchr(em, '@');
        if (!at || at == em || !strchr(at, '.') || strchr(em, ' '))
            return result(WF_INVALID, "Email address looks invalid.%s", "", 0);
    }

    memset(&record, 0, sizeof(record));
    record.id = get_next_user_id();
    if (record.id <= 0) return result(WF_IO_ERROR, "Could not read the accounts. Try again.%s", "", 0);
    copy_str(record.username, sizeof(record.username), username);
    copy_str(record.password, sizeof(record.password), password);
    copy_str(record.full_name, sizeof(record.full_name), name);
    copy_str(record.phone, sizeof(record.phone), ph);
    copy_str(record.email, sizeof(record.email), em);
    strcpy(record.role, "USER");
    get_current_datetime(record.registered_date, sizeof(record.registered_date));
    strcpy(record.department, "General");
    record.current_location_id = 0;
    record.is_available = 1;

    if (!save_user(&record)) return result(WF_IO_ERROR, "Could not save the account.%s", "", 0);
    if (out) *out = record;

    snprintf(log_msg, sizeof(log_msg), "New user registered: '%s' (%s)", record.username, record.full_name);
    log_with_channel(log_msg, channel);
    return result(WF_OK, "Account '%s' created.", record.username, 0);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Public operations: each runs under the cross-process data lock, so a
 *  check ("is it PENDING?", "next ID is 503") still holds when the change
 *  is written, even if the console app and the website act at once.
 * ═══════════════════════════════════════════════════════════════════ */

static WfResult busy(void) {
    return result(WF_IO_ERROR, "The data files are busy (another change is in progress). Try again.%s", "", 0);
}

#define LOCKED(call) do { WfResult r_; if (!fm_lock()) return busy(); r_ = (call); fm_unlock(); return r_; } while (0)

WfResult wf_finalize_submission(Complaint* draft, const char* username, const char* channel,
                                double* dup_similarity, double* dup_threshold) {
    LOCKED(finalize_submission_impl(draft, username, channel, dup_similarity, dup_threshold));
}

WfResult wf_review(int complaint_id, const char* admin_username, const char* channel) {
    LOCKED(review_impl(complaint_id, admin_username, channel));
}

WfResult wf_escalate(int complaint_id, int new_severity, const char* admin_username, const char* channel) {
    LOCKED(escalate_impl(complaint_id, new_severity, admin_username, channel));
}

WfResult wf_assign_department(int complaint_id, int department_id, const char* admin_username,
                              const char* channel) {
    LOCKED(assign_department_impl(complaint_id, department_id, admin_username, channel));
}

WfResult wf_assign_worker(int complaint_id, int worker_id, const char* officer_username, const char* channel) {
    LOCKED(assign_worker_impl(complaint_id, worker_id, officer_username, channel));
}

WfResult wf_start_task(int complaint_id, int worker_id, const char* channel, RouteResult* route) {
    LOCKED(start_task_impl(complaint_id, worker_id, channel, route));
}

WfResult wf_complete_task(int complaint_id, int worker_id, const char* channel) {
    LOCKED(complete_task_impl(complaint_id, worker_id, channel));
}

WfResult wf_set_location(int user_id, int location_id, const char* channel) {
    LOCKED(set_location_impl(user_id, location_id, channel));
}

WfResult wf_set_availability(int user_id, int available, const char* channel) {
    LOCKED(set_availability_impl(user_id, available, channel));
}

WfResult wf_register_user(const char* username, const char* password, const char* full_name,
                          const char* phone, const char* email, const char* channel,
                          UserRecord* out) {
    LOCKED(register_user_impl(username, password, full_name, phone, email, channel, out));
}
