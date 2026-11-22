/*
 * priority_queue.h
 * Max-heap priority queue for complaint severity ordering.
 * Highest-severity complaints are served first.
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "complaint_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Priority Queue — backed by a dynamic array (max-heap) */
typedef struct {
    Complaint* complaints;
    int size;
    int capacity;
} PriorityQueue;

/* ─── Operations ─── */

/* Initialize the priority queue with a given initial capacity */
void pq_init(PriorityQueue* pq, int capacity);

/* Insert a complaint into the priority queue */
void pq_insert(PriorityQueue* pq, Complaint complaint);

/* Extract the complaint with the highest severity */
Complaint pq_extract_max(PriorityQueue* pq);

/* Peek at the highest-severity complaint without removing it */
Complaint pq_peek(const PriorityQueue* pq);

/* Check if the priority queue is empty */
int pq_is_empty(const PriorityQueue* pq);

/* Get the current size of the priority queue */
int pq_size(const PriorityQueue* pq);

/* Update the priority (severity) of a complaint by ID */
int pq_update_priority(PriorityQueue* pq, int complaint_id, int new_severity);

/* Get all complaints sorted by severity (descending) into output array */
int pq_get_sorted(PriorityQueue* pq, Complaint* output, int max_count);

/* Free all memory used by the priority queue */
void pq_free(PriorityQueue* pq);

/* Heap ordering: 1 if a should be served before b (severity, then age) */
int pq_higher(const Complaint* a, const Complaint* b);

/* Internal heap operations */
void pq_heapify_up(PriorityQueue* pq, int index);
void pq_heapify_down(PriorityQueue* pq, int index);
void pq_swap(Complaint* a, Complaint* b);
void pq_resize(PriorityQueue* pq);

#ifdef __cplusplus
}
#endif

#endif /* PRIORITY_QUEUE_H */
