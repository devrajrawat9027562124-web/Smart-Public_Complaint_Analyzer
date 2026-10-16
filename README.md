# Smart Public Complaint Analyzer

**Documentation reviewed:** 2026-10-16

Smart Public Complaint Analyzer is a console-based C/C++ application for submitting and managing civic complaints. It uses a C engine for file storage, text analysis, duplicate detection, complaint prioritization, and route calculation, with a C++ interface for sign-in and role-specific menus.

## About the topic

Public complaint analysis is the process of collecting reports from residents about local issues, such as water outages, unsafe roads, or sanitation problems, and organizing them so the responsible teams can respond. When many complaints arrive, staff may have difficulty spotting urgent cases, recognizing repeated reports about the same issue, and deciding which department should handle them.

This project demonstrates how a civic complaint system can help residents submit and track issues while helping administrators and department staff review, prioritize, and assign work. Text-based severity and category suggestions, duplicate detection, complaint statistics, and worker route planning can make that workflow more organized and help staff focus attention on urgent or frequently reported problems. These features are decision-support aids: staff should review the results, and the application does not guarantee that a complaint will be resolved or that its analysis is correct.

## Contents

- [About the topic](#about-the-topic)
- [Capabilities](#capabilities)
- [Roles](#roles)
- [Technical design](#technical-design)
- [Build and run](#build-and-run)
- [Using the application](#using-the-application)
- [Data files](#data-files)
- [Dataset generator](#dataset-generator)
- [Tests](#tests)
- [Limits and security notes](#limits-and-security-notes)
- [Project layout](#project-layout)
- [History and dates](#history-and-dates)

## Capabilities

- Register citizen accounts and authenticate users by username and password.
- Submit complaints with a title, description, category, and Dehradun location.
- Analyze complaint text with keyword-based rules to:
  - estimate severity on a 1–10 scale, including negation and amplifier handling;
  - suggest one of five complaint categories; and
  - flag potentially irrelevant or spam-like submissions.
- Compare new complaints with existing complaints to identify likely duplicates. Similarity uses Jaccard word overlap, normalized Levenshtein similarity, category, and location. Generic stop words and title subjects are considered to reduce false matches; the matching threshold varies with user and location.
- Review pending complaints in descending severity order using a max-heap priority queue.
- Assign complaints to departments and workers, track complaint status, and record completion.
- View overall and department-level complaint statistics.
- Calculate shortest paths through a built-in, weighted Dehradun road graph using Dijkstra's algorithm.
- Persist users, departments, complaints, and activity logs in pipe-delimited text files.
- Display a role-specific terminal interface with colored status and severity indicators.

The category set is **Water Supply**, **Electricity**, **Roads & Transport**, **Sanitation**, and **Public Safety**. The complaint statuses represented in the application and sample data are `PENDING`, `REVIEWED`, `ASSIGNED`, `IN_PROGRESS`, and `COMPLETED`.

## Roles

| Role | Main operations |
|---|---|
| `USER` | Submit complaints, view and track personal complaints, and view a profile. |
| `ADMIN` | Review pending complaints by priority, view all complaints and statistics, escalate severity, assign complaints to departments, manage departments and workers, and view the city map. |
| `DEPT_OFFICER` | View complaints and statistics for the officer's department, assign department workers, and view worker status. |
| `WORKER` | View assigned tasks, request an optimized route, mark tasks complete, update location, and view a profile. |

New registrations create `USER` accounts. The account role determines which C++ subclass and menu are created at login.

## Technical design

- **Language/toolchain:** C engine and C++11 interface, built with GCC/G++ and GNU Make.
- **Engine (`src/engine/`):**
  - `file_manager.c` — pipe-delimited persistence, record queries and updates, statistics, initialization, and activity logging.
  - `nlp_analyzer.c` — lowercase tokenization, keyword dictionaries, severity analysis, category suggestions, and irrelevance detection.
  - `duplicate_detector.c` — Jaccard and Levenshtein similarity with category/location weighting and duplicate thresholds.
  - `priority_queue.c` — dynamically sized max-heap for severity-ordered complaints.
  - `route_optimizer.c` — 18-node Dehradun graph and Dijkstra shortest-path search.
  - `hash_table.c` — separate-chaining hash table used for keyword lookup.
- **Interface (`src/interface/`):** C++ classes for users, administrators, department officers, field workers, authentication, dashboards, and menu flow.
- **Entry point:** `main.cpp` initializes the menu controller. On Windows it enables terminal color support and sets the working directory to the project root based on the executable location.
- **Limits:** Core capacities and text lengths are defined in `include/engine/complaint_types.h`; current limits include 2,000 complaints, 200 user records, 10 departments, 50 workers, and 18 graph locations.

The NLP component is deterministic and dictionary/rule based; it does not call an external NLP service or load a trained machine-learning model.

## Build and run

Install a Windows-compatible GCC/G++ toolchain and GNU Make (for example, MinGW-w64 with `mingw32-make`). Run the following from the project root:

```powershell
mingw32-make
.\bin\complaint_analyzer.exe
```

If GNU Make is available as `make`, use `make` in place of `mingw32-make`. The default build target compiles the C and C++ sources and links `bin/complaint_analyzer.exe`. The Makefile also provides:

```powershell
mingw32-make run
mingw32-make clean
mingw32-make cleandata
```

`run` builds and starts the application. **`clean` removes `obj/` and `bin/`, including files stored under `bin/`. `cleandata` removes the root `data/` directory, including persisted users, complaints, departments, and logs.** Back up any data you need before using either cleanup target.

## Using the application

1. Start the executable and choose **Login** or **Sign Up**.
2. Sign in with an account whose role has the required operation.
3. For a new complaint, enter the title and description. The application displays its severity estimate and category suggestion; the category can be accepted or selected from the supported list.
4. Administrators review pending complaints and route them to departments. Department officers can assign workers. Workers can view tasks, calculate routes, update their location, and mark work complete.
5. Use the role menu's logout option to return to the main menu.

On first startup, the application creates the root `data/` directory and initializes missing user, department, complaint, and log files. Default accounts are seeded only when `data/users.txt` does not already exist. Existing files are not automatically reset or reseeded.

## Data files

The application reads and writes data relative to the project root:

| File | Contents |
|---|---|
| `data/users.txt` | User records, including citizen, admin, officer, and worker accounts. Workers are represented as user records with role `WORKER`; there is no separate workers file in the current data format. |
| `data/complaints.txt` | Complaint records. |
| `data/departments.txt` | Department IDs, names, officer usernames, and complaint counters. |
| `data/logs.txt` | Timestamped application activity, such as sign-ins and complaint actions. |

Records are text rows separated by `|`. Complaint rows contain 17 fields in this order:

1. complaint ID
2. submitting user ID
3. title
4. description
5. category
6. location name
7. location ID
8. final severity
9. NLP severity score
10. duplicate flag
11. ID of the original complaint, if duplicated
12. irrelevant/spam flag
13. status
14. department
15. worker ID, or `-1` if unassigned
16. submission date
17. completion date

The route graph uses these location IDs: Clock Tower (0), Paltan Bazaar (1), Rajpur Road (2), ISBT Dehradun (3), Dehradun Railway Stn (4), Race Course (5), Dalanwala (6), Clement Town (7), Prem Nagar (8), Sahastradhara (9), Ballupur Chowk (10), GMS Road (11), Nagar Nigam (12), Doon Hospital (13), Survey Chowk (14), Rispana Bridge (15), Collectorate (16), and IT Park Sahastradhara (17).

## Dataset generator

`generate_dataset.py` creates synthetic citizen accounts and complaint records for Dehradun. Its source specifies a fixed random seed, 50 generated citizen accounts, and 1,000 generated complaints, and it prints category, severity, and status counts.

Run it from the project root with:

```powershell
python generate_dataset.py
```

**Back up `data/users.txt` and `data/complaints.txt` first.** The generator writes new contents to those files rather than appending to them; running it can replace existing application data. The script chooses the first available `data/` or `bin/data/` directory, which is normally the root `data/` directory in this project.

## Tests

`test_duplicate.c` is a standalone duplicate-detection test program. It covers differently worded complaints about different objects, a re-submission by the same user, unrelated categories, and reports of the same issue at one location. It is not wired into a Makefile test target.

With GCC installed and `bin/` present, compile and run it from the project root:

```powershell
gcc -Wall -Wextra -I. test_duplicate.c src/engine/duplicate_detector.c src/engine/nlp_analyzer.c src/engine/hash_table.c -o bin/test_duplicate.exe
.\bin\test_duplicate.exe
```

## Limits and security notes

- This is a local, interactive console application; the current persistence layer is plain text rather than a database.
- Passwords are stored and compared as plain text in `data/users.txt`. Do not use real or reused passwords, and do not expose real personal information in sample data.
- The file format uses `|` as a field separator. Entered fields containing that delimiter are not escaped by the documented storage format.
- The NLP and duplicate detector are rule-based aids. Their outputs should be reviewed by staff rather than treated as authoritative decisions.
- The route graph is a fixed model with approximate edge distances, not a live map or traffic service.
- Several paths and data structures have fixed maximum capacities declared in `complaint_types.h`.

## Project layout

```text
.
├── Makefile
├── main.cpp
├── generate_dataset.py
├── test_duplicate.c
├── data/
│   ├── complaints.txt
│   ├── departments.txt
│   ├── logs.txt
│   └── users.txt
├── include/
│   ├── engine/       # C data types and engine interfaces
│   └── interface/    # C++ role and UI interfaces
├── src/
│   ├── engine/       # C storage, analysis, queue, and routing modules
│   └── interface/    # C++ application and role implementations
├── obj/              # Build object files
└── bin/              # Built executable and generated build output
```

## History and dates

| Illustrative date | Example contribution | Relevant files |
|---|---|---|
| 2026-09-17 | Organized the project entry point and initial build targets for the mixed C/C++ application. | `main.cpp`, `Makefile` |
| 2026-09-18 | Defined shared complaint, user, department, location, and statistics structures and core capacity limits. | `include/engine/complaint_types.h` |
| 2026-09-19 | Added the hash-table interface and its separate-chaining data structure for keyword lookup. | `include/engine/hash_table.h`, `src/engine/hash_table.c` |
| 2026-09-20 | Implemented hash-table initialization, insertion, search, removal, and cleanup operations. | `src/engine/hash_table.c` |
| 2026-09-21 | Set up file-manager interfaces for user and complaint records, departments, and activity logs. | `include/engine/file_manager.h`, `src/engine/file_manager.c` |
| 2026-09-22 | Added initialization of missing data files and default department and role records. | `src/engine/file_manager.c`, `data/departments.txt` |
| 2026-09-23 | Implemented user record loading, saving, authentication, ID generation, and username checks. | `src/engine/file_manager.c`, `include/engine/file_manager.h` |
| 2026-09-24 | Added the C++ person abstraction and conversion between interface users and engine records. | `include/interface/Person.h`, `src/interface/Person.cpp` |
| 2026-09-25 | Added role-based login construction and citizen account registration checks. | `include/interface/AuthManager.h`, `src/interface/AuthManager.cpp` |
| 2026-09-26 | Created shared console presentation helpers for banners, complaint cards, status, severity, and statistics. | `include/interface/Dashboard.h`, `src/interface/Dashboard.cpp` |
| 2026-09-27 | Connected the top-level application loop to login, signup, logout, and role-specific menus. | `include/interface/MenuSystem.h`, `src/interface/MenuSystem.cpp`, `main.cpp` |
| 2026-09-28 | Added the citizen dashboard and complaint-list and tracking menu flows. | `include/interface/User.h`, `src/interface/User.cpp` |
| 2026-09-29 | Implemented complaint entry and integrated text analysis and category selection into submission. | `src/interface/User.cpp`, `include/engine/nlp_analyzer.h` |
| 2026-09-30 | Added lowercase tokenization and keyword dictionaries for severity scoring. | `src/engine/nlp_analyzer.c`, `include/engine/nlp_analyzer.h` |
| 2026-10-01 | Extended text analysis with negation and amplifier handling and category keyword matching. | `src/engine/nlp_analyzer.c` |
| 2026-10-02 | Added irrelevant-content detection and connected analysis results to complaint submission. | `src/engine/nlp_analyzer.c`, `src/interface/User.cpp` |
| 2026-10-03 | Implemented text similarity primitives and a weighted score for comparing complaints. | `include/engine/duplicate_detector.h`, `src/engine/duplicate_detector.c` |
| 2026-10-04 | Refined duplicate matching with stop-word filtering, title-subject comparison, and user/location-sensitive thresholds. | `src/engine/duplicate_detector.c` |
| 2026-10-05 | Added standalone examples covering likely duplicates and similar-looking, unrelated complaints. | `test_duplicate.c` |
| 2026-10-06 | Implemented the complaint severity max-heap and priority queue operations. | `include/engine/priority_queue.h`, `src/engine/priority_queue.c` |
| 2026-10-07 | Added administrator review of pending complaints in descending severity order. | `include/interface/Admin.h`, `src/interface/Admin.cpp` |
| 2026-10-08 | Added administrator workflows for viewing complaints, escalation, and department assignment. | `src/interface/Admin.cpp`, `src/engine/file_manager.c` |
| 2026-10-09 | Added department-officer views for department complaints and worker assignment. | `include/interface/DepartmentOfficer.h`, `src/interface/DepartmentOfficer.cpp` |
| 2026-10-10 | Implemented the fixed Dehradun location graph and shortest-path calculation. | `include/engine/route_optimizer.h`, `src/engine/route_optimizer.c` |
| 2026-10-11 | Added worker task views, task completion, location updates, and route display integration. | `include/interface/Worker.h`, `src/interface/Worker.cpp` |
| 2026-10-12 | Expanded file-manager query and statistics operations used by user, department, and admin dashboards. | `src/engine/file_manager.c`, `include/engine/file_manager.h` |
| 2026-10-13 | Added a reproducible synthetic-data generator for citizen accounts and Dehradun complaint records. | `generate_dataset.py` |
| 2026-10-14 | Populated and reviewed sample users, complaints, departments, and activity data for local runs. | `data/users.txt`, `data/complaints.txt`, `data/departments.txt`, `data/logs.txt` |
| 2026-10-15 | Consolidated build dependencies and Windows-oriented build, run, and cleanup targets. | `Makefile`, `main.cpp` |
| 2026-10-16 | Documented the current feature set, data formats, setup, test instructions, and operational limitations. | `README.md` |
