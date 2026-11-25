# Tracing weapon hit feedback

A red hit marker and the aiming reticle can share screen space while coming from different graph paths. Start by separating ordinary hit feedback, fatal feedback, the idle reticle, and incoming damage direction.

## Find the active graph

Export the weapon UI graphs and `helix/nodes/compounds/crosshair_damage_feedback.mcompoundnode` with Hunter. The compound's sidecar associates the `HC_CombatNode_DamageDealt` graph pin `-1` with `Has Dealt Damage` and pin `-3` with `Fatal`. In the matching runtime connection context (`mask=0x3B4D`), the corresponding output slots were `+0x28` and `+0x38`. These offsets describe that context on the recorded build.

Shared source compounds and shipped compiled graphs serve different roles. Editing the source compound alone did not affect the compiled weapon UI. The visibly obvious UI-root diagnostic also showed that the equipped rapid-fire blaster was using the repeater graph. For the E-11, an isolated load test identified `crosshair_assault_rifle_ammunition.muigraph`. Establish file selection before interpreting a subtle patch or deriving a graph name from a loadout label.

## Isolate and verify the input

1. Capture a miss, an ordinary hit, and a kill with the [observer](../../tools/outlaws-event-observer/). The F6 gate suppressed ordinary hit feedback. The surviving horizontal kill tick identified fatal feedback as a separate branch.
2. In the compiled graph, locate both variable setters through their own connections. The [BV2 tool](../../tools/snowdrop-bv2/) redirects their inputs to that graph's existing constant-false slot. This preserves the source damage record and the rest of the graph.
3. Use the patch report and reparse to verify file size, offsets, and edited records. Test the patch in isolation, including ordinary crosshair, reload or heat, and scope behavior where applicable.

The complete guarded batch matched 16 graphs and changed 34 setter records. In-game confirmation covers the tested Kay blaster configurations, E-11, and DLT-A20 sniper scoped and unscoped. The DLT-A20 result is a batch observation. Its active file and the untested configurations require separate checks.

## Follow-up lesson

Changing scale, sector count, or selected alpha values did not remove the target hit feedback in the tested setup. The experimental crosshair batch changed the main reticle color and removed major elements, while a faint ring and the DLT scope reticle remained. Two plausible 48-sector renderers failed the follow-up test. Keep crosshair and hitmarker results separate when choosing an example. [Binary layout and experiment record](../../tools/snowdrop-bv2/RESEARCH.md)
