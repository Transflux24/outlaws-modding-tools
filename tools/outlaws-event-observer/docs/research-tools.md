# Offline research helpers

The scripts under `scripts/research/` inspect the game executable without modifying it.

- `find_rip_xrefs.py` locates simple x64 RIP-relative references to selected ASCII strings in a PE image and uses only the Python standard library.
- `find_nearby_rip_xrefs.py` also reports RIP-relative LEA/MOV targets near each string, which is useful when code references a qualified-name prefix, suffix, or adjacent compact type record.
- `find_rel32_xrefs.py` looks for candidate relative calls to specified relative virtual addresses (RVAs).
- `find_hex_patterns.py` locates byte sequences, optionally in executable PE sections only.
- `disassemble_pe_range.py` disassembles a bounded PE RVA range and requires Capstone. Pass a local package directory with `--python-path` when Capstone is not installed globally.

Run from the observer directory and substitute the path to your installed executable:

```powershell
python scripts/research/find_rel32_xrefs.py "PATH_TO_OUTLAWS_EXE" 0xC1DDF0
python scripts/research/disassemble_pe_range.py "PATH_TO_OUTLAWS_EXE" 0xC1DDF0 --before 0x30 --after 0x80
```

Check instruction boundaries in the disassembly and compare with a runtime trace before assigning a caller to a function. [Trailblazer example](../../../docs/research/trailblazer-interactions.md)

