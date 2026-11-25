# Investigating native item registration and persistence

The extracted schemas and saves support several different item paths. Charms are native inventory and equipment entries with stable UIDs, existing slot groups, and optional talents. Consumables follow count and consumption rules. Special equipment such as binoculars also depends on native input and ability behavior. Starting with an existing category keeps a new definition test focused on registration.

## What the data shows

Shipped loot tables can include item definition files and hand an item to inventory. The examined USM build selects a loose file when Snowdrop requests its path. Together these support a reference-driven discovery hypothesis for new `.mitem` paths. An isolated file with no incoming reference is a useful negative control.

Major-charm UIDs were identifiable in decompressed saves as two byte-reversed 64-bit words. One save pair showed changes in reference counts consistent with an equipment change, though there was no paired observed equip action to assign a meaning to every occurrence. For local metadata inspection, use the [save helpers](../../tools/outlaws-save-inspection/).

## A staged reproduction

Use disposable save copies and a known, repeatable reward interaction. The static investigation identified the general Nix pickpocket table `loot_pickpocket_opportunity` in `loot_living_world.mloottable` as a candidate. Its `myAlwaysRolls` branch gives a way to add one controlled grant while preserving the normal random reward.

1. Record an unchanged reward baseline.
2. Add a duplicate-protected grant for an existing major charm absent from the test save. Confirm the reward path can grant that type before introducing a new UID.
3. Use a fresh UID in an existing loaded item definition path and repeat from the same baseline save. Check acquisition, loadout display, equip behavior, and save/reload independently.
4. Move the fresh UID to a new `.mitem` path and have a known loaded asset reference it. Compare with a run where that reference is absent.
5. Use a separate copy for a removal-and-reload test. Report whether the item persists, disappears, or leaves an unresolved equipped reference.

The fresh-UID and fresh-path steps remain hypotheses. A successful file request, visible reward, native inventory entry, and surviving save are different observations with different failure modes.
