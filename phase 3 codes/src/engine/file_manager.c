/*
 * file_manager.c
 * File I/O for persistent storage using pipe-delimited .txt files.
 * Handles CRUD for users, complaints, departments, and activity logging.
 *
 * Robustness rules (shared with web/server.py, which reads the same files):
 *   - A UTF-8 byte-order mark at the start of a file is ignored.
 *   - Fields are split on '|' and may be empty (sscanf's %[^|] cannot read
 *     empty fields and has no length limit, so it is no longer used).
 *   - Text written to a file has '|', CR, LF and TAB replaced, so one record
 *     always stays on one line with a fixed number of fields.
 *   - Updates stream the file into a temporary copy and then replace the
 *     original, so records the parser cannot read are preserved, and no
 *     2,000-record arrays are placed on the stack.
 *   - The console app and every engine_cli process (one per web request)
 *     share these files, so every change holds a lock on data/.lock for
 *     the whole read-modify-write (fm_lock), and a file that cannot be
 *     opened for a moment (Windows: another process is replacing it) is
 *     retried, never mistaken for a missing or empty file.
 */

#include "../../include/engine/file_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/file.h>
#endif

#define LINE_BUF        4096
#define MAX_FIELDS      64
#define COMPLAINT_FIELDS 17

/* ═══════════════════════════════════════════════════════════════════
 *  Small helpers
 * ═══════════════════════════════════════════════════════════════════ */

/* Get current date/time as "YYYY-MM-DD HH:MM" */
void get_current_datetime(char* buffer, int size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M", t);
}

/* Remove a UTF-8 BOM and the trailing newline from a line read by fgets */
static void strip_line(char* line) {
    if ((unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB &&
        (unsigned char)line[2] == 0xBF) {
        memmove(line, line + 3, strlen(line + 3) + 1);
    }
    line[strcspn(line, "\r\n")] = '\0';
}

/* Split a line in place on '|'. Empty fields are kept. */
static int split_fields(char* line, char* fields[], int max_fields) {
    int n = 0;
    char* p = line;
    fields[n++] = p;
    while (*p) {
        if (*p == '|') {
            *p = '\0';
            if (n < max_fields) fields[n++] = p + 1;
        }
        p++;
    }
    return n;
}

static void copy_str(char* dst, size_t cap, const char* src) {
    if (cap == 0) return;
    strncpy(dst, src ? src : "", cap - 1);
    dst[cap - 1] = '\0';
}

/* A number that does not fit in an int (e.g. an ID of 99999999999) gives
 * fallback: it used to be clamped to INT_MAX, and the next ID then
 * overflowed to -2147483648. */
static int to_int(const char* s, int fallback) {
    char* end;
    long v;
    if (!s) return fallback;
    while (*s == ' ') s++;
    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s || errno == ERANGE || v > INT_MAX || v < INT_MIN) return fallback;
    return (int)v;
}

/* Replace characters that would break the one-record-per-line format */
void sanitize_field(char* s) {
    for (; *s; s++) {
        if (*s == '|') *s = '/';
        else if (*s == '\r' || *s == '\n' || *s == '\t') *s = ' ';
    }
}

static void clean_copy(char* dst, size_t cap, const char* src) {
    copy_str(dst, cap, src);
    sanitize_field(dst);
}

/* Leading integer of a record line (its ID), or -1 */
static int leading_id(const char* line) {
    const char* p = line;
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB &&
        (unsigned char)p[2] == 0xBF) p += 3;
    return to_int(p, -1);
}

static FILE* open_data(const char* path, const char* mode);

/* Make sure appending to a file starts on a new line */
static void ensure_trailing_newline(const char* path) {
    FILE* f = open_data(path, "rb");
    int need = 0;
    if (!f) return;
    if (fseek(f, 0, SEEK_END) == 0 && ftell(f) > 0) {
        fseek(f, -1, SEEK_END);
        need = (fgetc(f) != '\n');
    }
    fclose(f);
    if (need) {
        f = open_data(path, "a");
        if (f) { fputc('\n', f); fclose(f); }
    }
}

static void short_sleep(void) {
#ifdef _WIN32
    Sleep(25);
#else
    usleep(25000);
#endif
}

/* fopen for a data file. While another process replaces a file, Windows
 * refuses to open it for a moment (sharing violation, errno EACCES). That
 * used to be treated like a missing file: users.txt was then re-seeded with
 * the default accounts, a complaint search found nothing, and the next ID
 * restarted at 1. Retry for up to 2 s; only ENOENT means "missing". */
static FILE* open_data(const char* path, const char* mode) {
    for (int attempt = 0; attempt < 80; attempt++) {
        FILE* f = fopen(path, mode);
        if (f) return f;
        if (errno == ENOENT) return NULL;
        short_sleep();
    }
    return NULL;
}

/* Replace dst with tmp. Retries in case another process (the console app
 * or the web server) is reading the file at that moment. */
static int replace_file(const char* tmp, const char* dst) {
    for (int attempt = 0; attempt < 200; attempt++) {
#ifdef _WIN32
        if (MoveFileExA(tmp, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return 1;
#else
        if (rename(tmp, dst) == 0) return 1;
#endif
        short_sleep();
    }
    remove(tmp);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Record <-> line conversion
 * ═══════════════════════════════════════════════════════════════════ */

static int parse_user(char* line, UserRecord* u) {
    char* f[MAX_FIELDS];
    int n;
    strip_line(line);
    if (line[0] == '\0') return 0;
    n = split_fields(line, f, MAX_FIELDS);
    if (n < 9) return 0;

    memset(u, 0, sizeof(*u));
    u->id = to_int(f[0], -1);
    if (u->id < 0) return 0;
    copy_str(u->username, sizeof(u->username), f[1]);
    copy_str(u->password, sizeof(u->password), f[2]);
    copy_str(u->full_name, sizeof(u->full_name), f[3]);
    copy_str(u->phone, sizeof(u->phone), f[4]);
    copy_str(u->email, sizeof(u->email), f[5]);
    copy_str(u->role, sizeof(u->role), f[6]);
    copy_str(u->registered_date, sizeof(u->registered_date), f[7]);
    copy_str(u->department, sizeof(u->department), f[8]);
    u->current_location_id = (n > 9) ? to_int(f[9], 0) : 0;
    u->is_available = (n > 10) ? to_int(f[10], 1) : 1;
    return u->username[0] != '\0';
}

static void write_user(FILE* f, const UserRecord* u) {
    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN], name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN], email[MAX_EMAIL_LEN], role[MAX_STATUS_LEN];
    char reg[MAX_DATE_LEN], dept[MAX_DEPT_LEN];
    clean_copy(username, sizeof(username), u->username);
    clean_copy(password, sizeof(password), u->password);
    clean_copy(name, sizeof(name), u->full_name);
    clean_copy(phone, sizeof(phone), u->phone);
    clean_copy(email, sizeof(email), u->email);
    clean_copy(role, sizeof(role), u->role);
    clean_copy(reg, sizeof(reg), u->registered_date);
    clean_copy(dept, sizeof(dept), u->department);
    fprintf(f, "%d|%s|%s|%s|%s|%s|%s|%s|%s|%d|%d\n",
            u->id, username, password, name, phone, email, role, reg, dept,
            u->current_location_id, u->is_available);
}

static int parse_complaint(char* line, Complaint* c) {
    char* f[MAX_FIELDS];
    char merged_desc[MAX_DESC_LEN];
    int n, extra;
    strip_line(line);
    if (line[0] == '\0') return 0;
    n = split_fields(line, f, MAX_FIELDS);
    if (n < 15) return 0;

    /* A legacy description that contained '|' produces extra fields:
     * merge them back into the description so the columns line up. */
    extra = (n > COMPLAINT_FIELDS) ? n - COMPLAINT_FIELDS : 0;
    merged_desc[0] = '\0';
    for (int i = 3; i <= 3 + extra; i++) {
        if (i > 3) strncat(merged_desc, " / ", sizeof(merged_desc) - strlen(merged_desc) - 1);
        strncat(merged_desc, f[i], sizeof(merged_desc) - strlen(merged_desc) - 1);
    }
    if (extra > 0) {
        for (int i = 4; i + extra < n; i++) f[i] = f[i + extra];
        n -= extra;
    }

    memset(c, 0, sizeof(*c));
    c->id = to_int(f[0], -1);
    if (c->id < 0) return 0;
    c->user_id = to_int(f[1], -1);
    copy_str(c->title, sizeof(c->title), f[2]);
    copy_str(c->description, sizeof(c->description), merged_desc);
    copy_str(c->category, sizeof(c->category), f[4]);
    copy_str(c->location, sizeof(c->location), f[5]);
    c->location_id = to_int(f[6], 0);
    c->severity = to_int(f[7], 1);
    c->nlp_score = to_int(f[8], c->severity);
    c->is_duplicate = to_int(f[9], 0);
    c->duplicate_of = to_int(f[10], -1);
    c->is_irrelevant = to_int(f[11], 0);
    copy_str(c->status, sizeof(c->status), f[12]);
    copy_str(c->department, sizeof(c->department), f[13]);
    c->worker_id = to_int(f[14], -1);
    copy_str(c->submitted_date, sizeof(c->submitted_date), n > 15 ? f[15] : "");
    copy_str(c->completed_date, sizeof(c->completed_date), n > 16 ? f[16] : "N/A");
    if (c->completed_date[0] == '\0') strcpy(c->completed_date, "N/A");
    return 1;
}

static void write_complaint(FILE* f, const Complaint* c) {
    char title[MAX_TITLE_LEN], desc[MAX_DESC_LEN], cat[MAX_CATEGORY_LEN];
    char loc[MAX_LOCATION_LEN], status[MAX_STATUS_LEN], dept[MAX_DEPT_LEN];
    char sub[MAX_DATE_LEN], done[MAX_DATE_LEN];
    clean_copy(title, sizeof(title), c->title);
    clean_copy(desc, sizeof(desc), c->description);
    clean_copy(cat, sizeof(cat), c->category);
    clean_copy(loc, sizeof(loc), c->location);
    clean_copy(status, sizeof(status), c->status);
    clean_copy(dept, sizeof(dept), c->department);
    clean_copy(sub, sizeof(sub), c->submitted_date);
    clean_copy(done, sizeof(done), c->completed_date[0] ? c->completed_date : "N/A");
    fprintf(f, "%d|%d|%s|%s|%s|%s|%d|%d|%d|%d|%d|%d|%s|%s|%d|%s|%s\n",
            c->id, c->user_id, title, desc, cat, loc, c->location_id,
            c->severity, c->nlp_score, c->is_duplicate, c->duplicate_of,
            c->is_irrelevant, status, dept, c->worker_id, sub, done);
}

static int parse_department(char* line, Department* d) {
    char* f[MAX_FIELDS];
    int n;
    strip_line(line);
    if (line[0] == '\0') return 0;
    n = split_fields(line, f, MAX_FIELDS);
    if (n < 3) return 0;
    memset(d, 0, sizeof(*d));
    d->id = to_int(f[0], -1);
    if (d->id < 0) return 0;
    copy_str(d->name, sizeof(d->name), f[1]);
    copy_str(d->officer_username, sizeof(d->officer_username), f[2]);
    d->complaint_count = (n > 3) ? to_int(f[3], 0) : 0;
    d->resolved_count = (n > 4) ? to_int(f[4], 0) : 0;
    return d->name[0] != '\0';
}

static void write_department(FILE* f, const Department* d) {
    char name[MAX_DEPT_LEN], officer[MAX_USERNAME_LEN];
    clean_copy(name, sizeof(name), d->name);
    clean_copy(officer, sizeof(officer), d->officer_username);
    fprintf(f, "%d|%s|%s|%d|%d\n", d->id, name, officer,
            d->complaint_count, d->resolved_count);
}

/* Stream `path` to a temporary file, replacing the first record whose ID
 * matches `id` by calling `writer(out, record)`. Other lines are copied as-is. */
typedef void (*RecordWriter)(FILE*, const void*);

static int rewrite_record_locked(const char* path, int id, RecordWriter writer, const void* record) {
    char tmp[MAX_PATH_LEN];
    char line[LINE_BUF];
    int found = 0;
    FILE* in = open_data(path, "r");
    FILE* out;
    if (!in) return 0;
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    out = fopen(tmp, "w");
    if (!out) { fclose(in); return 0; }

    while (fgets(line, sizeof(line), in)) {
        strip_line(line);
        if (line[0] == '\0') continue;
        if (!found && leading_id(line) == id) {
            writer(out, record);
            found = 1;
        } else {
            fprintf(out, "%s\n", line);
        }
    }
    fclose(in);
    if (fclose(out) != 0 || !found) {
        remove(tmp);
        return 0;
    }
    return replace_file(tmp, path);
}

static int rewrite_record(const char* path, int id, RecordWriter writer, const void* record) {
    int ok;
    if (!fm_lock()) return 0;
    ok = rewrite_record_locked(path, id, writer, record);
    fm_unlock();
    return ok;
}

static void user_writer(FILE* f, const void* r) { write_user(f, (const UserRecord*)r); }
static void complaint_writer(FILE* f, const void* r) { write_complaint(f, (const Complaint*)r); }
static void department_writer(FILE* f, const void* r) { write_department(f, (const Department*)r); }

/* ═══════════════════════════════════════════════════════════════════
 *  Initialization & Seeding
 * ═══════════════════════════════════════════════════════════════════ */

static void ensure_data_dir(void) {
    struct stat st;
    if (stat("data", &st) == -1) {
#ifdef _WIN32
        _mkdir("data");
#else
        mkdir("data", 0755);
#endif
    }
}

/* ─── Cross-process lock (data/.lock) ───
 * Held for a whole read-modify-write so that two processes (the console
 * app, and the engine_cli started for each web request) cannot interleave:
 * without it two submissions got the same ID, appends were lost when
 * another process replaced the file, and records were overwritten.
 * Re-entrant within a process. The operating system releases the lock if
 * the process ends, so a crash cannot leave it stuck. */
static int lock_depth = 0;
#ifdef _WIN32
static HANDLE lock_handle = INVALID_HANDLE_VALUE;
#else
static int lock_fd = -1;
#endif

int fm_lock(void) {
    if (lock_depth > 0) { lock_depth++; return 1; }
    ensure_data_dir();
#ifdef _WIN32
    /* Share mode READ: a second writer is refused (it asks for write
     * access), while tools that only copy data/ can still read the file. */
    for (int attempt = 0; attempt < 400; attempt++) {           /* up to ~10 s */
        HANDLE h = CreateFileA(FILE_LOCK, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                               NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE) { lock_handle = h; lock_depth = 1; return 1; }
        DWORD err = GetLastError();
        if (err != ERROR_SHARING_VIOLATION && err != ERROR_ACCESS_DENIED) return 0;
        short_sleep();
    }
    return 0;
#else
    int fd = open(FILE_LOCK, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return 0;
    if (flock(fd, LOCK_EX) != 0) { close(fd); return 0; }
    lock_fd = fd;
    lock_depth = 1;
    return 1;
#endif
}

void fm_unlock(void) {
    if (lock_depth == 0 || --lock_depth > 0) return;
#ifdef _WIN32
    CloseHandle(lock_handle);
    lock_handle = INVALID_HANDLE_VALUE;
#else
    flock(lock_fd, LOCK_UN);
    close(lock_fd);
    lock_fd = -1;
#endif
}

/* True only when the file really does not exist (not: could not be opened) */
static int file_missing(const char* path) {
    struct stat st;
    return stat(path, &st) != 0 && errno == ENOENT;
}

/* Create a new file for writing; fails if it already exists, so an
 * existing file can never be truncated (O_EXCL) */
static FILE* create_new_file(const char* path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    FILE* f;
    if (fd < 0) return NULL;
    f = fdopen(fd, "w");
    if (!f) close(fd);
    return f;
}

void init_data_files(void) {
    ensure_data_dir();

    char datetime[MAX_DATE_LEN];
    get_current_datetime(datetime, sizeof(datetime));

    /* Seed users.txt only if it truly doesn't exist. This runs at the start
     * of every engine_cli call: an open that failed because another process
     * was replacing the file used to truncate it to the 16 default accounts. */
    int locked = fm_lock();
    FILE* f = file_missing(FILE_USERS) ? create_new_file(FILE_USERS) : NULL;
    {
        if (f) {
            /* Admin account */
            fprintf(f, "1|admin|admin123|System Administrator|9999999999|admin@gov.in|ADMIN|%s|General|0|1\n", datetime);

            /* Department Officers */
            fprintf(f, "2|water_officer|officer123|Rajesh Kumar|9876543210|rajesh@water.gov.in|DEPT_OFFICER|%s|Water Supply|12|1\n", datetime);
            fprintf(f, "3|elec_officer|officer123|Priya Sharma|9876543211|priya@elec.gov.in|DEPT_OFFICER|%s|Electricity|12|1\n", datetime);
            fprintf(f, "4|road_officer|officer123|Amit Singh|9876543212|amit@roads.gov.in|DEPT_OFFICER|%s|Roads & Transport|12|1\n", datetime);
            fprintf(f, "5|sanit_officer|officer123|Neha Gupta|9876543213|neha@sanit.gov.in|DEPT_OFFICER|%s|Sanitation|12|1\n", datetime);
            fprintf(f, "6|safety_officer|officer123|Vikram Rawat|9876543214|vikram@safety.gov.in|DEPT_OFFICER|%s|Public Safety|12|1\n", datetime);

            /* Workers (2 per department) */
            fprintf(f, "7|water_worker1|worker123|Suresh Negi|9800000001|suresh@water.gov.in|WORKER|%s|Water Supply|0|1\n", datetime);
            fprintf(f, "8|water_worker2|worker123|Ramesh Thapa|9800000002|ramesh@water.gov.in|WORKER|%s|Water Supply|1|1\n", datetime);
            fprintf(f, "9|elec_worker1|worker123|Deepak Joshi|9800000003|deepak@elec.gov.in|WORKER|%s|Electricity|4|1\n", datetime);
            fprintf(f, "10|elec_worker2|worker123|Manoj Bisht|9800000004|manoj@elec.gov.in|WORKER|%s|Electricity|5|1\n", datetime);
            fprintf(f, "11|road_worker1|worker123|Anil Rana|9800000005|anil@roads.gov.in|WORKER|%s|Roads & Transport|11|1\n", datetime);
            fprintf(f, "12|road_worker2|worker123|Sanjay Dhasmana|9800000006|sanjay@roads.gov.in|WORKER|%s|Roads & Transport|14|1\n", datetime);
            fprintf(f, "13|sanit_worker1|worker123|Kishore Pant|9800000007|kishore@sanit.gov.in|WORKER|%s|Sanitation|3|1\n", datetime);
            fprintf(f, "14|sanit_worker2|worker123|Bharat Nautiyal|9800000008|bharat@sanit.gov.in|WORKER|%s|Sanitation|6|1\n", datetime);
            fprintf(f, "15|safety_worker1|worker123|Rakesh Chauhan|9800000009|rakesh@safety.gov.in|WORKER|%s|Public Safety|7|1\n", datetime);
            fprintf(f, "16|safety_worker2|worker123|Mahesh Semwal|9800000010|mahesh@safety.gov.in|WORKER|%s|Public Safety|8|1\n", datetime);

            fclose(f);
        }
    }

    /* Seed departments.txt only if it truly doesn't exist (same reason) */
    f = file_missing(FILE_DEPARTMENTS) ? create_new_file(FILE_DEPARTMENTS) : NULL;
    if (f) {
        fprintf(f, "1|Water Supply|water_officer|0|0\n");
        fprintf(f, "2|Electricity|elec_officer|0|0\n");
        fprintf(f, "3|Roads & Transport|road_officer|0|0\n");
        fprintf(f, "4|Sanitation|sanit_officer|0|0\n");
        fprintf(f, "5|Public Safety|safety_officer|0|0\n");
        fclose(f);
    }

    /* Ensure complaints.txt and logs.txt exist ("a" never truncates) */
    if (file_missing(FILE_COMPLAINTS) && (f = open_data(FILE_COMPLAINTS, "a"))) fclose(f);
    if (file_missing(FILE_LOGS) && (f = open_data(FILE_LOGS, "a"))) fclose(f);
    if (locked) fm_unlock();
}

/* ═══════════════════════════════════════════════════════════════════
 *  User Operations
 * ═══════════════════════════════════════════════════════════════════ */

/* Append one record line under the lock */
static int append_record(const char* path, RecordWriter writer, const void* record) {
    FILE* f;
    int ok = 0;
    if (!fm_lock()) return 0;
    ensure_trailing_newline(path);
    f = open_data(path, "a");
    if (f) {
        writer(f, record);
        ok = (fclose(f) == 0);
    }
    fm_unlock();
    return ok;
}

/* Highest ID at the start of any line, +1. Lines the parser cannot read
 * count too, so a new record never reuses their ID (update_complaint would
 * otherwise rewrite the old line). -1 if the file can't be read or IDs ran
 * out; callers must hold the lock until the new record is saved. */
static int next_id_in(const char* path) {
    char line[LINE_BUF];
    int max_id = 0, at_line_start = 1;
    FILE* f = open_data(path, "r");
    if (!f) return (errno == ENOENT) ? 1 : -1;
    while (fgets(line, sizeof(line), f)) {
        if (at_line_start) {
            int id = leading_id(line);
            if (id > max_id) max_id = id;
        }
        at_line_start = (strchr(line, '\n') != NULL);
    }
    fclose(f);
    return (max_id == INT_MAX) ? -1 : max_id + 1;
}

int save_user(const UserRecord* user) {
    return append_record(FILE_USERS, user_writer, user);
}

int load_users(UserRecord* users, int max_count) {
    FILE* f = open_data(FILE_USERS, "r");
    char line[LINE_BUF];
    int count = 0;
    if (!f) return 0;
    while (count < max_count && fgets(line, sizeof(line), f)) {
        if (parse_user(line, &users[count])) count++;
    }
    fclose(f);
    return count;
}

/* Visit users one at a time; stop early when visit() returns non-zero.
 * Returns 1 if stopped, 0 if every user was visited, -1 if the file could
 * not be read (callers must not take that as "not found"). */
static int for_each_user(int (*visit)(const UserRecord*, void*), void* ctx) {
    FILE* f = open_data(FILE_USERS, "r");
    char line[LINE_BUF];
    UserRecord u;
    int stopped = 0;
    if (!f) return (errno == ENOENT) ? 0 : -1;
    while (fgets(line, sizeof(line), f)) {
        if (parse_user(line, &u) && visit(&u, ctx)) { stopped = 1; break; }
    }
    fclose(f);
    return stopped;
}

typedef struct { const char* username; const char* password; int by_id; int id; UserRecord* out; } UserQuery;

static int match_user(const UserRecord* u, void* ctx) {
    UserQuery* q = (UserQuery*)ctx;
    if (q->by_id) {
        if (u->id != q->id) return 0;
    } else {
        if (strcmp(u->username, q->username) != 0) return 0;
        if (q->password && strcmp(u->password, q->password) != 0) return 0;
    }
    if (q->out) *q->out = *u;
    return 1;
}

int authenticate_user(const char* username, const char* password,
                      const char* filepath, UserRecord* record) {
    UserQuery q = { username, password, 0, 0, record };
    (void)filepath;  /* We always use FILE_USERS */
    if (!username || !password || !username[0]) return 0;
    return for_each_user(match_user, &q) == 1;
}

int find_user_by_username(const char* username, UserRecord* record) {
    UserQuery q = { username, NULL, 0, 0, record };
    if (!username || !username[0]) return 0;
    return for_each_user(match_user, &q) == 1;
}

int find_user_by_id(int id, UserRecord* record) {
    UserQuery q = { NULL, NULL, 1, id, record };
    return for_each_user(match_user, &q) == 1;
}

int update_user(const UserRecord* user) {
    return rewrite_record(FILE_USERS, user->id, user_writer, user);
}

int get_next_user_id(void) {
    return next_id_in(FILE_USERS);
}

/* 1 if taken, 0 if free, -1 if users.txt could not be read */
int username_exists(const char* username) {
    UserQuery q = { username, NULL, 0, 0, NULL };
    if (!username || !username[0]) return 0;
    return for_each_user(match_user, &q);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Complaint Operations
 * ═══════════════════════════════════════════════════════════════════ */

int save_complaint(const Complaint* complaint) {
    return append_record(FILE_COMPLAINTS, complaint_writer, complaint);
}

/* Visit complaints one at a time; stop early when visit() returns non-zero.
 * Returns 1 if stopped, 0 if all were visited, -1 if the file could not be read. */
static int for_each_complaint(int (*visit)(const Complaint*, void*), void* ctx) {
    FILE* f = open_data(FILE_COMPLAINTS, "r");
    char line[LINE_BUF];
    Complaint c;
    int stopped = 0;
    if (!f) return (errno == ENOENT) ? 0 : -1;
    while (fgets(line, sizeof(line), f)) {
        if (parse_complaint(line, &c) && visit(&c, ctx)) { stopped = 1; break; }
    }
    fclose(f);
    return stopped;
}

int load_complaints(Complaint* complaints, int max_count) {
    FILE* f = open_data(FILE_COMPLAINTS, "r");
    char line[LINE_BUF];
    int count = 0;
    if (!f) return 0;
    while (count < max_count && fgets(line, sizeof(line), f)) {
        if (parse_complaint(line, &complaints[count])) count++;
    }
    fclose(f);
    return count;
}

typedef struct {
    int mode;               /* 0 = by user, 1 = by department, 2 = by worker */
    int id;
    const char* department;
    Complaint* out;
    int max_count;
    int count;
} ComplaintFilter;

static int collect_complaint(const Complaint* c, void* ctx) {
    ComplaintFilter* q = (ComplaintFilter*)ctx;
    int match = 0;
    if (q->mode == 0) match = (c->user_id == q->id);
    else if (q->mode == 1) match = (strcmp(c->department, q->department) == 0);
    else match = (c->worker_id == q->id);
    if (match) {
        q->out[q->count++] = *c;
        if (q->count >= q->max_count) return 1;
    }
    return 0;
}

int load_complaints_by_user(int user_id, Complaint* complaints, int max_count) {
    ComplaintFilter q = { 0, user_id, NULL, complaints, max_count, 0 };
    if (max_count <= 0) return 0;
    for_each_complaint(collect_complaint, &q);
    return q.count;
}

int load_complaints_by_department(const char* department,
                                  Complaint* complaints, int max_count) {
    ComplaintFilter q = { 1, 0, department, complaints, max_count, 0 };
    if (max_count <= 0 || !department) return 0;
    for_each_complaint(collect_complaint, &q);
    return q.count;
}

int load_complaints_by_worker(int worker_id, Complaint* complaints, int max_count) {
    ComplaintFilter q = { 2, worker_id, NULL, complaints, max_count, 0 };
    if (max_count <= 0) return 0;
    for_each_complaint(collect_complaint, &q);
    return q.count;
}

typedef struct { int id; Complaint* out; } ComplaintLookup;

static int match_complaint(const Complaint* c, void* ctx) {
    ComplaintLookup* q = (ComplaintLookup*)ctx;
    if (c->id != q->id) return 0;
    if (q->out) *q->out = *c;
    return 1;
}

int find_complaint(int complaint_id, Complaint* out) {
    ComplaintLookup q = { complaint_id, out };
    return for_each_complaint(match_complaint, &q) == 1;
}

int update_complaint(const Complaint* complaint) {
    return rewrite_record(FILE_COMPLAINTS, complaint->id, complaint_writer, complaint);
}

/* Write back records loaded with load_complaints (re-analysis). The file is
 * streamed: each line that parses is matched, in order, with the next
 * record and replaced by it; every other line (records the parser cannot
 * read, records beyond MAX_COMPLAINTS) is copied unchanged. It used to
 * write only the records it was given, which deleted all the others. */
int save_all_complaints(const Complaint* complaints, int count) {
    char tmp[MAX_PATH_LEN], line[LINE_BUF], copy[LINE_BUF];
    FILE *in, *out;
    int next = 0, ok;
    Complaint c;

    if (!fm_lock()) return 0;
    in = open_data(FILE_COMPLAINTS, "r");
    if (!in) { fm_unlock(); return 0; }
    snprintf(tmp, sizeof(tmp), "%s.tmp", FILE_COMPLAINTS);
    out = fopen(tmp, "w");
    if (!out) { fclose(in); fm_unlock(); return 0; }

    while (fgets(line, sizeof(line), in)) {
        strip_line(line);
        if (line[0] == '\0') continue;
        memcpy(copy, line, sizeof(copy));
        if (next < count && parse_complaint(copy, &c) && c.id == complaints[next].id) {
            write_complaint(out, &complaints[next]);
            next++;
        } else {
            fprintf(out, "%s\n", line);
        }
    }
    fclose(in);
    /* Every record must have found its line, else the file changed since it was loaded */
    ok = (fclose(out) == 0 && next == count);
    if (!ok) remove(tmp);
    else ok = replace_file(tmp, FILE_COMPLAINTS);
    fm_unlock();
    return ok;
}

int get_next_complaint_id(void) {
    return next_id_in(FILE_COMPLAINTS);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Department Operations
 * ═══════════════════════════════════════════════════════════════════ */

int save_department(const Department* dept) {
    return append_record(FILE_DEPARTMENTS, department_writer, dept);
}

int load_departments(Department* departments, int max_count) {
    FILE* f = open_data(FILE_DEPARTMENTS, "r");
    char line[LINE_BUF];
    int count = 0;
    if (!f) return 0;
    while (count < max_count && fgets(line, sizeof(line), f)) {
        if (parse_department(line, &departments[count])) count++;
    }
    fclose(f);
    return count;
}

int update_department(const Department* dept) {
    return rewrite_record(FILE_DEPARTMENTS, dept->id, department_writer, dept);
}

typedef struct { Department* depts; int count; } DeptCounter;

static int count_for_department(const Complaint* c, void* ctx) {
    DeptCounter* q = (DeptCounter*)ctx;
    for (int i = 0; i < q->count; i++) {
        if (strcmp(q->depts[i].name, c->department) == 0) {
            q->depts[i].complaint_count++;
            if (strcmp(c->status, "COMPLETED") == 0) q->depts[i].resolved_count++;
            break;
        }
    }
    return 0;
}

/* Recompute each department's complaint/resolved counters from complaints.txt.
 * The departments file is streamed: rows that were loaded get new counters,
 * every other line is copied unchanged (rows past MAX_DEPARTMENTS or rows
 * the parser cannot read used to be deleted). */
int refresh_department_counts(void) {
    Department depts[MAX_DEPARTMENTS];
    DeptCounter q;
    char tmp[MAX_PATH_LEN], line[LINE_BUF], copy[LINE_BUF];
    FILE *in, *out;
    int count, ok = 0;

    if (!fm_lock()) return 0;
    count = load_departments(depts, MAX_DEPARTMENTS);
    for (int i = 0; i < count; i++) {
        depts[i].complaint_count = 0;
        depts[i].resolved_count = 0;
    }
    q.depts = depts;
    q.count = count;
    /* If complaints.txt can't be read, keep the old counters rather than writing zeros */
    if (count == 0 || for_each_complaint(count_for_department, &q) < 0) { fm_unlock(); return 0; }

    in = open_data(FILE_DEPARTMENTS, "r");
    snprintf(tmp, sizeof(tmp), "%s.tmp", FILE_DEPARTMENTS);
    out = in ? fopen(tmp, "w") : NULL;
    if (in && out) {
        while (fgets(line, sizeof(line), in)) {
            Department d;
            int index = -1;
            strip_line(line);
            if (line[0] == '\0') continue;
            memcpy(copy, line, sizeof(copy));
            if (parse_department(copy, &d)) {
                for (int i = 0; i < count; i++)
                    if (depts[i].id == d.id) { index = i; break; }
            }
            if (index >= 0) write_department(out, &depts[index]);
            else fprintf(out, "%s\n", line);
        }
        ok = (fclose(out) == 0);
        if (!ok) remove(tmp);
    } else if (out) {
        fclose(out);
        remove(tmp);
    }
    if (in) fclose(in);
    if (ok) ok = replace_file(tmp, FILE_DEPARTMENTS);
    fm_unlock();
    return ok;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Worker Operations
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct { const char* department; UserRecord* out; int max_count; int count; } WorkerQuery;

static int collect_worker(const UserRecord* u, void* ctx) {
    WorkerQuery* q = (WorkerQuery*)ctx;
    if (strcmp(u->role, "WORKER") == 0 && strcmp(u->department, q->department) == 0) {
        q->out[q->count++] = *u;
        if (q->count >= q->max_count) return 1;
    }
    return 0;
}

int load_workers_by_department(const char* department,
                               UserRecord* workers, int max_count) {
    WorkerQuery q = { department, workers, max_count, 0 };
    if (max_count <= 0 || !department) return 0;
    for_each_user(collect_worker, &q);
    return q.count;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Statistics
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct { const char* department; ComplaintStats* stats; } StatsQuery;

static int add_to_stats(const Complaint* c, void* ctx) {
    StatsQuery* q = (StatsQuery*)ctx;
    ComplaintStats* s = q->stats;
    if (q->department && strcmp(c->department, q->department) != 0) return 0;
    s->total++;
    if (strcmp(c->status, "PENDING") == 0) s->pending++;
    else if (strcmp(c->status, "REVIEWED") == 0) s->reviewed++;
    else if (strcmp(c->status, "ASSIGNED") == 0) s->assigned++;
    else if (strcmp(c->status, "IN_PROGRESS") == 0) s->in_progress++;
    else if (strcmp(c->status, "COMPLETED") == 0) s->completed++;
    return 0;
}

ComplaintStats get_complaint_stats(void) {
    ComplaintStats stats = {0, 0, 0, 0, 0, 0};
    StatsQuery q = { NULL, &stats };
    for_each_complaint(add_to_stats, &q);
    return stats;
}

ComplaintStats get_department_stats(const char* department) {
    ComplaintStats stats = {0, 0, 0, 0, 0, 0};
    StatsQuery q = { department, &stats };
    if (!department) return stats;
    for_each_complaint(add_to_stats, &q);
    return stats;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Logging
 * ═══════════════════════════════════════════════════════════════════ */

void log_activity(const char* message) {
    char clean[512];
    char datetime[MAX_DATE_LEN];
    FILE* f;

    copy_str(clean, sizeof(clean), message);
    for (char* p = clean; *p; p++) {
        if (*p == '\r' || *p == '\n') *p = ' ';
    }

    if (!fm_lock()) return;
    ensure_trailing_newline(FILE_LOGS);
    f = open_data(FILE_LOGS, "a");
    if (f) {
        get_current_datetime(datetime, sizeof(datetime));
        fprintf(f, "[%s] %s\n", datetime, clean);
        fclose(f);
    }
    fm_unlock();
}
