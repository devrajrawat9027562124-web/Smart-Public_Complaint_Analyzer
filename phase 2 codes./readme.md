# Smart Public Complaint Analyzer

Smart Public Complaint Analyzer is a small complaint-management project with two separate interfaces:

1. A native C/C++ console application for registering and managing complaints.
2. A static HTML, CSS, and JavaScript website that demonstrates a browser-based complaint workflow.

The console application and website do **not** share a database or synchronize their records. The console program stores its records in text files, while the website stores its records in the browser's `localStorage`.

## Contents

- [Capabilities](#capabilities)
- [Technologies and design](#technologies-and-design)
- [Console application workflow](#console-application-workflow)
- [Priority assignment](#priority-assignment)
- [Files and persistence](#files-and-persistence)
- [Build and run the console application](#build-and-run-the-console-application)
- [Use the website](#use-the-website)
- [Project structure](#project-structure)
- [Current limitations](#current-limitations)

## Capabilities

### Console application

The console interface has two roles selected from the main menu:

**Citizen**

- Register a complaint by entering an ID, name, category, description, and location.
- View all saved complaints.
- Search for a complaint by ID.

**Admin**

- View all complaints.
- Change a complaint's status to Pending, In Progress, or Resolved.
- Assign a department and an employee.
- View the priority queue.
- Run the priority-related sort operation.
- View counts of total, pending, in-progress, and resolved complaints.
- Filter complaints by status or category.

New console complaints begin with Pending status and Not Assigned department and employee values. Their priority is assigned from the description and, in some cases, the category.

### Static website

The website contains a home page, a complaint registration form, a tracking page, and an admin page. In the browser, it can register and track complaints, search and sort the displayed records, update a record's department and status, and delete a record.

## Technologies and design

### C and C++

The console program uses both languages:

- `main.cpp` implements the application entry point and the Citizen and Admin menus using C++ console input/output.
- `complaint_ds.c` implements complaint operations, file handling, priority assignment, and linked-list processing in C.
- `complaint_ds.h` defines the C structures and declares the functions that the C++ menu calls.
- The header wraps its declarations in `extern "C"` when included by C++. This gives the C functions C-compatible linkage, allowing the C++ program to link to implementations compiled as C.
- `Complaint.h` defines a C++ `Complaint` class. The current console data-management routines use the `ComplaintNode` C structure rather than this class.

The console program uses standard C/C++ facilities: `stdio.h` for text-file and formatted input/output operations, `stdlib.h` for dynamic allocation, `string.h` for string operations, `ctype.h` for lowercase conversion, `time.h` for status-change timestamps, and C++ `iostream` for menus and prompts. It does not require an external database or third-party library.

### Data structures

Each console complaint is represented by a `ComplaintNode` structure. It holds:

- An integer complaint ID.
- Fixed-size character arrays for the citizen name, category, description, location, priority, status, department, and employee.
- An integer priority value: 3 for High, 2 for Medium, or 1 for Low.
- A pointer to the next complaint.

The `head` pointer in `main.cpp` is the start of a singly linked list of complaints. Loading and registration append complaint nodes to this list. Display, search, filtering, and statistics walk the list from beginning to end.

The priority queue is a second linked list. Each `PriorityNode` points to a complaint node and to the next priority-queue entry. The insertion function orders queue entries by descending numeric priority, so higher values appear first. The queue refers to the complaint nodes; it does not make duplicate complaint records.

The website uses JavaScript objects and arrays instead of these C structures. Its records are serialized to JSON for storage in `localStorage`.

## Console application workflow

1. On startup, `main()` calls `loadComplaints()` to read `complaints.txt` into the complaint linked list.
2. It visits the loaded complaints and inserts them into the priority queue.
3. The main menu lets the user open the Citizen menu, open the Admin menu, or exit.
4. Registering a complaint allocates and fills a new node, assigns a priority, initializes its status and assignments, appends it to the list, and saves the complaint list.
5. The Citizen/Admin actions traverse or update the in-memory list. Status and assignment changes are saved back to the text file.
6. When a status changes, a history entry is appended to `status_history.txt`.

The menus are text-based and expect the user to enter the numeric choice shown. Run the program from its project directory so its relative data-file paths point to the intended `complaints.txt` and `status_history.txt`.

### How the Admin operations work

- **Update status:** Finds the first complaint with the entered ID, records the old and new statuses in the history file, changes the in-memory status, then saves all complaints.
- **Assign department / employee:** Finds a complaint by ID, updates the selected text field, then saves all complaints.
- **View priority queue:** Traverses and prints queue entries in queue order.
- **Sort complaints by priority:** Compares priority values in the complaint list and swaps priority values and labels between nodes. It does not reorder the complaint nodes themselves.
- **Dashboard:** Traverses the list and counts records whose status exactly matches Pending, In Progress, or Resolved.
- **Filter:** Traverses the list and prints complaints whose status or category exactly matches the selected value.

## Priority assignment

The console's `assignPriority()` function checks the complaint description against three built-in keyword lists. Matching is case-insensitive: the text is first converted to lowercase, then searched for each keyword.

The checks run in this order:

1. If any High-priority keyword is found, assign High (3).
2. Otherwise, if any Medium-priority keyword is found, assign Medium (2).
3. Otherwise, if any Low-priority keyword is found, assign Low (1).
4. Otherwise, if the category is Road, Water, or Garbage (case-insensitive), assign Medium (2).
5. For any other unmatched description/category, assign Low (1).

The keyword lists are maintained in `complaint_ds.c`. Examples include fire, gas leak, exposed wire, or collapse for High; pothole, blocked drain, or water shortage for Medium; and peeling paint or minor litter for Low. Since High checks happen first, a description matching both High and Medium keywords receives High.

The website has a separate, simpler JavaScript priority function. It checks a smaller set of words in the combined category and description text and returns High, Medium, or Low. Its rules are not the same as the console application's rules.

## Files and persistence

### Console data files

`complaints.txt` stores one complaint per line as ten pipe-separated fields:

```text
id|citizen name|category|description|location|priority|priority value|status|department|employee
```

For example:

```text
123|Asha Rao|Road|Pothole near the market|Main Street|Medium|2|Pending|Not Assigned|Not Assigned
```

The loader expects all ten fields. Keep the pipe character (`|`) out of entered field values because it is the delimiter. The application rewrites this file when saving, so keep a backup before making manual edits.

`status_history.txt` is an append-only log for status changes. Each entry includes the complaint ID, previous status, new status, and local date/time. A change to department or employee does not create a status-history entry.

These files are opened using paths relative to the program's current working directory. If the program is launched from another directory, it may create or look for data files there instead of in the project folder.

### Website data

The website keeps complaint records and the next complaint ID in browser `localStorage`, using the keys `spca_complaints_v2` and `spca_next_complaint_id_v2`. Its data is local to that browser storage context. Clearing browser data or using another browser/profile can make those records unavailable.

There is no server or database connecting the website to the C/C++ application. Updating a complaint in one interface does not update the other interface's data.

## Build and run the console application

### Requirements

- Windows
- MinGW-w64 (or another toolchain that provides `gcc` and `g++`) available on `PATH`

Open PowerShell in the project directory and build the C module, then link it with the C++ application:

```powershell
$object = Join-Path $env:TEMP 'complaint_ds_windows.o'
gcc -std=c11 -Wall -Wextra -c .\complaint_ds.c -o $object
if ($LASTEXITCODE -ne 0) { throw 'C compilation failed.' }

g++ -std=c++17 -Wall -Wextra .\main.cpp $object -o .\complaint_analyzer.exe
if ($LASTEXITCODE -ne 0) { throw 'C++ build failed.' }

Remove-Item -LiteralPath $object
```

The first command compiles `complaint_ds.c` as C into a temporary object file. The second compiles `main.cpp` as C++ and links it with that object to create `complaint_analyzer.exe`. The temporary object is removed after a successful build.

Run the program from the same project directory:

```powershell
.\complaint_analyzer.exe
```

The executable can be rebuilt by repeating the build commands after changing either source file.

## Use the website

Open `website/index.html` in a modern web browser. The website is static and does not need a build step or package installation.

- **Home (`index.html`):** Introduction and navigation to registration and tracking.
- **Register (`register.html`):** Collects citizen ID, name, category, location, and description. JavaScript validates required fields, checks for a matching category/location/description duplicate, assigns the next numeric ID, calculates priority, and stores the record in browser storage.
- **Track (`track.html`):** Looks up a complaint by its numeric ID in the browser's stored records and displays its details.
- **Admin (`admin.html`):** Displays stored complaints; allows text search and sorting by newest ID or priority; provides controls to update department/status or delete a complaint.
- **Styles (`style.css`):** Controls the visual layout and styling of the website pages.
- **Behavior (`script.js`):** Handles form submission, browser storage, priority calculation, tracking, filtering/search, sorting, and admin updates.

The website has no authentication or server-side access control. Its Admin page is a demonstration interface, not a protected production admin system. Website deletion only removes a record from that browser's stored array; there is no server copy.

## Project structure

```text
Smart-Public-Complaint-Analyzer 2/
|-- Complaint.h             C++ Complaint class declaration
|-- complaint_ds.c          C implementation of complaint operations
|-- complaint_ds.h          C structures and function declarations
|-- complaints.txt          Console application's complaint records
|-- main.cpp                Console menus and application entry point
|-- status_history.txt      Console status-change log
|-- README.md               Project documentation
`-- website/
    |-- admin.html          Browser-based admin interface
    |-- index.html          Website home page
    |-- register.html       Browser complaint registration form
    |-- script.js           Website behavior and localStorage handling
    |-- style.css           Website styling
    `-- track.html          Browser complaint tracking page
```

## Current limitations

- The console application is a local, single-user program; it has no login, network service, or shared database.
- Complaint IDs are supplied manually in the console, and the program does not enforce ID uniqueness. Searching by a duplicated ID returns the first matching record.
- The console file format is delimiter-based rather than escaped or database-backed. A pipe in a field can prevent a line from loading correctly.
- Console priority classification is a keyword heuristic, not an external analysis or machine-learning service. Unrecognized descriptions fall back to category rules or Low priority.
- The console's priority sort operation swaps priority labels/values but leaves complaint-list order unchanged.
- The website has separate browser-local data, simpler priority rules, and no authentication. It is not connected to the console records.
