#!/usr/bin/env python3
"""Find byte patterns with optional ?? wildcards in PE sections."""

from __future__ import annotations

import argparse
from pathlib import Path
import re

from find_rip_xrefs import read_sections


def compile_pattern(text: str) -> re.Pattern[bytes]:
    parts = text.split()
    if not parts:
        raise ValueError("empty pattern")
    encoded = bytearray()
    for part in parts:
        if part == "??":
            encoded.extend(b".")
        else:
            encoded.extend(re.escape(bytes((int(part, 16),))))
    return re.compile(bytes(encoded), re.DOTALL)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("patterns", nargs="+")
    parser.add_argument("--executable-only", action="store_true")
    parser.add_argument("--context", type=lambda value: int(value, 0), default=0x10)
    args = parser.parse_args()

    data = args.image.read_bytes()
    image_base, sections = read_sections(data)
    if args.executable_only:
        sections = [section for section in sections if section.characteristics & 0x20000000]

    for text in args.patterns:
        pattern = compile_pattern(text)
        print(f"[{text}]")
        found = 0
        for section in sections:
            raw = data[section.raw_offset : section.raw_offset + section.raw_size]
            for match in pattern.finditer(raw):
                found += 1
                index = match.start()
                rva = section.virtual_address + index
                start = max(0, index - args.context)
                end = min(len(raw), match.end() + args.context)
                print(
                    f"  {section.name}+0x{index:X} RVA 0x{rva:X} "
                    f"VA 0x{image_base + rva:X}"
                )
                print(f"    bytes {raw[start:end].hex(' ')}")
        if not found:
            print("  no matches")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
