"""Concurrency test for the shared data files.

Runs many engine_cli processes at the same time - as the website does under
load, alongside the console app - against a temporary COPY of data/ (your
real data is never touched), then checks that nothing was lost:

  1. 60 location updates in parallel with 60 read-only calls:
     users.txt must keep every account.
  2. 200 severity changes in parallel with 80 submissions: every
     submission must be saved exactly once, with its own new ID.

    python tools/stress_test.py          (after mingw32-make)

Exit status 1 if a check fails.
"""
import collections
import os
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLI = os.path.join(ROOT, "bin", "engine_cli.exe" if os.name == "nt" else "engine_cli")


def main():
    if not os.path.exists(CLI):
        print(f"{CLI} not found. Build it first:  mingw32-make", file=sys.stderr)
        return 1
    work = tempfile.mkdtemp(prefix="spca-stress-")
    try:
        shutil.copytree(os.path.join(ROOT, "data"), os.path.join(work, "data"),
                        ignore=shutil.ignore_patterns(".lock", "*.tmp"))
        env = dict(os.environ, SPCA_ROOT=work)

        def run(args, stdin=""):
            p = subprocess.run([CLI, *args], input=stdin.encode(), capture_output=True, env=env)
            return p.stdout.decode("utf-8", "replace")

        def lines(name):
            with open(os.path.join(work, "data", name), encoding="utf-8") as f:
                return [l for l in f.read().splitlines() if l.strip()]

        users = [l.split("|") for l in lines("users.txt")]
        worker = next(u for u in users if u[6] == "WORKER")[0]
        admin = next(u for u in users if u[6] == "ADMIN")[1]
        citizen = next(u for u in users if u[6] == "USER")
        target = next(l.split("|")[0] for l in lines("complaints.txt") if l.split("|")[12] != "COMPLETED")
        ok = True

        before = len(lines("users.txt"))
        jobs = [["set-location", worker, str(i % 18)] for i in range(60)] + [["graph"] for _ in range(60)]
        with ThreadPoolExecutor(32) as pool:
            list(pool.map(run, jobs))
        after = len(lines("users.txt"))
        passed = after == before
        ok &= passed
        print(f"{'PASS' if passed else 'FAIL'}  accounts kept under parallel updates: {before} -> {after}")

        before = lines("complaints.txt")
        top = max(int(l.split("|", 1)[0]) for l in before)

        def job(i):
            if i < 200:
                return "e", run(["escalate", target, str(i % 10 + 1), admin])
            k = i - 200
            return "s", run(["submit"], f"{citizen[0]}\n{citizen[1]}\n{k % 18}\nWater Supply\n"
                                        f"Stress test pipe burst {k}\nWater pipe {k} burst near the market, road flooded\n")

        with ThreadPoolExecutor(32) as pool:
            results = list(pool.map(job, range(280)))
        after = lines("complaints.txt")
        saved = sum(1 for kind, r in results if kind == "s" and '"ok":true' in r)
        ids = [int(l.split("|", 1)[0]) for l in after]
        repeated = sorted(i for i, n in collections.Counter(ids).items() if n > 1)
        passed = saved == 80 and len(after) - len(before) == 80 and not repeated and max(ids) == top + 80
        ok &= passed
        print(f"{'PASS' if passed else 'FAIL'}  80 parallel submissions: {saved} saved, "
              f"{len(after) - len(before)} new rows, repeated IDs: {repeated or 'none'}")
        return 0 if ok else 1
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
