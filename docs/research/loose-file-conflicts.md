# Comparing loose-file mods with vanilla

Start with the files each mod supplies. An MO2 conflict means packages touch the same relative path. Compare hashes to distinguish identical shared files from different replacements, then compare each replacement against the corresponding export from your installed build.

1. List each package's paths relative to the game root. Normalize installer variants so that a path under `helix` is compared with the same path in every package.
2. Export those paths from the main archive with Hunter. Keep clean game exports and mod files in separate directories.
3. Hash both copies with `Get-FileHash`. For text, use a line diff to identify changed settings. For binary graph data, compare the matching versions with Hunter sidecars or BV2 layout reports.
4. Test one change at a time in MO2. For shared-path changes you want to combine, use a copy of the current vanilla file as the baseline and bring across only the verified changes.

## Camera and movement example

An audit of eight camera and movement packages found 64 distinct asset paths with a vanilla counterpart for each. Some repeated paths had the same content across every package that supplied them. Others, including crouch and scope presets and the default movement configuration, had different replacements. Several camera-oriented packages also replaced the standing lockpicking compound. A package description alone gives limited information about its file footprint.

The depth-of-field example changed seven lines in `helix/game system data/fruit/equipment/aiming.fruit`. Camera packages also touched that same file. A conflict at that path is resolved as a file replacement, so test the intended settings together in the resulting loaded copy.

For a movement test, distinguish the deadzone, analog thresholds, and speed values. Use a slow sweep of the stick, an immediate full deflection, and diagonal changes to demonstrate that walk, jog, and run are separately reachable. Compare crouch and special movement states with vanilla to check the scope of the change.
