# Outlaws Event Observer

A Windows x64 diagnostic plugin for one verified Star Wars Outlaws executable. It records selected combat UI nodes, world-interaction and UI-state events, and session markers. Optional probes reproduce specific investigations in the [research guides](../../docs/research/README.md).

## Supported game build

| Executable fingerprint | Value |
| --- | --- |
| PE timestamp | `0x684DE875` |
| PE image size | `0x2054C000` |
| File size | `528043656` bytes |
| Recorded SHA-256 | `371BE3E5657B6F46F8C9E2D331696BDE754DF16BFC23F7D5DCCACD9FE19B1D2F` |

Run `Get-FileHash .\Outlaws.exe -Algorithm SHA256` from the game folder before installing. With `ExactBuildOnly=true`, the plugin checks the three PE and file-size fields at startup. Use the SHA-256 above as an additional manual check. Keep the guard on. A port to another executable needs verified function addresses, layouts, signatures, and runtime controls.

## Build and install

1. Install Visual Studio 2022 or its Build Tools with Desktop development with C++, the v143 toolset, and a Windows SDK. The required MinHook sources are included.
2. Open a Developer PowerShell for Visual Studio, switch to this directory, and run:

   ```powershell
   msbuild outlaws_event_observer.sln /m /p:Configuration=Release /p:Platform=x64
   ```

3. Install [Universal Snowdrop Modloader](https://www.nexusmods.com/starwarsoutlaws/mods/24) following the author's directions. The tested setup had the loader proxy beside `Outlaws.exe`.
4. Put `x64\Release\outlaws_event_observer.dll` and the supplied `outlaws_event_observer.ini` together in the game folder's `plugins\` directory, or in an MO2 mod with a top-level `plugins\` directory. Launch through MO2 for the latter setup.
5. To remove the observer, exit the game and remove the two observer files from `plugins\`, or disable its MO2 mod.

## First capture

1. Set `[Session] TestLabel=first_capture` in the INI before launch.
2. Load a save. Press F10 for an idle snapshot.
3. Press F8 to begin capture. Press F9 before and after one clearly identifiable action, such as opening the Nix Treats menu. Repeat the action for a comparison.
4. Press F8 again to stop and exit normally. Open `outlaws_event_observer.log` and the `_events.tsv`, `_states.tsv`, and `_snapshots.tsv` files with the same basename. The optional consumer trace uses `_consumers.tsv`. Look beside the executable or in MO2's Overwrite/output mod.
5. Check capture boundaries and dropped-event counts before interpreting a missing row. F9 writes a numbered marker and plays an optional audio cue. The overlay is a separate Windows layer, so game-only recording can omit it.

The included INI enables baseline combat and interaction reporting. F5 requires `CouchUidWatch=true` and arms an experimental data watch. F6 and F7 toggle behavior-changing diagnostics for hit and fatal UI feedback during active capture when the corresponding damage hook is available. Leave those hotkeys alone during ordinary observation.

[Capture settings, log analysis, and test recipes](docs/testing.md) · [Offline executable research helpers](docs/research-tools.md) · [License](../../LICENSE) · [Third-party notices](THIRD_PARTY_NOTICES.md)
