# Star Wars Outlaws Modding Tools

Source code and field guides for investigating the Windows PC release of Star Wars Outlaws. Use the tools to load a controlled loose-file mod, inspect Snowdrop graph exports, and record what the game does during a repeatable test.

| Tool | What it does |
| --- | --- |
| [Snowdrop BV2](tools/snowdrop-bv2/) | Inspect FFBV2 files, map selected binary pools, and generate narrow, hash-checked weapon UI patches. |
| [Outlaws Event Observer](tools/outlaws-event-observer/) | Build an exact-build Windows plugin and collect combat UI or interaction traces. Includes offline executable and trace-analysis helpers. |
| [Mod Organizer 2 game plugin](tools/mo2-star-wars-outlaws/) | Add Outlaws to MO2 and manage loose `helix` assets and plugin payloads. |
| [Save inspection helpers](tools/outlaws-save-inspection/) | Extract Zstandard frames from local save copies and summarize recognizable metadata. |

## Start here

For asset research, export one relevant file from your game installation with [Hunter](https://dtzxporter.com/tools/hunter). Keep the binary and its `.decompiled` sidecar together, then run the BV2 `inspect` command. Use MO2 and the [Universal Snowdrop Modloader](https://www.nexusmods.com/starwarsoutlaws/mods/24) when you need an isolated in-game A/B test.

For runtime research, check your executable against the [Observer build profile](tools/outlaws-event-observer/README.md#supported-game-build), then follow the capture guide. Python-only tools can be used independently of the observer and MO2.

The [research guides](docs/research/README.md) show how to identify a live weapon graph, trace native interactions, separate item registration from inventory persistence, and map the Imperial wanted system. They include failed experiments and the observations that ruled them out.

## Fork and continue

This is a community handoff. Fork it, adapt individual tools, extend compatibility, and redistribute your changes under the [MIT License](LICENSE). Keep the copyright and license notice with copies or substantial portions of the original material. The license covers the original code and documentation here. MinHook keeps its own terms. Obtain the game, game assets, and external tools separately. [Third-party notices](THIRD_PARTY_NOTICES.md)

When sharing a result, include the tool version, game build or input hash, exact command or observer settings, control run, and the behavior you observed. A small reproduction using your own extracts is enough for someone else to repeat the test.
