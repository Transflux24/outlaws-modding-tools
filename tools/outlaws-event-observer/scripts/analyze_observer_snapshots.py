#!/usr/bin/env python3
"""Summarize and validate Outlaws Event Observer structured snapshots."""

from __future__ import annotations

import argparse
import csv
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

SUPPORTED_COVERAGE = {
    "combat_ui_only",
    "combat_ui_interaction_nodes",
    "combat_ui_interaction_callbacks",
}


@dataclass
class Session:
    heading: str
    rows: list[dict[str, str]] = field(default_factory=list)


def parse_sessions(path: Path) -> list[Session]:
    sessions: list[Session] = []
    current: Session | None = None
    header: list[str] | None = None

    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        for raw_line in stream:
            line = raw_line.rstrip("\r\n")
            if not line:
                continue
            if line.startswith("# session "):
                current = Session(line[2:])
                sessions.append(current)
                header = None
                continue
            if line.startswith("sequence\t"):
                header = next(csv.reader([line], delimiter="\t"))
                continue
            if line.startswith("#"):
                continue
            if current is None or header is None:
                raise ValueError(f"data row appeared before a session/header: {line[:80]}")
            values = next(csv.reader([line], delimiter="\t"))
            if len(values) != len(header):
                raise ValueError(
                    f"session {len(sessions)} row has {len(values)} fields; expected {len(header)}"
                )
            current.rows.append(dict(zip(header, values)))
    return sessions


def summarize(sessions: list[Session], expect_smoke: bool) -> int:
    failures = 0
    expected = {
        "observer_ready",
        "manual_F10",
        "capture_start",
        "MANUAL_MARKER_1",
        "capture_stop",
    }

    if not sessions:
        print("No observer sessions found.")
        return 1

    for index, session in enumerate(sessions, start=1):
        print(f"Session {index}: {session.heading}")
        if not session.rows:
            print("  snapshots: 0")
            failures += 1
            continue

        labels = Counter(row.get("test_label", "") for row in session.rows)
        reasons = [row.get("reason", "") for row in session.rows]
        reason_counts = Counter(reasons)
        first = session.rows[0]
        last = session.rows[-1]
        label_text = ", ".join(
            "{} ({})".format(label or "<blank>", count) for label, count in labels.items()
        )
        reason_text = ", ".join(
            "{} ({})".format(reason, count) for reason, count in reason_counts.items()
        )
        print(f"  snapshots: {len(session.rows)}")
        print(f"  labels: {label_text}")
        print(f"  reasons: {reason_text}")
        print(
            "  qpc_us: "
            f"{first.get('qpc_us', '?')} -> {last.get('qpc_us', '?')}  "
            f"events: {first.get('event_sequence', '?')} -> {last.get('event_sequence', '?')}  "
            f"states: {first.get('state_sequence', '?')} -> {last.get('state_sequence', '?')}"
        )
        print(
            "  dropped: "
            f"events={last.get('dropped_events', '?')} "
            f"consumers={last.get('dropped_consumers', '?')} "
            f"coverage={last.get('coverage', '?')}"
        )

        sequences = [int(row["sequence"]) for row in session.rows]
        if sequences != list(range(1, len(sequences) + 1)):
            print(f"  ERROR: non-contiguous snapshot sequence: {sequences}")
            failures += 1
        if len(labels) != 1:
            print("  ERROR: test label changed within one process session")
            failures += 1
        if any(row.get("coverage") not in SUPPORTED_COVERAGE for row in session.rows):
            print("  ERROR: unexpected or missing coverage marker")
            failures += 1
        if expect_smoke:
            missing = sorted(expected.difference(reasons))
            if missing:
                print(f"  ERROR: smoke-test snapshots missing: {', '.join(missing)}")
                failures += 1
            else:
                print("  smoke test: expected snapshots present")
        print()

    if failures:
        print(f"Validation finished with {failures} issue(s).")
        return 1
    print("Validation passed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path, help="path to outlaws_event_observer_snapshots.tsv")
    parser.add_argument(
        "--expect-smoke",
        action="store_true",
        help="require observer_ready, F10, capture start/stop, and marker 1 in every session",
    )
    args = parser.parse_args()
    return summarize(parse_sessions(args.path), args.expect_smoke)


if __name__ == "__main__":
    raise SystemExit(main())
