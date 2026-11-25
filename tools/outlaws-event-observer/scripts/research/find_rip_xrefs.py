#!/usr/bin/env python3
"""Find simple x64 RIP-relative references to ASCII strings in a PE image."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import struct


@dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int
    characteristics: int

    def contains_file_offset(self, offset: int) -> bool:
        return self.raw_offset <= offset < self.raw_offset + self.raw_size

    def file_offset_to_rva(self, offset: int) -> int:
        return self.virtual_address + offset - self.raw_offset


def read_sections(data: bytes) -> tuple[int, list[Section]]:
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("not a PE image")
    coff = pe_offset + 4
    _, section_count, _, _, _, optional_size, _ = struct.unpack_from(
        "<HHIIIHH", data, coff
    )
    optional = coff + 20
    magic = struct.unpack_from("<H", data, optional)[0]
    if magic != 0x20B:
        raise ValueError("expected a 64-bit PE image")
    image_base = struct.unpack_from("<Q", data, optional + 24)[0]
    cursor = optional + optional_size
    sections: list[Section] = []
    for _ in range(section_count):
        name = data[cursor : cursor + 8].rstrip(b"\0").decode("ascii", "replace")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, cursor + 8
        )
        characteristics = struct.unpack_from("<I", data, cursor + 36)[0]
        sections.append(
            Section(
                name,
                virtual_address,
                virtual_size,
                raw_offset,
                raw_size,
                characteristics,
            )
        )
        cursor += 40
    return image_base, sections


def find_all(data: bytes, needle: bytes) -> list[int]:
    results: list[int] = []
    start = 0
    while True:
        offset = data.find(needle, start)
        if offset < 0:
            return results
        results.append(offset)
        start = offset + 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("strings", nargs="+")
    args = parser.parse_args()

    data = args.image.read_bytes()
    image_base, sections = read_sections(data)
    executable = [s for s in sections if s.characteristics & 0x20000000]

    targets: dict[int, list[tuple[str, int, Section]]] = {}
    missing: dict[str, list[int]] = {}
    for text in args.strings:
        for string_offset in find_all(data, text.encode("ascii") + b"\0"):
            owner = next(
                (section for section in sections if section.contains_file_offset(string_offset)),
                None,
            )
            if owner is None:
                missing.setdefault(text, []).append(string_offset)
                continue
            target_rva = owner.file_offset_to_rva(string_offset)
            targets.setdefault(target_rva, []).append((text, string_offset, owner))

    references: dict[int, list[tuple[Section, int, int, int, int]]] = {
        rva: [] for rva in targets
    }
    rip_relative = re.compile(
        rb"[\x40-\x4f][\x8b\x8d][\x05\x0d\x15\x1d\x25\x2d\x35\x3d].{4}",
        re.DOTALL,
    )
    for section in executable:
        raw = data[section.raw_offset : section.raw_offset + section.raw_size]
        for match in rip_relative.finditer(raw):
            index = match.start()
            rex, opcode, modrm = raw[index : index + 3]
            displacement = struct.unpack_from("<i", raw, index + 3)[0]
            instruction_rva = section.virtual_address + index
            destination = instruction_rva + 7 + displacement
            if destination in references:
                references[destination].append((section, index, rex, opcode, modrm))

    for text in args.strings:
        print(f"[{text}]")
        for string_offset in missing.get(text, []):
            print(f"  string file+0x{string_offset:X} (not in a section)")
        matched = [
            (rva, entry)
            for rva, entries in targets.items()
            for entry in entries
            if entry[0] == text
        ]
        if not matched:
            print("  string not found")
        for target_rva, (_, _, owner) in matched:
            print(
                f"  string {owner.name}+0x{target_rva - owner.virtual_address:X} "
                f"RVA 0x{target_rva:X} VA 0x{image_base + target_rva:X}"
            )
            refs = references[target_rva]
            if not refs:
                print("    no simple RIP-relative LEA/MOV references")
            for section, index, rex, opcode, modrm in refs:
                rva = section.virtual_address + index
                context_start = max(0, index - 16)
                context_end = min(section.raw_size, index + 23)
                context = data[
                    section.raw_offset + context_start : section.raw_offset + context_end
                ]
                print(
                    f"    xref {section.name}+0x{index:X} RVA 0x{rva:X} "
                    f"VA 0x{image_base + rva:X} op={rex:02X}{opcode:02X}{modrm:02X}"
                )
                print(f"      bytes {context.hex(' ')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
