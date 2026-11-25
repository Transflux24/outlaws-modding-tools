# Changelog

## Unreleased

- Relaxed the Nix-bowl inner-evaluator filter to the verified same-thread outer gate, retaining the actual caller and an outer-call inner-count delta after the first strict return-address pass produced no inner evidence.
- Added a same-thread, bowl-gated inner evaluator trace at RVA `0xC1DA30` that records the six actual native arguments, adjusted layout, runtime fields, and within-call changes without retaining unrelated graph evaluations.
- Extended the Nix-bowl graph-dispatch trace with guarded before/after fingerprints of the selected slot and its `+0x18` evaluator subobject, including direct game RVAs and candidate pointee-vtable RVAs.
- Added an exact-build Nix-bowl graph-dispatch diagnostic at RVA `0x5B1AD0`, filtered to the verified native open and close callsites. It records selector, selected graph/runtime identities, execution state, and the original helper result without logging other users of the shared helper.
- Added opt-in observed-state diagnostics for interaction listener/modifier evaluators. Initial post-states and cross-invocation changes now retain the previous/current snapshots, evaluator caller stack, runtime wrapper, and callback owner/node identity.
- Added opt-in pre/post transition capture for interaction listener and modifier evaluators. Changed calls now record the exact caller stack, context values, runtime wrapper, and registered callback owner/node identity without enabling the raw per-frame evaluator trace.
- Corrected the finalized Target Track payload parser to six `0x10` records, each containing an explicit packed index/type word and an object pointer at `+0x08`.
- Extended the couch-filtered Target Track capture with guarded archive-record, shared `+0x1B0/+0x1C0` object, and six-entry `+0x170` payload-vector dumps.
- Added a couch-filtered hook around the complete Target Track archive routine at RVA `0x1B12D60`. It records the finalized owner layout, before/after field changes, third context argument, component vector, return value, and native caller stack without tracing unrelated Target Track blocks.
- Added an exact-build outer sequence-block collection-load hook at RVA `0xC82030`, filtered by a same-thread couch marker from the nested level-entity archive. It records the post-load block layout, component vector, owner link, caller, and return value without tracing unrelated collections. Correlation uses a bounded internal thread-ID registry.
- Added an exact-build, read-only `HC_SequenceBlockComponent_LevelEntity` UID archive trace and a couch-specific F10 match summary for the Trailblazer lounge investigation.
- Added couch-filtered native stack and complete component-byte capture to identify the sequence loader/consumer chain without widening the hook to unrelated entities.
- Added a guarded couch-filtered dump of the owning sequence block's vtable and leading `0x100` bytes when the owner link is populated.
- Added an opt-in, F5-gated hardware data watch for both halves of the two newest verified couch UIDs. It preserves prior thread debug-register state, skips occupied debug slots, acknowledges rotation only after all active thread contexts are updated, adds newly spawned streaming threads, ignores recycled allocations after the UID changes, caps itself at 128 valid hits, and records the UID consumer's instruction boundary, registers, code bytes, and raw stack words.
- Added a nearby RIP-reference research helper for compact type records whose code references a qualified-name prefix or adjacent registration data.
- Added an exact-build, contracts-gated trace of the native UI execution-state manager setter at RVA `0x4A48240`. It samples only state IDs `38` and `13` across the verified current (`+0xA0`) and pending (`+0xB0`) bitsets and emits a read-only retirement boundary when both become absent.
- Added exact-build factory/evaluator coverage for `UI:SetExecutionState`, dormant until the filtered contracts-terminal OnDemand activation arms a capture window. The observer aggregates per-node state IDs and first/last call timing for close-path correlation without changing UI state.
- Added exact-build factory observation for `HC_UI:MenuGraphStack_OnDemand` and an F10 guarded object-layout/string dump.
- Added the verified live-vtable evaluator at RVA `0x47B4060`, filtered to objects whose `+0x38` path is exactly `helix/ui/graphs/menu_contracts_terminal.muigraph`.
- Added a type-specific Boolean callback patch for UI execution-state changes, allowing terminal menu transitions to be observed even when the owning script evaluator does not run during the capture window. The exact 16-byte forwarding thunk is patched directly because the generic hook library cannot decode its AVX prologue.
- Added exact-build factory/evaluator coverage for `HC_ScriptUIExecutionStateListenerNode`.
- Added type-specific interaction-listener callback registration, signal, and destruction tracing.
- Added callback/UI execution-state counts to structured snapshots and a focused bowl-versus-terminal test.
- Added exact-build factory and evaluator hooks for `HC_ScriptInteractionListenerNode` and `HC_ScriptInteractionModifierNode`.
- Added changed-state capture for interaction context identity, runtime pointer, and execution-state fields.
- Expanded structured snapshots and their coverage marker with interaction listener/modifier instance and state counts.
- Added a controlled Trailblazer interaction-lifecycle test procedure.
- Added Windows CI for the Release build and portable snapshot validation.
- Added structured session snapshots with test labels, focus/window metadata, counters, and an explicit coverage marker.
- Added automatic snapshots for observer readiness, capture boundaries, and manual markers.
- Upgraded F10 to write a structured snapshot while retaining the known-instance/vtable dump.
- Added snapshot validation and existing trace-analysis helpers to the project.

## 0.1.0 — 2026-11-14

- Added exact-build validation for the analyzed Outlaws executable.
- Added read-only tracing for the known combat UI evaluators and state outputs.
- Added video-aligned markers, overlay notifications, and optional audio cues.
- Added reversible diagnostic gates used to identify the hitmarker and fatal-marker outputs.
- Added optional short-window consumer tracing.

