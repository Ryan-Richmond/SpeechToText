#!/usr/bin/env python3
"""
vox-prompt-eval: score prompt-eval results from the Swift harness.

Usage:
    python3 score.py --results <path-to-results.json> [--threshold 0.85]

Input JSON schema (one object per golden):
    {
      "id": "001_filler_removal",
      "category": "filler_removal",
      "input": "um so like i think we should meet at 3pm",
      "reference": "I think we should meet at 3pm.",
      "got": "I think we should meet at 3pm."
    }

Exit code: 0 on pass, 1 on any category below threshold.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Iterable


def normalize(s: str) -> str:
    s = s.strip().lower()
    s = re.sub(r"[^\w\s]", "", s)
    s = re.sub(r"\s+", " ", s)
    return s


def edit_distance(a: str, b: str) -> int:
    if a == b:
        return 0
    if not a:
        return len(b)
    if not b:
        return len(a)
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        curr = [i]
        for j, cb in enumerate(b, 1):
            curr.append(
                min(
                    curr[-1] + 1,
                    prev[j] + 1,
                    prev[j - 1] + (ca != cb),
                )
            )
        prev = curr
    return prev[-1]


def norm_edit(a: str, b: str) -> float:
    if not a and not b:
        return 0.0
    return edit_distance(a, b) / max(len(a), len(b))


def tokens(s: str) -> set[str]:
    return set(normalize(s).split())


def case_pass(input_: str, reference: str, got: str) -> tuple[bool, str]:
    ref_n, got_n = normalize(reference), normalize(got)
    if ref_n == got_n:
        return True, "exact"

    dist = norm_edit(ref_n, got_n)
    if dist > 0.10:
        return False, f"edit_dist={dist:.2f} > 0.10"

    # No hallucinated content: every got-token must appear in input or reference.
    allowed = tokens(input_) | tokens(reference)
    extra = tokens(got) - allowed
    if extra:
        return False, f"hallucinated: {sorted(extra)}"

    return True, f"close (edit_dist={dist:.2f})"


def main(argv: Iterable[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", type=Path, required=True)
    ap.add_argument("--threshold", type=float, default=0.85)
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args(list(argv))

    if not args.results.exists():
        print(f"error: results file not found: {args.results}", file=sys.stderr)
        return 2

    data = json.loads(args.results.read_text())

    by_cat: dict[str, list[tuple[str, bool, str]]] = defaultdict(list)
    for row in data:
        ok, reason = case_pass(row["input"], row["reference"], row["got"])
        by_cat[row.get("category", "uncategorized")].append((row["id"], ok, reason))

    print("== vox-prompt-eval ==")
    overall_pass = 0
    overall_total = 0
    failed_categories = 0

    for cat, cases in sorted(by_cat.items()):
        passed = sum(1 for _, ok, _ in cases if ok)
        total = len(cases)
        rate = passed / total if total else 1.0
        status = "PASS" if rate >= args.threshold else "FAIL"
        if status == "FAIL":
            failed_categories += 1
        print(f"[{status}] {cat}: {passed}/{total} ({rate:.0%})")
        if args.verbose or status == "FAIL":
            for cid, ok, reason in cases:
                if not ok:
                    print(f"    - {cid}: {reason}")
        overall_pass += passed
        overall_total += total

    overall_rate = overall_pass / overall_total if overall_total else 1.0
    print()
    print(f"Overall: {overall_pass}/{overall_total} ({overall_rate:.0%})")
    print(f"Result: {'PASS' if failed_categories == 0 else 'FAIL'}")
    return 0 if failed_categories == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
