#!/usr/bin/env python3
"""Deduplicate and summarize the newest Outlaws observer state session."""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()
    lines = args.trace.read_text(encoding="utf-8", errors="replace").splitlines()
    starts = [index for index, line in enumerate(lines) if line.startswith("# session ")]
    if not starts:
        raise SystemExit("no sessions found")
    start = starts[-1]
    rows = []
    for line in lines[start + 1 :]:
        if not line or line.startswith("sequence") or line.startswith("#"):
            continue
        columns = line.split("\t", 6)
        if len(columns) != 7:
            continue
        rows.append(
            {
                "seq": int(columns[0]),
                "us": int(columns[1]),
                "thread": int(columns[2]),
                "state": columns[3],
                "self": columns[4],
                "context": columns[5],
                "details": columns[6],
            }
        )
    last = {}
    deduped = []
    for row in rows:
        mask = row["details"].split(";", 1)[0] if row["details"].startswith("mask=") else ""
        key = (row["state"], row["self"], mask)
        signature = row["details"]
        if row["state"].startswith("MANUAL_MARKER_") or last.get(key) != signature:
            deduped.append(row)
            last[key] = signature

    print(lines[start])
    print(f"raw_rows={len(rows)} deduped_rows={len(deduped)}")
    print("\nDEDUPED COUNTS")
    for state, count in Counter(row["state"] for row in deduped).most_common():
        unique = len({row["details"] for row in deduped if row["state"] == state})
        print(f"{state:30s} rows={count:5d} unique_details={unique:4d}")

    markers = [row for row in deduped if row["state"].startswith("MANUAL_MARKER_")]
    print("\nMARKER NEIGHBORHOODS (deduplicated transitions, previous/next 1.0s)")
    for marker in markers:
        nearby = [
            row for row in deduped
            if not row["state"].startswith("MANUAL_MARKER_")
            and abs(row["us"] - marker["us"]) <= 1_000_000
        ]
        print(f"\n{marker['state']} at {marker['us'] / 1_000_000:.6f}s")
        for row in nearby:
            delta = (row["us"] - marker["us"]) / 1_000_000
            print(f"  {delta:+.6f}s {row['state']} {row['details']}")

    print("\nALL NON-GRAPHICS TRANSITIONS")
    for row in deduped:
        if row["state"].startswith("Graphics"):
            continue
        print(f"{row['us'] / 1_000_000:.6f}\t{row['state']}\t{row['details']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
