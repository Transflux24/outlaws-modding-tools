from __future__ import annotations

import struct
from typing import Tuple


class ScalarDecodeError(ValueError):
    pass


def _require(data: bytes, offset: int, size: int) -> bytes:
    if offset < 0 or offset + size > len(data):
        raise ScalarDecodeError(
            f"need {size} bytes at offset {offset}, but buffer length is {len(data)}"
        )
    return data[offset : offset + size]


def decode_varuint32(data: bytes, offset: int = 0) -> Tuple[int, int]:
    """Decode Hunter/Snowdrop's low-two-bit tagged unsigned 32-bit value."""
    first = _require(data, offset, 1)[0]
    tag = first & 0x03
    if tag == 0:
        return first >> 2, offset + 1
    if tag == 1:
        packed = int.from_bytes(_require(data, offset, 2), "little")
        return packed >> 2, offset + 2
    if tag == 2:
        packed = int.from_bytes(_require(data, offset, 4), "little")
        return packed >> 2, offset + 4
    return struct.unpack("<I", _require(data, offset + 1, 4))[0], offset + 5


def encode_varuint32(value: int) -> bytes:
    """Encode Hunter/Snowdrop's low-two-bit tagged unsigned 32-bit value."""
    if not 0 <= value <= 0xFFFFFFFF:
        raise ValueError("unsigned value must fit in 32 bits")
    if value <= 0x3F:
        return bytes([(value << 2) | 0])
    if value <= 0x3FFF:
        return ((value << 2) | 1).to_bytes(2, "little")
    if value <= 0x3FFFFFFF:
        return ((value << 2) | 2).to_bytes(4, "little")
    return b"\x03" + struct.pack("<I", value)


def decode_low_tag_varint32(data: bytes, offset: int = 0) -> Tuple[int, int]:
    """Decode the low-two-bit tagged signed form used by graph pins."""
    first = _require(data, offset, 1)[0]
    tag = first & 0x03
    if tag == 0:
        return _sign_extend(first >> 2, 6), offset + 1
    if tag == 1:
        packed = int.from_bytes(_require(data, offset, 2), "little")
        return _sign_extend(packed >> 2, 14), offset + 2
    if tag == 2:
        packed = int.from_bytes(_require(data, offset, 4), "little")
        return _sign_extend(packed >> 2, 30), offset + 4
    return struct.unpack("<i", _require(data, offset + 1, 4))[0], offset + 5


def encode_low_tag_varint32(value: int) -> bytes:
    """Encode the low-two-bit tagged signed form used by graph pins."""
    if not -0x80000000 <= value <= 0x7FFFFFFF:
        raise ValueError("signed value must fit in 32 bits")
    if -0x20 <= value <= 0x1F:
        return bytes([((value & 0x3F) << 2) | 0])
    if -0x2000 <= value <= 0x1FFF:
        payload = value & 0x3FFF
        return ((payload << 2) | 1).to_bytes(2, "little")
    if -0x20000000 <= value <= 0x1FFFFFFF:
        payload = value & 0x3FFFFFFF
        return ((payload << 2) | 2).to_bytes(4, "little")
    return b"\x03" + struct.pack("<i", value)


def _sign_extend(value: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def decode_varint32(data: bytes, offset: int = 0) -> Tuple[int, int]:
    """Decode Hunter/Snowdrop's high-two-bit tagged signed 32-bit value."""
    first = _require(data, offset, 1)[0]
    tag = first >> 6
    if tag == 0:
        return _sign_extend(first & 0x3F, 6), offset + 1
    if tag == 1:
        second = _require(data, offset + 1, 1)[0]
        packed = (first & 0x3F) | (second << 6)
        return _sign_extend(packed, 14), offset + 2
    if tag == 2:
        tail = int.from_bytes(_require(data, offset + 1, 3), "little")
        packed = (first & 0x3F) | (tail << 6)
        return _sign_extend(packed, 30), offset + 4
    return struct.unpack("<i", _require(data, offset + 1, 4))[0], offset + 5


def encode_varint32(value: int) -> bytes:
    """Encode Hunter/Snowdrop's high-two-bit tagged signed 32-bit value."""
    if not -0x80000000 <= value <= 0x7FFFFFFF:
        raise ValueError("signed value must fit in 32 bits")
    if -0x20 <= value <= 0x1F:
        return bytes([value & 0x3F])
    if -0x2000 <= value <= 0x1FFF:
        payload = value & 0x3FFF
        return bytes([(payload & 0x3F) | 0x40, (payload >> 6) & 0xFF])
    if -0x20000000 <= value <= 0x1FFFFFFF:
        payload = value & 0x3FFFFFFF
        return bytes([(payload & 0x3F) | 0x80]) + (payload >> 6).to_bytes(3, "little")
    return b"\xC0" + struct.pack("<i", value)

