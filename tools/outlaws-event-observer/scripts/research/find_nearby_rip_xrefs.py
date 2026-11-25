#!/usr/bin/env python3
"""Find x64 RIP-relative LEA/MOV targets near selected ASCII strings in a PE image."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import struct

from find_rip_xrefs import Section, find_all, read_sections


@dataclass(frozen=True)
class Reference:
    section: Section
    index: int
    length: int
    rex: int | None
    opcode: int
    modrm: int
    destination: int


def iter_rip_references(data: bytes, section: Section):
    raw = data[section.raw_offset : section.raw_offset + section.raw_size]
    pattern = re.compile(
        rb"(?:[\x40-\x4f])?[\x8b\x8d][\x05\x0d\x15\x1d\x25\x2d\x35\x3d].{4}",
        re.DOTALL,
    )
    for match in pattern.finditer(raw):
        index = match.start()
        encoded = match.group()
        rex = encoded[0] if len(encoded) == 7 else None
        opcode_index = 1 if rex is not None else 0
        opcode = encoded[opcode_index]
        modrm = encoded[opcode_index + 1]
        length = len(encoded)
        displacement = struct.unpack_from("<i", encoded, opcode_index + 2)[0]
        instruction_rva = section.virtual_address + index
        yield Reference(
            section=section,
            index=index,
            length=length,
            rex=rex,
            opcode=opcode,
            modrm=modrm,
            destination=instruction_rva + length + displacement,
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("strings", nargs="+")
    parser.add_argument("--radius", type=lambda value: int(value, 0), default=0x80)
    parser.add_argument("--context", type=lambda value: int(value, 0), default=0x10)
    args = parser.parse_args()

    data = args.image.read_bytes()
    image_base, sections = read_sections(data)
    executable = [section for section in sections if section.characteristics & 0x20000000]
    references = [
        reference
        for section in executable
        for reference in iter_rip_references(data, section)
    ]

    for text in args.strings:
        print(f"[{text}]")
        occurrences = find_all(data, text.encode("ascii"))
        if not occurrences:
            print("  string not found")
            continue
        for string_offset in occurrences:
            owner = next(
                (section for section in sections if section.contains_file_offset(string_offset)),
                None,
            )
            if owner is None:
                print(f"  string file+0x{string_offset:X} (not in a section)")
                continue
            target_rva = owner.file_offset_to_rva(string_offset)
            print(
                f"  string {owner.name}+0x{target_rva - owner.virtual_address:X} "
                f"RVA 0x{target_rva:X} VA 0x{image_base + target_rva:X}"
            )
            nearby = [
                reference
                for reference in references
                if target_rva - args.radius
                <= reference.destination
                <= target_rva + len(text) + args.radius
            ]
            if not nearby:
                print(f"    no RIP-relative LEA/MOV targets within 0x{args.radius:X} bytes")
                continue
            for reference in sorted(nearby, key=lambda item: (abs(item.destination - target_rva), item.destination)):
                raw = data[
                    reference.section.raw_offset :
                    reference.section.raw_offset + reference.section.raw_size
                ]
                instruction_rva = reference.section.virtual_address + reference.index
                start = max(0, reference.index - args.context)
                end = min(len(raw), reference.index + reference.length + args.context)
                context = raw[start:end]
                prefix = "" if reference.rex is None else f"{reference.rex:02X}"
                print(
                    f"    xref {reference.section.name}+0x{reference.index:X} "
                    f"RVA 0x{instruction_rva:X} VA 0x{image_base + instruction_rva:X} "
                    f"op={prefix}{reference.opcode:02X}{reference.modrm:02X} "
                    f"target=0x{reference.destination:X} delta={reference.destination - target_rva:+#x}"
                )
                print(f"      bytes {context.hex(' ')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
