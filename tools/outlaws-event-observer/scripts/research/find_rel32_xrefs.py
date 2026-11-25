#!/usr/bin/env python3
"""Find direct x64 rel32 CALL/JMP references to PE RVAs."""

from __future__ import annotations

import argparse
from pathlib import Path
import struct

from find_rip_xrefs import read_sections


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("rvas", nargs="+", type=lambda value: int(value, 0))
    parser.add_argument("--context", type=lambda value: int(value, 0), default=0x10)
    args = parser.parse_args()

    data = args.image.read_bytes()
    image_base, sections = read_sections(data)
    executable = [section for section in sections if section.characteristics & 0x20000000]

    targets = set(args.rvas)
    references: dict[int, list[tuple[object, int, int]]] = {
        target: [] for target in targets
    }
    for section in executable:
        raw = data[section.raw_offset : section.raw_offset + section.raw_size]
        for opcode in (0xE8, 0xE9):
            index = raw.find(bytes((opcode,)))
            while 0 <= index <= len(raw) - 5:
                displacement = struct.unpack_from("<i", raw, index + 1)[0]
                instruction_rva = section.virtual_address + index
                target_rva = instruction_rva + 5 + displacement
                if target_rva in references:
                    references[target_rva].append((section, index, opcode))
                index = raw.find(bytes((opcode,)), index + 1)

    for target_rva in args.rvas:
        print(f"[RVA 0x{target_rva:X} VA 0x{image_base + target_rva:X}]")
        if not references[target_rva]:
            print("  no direct rel32 references")
            continue
        for section, index, opcode in references[target_rva]:
            raw = data[section.raw_offset : section.raw_offset + section.raw_size]
            instruction_rva = section.virtual_address + index
            start = max(0, index - args.context)
            end = min(len(raw), index + 5 + args.context)
            context = raw[start:end]
            mnemonic = "call" if opcode == 0xE8 else "jmp"
            print(
                f"  {mnemonic} {section.name}+0x{index:X} "
                f"RVA 0x{instruction_rva:X} VA 0x{image_base + instruction_rva:X}"
            )
            print(f"    bytes {context.hex(' ')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
