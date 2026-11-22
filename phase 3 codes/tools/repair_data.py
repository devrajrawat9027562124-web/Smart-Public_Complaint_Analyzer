"""
repair_data.py — fix structural problems in the data/ files.

Run from anywhere:   python tools/repair_data.py [--dry-run]

What it fixes (each step is idempotent, so running it twice is safe):
  1. users.txt      - removes a UTF-8 byte-order mark (it hid the first account
                      from the C parser), drops exact duplicate accounts (same
                      username), and gives a new ID to accounts that share an ID
                      with a different account. Complaints that the activity log
                      shows were submitted by a re-numbered account are moved to
                      its new ID.
  2. complaints.txt - gives a new ID to a complaint that reuses an existing ID;
                      makes completion dates consistent (a COMPLETED complaint
                      cannot finish before it was submitted; other statuses
                      have no completion date).
  3. departments.txt - recomputed by `engine_cli refresh-departments` (run after).

A timestamped copy of data/ is written to _backup_<timestamp>/ first.
It does NOT change severities, statuses, categories or text.
"""

import datetime as dt
import hashlib
import os
import re
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data")
USERS = os.path.join(DATA, "users.txt")
COMPLAINTS = os.path.join(DATA, "complaints.txt")
LOGS = os.path.join(DATA, "logs.txt")
DATE_FMT = "%Y-%m-%d %H:%M"


def read_rows(path):
    with open(path, encoding="utf-8-sig") as f:  # utf-8-sig strips a BOM
        return [line.rstrip("\r\n").split("|") for line in f if line.strip()]


def write_rows(path, rows):
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        for r in rows:
            f.write("|".join(r) + "\n")
    os.replace(tmp, path)


def parse_date(s):
    try:
        return dt.datetime.strptime(s.strip(), DATE_FMT)
    except ValueError:
        return None


def stable_offset(cid):
    """Deterministic 1-21 day, 0-8 hour offset derived from the complaint ID."""
    h = int(hashlib.sha1(str(cid).encode()).hexdigest(), 16)
    return dt.timedelta(days=1 + h % 21, hours=(h // 21) % 9, minutes=(h // 189) % 60)


def main():
    dry_run = "--dry-run" in sys.argv
    report = []
    now = dt.datetime.now().replace(second=0, microsecond=0)

    with open(USERS, "rb") as f:
        had_bom = f.read(3) == b"\xef\xbb\xbf"
    users = read_rows(USERS)
    complaints = read_rows(COMPLAINTS)
    log_lines = open(LOGS, encoding="utf-8-sig").read().splitlines() if os.path.exists(LOGS) else []

    if had_bom:
        report.append("users.txt: removed the UTF-8 byte-order mark (first account was unreadable)")

    # ── 1a. exact duplicate accounts (same username) ──
    seen, unique_users, dropped = set(), [], []
    for u in users:
        if len(u) < 9:
            unique_users.append(u)
            continue
        if u[1] in seen:
            dropped.append(u[1])
            continue
        seen.add(u[1])
        unique_users.append(u)
    users = unique_users
    if dropped:
        report.append(f"users.txt: dropped {len(dropped)} repeated account rows (same username listed twice, "
                      f"e.g. '{dropped[0]}')")

    # ── who did what, according to the activity log ──
    submitted_by = {}      # (complaint_id, title) -> username
    active_users = set()   # usernames with any logged activity
    for line in log_lines:
        m = re.search(r"User '([^']+)' submitted complaint #(\d+): (.*)$", line)
        if m:
            submitted_by[(m.group(2), m.group(3).strip())] = m.group(1)
        m = re.search(r"(?:User|Admin|Officer|Worker) '([^']+)'", line)
        if m:
            active_users.add(m.group(1))

    # ── 1b. different accounts sharing one ID ──
    by_id = {}
    for u in users:
        by_id.setdefault(u[0], []).append(u)
    next_id = max(int(u[0]) for u in users if u[0].isdigit()) + 1
    moved = {}   # username -> (old id, new id)
    for uid, group in by_id.items():
        if len(group) < 2:
            continue
        # Generated accounts have no logged activity; accounts that people
        # actually used do. The account with no activity keeps the ID (the
        # generated complaints point at it); the others are re-numbered.
        quiet = [u for u in group if u[1] not in active_users]
        keeper = quiet[0] if len(quiet) == 1 else group[0]
        for u in group:
            if u is keeper:
                continue
            moved[u[1]] = (uid, str(next_id))
            report.append(f"users.txt: '{u[1]}' shared id {uid} with '{keeper[1]}' -> new id {next_id}")
            u[0] = str(next_id)
            next_id += 1

    # ── 2a. complaints owned by re-numbered accounts (from the log) ──
    for c in complaints:
        owner = submitted_by.get((c[0], c[2].strip()))
        if owner in moved and c[1] == moved[owner][0]:
            report.append(f"complaints.txt: #{c[0]} '{c[2]}' belongs to '{owner}' -> user id {moved[owner][1]}")
            c[1] = moved[owner][1]

    # ── 2b. duplicate complaint IDs ──
    max_cid = max(int(c[0]) for c in complaints if c[0].isdigit())
    seen_ids = set()
    for c in complaints:
        if c[0] in seen_ids:
            max_cid += 1
            report.append(f"complaints.txt: second complaint #{c[0]} '{c[2]}' -> #{max_cid}")
            c[0] = str(max_cid)
        seen_ids.add(c[0])

    # ── 2c. completion dates ──
    fixed_dates = 0
    for c in complaints:
        if len(c) < 17:
            continue
        status, sub, done = c[12], parse_date(c[15]), parse_date(c[16])
        if status != "COMPLETED":
            if c[16] != "N/A":
                c[16] = "N/A"
                fixed_dates += 1
            continue
        if sub is None:
            continue
        if done is None or done < sub:
            new_done = min(sub + stable_offset(c[0]), now)
            if new_done <= sub:
                new_done = sub + dt.timedelta(hours=1)
            c[16] = new_done.strftime(DATE_FMT)
            fixed_dates += 1
    if fixed_dates:
        report.append(f"complaints.txt: fixed {fixed_dates} completion date(s) "
                      "(COMPLETED before submission, or a date on an unfinished complaint)")

    if not report:
        print("Nothing to repair - data/ is already consistent.")
        return 0

    print("\n".join("  - " + r for r in report))
    if dry_run:
        print("\nDry run: no files were changed.")
        return 0

    stamp = now.strftime("%Y%m%d-%H%M")
    backup = os.path.join(ROOT, f"_backup_{stamp}")
    if not os.path.exists(backup):
        shutil.copytree(DATA, os.path.join(backup, "data"))
    write_rows(USERS, users)
    write_rows(COMPLAINTS, complaints)
    with open(LOGS, "a", encoding="utf-8") as f:
        f.write(f"[{now.strftime(DATE_FMT)}] Data repaired by tools/repair_data.py ({len(report)} fixes)\n")
    print(f"\nRepaired. Backup of the previous files: {backup}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
