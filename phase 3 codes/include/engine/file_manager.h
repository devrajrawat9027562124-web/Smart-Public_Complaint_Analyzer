/*
 * file_manager.h
 * File I/O and persistent storage using pipe-delimited .txt files.
 * Handles CRUD operations for users, complaints, departments, and logs.
 */

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "complaint_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Initialization ─── */

/* Initialize data directory and seed default data if files don't exist */
void init_data_files(void);

/* ─── Cross-process lock ───
 * Hold it for a whole read-modify-write sequence (e.g. next ID + save, or
 * find + update) so the console app and engine_cli processes cannot
 * interleave. Re-entrant; every write function below takes it too.
 * fm_lock returns 0 if the lock could not be had within about 10 s. */
int fm_lock(void);
void fm_unlock(void);

/* ─── User Operations ─── */

/* Save a new user record (append) */
int save_user(const UserRecord* user);

/* Load all users into array, returns count */
int load_users(UserRecord* users, int max_count);

/* Authenticate user by username and password, fills record if found */
int authenticate_user(const char* username, const char* password,
                      const char* filepath, UserRecord* record);

/* Update an existing user record */
int update_user(const UserRecord* user);

/* Get next available user ID (-1 if users.txt can't be read) */
int get_next_user_id(void);

/* 1 if the username exists, 0 if not, -1 if users.txt can't be read */
int username_exists(const char* username);

/* Find a user by username / by ID (returns 1 if found; record may be NULL) */
int find_user_by_username(const char* username, UserRecord* record);
int find_user_by_id(int id, UserRecord* record);

/* ─── Complaint Operations ─── */

/* Save a new complaint (append) */
int save_complaint(const Complaint* complaint);

/* Load all complaints into array, returns count */
int load_complaints(Complaint* complaints, int max_count);

/* Load complaints for a specific user */
int load_complaints_by_user(int user_id, Complaint* complaints, int max_count);

/* Load complaints for a specific department */
int load_complaints_by_department(const char* department,
                                  Complaint* complaints, int max_count);

/* Load complaints assigned to a specific worker */
int load_complaints_by_worker(int worker_id, Complaint* complaints, int max_count);

/* Update an existing complaint */
int update_complaint(const Complaint* complaint);

/* Get next available complaint ID (-1 if complaints.txt can't be read) */
int get_next_complaint_id(void);

/* Find one complaint by ID (returns 1 if found; out may be NULL) */
int find_complaint(int complaint_id, Complaint* out);

/* Write back records loaded with load_complaints, replacing each one's line
 * (atomic replace). Lines that were not loaded are kept unchanged. Fails if
 * the file no longer matches the records. */
int save_all_complaints(const Complaint* complaints, int count);

/* ─── Department Operations ─── */

/* Save a new department (append) */
int save_department(const Department* dept);

/* Load all departments into array, returns count */
int load_departments(Department* departments, int max_count);

/* Update a department record */
int update_department(const Department* dept);

/* Recompute every department's complaint/resolved counters from complaints */
int refresh_department_counts(void);

/* ─── Worker Operations ─── */

/* Load workers for a specific department */
int load_workers_by_department(const char* department,
                               UserRecord* workers, int max_count);

/* ─── Statistics ─── */

/* Compute complaint statistics */
ComplaintStats get_complaint_stats(void);

/* Get complaint stats for a specific department */
ComplaintStats get_department_stats(const char* department);

/* ─── Logging ─── */

/* Log an activity message */
void log_activity(const char* message);

/* Get current date/time as string */
void get_current_datetime(char* buffer, int size);

/* Replace '|', CR, LF and TAB in a text field so it cannot break a record */
void sanitize_field(char* s);

#ifdef __cplusplus
}
#endif

#endif /* FILE_MANAGER_H */
