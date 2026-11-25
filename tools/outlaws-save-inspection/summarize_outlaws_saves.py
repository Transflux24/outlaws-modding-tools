from __future__ import annotations

import csv
import pathlib
import re
import sys


PATTERN = re.compile(
    rb"myBuildCL = (?P<build>\d+), myIsAutoSave = (?P<autosave>\w+), "
    rb"myLastGalaxyLocation = #(?P<location>[0-9A-F]+), "
    rb"myLastProgressedQuest = #(?P<quest>[0-9A-F]+), .*?"
    rb"myPlaytimeInMinutes = l(?P<minutes>\d+), "
    rb"myPlaytimeInSeconds = l(?P<seconds>\d+), .*?"
    rb"myUTCTimestamp = (?P<timestamp>\d+)"
)


def main(source_dir: pathlib.Path, output_csv: pathlib.Path) -> None:
    rows = []
    for path in sorted(source_dir.glob("*.raw")):
        match = PATTERN.search(path.read_bytes())
        if not match:
            continue
        row = {key: value.decode("ascii") for key, value in match.groupdict().items()}
        row["file"] = path.name[:-4] if path.name.endswith(".raw") else path.name
        seconds = int(row["seconds"])
        row["playtime"] = f"{seconds // 3600}:{seconds % 3600 // 60:02d}:{seconds % 60:02d}"
        rows.append(row)

    fieldnames = ["file", "playtime", "autosave", "quest", "location", "build", "timestamp"]
    output_csv.parent.mkdir(parents=True, exist_ok=True)
    with output_csv.open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)

    for row in rows:
        print("\t".join(row[field] for field in ("file", "playtime", "quest", "location")))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: summarize_outlaws_saves.py RAW_DIR OUTPUT_CSV")
    main(pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]))
