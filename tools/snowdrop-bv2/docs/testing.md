# Testing

Run the portable unit suite from the repository root:

```powershell
python -m unittest discover -s tests -v
```

The scalar and synthetic-binary tests contain no proprietary game data and should run anywhere with a compatible Python interpreter.

`tests/test_outlaws_corpus.py` runs when an optional corpus exists at `./corpus` or at the path set in `OUTLAWS_BV2_CORPUS`. Supply your own unmodified exports for the same build, including the original `helix` tree and Hunter `.decompiled` sidecars. The reference set contains 20 weapon UI graphs and the damage-feedback compound. The expected filenames and build-specific hashes are in the tests and patch code.

Before a release:

1. Run the complete local suite with your own reference corpus available.
2. Produce patches only from hash-matched vanilla inputs.
3. Reparse and verify every output.
4. Review the generated patch report.
5. Confirm the mod package contains only intended relative paths.
6. Record in-game coverage as verified, structurally covered, or untested.

