# Smart Public Complaint Analyzer

**Documentation reviewed:** 2026-11-01

Smart Public Complaint Analyzer is a C/C++ application for submitting and managing civic complaints. A C engine handles file storage, text analysis, duplicate detection, complaint prioritization, route calculation and the complaint workflow. Two interfaces use that same engine and the same data files:

- a **C++ console app** with sign-in and role-specific menus, and
- a **website** (`web/`) with the same roles, live analysis, dashboards and a city map.

## About the topic

Public complaint analysis is the process of collecting reports from residents about local issues, such as water outages, unsafe roads, or sanitation problems, and organizing them so the responsible teams can respond. When many complaints arrive, staff may have difficulty spotting urgent cases, recognizing repeated reports about the same issue, and deciding which department should handle them.

This project demonstrates how a civic complaint system can help residents submit and track issues while helping administrators and department staff review, prioritize, and assign work. Text-based severity and category suggestions, duplicate detection, complaint statistics, and worker route planning can make that workflow more organized and help staff focus attention on urgent or frequently reported problems. These features are decision-support aids: staff should review the results, and the application does not guarantee that a complaint will be resolved or that its analysis is correct.

## Contents

- [About the topic](#about-the-topic)
- [Capabilities](#capabilities)
- [Roles](#roles)
- [Technical design](#technical-design)
- [Concepts and technologies used](#concepts-and-technologies-used)
- [Build and run](#build-and-run)
- [Using the application](#using-the-application)
- [Website](#website)
- [Data files](#data-files)
- [Dataset generator](#dataset-generator)
- [Repairing and re-analysing data](#repairing-and-re-analysing-data)
- [Tests](#tests)
- [Limits and security notes](#limits-and-security-notes)
- [Project layout](#project-layout)
- [History and dates](#history-and-dates)

## Capabilities

- Register citizen accounts and authenticate users by username and password.
- Submit complaints with a title, description, category, and Dehradun location.
- Analyze complaint text with keyword-based rules to:
  - estimate severity on a 1–10 scale, including negation and amplifier handling and simple word forms ("accidents" matches "accident", "damaged" matches "damage");
  - suggest one of five complaint categories; and
  - flag potentially irrelevant or spam-like submissions.
- Compare new complaints with existing complaints to identify likely duplicates. Similarity uses Jaccard word overlap, normalized Levenshtein similarity, category, and location. Generic stop words and title subjects are considered to reduce false matches; the matching threshold varies with user and location.
- Review pending complaints in descending severity order using a max-heap priority queue.
- Assign complaints to departments and workers, track complaint status, and record completion. The workflow order is enforced for every interface: an admin reviews and routes a complaint to a department, the department officer then assigns an available worker, and the worker starts the task before completing it.
- Let field workers switch between *available* and *busy*; officers can assign new tasks only to available workers.
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
| `DEPT_OFFICER` | View complaints and statistics for the officer's department, assign department workers to complaints an admin has routed there, and view worker status and load. |
| `WORKER` | View assigned tasks, start a task with an optimized route, mark started tasks complete, update location, switch between available and busy, and view a profile. |

New registrations create `USER` accounts. The account role determines which C++ subclass and menu are created at login.

## Technical design

- **Language/toolchain:** C engine and C++11 interface, built with GCC/G++ and GNU Make.
- **Engine (`src/engine/`):**
  - `file_manager.c` — pipe-delimited persistence, record queries and updates, statistics, initialization, and activity logging. Every change holds a lock on `data/.lock` for its whole read-modify-write, so the console app and the website's `engine_cli` processes can write at the same time without losing or duplicating records.
  - `nlp_analyzer.c` — lowercase tokenization, keyword dictionaries, severity analysis, category suggestions, and irrelevance detection.
  - `duplicate_detector.c` — Jaccard and Levenshtein similarity with category/location weighting and duplicate thresholds.
  - `priority_queue.c` — dynamically sized max-heap for severity-ordered complaints.
  - `route_optimizer.c` — 18-node Dehradun graph and Dijkstra shortest-path search.
  - `hash_table.c` — separate-chaining hash table used for keyword lookup.
  - `workflow.c` — the complaint workflow rules (submit, review, escalate, assign department, assign worker, start, complete, change location, register). Both interfaces call these functions, so they validate and log every action the same way.
- **Interface (`src/interface/`):** C++ classes for users, administrators, department officers, field workers, authentication, dashboards, and menu flow.
- **Web bridge (`src/cli/engine_cli.c`):** a small command-line program that exposes the engine to the website. It prints one JSON object per command.
- **Website (`web/`):** `server.py` (Python standard library only) serves the single-page frontend in `web/static/` and the JSON API described in `web/API.md`. It reads the data files directly, and sends every analysis and every write through `bin/engine_cli`.
- **Entry point:** `main.cpp` initializes the menu controller. On Windows it enables terminal color support and sets the working directory to the project root based on the executable location.
- **Limits:** Core capacities and text lengths are defined in `include/engine/complaint_types.h`; current limits include 2,000 complaints, 200 user records, 10 departments, 50 workers, and 18 graph locations.

The NLP component is deterministic and dictionary/rule based; it does not call an external NLP service or load a trained machine-learning model.

## Concepts and technologies used

A brief list of what the program uses, grouped by area. The [Technical design](#technical-design) section and the files named here have the details.

| Area | What is used | Where |
|---|---|---|
| Languages | C (engine), C++11 (console app), Python 3 (website server and tools), JavaScript, HTML and CSS (website) | `src/`, `web/`, `tools/` |
| Data structures | Hash table with separate chaining and the djb2 string hash; max-heap priority queue in an array that grows with `realloc`; weighted graph stored as an adjacency matrix; C structs for complaints, users and departments | `hash_table.c`, `priority_queue.c`, `route_optimizer.c`, `complaint_types.h` |
| Algorithms | Dijkstra's shortest path (array version, O(V²)) with path reconstruction; Levenshtein edit distance (dynamic programming with two rows); Jaccard set similarity; heap insert and extract (heapify up/down), older complaint first on equal severity | `route_optimizer.c`, `duplicate_detector.c`, `priority_queue.c` |
| Rule-based text analysis | Lowercase tokenization; weighted keyword dictionaries; negation and amplifier handling; suffix rules for word forms; stop-word filtering; spam/irrelevance rules | `nlp_analyzer.c`, `duplicate_detector.c` |
| Object-oriented programming (C++) | Abstract base class `Person` with pure virtual functions; inheritance (`User`, `Admin`, `Worker`, `DepartmentOfficer`); runtime polymorphism (login returns a `Person*` of the account's role); encapsulation with `protected`/`private` members; static helper class `Dashboard`; `std::string` and `std::vector` | `include/interface/`, `src/interface/` |
| Mixing C and C++ | `extern "C"` blocks in the engine headers, so the C++ code links to the C functions | `include/engine/*.h` |
| File handling | Pipe-delimited text records; each change is written to a temporary file that then replaces the original | `file_manager.c` |
| Concurrency | Cross-process lock on `data/.lock` (Windows `CreateFileA` share mode, `flock` on Linux/macOS); multi-threaded web server; stress test with many parallel processes | `file_manager.c`, `server.py`, `tools/stress_test.py` |
| Workflow (state machine) | Fixed status order `PENDING` → `REVIEWED` → `ASSIGNED` → `IN_PROGRESS` → `COMPLETED`, checked the same way for every role and both interfaces | `workflow.c` |
| Windows API | `GetModuleFileNameA` and `SetCurrentDirectoryA` (find the project root), `SetConsoleMode` and `SetConsoleOutputCP` (colours and UTF-8 output), `MoveFileExA` (replace a file) | `main.cpp`, `engine_cli.c`, `file_manager.c` |
| Console interface | ANSI escape codes for colour; box-drawing characters; input read one line at a time | `Dashboard.cpp`, `MenuSystem.cpp` |
| Communication between programs | The website starts `engine_cli` for each request: input fields on stdin, one JSON object on stdout | `engine_cli.c`, `server.py` |
| Web server (Python standard library only) | `http.server` with `socketserver.ThreadingMixIn`; `subprocess`; `json`; session cookies (`secrets`, `HttpOnly`, `SameSite=Strict`); login throttling; `hmac.compare_digest` for password checks; Content-Security-Policy header | `web/server.py` |
| Web frontend (no external libraries) | JavaScript modules; hash-based page routing; `fetch` API; SVG charts and city map drawn in code; light and dark themes (`prefers-color-scheme`, choice saved in `localStorage`) | `web/static/` |
| Build | GNU Make with pattern rules and header dependencies; GCC/G++ with `-Wall -Wextra -O2` and `-std=c++11`; 8 MB stack on Windows | `Makefile` |
| Testing and evaluation | Duplicate-detection test program; NLP evaluation (mean absolute error, Spearman correlation, category accuracy); concurrency stress test | `test_duplicate.c`, `tools/eval_nlp.py`, `tools/eval_nlp.c`, `tools/stress_test.py` |
| Data tools | Synthetic dataset generator with a fixed random seed; data repair script that backs up `data/` first | `generate_dataset.py`, `tools/repair_data.py` |

## Build and run

Install a Windows-compatible GCC/G++ toolchain and GNU Make (for example MinGW with `mingw32-make`), and Python 3 for the website. Run the following from the project root, in cmd, PowerShell or Git Bash:

```powershell
mingw32-make            # builds bin/complaint_analyzer.exe and bin/engine_cli.exe
.\bin\complaint_analyzer.exe
```

If GNU Make is available as `make`, use `make` in place of `mingw32-make`. The Makefile also provides:

```powershell
mingw32-make run        # build and start the console app
mingw32-make web        # build engine_cli and start the website
mingw32-make test       # build and run the duplicate-detection tests
mingw32-make eval       # measure the text analysis against hand-labelled examples
mingw32-make stress     # check that parallel writes lose nothing (uses a copy of data/)
mingw32-make clean      # remove obj/ and the built .exe files (data is kept)
mingw32-make cleandata  # DELETE data/ (users, complaints, departments, logs)
```

**`cleandata` removes the root `data/` directory, including persisted users, complaints, departments, and logs.** Back up any data you need first. `clean` no longer deletes the whole `bin/` folder.

## Using the application

1. Start the executable and choose **Login** or **Sign Up**.
2. Sign in with an account whose role has the required operation.
3. For a new complaint, enter the title and description. The application displays its severity estimate and category suggestion; the category can be accepted or selected from the supported list.
4. Administrators review pending complaints and route them to departments. Department officers then assign an available worker. Workers start a task (which calculates the route), mark it complete once done, update their location, and switch between available and busy.
5. Use the role menu's logout option to return to the main menu.

On first startup, the application creates the root `data/` directory and initializes missing user, department, complaint, and log files. Default accounts are seeded only when `data/users.txt` does not already exist. Existing files are not automatically reset or reseeded.

## Website

```powershell
mingw32-make            # once, to build bin/engine_cli.exe
python web/server.py --open
```

The site opens at <http://localhost:8000/>. Options: `--port 8080`, `--host 0.0.0.0` (other devices on your network), `--verbose` (log each request). Sign in with any account from `data/users.txt`. While the server listens only on 127.0.0.1 (the default), the sign-in page offers one-click demo accounts for each role.

| Role | What the website adds |
|---|---|
| Visitor | Live city-wide numbers, the city map with open complaints per location, and an explanation of how the analysis works |
| Citizen | Report a problem with a **live analysis** as you type (severity 1–10, the keywords behind it with their negation/amplifier effects, suggested category, spam check, and a warning if it duplicates an earlier complaint); track each complaint through the five workflow steps |
| Administrator | City dashboard (status, category, severity, monthly trend, resolution time, locations); the review queue in the C max-heap's order; review, escalate and assign; for a duplicate, a "Why flagged?" breakdown of every similarity component; departments, workers and the activity log |
| Department officer | Department dashboard; assign the nearest available worker (road distances by Dijkstra) and see the route |
| Field worker | Task list, start a task with its shortest route, mark a started task complete, update location, switch between available and busy |

The website and the console app share `data/`, so changes made in one appear in the other. Actions taken on the website, sign-ins included, are marked `[web]` in `data/logs.txt`. The API is documented in `web/API.md`.

Text limits are in bytes, as stored by the C engine (title 99, description 499): a Hindi letter takes 3 bytes, so the form counts bytes and refuses text over the limit instead of cutting it.

## Data files

The application reads and writes data relative to the project root:

| File | Contents |
|---|---|
| `data/users.txt` | User records, including citizen, admin, officer, and worker accounts. Workers are represented as user records with role `WORKER`; there is no separate workers file in the current data format. |
| `data/complaints.txt` | Complaint records. |
| `data/departments.txt` | Department IDs, names, officer usernames, and complaint counters. |
| `data/logs.txt` | Timestamped application activity, such as sign-ins and complaint actions. |
| `data/.lock` | An empty file the programs lock while they change `data/`. Leave it; it is recreated if deleted. |

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

`generate_dataset.py` creates synthetic citizen accounts and complaint records for Dehradun, with a fixed random seed. It prints category, severity, and status counts.

```powershell
python generate_dataset.py                    # 1,000 complaints
python generate_dataset.py --complaints 500
```

- It **replaces `data/complaints.txt`**. It copies `data/` to `_backup_<date-time>/` first.
- Existing accounts are kept. Generated citizens are added only if their username is new, using IDs after the highest existing ID, so running it again never duplicates accounts or reuses an ID.
- A completed complaint always has a completion date after its submission date.
- It does not invent `nlp_score` or duplicate/spam flags. After writing, it runs `bin/engine_cli reanalyze`, so those fields come from the C engine. Build the project first.
- It always writes to the root `data/` folder (the one both interfaces use).

## Repairing and re-analysing data

```powershell
python tools/repair_data.py --dry-run    # show what would change
python tools/repair_data.py              # fix it (backs up data/ first)
.\bin\engine_cli.exe reanalyze --dry-run # preview re-analysis
.\bin\engine_cli.exe reanalyze           # recompute nlp_score, spam and duplicate flags
```

`repair_data.py` fixes structural problems and is safe to run repeatedly:

- it removes a UTF-8 byte-order mark (which hid the first account from the C parser);
- it drops repeated account rows;
- it gives a new ID to an account that shares an ID with another account, and uses the activity log to move that account's complaints with it;
- it renumbers a complaint that reuses an ID;
- it fixes impossible completion dates.

It does not change severities, statuses, categories or text.

`reanalyze` processes complaints in submission-date order and compares each one only with complaints submitted before it, as if they had arrived one at a time. A complaint is never linked to one that was already completed before it was made, and complaints flagged as spam are not checked for duplicates (the same rules as a new submission). It does not change the stored (final) severity, status, category or text, and it keeps any line it cannot read.

## Tests

`test_duplicate.c` is a duplicate-detection test program. It covers:

- differently worded complaints about different objects;
- a re-submission by the same user;
- unrelated categories;
- reports of the same issue at one location (informational).

Run it with:

```powershell
mingw32-make test
```

It exits with a non-zero status if a critical case fails.

`mingw32-make eval` (`tools/eval_nlp.py`) scores the text analysis against the 150 complaint templates in `generate_dataset.py`, whose severities were chosen by hand. Current result:

| Measure | Value |
|---|---|
| Severity: mean absolute error / within ±1 / Spearman | 1.41 / 61% / 0.75 |
| Critical templates (9–10) scored 4 or less | 0 |
| Category suggestion correct | 80% |
| Real complaints flagged as spam | 0 |

The labels and the keyword lists were written by the same people and the templates are also the source of the sample data, so treat these as a consistency check for comparing rule changes, not as accuracy on real complaints. (Scoring by the strongest keyword instead of the average was tried and did worse: error 1.83, Spearman 0.67.)

`mingw32-make stress` (`tools/stress_test.py`) runs hundreds of `engine_cli` processes at once against a temporary copy of `data/` and checks that no account is lost and every submission is saved exactly once with its own ID.

## Limits and security notes

- The persistence layer is plain text rather than a database. Writes go through a temporary file that then replaces the original, under a lock on `data/.lock`, so the console app and the website can run at the same time: changes are applied one after another, and each one re-checks the complaint (for example, a task completed elsewhere cannot be completed twice).
- Passwords are stored and compared as plain text in `data/users.txt`. Do not use real or reused passwords, do not expose real personal information in sample data, and do not put the website on the public internet. With `--host` set to a network address, the demo-account shortcut is hidden, but the passwords are still plain text.
- The file format uses `|` as a field separator. On save, `|` in entered text becomes `/`, and line breaks become spaces, so a record always stays on one line.
- The NLP reads English words only. Text in other scripts (for example Hindi) gets the minimum severity and no category, and may be flagged as possible spam.
- The sample data is synthetic and built from about 145 templates. Many complaints therefore share a title, and the duplicate detector links such pairs even across locations, because their text is almost identical.
- The NLP and duplicate detector are rule-based aids. Their outputs should be reviewed by staff rather than treated as authoritative decisions. A negation ("not", "no") reduces only the next keyword, skipping articles ("not a fire"); "no garbage collection" deliberately keeps "garbage" at full weight, because people describe problems that way.
- The route graph is a fixed model with approximate edge distances, not a live map or traffic service.
- Several paths and data structures have fixed maximum capacities declared in `complaint_types.h`.

## Project layout

```text
.
├── Makefile
├── main.cpp
├── generate_dataset.py
├── test_duplicate.c
├── tools/
│   ├── repair_data.py
│   ├── eval_nlp.py   # + eval_nlp.c: NLP measured against hand-labelled templates
│   └── stress_test.py
├── web/
│   ├── server.py     # website + JSON API (Python standard library)
│   ├── API.md        # API contract
│   └── static/       # single-page frontend (HTML/CSS/JS, no build step)
├── data/
│   ├── complaints.txt
│   ├── departments.txt
│   ├── logs.txt
│   └── users.txt
├── include/
│   ├── engine/       # C data types and engine interfaces
│   └── interface/    # C++ role and UI interfaces
├── src/
│   ├── engine/       # C storage, analysis, queue, routing and workflow modules
│   ├── interface/    # C++ application and role implementations
│   └── cli/          # engine_cli: JSON bridge used by the website
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
| 2026-10-31 | Fixed the stack overflow in admin review (large arrays moved to the heap); BOM, empty-field, `\|`-in-text and ID-collision handling; moved the workflow into the shared `workflow.c`; made duplicate detection use true set Jaccard and count two-letter subjects (for example "AC"); added an age tie-break to the priority queue; added `engine_cli`, the website, `tools/repair_data.py`, a portable Makefile, and the generator fixes. | `src/`, `include/`, `web/`, `tools/`, `Makefile`, `generate_dataset.py` |
| 2026-11-01 | Fixed data loss when the console app and the website write at the same time (a cross-process lock on `data/.lock`; `users.txt` could be reset to the default accounts and complaint IDs repeated); `reanalyze` and the department refresh no longer delete rows; the workflow order is enforced (officers assign only routed complaints, workers complete only started tasks), with an admin-role check; worker available/busy switch; console input read line by line (no crash on long names, no half-saved complaints, no 'General' category); NLP word forms, negation across articles, typographic apostrophes and fewer false spam flags, measured with `tools/eval_nlp.py`; duplicates are not linked to complaints closed earlier, and `reanalyze` runs in date order; website fixes for request handling, Host checks, privacy of other departments' records, the timeline, byte limits and input types; `tools/stress_test.py`. | `src/`, `include/`, `web/`, `tools/`, `Makefile`, `data/` |
