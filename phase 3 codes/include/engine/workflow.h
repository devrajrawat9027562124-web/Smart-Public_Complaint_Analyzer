/*
 * workflow.h
 * Complaint workflow rules shared by every interface (the C++ console app
 * and the web server's engine_cli). Each operation validates its input,
 * updates the data files, writes the activity log and keeps department
 * counters in sync, so both interfaces behave identically.
 */

#ifndef WORKFLOW_H
#define WORKFLOW_H

#include "complaint_types.h"
#include "nlp_analyzer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Result codes */
#define WF_OK         0
#define WF_NOT_FOUND  1
#define WF_FORBIDDEN  2
#define WF_INVALID    3
#define WF_IO_ERROR   4

typedef struct {
    int code;            /* WF_OK or an error code */
    char message[200];   /* human-readable outcome */
} WfResult;

/* Status order: PENDING=0, REVIEWED=1, ASSIGNED=2, IN_PROGRESS=3, COMPLETED=4, unknown=-1 */
int wf_status_rank(const char* status);

/* "CRITICAL", "HIGH", "MEDIUM", "LOW" or "MINOR" (same bands as the console) */
const char* wf_severity_label(int severity);

/* 1 if `category` is one of the five departments ("General" is not:
 * no department handles it) */
int wf_valid_category(const char* category);

/* Step 1 of a submission: run the NLP on draft->title/description.
 * Sets severity, nlp_score and is_irrelevant, and writes the suggested
 * category to `suggested` (MAX_CATEGORY_LEN bytes). Nothing is saved. */
void wf_analyze_draft(NLPAnalyzer* nlp, Complaint* draft, char* suggested);

/* Step 2 of a submission. `draft` must have user_id, title, description,
 * category (one of the five) and location_id set (and the fields from
 * wf_analyze_draft). Unless it was flagged as spam, checks for duplicates
 * against the stored complaints that were still open, then assigns the ID,
 * location name, department, status and dates, saves and logs.
 * dup_similarity / dup_threshold (may be NULL) receive the duplicate match.
 *
 * Every operation below runs under the cross-process data lock (fm_lock)
 * and checks that the acting account has the role named in its comment. */
WfResult wf_finalize_submission(Complaint* draft, const char* username, const char* channel,
                                double* dup_similarity, double* dup_threshold);

/* ADMIN: PENDING -> REVIEWED */
WfResult wf_review(int complaint_id, const char* admin_username, const char* channel);

/* ADMIN: set the final severity (1-10) of a complaint that is not completed */
WfResult wf_escalate(int complaint_id, int new_severity, const char* admin_username,
                     const char* channel);

/* ADMIN: route a complaint to a department (sets department and category).
 * PENDING/REVIEWED -> ASSIGNED. Changing department clears the worker. */
WfResult wf_assign_department(int complaint_id, int department_id, const char* admin_username,
                              const char* channel);

/* DEPT_OFFICER: assign an available worker of the officer's own department
 * to a complaint of that department that an admin has routed there
 * (status ASSIGNED) and that has no worker yet. */
WfResult wf_assign_worker(int complaint_id, int worker_id, const char* officer_username,
                          const char* channel);

/* WORKER: start an assigned task (-> IN_PROGRESS); `route` (may be NULL)
 * receives the Dijkstra route from the worker's location to the complaint.
 * On a task already in progress it only recalculates the route. */
WfResult wf_start_task(int complaint_id, int worker_id, const char* channel, RouteResult* route);

/* WORKER: complete a started task (IN_PROGRESS -> COMPLETED, completion date = now) */
WfResult wf_complete_task(int complaint_id, int worker_id, const char* channel);

/* Update a user's current location (used by workers) */
WfResult wf_set_location(int user_id, int location_id, const char* channel);

/* WORKER: switch between available (officers may assign new tasks) and
 * busy (they may not). Tasks already assigned are not affected. */
WfResult wf_set_availability(int user_id, int available, const char* channel);

/* Register a citizen (role USER). Rules: username 3-30 chars [A-Za-z0-9_.-]
 * and unique; password 4-49 chars without spaces or '|'; full name required;
 * phone optional (10-15 digits); email optional (must look like an address).
 * On success `out` (may be NULL) receives the saved record. */
WfResult wf_register_user(const char* username, const char* password, const char* full_name,
                          const char* phone, const char* email, const char* channel,
                          UserRecord* out);

#ifdef __cplusplus
}
#endif

#endif /* WORKFLOW_H */
