# Star Wars Outlaws research guides

These examples document how to go from a hypothesis to a testable finding. Each records what was observed in game, what was established from game data, and what still needs a runtime check. Executable addresses and binary layouts refer to the [recorded Windows build](../../tools/outlaws-event-observer/README.md#supported-game-build).

- [Combat UI and hit feedback](hit-feedback.md) shows how to identify the active graph and verify a two-input patch.
- [Loose-file conflict checks](loose-file-conflicts.md) shows how to compare shared paths and spot unrelated changes in camera and movement packages.
- [Trailblazer interactions](trailblazer-interactions.md) shows how to connect a visible interaction to runtime calls and distinguish adjacent functions.
- [Native items and inventory](native-items.md) describes the evidence for native equipment and a staged test for new item IDs.
- [Imperial wanted system](imperial-wanted.md) maps the static configuration and separates it from the observed first-tier escape.

For a new investigation, begin with an unmodified run. Export only the assets needed to answer the question with [Hunter](https://dtzxporter.com/tools/hunter) and retain their relative paths and SHA-256 hashes locally. Use the sidecar to map names and links, then use the original binary to confirm the file layout. Change one variable per run and record visible effects, negative controls, and which configurations were actually exercised.
