# Smart Public Complaint Analyzer

**Team CivicSync (DSCPP-III-2026-T031)**: B.Tech 3rd-semester project-based learning (Data Structures with C and Object-Oriented Programming with C++), Graphic Era.

**README updated:** 2026-11-22

Smart Public Complaint Analyzer lets residents of Dehradun report civic problems, such as water outages, broken roads and garbage, and helps the city staff deal with them. A **C engine** estimates how severe each complaint is, suggests a category, flags spam and duplicates, orders pending complaints in a max-heap, and plans a worker's route with Dijkstra's algorithm. A **C++ console app** with role-based menus uses that engine. A **website** also uses it, through a small command-line bridge, and reads the same data files.

This README covers the whole project: the Phase I proposal, the Phase II prototype and the final Phase III system. The detailed technical documentation of the final code is in [`phase 3 codes/README.md`](phase%203%20codes/README.md).

## Contents

- [Team](#team)
- [The problem](#the-problem)
- [Project journey](#project-journey)
- [How the evaluators' feedback was addressed](#how-the-evaluators-feedback-was-addressed)
- [Final system: features](#final-system-features)
- [Architecture](#architecture)
- [Concepts and technologies](#concepts-and-technologies)
- [Repository layout](#repository-layout)
- [Getting the code](#getting-the-code)
- [Build and run](#build-and-run)
- [Testing and results](#testing-and-results)
- [Known limitations](#known-limitations)
- [Future work](#future-work)
- [Timeline](#timeline)

## Team

| Member | GitHub | Phase II work (from the Phase II report) | Phase III work |
|---|---|---|---|
| Devraj Rawat (team lead) | [devrajrawat9027562124-web](https://github.com/devrajrawat9027562124-web) | Complaint registration, linked-list storage, display and search | C engine: data types, file storage with the cross-process lock, max-heap priority queue, city graph and Dijkstra routing, `engine_cli` bridge, concurrency stress test |
| Stuti Mahajan | [stutimahajan19-tech](https://github.com/stutimahajan19-tech) | Priority queue, priority sorting and priority handling; planned duplicate detection | C engine: hash table, NLP analyzer, duplicate detector, workflow rules, NLP evaluation, duplicate-detection test |
| Rajat | [Rajat-2006](https://github.com/Rajat-2006) | Status updates, department and employee assignment, joining the C modules to the C++ application | C++ console app: `Person` class hierarchy, `AuthManager`, `Dashboard`, `MenuSystem`, `main.cpp` |
| Archi Panwar | [Archi-717](https://github.com/Archi-717) | Website (HTML, CSS, JavaScript, browser storage), tracking page, admin dashboard; C++ module development | Website (`web/`): Python server, JSON API and single-page frontend. *Not uploaded to this repository yet.* |

Shared by the team: `Makefile`, the Phase III `README.md`, `generate_dataset.py` and `tools/repair_data.py`.

## The problem

City offices receive many complaints at once: a broken street light, a pothole, garbage that wasn't collected. When they are handled on paper or in arrival order:

- urgent cases wait behind minor ones;
- the same problem reported by several people is handled several times;
- complaints reach the wrong department; and
- citizens can't see what happened to their complaint.

The project's goal is a single system that stores every complaint and ranks it by urgency. It detects repeats, routes each complaint to the right department and worker, and lets everyone follow its status. Along the way it uses C data structures and C++ object-oriented programming on a real problem.

## Project journey

### Phase I: proposal (September 2026)

Documents: [`phase 1/phase1_report.pdf`](phase%201/phase1_report.pdf), [`phase 1/phase1_ppt.pdf`](phase%201/phase1_ppt.pdf)

We proposed a modular design: **C as the data-structure engine** and **C++ as the object-oriented application layer**. The proposal planned:

- arrays and linked lists for complaint records;
- queues and priority queues for urgency;
- hashing for department mapping;
- BSTs for indexing;
- graphs with BFS and DFS for routing and escalation; and
- C++ classes for users, administrators, departments and employees, with file handling, the STL and exception handling.

Evaluator question: **how are duplicate complaints handled?**

### Phase II: first working version (October 2026)

Code and documents: [`phase 2 codes./`](phase%202%20codes./) (see its [`readme.md`](phase%202%20codes./readme.md)), Phase II report and PPT in the same folder.

- **C module (`complaint_ds.c`):** complaints in a **singly linked list**, a **linked-node priority queue** (High, Medium, Low, first-come first-served within a level), priority sorting, search by ID, filtering and statistics. A timestamped status history is kept in `status_history.txt`.
- **C++ console (`main.cpp`, `Complaint.h`):** citizen and admin menus. It calls the C module through `extern "C"`. Priority was set automatically from words in the description.
- **Website prototype:** `index.html`, `register.html`, `track.html` and `admin.html`, with `script.js` and `style.css`. It stored data in the browser's `localStorage` and rejected exact duplicates (same category, location and text). It was **standalone**: it did not share data with the console app.
- **Phase II testing (from the report):** registration and tracking, the data structures and prioritisation, and admin management all passed.

Evaluator feedback: **improve the terminal output** and **keep the website in sync with the data**.

### Phase III: final system (October–November 2026)

Code: [`phase 3 codes/`](phase%203%20codes/)

The engine was rewritten as separate modules, and both interfaces now share it. The table compares the Phase I plan with what we built:

| Planned in Phase I / built in Phase II | Final Phase III design | Why |
|---|---|---|
| Linked-list complaint storage | Records in text files, loaded into arrays | Every operation re-reads the shared files, so the console app and the website always see the latest data. Arrays also suit the heap. |
| Linked-node priority queue (High/Medium/Low) | **Array max-heap** on a 1–10 severity score, older complaint first on a tie | Insert and extract are O(log n) instead of O(n). The score comes from the text, and an admin can change it. |
| Exact-match duplicates (website only) | **Similarity-based duplicate detection** (Jaccard, Levenshtein, category, location) | It catches reworded repeats and avoids false matches such as "AC not working" and "Fan not working". |
| Hashing for department mapping | **Hash table** (djb2, separate chaining) for the NLP keyword dictionaries | Each of the five categories maps directly to one department. The many keyword lookups needed fast O(1) average search. |
| Graph with BFS/DFS for routing | **Weighted graph** of 18 Dehradun locations, **Dijkstra** shortest path | Roads have different lengths, and BFS ignores edge weights. |
| BST for indexing | Not built | Lookups scan the file. With at most 2,000 complaints, this was fast enough. |
| C++ exception handling and templates | Not used. Errors are passed as result codes (`WfResult`) shared with the C engine. `std::string` and `std::vector` are used. | One error format works for both the console app and the website. |
| Standalone website using `localStorage` | Website on the **same data files**, writing through the **same C engine** | Both interfaces apply the same rules, and a change made in one appears in the other. |

New in Phase III:
- a shared workflow state machine;
- a cross-process file lock;
- live text analysis on the website;
- dashboards and a city map;
- a synthetic dataset generator;
- a data repair tool; and
- measured evaluation and stress tests.

## How the evaluators' feedback was addressed

| Feedback | What we did |
|---|---|
| **Phase I: how are duplicates handled?** | `duplicate_detector.c` compares a new complaint with every earlier open complaint. The score is `(0.4·Jaccard + 0.3·Levenshtein + 0.2·same category + 0.1·same location) × title penalty`, computed after common words are removed. The threshold depends on context: 0.75 for the same user, 0.82 for the same location, 0.88 otherwise. A duplicate is **flagged and linked to the original, not deleted**, and the website shows a "Why flagged?" breakdown of each component. Complaints closed before the new one was made are not matched. |
| **Phase II: improve the terminal output** | A `Dashboard` class gives the console colour-coded severity and status (ANSI colours), box-drawing frames and UTF-8 output on Windows (`SetConsoleMode`, `SetConsoleOutputCP`). Input is read one line at a time, so long or unexpected input doesn't break the menus. |
| **Phase II: sync the website with the data** | The Phase III website reads the same `data/*.txt` files as the console app. It sends every analysis and every change through `bin/engine_cli`, which runs the same C engine and workflow rules. A lock on `data/.lock` lets both run at the same time without losing records. |

## Final system: features

- **Accounts:** citizens sign up and log in. The role decides the menu: citizen, admin, department officer or field worker.
- **Text analysis (rule-based, deterministic):**
  - severity 1–10 from weighted keywords, with negation ("not a fire"), amplifiers ("very") and simple word forms ("accidents" matches "accident");
  - a suggested category out of Water Supply, Electricity, Roads & Transport, Sanitation and Public Safety; and
  - a spam check.
- **Duplicate detection:** described in the table above.
- **Priority review:** pending complaints come out of a max-heap, most severe first.
- **Workflow, enforced for every interface:** `PENDING → REVIEWED → ASSIGNED → IN_PROGRESS → COMPLETED`.
  1. The admin reviews the complaint and routes it to a department.
  2. The department officer assigns an available worker.
  3. The worker starts the task and gets a shortest route, then marks it complete.
- **Routing:** Dijkstra's algorithm on a road graph of 18 Dehradun locations and 32 roads, with approximate distances in km.
- **Statistics:** city-wide and per-department counts, plus website dashboards (status, category, severity, monthly trend, resolution time, locations).
- **Storage:** pipe-delimited text files, written through a temporary file that replaces the original. A cross-process lock is held during each change, and every action is recorded in an activity log.

| Role | Main operations |
|---|---|
| Citizen (`USER`) | Submit a complaint (with live analysis on the website), track own complaints, view profile |
| `ADMIN` | Review the priority queue, escalate severity, route to departments, view all complaints, statistics and the city map, manage departments and workers |
| `DEPT_OFFICER` | See the department's complaints and statistics, assign an available worker (with road distances), see worker load |
| `WORKER` | See assigned tasks, start a task with its shortest route, complete it, update location, switch between available and busy |

## Architecture

```text
 Console app (C++)                          Website (browser, JavaScript)
 main.cpp -> MenuSystem -> Person*          web/static/*  --fetch /api-->  web/server.py (Python)
          |                                                                     | runs bin/engine_cli
          +-----------------------------+---------------------------------------+
                                        |
                          C ENGINE  (src/engine/*.c)
        workflow -> nlp_analyzer, duplicate_detector, priority_queue, route_optimizer, hash_table
                          file_manager  (lock on data/.lock)
                                        |
                          data/*.txt  (pipe-delimited records)
```

`engine_cli` reads free text from standard input, never from shell arguments, and prints one JSON object per command. The website and the console app therefore run the same C code on the same files.

## Concepts and technologies

| Area | What is used | Where (in `phase 3 codes/`) |
|---|---|---|
| Data structures | Hash table with separate chaining and the djb2 hash; max-heap in a growable array (`realloc`); weighted graph as an adjacency matrix; C structs | `hash_table.c`, `priority_queue.c`, `route_optimizer.c`, `complaint_types.h` |
| Algorithms | Dijkstra (array version, O(V²)) with path reconstruction; Levenshtein edit distance (dynamic programming with two rows); Jaccard set similarity; heapify up and down | `route_optimizer.c`, `duplicate_detector.c`, `priority_queue.c` |
| Text analysis | Tokenizing; weighted keyword dictionaries; negation and amplifiers; suffix rules; stop words; spam rules | `nlp_analyzer.c`, `duplicate_detector.c` |
| OOP (C++11) | Abstract base class `Person` with pure virtual functions and a virtual destructor; inheritance (`User`, `Admin`, `DepartmentOfficer`, `Worker`); runtime polymorphism (the `AuthManager::login` factory returns a `Person*`); encapsulation; static utility class `Dashboard` | `include/interface/`, `src/interface/` |
| C and C++ together | `extern "C"` in the engine headers | `include/engine/*.h` |
| Files and concurrency | Text records written through a temporary file and an atomic replace; lock across processes (`CreateFileA` share mode on Windows, `flock` elsewhere) | `file_manager.c` |
| Workflow | Fixed status order, checked the same way for both interfaces, with role checks | `workflow.c` |
| Web server | Python standard library: `http.server` with threads, `subprocess`, sessions (`HttpOnly`, `SameSite=Strict`), login throttling, `hmac.compare_digest`, Content-Security-Policy | `web/server.py` (upload pending) |
| Web frontend | JavaScript modules, hash routing, `fetch`, SVG charts and map drawn in code, light and dark themes, text nodes only (no `innerHTML`) | `web/static/` (upload pending) |
| Build and tests | GNU Make; GCC/G++ `-Wall -Wextra -O2`; duplicate test, NLP evaluation, stress test | `Makefile`, `test_duplicate.c`, `tools/` |

## Repository layout

```text
.
├── README.md                 this file
├── phase 1/                  Phase I proposal report and PPT
├── phase 2 codes./           Phase II prototype
│   ├── complaint_ds.c/.h     linked list, linked-node priority queue, sorting, search
│   ├── Complaint.h, main.cpp C++ console application
│   ├── *.html, script.js, style.css   standalone website (localStorage)
│   ├── complaints.txt, status_history.txt
│   ├── readme.md             Phase II documentation
│   └── PhaseII_PPT.pdf, phase II report.pdf
└── phase 3 codes/            final system
    ├── Makefile, main.cpp, README.md, generate_dataset.py, test_duplicate.c
    ├── include/engine/       C data types and engine interfaces
    ├── include/interface/    C++ class declarations
    ├── src/engine/           C engine: storage, NLP, duplicates, heap, routing, workflow
    ├── src/interface/        C++ console application
    ├── src/cli/              engine_cli: JSON bridge for the website
    ├── tools/                eval_nlp.c/.py, stress_test.py, repair_data.py
    └── web/                  website (upload pending)
```

`data/`, `bin/` and `obj/` are not in the repository. The Makefile creates `bin/` and `obj/`, and the program creates `data/` with default accounts on first run.

## Getting the code

The folder `phase 2 codes.` ends with a dot, and Windows can't create such a folder. On Windows, a plain `git clone` therefore fails at checkout. Fetch only the other folders instead:

```powershell
git clone --no-checkout https://github.com/devrajrawat9027562124-web/Smart-Public_Complaint_Analyzer.git
cd Smart-Public_Complaint_Analyzer
git -c core.protectNTFS=false restore --source=main --worktree -- "phase 3 codes" "phase 1" README.md
```

On Linux and macOS, a normal `git clone` works.

## Build and run

Requirements: GCC/G++ and GNU Make (on Windows, MinGW with `mingw32-make`), plus Python 3 for the tools and the website.

```powershell
cd "phase 3 codes"
mingw32-make                  # builds bin/complaint_analyzer.exe and bin/engine_cli.exe
.\bin\complaint_analyzer.exe  # console app
python generate_dataset.py    # optional: 1,000 sample complaints (backs up data/ first)
mingw32-make test             # duplicate-detection test
mingw32-make eval             # NLP measured against hand-labelled templates
mingw32-make stress           # parallel-write test on a copy of data/
mingw32-make web              # website at http://localhost:8000 (needs web/, not uploaded yet)
```

On first run, the program creates `data/` with demo accounts. **These are for local testing only.**

| Role | Username | Password |
|---|---|---|
| Admin | `admin` | `admin123` |
| Department officers | `water_officer`, `elec_officer`, `road_officer`, `sanit_officer`, `safety_officer` | `officer123` |
| Field workers | `water_worker1`, `water_worker2`, `elec_worker1`, … (two per department) | `worker123` |

New sign-ups are citizen accounts.

## Testing and results

All results below were measured on 2026-11-22. The Phase III code in this repository was also built from a clean copy that day, and its duplicate test passed.

| Test | Result |
|---|---|
| Duplicate detection (`mingw32-make test`) | All 3 critical cases pass: different objects with similar wording are not flagged (score 0.17); a re-post by the same user is flagged (score 0.87); unrelated categories are not flagged (0.11). The informational case, the same issue reported by two users at one place, scores 0.53, below its 0.82 threshold, so it is **not** flagged. |
| NLP evaluation (`mingw32-make eval`, 150 hand-labelled templates) | Severity mean absolute error **1.41**, within ±1: **61%**, Spearman **0.75**; category suggestion correct **80%**; real complaints flagged as spam: **0**; critical templates (9–10) scored 4 or less: **0**; templates with no severity keyword (score 1): 15 |
| Concurrency (`mingw32-make stress`) | 60 location updates alongside 60 reads: 68 of 68 accounts kept. 80 submissions alongside 200 severity changes: 80 of 80 saved, each with its own ID, none repeated. |

The NLP labels and the keyword lists were written by the same team, and the templates are also the source of the sample data. So the evaluation shows how consistent the rules are, not how accurate they would be on real complaints. Scoring by the strongest keyword instead of the average was tried and did worse (error 1.83, Spearman 0.67).

## Known limitations

- **Passwords are stored in plain text** in `data/users.txt`, and default demo accounts are created on first run. Don't use real passwords, and don't put the website on the public internet.
- **Text files, not a database.** Each operation reads the whole file. Capacities are fixed in `complaint_types.h` (2,000 complaints, 200 users). Complaints after row 2,000 are saved but not loaded, so the queue and the duplicate check don't see them.
- **The text analysis is keyword-based and English-only.** A complaint with no keyword in the dictionary gets severity 1: "No water supply" scores 1/10. Averaging can lower the score of a serious complaint that also uses mild words. Typos, Hindi and Hinglish are not understood, and words aren't interpreted by context ("dead end").
- **Duplicate detection can miss reworded reports** of the same problem by different people at the same place (see the test above). Nearly identical text at different places can be flagged.
- **The road graph is a fixed model** with approximate distances, not a live map.
- The **website source** (`web/`) has not been uploaded to this repository yet.

## Future work

- Store salted, slow password hashes (for example bcrypt) instead of plain text, and remove the fixed demo passwords.
- Move storage to a database such as SQLite and remove the 2,000-complaint limit.
- Improve severity and duplicate detection with a model trained on real, labelled complaints, measured against the current rules.
- Support Hindi and Hinglish, and tolerate typos.
- Use real coordinates and travel times for routing.

## Timeline

| Date | Milestone |
|---|---|
| 2026-09-01 | Repository created; Phase I proposal report and PPT uploaded |
| 2026-10-16 to 10-22 | Phase II: C linked-list module, C++ console, website prototype, Phase II report and PPT |
| Oct–Nov 2026 | Phase III developed: modular C engine, shared workflow, C++ console rewrite, `engine_cli`, website on shared data, file lock, NLP evaluation and stress test |
| 2026-11-22 | Phase III code uploaded to `phase 3 codes/` (engine and console; website pending); Phase I and II documents moved into their phase folders; this README updated |
