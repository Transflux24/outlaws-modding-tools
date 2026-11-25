#!/usr/bin/env python3
"""Summarize the newest session in an Outlaws Event Observer TSV trace."""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from pathlib import Path
from statistics import median


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()
    lines = args.trace.read_text(encoding="utf-8", errors="replace").splitlines()
    starts = [index for index, line in enumerate(lines) if line.startswith("# session ")]
    if not starts:
        raise SystemExit("no observer sessions found")
    start = starts[-1]
    session = lines[start]
    records = []
    for line in lines[start + 1 :]:
        if not line or line.startswith("sequence") or line.startswith("#"):
            continue
        columns = line.split("\t")
        if len(columns) != 7:
            continue
        records.append(
            {
                "sequence": int(columns[0]),
                "us": int(columns[1]),
                "thread": int(columns[2]),
                "event": columns[3],
                "self": columns[4],
                "context": columns[5],
                "caller": columns[6],
            }
        )
    if not records:
        raise SystemExit("newest observer session contains no records")

    ordinary = [record for record in records if not record["event"].startswith("MANUAL_MARKER_")]
    markers = [record for record in records if record["event"].startswith("MANUAL_MARKER_")]
    print(session)
    print(
        f"records={len(records)} duration={(records[-1]['us'] - records[0]['us']) / 1_000_000:.3f}s "
        f"markers={len(markers)}"
    )
    counts = Counter(record["event"] for record in ordinary)
    by_event = defaultdict(list)
    for record in ordinary:
        by_event[record["event"]].append(record)
    print("\nEVENT SUMMARY")
    for event, count in counts.most_common():
        items = by_event[event]
        instances = len({item["self"] for item in items})
        contexts = len({item["context"] for item in items})
        gaps = [b["us"] - a["us"] for a, b in zip(items, items[1:]) if b["us"] >= a["us"]]
        gap_text = f"{median(gaps) / 1000:.3f}ms" if gaps else "n/a"
        print(
            f"{event:28s} count={count:6d} instances={instances:3d} "
            f"contexts={contexts:5d} median_gap={gap_text}"
        )

    windows = [(-2_000_000, 0, "previous 2s"), (-500_000, 500_000, "+/-0.5s"), (0, 2_000_000, "next 2s")]
    print("\nMARKER WINDOWS")
    for marker in markers:
        print(f"{marker['event']} at {marker['us'] / 1_000_000:.3f}s")
        for lower, upper, label in windows:
            window_counts = Counter(
                record["event"]
                for record in ordinary
                if marker["us"] + lower <= record["us"] < marker["us"] + upper
            )
            compact = ", ".join(f"{name}={value}" for name, value in sorted(window_counts.items()))
            print(f"  {label:12s} {compact}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
