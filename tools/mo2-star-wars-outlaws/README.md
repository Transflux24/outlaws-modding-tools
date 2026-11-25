# Star Wars Outlaws support for Mod Organizer 2

This is a Basic Games plugin for Mod Organizer 2 (MO2), tested against the
Basic Games interface bundled with MO2 2.5.2 stable. It adds a Star Wars
Outlaws game instance, recognizes both the standard and Plus game executables,
and detects installations from either Steam or Ubisoft Connect.

The plugin has passed local syntax and contract tests and has been tested in
MO2 2.5.2 with the Ubisoft Connect edition. MO2 launches the game correctly
when Ubisoft Connect is already open, and ordinary loose-file mods have been
confirmed through the virtualized filesystem.

## Install

1. Close MO2.
2. Copy `game_starwarsoutlaws.py` into:

   `Mod Organizer 2\plugins\basic_games\games\`

3. Restart MO2 and create a new portable or global instance.
4. Choose **Star Wars Outlaws**. If it is not detected automatically, browse to
   the folder containing `Outlaws.exe`.
5. Start Ubisoft Connect before launching the game through MO2.

Ubisoft Connect installations are detected through the launcher's registry
entry. Manual browsing remains available when automatic detection is not.

## How mods should look in MO2

For ordinary loose-file asset mods, the top of the installed mod should usually
look like this:

```text
helix\
  baked\
  game system data\
  graph objects\
  ...
```

The installer also recognizes archives that begin directly with `baked`,
`game system data`, `graph objects`, or `sdf`, and offers to place those folders
under `helix`.

Native plugin/script mods can use top-level `plugins` or `scripts` folders when
their own instructions call for them.

## Important loader limitation

Install USM's proxy loader (`version.dll` and its accompanying configuration)
directly in the real Star Wars Outlaws game folder. Do not rely on MO2 to
virtualize a replacement DLL beside `Outlaws.exe`: MO2's virtual file system is
not reliable for top-level executable/DLL injection.

Once USM is physically installed, its actual mods and loose `helix` assets can
still be kept as separate MO2 mods so that they can be enabled, disabled, and
ordered cleanly.

The same caution applies to another loader whose instructions require a proxy
DLL beside the game executable. Follow that loader's installation instructions
for the proxy; use MO2 for the mod payloads.

## Usage notes

- Launch Star Wars Outlaws from MO2 whenever you want MO2-managed files active.
- Prefer the normal **Star Wars Outlaws** executable. The plugin also exposes
  **Star Wars Outlaws Plus** in case that is the appropriate entry point for
  your edition.
- This initial plugin intentionally does not redirect saves or configuration
  files into MO2 profiles. Outlaws stores more than simple settings in its
  documents tree, so leaving those files shared avoids accidental save/state
  surprises.
- MO2 shows file conflicts between managed mods. It cannot report conflicts
  against loader files you installed physically unless you also keep a
  reference copy or record of those files.

## Removal

Close MO2 and delete:

`Mod Organizer 2\plugins\basic_games\games\game_starwarsoutlaws.py`

Removing the plugin does not delete the game, saves, or any MO2 mods.

[Testing](docs/testing.md) · [License](../../LICENSE) · [Third-party notices](THIRD_PARTY_NOTICES.md)
