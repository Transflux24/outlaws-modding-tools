from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
from pathlib import Path
import re
import struct
from typing import Dict, Iterable, List, Optional

from .scalars import decode_varuint32


MAGIC = b"\xffBV2"
SUPPORTED_EXTENSIONS = {".muigraph", ".mcompoundnode"}
_STRING_TABLE_RE = re.compile(rb"(?:[\x20-\x7e]+\x00){4,}")


class BV2FormatError(ValueError):
    pass


@dataclass(frozen=True)
class StringTable:
    offset: int
    end: int
    count: int
    byte_length: int
    detection: str
    first_strings: List[str]
    last_strings: List[str]


@dataclass(frozen=True)
class BV2Probe:
    path: str
    size: int
    sha256: str
    magic_hex: str
    version_label: str
    initial_sequence_count: int
    initial_count_end_offset: int
    string_table: Optional[StringTable]
    marker_offsets: Dict[str, List[int]]
    raw_float32_offsets: Dict[str, List[int]]

    def to_dict(self) -> dict:
        return asdict(self)


def _all_offsets(data: bytes, needle: bytes) -> List[int]:
    offsets: List[int] = []
    start = 0
    while True:
        offset = data.find(needle, start)
        if offset < 0:
            return offsets
        offsets.append(offset)
        start = offset + 1


def _detect_string_table(data: bytes) -> Optional[StringTable]:
    # The two Outlaws BV2 graph layouts in the research corpus begin their
    # interned string tables with one of these root keys. Starting at the root
    # avoids treating printable bytes at the end of the structural prefix as
    # part of the first string.
    for anchor in (b"ConstantNodesData\x00", b"connectionsById\x00"):
        start = data.find(anchor)
        if start >= 0:
            strings: List[str] = []
            cursor = start
            while cursor < len(data):
                end = data.find(b"\x00", cursor)
                if end < 0 or end == cursor:
                    break
                raw = data[cursor:end]
                if any(byte < 0x20 or byte > 0x7E for byte in raw):
                    break
                strings.append(raw.decode("ascii"))
                cursor = end + 1
            if len(strings) >= 4:
                return StringTable(
                    offset=start,
                    end=cursor,
                    count=len(strings),
                    byte_length=cursor - start,
                    detection="known-root-key",
                    first_strings=strings[:8],
                    last_strings=strings[-8:],
                )

    matches = list(_STRING_TABLE_RE.finditer(data))
    if not matches:
        return None

    # BV2 keeps its interned printable strings in one long NUL-delimited run.
    # Choosing the match with the most separators is more robust than assuming
    # a fixed offset, which varies substantially by graph.
    match = max(matches, key=lambda item: (item.group(0).count(b"\x00"), len(item.group(0))))
    strings = [
        item.decode("ascii")
        for item in match.group(0).split(b"\x00")
        if item
    ]
    return StringTable(
        offset=match.start(),
        end=match.end(),
        count=len(strings),
        byte_length=match.end() - match.start(),
        detection="longest-printable-run",
        first_strings=strings[:8],
        last_strings=strings[-8:],
    )


def probe_bytes(data: bytes, path_label: str = "<memory>") -> BV2Probe:
    if len(data) < len(MAGIC) or data[:4] != MAGIC:
        observed = data[:4].hex(" ") if data else "<empty>"
        raise BV2FormatError(
            f"{path_label}: expected FFBV2 magic {MAGIC.hex(' ')}, observed {observed}"
        )

    markers = (
        "UI_HitFeedback",
        "native:MultiSelect2",
        "HC_CombatNode_DamageDealt",
        "crosshair_damage_feedback.mcompoundnode",
    )
    marker_offsets = {
        marker: _all_offsets(data, marker.encode("ascii")) for marker in markers
    }

    # These observations prevent a future patcher from assuming that a
    # semantic float can be found by blind byte replacement.
    float_offsets = {
        label: _all_offsets(data, struct.pack("<f", value))
        for label, value in (("0.0", 0.0), ("0.75", 0.75), ("1.0", 1.0), ("1.5", 1.5))
    }

    initial_sequence_count, count_end = decode_varuint32(data, 4)

    return BV2Probe(
        path=path_label,
        size=len(data),
        sha256=hashlib.sha256(data).hexdigest(),
        magic_hex=data[:4].hex(" "),
        version_label="FFBV2",
        initial_sequence_count=initial_sequence_count,
        initial_count_end_offset=count_end,
        string_table=_detect_string_table(data),
        marker_offsets=marker_offsets,
        raw_float32_offsets=float_offsets,
    )


def probe_file(path: Path) -> BV2Probe:
    path = path.resolve()
    return probe_bytes(path.read_bytes(), str(path))


def iter_bv2_files(root: Path) -> Iterable[Path]:
    root = root.resolve()
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix.lower() in SUPPORTED_EXTENSIONS:
            yield path

