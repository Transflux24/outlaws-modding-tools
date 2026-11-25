# Snowdrop BV2

A Python toolkit for inspecting Snowdrop `FFBV2` binaries, including Outlaws weapon UI graphs and shared compounds. It identifies file hashes and examines string tables, compact scalars, and supported pools. Its targeted writers cover specific verified inputs.

## Inspect an export

Use Python 3.8 or later. Open a terminal in this directory. The commands below run directly from the source tree with the Python standard library.

1. Open the game's main `helix/sdf/pc/data/sdf.sdftoc` in Hunter. Export the relevant asset as a raw file. Keep the original game-relative path and the adjacent `.decompiled` sidecar if Hunter supplies one. [Hunter download and usage](https://dtzxporter.com/tools/hunter)
2. Point `inspect` at the binary file. It reports the recognized format, SHA-256, string-table information, and sidecar metadata.
3. Use `corpus` for a directory inventory or `layout` to examine the supported pools in a single file. `plan-hit-feedback` follows the matching sidecar references.

```powershell
python -m snowdrop_bv2 inspect "PATH_TO_GRAPH"
python -m snowdrop_bv2 layout "PATH_TO_GRAPH" --output layout.json
python -m snowdrop_bv2 corpus "PATH_TO_EXPORT_DIRECTORY" --output corpus.json
python -m snowdrop_bv2 plan-hit-feedback "PATH_TO_EXPORT_DIRECTORY" --output hit-feedback-plan.json
```

## Reproduce the hitmarker case study

For a single matching weapon graph, use:

```powershell
python -m snowdrop_bv2 patch-weapon-hitmarker-gates "PATH_TO_VANILLA_GRAPH" "PATH_TO_NEW_GRAPH" --report patch.json
```

For the verified batch, point the first argument at the directory containing the unmodified weapon graphs, and the second at a fresh directory. Each expected input must have the matching filename and SHA-256.

```powershell
python -m snowdrop_bv2 patch-verified-weapon-batch "PATH_TO_VANILLA_GRAPH_FOLDER" "PATH_TO_NEW_GRAPH_FOLDER" --report patch-batch.json
```

Give the output binary and report separate paths outside your original extract directory. The targeted writers create a new graph, check the matched record and key file invariants, and report the change. Review the result against the original export and deploy it at the same relative `helix` path in a separate loader or MO2 mod.

The ordinary/fatal hitmarker batch has partial runtime coverage. The `patch-verified-weapon-batch-no-crosshair` command also requires the original Hunter sidecars and remains an experiment: major reticle elements disappeared, while a faint ring and the scoped DLT reticle remained. `python -m snowdrop_bv2 --help` lists all commands. Use a command's `--help` for its arguments.

[Hit-feedback case study](../../docs/research/hit-feedback.md) · [FFBV2 format notes](RESEARCH.md) · [Testing](docs/testing.md) · [License](../../LICENSE)
