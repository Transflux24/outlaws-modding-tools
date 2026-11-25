# Tracing native Trailblazer interactions

The Trailblazer is useful for controlled comparisons because nearby prompts exercise several patterns. The Nix bowl opens a native loadout category. The wardrobe delegates to the loadout menu. The contracts terminal loads a menu graph on demand and coordinates UI state with a world interaction. The Nix hoard uses separate game values, prop variants, and a loot table for a recurring interaction.

## Mark a repeatable action

Start with a stable prompt and mark it with F9. Open the interaction, mark the visible menu, close it, and mark return to control. Repeat once. For a comparison, perform the same sequence at another prompt. Use F8 to bound the capture and F10 for the final snapshot. [Observer instructions](../../tools/outlaws-event-observer/docs/testing.md)

Separate a state change within one evaluator call from a difference between consecutive calls. Bowl modifier states alternated `8` on open and `7` on close across repeated activations. The observer did not see the change happen within the sampled evaluator call, so a before/after filter on that call alone would miss it.

## Follow the call sites

On the supported build, bowl open and close called the helper at RVA `0x5B1AD0` and returned to `0x42F8709` and `0x42F8AA3`, respectively. The filtered trace associated them with selectors `10` and `15`. These selectors choose entries and are distinct from the observed modifier states.

The helper calls the dispatcher at `0xC1DDF0`. An adjacent wrapper at `0xC1DDD0` calls `0xC1DA30`. A same-thread probe observed two opens and two closes with zero calls at `0xC1DA30`. Removing the original exact-return-address filter made that negative result interpretable. Checking where each function actually began explained the result and left the larger dispatcher as an unsampled entry point for follow-up work.

## Compare with a different interaction

For the contracts terminal, look for `helix/ui/graphs/menu_contracts_terminal.muigraph` on the observed OnDemand node. The terminal-gated state trace samples state IDs `38` and `13` in the manager's current and pending sets. Their retirement provides a distinct close boundary that can be compared with the video or F9 marker.

A separate sequence probe identified the couch entity UID in archived sequence components. That establishes an asset reference. Resolving it to an actionable world interaction would require additional evidence.
