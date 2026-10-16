# 📋 Smart Public Complaint Analyzer

## Comprehensive Project Documentation — Concepts, Data Structures, Algorithms & Design Patterns

---

## 📖 Table of Contents

1. [Project Overview](#1-project-overview)
2. [Architecture & Layered Design](#2-architecture--layered-design)
3. [C/C++ Interoperability (`extern "C"`)](#3-cc-interoperability-extern-c)
4. [Data Structures Used](#4-data-structures-used)
   - 4.1 [Structs (C Structures)](#41-structs-c-structures)
   - 4.2 [Hash Table (Chaining with Linked Lists)](#42-hash-table-chaining-with-linked-lists)
   - 4.3 [Priority Queue (Max-Heap)](#43-priority-queue-max-heap)
   - 4.4 [Graph (Adjacency Matrix)](#44-graph-adjacency-matrix)
   - 4.5 [Arrays (Static & Dynamic)](#45-arrays-static--dynamic)
   - 4.6 [Linked List (within Hash Table)](#46-linked-list-within-hash-table)
5. [Algorithms Used](#5-algorithms-used)
   - 5.1 [Dijkstra's Shortest Path Algorithm](#51-dijkstras-shortest-path-algorithm)
   - 5.2 [Levenshtein Distance (Edit Distance — Dynamic Programming)](#52-levenshtein-distance-edit-distance--dynamic-programming)
   - 5.3 [Jaccard Similarity (Set-Based)](#53-jaccard-similarity-set-based)
   - 5.4 [djb2 Hash Function](#54-djb2-hash-function)
   - 5.5 [NLP Keyword-Based Severity Analysis](#55-nlp-keyword-based-severity-analysis)
   - 5.6 [Heapify (Up & Down)](#56-heapify-up--down)
   - 5.7 [Tokenization](#57-tokenization)
   - 5.8 [Spam / Irrelevance Detection](#58-spam--irrelevance-detection)
6. [Object-Oriented Programming Concepts (C++)](#6-object-oriented-programming-concepts-c)
   - 6.1 [Classes & Objects](#61-classes--objects)
   - 6.2 [Inheritance](#62-inheritance)
   - 6.3 [Polymorphism (Virtual Functions)](#63-polymorphism-virtual-functions)
   - 6.4 [Abstraction (Abstract Base Class)](#64-abstraction-abstract-base-class)
   - 6.5 [Encapsulation](#65-encapsulation)
   - 6.6 [Constructor Overloading](#66-constructor-overloading)
7. [Design Patterns](#7-design-patterns)
   - 7.1 [Factory Pattern](#71-factory-pattern)
   - 7.2 [Layered Architecture Pattern](#72-layered-architecture-pattern)
   - 7.3 [MVC-like Separation](#73-mvc-like-separation)
8. [File I/O & Persistent Storage](#8-file-io--persistent-storage)
9. [Build System (Makefile)](#9-build-system-makefile)
10. [Memory Management](#10-memory-management)
11. [Preprocessor Directives & Include Guards](#11-preprocessor-directives--include-guards)
12. [Role-Based Access Control (RBAC)](#12-role-based-access-control-rbac)
13. [Complete File Structure Map](#13-complete-file-structure-map)
14. [How Everything Connects — End-to-End Flow](#14-how-everything-connects--end-to-end-flow)

---

## 1. Project Overview

The **Smart Public Complaint Analyzer** is a console-based application built using **C (Engine Layer)** and **C++ (Interface Layer)**. It allows citizens to submit public complaints (water supply issues, road damage, electricity problems, etc.), which are then automatically analyzed using **NLP-based keyword scoring**, checked for **duplicates** using string-similarity algorithms, prioritized via a **max-heap priority queue**, and routed to field workers using **Dijkstra's shortest path algorithm** on a real city graph (Dehradun).

**Key Features:**
- User registration and authentication with role-based access (User, Admin, Department Officer, Worker)
- NLP-based automatic severity scoring (1–10) of complaints
- Automatic category suggestion (Water, Electricity, Roads, Sanitation, Public Safety)
- Duplicate complaint detection using Levenshtein Distance and Jaccard Similarity
- Spam/irrelevant complaint filtering
- Priority-based complaint queue (Max-Heap) — highest severity served first
- City graph with Dijkstra's algorithm for optimal worker routing
- Persistent file-based storage using pipe-delimited `.txt` files
- Rich console UI with ANSI color codes, box-drawing characters, and progress bars

---

## 2. Architecture & Layered Design

The project follows a **two-layer architecture** that separates concerns cleanly:

```
┌─────────────────────────────────────────────────┐
│           INTERFACE LAYER (C++)                 │
│  MenuSystem, AuthManager, Dashboard             │
│  Person, User, Admin, Worker, DepartmentOfficer │
│  (OOP: Inheritance, Polymorphism, Encapsulation)│
├─────────────────────────────────────────────────┤
│            ENGINE LAYER (C)                     │
│  file_manager, nlp_analyzer, hash_table         │
│  priority_queue, duplicate_detector             │
│  route_optimizer                                │
│  (DSA: Hash Tables, Heaps, Graphs, Dijkstra)   │
└─────────────────────────────────────────────────┘
         ↕ linked via extern "C"
```

| Layer | Language | Responsibility |
|-------|----------|---------------|
| **Engine** | C | Core data structures, algorithms, file I/O, NLP analysis |
| **Interface** | C++ | User interaction, OOP class hierarchy, menus, dashboards |

**Why this separation?**
- The C engine is lightweight, fast, and handles all computation-heavy work (hashing, graph traversal, text analysis).
- The C++ interface leverages OOP for clean role-based user management and rich console output.
- The two layers communicate via `extern "C"` linkage, allowing C++ code to call C functions directly.

---

## 3. C/C++ Interoperability (`extern "C"`)

### What is it?
`extern "C"` is a **linkage specification** in C++ that tells the compiler to use **C-style name mangling** for the enclosed function declarations. This is necessary because C++ normally "mangles" function names (to support overloading), but C does not.

### How it is used in the project:

**In C header files** (e.g., `hash_table.h`, `priority_queue.h`):
```c
#ifdef __cplusplus
extern "C" {
#endif

void ht_init(HashTable* ht, int size);
void ht_insert(HashTable* ht, const char* key, int value);
// ... more C functions

#ifdef __cplusplus
}
#endif
```

**In C++ interface** (e.g., `Person.h`):
```cpp
extern "C" {
    #include "../../include/engine/complaint_types.h"
    #include "../../include/engine/file_manager.h"
    #include "../../include/engine/nlp_analyzer.h"
    // ... more C headers
}
```

### Why it matters:
- Without `extern "C"`, the C++ compiler would mangle the function names differently from how the C compiler compiled them, causing **linker errors**.
- This is the standard technique for **mixed C/C++ projects**.

---

## 4. Data Structures Used

### 4.1 Structs (C Structures)

**Concept:** A `struct` is a composite data type that groups related variables under one name. Each variable inside is called a **member** or **field**.

**Structs defined in the project (`complaint_types.h`):**

| Struct | Purpose | Key Fields |
|--------|---------|------------|
| `Complaint` | Stores all data for a single complaint | `id`, `user_id`, `title`, `description`, `category`, `location`, `severity`, `nlp_score`, `is_duplicate`, `status`, `worker_id` |
| `UserRecord` | Stores user/person information | `id`, `username`, `password`, `full_name`, `role`, `department`, `current_location_id`, `is_available` |
| `Location` | A single graph node (city location) | `id`, `name` |
| `CityGraph` | The entire city as a weighted graph | `adj_matrix[20][20]`, `locations[20]`, `num_locations` |
| `RouteResult` | Output of Dijkstra's algorithm | `path[]`, `path_length`, `total_distance` |
| `Department` | Government department info | `id`, `name`, `officer_username`, `complaint_count`, `resolved_count` |
| `ComplaintStats` | Aggregate statistics | `total`, `pending`, `reviewed`, `assigned`, `in_progress`, `completed` |

**Example — Complaint struct:**
```c
typedef struct {
    int id;
    int user_id;
    char title[100];
    char description[500];
    char category[50];
    char location[100];
    int location_id;
    int severity;        // Final severity (1-10), may be admin-overridden
    int nlp_score;       // Original NLP-computed severity
    int is_duplicate;    // 0 = not duplicate, 1 = duplicate
    int duplicate_of;    // ID of original if duplicate, -1 otherwise
    int is_irrelevant;   // 0 = relevant, 1 = irrelevant
    char status[20];
    char department[50];
    int worker_id;       // -1 if unassigned
    char submitted_date[20];
    char completed_date[20];
} Complaint;
```

**How it's used:** Every complaint submitted by a user is stored in this struct. The NLP analyzer fills `severity` and `nlp_score`, the duplicate detector sets `is_duplicate` and `duplicate_of`, and the admin/officer updates `status`, `department`, and `worker_id`.

---

### 4.2 Hash Table (Chaining with Linked Lists)

**Concept:** A hash table is a data structure that provides **O(1) average-time** lookups, insertions, and deletions by mapping keys to array indices using a **hash function**. When two keys hash to the same index (**collision**), a **linked list** at that bucket resolves the conflict — this is called **separate chaining**.

**Files:** `hash_table.h`, `hash_table.c`

**Structure:**
```c
typedef struct HashNode {
    char key[50];
    int value;
    struct HashNode* next;   // Linked list for collision chaining
} HashNode;

typedef struct {
    HashNode** buckets;      // Array of pointers to linked lists
    int size;                // Number of buckets
    int count;               // Number of entries
} HashTable;
```

**How it works in the project:**

```
Index 0:  NULL
Index 1:  ["fire" → 10] → ["urgent" → 6] → NULL
Index 2:  ["broken" → 6] → NULL
  ...
Index 126: ["flood" → 9] → NULL
```

1. **NLP Analyzer** uses **8 separate hash tables** to store keyword dictionaries:
   - `keyword_dict` — 120+ severity keywords (e.g., "fire" → weight 10, "pothole" → weight 4)
   - `negation_dict` — negation words ("not", "never", "no")
   - `amplifier_dict` — amplifier words ("very", "extremely", "severely")
   - 5 category tables — `category_water`, `category_electricity`, `category_roads`, `category_sanitation`, `category_safety`

2. When a complaint is submitted, the text is **tokenized** and each token is **looked up** in the hash table in O(1) time.

**Key Operations:**
| Operation | Function | Time Complexity |
|-----------|----------|----------------|
| Initialize | `ht_init(ht, 127)` | O(n) where n = bucket count |
| Insert | `ht_insert(ht, "fire", 10)` | O(1) average |
| Search | `ht_search(ht, "fire")` → returns 10 | O(1) average |
| Contains | `ht_contains(ht, "fire")` → returns 1 | O(1) average |
| Remove | `ht_remove(ht, "fire")` | O(1) average |
| Free | `ht_free(ht)` | O(n + m), n=buckets, m=entries |

**Why size 127?** The bucket count `HT_DEFAULT_SIZE = 127` is a **prime number**, which provides better distribution and fewer collisions with the djb2 hash function.

---

### 4.3 Priority Queue (Max-Heap)

**Concept:** A **priority queue** is an abstract data type where each element has a priority, and the element with the **highest priority** is served first. This project implements it using a **max-heap** — a complete binary tree stored in an array where every parent node has a value ≥ its children.

**Files:** `priority_queue.h`, `priority_queue.c`

**Structure:**
```c
typedef struct {
    Complaint* complaints;  // Dynamic array (heap storage)
    int size;               // Current number of elements
    int capacity;           // Allocated capacity
} PriorityQueue;
```

**How the heap is stored in an array:**
```
Array index:  [0]  [1]  [2]  [3]  [4]  [5]  [6]
Severity:      10    8    9    5    7    3    6

Tree view:
              10 (index 0)
            /    \
         8 (1)    9 (2)
        / \      / \
      5(3) 7(4) 3(5) 6(6)

Parent of i  = (i - 1) / 2
Left child   = 2 * i + 1
Right child  = 2 * i + 2
```

**How it's used in the project:**

The priority queue is used to manage complaints **sorted by severity**. When an admin or department officer views pending complaints, they see the **most severe complaints first**.

**Key Operations:**
| Operation | Function | Time Complexity | Description |
|-----------|----------|----------------|-------------|
| Insert | `pq_insert(pq, complaint)` | O(log n) | Adds complaint and heapifies up |
| Extract Max | `pq_extract_max(pq)` | O(log n) | Removes highest severity complaint |
| Peek | `pq_peek(pq)` | O(1) | View top without removing |
| Update Priority | `pq_update_priority(pq, id, new_severity)` | O(n + log n) | Find by ID, update, re-heapify |
| Get Sorted | `pq_get_sorted(pq, output, max)` | O(n log n) | Extract all in sorted order |

**Heapify Up (after insertion):**
```c
void pq_heapify_up(PriorityQueue* pq, int index) {
    while (index > 0 && pq->complaints[parent(index)].severity
                      < pq->complaints[index].severity) {
        pq_swap(&pq->complaints[parent(index)], &pq->complaints[index]);
        index = parent(index);
    }
}
```
*Walks up the tree, swapping with parent if the current node is larger.*

**Heapify Down (after extraction):**
```c
void pq_heapify_down(PriorityQueue* pq, int index) {
    int largest = index;
    int l = left_child(index);
    int r = right_child(index);
    
    if (l < pq->size && pq->complaints[l].severity > pq->complaints[largest].severity)
        largest = l;
    if (r < pq->size && pq->complaints[r].severity > pq->complaints[largest].severity)
        largest = r;
    
    if (largest != index) {
        pq_swap(&pq->complaints[index], &pq->complaints[largest]);
        pq_heapify_down(pq, largest);   // Recursive call
    }
}
```
*Walks down the tree, swapping with the larger child.*

**Dynamic Resizing:** When the heap is full, `pq_resize()` doubles the capacity using `realloc()`.

---

### 4.4 Graph (Adjacency Matrix)

**Concept:** A **graph** is a data structure consisting of **nodes (vertices)** and **edges (connections)** between them. This project models the city of **Dehradun** as a **weighted undirected graph** using an **adjacency matrix**.

**Files:** `route_optimizer.h`, `route_optimizer.c`

**Structure:**
```c
typedef struct {
    int adj_matrix[20][20];    // adj_matrix[i][j] = distance in km (0 = no edge)
    Location locations[20];     // Array of location nodes
    int num_locations;          // Number of nodes
} CityGraph;
```

**Adjacency Matrix Representation:**
```
         Clock  Paltan  Rajpur  ISBT  Railway  ...
Clock    [  0      1       3      0      2     ...]
Paltan   [  1      0       0      0      0     ...]
Rajpur   [  3      0       0      0      0     ...]
ISBT     [  0      0       0      0      0     ...]
Railway  [  2      0       0      0      0     ...]
  ...
```
- `adj_matrix[i][j] > 0` means there is a road of that distance (km) between locations i and j.
- `adj_matrix[i][j] = 0` means no direct road.
- The matrix is **symmetric** (undirected): `adj_matrix[i][j] == adj_matrix[j][i]`.

**Real Dehradun Locations (18 nodes):**

| ID | Location Name | Description |
|----|--------------|-------------|
| 0 | Clock Tower | City center / Ghanta Ghar |
| 1 | Paltan Bazaar | Commercial hub |
| 2 | Rajpur Road | Main arterial road |
| 3 | ISBT Dehradun | Inter-State Bus Terminal |
| 4 | Dehradun Railway Stn | Railway Station |
| 5 | Race Course | Central open area |
| 6 | Dalanwala | Residential area |
| 7 | Clement Town | Cantonment area |
| 8 | Prem Nagar | Residential / industrial |
| 9 | Sahastradhara | Tourist / outer area |
| 10 | Ballupur Chowk | Intersection on Rajpur Rd |
| 11 | GMS Road | IT / commercial corridor |
| 12 | Nagar Nigam (Municipal) | Municipal Corporation HQ |
| 13 | Doon Hospital | Main government hospital |
| 14 | Survey Chowk | Major intersection |
| 15 | Rispana Bridge | Bridge / river crossing |
| 16 | Collectorate | District administration |
| 17 | IT Park Sahastradhara | IT Park |

**How it's used:** When a worker is assigned a complaint, the system uses **Dijkstra's algorithm** on this graph to find the shortest path from the worker's current location to the complaint location.

---

### 4.5 Arrays (Static & Dynamic)

**Static Arrays:** Used throughout the project for fixed-size buffers.
```c
char title[MAX_TITLE_LEN];        // Fixed 100-char buffer
Complaint complaints[MAX_COMPLAINTS]; // Array of up to 500 complaints
int adj_matrix[MAX_LOCATIONS][MAX_LOCATIONS]; // 2D array for graph
```

**Dynamic Arrays:** The priority queue uses `malloc()` / `realloc()` for dynamic sizing.
```c
pq->complaints = (Complaint*)malloc(sizeof(Complaint) * capacity);
// ... later, when full:
pq->complaints = (Complaint*)realloc(pq->complaints, sizeof(Complaint) * new_capacity);
```

---

### 4.6 Linked List (within Hash Table)

**Concept:** A linked list is a linear data structure where each element (node) points to the next. It's used here within the hash table for **separate chaining** — resolving hash collisions.

**Structure:**
```c
typedef struct HashNode {
    char key[50];
    int value;
    struct HashNode* next;  // Pointer to next node in the chain
} HashNode;
```

**How chaining works:**
```
Bucket 42: ["fire" | 10 | *] → ["collapsing" | 10 | *] → NULL
```
When "fire" and "collapsing" both hash to bucket 42, they form a linked list. Insertion prepends to the head (O(1)), and search walks the chain.

---

## 5. Algorithms Used

### 5.1 Dijkstra's Shortest Path Algorithm

**Concept:** Dijkstra's algorithm finds the **shortest path** from a source node to all other nodes (or a specific destination) in a **weighted graph with non-negative edge weights**. It uses a **greedy approach** — always expanding the unvisited node with the smallest known distance.

**File:** `route_optimizer.c` → `dijkstra()`

**Algorithm Steps:**
```
1. Initialize all distances to ∞ (INF = 999999), source distance to 0.
2. Mark all nodes as unvisited.
3. Repeat until destination is reached or all reachable nodes visited:
   a. Select the unvisited node u with the minimum distance.
   b. Mark u as visited.
   c. For each neighbor v of u:
      - If dist[u] + weight(u,v) < dist[v]:
        - Update dist[v] = dist[u] + weight(u,v)
        - Record prev[v] = u (for path reconstruction)
4. Reconstruct the path by backtracking from destination using prev[].
```

**Implementation:**
```c
int dijkstra(CityGraph* graph, int source, int destination, RouteResult* result) {
    int dist[MAX_LOCATIONS];   // Shortest known distances
    int visited[MAX_LOCATIONS]; // Visited flags
    int prev[MAX_LOCATIONS];   // Previous node in shortest path
    
    // Initialize
    for (int i = 0; i < n; i++) {
        dist[i] = INF;  // 999999
        visited[i] = 0;
        prev[i] = -1;
    }
    dist[source] = 0;
    
    // Main loop
    for (int count = 0; count < n; count++) {
        // Find minimum distance unvisited node
        int u = -1, min_dist = INF;
        for (int v = 0; v < n; v++) {
            if (!visited[v] && dist[v] < min_dist) {
                min_dist = dist[v]; u = v;
            }
        }
        if (u == -1 || u == destination) break;
        visited[u] = 1;
        
        // Relax neighbors
        for (int v = 0; v < n; v++) {
            if (!visited[v] && graph->adj_matrix[u][v] > 0) {
                int new_dist = dist[u] + graph->adj_matrix[u][v];
                if (new_dist < dist[v]) {
                    dist[v] = new_dist;
                    prev[v] = u;
                }
            }
        }
    }
    // Reconstruct path from prev[] ...
}
```

**Time Complexity:** O(V²) where V = number of vertices (locations). This is the basic implementation without a min-heap.

**Use in the project:**
- A worker at "Rajpur Road" is assigned a complaint at "Sahastradhara".
- Dijkstra finds: **Rajpur Road → Ballupur Chowk → Sahastradhara** (total: 9 km).
- The optimized route is displayed to the worker.

---

### 5.2 Levenshtein Distance (Edit Distance — Dynamic Programming)

**Concept:** The Levenshtein distance between two strings is the **minimum number of single-character edits** (insertions, deletions, or substitutions) required to transform one string into the other. It is a classic **dynamic programming** algorithm.

**File:** `duplicate_detector.c` → `levenshtein_distance()`

**Algorithm:**
```
For strings s1 (length m) and s2 (length n):
- Create a matrix dp[m+1][n+1]
- dp[i][j] = edit distance between s1[0..i-1] and s2[0..j-1]
- Recurrence:
    dp[i][j] = min(
        dp[i-1][j] + 1,       // Deletion
        dp[i][j-1] + 1,       // Insertion
        dp[i-1][j-1] + cost   // Substitution (cost=0 if chars match)
    )
```

**Space Optimization in the project:** Instead of a full m×n matrix, the implementation uses only **two rows** (prev_row and curr_row), reducing space from O(m×n) to O(n):
```c
int* prev_row = (int*)malloc((len2 + 1) * sizeof(int));
int* curr_row = (int*)malloc((len2 + 1) * sizeof(int));
```

**Levenshtein Similarity (normalized to 0.0–1.0):**
```c
double levenshtein_similarity(const char* s1, const char* s2) {
    int distance = levenshtein_distance(s1, s2);
    int max_len = max(strlen(s1), strlen(s2));
    return 1.0 - ((double)distance / max_len);
}
// "broken pipe" vs "broken pipe leak" → similarity ≈ 0.73
```

**Use in the project:** Detects if a newly submitted complaint is textually similar to an existing one — contributes 30% to the combined similarity score.

---

### 5.3 Jaccard Similarity (Set-Based)

**Concept:** Jaccard similarity measures the similarity between two **sets** as the size of their intersection divided by the size of their union:

```
J(A, B) = |A ∩ B| / |A ∪ B|
```

**File:** `duplicate_detector.c` → `jaccard_similarity()`

**How it works:**
1. Tokenize both complaint texts into **word sets** (lowercase, skip words ≤ 2 chars).
2. Count the number of **shared words** (intersection).
3. Compute: `Jaccard = intersection / (count1 + count2 - intersection)`

**Example:**
```
Complaint A: "broken water pipe leaking badly"
    → words: {broken, water, pipe, leaking, badly}

Complaint B: "water pipe broken and leaking"
    → words: {water, pipe, broken, and, leaking}

Intersection: {broken, water, pipe, leaking} = 4
Union: {broken, water, pipe, leaking, badly, and} = 6
Jaccard = 4/6 ≈ 0.67
```

**Combined Similarity Score (weighted):**
```c
double combined = (JACCARD_WEIGHT * jac_sim)         // 0.4 × Jaccard
                + (LEVENSHTEIN_WEIGHT * lev_sim)     // 0.3 × Levenshtein
                + (CATEGORY_WEIGHT * cat_sim)        // 0.2 × category match
                + (LOCATION_WEIGHT * loc_sim);       // 0.1 × location match
```

If `combined ≥ 0.75` (DUPLICATE_THRESHOLD), the complaint is flagged as **DUPLICATE**.

---

### 5.4 djb2 Hash Function

**Concept:** A hash function maps arbitrary-length keys to fixed-size integer indices. The **djb2** algorithm (by Dan Bernstein) is a simple, fast, and effective string hash function.

**File:** `hash_table.c` → `ht_hash()`

**Formula:** `hash = hash * 33 + c` for each character c, starting with hash = 5381.

```c
unsigned long ht_hash(const char* key, int table_size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash % table_size;
}
```

**Why 5381?** It's an odd prime that empirically produces good distribution with the multiplication factor of 33.

**Why `hash << 5 + hash`?** This is equivalent to `hash * 32 + hash = hash * 33`, but using a bit shift is faster than multiplication.

---

### 5.5 NLP Keyword-Based Severity Analysis

**Concept:** A simplified Natural Language Processing (NLP) approach that analyzes complaint text to automatically compute a **severity score from 1 to 10**.

**File:** `nlp_analyzer.c` → `analyze_severity()`

**How it works:**

```
Step 1: Combine title + description text
Step 2: Tokenize into lowercase words
Step 3: For each word:
   - If it's a NEGATION word ("not", "never") → set negate_next flag
   - If it's an AMPLIFIER word ("very", "extremely") → set amplify_next flag
   - If it's a KEYWORD in the dictionary → get its weight
       - Apply amplifier: weight × 1.5
       - Apply negation: weight × 0.3
       - Add to total score
Step 4: Compute average score per keyword hit
Step 5: Add bonus for multiple keyword hits:
   - 5+ hits → +2.0, 3-4 hits → +1.0, 2 hits → +0.5
Step 6: Clamp result to range [1, 10]
```

**Keyword Weight Categories:**
| Category | Weight | Example Keywords |
|----------|--------|-----------------|
| Critical | 8-10 | fire, collapse, electrocution, flood, explosion, death |
| Urgent | 5-7 | broken, sewage, blocked, overflow, burst, blackout |
| Moderate | 3-5 | damaged, leaking, pothole, garbage, noise, dirty |
| Low | 1-2 | request, suggestion, minor, cosmetic, paint, clean |

**Example:**
```
Input: "Very dangerous exposed electrical wires sparking near school"

Tokens: [very, dangerous, exposed, electrical, wires, sparking, near, school]

- "very"      → AMPLIFIER (amplify_next = 1)
- "dangerous" → KEYWORD weight=8, amplified: 8 × 1.5 = 12.0
- "exposed"   → KEYWORD weight=6
- "electrical" → not in keyword_dict (skip)
- "wires"     → KEYWORD weight=5
- "sparking"  → KEYWORD weight=7
- "near"      → not in dict
- "school"    → not in dict

total_score = 12.0 + 6 + 5 + 7 = 30.0
keyword_hits = 4
avg_score = 30.0 / 4 = 7.5
hit_bonus = 1.0 (3-4 hits)
final = 7.5 + 1.0 = 8.5 → round to 9

Severity: 9 (CRITICAL)
```

---

### 5.6 Heapify (Up & Down)

These are the core operations that maintain the **max-heap property**: every parent's severity ≥ its children's severity.

**Heapify Up (Bubble Up):** Used after **inserting** a new element at the bottom of the heap.
```
Insert severity 9 at the end:
        10                 10
       / \                / \
      8    6    →        9    6
     / \                / \
    5    9             5    8
         ↑ swap with parent
```

**Heapify Down (Sink Down):** Used after **extracting** the max (root) — the last element is moved to root and sunk down.
```
Extract 10, move 5 to root:
         5                  9                  9
        / \                / \                / \
       9    6    →        5    6    →        8    6
      / \                / \                /
     8    3             8    3             5
```

---

### 5.7 Tokenization

**Concept:** Splitting raw text into individual **tokens** (words), converting to lowercase, and filtering by length.

**File:** `nlp_analyzer.c` → `tokenize()`

```c
int tokenize(const char* text, char tokens[][MAX_TOKEN_LEN], int max_tokens) {
    // Scans text character by character
    // Splits on any non-alphanumeric character (spaces, punctuation, etc.)
    // Converts each token to lowercase
    // Returns the number of tokens extracted
}
```

**Used by:** `analyze_severity()`, `detect_irrelevant()`, `suggest_category()`, `jaccard_similarity()`

---

### 5.8 Spam / Irrelevance Detection

**File:** `nlp_analyzer.c` → `detect_irrelevant()`

**Checks performed:**
1. **Too short** — description < 10 characters AND title < 10 characters
2. **Gibberish detection** — more than 6 consecutive consonants (e.g., "xyzqrtp")
3. **Low vowel ratio** — if vowels < 20% of alphabetic characters
4. **Repeated characters** — more than 5 consecutive identical characters (e.g., "aaaaaaa")
5. **Too few words** — fewer than 3 meaningful words

If any check fails, the complaint is flagged as `is_irrelevant = 1`.

---

## 6. Object-Oriented Programming Concepts (C++)

### 6.1 Classes & Objects

**Concept:** A **class** is a blueprint for creating objects. It bundles **data** (member variables) and **behavior** (member functions/methods) together.

**Classes in the project:**

| Class | File | Purpose |
|-------|------|---------|
| `Person` | Person.h/cpp | Abstract base class for all user roles |
| `User` | User.h/cpp | Regular citizen — submits and tracks complaints |
| `Admin` | Admin.h/cpp | System administrator — reviews, escalates, assigns |
| `Worker` | Worker.h/cpp | Field worker — views tasks, gets routes, completes work |
| `DepartmentOfficer` | DepartmentOfficer.h/cpp | Department manager — assigns workers to complaints |
| `AuthManager` | AuthManager.h/cpp | Static authentication utility class |
| `Dashboard` | Dashboard.h/cpp | Static console UI utility class |
| `MenuSystem` | MenuSystem.h/cpp | Main application controller |

---

### 6.2 Inheritance

**Concept:** Inheritance allows a class (child/derived) to **inherit** properties and methods from another class (parent/base). This promotes **code reuse**.

**Inheritance hierarchy:**
```
              Person (Abstract Base)
             /    |        \        \
          User   Admin   Worker   DepartmentOfficer
```

```cpp
class User : public Person {             // User inherits from Person
    void showDashboard() override;        // Implements pure virtual
    void showMenu() override;             // Implements pure virtual
    void submitComplaint();               // User-specific method
};
```

All four derived classes inherit:
- Common data: `id`, `username`, `fullName`, `role`, `department`, etc.
- Common methods: `getId()`, `getUsername()`, `getRole()`, `toUserRecord()`, etc.

Each adds its own specialized behavior (submit complaints, review complaints, view routes, assign workers).

---

### 6.3 Polymorphism (Virtual Functions)

**Concept:** **Polymorphism** allows a **base class pointer** to call the **correct derived class method** at runtime. This is achieved through **virtual functions** and **vtable dispatch**.

**Declaration in Person.h:**
```cpp
class Person {
public:
    virtual void showDashboard() = 0;  // Pure virtual
    virtual void showMenu() = 0;       // Pure virtual
    virtual ~Person() = default;       // Virtual destructor
};
```

**Usage in AuthManager.cpp (Factory Pattern):**
```cpp
Person* AuthManager::login(const std::string& username, const std::string& password) {
    // Returns User*, Admin*, Worker*, or DepartmentOfficer*
    // All stored as Person*
    if (strcmp(record.role, "ADMIN") == 0)
        return new Admin(record);
    else if (strcmp(record.role, "WORKER") == 0)
        return new Worker(record);
    // ...
}
```

**Usage in MenuSystem (polymorphic dispatch):**
```cpp
Person* currentUser = AuthManager::login(username, password);
currentUser->showDashboard();  // Calls User::showDashboard() or Admin::showDashboard()
currentUser->showMenu();       // Calls the correct role's menu
```

The same code `currentUser->showDashboard()` behaves **differently** depending on the actual runtime type — this is **runtime polymorphism**.

---

### 6.4 Abstraction (Abstract Base Class)

**Concept:** An **abstract class** cannot be instantiated directly. It defines an interface (pure virtual functions) that derived classes **must** implement.

```cpp
class Person {
public:
    virtual void showDashboard() = 0;  // = 0 makes it PURE virtual
    virtual void showMenu() = 0;       // Must be overridden by subclasses
};

// Person p;                     // ❌ ERROR: Cannot instantiate abstract class
// User u(record);               // ✅ OK: User implements all pure virtuals
```

**Purpose:** Forces all user roles to have a `showDashboard()` and `showMenu()` method, ensuring a consistent interface while allowing each role to implement its own behavior.

---

### 6.5 Encapsulation

**Concept:** **Encapsulation** hides internal data behind access modifiers (`private`, `protected`, `public`) and exposes only necessary interfaces through **getters/setters**.

```cpp
class Person {
protected:    // Accessible by this class and derived classes only
    int id;
    std::string username;
    std::string password;
    std::string fullName;
    // ...

public:       // Accessible by everyone
    int getId() const { return id; }
    std::string getUsername() const { return username; }
    void setDepartment(const std::string& dept) { department = dept; }
};
```

- **`protected`** members are hidden from outside code but accessible to child classes (User, Admin, etc.).
- **Getters** provide read-only access to private data.
- **Setters** allow controlled modification.

---

### 6.6 Constructor Overloading

**Concept:** A class can have **multiple constructors** with different parameter lists. The compiler chooses the correct one based on the arguments provided.

```cpp
class Person {
public:
    // Constructor 1: Manual initialization
    Person(int id, const std::string& username, const std::string& fullName,
           const std::string& role);
    
    // Constructor 2: Initialize from C struct
    Person(const UserRecord& record);
};
```

The second constructor demonstrates **C/C++ interop** — it converts a C `UserRecord` struct into a C++ `Person` object by copying each field.

---

## 7. Design Patterns

### 7.1 Factory Pattern

**Concept:** The **Factory Pattern** creates objects without specifying the exact class. A factory method returns a base class pointer, and the concrete type is determined at runtime.

**Implementation in `AuthManager::login()`:**
```cpp
Person* AuthManager::login(const std::string& username, const std::string& password) {
    UserRecord record;
    authenticate_user(username.c_str(), password.c_str(), FILE_USERS, &record);
    
    // Factory decision based on role
    if (strcmp(record.role, "ADMIN") == 0)
        return new Admin(record);           // Creates Admin object
    else if (strcmp(record.role, "DEPT_OFFICER") == 0)
        return new DepartmentOfficer(record); // Creates DeptOfficer object
    else if (strcmp(record.role, "WORKER") == 0)
        return new Worker(record);          // Creates Worker object
    else
        return new User(record);            // Creates User object
}
```

**Why use it?**
- The caller (`MenuSystem`) doesn't need to know which specific class was created.
- Adding a new role only requires adding a new `else if` branch — the rest of the code is unchanged.

---

### 7.2 Layered Architecture Pattern

The project separates concerns into two layers:
- **Engine Layer (C):** Pure data processing — no UI, no user interaction.
- **Interface Layer (C++):** All user interaction — menus, input/output, display.

This means the engine can be reused with a different UI (e.g., a GUI) without any changes.

---

### 7.3 MVC-like Separation

| MVC Component | Project Equivalent |
|--------------|-------------------|
| **Model** | C structs (`Complaint`, `UserRecord`), file storage (`file_manager.c`) |
| **View** | `Dashboard` class (all console output, colors, tables) |
| **Controller** | `MenuSystem`, `AuthManager`, role-specific classes (User, Admin, etc.) |

---

## 8. File I/O & Persistent Storage

**Concept:** All data is persisted to **pipe-delimited `.txt` files** using C's `stdio.h` functions (`fopen`, `fprintf`, `fgets`, `sscanf`, `fclose`).

**Data Files:**

| File | Format | Contents |
|------|--------|----------|
| `data/users.txt` | `id\|username\|password\|name\|phone\|email\|role\|date\|dept\|loc\|avail` | All user accounts |
| `data/complaints.txt` | `id\|uid\|title\|desc\|cat\|loc\|locid\|sev\|nlp\|dup\|dupof\|irrel\|status\|dept\|wid\|sdate\|cdate` | All complaints |
| `data/departments.txt` | `id\|name\|officer_username\|complaint_count\|resolved_count` | Departments |
| `data/workers.txt` | Worker records | Field workers |
| `data/logs.txt` | `[datetime] message` | Activity log |

**Key Operations:**
| Operation | Function | Description |
|-----------|----------|-------------|
| Save (append) | `save_user()`, `save_complaint()` | Appends a new record to end of file |
| Load (read all) | `load_users()`, `load_complaints()` | Reads all records into an array |
| Update (rewrite) | `update_user()`, `update_complaint()` | Loads all, modifies one, rewrites entire file |
| Filter | `load_complaints_by_user()`, `load_complaints_by_dept()` | Load all, filter by field |
| Auto-ID | `get_next_id()` | Scans file for max ID, returns max+1 |

**Data Seeding:** On first run, `init_data_files()` creates:
- 1 admin account (`admin` / `admin123`)
- 5 department officer accounts
- 10 worker accounts (2 per department)
- 5 departments (Water Supply, Electricity, Roads, Sanitation, Public Safety)

---

## 9. Build System (Makefile)

**Concept:** A `Makefile` automates the compilation and linking process. The `make` command reads it and compiles only what has changed.

**Key details of this project's Makefile:**

```makefile
CC = gcc           # C compiler for engine (.c files)
CXX = g++          # C++ compiler for interface (.cpp files)
CFLAGS = -Wall -Wextra -O2
CXXFLAGS = -Wall -Wextra -O2 -std=c++11
```

**Compilation flow:**
```
src/engine/*.c   → gcc → obj/*.o     (C object files)
src/interface/*.cpp → g++ → obj/*.o  (C++ object files)
main.cpp         → g++ → obj/main.o

All .o files → g++ (linker) → bin/complaint_analyzer.exe
```

**Why link with g++ (not gcc)?** Because the project uses C++ features (classes, `std::string`, `new`/`delete`), the linker must be the C++ compiler to include the C++ standard library.

---

## 10. Memory Management

The project uses both **static and dynamic memory allocation**:

| Type | Where | Mechanism |
|------|-------|-----------|
| Static arrays | `Complaint complaints[500]` | Stack-allocated, fixed size |
| Dynamic heap | `PriorityQueue.complaints` | `malloc()` / `realloc()` / `free()` |
| Dynamic nodes | `HashNode` entries | `malloc()` / `free()` |
| Polymorphic objects | `new Admin(record)` | C++ `new` / `delete` |

**Proper cleanup:**
- `ht_free()` — walks all buckets and frees all linked list nodes, then frees the bucket array.
- `pq_free()` — frees the dynamic complaint array.
- `MenuSystem::~MenuSystem()` — deletes the current `Person*` object (virtual destructor ensures correct cleanup).

---

## 11. Preprocessor Directives & Include Guards

**Include Guards** prevent a header file from being included multiple times (which would cause redefinition errors):
```c
#ifndef HASH_TABLE_H    // If not defined...
#define HASH_TABLE_H    // Define it

// ... declarations ...

#endif                   // End guard
```

**Macro Constants** define project-wide configuration:
```c
#define MAX_COMPLAINTS      500    // Maximum number of complaints in memory
#define MAX_LOCATIONS       20     // Maximum graph nodes
#define INF                 999999 // Infinity for Dijkstra
#define DUPLICATE_THRESHOLD 0.75   // Similarity threshold for duplicate detection
#define HT_DEFAULT_SIZE     127    // Hash table bucket count (prime)
#define AMPLIFIER_MULT      1.5    // NLP amplifier multiplier
#define NEGATION_MULT       0.3    // NLP negation multiplier
```

---

## 12. Role-Based Access Control (RBAC)

Each user has one of four roles, and the system **restricts functionality** based on that role:

| Role | Can Do | Cannot Do |
|------|--------|-----------|
| **USER** | Submit complaints, view own complaints, view profile | Review others' complaints, assign workers |
| **ADMIN** | Review all complaints, escalate severity, assign to departments, view statistics, manage departments, manage workers, view city map | Submit complaints |
| **DEPT_OFFICER** | View department complaints, assign workers, view worker status | View other departments, escalate severity |
| **WORKER** | View assigned tasks, get optimized route (Dijkstra), mark tasks complete, update location | Assign complaints, review |

The `AuthManager::login()` factory method ensures the correct class is instantiated based on the role, and polymorphism ensures the correct menu/dashboard is shown.

---

## 13. Complete File Structure Map

```
smart-complaint-analyzer/
├── main.cpp                          ← Entry point (creates MenuSystem, calls run())
├── Makefile                          ← Build automation
│
├── include/
│   ├── engine/                       ← C header files
│   │   ├── complaint_types.h         ← All structs, constants, typedefs
│   │   ├── hash_table.h             ← Hash table with chaining
│   │   ├── priority_queue.h         ← Max-heap priority queue
│   │   ├── nlp_analyzer.h           ← NLP severity analysis
│   │   ├── duplicate_detector.h     ← Levenshtein + Jaccard duplicate detection
│   │   ├── file_manager.h           ← File I/O (CRUD operations)
│   │   └── route_optimizer.h        ← Graph + Dijkstra's algorithm
│   │
│   └── interface/                    ← C++ header files
│       ├── Person.h                  ← Abstract base class
│       ├── User.h                    ← User subclass
│       ├── Admin.h                   ← Admin subclass
│       ├── Worker.h                  ← Worker subclass
│       ├── DepartmentOfficer.h       ← DeptOfficer subclass
│       ├── AuthManager.h            ← Authentication (factory pattern)
│       ├── Dashboard.h              ← Console UI utilities
│       └── MenuSystem.h             ← Main app loop
│
├── src/
│   ├── engine/                       ← C implementation files
│   │   ├── hash_table.c             ← djb2 hashing, chaining, CRUD
│   │   ├── priority_queue.c         ← Heap insert/extract/heapify
│   │   ├── nlp_analyzer.c           ← 120+ keywords, tokenizer, scorer
│   │   ├── duplicate_detector.c     ← Levenshtein, Jaccard, weighted combo
│   │   ├── file_manager.c           ← All file I/O + data seeding
│   │   └── route_optimizer.c        ← Dijkstra, 18-node Dehradun graph
│   │
│   └── interface/                    ← C++ implementation files
│       ├── Person.cpp                ← Base class constructors
│       ├── User.cpp                  ← Submit, view, track complaints
│       ├── Admin.cpp                 ← Review, escalate, assign, stats
│       ├── Worker.cpp                ← Tasks, routes, completion
│       ├── DepartmentOfficer.cpp     ← Dept management, worker assignment
│       ├── AuthManager.cpp           ← Login/signup factory
│       ├── Dashboard.cpp             ← ANSI colors, tables, progress bars
│       └── MenuSystem.cpp            ← Main loop, routing
│
├── data/                             ← Persistent storage
│   ├── users.txt                     ← Pipe-delimited user records
│   ├── complaints.txt                ← Pipe-delimited complaint records
│   ├── departments.txt               ← Department data
│   ├── workers.txt                   ← Worker data
│   └── logs.txt                      ← Activity log
│
├── obj/                              ← Compiled object files (.o)
└── bin/                              ← Final executable
    └── complaint_analyzer.exe
```

---

## 14. How Everything Connects — End-to-End Flow

### Flow 1: User Submits a Complaint

```
User types complaint text
        │
        ▼
┌─────────────────────────────┐
│  1. NLP Analyzer             │
│  - Tokenize text            │
│  - Look up keywords in      │
│    hash table (O(1) each)   │
│  - Apply amplifiers/negation │
│  - Compute severity (1-10)  │
│  - Suggest category         │
│  - Detect spam/irrelevant   │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  2. Duplicate Detector       │
│  - Load existing complaints │
│  - For each existing:       │
│    • Levenshtein similarity │
│    • Jaccard similarity     │
│    • Category match         │
│    • Location match         │
│  - Weighted combination     │
│  - If ≥ 0.75 → DUPLICATE   │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  3. File Manager             │
│  - Auto-assign complaint ID │
│  - Save to complaints.txt   │
│  - Log activity to logs.txt │
└──────────┬──────────────────┘
           │
           ▼
     Complaint saved with
     auto-computed severity,
     suggested category, and
     duplicate/spam flags.
```

### Flow 2: Admin Reviews & Assigns

```
Admin logs in (Factory Pattern → new Admin(record))
        │
        ▼
┌─────────────────────────────┐
│  1. Priority Queue           │
│  - Load all PENDING          │
│    complaints into max-heap  │
│  - Display highest severity  │
│    first (extract_max)       │
└──────────┬──────────────────┘
           │
           ▼
Admin reviews complaint, optionally overrides severity
           │
           ▼
Admin assigns to department → update_complaint() → rewrite file
```

### Flow 3: Worker Gets Optimized Route

```
Worker logs in → views assigned complaints
        │
        ▼
┌─────────────────────────────┐
│  1. Route Optimizer          │
│  - Load city graph (18      │
│    Dehradun locations)       │
│  - Worker's current location │
│    = source node             │
│  - Complaint location        │
│    = destination node        │
│  - Run Dijkstra's algorithm  │
│  - Reconstruct shortest path │
│  - Display route with        │
│    distances                 │
└──────────┬──────────────────┘
           │
           ▼
Worker follows route, completes task, marks as COMPLETED
```

---

## 📚 Summary of All Concepts

| # | Concept | Where Used | Purpose |
|---|---------|------------|---------|
| 1 | **Structs** | `complaint_types.h` | Group related data (complaints, users, locations) |
| 2 | **Hash Table** | `hash_table.c` | O(1) keyword lookups for NLP |
| 3 | **Linked List** | `hash_table.c` (chaining) | Resolve hash collisions |
| 4 | **Priority Queue (Max-Heap)** | `priority_queue.c` | Serve highest-severity complaints first |
| 5 | **Graph (Adjacency Matrix)** | `route_optimizer.c` | Model city road network |
| 6 | **Dijkstra's Algorithm** | `route_optimizer.c` | Find shortest path for workers |
| 7 | **Levenshtein Distance (DP)** | `duplicate_detector.c` | Measure text edit distance |
| 8 | **Jaccard Similarity** | `duplicate_detector.c` | Measure word-set overlap |
| 9 | **djb2 Hash Function** | `hash_table.c` | Convert strings to array indices |
| 10 | **Tokenization** | `nlp_analyzer.c` | Split text into words |
| 11 | **NLP Severity Scoring** | `nlp_analyzer.c` | Auto-rate complaint urgency |
| 12 | **Spam Detection** | `nlp_analyzer.c` | Filter gibberish/irrelevant |
| 13 | **Classes & Objects** | All interface files | OOP blueprint + instances |
| 14 | **Inheritance** | Person → User/Admin/Worker/DeptOfficer | Code reuse |
| 15 | **Polymorphism** | `showDashboard()`, `showMenu()` | Runtime method dispatch |
| 16 | **Abstraction** | `Person` abstract class | Force interface contract |
| 17 | **Encapsulation** | `protected`/`public` in Person | Data hiding |
| 18 | **Constructor Overloading** | `Person(id, ...)` vs `Person(UserRecord&)` | Multiple init paths |
| 19 | **Factory Pattern** | `AuthManager::login()` | Create correct subclass at runtime |
| 20 | **File I/O** | `file_manager.c` | Persistent pipe-delimited storage |
| 21 | **extern "C"** | All headers, Person.h | C/C++ interoperability |
| 22 | **Makefile** | `Makefile` | Build automation with gcc/g++ |
| 23 | **Dynamic Memory** | `malloc`, `realloc`, `new` | Heap allocation |
| 24 | **Include Guards** | `#ifndef ... #endif` | Prevent double inclusion |
| 25 | **Preprocessor Macros** | `#define` constants | Configuration constants |
| 26 | **RBAC** | AuthManager + class hierarchy | Role-based feature access |
| 27 | **ANSI Color Codes** | `Dashboard.h` | Rich console UI |
| 28 | **Static Methods** | `Dashboard`, `AuthManager` | Utility functions without instances |
| 29 | **Virtual Destructor** | `Person::~Person()` | Proper polymorphic cleanup |
| 30 | **Heapify (Up/Down)** | `priority_queue.c` | Maintain heap invariant |

---

> **Document Version:** 1.0  
> **Project:** Smart Public Complaint Analyzer  
> **Languages:** C (Engine) + C++ (Interface)  
> **Author:** Auto-Generated Documentation
