from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
import struct
from typing import List

from .binary import BV2FormatError, MAGIC
from .scalars import decode_low_tag_varint32, decode_varuint32


@dataclass(frozen=True)
class ScalarEntry:
    index: int
    value: int
    start: int
    end: int
    encoding_hex: str


@dataclass(frozen=True)
class BV2Layout:
    path: str
    version: int
    string_count: int
    value_count: int
    header_unknown: int
    integer_count: int
    root_value_index: int
    header_end: int
    string_lengths_end: int
    strings_start: int
    strings_end: int
    integers_start: int
    integers_end: int
    descriptors_start: int
    descriptors_end: int
    strings: List[str]
    integers: List[ScalarEntry]
    descriptors: List[ScalarEntry]

    def summary(self) -> dict:
        payload = asdict(self)
        payload.pop("integers")
        payload.pop("descriptors")
        return payload


def decode_inline_float_token(token: int) -> float:
    """Decode the compact immediate-float token used by BV2 value records."""
    if token & 0x03 != 1:
        raise ValueError(f"{token} is not an inline-float token")
    payload = token >> 2
    bits = (
        payload + 0x30000000
        if payload >= 0
        else payload - 0x30000000
    ) & 0xFFFFFFFF
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def encode_inline_float_token(value: float) -> int:
    """Encode a representable f32 as BV2's compact immediate-float token."""
    bits = struct.unpack("<I", struct.pack("<f", value))[0]

    positive_payload = bits - 0x30000000
    if 0 <= positive_payload <= 0x1FFFFFFF:
        return (positive_payload << 2) | 1

    negative_payload = (bits + 0x30000000) & 0xFFFFFFFF
    if negative_payload >= 0x80000000:
        negative_payload -= 0x100000000
    if -0x20000000 <= negative_payload < 0:
        return (negative_payload << 2) | 1

    raise ValueError(f"{value!r} is not representable as an inline BV2 float")


def string_reference_token(index: int) -> int:
    if index < 0:
        raise ValueError("string index cannot be negative")
    return (index << 3) | 2


def _decode_unsigned_entry(data: bytes, index: int, offset: int) -> ScalarEntry:
    value, end = decode_varuint32(data, offset)
    return ScalarEntry(index, value, offset, end, data[offset:end].hex(" "))


def _decode_signed_entry(data: bytes, index: int, offset: int) -> ScalarEntry:
    value, end = decode_low_tag_varint32(data, offset)
    return ScalarEntry(index, value, offset, end, data[offset:end].hex(" "))


def parse_layout_bytes(data: bytes, path_label: str = "<memory>") -> BV2Layout:
    if data[:4] != MAGIC:
        raise BV2FormatError(f"{path_label}: expected FFBV2 magic")

    cursor = 4
    version, cursor = decode_varuint32(data, cursor)
    if version != 4:
        raise BV2FormatError(
            f"{path_label}: unsupported BV2 layout version {version}; expected 4"
        )
    string_count, cursor = decode_varuint32(data, cursor)
    value_count, cursor = decode_varuint32(data, cursor)
    header_unknown, cursor = decode_varuint32(data, cursor)
    integer_count, cursor = decode_varuint32(data, cursor)
    root_value_index, cursor = decode_varuint32(data, cursor)
    header_end = cursor

    length_codes = []
    for _ in range(string_count):
        value, cursor = decode_varuint32(data, cursor)
        length_codes.append(value)
    string_lengths_end = cursor
    strings_start = cursor

    strings = []
    for index, code in enumerate(length_codes):
        byte_length = (code >> 1) + 1
        end = cursor + byte_length
        if end > len(data):
            raise BV2FormatError(
                f"{path_label}: string {index} extends beyond end of file"
            )
        raw = data[cursor:end]
        if not raw or raw[-1] != 0:
            raise BV2FormatError(
                f"{path_label}: string {index} is not NUL terminated"
            )
        strings.append(raw[:-1].decode("utf-8"))
        cursor = end
    strings_end = cursor

    integers_start = cursor
    integers = []
    for index in range(integer_count):
        entry = _decode_signed_entry(data, index, cursor)
        integers.append(entry)
        cursor = entry.end
    integers_end = cursor

    descriptors_start = cursor
    descriptors = []
    for index in range(value_count):
        entry = _decode_unsigned_entry(data, index, cursor)
        descriptors.append(entry)
        cursor = entry.end
    descriptors_end = cursor
    if cursor != len(data):
        raise BV2FormatError(
            f"{path_label}: {len(data) - cursor} unparsed bytes remain at offset {cursor}"
        )

    return BV2Layout(
        path=path_label,
        version=version,
        string_count=string_count,
        value_count=value_count,
        header_unknown=header_unknown,
        integer_count=integer_count,
        root_value_index=root_value_index,
        header_end=header_end,
        string_lengths_end=string_lengths_end,
        strings_start=strings_start,
        strings_end=strings_end,
        integers_start=integers_start,
        integers_end=integers_end,
        descriptors_start=descriptors_start,
        descriptors_end=descriptors_end,
        strings=strings,
        integers=integers,
        descriptors=descriptors,
    )


def parse_layout(path: Path) -> BV2Layout:
    path = path.resolve()
    return parse_layout_bytes(path.read_bytes(), str(path))
