# FFBV2 research record

This file separates confirmed observations from working hypotheses. A fact is
not promoted to "confirmed" until it has a corpus test or an independently
readable implementation reference.

## Confirmed

### File identity

All 19 current Outlaws UI fixtures begin with:

```text
FF 42 56 32    .BV2
```

The first byte is a binary sentinel, so the complete magic is
`FF 42 56 32` (`FFBV2`).

### Initial compact count

The first value at offset 4 uses the unsigned compact integer encoding below.
It decodes to `4` in all 19 fixtures. Its semantic name remains deliberately
unresolved; the tool calls it `initial_sequence_count` rather than assuming it
is a root-node count.

### Compact unsigned 32-bit integer

The low two bits of the first byte select the storage size:

| Tag | Payload | Total bytes |
| --- | ---: | ---: |
| `00` | 6 bits | 1 |
| `01` | 14 bits | 2 |
| `10` | 30 bits | 4 |
| `11` | following raw `u32` | 5 |

Multi-byte payloads are little-endian. The compact payload is shifted left two
bits and combined with the tag.

### Compact signed 32-bit integer

The high two bits of the first byte select the storage size:

| Tag | Payload | Total bytes |
| --- | ---: | ---: |
| `00` | signed 6 bits | 1 |
| `01` | signed 14 bits | 2 |
| `10` | signed 30 bits | 4 |
| `11` | following raw `i32` | 5 |

The compact forms use two's-complement sign extension. Boundary values for all
four forms have automated encode/decode round-trip coverage.

### Low-tag compact signed 32-bit integer

Graph connection pins use a second signed codec. Its size tags occupy the low
two bits, matching the unsigned codec, while its payload is sign-extended as
6, 14, or 30 bits. The raw fallback is a tag byte followed by an `i32`.

Confirmed examples in the shared component are `FC -> -1`, `08 -> 2`,
`0C -> 3`, and `10 -> 4`.

### String storage

Both graph layouts contain a long NUL-delimited printable string region after
the compact structural prefix. Current known root anchors are:

- `ConstantNodesData` for `.muigraph` files;
- `connectionsById` for the shared `.mcompoundnode` fixture.

The parser records the exact anchored region and falls back to the longest
printable NUL-delimited run only for an unknown root layout.

### Exact shared hit-feedback chain

Hunter's decompilation identifies one direct semantic chain in
`crosshair_damage_feedback.mcompoundnode`:

```text
462866  UI_HitFeedback constant
   -> 462867  HC_UserOptionsNodes_GetOptionInteger
   -> 462868  native:MultiSelect2
      pin 2 <- 462869  float 0.75  (Reduced)
      pin 3 <- 462870  float 1.0   (Normal)
      pin 4 <- 462871  float 1.5   (Large)
```

The source file SHA-256 is:

```text
987bf7e19c3e90a7d21ce3545daac2861debc475c022e359699624f4fbc5eda0
```

The planner asserts the node types, connection directions, target pins, and
three expected values. It finds exactly one direct chain.

### Exact binary connection anchor

The shared component's compact unsigned node IDs are present in one unique,
ordered connection run. For the current verified file:

- the semantic run occupies bytes `11947..12005` (end offset `12006`);
- Reduced node `462869` begins at byte `11977`;
- Normal node `462870` begins at byte `11987`;
- Large node `462871` begins at byte `11997`.

This establishes an exact bridge between Hunter's semantic output and the raw
file. It does **not** yet establish the value-pool record for `0.75`, so these
offsets are diagnostic anchors and are not writable patch offsets.

The connection bytes now decode to the exact six expected tuples, including
the selector inputs `Reduced -> pin 2`, `Normal -> pin 3`, and `Large -> pin 4`.

### Verified shared-component pool layout

Static analysis of Hunter's FFBV2 reader and a complete parse of the verified
shared component establish this layout:

- layout version: `4`;
- interned strings: `219`, stored at bytes `277..5100`;
- compact signed integer records: `12,843`, stored at bytes `5101..36278`;
- value descriptors: `1,833`, stored at bytes `36279..38111`;
- the descriptor stream ends exactly at EOF.

The same pool parser consumes all 19 current Outlaws fixtures exactly to EOF.

The integer records use the low-tag signed codec. Value tokens use their low
two bits as a semantic tag. Tag `01` is an inline float whose payload is
reconstructed around `0x30000000`.

The Reduced value is the unique inline-float token for `0.75`:

- integer record index: `2991`;
- byte range: `12090..12094`;
- encoded bytes: `03 01 00 00 3D`;
- decoded token: `1023410177`;
- decoded float: `0.75`.

It appears in the exact constant-record sequence for the interned keys and
values `value`, `valueType`, `native:Constant`, `0.75`, and `float`. The Large
record has the same sequence with the unique `1.5` token immediately after it.

### Size-preserving Reduced-to-zero patch

Zero is represented as integer token `0`, which is valid for a constant whose
`valueType` remains `float` (the neighboring Normal constant likewise uses an
integer-valued representation). The decoder accepts a non-canonical raw
five-byte integer encoding, so the writer replaces:

```text
03 01 00 00 3D    inline float 0.75
03 00 00 00 00    raw integer token 0
```

Only bytes `12091` and `12094` differ. File size, all pool counts, all later
offsets, and the verified connection run remain unchanged. The writer reparses
the output and requires integer record `2991` to be the only changed record.

### Target population

The current corpus contains:

- 21 valid FFBV2 files after adding the center-reticle source compounds;
- 17 hit-feedback candidates, including the shared compound;
- 4 deliberate exclusions: grenade-launcher crosshair, incoming-damage
  direction UI, and the two center-reticle source compounds;
- 2 weapon graphs that explicitly reference the shared compound: Myrmite rifle
  and rotary cannon.

The reference does not make the loose shared component a runtime replacement
for an already-compiled weapon graph. Hunter shows the hit-feedback constants,
variables, and render node compiled into each weapon `.muigraph`. This explains
why changing only `crosshair_damage_feedback.mcompoundnode` did not alter Kay's
blaster feedback in game.

### Verified compiled Kay-blaster target

`crosshair_blaster.muigraph` contains one compiled `UI_HitFeedback` option,
one `0.75` Reduced token, one `1.5` Large token, and one
`UI:GraphicsMultiSector` node. Its verified source SHA-256 is:

```text
77b4d30f8dbb3e244eb031916b3e7f3809dbfacb54b876b088062e25726afcd6
```

The Reduced token is uniquely located by the compiled constant-record sequence
`Data`, `Type`, `0.75`, `float`:

- integer record index: `1828`;
- byte range: `23166..23170`;
- encoded bytes: `03 01 00 00 3D`.

The graph writer applies the same size-preserving zero representation and
requires this to be the only changed integer record before emitting a file.

In-game testing showed that changing this scale value does not alter the red
circle or tick animation. It is retained as a documented failed diagnostic.

### Verified Kay-blaster renderer target

The compiled graph contains seven `UI:GraphicsMultiSector` instances. Only the
two inherited hit-feedback instances, node IDs `462508` and `461529`, reference
constant-pool entry `299`, the integer value `4` connected as their sector
count. None of the other five sector renderers use that constant.

The binary value record is uniquely anchored by the contiguous constant
sequence `UI_HitFeedback`, `1.5`, `25`, `4`, and `45` with their declared
types. Its writable target is:

- integer record index: `2908`;
- byte range: `25190..25190`;
- encoded byte: `40` (literal integer `4` token);
- replacement byte: `00` (literal integer `0` token).

This disables sector generation for those two renderers without changing the
five other multi-sector graphics or the surrounding crosshair graph.

In-game testing showed that the visible circle and ticks also survive this
change, proving those sector renderers are not the regular projectile feedback.

### Verified damage-feedback alpha target

The regular projectile feedback uses `UI:GraphicsLines` nodes `427834` and
`457953`. Both receive `Hit Feedback Color` at pin `1104`. That color is built
from a unique four-entry alpha pattern that alternates visible `1.0` and the
engine's near-zero representation. The visible entries are integer records
`3248`, `3250`, `3252`, and `3254`, beginning at byte offsets `25806`, `25812`,
`25818`, and `25824`.

The same alpha array is referenced by five damage-feedback color builders,
covering projectile, fatal, headshot, and weak-point variants. Replacing the
four visible values with zero should suppress those feedback colors without
altering the center reticle, heat meter, or unrelated sector graphics.

In-game testing showed that this change restores the ordinary blaster UI but
does not alter the reported red circle or ticks. Those visuals therefore do
not belong to the damage-feedback color branch.

### Verified hostile center-reticle color target

The extracted `crosshair_pistol_blaster_center_element.mcompoundnode` delegates
its color to `crosshair_center_element_color.mcompoundnode`. That component
reads `HC_CombatUINode_PlayerAimTarget`, tests friendly/hostile state, and
selects among preset colors. Its logic is compiled inline into
`crosshair_blaster.muigraph`.

The hostile preset UID
`B581CAC85FB3CDA10000004B5E15171C` is unique in the compiled graph and occupies
integer records `2016..2019`. The default reticle preset UID is
`B581CAC85FB3CD4D00000042D1D07B34` at records `2003..2006`. A guarded
diagnostic replaces only the three differing hostile UID records with the
default values. The resulting file changes six bytes, preserves the file size
and all layout counts, and leaves every renderer and damage-feedback branch
untouched.

This diagnostic is intended to confirm whether the red circle and ticks are
the hostile/on-target reticle state rather than projectile hit feedback.

### Raw float warning

The Blaster graph contains no ordinary little-endian IEEE-754 byte sequence for
either `0.75` or `1.5`, even though Hunter reports both constants. A blind float
search-and-replace is therefore neither sufficient nor safe.

### Confirmed Has Dealt Damage gate and compiled connection

The observer's runtime F6 gate forced only the `Has Dealt Damage` output false.
In a focused in-game check, that suppressed both the red hit circle and its tick
marks. Restoring the output restored the visuals. This identifies the output
itself, rather than a renderer, color, scale, or user-option branch.

Hunter's compiled `DamageDealt` record is unique in `crosshair_blaster.muigraph`.
Its 15 outputs map `Has Dealt Damage` to connection index `551`; every other
damage output has a separate index. BV2 stores Hunter's integer literals as
tokens shifted left by two bits, so connection `551` is integer-pool token
`2204`. The full 20-integer record begins at pool index `16789`, and the target
is pool index `16795`.

The first asset attempt replaced only that output with the graph's standard
disconnected sentinel, `65535` (token `262140`). It loaded without disturbing
the rest of the blaster UI, but the hitmarkers remained. Disconnecting the
producer therefore leaves slot `551` stale or otherwise does not force the
consumer false; it is not equivalent to the successful runtime write.

The graph has one compiled ConstantNodesData bool `false`, connection slot
`200`. The corrected patch leaves the complete DamageDealt record untouched
and changes only the `Has Dealt Damage` set-variable record at integer-pool
index `7721`: its data input at pool index `7727` is redirected from live slot
`551` to constant-false slot `200`. Both tokens use two-byte encodings, making
this a size-preserving edit. The emitted file is fully reparsed and must retain
every descriptor, string, count, DamageDealt output, and other integer value.

### Active upgraded-blaster graph identified

An isolated zero-byte replacement established that Kay's upgraded rapid-fire
configuration instantiates
`crosshair_pistol_blaster_repeater_overheat.muigraph`, not
`crosshair_blaster.muigraph`. The entire blaster interface disappeared only
when that repeater graph was replaced. This proves that the loose override
path is honored for the tested weapon state and explains why otherwise-valid
edits to `crosshair_blaster.muigraph` produced no visible change.

The repeater graph contains the same `DamageDealt` and `Has Dealt Damage`
semantics with different compiled numbering. Its live damage boolean is
connection `578`, its compiled constant-false slot is `210`, and its setter
record begins at integer-pool index `7931`. The targeted edit changes the
setter input at pool index `7937` from `578` to `210`. This is a one-byte
change at file offset `35022`; the 99,082-byte file size, strings,
descriptors, counts, DamageDealt record, and all other integer values remain
unchanged.

The first in-game repeater test suppressed the hit circle and all ordinary,
head, weak-point, and apparent armor-hit ticks, but one horizontal marker
remained on kills. The adjacent `Fatal` set-variable record begins at pool
index `7940` and reads DamageDealt connection `577` at pool index `7946`.
The complete repeater patch redirects both the `Has Dealt Damage` and `Fatal`
setters to constant-false slot `210`. No other DamageDealt outputs or graph
records are changed.

The combined patch was verified in game. Kay's normal repeater-blaster UI
remained functional, while the red hit circle, ordinary hit ticks, head and
weak-point hit feedback, and the horizontal fatal-hit marker were all absent.
This confirms that the two setters are sufficient for the tested rapid-fire
configuration.

### Kay blaster batch

All nine extracted Kay-blaster graphs contain one unique DamageDealt record,
one `Has Dealt Damage` setter, one `Fatal` setter, and one compiled
constant-false slot. A guarded batch writer now derives the two live
connections from each graph's own DamageDealt outputs rather than assuming
the repeater's connection numbers. It requires the exact filename and
SHA-256 of every extracted vanilla graph, then changes only the two setter
inputs. Every emitted graph retains its original size, strings, descriptors,
pool counts, graph layout, and DamageDealt record.

The batch covers the base, normal blaster, heavy, repeater, three charge-up,
and two ion graph variants. Its repeater output is byte-for-byte identical to
the previously verified complete repeater patch. The remaining eight variants
awaited in-game testing. A follow-up pass verified the first two tiers of all
three blaster slots, including the charge configurations: normal weapon UI
remained functional and the hit/fatal markers remained suppressed. The third
tier of each slot was not unlocked in the current save and remains untested.

### E-11 pickup weapon

Isolated zero-file probes ruled out `crosshair_rifle_ammunition.muigraph` and
identified `crosshair_assault_rifle_ammunition.muigraph` as the E-11's active
UI graph: only the assault-rifle probe removed its weapon interface. That
graph maps `Has Dealt Damage` to connection `528`, `Fatal` to connection
`525`, and contains compiled constant-false slot `176`. The targeted E-11
patch changes only those two setter inputs and preserves the complete
DamageDealt record and graph layout.

The targeted E-11 patch was verified in game. Its crosshair and ammunition UI
remained functional, while ordinary and fatal hit markers were suppressed.

### Complete targeted weapon batch

The same guarded analysis was applied to every remaining extracted weapon
graph that contains the DamageDealt ordinary/fatal feedback structure. The
combined batch contains 16 graphs: nine Kay-blaster variants and seven pickup
weapon variants. Fifteen graphs contain one setter pair. The charge-up
marksman-rifle graph contains two independent DamageDealt records, so both
pairs are redirected for a total of four setter edits in that file.

Across the batch, 34 integer-pool setter records are changed. Each graph is
gated by its exact filename and vanilla SHA-256; every emitted file preserves
its size, strings, descriptors, pool counts, graph layout, and complete
DamageDealt output records. The grenade-launcher graph is intentionally
excluded because it has no matching DamageDealt hit/fatal gates. The original
mod's `hud_combat_damage_direction.muigraph` replacement is also excluded
because it controls incoming-damage direction rather than outgoing weapon hit
feedback.

The combined 16-graph build passed a startup/load and regression test with
Kay's current blaster setup and the E-11. A DLT-A20 Sniper also retained its
normal interface and suppressed hit/fatal feedback both scoped in and scoped
out. The DLT-A20's exact graph path was not isolated, so this is recorded as a
functional batch result rather than a graph-to-weapon mapping.

### Targeted crosshair-removal experiments (paused)

A companion build redirected each graph's unique `Main Color` or
`Crosshair Color` setter to an existing transparent `float4`. This removed the
major reticle elements in game while leaving a faint center ring. Two guarded
follow-up targets were tested across Kay's nine graphs:

1. two opaque-white `UI:GraphicsSector` records with 48 sectors;
2. one faint-white (`1 1 1 0.243137`) 48-sector record with inner/outer radii
   `0.925/0.985`.

Neither follow-up removed the surviving ring. The first pair is explicitly
excluded from the later build after being disproven. The second locator remains
structurally verified but is also disproven as the visible ring owner.

A one-file `crosshair_blaster.muigraph` test also showed that a base-looking
equipment selection can instantiate a different `crosshair_pistol_*` graph;
menu labels must not be used as graph identity. The remaining question is
whether the ring belongs to a weapon graph or to a shared aiming HUD layer.
See the [hit-feedback case study](../../docs/research/hit-feedback.md).

## Unresolved

1. The remaining nested container grammar within the integer token stream.
2. The pooled-constant linkage used by the 16 candidate `.muigraph` files.
3. Which additional graph variants are selected by Kay's other blaster fire
   modes and upgrades.

## External implementation leads

- Hunter 2.2.9 contains internal Rust units named `binary_values.rs`,
  `bit_stream.rs`, and `snowdrop.rs`.
- Hunter's `bit_stream.rs` path resolves to DTZxPorter's public `porter-lib`
  implementation, which provides LSB/MSB bit readers and writers:
  <https://github.com/dtzxporter/porter-lib>
- A current Avatar mod author documents decoding, editing, rebuilding, and
  successfully loading `.muigraph` files. That establishes feasibility in a
  related modern Snowdrop game, but not binary compatibility with Outlaws:
  <https://www.nexusmods.com/avatarfrontiersofpandora/mods/2791>

