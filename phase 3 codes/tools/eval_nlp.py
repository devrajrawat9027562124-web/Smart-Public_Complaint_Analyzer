"""Measure the C engine's text analysis against hand-labelled examples.

The reference is the 150 complaint templates in generate_dataset.py, each
with a severity chosen by hand. Run (after `mingw32-make eval`):

    python tools/eval_nlp.py

Caveat: the labels and the keyword dictionaries were written by the same
people, and the templates are also the source of the sample data, so these
numbers say how consistent the rules are with that judgement, not how they
would do on real complaints. Use them to compare versions of the rules.
"""
import ast
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "bin", "eval_nlp.exe" if os.name == "nt" else "eval_nlp")
GROUPS = {"WATER_COMPLAINTS": "Water Supply", "ELECTRICITY_COMPLAINTS": "Electricity",
          "ROADS_COMPLAINTS": "Roads & Transport", "SANITATION_COMPLAINTS": "Sanitation",
          "SAFETY_COMPLAINTS": "Public Safety"}


def load_templates():
    """(title, description, category, severity) from generate_dataset.py, read without running it."""
    with open(os.path.join(ROOT, "generate_dataset.py"), encoding="utf-8") as f:
        tree = ast.parse(f.read())
    out = []
    for node in tree.body:
        if isinstance(node, ast.Assign) and getattr(node.targets[0], "id", None) in GROUPS:
            for title, desc, severity in ast.literal_eval(node.value):
                out.append((title, desc, GROUPS[node.targets[0].id], severity))
    return out


def spearman(xs, ys):
    def ranks(v):
        order = sorted(range(len(v)), key=lambda i: v[i])
        r, i = [0.0] * len(v), 0
        while i < len(v):
            j = i
            while j + 1 < len(v) and v[order[j + 1]] == v[order[i]]:
                j += 1
            for k in range(i, j + 1):
                r[order[k]] = (i + j) / 2
            i = j + 1
        return r
    a, b = ranks(xs), ranks(ys)
    ma, mb = sum(a) / len(a), sum(b) / len(b)
    cov = sum((x - ma) * (y - mb) for x, y in zip(a, b))
    return cov / (sum((x - ma) ** 2 for x in a) ** 0.5 * sum((y - mb) ** 2 for y in b) ** 0.5)


def main():
    if not os.path.exists(EXE):
        print(f"{EXE} not found. Build it first:  mingw32-make eval", file=sys.stderr)
        return 1
    templates = load_templates()
    text = "".join(f"{t}\t{d}\n" for t, d, _, _ in templates)
    out = subprocess.run([EXE], input=text.encode("utf-8"), capture_output=True, check=True).stdout.decode()
    pred = [(int(s), c, int(i)) for s, c, i in (line.split("\t") for line in out.splitlines())]

    n = len(templates)
    labels = [t[3] for t in templates]
    sev = [p[0] for p in pred]
    missed = [t[0] for t, s in zip(templates, sev) if t[3] >= 9 and s <= 4]
    print(f"Hand-labelled templates: {n}")
    print(f"Severity  mean absolute error {sum(abs(a - b) for a, b in zip(sev, labels)) / n:.2f}"
          f"   within +-1: {sum(abs(a - b) <= 1 for a, b in zip(sev, labels)) / n:.0%}"
          f"   Spearman {spearman(sev, labels):.2f}")
    print(f"No severity keyword found (score 1): {sev.count(1)}")
    print(f"Critical (9-10) scored 4 or less: {len(missed)} {missed[:5]}")
    print(f"Category suggestion correct: {sum(p[1] == t[2] for p, t in zip(pred, templates)) / n:.0%}")
    print(f"Real complaints flagged as spam: {sum(p[2] for p in pred)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
