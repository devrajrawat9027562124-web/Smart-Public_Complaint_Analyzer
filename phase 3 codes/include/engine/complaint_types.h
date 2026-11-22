/*
 * complaint_types.h
 * Core data structures and constants for the Smart Public Complaint Analyzer
 * All structs, typedefs, and macro constants are defined here.
 */

#ifndef COMPLAINT_TYPES_H
#define COMPLAINT_TYPES_H

/* ─── Configuration Constants ─── */

#define MAX_TITLE_LEN       100
#define MAX_DESC_LEN        500
#define MAX_CATEGORY_LEN    50
#define MAX_LOCATION_LEN    100
#define MAX_STATUS_LEN      20
#define MAX_DEPT_LEN        50
#define MAX_NAME_LEN        100
#define MAX_USERNAME_LEN    50
#define MAX_PASSWORD_LEN    50
#define MAX_PHONE_LEN       20
#define MAX_EMAIL_LEN       100
#define MAX_DATE_LEN        20

#define MAX_COMPLAINTS      2000
#define MAX_USERS           200
#define MAX_DEPARTMENTS     10
#define MAX_WORKERS         50
#define MAX_LOCATIONS       20
#define MAX_PATH_LEN        256

#define INF                 999999

/* NLP Constants */
#define MAX_TOKENS          200
#define MAX_TOKEN_LEN       50
#define HT_DEFAULT_SIZE     127
#define AMPLIFIER_MULT      1.5
#define NEGATION_MULT       0.3
#define MAX_SEVERITY        10
#define MIN_SEVERITY        1

/* Duplicate Detection Constants */
#define DUPLICATE_THRESHOLD 0.75
#define JACCARD_WEIGHT      0.4
#define LEVENSHTEIN_WEIGHT  0.3
#define CATEGORY_WEIGHT     0.2
#define LOCATION_WEIGHT     0.1

/* File Paths */
#define FILE_USERS          "data/users.txt"
#define FILE_COMPLAINTS     "data/complaints.txt"
#define FILE_DEPARTMENTS    "data/departments.txt"
#define FILE_WORKERS        "data/workers.txt"
#define FILE_LOGS           "data/logs.txt"
#define FILE_LOCK           "data/.lock"     /* held while any process changes data/ */

/* ─── Structs ─── */

/* Complaint: stores all data for a single complaint */
typedef struct {
    int id;
    int user_id;
    char title[MAX_TITLE_LEN];
    char description[MAX_DESC_LEN];
    char category[MAX_CATEGORY_LEN];
    char location[MAX_LOCATION_LEN];
    int location_id;
    int severity;           /* Final severity (1-10), may be admin-overridden */
    int nlp_score;          /* Original NLP-computed severity */
    int is_duplicate;       /* 0 = not duplicate, 1 = duplicate */
    int duplicate_of;       /* ID of original if duplicate, -1 otherwise */
    int is_irrelevant;      /* 0 = relevant, 1 = irrelevant */
    char status[MAX_STATUS_LEN];
    char department[MAX_DEPT_LEN];
    int worker_id;          /* -1 if unassigned */
    char submitted_date[MAX_DATE_LEN];
    char completed_date[MAX_DATE_LEN];
} Complaint;

/* UserRecord: stores user/person information */
typedef struct {
    int id;
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char full_name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN];
    char email[MAX_EMAIL_LEN];
    char role[MAX_STATUS_LEN];          /* USER, ADMIN, WORKER, DEPT_OFFICER */
    char registered_date[MAX_DATE_LEN];
    char department[MAX_DEPT_LEN];
    int current_location_id;
    int is_available;                   /* 1 = available, 0 = busy */
} UserRecord;

/* Location: a single graph node (city location) */
typedef struct {
    int id;
    char name[MAX_LOCATION_LEN];
} Location;

/* CityGraph: the entire city as a weighted graph */
typedef struct {
    int adj_matrix[MAX_LOCATIONS][MAX_LOCATIONS];
    Location locations[MAX_LOCATIONS];
    int num_locations;
} CityGraph;

/* RouteResult: output of Dijkstra's algorithm */
typedef struct {
    int path[MAX_LOCATIONS];
    int path_length;
    int total_distance;
} RouteResult;

/* Department: government department info */
typedef struct {
    int id;
    char name[MAX_DEPT_LEN];
    char officer_username[MAX_USERNAME_LEN];
    int complaint_count;
    int resolved_count;
} Department;

/* ComplaintStats: aggregate statistics */
typedef struct {
    int total;
    int pending;
    int reviewed;
    int assigned;
    int in_progress;
    int completed;
} ComplaintStats;

#endif /* COMPLAINT_TYPES_H */
