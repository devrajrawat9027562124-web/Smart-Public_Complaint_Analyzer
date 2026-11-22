"""
Smart Public Complaint Analyzer - web server.

    python web/server.py [--host 127.0.0.1] [--port 8000] [--open]

Serves the single-page site in web/static/ and the JSON API documented in
web/API.md. It uses only the Python standard library.

Design:
  * Reads come straight from the same pipe-delimited files as the console
    app (data/*.txt), parsed with the same rules as src/engine/file_manager.c.
  * Every analysis and every write goes through bin/engine_cli(.exe), i.e.
    the project's own C engine (NLP, duplicate detection, max-heap priority
    queue, Dijkstra, workflow rules). The website and the console app
    therefore cannot disagree about how a complaint is handled.
"""

import argparse
import hmac
import http.server
import json
import mimetypes
import os
import re
import secrets
import socketserver
import statistics
import subprocess
import sys
import threading
import time
import urllib.parse
import webbrowser
from datetime import datetime

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# The folder that holds data/ (override with SPCA_ROOT, e.g. to test on a copy)
DATA_ROOT = os.path.abspath(os.environ.get("SPCA_ROOT") or ROOT)
DATA = os.path.join(DATA_ROOT, "data")
STATIC = os.path.join(ROOT, "web", "static")
CLI = os.path.join(ROOT, "bin", "engine_cli.exe" if os.name == "nt" else "engine_cli")

FILES = {
    "users": os.path.join(DATA, "users.txt"),
    "complaints": os.path.join(DATA, "complaints.txt"),
    "departments": os.path.join(DATA, "departments.txt"),
    "logs": os.path.join(DATA, "logs.txt"),
}

CATEGORIES = ["Water Supply", "Electricity", "Roads & Transport", "Sanitation", "Public Safety"]
STATUSES = ["PENDING", "REVIEWED", "ASSIGNED", "IN_PROGRESS", "COMPLETED"]
DATE_FMT = "%Y-%m-%d %H:%M"
SESSION_COOKIE = "spca_session"
SESSION_TTL = 8 * 3600
MAX_BODY = 64 * 1024

# Field size limits in bytes (C buffers minus the terminating NUL)
LIMITS = {"title": 99, "description": 499, "category": 49, "username": 49, "password": 49,
          "full_name": 99, "phone": 19, "email": 99}

# Schematic map layout (x, y in a 1000 x 640 box), found by a small layout
# search that minimises road crossings (2 of 32 roads cross) and keeps
# locations apart. It is a diagram of route_optimizer.c's graph, not GPS.
LAYOUT = {
    0: (361, 118), 1: (612, 197), 2: (718, 147), 3: (190, 292), 4: (322, 386),
    5: (617, 306), 6: (702, 371), 7: (179, 552), 8: (72, 291), 9: (905, 137),
    10: (794, 234), 11: (209, 432), 12: (510, 211), 13: (414, 204), 14: (402, 478),
    15: (378, 297), 16: (533, 374), 17: (898, 274),
}


# ═══════════════════════════════════════════════════════════════════
#  Data files (same parsing rules as file_manager.c)
# ═══════════════════════════════════════════════════════════════════

def _to_int(s, fallback):
    try:
        return int(str(s).strip())
    except ValueError:
        return fallback


def parse_users(lines):
    users = []
    for line in lines:
        f = line.split("|")
        if len(f) < 9 or not f[1]:
            continue
        uid = _to_int(f[0], -1)
        if uid < 0:
            continue
        users.append({
            "id": uid, "username": f[1], "password": f[2], "full_name": f[3], "phone": f[4],
            "email": f[5], "role": f[6], "registered_date": f[7], "department": f[8],
            "location_id": _to_int(f[9], 0) if len(f) > 9 else 0,
            "is_available": _to_int(f[10], 1) != 0 if len(f) > 10 else True,
        })
    return users


def parse_complaints(lines):
    out = []
    for line in lines:
        f = line.split("|")
        if len(f) < 15:
            continue
        extra = len(f) - 17
        if extra > 0:  # legacy description containing '|'
            f = f[:3] + [" / ".join(f[3:4 + extra])] + f[4 + extra:]
        cid = _to_int(f[0], -1)
        if cid < 0:
            continue
        sev = _to_int(f[7], 1)
        done = f[16] if len(f) > 16 and f[16] and f[16] != "N/A" else None
        out.append({
            "id": cid, "user_id": _to_int(f[1], -1), "title": f[2], "description": f[3],
            "category": f[4], "location": f[5], "location_id": _to_int(f[6], 0),
            "severity": sev, "nlp_score": _to_int(f[8], sev),
            "is_duplicate": _to_int(f[9], 0) == 1, "duplicate_of": _to_int(f[10], -1),
            "is_irrelevant": _to_int(f[11], 0) == 1, "status": f[12], "department": f[13],
            "worker_id": _to_int(f[14], -1),
            "submitted_date": f[15] if len(f) > 15 else "", "completed_date": done,
        })
    return out


def parse_departments(lines):
    out = []
    for line in lines:
        f = line.split("|")
        if len(f) < 3:
            continue
        did = _to_int(f[0], -1)
        if did >= 0 and f[1]:
            out.append({"id": did, "name": f[1], "officer_username": f[2]})
    return out


LOG_RE = re.compile(r"^\[([^\]]+)\]\s?(.*)$")


def parse_logs(lines):
    out = []
    for line in lines:
        m = LOG_RE.match(line)
        if m:
            out.append({"time": m.group(1), "message": m.group(2)})
    return out


PARSERS = {"users": parse_users, "complaints": parse_complaints,
           "departments": parse_departments, "logs": parse_logs}


class Store:
    """Parsed data files, re-read only when a file changes on disk."""

    def __init__(self):
        self._cache = {}
        self._lock = threading.Lock()

    def get(self, name):
        path = FILES[name]
        for attempt in range(5):
            try:
                st = os.stat(path)
                key = (st.st_mtime_ns, st.st_size)
                with self._lock:
                    hit = self._cache.get(name)
                    if hit and hit[0] == key:
                        return hit[1]
                with open(path, encoding="utf-8-sig", errors="replace") as f:
                    lines = [ln.rstrip("\r\n") for ln in f if ln.strip()]
                value = PARSERS[name](lines)
                with self._lock:
                    self._cache[name] = (key, value)
                return value
            except FileNotFoundError:
                return []
            except PermissionError:   # file being replaced by the engine right now
                time.sleep(0.05 * (attempt + 1))
        raise RuntimeError(f"Could not read {path}")

    def users_by_id(self):
        return {u["id"]: u for u in self.get("users")}

    def complaint(self, cid):
        for c in self.get("complaints"):
            if c["id"] == cid:
                return c
        return None


STORE = Store()


# ═══════════════════════════════════════════════════════════════════
#  C engine bridge
# ═══════════════════════════════════════════════════════════════════

class ApiError(Exception):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status
        self.message = message


WRITE_LOCK = threading.Lock()
CODE_STATUS = {"not_found": 404, "forbidden": 403, "invalid": 400}


def clean_field(value, limit_key=None, label=None):
    """One stdin line for engine_cli: no newlines or '|', trimmed.

    The value must be text (a JSON object or list used to be stored as
    "{'a': 1}"). Text over the limit is refused, not cut: limits are in
    UTF-8 bytes (the C engine's buffers), and Hindi letters take 3 bytes
    each, so a long Hindi title used to lose most of its words silently."""
    if value is None:
        return ""
    if not isinstance(value, str):
        raise ApiError(400, f"'{label or limit_key or 'field'}' must be text.")
    s = re.sub(r"[\r\n\t]+", " ", value).replace("|", "/").strip()
    if limit_key and len(s.encode("utf-8")) > LIMITS[limit_key]:
        raise ApiError(400, f"{(label or limit_key).capitalize()} is too long: at most {LIMITS[limit_key]} bytes "
                            "(an English letter is 1 byte, a Hindi letter 3).")
    return s


def engine(*args, stdin_fields=None, write=False):
    if not os.path.exists(CLI):
        raise ApiError(500, "bin/engine_cli is missing. Build it with: mingw32-make")
    data = "\n".join(stdin_fields) + "\n" if stdin_fields is not None else ""
    env = dict(os.environ, SPCA_ROOT=DATA_ROOT)
    lock = WRITE_LOCK if write else None
    if lock:
        lock.acquire()
    try:
        proc = subprocess.run([CLI, *[str(a) for a in args]], input=data.encode("utf-8"),
                              capture_output=True, cwd=DATA_ROOT, env=env, timeout=60)
    except subprocess.TimeoutExpired:
        raise ApiError(504, "The C engine took too long to respond.")
    finally:
        if lock:
            lock.release()
    try:
        out = json.loads(proc.stdout.decode("utf-8", errors="replace"))
    except ValueError:
        raise ApiError(500, "The C engine returned an unreadable response.")
    if not out.get("ok"):
        raise ApiError(CODE_STATUS.get(out.get("code"), 500), out.get("error", "Engine error"))
    return out


_GRAPH = None


def graph():
    global _GRAPH
    if _GRAPH is None:
        g = engine("graph")
        nodes = [{"id": n["id"], "name": n["name"], "x": LAYOUT.get(n["id"], (0, 0))[0],
                  "y": LAYOUT.get(n["id"], (0, 0))[1]} for n in g["nodes"]]
        _GRAPH = {"nodes": nodes, "edges": g["edges"], "width": 1000, "height": 640}
    return _GRAPH


def location_name(loc_id):
    for n in graph()["nodes"]:
        if n["id"] == loc_id:
            return n["name"]
    return "Unknown"


def route(src, dst):
    return engine("route", src, dst)["route"]


# ═══════════════════════════════════════════════════════════════════
#  Presentation helpers
# ═══════════════════════════════════════════════════════════════════

def severity_label(sev):
    if sev >= 9:
        return "CRITICAL"
    if sev >= 7:
        return "HIGH"
    if sev >= 5:
        return "MEDIUM"
    if sev >= 3:
        return "LOW"
    return "MINOR"


def user_public(u):
    return {
        "id": u["id"], "username": u["username"], "full_name": u["full_name"], "role": u["role"],
        "department": u["department"], "location_id": u["location_id"],
        "location": location_name(u["location_id"]), "phone": u["phone"], "email": u["email"],
        "registered_date": u["registered_date"], "is_available": u["is_available"],
    }


def is_staff(user):
    return user["role"] in ("ADMIN", "DEPT_OFFICER")


def complaint_json(c, viewer, users=None):
    users = users if users is not None else STORE.users_by_id()
    worker = users.get(c["worker_id"]) if c["worker_id"] > 0 else None
    out = dict(c)
    out["severity_label"] = severity_label(c["severity"])
    out["duplicate_of"] = c["duplicate_of"] if c["is_duplicate"] and c["duplicate_of"] > 0 else None
    out["worker_id"] = c["worker_id"] if c["worker_id"] > 0 else None
    out["worker_name"] = worker["full_name"] if worker else None
    if is_staff(viewer) or c["user_id"] == viewer["id"]:
        owner = users.get(c["user_id"])
        out["submitter_name"] = owner["full_name"] if owner else None
    return out


def can_see(user, c):
    role = user["role"]
    if role == "ADMIN":
        return True
    if role == "DEPT_OFFICER":
        return c["department"] == user["department"]
    if role == "WORKER":
        return c["worker_id"] == user["id"]
    return c["user_id"] == user["id"]


def visible_complaints(user):
    return [c for c in STORE.get("complaints") if can_see(user, c)]


def parse_dt(s):
    try:
        return datetime.strptime((s or "").strip(), DATE_FMT)
    except ValueError:
        return None


def compute_stats(items, scope, include_nlp=True):
    totals = {"total": len(items)}
    for s in STATUSES:
        totals[s.lower()] = sum(1 for c in items if c["status"] == s)
    totals["open"] = totals["total"] - totals["completed"]
    totals["duplicates"] = sum(1 for c in items if c["is_duplicate"])
    totals["irrelevant"] = sum(1 for c in items if c["is_irrelevant"])

    cats = list(CATEGORIES) + sorted({c["category"] for c in items} - set(CATEGORIES))
    by_category = []
    for name in cats:
        group = [c for c in items if c["category"] == name]
        if not group and name not in CATEGORIES:
            continue
        by_category.append({
            "name": name, "total": len(group),
            "open": sum(1 for c in group if c["status"] != "COMPLETED"),
            "completed": sum(1 for c in group if c["status"] == "COMPLETED"),
            "avg_severity": round(sum(c["severity"] for c in group) / len(group), 2) if group else 0,
        })

    by_location = []
    for n in graph()["nodes"]:
        group = [c for c in items if c["location_id"] == n["id"]]
        by_location.append({
            "id": n["id"], "name": n["name"], "total": len(group),
            "open": sum(1 for c in group if c["status"] != "COMPLETED"),
            "avg_severity": round(sum(c["severity"] for c in group) / len(group), 2) if group else 0,
        })

    histogram = [{"severity": s, "count": sum(1 for c in items if c["severity"] == s)}
                 for s in range(1, 11)]

    months = {}
    for c in items:
        d = parse_dt(c["submitted_date"])
        if d:
            months.setdefault(d.strftime("%Y-%m"), {"submitted": 0, "completed": 0})["submitted"] += 1
        d = parse_dt(c["completed_date"])
        if c["status"] == "COMPLETED" and d:
            months.setdefault(d.strftime("%Y-%m"), {"submitted": 0, "completed": 0})["completed"] += 1
    monthly = []
    this_month = datetime.now().strftime("%Y-%m")
    if months:
        first, last = min(months), max(months)
        y, m = int(first[:4]), int(first[5:])
        while f"{y:04d}-{m:02d}" <= last:
            key = f"{y:04d}-{m:02d}"
            # The current month is still running: flagged so the chart does
            # not show it as a sudden drop
            monthly.append({"month": key, **months.get(key, {"submitted": 0, "completed": 0}),
                            "partial": key == this_month})
            m += 1
            if m > 12:
                y, m = y + 1, 1

    days = []
    for c in items:
        if c["status"] != "COMPLETED":
            continue
        a, b = parse_dt(c["submitted_date"]), parse_dt(c["completed_date"])
        if a and b and b >= a:
            days.append((b - a).total_seconds() / 86400)
    resolution = {"count": len(days),
                  "avg_days": round(sum(days) / len(days), 1) if days else None,
                  "median_days": round(statistics.median(days), 1) if days else None}

    out = {"scope": scope, "totals": totals, "by_category": by_category, "by_location": by_location,
           "severity_histogram": histogram, "monthly": monthly, "resolution": resolution}
    if include_nlp:
        n = len(items)
        out["nlp_agreement"] = {
            "n": n,
            "exact": round(sum(1 for c in items if c["nlp_score"] == c["severity"]) / n, 3) if n else None,
            "within_1": round(sum(1 for c in items if abs(c["nlp_score"] - c["severity"]) <= 1) / n, 3) if n else None,
            "mae": round(sum(abs(c["nlp_score"] - c["severity"]) for c in items) / n, 2) if n else None,
        }
    return out


# ═══════════════════════════════════════════════════════════════════
#  Sessions and login throttling
# ═══════════════════════════════════════════════════════════════════

SESSIONS = {}
SESSIONS_LOCK = threading.Lock()
FAILED_LOGINS = {}


def new_session(user_id):
    token = secrets.token_urlsafe(32)
    with SESSIONS_LOCK:
        SESSIONS[token] = {"user_id": user_id, "expires": time.time() + SESSION_TTL}
    return token


def session_user(token):
    if not token:
        return None
    with SESSIONS_LOCK:
        s = SESSIONS.get(token)
        if not s or s["expires"] < time.time():
            SESSIONS.pop(token, None)
            return None
        s["expires"] = time.time() + SESSION_TTL
        uid = s["user_id"]
    return STORE.users_by_id().get(uid)


def throttled(ip):
    now = time.time()
    recent = [t for t in FAILED_LOGINS.get(ip, []) if now - t < 60]
    FAILED_LOGINS[ip] = recent
    return len(recent) >= 8


# ═══════════════════════════════════════════════════════════════════
#  Request handler
# ═══════════════════════════════════════════════════════════════════

ROUTES = []


def route_to(method, pattern, auth=True, roles=None):
    def deco(fn):
        ROUTES.append((method, re.compile("^" + pattern + "$"), fn, auth, roles))
        return fn
    return deco


def int_param(value, name, lo=None, hi=None):
    # true/false and 5.9 used to become 1 and 5 silently
    if isinstance(value, (bool, float)):
        raise ApiError(400, f"'{name}' must be a whole number.")
    try:
        v = int(value)
    except (TypeError, ValueError):
        raise ApiError(400, f"'{name}' must be a whole number.")
    if (lo is not None and v < lo) or (hi is not None and v > hi):
        raise ApiError(400, f"'{name}' is out of range.")
    return v


class Handler(http.server.BaseHTTPRequestHandler):
    server_version = "SPCA/1.0"
    protocol_version = "HTTP/1.1"
    timeout = 30   # a client that stops sending (e.g. Content-Length: -1) no longer holds a thread forever

    # ── plumbing ──
    def log_message(self, fmt, *args):
        if self.server.verbose:
            super().log_message(fmt, *args)

    def _security_headers(self):
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("X-Frame-Options", "DENY")
        self.send_header("Content-Security-Policy",
                         "default-src 'self'; img-src 'self' data:; style-src 'self' 'unsafe-inline'; "
                         "script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none'")

    def send_json(self, status, obj, cookie=None):
        body = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        if cookie is not None:
            self.send_header("Set-Cookie", cookie)
        self._security_headers()
        self.end_headers()
        self.wfile.write(body)

    def cookie_token(self):
        raw = self.headers.get("Cookie", "")
        for part in raw.split(";"):
            k, _, v = part.strip().partition("=")
            if k == SESSION_COOKIE:
                return v
        return None

    def take_body(self):
        """Read the whole request body before anything can fail.

        An error raised before the body was read (401, 403, 404, 429...)
        used to leave it in the connection, where it was glued to the front
        of the next request on the same keep-alive connection (400)."""
        self._raw_body = b""
        raw = self.headers.get("Content-Length")
        if raw in (None, ""):
            return
        try:
            length = int(raw)
        except ValueError:
            length = -1
        if length < 0 or length > MAX_BODY:
            self.close_connection = True   # the body can't be skipped safely
            if length < 0:
                raise ApiError(400, "Invalid Content-Length.")
            raise ApiError(413, "Request body too large.")
        self._raw_body = self.rfile.read(length)

    def read_body(self):
        raw = getattr(self, "_raw_body", b"")
        if not raw:
            return {}
        try:
            data = json.loads(raw.decode("utf-8"))
        except ValueError:
            raise ApiError(400, "Request body must be JSON.")
        if not isinstance(data, dict):
            raise ApiError(400, "Request body must be a JSON object.")
        return data

    def do_GET(self):
        self.dispatch("GET")

    def do_POST(self):
        self.dispatch("POST")

    def dispatch(self, method):
        parsed = urllib.parse.urlsplit(self.path)
        path = parsed.path
        try:
            if method == "POST":
                self.take_body()
            # DNS rebinding: a page on another site could otherwise point its
            # own domain name at 127.0.0.1 and read this API (in demo mode it
            # lists account passwords). Only our own host names are accepted.
            allowed = self.server.allowed_hosts
            if allowed is not None and (self.headers.get("Host") or "").lower() not in allowed:
                raise ApiError(400, "Unexpected Host header.")
        except ApiError as e:
            return self.send_json(e.status, {"error": e.message})
        if not path.startswith("/api/"):
            if method == "GET":
                return self.serve_static(path)
            return self.send_json(405, {"error": "Method not allowed."})
        self.query = {k: v[-1] for k, v in urllib.parse.parse_qs(parsed.query).items()}
        try:
            if method == "POST":
                # Basic CSRF defence: the frontend always sends JSON from the same origin.
                ctype = self.headers.get("Content-Type", "")
                if self.headers.get("Content-Length", "0") not in ("", "0") and "application/json" not in ctype:
                    raise ApiError(415, "Use Content-Type: application/json.")
                origin = self.headers.get("Origin")
                if origin and urllib.parse.urlsplit(origin).netloc != self.headers.get("Host"):
                    raise ApiError(403, "Cross-origin request refused.")
            for m, rx, fn, auth, roles in ROUTES:
                if m != method:
                    continue
                match = rx.match(path)
                if not match:
                    continue
                self.user = session_user(self.cookie_token())
                if auth and not self.user:
                    raise ApiError(401, "Please sign in.")
                if roles and self.user["role"] not in roles:
                    raise ApiError(403, "Your role cannot do this.")
                result = fn(self, *match.groups())
                if isinstance(result, tuple):
                    return self.send_json(200, result[0], cookie=result[1])
                return self.send_json(200, result)
            raise ApiError(404, "Unknown API endpoint.")
        except ApiError as e:
            self.send_json(e.status, {"error": e.message})
        except Exception as e:  # never leak a traceback to the browser
            sys.stderr.write(f"[error] {method} {path}: {e!r}\n")
            self.send_json(500, {"error": "Internal server error."})

    def serve_static(self, path):
        if path in ("", "/"):
            path = "/index.html"
        rel = urllib.parse.unquote(path).lstrip("/")
        # Only plain relative names such as "js/views/tasks.js". This check
        # must come before any filesystem call: "/%5c%5chost%5cshare%5cx.js"
        # used to become \\host\share\x.js, and realpath() alone made Windows
        # connect to that host (sending the user's login hash) for ~20 s.
        if not re.fullmatch(r"[A-Za-z0-9_.\-/]*", rel) or ".." in rel.split("/"):
            return self.send_json(404, {"error": "Not found."})
        full = os.path.realpath(os.path.join(STATIC, rel))
        if not full.startswith(os.path.realpath(STATIC) + os.sep) or not os.path.isfile(full):
            if os.path.splitext(rel)[1]:   # a missing asset, not an app route
                return self.send_json(404, {"error": "Not found."})
            full = os.path.join(STATIC, "index.html")  # SPA fallback
            if not os.path.isfile(full):
                return self.send_json(404, {"error": "Frontend not found (web/static/index.html)."})
        ctype = mimetypes.guess_type(full)[0] or "application/octet-stream"
        if full.endswith(".js") or full.endswith(".mjs"):
            ctype = "text/javascript"
        if ctype.startswith("text/") or ctype in ("application/json", "image/svg+xml"):
            ctype += "; charset=utf-8"
        with open(full, "rb") as f:
            body = f.read()
        self.send_response(200)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        self._security_headers()
        self.end_headers()
        self.wfile.write(body)


# ═══════════════════════════════════════════════════════════════════
#  API endpoints
# ═══════════════════════════════════════════════════════════════════

def session_cookie(token, max_age):
    return f"{SESSION_COOKIE}={token}; Path=/; HttpOnly; SameSite=Strict; Max-Age={max_age}"


@route_to("GET", r"/api/public/summary", auth=False)
def api_public_summary(h):
    out = compute_stats(STORE.get("complaints"), "all", include_nlp=False)
    out["demo_accounts"] = demo_accounts() if h.server.demo else []
    return out


def demo_accounts():
    users = STORE.get("users")
    picks = []
    for role in ("ADMIN", "DEPT_OFFICER", "WORKER"):
        u = next((u for u in users if u["role"] == role), None)
        if u:
            picks.append(u)
    citizens = [u for u in users if u["role"] == "USER"]
    if citizens:
        counts = {}
        for c in STORE.get("complaints"):
            counts[c["user_id"]] = counts.get(c["user_id"], 0) + 1
        picks.append(max(citizens, key=lambda u: counts.get(u["id"], 0)))
    return [{"role": u["role"], "username": u["username"], "password": u["password"]} for u in picks]


@route_to("GET", r"/api/graph", auth=False)
def api_graph(h):
    return graph()


@route_to("GET", r"/api/route", auth=False)
def api_route(h):
    n = len(graph()["nodes"])
    src = int_param(h.query.get("from"), "from", 0, n - 1)
    dst = int_param(h.query.get("to"), "to", 0, n - 1)
    return route(src, dst)


@route_to("POST", r"/api/login", auth=False)
def api_login(h):
    ip = h.client_address[0]
    if throttled(ip):
        raise ApiError(429, "Too many failed sign-in attempts. Wait a minute and try again.")
    body = h.read_body()
    username, password = body.get("username"), body.get("password")
    if not isinstance(username, str) or not isinstance(password, str):
        raise ApiError(400, "Enter a username and a password.")
    username = username.strip()
    user = next((u for u in STORE.get("users") if u["username"] == username), None)
    if not user or not hmac.compare_digest(user["password"].encode(), password.encode()):
        FAILED_LOGINS.setdefault(ip, []).append(time.time())
        raise ApiError(401, "Invalid username or password.")
    token = new_session(user["id"])
    # Logged like a console sign-in, so the Activity page shows both
    try:
        engine("log-signin", user["id"], write=True)
    except ApiError:
        pass   # signing in must not fail because the log could not be written
    return {"user": user_public(user)}, session_cookie(token, SESSION_TTL)


@route_to("POST", r"/api/signup", auth=False)
def api_signup(h):
    body = h.read_body()
    password = body.get("password")
    # Checked here, not cut: a 53-character password used to be stored as its
    # first 49, so the account could never sign in; null was stored as "None"
    if not isinstance(password, str):
        raise ApiError(400, "Choose a password.")
    if "\n" in password or "\r" in password:
        raise ApiError(400, "Password cannot contain line breaks.")
    if len(password.encode("utf-8")) > LIMITS["password"]:
        raise ApiError(400, f"Password is too long: at most {LIMITS['password']} bytes.")
    fields = [clean_field(body.get("username"), "username"),
              password,
              clean_field(body.get("full_name"), "full_name", "full name"),
              clean_field(body.get("phone"), "phone"),
              clean_field(body.get("email"), "email")]
    out = engine("signup", stdin_fields=fields, write=True)
    user = STORE.users_by_id().get(out["id"])
    if not user:
        raise ApiError(500, "Account was created but could not be loaded.")
    token = new_session(user["id"])
    return {"user": user_public(user)}, session_cookie(token, SESSION_TTL)


@route_to("POST", r"/api/logout", auth=False)
def api_logout(h):
    token = h.cookie_token()
    if token:
        with SESSIONS_LOCK:
            SESSIONS.pop(token, None)
    return {"ok": True}, session_cookie("", 0)


@route_to("GET", r"/api/me")
def api_me(h):
    return {"user": user_public(h.user)}


@route_to("POST", r"/api/me/location", roles=("WORKER",))
def api_me_location(h):
    body = h.read_body()
    loc = int_param(body.get("location_id"), "location_id", 0, len(graph()["nodes"]) - 1)
    engine("set-location", h.user["id"], loc, write=True)
    return {"user": user_public(STORE.users_by_id()[h.user["id"]])}


@route_to("POST", r"/api/me/availability", roles=("WORKER",))
def api_me_availability(h):
    available = h.read_body().get("available")
    if not isinstance(available, bool):
        raise ApiError(400, "'available' must be true or false.")
    out = engine("set-availability", h.user["id"], 1 if available else 0, write=True)
    return {"user": user_public(STORE.users_by_id()[h.user["id"]]), "message": out["message"]}


# ── complaints ──

SORTS = {
    "submitted_desc": (lambda c: (c["submitted_date"], c["id"]), True),
    "submitted_asc": (lambda c: (c["submitted_date"], c["id"]), False),
    "severity_desc": (lambda c: (c["severity"], c["id"]), True),
    "severity_asc": (lambda c: (c["severity"], c["id"]), False),
    "id_asc": (lambda c: c["id"], False),
    "id_desc": (lambda c: c["id"], True),
}


@route_to("GET", r"/api/complaints")
def api_complaints(h):
    q = h.query
    items = visible_complaints(h.user)
    text = (q.get("q") or "").strip().lower()
    if text:
        m = re.fullmatch(r"#?(\d+)", text)
        if m:
            items = [c for c in items if c["id"] == int(m.group(1))]
        else:
            items = [c for c in items if text in c["title"].lower() or text in c["description"].lower()
                     or text in c["location"].lower()]
    if q.get("status"):
        items = [c for c in items if c["status"] == q["status"]]
    if q.get("category"):
        items = [c for c in items if c["category"] == q["category"]]
    if q.get("department"):
        items = [c for c in items if c["department"] == q["department"]]
    if q.get("location_id", "") != "":
        loc = int_param(q["location_id"], "location_id")
        items = [c for c in items if c["location_id"] == loc]
    if q.get("worker_id", "") != "":
        wid = int_param(q["worker_id"], "worker_id")
        items = [c for c in items if c["worker_id"] == wid]
    if q.get("min_severity", "") != "":
        ms = int_param(q["min_severity"], "min_severity", 1, 10)
        items = [c for c in items if c["severity"] >= ms]
    flag = q.get("flag")
    if flag == "duplicate":
        items = [c for c in items if c["is_duplicate"]]
    elif flag == "irrelevant":
        items = [c for c in items if c["is_irrelevant"]]
    elif flag == "clean":
        items = [c for c in items if not c["is_duplicate"] and not c["is_irrelevant"]]

    key, reverse = SORTS.get(q.get("sort") or "submitted_desc", SORTS["submitted_desc"])
    items = sorted(items, key=key, reverse=reverse)
    page = int_param(q.get("page") or 1, "page", 1)
    size = int_param(q.get("page_size") or 20, "page_size", 1, 200)
    users = STORE.users_by_id()
    chunk = items[(page - 1) * size: page * size]
    return {"items": [complaint_json(c, h.user, users) for c in chunk], "total": len(items),
            "page": page, "page_size": size}


def complaint_for(h, cid):
    c = STORE.complaint(int(cid))
    if not c or not can_see(h.user, c):
        raise ApiError(404, f"Complaint #{cid} not found.")
    return c


@route_to("GET", r"/api/complaints/(\d+)")
def api_complaint_detail(h, cid):
    c = complaint_for(h, cid)
    users = STORE.users_by_id()

    original = None
    if c["is_duplicate"] and c["duplicate_of"] > 0:
        o = STORE.complaint(c["duplicate_of"])
        if o:
            # Only what this viewer may see: officers used to get the full
            # record (description, citizen's name) of other departments'
            # complaints here
            if can_see(h.user, o):
                original = complaint_json(o, h.user, users)
            else:  # public facts only
                original = {k: o[k] for k in ("id", "title", "category", "location", "location_id",
                                              "status", "submitted_date")}
                original["severity"] = o["severity"]
                original["severity_label"] = severity_label(o["severity"])

    timeline = [{"time": c["submitted_date"], "message": "Complaint submitted"}]
    for e in STORE.get("logs"):
        # The entry's own complaint is the first "complaint #N" in it: a title
        # such as "complaint #5 resolved" (after the ID) used to put the entry
        # on #5's timeline too. Entries older than the complaint belong to an
        # earlier record that had the same ID (the data was regenerated).
        m = re.search(r"[Cc]omplaint #(\d+)\b", e["message"])
        if not m or int(m.group(1)) != c["id"] or "submitted complaint" in e["message"]:
            continue
        if e["time"] < c["submitted_date"]:
            continue
        message = e["message"]
        if h.user["role"] == "USER":   # citizens see what happened, not staff usernames
            message = re.sub(r" '[^']*'| \(ID:\d+\)| \[web\]", "", message)
        timeline.append({"time": e["time"], "message": message})
    if c["status"] == "COMPLETED" and c["completed_date"]:
        if not any("completed complaint" in e["message"] for e in timeline):
            timeline.append({"time": c["completed_date"], "message": "Marked as completed"})
    timeline.sort(key=lambda e: e["time"])

    worker, rt = None, None
    w = users.get(c["worker_id"]) if c["worker_id"] > 0 else None
    if w:
        if h.user["role"] == "USER":
            worker = {"id": w["id"], "full_name": w["full_name"]}
        else:
            worker = {"id": w["id"], "full_name": w["full_name"], "username": w["username"],
                      "location_id": w["location_id"], "location": location_name(w["location_id"])}
            if c["status"] != "COMPLETED":
                try:
                    rt = route(w["location_id"], c["location_id"])
                except ApiError:
                    rt = None
    return {"complaint": complaint_json(c, h.user, users), "original": original,
            "timeline": timeline, "worker": worker, "route": rt}


def draft_fields(body):
    title = clean_field(body.get("title"), "title")
    description = clean_field(body.get("description"), "description")
    category = clean_field(body.get("category"), "category")
    # "" = use the suggested category. "General" is not accepted: no
    # department handles it, so such a complaint could never be resolved.
    if category and category not in CATEGORIES:
        raise ApiError(400, "Unknown category.")
    return title, description, category


@route_to("POST", r"/api/analyze")
def api_analyze(h):
    body = h.read_body()
    title, description, category = draft_fields(body)
    loc = body.get("location_id")
    loc = "" if loc in (None, "") else int_param(loc, "location_id", 0, len(graph()["nodes"]) - 1)
    out = engine("analyze", stdin_fields=[str(h.user["id"]), str(loc), category, title, description])
    out.pop("ok", None)
    return out


@route_to("POST", r"/api/complaints", roles=("USER",))
def api_submit(h):
    body = h.read_body()
    title, description, category = draft_fields(body)
    if len(title) < 3:
        raise ApiError(400, "Please enter a title (at least 3 characters).")
    if len(description) < 10:
        raise ApiError(400, "Please describe the problem (at least 10 characters).")
    loc = int_param(body.get("location_id"), "location_id", 0, len(graph()["nodes"]) - 1)
    out = engine("submit", stdin_fields=[str(h.user["id"]), h.user["username"], str(loc),
                                         category, title, description], write=True)
    c = STORE.complaint(out["id"])
    return {"complaint": complaint_json(c, h.user), "analysis": out["analysis"], "message": out["message"]}


def after_write(h, cid, extra=None):
    out = {"complaint": complaint_json(STORE.complaint(int(cid)), h.user)}
    if extra:
        out.update(extra)
    return out


@route_to("POST", r"/api/complaints/(\d+)/review", roles=("ADMIN",))
def api_review(h, cid):
    complaint_for(h, cid)
    engine("review", cid, h.user["username"], write=True)
    return after_write(h, cid)


@route_to("POST", r"/api/complaints/(\d+)/escalate", roles=("ADMIN",))
def api_escalate(h, cid):
    complaint_for(h, cid)
    sev = int_param(h.read_body().get("severity"), "severity", 1, 10)
    engine("escalate", cid, sev, h.user["username"], write=True)
    return after_write(h, cid)


@route_to("POST", r"/api/complaints/(\d+)/assign-department", roles=("ADMIN",))
def api_assign_department(h, cid):
    complaint_for(h, cid)
    dept = int_param(h.read_body().get("department_id"), "department_id", 1)
    engine("assign-dept", cid, dept, h.user["username"], write=True)
    return after_write(h, cid)


@route_to("POST", r"/api/complaints/(\d+)/assign-worker", roles=("DEPT_OFFICER",))
def api_assign_worker(h, cid):
    complaint_for(h, cid)
    wid = int_param(h.read_body().get("worker_id"), "worker_id", 1)
    out = engine("assign-worker", cid, wid, h.user["username"], write=True)
    return after_write(h, cid, {"route": out.get("route")})


@route_to("POST", r"/api/complaints/(\d+)/start", roles=("WORKER",))
def api_start(h, cid):
    complaint_for(h, cid)
    out = engine("start", cid, h.user["id"], write=True)
    return after_write(h, cid, {"route": out.get("route")})


@route_to("POST", r"/api/complaints/(\d+)/complete", roles=("WORKER",))
def api_complete(h, cid):
    complaint_for(h, cid)
    engine("complete", cid, h.user["id"], write=True)
    return after_write(h, cid)


# ── management ──

@route_to("GET", r"/api/queue", roles=("ADMIN",))
def api_queue(h):
    limit = int_param(h.query.get("limit") or 50, "limit", 1, 2000)
    out = engine("queue", limit)
    by_id = {c["id"]: c for c in STORE.get("complaints")}
    users = STORE.users_by_id()
    items = [complaint_json(by_id[i], h.user, users) for i in out["ids"] if i in by_id]
    return {"items": items, "total": out["total"]}


@route_to("GET", r"/api/stats")
def api_stats(h):
    scope = {"ADMIN": "all", "DEPT_OFFICER": h.user["department"],
             "WORKER": "my tasks"}.get(h.user["role"], "my complaints")
    return compute_stats(visible_complaints(h.user), scope)


@route_to("GET", r"/api/departments", roles=("ADMIN", "DEPT_OFFICER"))
def api_departments(h):
    users = STORE.get("users")
    by_name = {u["username"]: u for u in users}
    complaints = STORE.get("complaints")
    out = []
    for d in STORE.get("departments"):
        group = [c for c in complaints if c["department"] == d["name"]]
        officer = by_name.get(d["officer_username"])
        out.append({
            "id": d["id"], "name": d["name"], "officer_username": d["officer_username"],
            "officer_name": officer["full_name"] if officer else None,
            "total": len(group),
            "open": sum(1 for c in group if c["status"] != "COMPLETED"),
            "completed": sum(1 for c in group if c["status"] == "COMPLETED"),
            "workers": sum(1 for u in users if u["role"] == "WORKER" and u["department"] == d["name"]),
        })
    return out


@route_to("GET", r"/api/workers", roles=("ADMIN", "DEPT_OFFICER"))
def api_workers(h):
    dept = h.user["department"] if h.user["role"] == "DEPT_OFFICER" else (h.query.get("department") or None)
    complaints = STORE.get("complaints")
    out = []
    for u in STORE.get("users"):
        if u["role"] != "WORKER" or (dept and u["department"] != dept):
            continue
        mine = [c for c in complaints if c["worker_id"] == u["id"]]
        out.append({
            "id": u["id"], "username": u["username"], "full_name": u["full_name"],
            "department": u["department"], "location_id": u["location_id"],
            "location": location_name(u["location_id"]), "is_available": u["is_available"],
            "active_tasks": sum(1 for c in mine if c["status"] != "COMPLETED"),
            "completed_tasks": sum(1 for c in mine if c["status"] == "COMPLETED"),
        })
    return out


@route_to("GET", r"/api/logs", roles=("ADMIN",))
def api_logs(h):
    limit = int_param(h.query.get("limit") or 100, "limit", 1, 1000)
    return list(reversed(STORE.get("logs")))[:limit]


@route_to("GET", r"/api/compare", roles=("ADMIN", "DEPT_OFFICER"))
def api_compare(h):
    a = complaint_for(h, int_param(h.query.get("a"), "a"))
    b_id = int_param(h.query.get("b"), "b")
    # The original a duplicate was linked to may be in another department;
    # comparing the pair only returns similarity numbers, so it is allowed
    # for the linked pair (it used to fail with 404 for officers)
    if a["duplicate_of"] == b_id and STORE.complaint(b_id):
        b = STORE.complaint(b_id)
    else:
        b = complaint_for(h, b_id)
    out = engine("compare", a["id"], b["id"])
    out.pop("ok", None)
    return out


# ═══════════════════════════════════════════════════════════════════
#  Entry point
# ═══════════════════════════════════════════════════════════════════

class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    # On Windows SO_REUSEADDR lets a second server bind the same port, and
    # requests then alternate between two processes with separate sessions
    allow_reuse_address = os.name != "nt"
    request_queue_size = 64   # the page loads ~18 JS modules in parallel
    allowed_hosts = None      # set in main(): None = any Host header


def main():
    ap = argparse.ArgumentParser(description="Smart Public Complaint Analyzer - website")
    ap.add_argument("--host", default="127.0.0.1", help="interface to bind (default 127.0.0.1)")
    ap.add_argument("--port", type=int, default=8000)
    ap.add_argument("--open", action="store_true", help="open the site in the default browser")
    ap.add_argument("--verbose", action="store_true", help="log every request")
    args = ap.parse_args()

    if not os.path.exists(CLI):
        print(f"error: {CLI} not found. Build it first:  mingw32-make", file=sys.stderr)
        return 1
    engine("refresh-departments", write=True)   # also creates data/ files if missing
    graph()

    srv = Server((args.host, args.port), Handler)
    srv.verbose = args.verbose
    srv.demo = args.host in ("127.0.0.1", "localhost", "::1")
    if srv.demo:   # on this computer only: accept requests for our own names only
        srv.allowed_hosts = {f"{name}:{args.port}" for name in ("localhost", "127.0.0.1", "[::1]")}
        if args.port == 80:
            srv.allowed_hosts |= {"localhost", "127.0.0.1", "[::1]"}
    url = f"http://{'localhost' if srv.demo else args.host}:{args.port}/"
    print(f"Smart Public Complaint Analyzer running at {url}  (Ctrl+C to stop)")
    print(f"Data: {DATA}")
    if not srv.demo:
        print("Note: bound to a network interface - demo accounts are hidden. "
              "Passwords are stored in plain text; do not expose this server publicly.")
    if args.open:
        threading.Timer(0.6, lambda: webbrowser.open(url)).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        srv.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
