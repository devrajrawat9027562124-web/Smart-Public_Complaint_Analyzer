# Web API (Smart Public Complaint Analyzer)

The web server (`web/server.py`, Python standard library only) serves the single-page
frontend from `web/static/` and a JSON API under `/api/`. All analysis and every write
goes through the C engine (`bin/engine_cli.exe`), so the website and the console app
share one implementation of NLP, duplicate detection, the priority queue and Dijkstra.

Conventions

- Request and response bodies are JSON (`Content-Type: application/json`).
- Success: HTTP 200 with a JSON object. Error: HTTP 4xx/5xx with `{"error": "message"}`.
  `401` = not logged in, `403` = role not allowed, `404` = not found, `400` = bad input.
- Text fields must be JSON strings and numbers whole numbers (`true` or `5.9` → 400).
  Text limits are in **UTF-8 bytes** (title 99, description 499, full name 99, password 49):
  a Hindi letter takes 3 bytes. Text over the limit is refused with 400, never cut.
- When the server runs on this computer only (the default, `127.0.0.1`), requests must use
  the host name `localhost`, `127.0.0.1` or `[::1]` with the server's port; any other `Host`
  header gets 400 (protection against DNS rebinding).
- Authentication is a session cookie (`spca_session`, HttpOnly) set by `/api/login` or
  `/api/signup`. The frontend sends `credentials: "same-origin"` (default for same origin).
- Dates are strings `"YYYY-MM-DD HH:MM"`. `completed_date` is `null` when not completed.
- Roles: `USER` (citizen), `ADMIN`, `DEPT_OFFICER`, `WORKER`.
- Statuses, in workflow order: `PENDING`, `REVIEWED`, `ASSIGNED`, `IN_PROGRESS`, `COMPLETED`.
- Categories / departments: `Water Supply`, `Electricity`, `Roads & Transport`,
  `Sanitation`, `Public Safety`. New complaints must have one of these; an old record may
  still say `General` (an admin can route it to a department).
- Severity label (same as console): >=9 `CRITICAL`, >=7 `HIGH`, >=5 `MEDIUM`, >=3 `LOW`, else `MINOR`.

## Objects

User

```json
{"id": 7, "username": "water_worker1", "full_name": "Suresh Negi", "role": "WORKER",
 "department": "Water Supply", "location_id": 0, "location": "Clock Tower",
 "phone": "9800000001", "email": "suresh@water.gov.in",
 "registered_date": "2026-10-05 23:12", "is_available": true}
```

Complaint

```json
{"id": 12, "user_id": 40, "title": "...", "description": "...",
 "category": "Water Supply", "department": "Water Supply",
 "location": "Clock Tower", "location_id": 0,
 "severity": 7, "severity_label": "HIGH", "nlp_score": 6,
 "is_duplicate": false, "duplicate_of": null, "is_irrelevant": false,
 "status": "ASSIGNED", "worker_id": 7, "worker_name": "Suresh Negi",
 "submitted_date": "2026-03-01 10:15", "completed_date": null,
 "submitter_name": "Deepak Agarwal"}
```

`submitter_name` is present only for ADMIN and DEPT_OFFICER (or the citizen's own complaints).

Route

```json
{"path": [0, 1, 5], "names": ["Clock Tower", "Paltan Bazaar", "Race Course"],
 "distance": 3, "steps": [{"from": 0, "to": 1, "km": 1}, {"from": 1, "to": 5, "km": 2}]}
```

Analysis (returned by `/api/analyze` and by complaint submission)

```json
{"severity": 7, "severity_label": "HIGH",
 "category": "Water Supply",
 "category_scores": {"Water Supply": 4, "Electricity": 0, "Roads & Transport": 1, "Sanitation": 0, "Public Safety": 0},
 "irrelevant": false,
 "keywords": [{"word": "burst", "weight": 7, "applied": 10.5, "modifier": "amplified"},
              {"word": "broken", "weight": 6, "applied": 1.8, "modifier": "negated"},
              {"word": "leak", "weight": 4, "applied": 4.0, "modifier": null}],
 "hit_bonus": 1.0,
 "duplicate": {"id": 33, "title": "...", "similarity": 0.86, "threshold": 0.82} }
```

`modifier` is `null`, `"amplified"`, `"negated"` or `"amplified+negated"` (e.g. "not very broken").
`duplicate` is `null` when no duplicate is found (or when no `location_id` was given).

## Public (no login)

| Method | Path | Result |
|---|---|---|
| GET | `/api/public/summary` | Aggregate stats only (no personal data): same shape as `/api/stats` minus `nlp_agreement`, plus `"demo_accounts": [{"role","username","password"}]` **only** when the server is bound to 127.0.0.1 (local demo); otherwise `demo_accounts` is `[]`. |
| GET | `/api/graph` | `{"nodes":[{"id","name","x","y"}], "edges":[{"a","b","km"}], "width":1000, "height":640}` — schematic city map; x/y are layout coordinates in that box (not GPS). |
| GET | `/api/route?from=<id>&to=<id>` | Route object (Dijkstra in C). |

## Auth

| Method | Path | Body | Result |
|---|---|---|---|
| POST | `/api/login` | `{"username","password"}` | `{"user": User}` + cookie. The sign-in is written to the activity log, marked `[web]`. |
| POST | `/api/signup` | `{"username","password","full_name","phone","email"}` | `{"user": User}` + cookie (always role USER) |
| POST | `/api/logout` | – | `{"ok": true}` |
| GET | `/api/me` | – | `{"user": User}` or 401 |
| POST | `/api/me/location` | `{"location_id"}` | `{"user": User}` (WORKER only) |
| POST | `/api/me/availability` | `{"available": true\|false}` | `{"user": User, "message"}` (WORKER only). Busy workers cannot be assigned new tasks; tasks they already have are kept. |

Signup rules: username 3–30 chars of letters, digits, `_`, `.`, `-` and unique ignoring case
(`Admin` is refused while `admin` exists); password 4–49 bytes without spaces or `|`;
full name required; phone optional (digits, 10–15); email optional.

## Complaints

| Method | Path | Who | Notes |
|---|---|---|---|
| GET | `/api/complaints` | any logged-in | Role-scoped: USER → own; DEPT_OFFICER → own department; WORKER → assigned to them; ADMIN → all. |
| GET | `/api/complaints/<id>` | scoped as above | `{"complaint", "original": Complaint\|null, "timeline": [{"time","message"}], "worker": User-lite\|null, "route": Route\|null}`. `route` = from the assigned worker's current location to the complaint (when a worker is assigned and status is not COMPLETED). `original` (for a duplicate) is the full complaint only if the viewer may see it, otherwise its public facts (id, title, category, location, status, dates, severity). `timeline` lists the log entries about this complaint; citizens see them without staff usernames. |
| POST | `/api/analyze` | any logged-in | Body `{"title","description","location_id"?}` → Analysis. Nothing is saved. |
| POST | `/api/complaints` | USER | Body `{"title","description","category","location_id"}`; `category` may be `""` to accept the NLP suggestion, which must then be one of the five categories (if the text names none, the request is refused: choose one). → `{"complaint", "analysis"}` |
| POST | `/api/complaints/<id>/review` | ADMIN | PENDING → REVIEWED. → `{"complaint"}` |
| POST | `/api/complaints/<id>/escalate` | ADMIN | Body `{"severity": 1..10}`; not on a COMPLETED complaint. → `{"complaint"}` |
| POST | `/api/complaints/<id>/assign-department` | ADMIN | Body `{"department_id"}`. Sets department + category; PENDING/REVIEWED → ASSIGNED; clears the worker if the department changes. → `{"complaint"}` |
| POST | `/api/complaints/<id>/assign-worker` | DEPT_OFFICER | Body `{"worker_id"}`; the complaint must be in the officer's department, routed there by an admin (status ASSIGNED) and without a worker; the worker must be available. → `{"complaint", "route"}` |
| POST | `/api/complaints/<id>/start` | WORKER | ASSIGNED to this worker → IN_PROGRESS. On a task already in progress it only returns the route again. → `{"complaint", "route"}` |
| POST | `/api/complaints/<id>/complete` | WORKER | IN_PROGRESS (started) → COMPLETED with completion date. → `{"complaint"}` |

Query parameters for `GET /api/complaints` (all optional):

- `q` — text search in title/description/location (case-insensitive), or `#123` for an id
- `status`, `category`, `location_id`, `department`, `worker_id`, `min_severity`
- `flag` — `duplicate` | `irrelevant` | `clean`
- `sort` — `submitted_desc` (default) | `submitted_asc` | `severity_desc` | `severity_asc` | `id_asc` | `id_desc`
- `page` (1-based, default 1), `page_size` (default 20, max 200)

Response: `{"items": [Complaint], "total": 123, "page": 1, "page_size": 20}`

## Management

| Method | Path | Who | Result |
|---|---|---|---|
| GET | `/api/queue?limit=50` | ADMIN | PENDING complaints in max-heap order (severity desc, then oldest first): `{"items": [Complaint], "total": n}` |
| GET | `/api/stats` | ADMIN (all), DEPT_OFFICER (own dept), WORKER (own tasks), USER (own complaints) | Stats object below |
| GET | `/api/departments` | ADMIN, DEPT_OFFICER | `[{"id","name","officer_username","officer_name","total","open","completed","workers"}]` |
| GET | `/api/workers?department=` | ADMIN, DEPT_OFFICER (forced to own dept) | `[{"id","username","full_name","department","location_id","location","is_available","active_tasks","completed_tasks"}]` |
| GET | `/api/logs?limit=100` | ADMIN | `[{"time","message"}]`, newest first |
| GET | `/api/compare?a=<id>&b=<id>` | ADMIN, DEPT_OFFICER | `{"jaccard","levenshtein","title","category","location","title_penalty","combined","threshold","same_user"}` (0–1 floats; `threshold` is the one the detector uses for this pair). `a` must be visible to the caller; `b` must be too, unless it is the complaint `a` was linked to as a duplicate. |

Stats object

```json
{"scope": "all",
 "totals": {"total": 502, "pending": 121, "reviewed": 99, "assigned": 97, "in_progress": 74,
            "completed": 111, "open": 391, "duplicates": 20, "irrelevant": 3},
 "by_category": [{"name": "Water Supply", "total": 102, "open": 80, "completed": 22, "avg_severity": 6.1}],
 "by_location": [{"id": 0, "name": "Clock Tower", "total": 30, "open": 22, "avg_severity": 5.4}],
 "severity_histogram": [{"severity": 1, "count": 3}, "... 10 entries"],
 "monthly": [{"month": "2025-01", "submitted": 20, "completed": 4, "partial": false}],
 "resolution": {"count": 111, "avg_days": 9.4, "median_days": 8.0},
 "nlp_agreement": {"n": 500, "exact": 0.31, "within_1": 0.72, "mae": 1.3}}
```

In `monthly`, `partial` is true for the current month, which is still running.

`nlp_agreement` compares the engine's `nlp_score` with the stored `severity`
(which the dataset/admin set). It is a sanity check of the rule-based NLP, not a
validated accuracy figure.
