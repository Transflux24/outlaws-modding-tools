# Capture and testing guide

Use the supplied INI as a starting point. Change the session label each time you change the test question, keep each capture focused, and preserve the log and TSV files together.

## Capture settings

| Question | Relevant options |
| --- | --- |
| What changes after a shot hits? | Enable `DamageDealt`, `PlayerAimTarget`, `IsHostile`, and `Graphics` under `[Hooks]`. Leave `RawEvaluatorCalls=false` for a changed-state trace. The hit-feedback gates under `[DiagnosticGates]` are off at startup. |
| Did an interaction evaluator change a field during one call? | Enable `InteractionLifecycle` and `InteractionTransitionDetails`. The transition trace compares before and after the original call. |
| Did a state differ between evaluations? | Enable `InteractionLifecycle` and `InteractionObservedStateDetails`. The first sampled value is tagged separately from a change. Use `InteractionCallbacks` to observe listener registration and signals. |
| Does a call belong to the Nix bowl? | Enable `NixBowlGraphDispatch`. It retains the verified open and close callers of the shared helper. `NixBowlInnerEvaluator` reproduces the historical zero-call test described in the interaction case study. |
| What does the contracts terminal activate? | Enable `UIExecutionState`, `MenuGraphStackOnDemand`, `UISetExecutionState`, and `UIExecutionStateManager`. The filtered menu path gates the setter and manager probes. |
| Where does a sequence reference a couch entity? | Enable `SequenceLevelEntity` for the archive trace. `SequenceBlockCollectionLoad` and `TargetTrackFromArchive` add enclosing records. `CouchUidWatch` adds an opt-in F5 hardware watch experiment. |

Unless specified otherwise, these options are in `[Hooks]`. The detail flags, raw-call flags, and `CouchUidWatch` are under `[Capture]`. The `[Display]` options `OverlayNotifications` and `AudioMarker` control capture feedback. Use `InteractionEvaluatorCalls=false` for focused tests that only need lifecycle detail. The optional `ConsumerTrace` experiment looks for typed reads after a damage pulse and produced no downstream matches in the tested graph.

The snapshot coverage field describes known combat UI and interaction nodes. A manual marker records when a tester observed something. For example, a marker when the wanted HUD appears is a time reference rather than a native wanted-level measurement.

## Read the traces

Run these commands from the observer directory with Python 3.8 or later. Substitute your own paths, process ID from the main log, and configured test label.

```powershell
python scripts/extract_observer_session.py "PATH_TO_LOG_FOLDER" "PATH_TO_SESSION_OUTPUT" --pid 1234 --label first_capture
python scripts/analyze_observer_snapshots.py "PATH_TO_SNAPSHOTS_TSV" --expect-smoke
python scripts/analyze_observer_trace.py "PATH_TO_EVENTS_TSV"
python scripts/analyze_observer_states.py "PATH_TO_STATES_TSV"
```

The smoke check expects `observer_ready`, `manual_F10`, `capture_start`, `MANUAL_MARKER_1`, and `capture_stop`. To verify the analyzer without the game, run:

```powershell
python scripts/analyze_observer_snapshots.py tests/fixtures/observer_snapshot_fixture.tsv --expect-smoke
```

## Probe recipes

- **Hit feedback.** Capture a miss, a nonfatal hit, and a fatal hit. Mark each result. In a separate comparison run, turn on the relevant F6 or F7 diagnostic only after recording a baseline. Compare equivalent shots and check the crosshair, reload, and ammo UI. [Hit feedback case study](../../../docs/research/hit-feedback.md)
- **Nix bowl.** Mark the idle prompt, open the menu, mark it, close it, and mark again. Repeat for a second cycle. With `NixBowlGraphDispatch=true`, compare the two open rows and two close rows. [Trailblazer case study](../../../docs/research/trailblazer-interactions.md)
- **Contracts terminal.** Capture idle, menu open, and return to gameplay. Inspect terminal-gated UI state rows and compare their timing with F9 marks. On close, look for the retirement row after state IDs `38` and `13` clear from both observed bitsets. [Trailblazer case study](../../../docs/research/trailblazer-interactions.md)
