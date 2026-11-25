from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
from pathlib import Path
from typing import Dict, List, Sequence, Tuple

from .binary import MAGIC, BV2FormatError
from .scalars import decode_low_tag_varint32, decode_varuint32, encode_varuint32
from .sidecar import probe_sidecar, sidecar_for


SHARED_COMPONENT_SHA256 = (
    "987bf7e19c3e90a7d21ce3545daac2861debc475c022e359699624f4fbc5eda0"
)


class TargetTraceError(ValueError):
    pass


@dataclass(frozen=True)
class EncodedIdTrace:
    node_id: int
    encoding_hex: str
    offsets: List[int]


@dataclass(frozen=True)
class HitFeedbackTrace:
    path: str
    sha256: str
    hash_gate: str
    semantic_chain: dict
    encoded_node_ids: Dict[str, EncodedIdTrace]
    connection_run_start: int
    connection_run_end: int
    connection_run_hex: str
    decoded_connections: List[dict]
    reduced_connection_source_offset: int
    status: str
    next_requirement: str

    def to_dict(self) -> dict:
        return asdict(self)


def _all_offsets(data: bytes, needle: bytes, start: int = 0) -> List[int]:
    offsets: List[int] = []
    cursor = start
    while True:
        offset = data.find(needle, cursor)
        if offset < 0:
            return offsets
        offsets.append(offset)
        cursor = offset + 1


def _find_interleaved_run(
    data: bytes,
    node_ids: Sequence[int],
    minimum_gap: int = 1,
    maximum_gap: int = 2,
) -> List[Tuple[int, int]]:
    """Find one ordered run of encoded IDs separated by compact pin values.

    A connection is stored as an encoded source ID, compact source pin,
    encoded target ID, compact target pin. Adjacent connections therefore
    leave one or two bytes between the ID encodings in this verified fixture.
    This routine deliberately recognizes only the exact semantic run instead
    of treating every similar integer as a graph connection.
    """

    encodings = [encode_varuint32(node_id) for node_id in node_ids]
    matches: List[List[Tuple[int, int]]] = []

    def extend(index: int, offset: int, run: List[Tuple[int, int]]) -> None:
        encoding = encodings[index]
        if not data.startswith(encoding, offset):
            return
        next_run = run + [(node_ids[index], offset)]
        if index + 1 == len(node_ids):
            matches.append(next_run)
            return
        next_encoding = encodings[index + 1]
        end = offset + len(encoding)
        for gap in range(minimum_gap, maximum_gap + 1):
            next_offset = end + gap
            if data.startswith(next_encoding, next_offset):
                extend(index + 1, next_offset, next_run)

    first = encodings[0]
    for offset in _all_offsets(data, first):
        extend(0, offset, [])

    if len(matches) != 1:
        raise TargetTraceError(
            "expected exactly one encoded hit-feedback connection run; "
            f"found {len(matches)}"
        )
    return matches[0]


def trace_hit_feedback(path: Path) -> HitFeedbackTrace:
    path = path.resolve()
    data = path.read_bytes()
    if data[:4] != MAGIC:
        raise BV2FormatError(f"{path}: expected FFBV2 magic")

    sidecar_path = sidecar_for(path)
    if sidecar_path is None:
        raise TargetTraceError(f"{path}: Hunter .decompiled sidecar is required")
    sidecar = probe_sidecar(sidecar_path)
    if len(sidecar.direct_hit_feedback_chains) != 1:
        raise TargetTraceError(
            "expected exactly one semantic hit-feedback chain in the sidecar"
        )
    chain = sidecar.direct_hit_feedback_chains[0]

    expected_values = {
        "reduced_value": "0.75",
        "normal_value": "1",
        "large_value": "1.5",
    }
    for field, expected in expected_values.items():
        if chain[field] != expected:
            raise TargetTraceError(
                f"semantic guard failed: {field} is {chain[field]!r}, expected {expected!r}"
            )

    ids = {
        "user_option": chain["user_option_constant_id"],
        "get_option": chain["get_option_node_id"],
        "multi_select": chain["multi_select_node_id"],
        "reduced": chain["reduced_constant_id"],
        "normal": chain["normal_constant_id"],
        "large": chain["large_constant_id"],
    }
    encoded = {
        name: EncodedIdTrace(
            node_id=node_id,
            encoding_hex=encode_varuint32(node_id).hex(" "),
            offsets=_all_offsets(data, encode_varuint32(node_id)),
        )
        for name, node_id in ids.items()
    }

    # Exact connection order from the verified sidecar:
    # UI option -> option reader -> selector -> output variable, followed by
    # Reduced/Normal/Large constants feeding selector pins 2/3/4.
    connection_ids = (
        ids["user_option"],
        ids["get_option"],
        ids["get_option"],
        ids["multi_select"],
        ids["multi_select"],
        462865,
        ids["reduced"],
        ids["multi_select"],
        ids["normal"],
        ids["multi_select"],
        ids["large"],
        ids["multi_select"],
    )
    run = _find_interleaved_run(data, connection_ids)
    run_start = run[0][1]
    last_id, last_offset = run[-1]
    run_end = last_offset + len(encode_varuint32(last_id))
    reduced_offset = next(
        offset for node_id, offset in run if node_id == ids["reduced"]
    )

    decoded_connections = []
    cursor = run_start
    while cursor < run_end:
        source_id, cursor = decode_varuint32(data, cursor)
        source_pin, cursor = decode_low_tag_varint32(data, cursor)
        target_id, cursor = decode_varuint32(data, cursor)
        target_pin, cursor = decode_low_tag_varint32(data, cursor)
        decoded_connections.append(
            {
                "source_id": source_id,
                "source_pin": source_pin,
                "target_id": target_id,
                "target_pin": target_pin,
            }
        )
    expected_connections = [
        {"source_id": ids["user_option"], "source_pin": 0, "target_id": ids["get_option"], "target_pin": 0},
        {"source_id": ids["get_option"], "source_pin": -1, "target_id": ids["multi_select"], "target_pin": 1},
        {"source_id": ids["multi_select"], "source_pin": 0, "target_id": 462865, "target_pin": 1},
        {"source_id": ids["reduced"], "source_pin": 0, "target_id": ids["multi_select"], "target_pin": 2},
        {"source_id": ids["normal"], "source_pin": 0, "target_id": ids["multi_select"], "target_pin": 3},
        {"source_id": ids["large"], "source_pin": 0, "target_id": ids["multi_select"], "target_pin": 4},
    ]
    if decoded_connections != expected_connections:
        raise TargetTraceError(
            "binary connection tuples disagree with the verified semantic chain"
        )

    digest = hashlib.sha256(data).hexdigest()
    hash_gate = "matched" if digest == SHARED_COMPONENT_SHA256 else "rejected"
    status = (
        "semantic-and-binary-anchor-verified"
        if hash_gate == "matched"
        else "unknown-revision-read-only"
    )
    return HitFeedbackTrace(
        path=str(path),
        sha256=digest,
        hash_gate=hash_gate,
        semantic_chain=chain,
        encoded_node_ids=encoded,
        connection_run_start=run_start,
        connection_run_end=run_end,
        connection_run_hex=data[run_start:run_end].hex(" "),
        decoded_connections=decoded_connections,
        reduced_connection_source_offset=reduced_offset,
        status=status,
        next_requirement=(
            "Resolve the reduced constant record's value-pool reference and "
            "prove an unchanged decode/encode round trip before writing."
        ),
    )
