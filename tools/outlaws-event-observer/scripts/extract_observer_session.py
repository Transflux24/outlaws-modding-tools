#!/usr/bin/env python3
"""Extract one observer process session from the live append-only logs."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import List


FILENAMES = (
    "outlaws_event_observer.log",
    "outlaws_event_observer_events.tsv",
    "outlaws_event_observer_states.tsv",
    "outlaws_event_observer_snapshots.tsv",
    "outlaws_event_observer_consumers.tsv",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def extract_tsv(lines: List[str], pid: int) -> List[str]:
    marker = "pid={}".format(pid)
    starts = [
        index
        for index, line in enumerate(lines)
        if line.startswith("# session ") and marker in line
    ]
    if len(starts) != 1:
        raise ValueError(
            "expected one TSV session for pid {}; found {}".format(pid, len(starts))
        )
    start = starts[0]
    end = next(
        (
            index
            for index in range(start + 1, len(lines))
            if lines[index].startswith("# session ")
        ),
        len(lines),
    )
    return lines[start:end]


def extract_log(lines: List[str], pid: int) -> List[str]:
    marker = "Outlaws Event Observer session start (pid {})".format(pid)
    starts = [index for index, line in enumerate(lines) if marker in line]
    if len(starts) != 1:
        raise ValueError(
            "expected one text-log session for pid {}; found {}".format(
                pid, len(starts)
            )
        )
    start = starts[0]
    end = next(
        (
            index
            for index in range(start + 1, len(lines))
            if "Outlaws Event Observer session start (pid " in lines[index]
        ),
        len(lines),
    )
    return lines[start:end]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_directory", type=Path)
    parser.add_argument("output_directory", type=Path)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--label", required=True)
    args = parser.parse_args()

    source_directory = args.source_directory.resolve()
    output_directory = args.output_directory.resolve()
    if output_directory.exists():
        raise SystemExit("refusing to overwrite existing directory: {}".format(output_directory))
    output_directory.mkdir(parents=True)

    manifest = {
        "pid": args.pid,
        "test_label": args.label,
        "source_directory": str(source_directory),
        "files": [],
    }
    for filename in FILENAMES:
        source = source_directory / filename
        if not source.is_file():
            raise SystemExit("missing observer source file: {}".format(source))
        lines = source.read_text(encoding="utf-8", errors="replace").splitlines()
        extracted = (
            extract_log(lines, args.pid)
            if source.suffix == ".log"
            else extract_tsv(lines, args.pid)
        )
        if filename in {
            "outlaws_event_observer.log",
            "outlaws_event_observer_snapshots.tsv",
        } and not any(args.label in line for line in extracted):
            raise SystemExit(
                "session {} in {} does not contain label {!r}".format(
                    args.pid, filename, args.label
                )
            )
        destination = output_directory / filename
        destination.write_text("\n".join(extracted) + "\n", encoding="utf-8")
        manifest["files"].append(
            {
                "name": filename,
                "source_size": source.stat().st_size,
                "source_sha256": sha256(source),
                "extracted_lines": len(extracted),
                "extracted_size": destination.stat().st_size,
                "extracted_sha256": sha256(destination),
            }
        )

    manifest_path = output_directory / "archive_manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print("archived pid={} label={} files={}".format(args.pid, args.label, len(FILENAMES)))
    for entry in manifest["files"]:
        print(
            "{} lines={} bytes={} sha256={}".format(
                entry["name"],
                entry["extracted_lines"],
                entry["extracted_size"],
                entry["extracted_sha256"],
            )
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
