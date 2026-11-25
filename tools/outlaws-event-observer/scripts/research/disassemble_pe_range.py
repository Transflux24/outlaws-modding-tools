#!/usr/bin/env python3
"""Disassemble a small RVA range from a 64-bit PE image with Capstone."""

from __future__ import annotations

import argparse
from pathlib import Path
import struct
import sys


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("rva", type=lambda value: int(value, 0))
    parser.add_argument("--before", type=lambda value: int(value, 0), default=0x80)
    parser.add_argument("--after", type=lambda value: int(value, 0), default=0x180)
    parser.add_argument("--python-path", type=Path)
    args = parser.parse_args()

    if args.python_path:
        sys.path.insert(0, str(args.python_path.resolve()))
    from capstone import Cs, CS_ARCH_X86, CS_MODE_64

    data = args.image.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    coff = pe + 4
    _, count, _, _, _, optional_size, _ = struct.unpack_from("<HHIIIHH", data, coff)
    optional = coff + 20
    image_base = struct.unpack_from("<Q", data, optional + 24)[0]
    cursor = optional + optional_size
    sections = []
    for _ in range(count):
        name = data[cursor : cursor + 8].rstrip(b"\0").decode("ascii", "replace")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, cursor + 8
        )
        sections.append((name, virtual_address, virtual_size, raw_offset, raw_size))
        cursor += 40

    start_rva = max(0, args.rva - args.before)
    end_rva = args.rva + args.after
    owner = next(
        (
            section
            for section in sections
            if section[1] <= start_rva and end_rva <= section[1] + section[4]
        ),
        None,
    )
    if owner is None:
        raise SystemExit("requested range is not contained in one raw PE section")
    name, virtual_address, _, raw_offset, _ = owner
    file_start = raw_offset + start_rva - virtual_address
    code = data[file_start : file_start + end_rva - start_rva]

    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.skipdata = True
    print(
        f"{args.image} {name} RVA 0x{start_rva:X}..0x{end_rva:X} "
        f"VA 0x{image_base + start_rva:X}"
    )
    for insn in md.disasm(code, image_base + start_rva):
        marker = "  <==" if insn.address == image_base + args.rva else ""
        print(
            f"0x{insn.address:016X}  {insn.bytes.hex(' '):<32} "
            f"{insn.mnemonic:<9} {insn.op_str}{marker}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
