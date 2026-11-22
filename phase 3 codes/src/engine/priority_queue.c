/*
 * priority_queue.c
 * Max-heap priority queue implementation.
 * Complaints with higher severity float to the top.
 */

#include "../../include/engine/priority_queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ─── Helper macros for heap index navigation ─── */
#define parent(i)       (((i) - 1) / 2)
#define left_child(i)   (2 * (i) + 1)
#define right_child(i)  (2 * (i) + 2)

/* Ordering: higher severity first; on equal severity the older complaint
 * (earlier submitted_date, then lower ID) comes first, so equally severe
 * complaints are served first-come, first-served. Returns 1 if a > b. */
int pq_higher(const Complaint* a, const Complaint* b) {
    if (a->severity != b->severity) return a->severity > b->severity;
    int cmp = strcmp(a->submitted_date, b->submitted_date);
    if (cmp != 0) return cmp < 0;
    return a->id < b->id;
}

/* Initialize the priority queue */
void pq_init(PriorityQueue* pq, int capacity) {
    pq->capacity = capacity;
    pq->size = 0;
    pq->complaints = (Complaint*)malloc(sizeof(Complaint) * capacity);
    if (!pq->complaints) {
        fprintf(stderr, "Error: Failed to allocate priority queue\n");
        pq->capacity = 0;
    }
}

/* Swap two complaints */
void pq_swap(Complaint* a, Complaint* b) {
    Complaint temp = *a;
    *a = *b;
    *b = temp;
}

/* Resize the queue (double capacity) */
void pq_resize(PriorityQueue* pq) {
    int new_capacity = pq->capacity * 2;
    Complaint* new_arr = (Complaint*)realloc(pq->complaints,
                                              sizeof(Complaint) * new_capacity);
    if (!new_arr) {
        fprintf(stderr, "Error: Failed to resize priority queue\n");
        return;
    }
    pq->complaints = new_arr;
    pq->capacity = new_capacity;
}

/* Heapify Up — restore heap property after insertion */
void pq_heapify_up(PriorityQueue* pq, int index) {
    while (index > 0 && pq_higher(&pq->complaints[index],
                                  &pq->complaints[parent(index)])) {
        pq_swap(&pq->complaints[parent(index)], &pq->complaints[index]);
        index = parent(index);
    }
}

/* Heapify Down — restore heap property after extraction */
void pq_heapify_down(PriorityQueue* pq, int index) {
    int largest = index;
    int l = left_child(index);
    int r = right_child(index);
    
    if (l < pq->size && pq_higher(&pq->complaints[l], &pq->complaints[largest]))
        largest = l;
    if (r < pq->size && pq_higher(&pq->complaints[r], &pq->complaints[largest]))
        largest = r;
    
    if (largest != index) {
        pq_swap(&pq->complaints[index], &pq->complaints[largest]);
        pq_heapify_down(pq, largest);
    }
}

/* Insert a complaint into the priority queue */
void pq_insert(PriorityQueue* pq, Complaint complaint) {
    if (pq->size >= pq->capacity) {
        if (pq->capacity <= 0) {
            pq_init(pq, 16);
        } else {
            pq_resize(pq);
        }
        if (pq->size >= pq->capacity) return;  /* allocation failed */
    }
    pq->complaints[pq->size] = complaint;
    pq->size++;
    pq_heapify_up(pq, pq->size - 1);
}

/* Extract the complaint with highest severity (caller checks pq_is_empty) */
Complaint pq_extract_max(PriorityQueue* pq) {
    Complaint max_complaint = pq->complaints[0];
    pq->complaints[0] = pq->complaints[pq->size - 1];
    pq->size--;
    if (pq->size > 0) {
        pq_heapify_down(pq, 0);
    }
    return max_complaint;
}

/* Peek at the top complaint without removing */
Complaint pq_peek(const PriorityQueue* pq) {
    return pq->complaints[0];
}

/* Check if the queue is empty */
int pq_is_empty(const PriorityQueue* pq) {
    return pq->size == 0;
}

/* Get current size */
int pq_size(const PriorityQueue* pq) {
    return pq->size;
}

/* Update the severity of a complaint by its ID */
int pq_update_priority(PriorityQueue* pq, int complaint_id, int new_severity) {
    int i;
    for (i = 0; i < pq->size; i++) {
        if (pq->complaints[i].id == complaint_id) {
            int old_severity = pq->complaints[i].severity;
            pq->complaints[i].severity = new_severity;
            
            if (new_severity > old_severity) {
                pq_heapify_up(pq, i);
            } else if (new_severity < old_severity) {
                pq_heapify_down(pq, i);
            }
            return 1;
        }
    }
    return 0;  /* Complaint ID not found */
}

/* Get all complaints sorted by severity (descending) */
int pq_get_sorted(PriorityQueue* pq, Complaint* output, int max_count) {
    /* Create a temporary copy of the queue */
    PriorityQueue temp;
    pq_init(&temp, pq->capacity > 0 ? pq->capacity : 1);
    if (!temp.complaints) return 0;
    
    for (int i = 0; i < pq->size; i++) {
        temp.complaints[i] = pq->complaints[i];
    }
    temp.size = pq->size;
    
    int count = 0;
    while (!pq_is_empty(&temp) && count < max_count) {
        output[count++] = pq_extract_max(&temp);
    }
    
    pq_free(&temp);
    return count;
}

/* Free all memory */
void pq_free(PriorityQueue* pq) {
    if (pq->complaints) {
        free(pq->complaints);
        pq->complaints = NULL;
    }
    pq->size = 0;
    pq->capacity = 0;
}
