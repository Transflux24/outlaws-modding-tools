from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
from pathlib import Path
import re
from typing import List, Optional

from .layout import (
    BV2Layout,
    decode_inline_float_token,
    encode_inline_float_token,
    parse_layout_bytes,
    string_reference_token,
)
from .scalars import encode_low_tag_varint32
from .targeting import SHARED_COMPONENT_SHA256, TargetTraceError, trace_hit_feedback


BLASTER_GRAPH_SHA256 = "77b4d30f8dbb3e244eb031916b3e7f3809dbfacb54b876b088062e25726afcd6"
REPEATER_BLASTER_GRAPH_SHA256 = "5a84a97afb352846cd0f6016ef24b96e0ffc82108a566716f01a509eadc5345d"
KAY_BLASTER_GRAPH_SHA256 = {
    "crosshair_blaster.muigraph": BLASTER_GRAPH_SHA256,
    "crosshair_pistol_blaster_heavy_overheat.muigraph": "2a5ebb347368f0f3a9736f9808b67f73d1699b9f54646dc8b86de63f2c9e498b",
    "crosshair_pistol_blaster_overheat.muigraph": "465fe9523ce0cf487adbcfaabc94ed8d46dbc0292e95d463d977a2499c1b7349",
    "crosshair_pistol_blaster_repeater_overheat.muigraph": REPEATER_BLASTER_GRAPH_SHA256,
    "crosshair_pistol_chargeup_concussion_blast_overheat.muigraph": "8fc50bd0da05fc7373576d9ad421ff0bf261d84612f8c95680007e7916af1f0b",
    "crosshair_pistol_chargeup_overheat.muigraph": "c7ceb81738a67ed5aa151588e9e16a59ddc58d70087349502a990e20df2ed9d7",
    "crosshair_pistol_chargeup_phasepulse_overheat.muigraph": "271d6371921c20860b1834d3bed420e543f8c25f2d8dbe28e59dd4c728d3a023",
    "crosshair_pistol_ion_blast_overheat.muigraph": "f3daa592f771b3e69d63fb9d1246f5e655e657fe53bb4453f1d58f6486935f26",
    "crosshair_pistol_ion_overheat.muigraph": "e46160714e4bc6e1d31dbef5b26d3653addc575eb18900354331dea55ea49cdd",
}
VERIFIED_WEAPON_GRAPH_SHA256 = {
    **KAY_BLASTER_GRAPH_SHA256,
    "crosshair_assault_rifle_ammunition.muigraph": "9746d9979292f1caba7609078ac8f0ed01ea121adf3b468754f9c3fe1bd34be6",
    "crosshair_burst_assault_rifle_ammunition.muigraph": "e3e5f785697a007d6853f73758e4aada45d04e3c3f9190b94764b4336ec3e1c7",
    "crosshair_marksmanrifle_ammunition.muigraph": "00e26e8b02f2c61d1b8d19ceb123ce7e5235b893d8cb8690cac61d56d3cebfe7",
    "crosshair_marksmanrifle_chargeup_ammunition.muigraph": "2eba1ef73d03482f3fbd90ab938d1123b6271c9e7d28a0f18f0a5d039192377a",
    "crosshair_myrmite_rifle_ammunition.muigraph": "bcde83fe1666185cb606773ae5dc9bb980404969ee630149f75f0c3e1e8a4d90",
    "crosshair_rifle_ammunition.muigraph": "d62ef2609c7f04d54f435211c9c7b72fbd591519b1de8bf6389e0f9b3ca045f0",
    "crosshair_rotarycannon.muigraph": "2fa62fd0f5fd7abc8aa0184d3924f38238b64bcb2119c0e856cd9138ff320910",
}


@dataclass(frozen=True)
class HitFeedbackPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    target_integer_index: int
    target_offset: int
    target_end: int
    original_encoding_hex: str
    replacement_encoding_hex: str
    original_value: float
    replacement_value: float
    changed_byte_offsets: List[int]
    connection_run_unchanged: bool
    layout_counts_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphHitFeedbackPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    target_integer_index: int
    target_offset: int
    target_end: int
    original_encoding_hex: str
    replacement_encoding_hex: str
    original_value: float
    replacement_value: float
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphRenderPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    target_integer_index: int
    target_offset: int
    target_end: int
    original_encoding_hex: str
    replacement_encoding_hex: str
    original_sector_count: int
    replacement_sector_count: int
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphAlphaPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    target_integer_indices: List[int]
    target_offsets: List[int]
    original_encoding_hex: str
    replacement_encoding_hex: str
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphPresetColorPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    target_integer_indices: List[int]
    target_offsets: List[int]
    original_uid: str
    replacement_uid: str
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphDamageGatePatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    size_delta: int
    has_dealt_damage_setter_record_start_index: int
    false_constant_index: int
    target_integer_index: int
    target_offset: int
    target_end: int
    replacement_end: int
    original_encoding_hex: str
    replacement_encoding_hex: str
    original_connection_index: int
    replacement_connection_index: int
    layout_counts_unchanged: bool
    descriptors_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphHitmarkerGatePatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    size_delta: int
    false_constant_index: int
    setter_record_start_indices: List[int]
    target_integer_indices: List[int]
    target_offsets: List[int]
    original_connection_indices: List[int]
    replacement_connection_index: int
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    descriptors_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphNoCrosshairPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    hitmarker_target_integer_indices: List[int]
    hitmarker_setter_record_start_indices: List[int]
    false_constant_index: int
    crosshair_color_variable_name: str
    crosshair_color_variable_id: str
    crosshair_color_setter_state_index: int
    crosshair_color_setter_node_id: int
    crosshair_color_setter_record_start_index: int
    crosshair_color_target_integer_index: int
    crosshair_color_original_connection_index: int
    transparent_float4_constant_index: int
    residual_ring_renderer_node_ids: List[int]
    residual_ring_renderer_record_start_indices: List[int]
    residual_ring_target_integer_indices: List[int]
    residual_ring_original_float4_constant_index: Optional[int]
    residual_ring_sector_count_constant_index: Optional[int]
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    descriptors_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphLoadCanaryPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    setter_integer_index: int
    setter_original_connection_index: int
    setter_replacement_connection_index: int
    root_enabled_integer_index: int
    root_enabled_original_token: int
    root_enabled_replacement_token: int
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    descriptors_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass(frozen=True)
class GraphRootCanaryPatchReport:
    input_path: str
    output_path: str
    input_sha256: str
    output_sha256: str
    input_size: int
    output_size: int
    root_enabled_integer_index: int
    root_enabled_original_token: int
    root_enabled_replacement_token: int
    changed_byte_offsets: List[int]
    layout_counts_unchanged: bool
    descriptors_unchanged: bool
    verification: str

    def to_dict(self) -> dict:
        return asdict(self)


def _index_of_string(layout: BV2Layout, value: str) -> int:
    matches = [index for index, item in enumerate(layout.strings) if item == value]
    if len(matches) != 1:
        raise TargetTraceError(
            f"expected one interned string {value!r}; found {len(matches)}"
        )
    return matches[0]


def _find_sequence(values: List[int], pattern: List[int]) -> List[int]:
    return [
        index
        for index in range(len(values) - len(pattern) + 1)
        if values[index : index + len(pattern)] == pattern
    ]


def _integer_tokens(values: List[int]) -> List[int]:
    """Encode Hunter's decompiled integer literals as BV2 integer tokens."""
    return [value << 2 for value in values]


def _split_lua_table_entries(text: str) -> List[str]:
    """Split the immediate table entries in a Hunter Lua sidecar section."""

    entries: List[str] = []
    depth = 0
    entry_start = None
    in_string = False
    escaped = False
    for index, character in enumerate(text):
        if in_string:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                in_string = False
            continue
        if character == '"':
            in_string = True
        elif character == "{":
            depth += 1
            if depth == 1:
                entry_start = index
        elif character == "}":
            if depth == 0:
                # Section slices include the named table's own trailing `},`.
                continue
            if depth == 1 and entry_start is not None:
                entries.append(text[entry_start : index + 1])
                entry_start = None
            depth -= 1
    if depth != 0 or in_string:
        raise TargetTraceError("Hunter sidecar table is incomplete")
    return entries


def _sidecar_section(text: str, marker: str, end_marker: str) -> str:
    try:
        start = text.index(marker) + len(marker)
        end = text.index(end_marker, start)
    except ValueError as error:
        raise TargetTraceError(
            f"Hunter sidecar is missing section boundary {marker!r}"
        ) from error
    return text[start:end]


def _sidecar_integer_record(entry: str) -> List[int]:
    return [
        int(match.group(1))
        for line in entry.splitlines()
        if (match := re.match(r"^\s*(-?\d+),?\s*$", line))
    ]


def locate_weapon_crosshair_color_setter(layout: BV2Layout, input_path: Path):
    """Locate the center-reticle color setter and a transparent float4 constant.

    Hunter's semantic sidecar identifies the exact Main Color/Crosshair Color
    variable and setter state. The optimized integer pool then has to contain
    one exact copy of the corresponding execution record. This deliberately
    avoids matching generic color setters used by reload, ammo, scope, or
    module-feedback elements.
    """

    sidecar_path = Path(str(input_path) + ".decompiled")
    if not sidecar_path.is_file():
        raise TargetTraceError(
            f"Hunter sidecar is required for crosshair targeting: {sidecar_path}"
        )
    text = sidecar_path.read_text(encoding="utf-8", errors="strict")

    variable_entries = _split_lua_table_entries(
        _sidecar_section(text, "\tVariables = {", "\n\t_OptimizedState")
    )
    color_variables = []
    for entry in variable_entries:
        name_match = re.search(r'name = "(Main Color|Crosshair Color)",', entry)
        if not name_match:
            continue
        id_match = re.search(r"varId = #&([0-9A-F]+)", entry)
        if not id_match:
            raise TargetTraceError("center-color variable is missing its varId")
        color_variables.append((name_match.group(1), id_match.group(1)))
    if len(color_variables) != 1:
        raise TargetTraceError(
            "expected exactly one Main Color/Crosshair Color variable; "
            f"found {len(color_variables)}"
        )
    variable_name, variable_id = color_variables[0]

    node_state_entries = _split_lua_table_entries(
        _sidecar_section(text, "\t\tNodeStates = {", "\n\t\tSize =")
    )
    setter_states = [
        index
        for index, entry in enumerate(node_state_entries)
        if variable_id in entry
        and 'accessTypeEnum = "Set"' in entry
        and 'type = "internal:VariableNode"' in entry
    ]
    if len(setter_states) != 1:
        raise TargetTraceError(
            f"expected one center-color setter state; found {len(setter_states)}"
        )
    setter_state_index = setter_states[0]

    execution_entries = _split_lua_table_entries(
        _sidecar_section(text, "\tExecInfo = {", "\n\t\tNodeStates = {")
    )
    setter_records = []
    for entry in execution_entries:
        record = _sidecar_integer_record(entry)
        if record and record[0] == setter_state_index:
            setter_records.append(record)
    if len(setter_records) != 1:
        raise TargetTraceError(
            f"expected one optimized center-color setter record; found {len(setter_records)}"
        )
    setter_record = setter_records[0]
    if len(setter_record) != 7 or setter_record[2] != -1 or setter_record[4] != 2:
        raise TargetTraceError(
            f"unexpected center-color setter record shape: {setter_record}"
        )

    constant_entries = [
        entry
        for entry in _split_lua_table_entries(
            _sidecar_section(
                text,
                "\tConstantNodesData = {",
                "\n\tContainsImpulseCheckpoints",
            )
        )
        if re.search(r"^\s*Data =", entry, re.MULTILINE)
    ]
    transparent_constants = [
        index
        for index, entry in enumerate(constant_entries)
        if re.search(
            r'Data = 0 0 1 0,\s*\n\s*Type = "float4"',
            entry,
        )
    ]
    if len(transparent_constants) != 1:
        raise TargetTraceError(
            "expected one transparent float4 constant (0 0 1 0); "
            f"found {len(transparent_constants)}"
        )
    transparent_constant_index = transparent_constants[0]

    values = [entry.value for entry in layout.integers]
    record_matches = _find_sequence(values, _integer_tokens(setter_record))
    if len(record_matches) != 1:
        raise TargetTraceError(
            "expected one binary center-color setter record; "
            f"found {len(record_matches)}"
        )
    record_start = record_matches[0]
    target = layout.integers[record_start + 6]
    original_connection_index = setter_record[6]
    if target.value != original_connection_index << 2:
        raise TargetTraceError("center-color setter source disagrees with sidecar")

    data_ref = string_reference_token(_index_of_string(layout, "Data"))
    type_ref = string_reference_token(_index_of_string(layout, "Type"))
    float4_ref = string_reference_token(_index_of_string(layout, "float4"))
    constant_starts = [
        index
        for index in range(len(values) - 1)
        if values[index : index + 2] == [data_ref, type_ref]
    ]
    if len(constant_starts) != len(constant_entries):
        raise TargetTraceError(
            "binary constant count disagrees with Hunter sidecar: "
            f"{len(constant_starts)} != {len(constant_entries)}"
        )
    constant_start = constant_starts[transparent_constant_index]
    constant_end = (
        constant_starts[transparent_constant_index + 1]
        if transparent_constant_index + 1 < len(constant_starts)
        else len(values)
    )
    if float4_ref not in values[constant_start:constant_end]:
        raise TargetTraceError("transparent constant is not typed float4 in binary")

    return {
        "variable_name": variable_name,
        "variable_id": variable_id,
        "setter_state_index": setter_state_index,
        "setter_node_id": setter_record[1],
        "setter_record": setter_record,
        "setter_record_start": record_start,
        "original_connection_index": original_connection_index,
        "target": target,
        "transparent_constant_index": transparent_constant_index,
    }


def locate_kay_residual_ring_renderers(layout: BV2Layout, input_path: Path):
    """Locate Kay's paired opaque white 48-sector renderers.

    The regular center-reticle graphics consume Main Color/Crosshair Color, but
    these separate UI:GraphicsSector paths use a literal white float4. They were
    investigated as residual-ring candidates and retained as a semantic locator,
    but in-game testing established that they are not the faint always-on ring.
    """

    if input_path.name not in KAY_BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            f"residual-ring targeting is limited to verified Kay graphs: {input_path.name}"
        )

    sidecar_path = Path(str(input_path) + ".decompiled")
    if not sidecar_path.is_file():
        raise TargetTraceError(
            f"Hunter sidecar is required for residual-ring targeting: {sidecar_path}"
        )
    text = sidecar_path.read_text(encoding="utf-8", errors="strict")

    node_state_entries = _split_lua_table_entries(
        _sidecar_section(text, "\t\tNodeStates = {", "\n\t\tSize =")
    )
    sector_states = [
        index
        for index, entry in enumerate(node_state_entries)
        if 'type = "UI:GraphicsSector"' in entry
    ]
    if len(sector_states) != 1:
        raise TargetTraceError(
            f"expected one UI:GraphicsSector state; found {len(sector_states)}"
        )
    sector_state_index = sector_states[0]

    constant_entries = [
        entry
        for entry in _split_lua_table_entries(
            _sidecar_section(
                text,
                "\tConstantNodesData = {",
                "\n\tContainsImpulseCheckpoints",
            )
        )
        if re.search(r"^\s*Data =", entry, re.MULTILINE)
    ]

    def matching_constants(pattern: str) -> List[int]:
        return [
            index
            for index, entry in enumerate(constant_entries)
            if re.search(pattern, entry)
        ]

    white_constants = matching_constants(
        r'Data = 1 1 1 1,\s*\n\s*Type = "float4"'
    )
    transparent_constants = matching_constants(
        r'Data = 0 0 1 0,\s*\n\s*Type = "float4"'
    )
    sector_count_constants = matching_constants(
        r'Data = 48,\s*\n\s*Type = "integer"'
    )
    if len(white_constants) != 1:
        raise TargetTraceError(
            f"expected one white float4 constant; found {len(white_constants)}"
        )
    if len(transparent_constants) != 1:
        raise TargetTraceError(
            "expected one transparent float4 constant (0 0 1 0); "
            f"found {len(transparent_constants)}"
        )
    if len(sector_count_constants) != 1:
        raise TargetTraceError(
            f"expected one integer 48 constant; found {len(sector_count_constants)}"
        )
    white_constant_index = white_constants[0]
    transparent_constant_index = transparent_constants[0]
    sector_count_constant_index = sector_count_constants[0]

    execution_entries = _split_lua_table_entries(
        _sidecar_section(text, "\tExecInfo = {", "\n\t\tNodeStates = {")
    )
    renderer_records = []
    for entry in execution_entries:
        record = _sidecar_integer_record(entry)
        if (
            len(record) > 31
            and record[0] == sector_state_index
            and record[8] == white_constant_index
            and record[31] == sector_count_constant_index
        ):
            renderer_records.append(record)
    if len(renderer_records) != 2:
        raise TargetTraceError(
            "expected two white 48-sector residual-ring renderer records; "
            f"found {len(renderer_records)}"
        )

    values = [entry.value for entry in layout.integers]
    record_starts = []
    targets = []
    for record in renderer_records:
        matches = _find_sequence(values, _integer_tokens(record))
        if len(matches) != 1:
            raise TargetTraceError(
                "expected one binary residual-ring renderer record; "
                f"found {len(matches)}"
            )
        record_start = matches[0]
        target = layout.integers[record_start + 8]
        if target.value != white_constant_index << 2:
            raise TargetTraceError(
                "residual-ring renderer color disagrees with Hunter sidecar"
            )
        record_starts.append(record_start)
        targets.append(target)

    return {
        "sector_state_index": sector_state_index,
        "renderer_node_ids": [record[1] for record in renderer_records],
        "renderer_records": renderer_records,
        "renderer_record_starts": record_starts,
        "targets": targets,
        "white_constant_index": white_constant_index,
        "transparent_constant_index": transparent_constant_index,
        "sector_count_constant_index": sector_count_constant_index,
    }


def locate_kay_faint_center_ring_renderer(layout: BV2Layout, input_path: Path):
    """Locate Kay's faint, always-on 48-sector center-ring renderer.

    The renderer has a distinctive semantic signature in every Kay graph: one
    UI:GraphicsSector fed the literal color 1 1 1 0.243137, 48 sectors, and
    inner/outer radii 0.925/0.985. All five traits are required before its color
    source is accepted as a binary patch target.
    """

    if input_path.name not in KAY_BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            f"faint-ring targeting is limited to verified Kay graphs: {input_path.name}"
        )

    sidecar_path = Path(str(input_path) + ".decompiled")
    if not sidecar_path.is_file():
        raise TargetTraceError(
            f"Hunter sidecar is required for faint-ring targeting: {sidecar_path}"
        )
    text = sidecar_path.read_text(encoding="utf-8", errors="strict")

    node_state_entries = _split_lua_table_entries(
        _sidecar_section(text, "\t\tNodeStates = {", "\n\t\tSize =")
    )
    sector_states = [
        index
        for index, entry in enumerate(node_state_entries)
        if 'type = "UI:GraphicsSector"' in entry
    ]
    if len(sector_states) != 1:
        raise TargetTraceError(
            f"expected one UI:GraphicsSector state; found {len(sector_states)}"
        )
    sector_state_index = sector_states[0]

    constant_entries = [
        entry
        for entry in _split_lua_table_entries(
            _sidecar_section(
                text,
                "\tConstantNodesData = {",
                "\n\tContainsImpulseCheckpoints",
            )
        )
        if re.search(r"^\s*Data =", entry, re.MULTILINE)
    ]

    def unique_constant(pattern: str, label: str) -> int:
        matches = [
            index
            for index, entry in enumerate(constant_entries)
            if re.search(pattern, entry)
        ]
        if len(matches) != 1:
            raise TargetTraceError(
                f"expected one {label} constant; found {len(matches)}"
            )
        return matches[0]

    faint_color_constant_index = unique_constant(
        r'Data = 1 1 1 0\.243137,\s*\n\s*Type = "float4"',
        "faint-white float4",
    )
    transparent_constant_index = unique_constant(
        r'Data = 0 0 1 0,\s*\n\s*Type = "float4"',
        "transparent float4",
    )
    sector_count_constant_index = unique_constant(
        r'Data = 48,\s*\n\s*Type = "integer"',
        "integer 48",
    )
    inner_radius_constant_index = unique_constant(
        r'Data = 0\.925,\s*\n\s*Type = "float"',
        "inner-radius 0.925",
    )
    outer_radius_constant_index = unique_constant(
        r'Data = 0\.985,\s*\n\s*Type = "float"',
        "outer-radius 0.985",
    )

    execution_entries = _split_lua_table_entries(
        _sidecar_section(text, "\tExecInfo = {", "\n\t\tNodeStates = {")
    )
    renderer_records = []
    for entry in execution_entries:
        record = _sidecar_integer_record(entry)
        if (
            len(record) > 31
            and record[0] == sector_state_index
            and record[8] == faint_color_constant_index
            and record[28] == inner_radius_constant_index
            and record[29] == outer_radius_constant_index
            and record[31] == sector_count_constant_index
        ):
            renderer_records.append(record)
    if len(renderer_records) != 1:
        raise TargetTraceError(
            "expected one faint 48-sector center-ring renderer record; "
            f"found {len(renderer_records)}"
        )
    renderer_record = renderer_records[0]

    values = [entry.value for entry in layout.integers]
    matches = _find_sequence(values, _integer_tokens(renderer_record))
    if len(matches) != 1:
        raise TargetTraceError(
            "expected one binary faint center-ring renderer record; "
            f"found {len(matches)}"
        )
    record_start = matches[0]
    target = layout.integers[record_start + 8]
    if target.value != faint_color_constant_index << 2:
        raise TargetTraceError(
            "faint center-ring renderer color disagrees with Hunter sidecar"
        )

    return {
        "sector_state_index": sector_state_index,
        "renderer_node_ids": [renderer_record[1]],
        "renderer_records": [renderer_record],
        "renderer_record_starts": [record_start],
        "targets": [target],
        "original_color_constant_index": faint_color_constant_index,
        "transparent_constant_index": transparent_constant_index,
        "sector_count_constant_index": sector_count_constant_index,
        "inner_radius_constant_index": inner_radius_constant_index,
        "outer_radius_constant_index": outer_radius_constant_index,
    }


_DAMAGE_DEALT_RECORD = [
    352,
    -1,
    -1,
    281,
    15,
    543,
    551,
    65535,
    549,
    550,
    65535,
    65535,
    552,
    65535,
    548,
    547,
    65535,
    544,
    545,
    546,
]
_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD = [129, 433335, -1, 353, 2, 51, 551]

_REPEATER_DAMAGE_DEALT_RECORD = [
    364,
    -1,
    -1,
    290,
    15,
    569,
    578,
    65535,
    577,
    574,
    65535,
    65535,
    571,
    65535,
    570,
    572,
    65535,
    573,
    575,
    576,
]
_REPEATER_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD = [
    129,
    433335,
    -1,
    364,
    2,
    51,
    578,
]
_REPEATER_FATAL_SET_VARIABLE_RECORD = [130, 433336, -1, 365, 2, 52, 577]


def locate_blaster_has_dealt_damage_connection(layout: BV2Layout):
    """Locate the compiled DamageDealt output feeding Has Dealt Damage."""
    values = [entry.value for entry in layout.integers]
    damage_pattern = _integer_tokens(_DAMAGE_DEALT_RECORD)
    damage_matches = _find_sequence(values, damage_pattern)
    if len(damage_matches) != 1:
        raise TargetTraceError(
            "expected one compiled DamageDealt output record; "
            f"found {len(damage_matches)}"
        )

    variable_pattern = _integer_tokens(_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD)
    variable_matches = _find_sequence(values, variable_pattern)
    if len(variable_matches) != 1:
        raise TargetTraceError(
            "expected one Has Dealt Damage set-variable record; "
            f"found {len(variable_matches)}"
        )

    target_index = damage_matches[0] + 6
    target = layout.integers[target_index]
    expected_token = 551 << 2
    if target.value != expected_token:
        raise TargetTraceError(
            "DamageDealt Has Dealt Damage output is not connection index 551"
        )
    if layout.integers[variable_matches[0] + 6].value != expected_token:
        raise TargetTraceError(
            "Has Dealt Damage set-variable does not consume connection index 551"
        )
    return damage_matches[0], target


def locate_blaster_has_dealt_damage_setter(layout: BV2Layout):
    """Locate the setter input and the graph's compiled constant-false slot."""
    values = [entry.value for entry in layout.integers]
    variable_pattern = _integer_tokens(_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD)
    variable_matches = _find_sequence(values, variable_pattern)
    if len(variable_matches) != 1:
        raise TargetTraceError(
            "expected one Has Dealt Damage set-variable record; "
            f"found {len(variable_matches)}"
        )

    data_ref = string_reference_token(_index_of_string(layout, "Data"))
    type_ref = string_reference_token(_index_of_string(layout, "Type"))
    bool_ref = string_reference_token(_index_of_string(layout, "bool"))
    # Hunter decompiles token 11 as the sole ConstantNodesData bool `false`.
    false_pattern = [data_ref, type_ref, 11, bool_ref]
    false_matches = _find_sequence(values, false_pattern)
    if len(false_matches) != 1:
        raise TargetTraceError(
            f"expected one compiled constant-false record; found {len(false_matches)}"
        )

    constant_starts = [
        index
        for index in range(len(values) - 1)
        if values[index : index + 2] == [data_ref, type_ref]
    ]
    false_constant_index = constant_starts.index(false_matches[0])
    if false_constant_index != 200:
        raise TargetTraceError(
            "compiled constant-false record is not connection slot 200"
        )

    target = layout.integers[variable_matches[0] + 6]
    if target.value != 551 << 2:
        raise TargetTraceError(
            "Has Dealt Damage setter does not currently read connection index 551"
        )
    return variable_matches[0], false_constant_index, target


def locate_repeater_has_dealt_damage_connection(layout: BV2Layout):
    """Locate the repeater DamageDealt output feeding Has Dealt Damage."""
    values = [entry.value for entry in layout.integers]
    damage_pattern = _integer_tokens(_REPEATER_DAMAGE_DEALT_RECORD)
    damage_matches = _find_sequence(values, damage_pattern)
    if len(damage_matches) != 1:
        raise TargetTraceError(
            "expected one compiled repeater DamageDealt output record; "
            f"found {len(damage_matches)}"
        )

    variable_pattern = _integer_tokens(
        _REPEATER_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD
    )
    variable_matches = _find_sequence(values, variable_pattern)
    if len(variable_matches) != 1:
        raise TargetTraceError(
            "expected one repeater Has Dealt Damage set-variable record; "
            f"found {len(variable_matches)}"
        )

    target_index = damage_matches[0] + 6
    target = layout.integers[target_index]
    expected_token = 578 << 2
    if target.value != expected_token:
        raise TargetTraceError(
            "repeater DamageDealt Has Dealt Damage output is not connection index 578"
        )
    if layout.integers[variable_matches[0] + 6].value != expected_token:
        raise TargetTraceError(
            "repeater Has Dealt Damage setter does not consume connection index 578"
        )
    return damage_matches[0], target


def locate_repeater_has_dealt_damage_setter(layout: BV2Layout):
    """Locate the repeater setter input and compiled constant-false slot."""
    values = [entry.value for entry in layout.integers]
    variable_pattern = _integer_tokens(
        _REPEATER_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD
    )
    variable_matches = _find_sequence(values, variable_pattern)
    if len(variable_matches) != 1:
        raise TargetTraceError(
            "expected one repeater Has Dealt Damage set-variable record; "
            f"found {len(variable_matches)}"
        )

    data_ref = string_reference_token(_index_of_string(layout, "Data"))
    type_ref = string_reference_token(_index_of_string(layout, "Type"))
    bool_ref = string_reference_token(_index_of_string(layout, "bool"))
    false_pattern = [data_ref, type_ref, 11, bool_ref]
    false_matches = _find_sequence(values, false_pattern)
    if len(false_matches) != 1:
        raise TargetTraceError(
            f"expected one compiled repeater constant-false record; found {len(false_matches)}"
        )

    constant_starts = [
        index
        for index in range(len(values) - 1)
        if values[index : index + 2] == [data_ref, type_ref]
    ]
    false_constant_index = constant_starts.index(false_matches[0])
    if false_constant_index != 210:
        raise TargetTraceError(
            "compiled repeater constant-false record is not connection slot 210"
        )

    target = layout.integers[variable_matches[0] + 6]
    if target.value != 578 << 2:
        raise TargetTraceError(
            "repeater Has Dealt Damage setter does not currently read connection index 578"
        )
    return variable_matches[0], false_constant_index, target


def locate_repeater_fatal_setter(layout: BV2Layout):
    """Locate the repeater Fatal setter fed by DamageDealt."""
    values = [entry.value for entry in layout.integers]
    fatal_pattern = _integer_tokens(_REPEATER_FATAL_SET_VARIABLE_RECORD)
    fatal_matches = _find_sequence(values, fatal_pattern)
    if len(fatal_matches) != 1:
        raise TargetTraceError(
            "expected one repeater Fatal set-variable record; "
            f"found {len(fatal_matches)}"
        )
    target = layout.integers[fatal_matches[0] + 6]
    if target.value != 577 << 2:
        raise TargetTraceError(
            "repeater Fatal setter does not currently read connection index 577"
        )
    return fatal_matches[0], target


def locate_kay_hitmarker_gates(layout: BV2Layout):
    """Locate Kay graph hit/fatal setters from their DamageDealt outputs."""
    values = [entry.value for entry in layout.integers]
    decoded = [value // 4 for value in values]
    damage_matches = [
        index
        for index in range(len(decoded) - 20)
        if decoded[index + 1 : index + 3] == [-1, -1]
        and decoded[index + 4] == 15
        and sum(
            value == 65535 for value in decoded[index + 5 : index + 20]
        )
        >= 3
    ]
    if len(damage_matches) != 1:
        raise TargetTraceError(
            "expected one Kay DamageDealt record with 15 outputs; "
            f"found {len(damage_matches)}"
        )

    damage_record_start = damage_matches[0]
    damage_record = decoded[damage_record_start : damage_record_start + 20]
    has_dealt_connection = damage_record[6]
    fatal_connection = damage_record[8]

    def find_setter(variable_id: int, connection: int, label: str):
        matches = [
            index
            for index in range(len(decoded) - 8)
            if decoded[index + 1] == variable_id
            and decoded[index + 2] == -1
            and decoded[index + 4] == 2
            and decoded[index + 6] == connection
            and decoded[index + 7 : index + 9] == [1, 0]
        ]
        if len(matches) != 1:
            raise TargetTraceError(
                f"expected one Kay {label} setter consuming connection "
                f"{connection}; found {len(matches)}"
            )
        record_start = matches[0]
        return record_start, layout.integers[record_start + 6]

    has_record_start, has_target = find_setter(
        433335, has_dealt_connection, "Has Dealt Damage"
    )
    fatal_record_start, fatal_target = find_setter(
        433336, fatal_connection, "Fatal"
    )

    data_ref = string_reference_token(_index_of_string(layout, "Data"))
    type_ref = string_reference_token(_index_of_string(layout, "Type"))
    bool_ref = string_reference_token(_index_of_string(layout, "bool"))
    false_matches = _find_sequence(values, [data_ref, type_ref, 11, bool_ref])
    if len(false_matches) != 1:
        raise TargetTraceError(
            f"expected one compiled constant-false record; found {len(false_matches)}"
        )
    constant_starts = [
        index
        for index in range(len(values) - 1)
        if values[index : index + 2] == [data_ref, type_ref]
    ]
    false_constant_index = constant_starts.index(false_matches[0])

    return {
        "damage_record_start": damage_record_start,
        "damage_record": damage_record,
        "has_dealt_connection": has_dealt_connection,
        "fatal_connection": fatal_connection,
        "has_record_start": has_record_start,
        "fatal_record_start": fatal_record_start,
        "has_target": has_target,
        "fatal_target": fatal_target,
        "false_constant_index": false_constant_index,
    }


def locate_weapon_hitmarker_gate_sets(layout: BV2Layout):
    """Locate every DamageDealt hit/fatal setter pair in a weapon graph."""
    values = [entry.value for entry in layout.integers]
    decoded = [value // 4 for value in values]
    damage_matches = [
        index
        for index in range(len(decoded) - 20)
        if decoded[index + 1 : index + 3] == [-1, -1]
        and decoded[index + 4] == 15
        and sum(
            value == 65535 for value in decoded[index + 5 : index + 20]
        )
        >= 3
    ]
    if not damage_matches:
        raise TargetTraceError("expected at least one weapon DamageDealt record")

    gate_sets = []
    for damage_record_start in damage_matches:
        damage_record = decoded[
            damage_record_start : damage_record_start + 20
        ]
        has_dealt_connection = damage_record[6]
        fatal_connection = damage_record[8]

        def find_setter(variable_id: int, connection: int, label: str):
            matches = [
                index
                for index in range(len(decoded) - 8)
                if decoded[index + 1] == variable_id
                and decoded[index + 2] == -1
                and decoded[index + 4] == 2
                and decoded[index + 6] == connection
                and decoded[index + 7 : index + 9] == [1, 0]
            ]
            if len(matches) != 1:
                raise TargetTraceError(
                    f"expected one {label} setter consuming connection "
                    f"{connection}; found {len(matches)}"
                )
            record_start = matches[0]
            return record_start, layout.integers[record_start + 6]

        has_record_start, has_target = find_setter(
            433335, has_dealt_connection, "Has Dealt Damage"
        )
        fatal_record_start, fatal_target = find_setter(
            433336, fatal_connection, "Fatal"
        )
        gate_sets.append(
            {
                "damage_record_start": damage_record_start,
                "damage_record": damage_record,
                "has_dealt_connection": has_dealt_connection,
                "fatal_connection": fatal_connection,
                "has_record_start": has_record_start,
                "fatal_record_start": fatal_record_start,
                "has_target": has_target,
                "fatal_target": fatal_target,
            }
        )

    data_ref = string_reference_token(_index_of_string(layout, "Data"))
    type_ref = string_reference_token(_index_of_string(layout, "Type"))
    bool_ref = string_reference_token(_index_of_string(layout, "bool"))
    false_matches = _find_sequence(values, [data_ref, type_ref, 11, bool_ref])
    if len(false_matches) != 1:
        raise TargetTraceError(
            f"expected one compiled constant-false record; found {len(false_matches)}"
        )
    constant_starts = [
        index
        for index in range(len(values) - 1)
        if values[index : index + 2] == [data_ref, type_ref]
    ]
    false_constant_index = constant_starts.index(false_matches[0])
    return gate_sets, false_constant_index


def locate_blaster_root_enabled(layout: BV2Layout):
    """Locate the optimized UI:Root `myEnabled = true` canary value."""
    values = [entry.value for entry in layout.integers]
    names = [
        "type",
        "_pinTypes",
        "myActiveDistance",
        "myCanBePreloaded",
        "myDefaultUseRealTime",
        "myEditorEnabled",
        "myEnabled",
        "myInstanceCap",
        "myRestrictions",
        "myRestrictionsNegated",
        "myScheduling",
        "myTargetType",
        "myUseConcurrency",
        "myUseExternalParent",
        "UI:Root",
    ]
    pattern = [
        string_reference_token(_index_of_string(layout, name))
        for name in names
    ]
    matches = _find_sequence(values, pattern)
    if len(matches) != 1:
        raise TargetTraceError(
            f"expected one optimized UI:Root state record; found {len(matches)}"
        )

    target = layout.integers[matches[0] + 20]
    if target.value != 7:
        raise TargetTraceError("optimized UI:Root myEnabled token is not true")
    return matches[0], target


_RETICLE_DEFAULT_COLOR_UID = "B581CAC85FB3CD4D00000042D1D07B34"
_RETICLE_HOSTILE_COLOR_UID = "B581CAC85FB3CDA10000004B5E15171C"
_RETICLE_DEFAULT_COLOR_VALUES = [1605619021, -1249785144, -774866124, 66]
_RETICLE_HOSTILE_COLOR_VALUES = [1605619105, -1249785144, 1578440476, 75]


def locate_blaster_hostile_reticle_color(layout: BV2Layout):
    """Locate the compiled hostile-target preset used by the center reticle."""
    values = [entry.value for entry in layout.integers]
    hostile_matches = _find_sequence(values, _RETICLE_HOSTILE_COLOR_VALUES)
    if len(hostile_matches) != 1:
        raise TargetTraceError(
            "expected one hostile center-reticle preset color; "
            f"found {len(hostile_matches)}"
        )
    default_matches = _find_sequence(values, _RETICLE_DEFAULT_COLOR_VALUES)
    if len(default_matches) != 1:
        raise TargetTraceError(
            "expected one default center-reticle preset color; "
            f"found {len(default_matches)}"
        )

    hostile_start = hostile_matches[0]
    default_start = default_matches[0]
    hostile = layout.integers[hostile_start : hostile_start + 4]
    default = layout.integers[default_start : default_start + 4]
    if [entry.value for entry in hostile] != _RETICLE_HOSTILE_COLOR_VALUES:
        raise TargetTraceError("hostile reticle preset UID did not match the verified value")
    if [entry.value for entry in default] != _RETICLE_DEFAULT_COLOR_VALUES:
        raise TargetTraceError("default reticle preset UID did not match the verified value")
    return hostile, default


def locate_reduced_value(layout: BV2Layout):
    values = [entry.value for entry in layout.integers]
    value_ref = string_reference_token(_index_of_string(layout, "value"))
    value_type_ref = string_reference_token(_index_of_string(layout, "valueType"))
    constant_ref = string_reference_token(_index_of_string(layout, "native:Constant"))
    float_ref = string_reference_token(_index_of_string(layout, "float"))
    reduced_token = encode_inline_float_token(0.75)
    large_token = encode_inline_float_token(1.5)

    reduced_pattern = [
        value_ref,
        value_type_ref,
        constant_ref,
        reduced_token,
        float_ref,
    ]
    large_pattern = [
        value_ref,
        value_type_ref,
        constant_ref,
        large_token,
        float_ref,
    ]
    reduced_matches = _find_sequence(values, reduced_pattern)
    large_matches = _find_sequence(values, large_pattern)
    if len(reduced_matches) != 1:
        raise TargetTraceError(
            f"expected one Reduced constant value record; found {len(reduced_matches)}"
        )
    if len(large_matches) != 1:
        raise TargetTraceError(
            f"expected one Large constant value record; found {len(large_matches)}"
        )
    if large_matches[0] <= reduced_matches[0]:
        raise TargetTraceError("Large constant record does not follow Reduced record")

    target_index = reduced_matches[0] + 3
    target = layout.integers[target_index]
    if decode_inline_float_token(target.value) != 0.75:
        raise TargetTraceError("Reduced token does not decode to exactly 0.75")
    return target


def locate_graph_reduced_value(layout: BV2Layout):
    """Locate the compiled-graph form of the Reduced hit-feedback constant."""
    values = [entry.value for entry in layout.integers]
    data_ref = string_reference_token(_index_of_string(layout, "Data"))
    type_ref = string_reference_token(_index_of_string(layout, "Type"))
    float_ref = string_reference_token(_index_of_string(layout, "float"))
    reduced_token = encode_inline_float_token(0.75)

    pattern = [data_ref, type_ref, reduced_token, float_ref]
    matches = _find_sequence(values, pattern)
    if len(matches) != 1:
        raise TargetTraceError(
            f"expected one compiled Reduced constant record; found {len(matches)}"
        )

    target = layout.integers[matches[0] + 2]
    if decode_inline_float_token(target.value) != 0.75:
        raise TargetTraceError("compiled Reduced token does not decode to exactly 0.75")
    return target


def locate_blaster_feedback_sector_count(layout: BV2Layout):
    """Locate the four-sector constant used only by the two hit-feedback renderers."""
    values = [entry.value for entry in layout.integers]
    data_ref = string_reference_token(_index_of_string(layout, "Data"))
    type_ref = string_reference_token(_index_of_string(layout, "Type"))
    ui_ref = string_reference_token(_index_of_string(layout, "UI_HitFeedback"))
    user_option_ref = string_reference_token(_index_of_string(layout, "User Option"))
    float_ref = string_reference_token(_index_of_string(layout, "float"))
    integer_ref = string_reference_token(_index_of_string(layout, "integer"))

    literal_four_token = 4 << 2
    pattern = [
        data_ref,
        type_ref,
        ui_ref,
        user_option_ref,
        0,
        2,
        data_ref,
        type_ref,
        encode_inline_float_token(1.5),
        float_ref,
        0,
        2,
        data_ref,
        type_ref,
        encode_inline_float_token(25.0),
        float_ref,
        0,
        2,
        data_ref,
        type_ref,
        literal_four_token,
        integer_ref,
        0,
        2,
        data_ref,
        type_ref,
        encode_inline_float_token(45.0),
        float_ref,
    ]
    matches = _find_sequence(values, pattern)
    if len(matches) != 1:
        raise TargetTraceError(
            f"expected one compiled hit-feedback renderer constant sequence; found {len(matches)}"
        )

    target = layout.integers[matches[0] + 20]
    if target.value != literal_four_token:
        raise TargetTraceError("hit-feedback sector count is not exactly four")
    return target


def locate_blaster_feedback_alpha_values(layout: BV2Layout):
    """Locate the four visible-alpha values used by damage-feedback colors."""
    values = [entry.value for entry in layout.integers]
    visible = encode_inline_float_token(1.0)
    transparent = encode_inline_float_token(4.656613e-10)
    pattern = [visible, transparent] * 4
    matches = _find_sequence(values, pattern)
    if len(matches) != 1:
        raise TargetTraceError(
            f"expected one compiled damage-feedback alpha pattern; found {len(matches)}"
        )

    targets = [layout.integers[matches[0] + offset] for offset in (0, 2, 4, 6)]
    if any(target.value != visible for target in targets):
        raise TargetTraceError("damage-feedback alpha target is not exactly 1.0")
    return targets


def _encode_replacement(value: float) -> tuple[bytes, int]:
    if value == 0.0:
        # Zero is not representable by the inline-float token scheme. The
        # neighboring Normal value demonstrates that integer values are
        # accepted for a Constant whose valueType remains "float". Keep the
        # raw five-byte form so all subsequent offsets remain unchanged.
        return b"\x03\x00\x00\x00\x00", 0

    try:
        token = encode_inline_float_token(value)
    except ValueError as error:
        raise TargetTraceError(str(error)) from error
    replacement = encode_low_tag_varint32(token)
    if len(replacement) != 5:
        raise TargetTraceError(
            f"replacement {value!r} does not have a five-byte encoding"
        )
    return replacement, token


def patch_hit_feedback(
    input_path: Path,
    output_path: Path,
    replacement_value: float = 0.0,
) -> HitFeedbackPatchReport:
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != SHARED_COMPONENT_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified shared hit-feedback component revision"
        )

    trace = trace_hit_feedback(input_path)
    if trace.hash_gate != "matched":
        raise TargetTraceError("semantic/binary trace did not pass the hash gate")
    layout = parse_layout_bytes(data, str(input_path))
    target = locate_reduced_value(layout)

    replacement, expected_token = _encode_replacement(replacement_value)
    if target.end - target.start != len(replacement):
        raise TargetTraceError(
            "verified Reduced token is not the expected five-byte encoding"
        )
    patched = data[: target.start] + replacement + data[target.end :]

    verified = parse_layout_bytes(patched, str(output_path))
    if verified.integers[target.index].value != expected_token:
        raise TargetTraceError("patched Reduced token did not decode as requested")
    if replacement_value != 0.0:
        decoded_value = decode_inline_float_token(
            verified.integers[target.index].value
        )
        expected_value = decode_inline_float_token(expected_token)
        if decoded_value != expected_value:
            raise TargetTraceError(
                f"patched Reduced token decoded to {decoded_value!r}, not "
                f"{expected_value!r}"
            )
    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != [target.index]:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    connection_before = data[trace.connection_run_start : trace.connection_run_end]
    connection_after = patched[trace.connection_run_start : trace.connection_run_end]
    connection_unchanged = connection_before == connection_after
    if not connection_unchanged:
        raise TargetTraceError("patch unexpectedly changed the connection run")

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged or len(data) != len(patched):
        raise TargetTraceError("patch changed layout counts or file size")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched)) if before != after
    ]
    return HitFeedbackPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        target_integer_index=target.index,
        target_offset=target.start,
        target_end=target.end,
        original_encoding_hex=data[target.start : target.end].hex(" "),
        replacement_encoding_hex=replacement.hex(" "),
        original_value=decode_inline_float_token(target.value),
        replacement_value=replacement_value,
        changed_byte_offsets=changed_offsets,
        connection_run_unchanged=connection_unchanged,
        layout_counts_unchanged=layout_counts_unchanged,
        verification="single-value-size-preserving-patch-verified",
    )


def patch_blaster_hit_feedback(
    input_path: Path,
    output_path: Path,
    replacement_value: float = 0.0,
) -> GraphHitFeedbackPatchReport:
    """Patch the compiled Kay-blaster graph's Reduced feedback scale."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified Kay-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    target = locate_graph_reduced_value(layout)
    replacement, expected_token = _encode_replacement(replacement_value)
    if target.end - target.start != len(replacement):
        raise TargetTraceError(
            "verified compiled Reduced token is not the expected five-byte encoding"
        )

    patched = data[: target.start] + replacement + data[target.end :]
    verified = parse_layout_bytes(patched, str(output_path))
    if verified.integers[target.index].value != expected_token:
        raise TargetTraceError("patched compiled Reduced token did not decode as requested")
    if replacement_value != 0.0:
        decoded_value = decode_inline_float_token(verified.integers[target.index].value)
        expected_value = decode_inline_float_token(expected_token)
        if decoded_value != expected_value:
            raise TargetTraceError(
                f"patched compiled Reduced token decoded to {decoded_value!r}, not "
                f"{expected_value!r}"
            )

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != [target.index]:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged or len(data) != len(patched):
        raise TargetTraceError("patch changed layout counts or file size")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched)) if before != after
    ]
    return GraphHitFeedbackPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        target_integer_index=target.index,
        target_offset=target.start,
        target_end=target.end,
        original_encoding_hex=data[target.start : target.end].hex(" "),
        replacement_encoding_hex=replacement.hex(" "),
        original_value=decode_inline_float_token(target.value),
        replacement_value=replacement_value,
        changed_byte_offsets=changed_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        verification="compiled-graph-single-value-size-preserving-patch-verified",
    )


def patch_blaster_feedback_renderer(
    input_path: Path,
    output_path: Path,
) -> GraphRenderPatchReport:
    """Set the two compiled hit-feedback GraphicsMultiSector renderers to zero sectors."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified Kay-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    target = locate_blaster_feedback_sector_count(layout)
    replacement = encode_low_tag_varint32(0)
    if target.end - target.start != len(replacement):
        raise TargetTraceError(
            "verified sector-count token is not the expected one-byte encoding"
        )

    patched = data[: target.start] + replacement + data[target.end :]
    verified = parse_layout_bytes(patched, str(output_path))
    if verified.integers[target.index].value != 0:
        raise TargetTraceError("patched sector-count token did not decode to zero")

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != [target.index]:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged or len(data) != len(patched):
        raise TargetTraceError("patch changed layout counts or file size")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched)) if before != after
    ]
    return GraphRenderPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        target_integer_index=target.index,
        target_offset=target.start,
        target_end=target.end,
        original_encoding_hex=data[target.start : target.end].hex(" "),
        replacement_encoding_hex=replacement.hex(" "),
        original_sector_count=4,
        replacement_sector_count=0,
        changed_byte_offsets=changed_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        verification="compiled-hit-feedback-renderer-single-byte-patch-verified",
    )


def patch_blaster_feedback_alpha(
    input_path: Path,
    output_path: Path,
) -> GraphAlphaPatchReport:
    """Make compiled damage-feedback color arrays fully transparent."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified Kay-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    targets = locate_blaster_feedback_alpha_values(layout)
    replacement = b"\x03\x00\x00\x00\x00"
    patched = bytearray(data)
    for target in targets:
        if target.end - target.start != len(replacement):
            raise TargetTraceError(
                "verified feedback-alpha token is not the expected five-byte encoding"
            )
        patched[target.start : target.end] = replacement
    patched = bytes(patched)

    verified = parse_layout_bytes(patched, str(output_path))
    if any(verified.integers[target.index].value != 0 for target in targets):
        raise TargetTraceError("patched feedback-alpha token did not decode to zero")

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    expected_indices = [target.index for target in targets]
    if differing_values != expected_indices:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged or len(data) != len(patched):
        raise TargetTraceError("patch changed layout counts or file size")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched)) if before != after
    ]
    return GraphAlphaPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        target_integer_indices=expected_indices,
        target_offsets=[target.start for target in targets],
        original_encoding_hex=targets[0].encoding_hex,
        replacement_encoding_hex=replacement.hex(" "),
        changed_byte_offsets=changed_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        verification="compiled-damage-feedback-alpha-four-value-patch-verified",
    )


def patch_blaster_hostile_reticle_color(
    input_path: Path,
    output_path: Path,
) -> GraphPresetColorPatchReport:
    """Replace only the hostile center-reticle preset with the default preset."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified Kay-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    hostile, default = locate_blaster_hostile_reticle_color(layout)
    patched = bytearray(data)
    changed_indices = []
    for hostile_entry, default_entry in zip(hostile, default):
        if hostile_entry.value == default_entry.value:
            continue
        replacement = encode_low_tag_varint32(default_entry.value)
        if len(replacement) != hostile_entry.end - hostile_entry.start:
            raise TargetTraceError(
                "reticle preset replacement would change the BV2 file size"
            )
        patched[hostile_entry.start : hostile_entry.end] = replacement
        changed_indices.append(hostile_entry.index)
    patched = bytes(patched)

    verified = parse_layout_bytes(patched, str(output_path))
    verified_values = [entry.value for entry in verified.integers]
    expected_values = [entry.value for entry in default]
    start = hostile[0].index
    if verified_values[start : start + 4] != expected_values:
        raise TargetTraceError("patched hostile reticle UID did not match the default UID")

    original_values = [entry.value for entry in layout.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, verified_values))
        if before != after
    ]
    if differing_values != changed_indices:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged or len(data) != len(patched):
        raise TargetTraceError("patch changed layout counts or file size")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched)) if before != after
    ]
    return GraphPresetColorPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        target_integer_indices=changed_indices,
        target_offsets=[layout.integers[index].start for index in changed_indices],
        original_uid=_RETICLE_HOSTILE_COLOR_UID,
        replacement_uid=_RETICLE_DEFAULT_COLOR_UID,
        changed_byte_offsets=changed_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        verification="compiled-hostile-center-reticle-preset-neutralized",
    )


def patch_blaster_has_dealt_damage_gate(
    input_path: Path,
    output_path: Path,
) -> GraphDamageGatePatchReport:
    """Feed Kay's Has Dealt Damage setter the compiled constant-false slot."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified Kay-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    record_start, false_constant_index, target = (
        locate_blaster_has_dealt_damage_setter(layout)
    )
    replacement = encode_low_tag_varint32(false_constant_index << 2)
    patched = data[: target.start] + replacement + data[target.end :]

    verified = parse_layout_bytes(patched, str(output_path))
    expected_token = false_constant_index << 2
    if verified.integers[target.index].value != expected_token:
        raise TargetTraceError(
            "patched Has Dealt Damage setter did not decode as constant false"
        )

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != [target.index]:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged:
        raise TargetTraceError("patch changed BV2 layout counts")
    if layout.strings != verified.strings:
        raise TargetTraceError("patch unexpectedly changed the string pool")
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not descriptors_unchanged:
        raise TargetTraceError("patch unexpectedly changed value descriptors")

    patched_setter_record = list(_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD)
    patched_setter_record[6] = false_constant_index
    patched_matches = _find_sequence(
        patched_values,
        _integer_tokens(patched_setter_record),
    )
    if patched_matches != [record_start]:
        raise TargetTraceError(
            "patched Has Dealt Damage setter was not uniquely preserved"
        )
    damage_matches = _find_sequence(
        patched_values,
        _integer_tokens(_DAMAGE_DEALT_RECORD),
    )
    if len(damage_matches) != 1:
        raise TargetTraceError(
            "patch unexpectedly changed the DamageDealt output record"
        )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    return GraphDamageGatePatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        size_delta=len(patched) - len(data),
        has_dealt_damage_setter_record_start_index=record_start,
        false_constant_index=false_constant_index,
        target_integer_index=target.index,
        target_offset=target.start,
        target_end=target.end,
        replacement_end=target.start + len(replacement),
        original_encoding_hex=data[target.start : target.end].hex(" "),
        replacement_encoding_hex=replacement.hex(" "),
        original_connection_index=551,
        replacement_connection_index=false_constant_index,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification="compiled-has-dealt-damage-setter-fed-constant-false",
    )


def patch_kay_hitmarker_gates(
    input_path: Path,
    output_path: Path,
) -> GraphHitmarkerGatePatchReport:
    """Force one verified Kay graph's hit and fatal feedback gates false."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    expected_hash = VERIFIED_WEAPON_GRAPH_SHA256.get(input_path.name)
    if expected_hash is None:
        raise TargetTraceError(
            f"unsupported verified weapon graph filename: {input_path.name}"
        )
    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected_hash:
        raise TargetTraceError(
            f"input SHA-256 is not the verified revision for {input_path.name}"
        )

    layout = parse_layout_bytes(data, str(input_path))
    gates = locate_kay_hitmarker_gates(layout)
    targets = [gates["has_target"], gates["fatal_target"]]
    replacement_index = gates["false_constant_index"]
    replacement = encode_low_tag_varint32(replacement_index << 2)
    for target in targets:
        if len(replacement) != target.end - target.start:
            raise TargetTraceError(
                "feedback-gate replacement changes encoded width"
            )

    patched_buffer = bytearray(data)
    for target in targets:
        patched_buffer[target.start : target.end] = replacement
    patched = bytes(patched_buffer)

    verified = parse_layout_bytes(patched, str(output_path))
    expected_token = replacement_index << 2
    for target in targets:
        if verified.integers[target.index].value != expected_token:
            raise TargetTraceError(
                "patched Kay feedback setter did not decode as constant false"
            )

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    target_indices = [target.index for target in targets]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != target_indices:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged:
        raise TargetTraceError("patch changed BV2 layout counts")
    if layout.strings != verified.strings:
        raise TargetTraceError("patch unexpectedly changed the string pool")
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not descriptors_unchanged:
        raise TargetTraceError("patch unexpectedly changed value descriptors")

    damage_record = gates["damage_record"]
    if [value // 4 for value in patched_values][
        gates["damage_record_start"] : gates["damage_record_start"] + 20
    ] != damage_record:
        raise TargetTraceError("patch unexpectedly changed DamageDealt outputs")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_byte_offsets = [
        index
        for index, (before, after) in enumerate(zip(data, patched))
        if before != after
    ]
    return GraphHitmarkerGatePatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        size_delta=len(patched) - len(data),
        false_constant_index=replacement_index,
        setter_record_start_indices=[
            gates["has_record_start"],
            gates["fatal_record_start"],
        ],
        target_integer_indices=target_indices,
        target_offsets=[target.start for target in targets],
        original_connection_indices=[
            gates["has_dealt_connection"],
            gates["fatal_connection"],
        ],
        replacement_connection_index=replacement_index,
        changed_byte_offsets=changed_byte_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification="compiled-kay-hit-and-fatal-setters-fed-constant-false",
    )


def patch_kay_blaster_batch(
    input_graphs_directory: Path,
    output_graphs_directory: Path,
) -> List[GraphHitmarkerGatePatchReport]:
    """Patch every verified Kay blaster graph into a new directory."""
    input_graphs_directory = input_graphs_directory.resolve()
    output_graphs_directory = output_graphs_directory.resolve()
    reports = []
    for filename in KAY_BLASTER_GRAPH_SHA256:
        reports.append(
            patch_kay_hitmarker_gates(
                input_graphs_directory / filename,
                output_graphs_directory / filename,
            )
        )
    return reports


def patch_weapon_hitmarker_gates(
    input_path: Path,
    output_path: Path,
) -> GraphHitmarkerGatePatchReport:
    """Patch every hit/fatal feedback pair in one verified weapon graph."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    expected_hash = VERIFIED_WEAPON_GRAPH_SHA256.get(input_path.name)
    if expected_hash is None:
        raise TargetTraceError(
            f"unsupported verified weapon graph filename: {input_path.name}"
        )
    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected_hash:
        raise TargetTraceError(
            f"input SHA-256 is not the verified revision for {input_path.name}"
        )

    layout = parse_layout_bytes(data, str(input_path))
    gate_sets, replacement_index = locate_weapon_hitmarker_gate_sets(layout)
    targets = []
    setter_record_starts = []
    original_connections = []
    for gates in gate_sets:
        targets.extend([gates["has_target"], gates["fatal_target"]])
        setter_record_starts.extend(
            [gates["has_record_start"], gates["fatal_record_start"]]
        )
        original_connections.extend(
            [gates["has_dealt_connection"], gates["fatal_connection"]]
        )

    replacement = encode_low_tag_varint32(replacement_index << 2)
    for target in targets:
        if len(replacement) != target.end - target.start:
            raise TargetTraceError(
                "feedback-gate replacement changes encoded width"
            )

    patched_buffer = bytearray(data)
    for target in targets:
        patched_buffer[target.start : target.end] = replacement
    patched = bytes(patched_buffer)
    verified = parse_layout_bytes(patched, str(output_path))

    expected_token = replacement_index << 2
    for target in targets:
        if verified.integers[target.index].value != expected_token:
            raise TargetTraceError(
                "patched weapon feedback setter did not decode as constant false"
            )

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    target_indices = [target.index for target in targets]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != target_indices:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged:
        raise TargetTraceError("patch changed BV2 layout counts")
    if layout.strings != verified.strings:
        raise TargetTraceError("patch unexpectedly changed the string pool")
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not descriptors_unchanged:
        raise TargetTraceError("patch unexpectedly changed value descriptors")

    patched_decoded = [value // 4 for value in patched_values]
    for gates in gate_sets:
        start = gates["damage_record_start"]
        if patched_decoded[start : start + 20] != gates["damage_record"]:
            raise TargetTraceError(
                "patch unexpectedly changed a DamageDealt output record"
            )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_byte_offsets = [
        index
        for index, (before, after) in enumerate(zip(data, patched))
        if before != after
    ]
    return GraphHitmarkerGatePatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        size_delta=len(patched) - len(data),
        false_constant_index=replacement_index,
        setter_record_start_indices=setter_record_starts,
        target_integer_indices=target_indices,
        target_offsets=[target.start for target in targets],
        original_connection_indices=original_connections,
        replacement_connection_index=replacement_index,
        changed_byte_offsets=changed_byte_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification="compiled-weapon-hit-and-fatal-setters-fed-constant-false",
    )


def patch_verified_weapon_batch(
    input_graphs_directory: Path,
    output_graphs_directory: Path,
) -> List[GraphHitmarkerGatePatchReport]:
    """Patch all verified Kay and pickup-weapon graphs."""
    input_graphs_directory = input_graphs_directory.resolve()
    output_graphs_directory = output_graphs_directory.resolve()
    reports = []
    for filename in VERIFIED_WEAPON_GRAPH_SHA256:
        reports.append(
            patch_weapon_hitmarker_gates(
                input_graphs_directory / filename,
                output_graphs_directory / filename,
            )
        )
    return reports


def patch_weapon_hitmarkers_and_crosshair(
    input_path: Path,
    output_path: Path,
) -> GraphNoCrosshairPatchReport:
    """Disable hit/fatal markers and make only the center reticle transparent."""

    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    expected_hash = VERIFIED_WEAPON_GRAPH_SHA256.get(input_path.name)
    if expected_hash is None:
        raise TargetTraceError(
            f"unsupported verified weapon graph filename: {input_path.name}"
        )
    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected_hash:
        raise TargetTraceError(
            f"input SHA-256 is not the verified revision for {input_path.name}"
        )

    layout = parse_layout_bytes(data, str(input_path))
    gate_sets, false_constant_index = locate_weapon_hitmarker_gate_sets(layout)
    color = locate_weapon_crosshair_color_setter(layout, input_path)
    residual_ring = (
        locate_kay_faint_center_ring_renderer(layout, input_path)
        if input_path.name in KAY_BLASTER_GRAPH_SHA256
        else None
    )
    if (
        residual_ring is not None
        and residual_ring["transparent_constant_index"]
        != color["transparent_constant_index"]
    ):
        raise TargetTraceError(
            "center-color and residual-ring locators disagree on transparent constant"
        )

    hitmarker_targets = []
    hitmarker_record_starts = []
    for gates in gate_sets:
        hitmarker_targets.extend([gates["has_target"], gates["fatal_target"]])
        hitmarker_record_starts.extend(
            [gates["has_record_start"], gates["fatal_record_start"]]
        )
    color_target = color["target"]
    residual_ring_targets = (
        residual_ring["targets"] if residual_ring is not None else []
    )
    false_replacement = encode_low_tag_varint32(false_constant_index << 2)
    transparent_replacement = encode_low_tag_varint32(
        color["transparent_constant_index"] << 2
    )
    for target in hitmarker_targets:
        if len(false_replacement) != target.end - target.start:
            raise TargetTraceError("hitmarker replacement changes encoded width")
    if len(transparent_replacement) != color_target.end - color_target.start:
        raise TargetTraceError("crosshair-color replacement changes encoded width")
    for target in residual_ring_targets:
        if len(transparent_replacement) != target.end - target.start:
            raise TargetTraceError(
                "residual-ring color replacement changes encoded width"
            )

    patched_buffer = bytearray(data)
    for target in hitmarker_targets:
        patched_buffer[target.start : target.end] = false_replacement
    patched_buffer[color_target.start : color_target.end] = transparent_replacement
    for target in residual_ring_targets:
        patched_buffer[target.start : target.end] = transparent_replacement
    patched = bytes(patched_buffer)
    verified = parse_layout_bytes(patched, str(output_path))

    for target in hitmarker_targets:
        if verified.integers[target.index].value != false_constant_index << 2:
            raise TargetTraceError("patched hitmarker setter is not constant false")
    if (
        verified.integers[color_target.index].value
        != color["transparent_constant_index"] << 2
    ):
        raise TargetTraceError(
            "patched center-color setter is not the transparent float4 constant"
        )
    for target in residual_ring_targets:
        if (
            verified.integers[target.index].value
            != color["transparent_constant_index"] << 2
        ):
            raise TargetTraceError(
                "patched residual-ring renderer is not the transparent float4 constant"
            )

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    expected_indices = sorted(
        [target.index for target in hitmarker_targets]
        + [color_target.index]
        + [target.index for target in residual_ring_targets]
    )
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != expected_indices:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged:
        raise TargetTraceError("patch changed BV2 layout counts")
    if layout.strings != verified.strings:
        raise TargetTraceError("patch unexpectedly changed the string pool")
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not descriptors_unchanged:
        raise TargetTraceError("patch unexpectedly changed value descriptors")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_byte_offsets = [
        index
        for index, (before, after) in enumerate(zip(data, patched))
        if before != after
    ]
    return GraphNoCrosshairPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        hitmarker_target_integer_indices=[
            target.index for target in hitmarker_targets
        ],
        hitmarker_setter_record_start_indices=hitmarker_record_starts,
        false_constant_index=false_constant_index,
        crosshair_color_variable_name=color["variable_name"],
        crosshair_color_variable_id=color["variable_id"],
        crosshair_color_setter_state_index=color["setter_state_index"],
        crosshair_color_setter_node_id=color["setter_node_id"],
        crosshair_color_setter_record_start_index=color["setter_record_start"],
        crosshair_color_target_integer_index=color_target.index,
        crosshair_color_original_connection_index=color[
            "original_connection_index"
        ],
        transparent_float4_constant_index=color["transparent_constant_index"],
        residual_ring_renderer_node_ids=(
            residual_ring["renderer_node_ids"] if residual_ring is not None else []
        ),
        residual_ring_renderer_record_start_indices=(
            residual_ring["renderer_record_starts"]
            if residual_ring is not None
            else []
        ),
        residual_ring_target_integer_indices=[
            target.index for target in residual_ring_targets
        ],
        residual_ring_original_float4_constant_index=(
            residual_ring["original_color_constant_index"]
            if residual_ring is not None
            else None
        ),
        residual_ring_sector_count_constant_index=(
            residual_ring["sector_count_constant_index"]
            if residual_ring is not None
            else None
        ),
        changed_byte_offsets=changed_byte_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification=(
            "compiled-hit-and-fatal-setters-fed-constant-false-and-"
            "center-color-setter-fed-transparent-float4-and-kay-"
            "residual-ring-renderers-fed-transparent-float4"
        ),
    )


def patch_verified_weapon_batch_no_crosshair(
    input_graphs_directory: Path,
    output_graphs_directory: Path,
) -> List[GraphNoCrosshairPatchReport]:
    """Build the verified 16-graph hitmarker-plus-crosshair alternative."""

    input_graphs_directory = input_graphs_directory.resolve()
    output_graphs_directory = output_graphs_directory.resolve()
    reports = []
    for filename in VERIFIED_WEAPON_GRAPH_SHA256:
        reports.append(
            patch_weapon_hitmarkers_and_crosshair(
                input_graphs_directory / filename,
                output_graphs_directory / filename,
            )
        )
    return reports


def patch_repeater_has_dealt_damage_gate(
    input_path: Path,
    output_path: Path,
) -> GraphDamageGatePatchReport:
    """Feed the repeater graph's Has Dealt Damage setter constant false."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != REPEATER_BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified repeater-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    record_start, false_constant_index, target = (
        locate_repeater_has_dealt_damage_setter(layout)
    )
    replacement = encode_low_tag_varint32(false_constant_index << 2)
    patched = data[: target.start] + replacement + data[target.end :]

    verified = parse_layout_bytes(patched, str(output_path))
    expected_token = false_constant_index << 2
    if verified.integers[target.index].value != expected_token:
        raise TargetTraceError(
            "patched repeater Has Dealt Damage setter did not decode as constant false"
        )

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != [target.index]:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged:
        raise TargetTraceError("patch changed BV2 layout counts")
    if layout.strings != verified.strings:
        raise TargetTraceError("patch unexpectedly changed the string pool")
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not descriptors_unchanged:
        raise TargetTraceError("patch unexpectedly changed value descriptors")

    patched_setter_record = list(_REPEATER_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD)
    patched_setter_record[6] = false_constant_index
    patched_matches = _find_sequence(
        patched_values,
        _integer_tokens(patched_setter_record),
    )
    if patched_matches != [record_start]:
        raise TargetTraceError(
            "patched repeater Has Dealt Damage setter was not uniquely preserved"
        )
    damage_matches = _find_sequence(
        patched_values,
        _integer_tokens(_REPEATER_DAMAGE_DEALT_RECORD),
    )
    if len(damage_matches) != 1:
        raise TargetTraceError(
            "patch unexpectedly changed the repeater DamageDealt output record"
        )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    return GraphDamageGatePatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        size_delta=len(patched) - len(data),
        has_dealt_damage_setter_record_start_index=record_start,
        false_constant_index=false_constant_index,
        target_integer_index=target.index,
        target_offset=target.start,
        target_end=target.end,
        replacement_end=target.start + len(replacement),
        original_encoding_hex=data[target.start : target.end].hex(" "),
        replacement_encoding_hex=replacement.hex(" "),
        original_connection_index=578,
        replacement_connection_index=false_constant_index,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification="compiled-repeater-has-dealt-damage-setter-fed-constant-false",
    )


def patch_repeater_hitmarker_gates(
    input_path: Path,
    output_path: Path,
) -> GraphHitmarkerGatePatchReport:
    """Force both repeater hit and fatal feedback gates false."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != REPEATER_BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified repeater-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    damage_record_start, false_constant_index, damage_target = (
        locate_repeater_has_dealt_damage_setter(layout)
    )
    fatal_record_start, fatal_target = locate_repeater_fatal_setter(layout)
    replacement = encode_low_tag_varint32(false_constant_index << 2)
    if len(replacement) != damage_target.end - damage_target.start:
        raise TargetTraceError("damage-gate replacement changes encoded width")
    if len(replacement) != fatal_target.end - fatal_target.start:
        raise TargetTraceError("fatal-gate replacement changes encoded width")

    patched_buffer = bytearray(data)
    patched_buffer[damage_target.start : damage_target.end] = replacement
    patched_buffer[fatal_target.start : fatal_target.end] = replacement
    patched = bytes(patched_buffer)

    verified = parse_layout_bytes(patched, str(output_path))
    expected_token = false_constant_index << 2
    for target in (damage_target, fatal_target):
        if verified.integers[target.index].value != expected_token:
            raise TargetTraceError(
                "patched repeater feedback setter did not decode as constant false"
            )

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    target_indices = [damage_target.index, fatal_target.index]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != target_indices:
        raise TargetTraceError(
            f"patch altered unexpected integer records: {differing_values}"
        )

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    if not layout_counts_unchanged:
        raise TargetTraceError("patch changed BV2 layout counts")
    if layout.strings != verified.strings:
        raise TargetTraceError("patch unexpectedly changed the string pool")
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not descriptors_unchanged:
        raise TargetTraceError("patch unexpectedly changed value descriptors")

    patched_damage_record = list(_REPEATER_HAS_DEALT_DAMAGE_SET_VARIABLE_RECORD)
    patched_damage_record[6] = false_constant_index
    if _find_sequence(
        patched_values, _integer_tokens(patched_damage_record)
    ) != [damage_record_start]:
        raise TargetTraceError("patched repeater damage setter was not preserved")

    patched_fatal_record = list(_REPEATER_FATAL_SET_VARIABLE_RECORD)
    patched_fatal_record[6] = false_constant_index
    if _find_sequence(
        patched_values, _integer_tokens(patched_fatal_record)
    ) != [fatal_record_start]:
        raise TargetTraceError("patched repeater Fatal setter was not preserved")

    damage_matches = _find_sequence(
        patched_values,
        _integer_tokens(_REPEATER_DAMAGE_DEALT_RECORD),
    )
    if len(damage_matches) != 1:
        raise TargetTraceError(
            "patch unexpectedly changed the repeater DamageDealt output record"
        )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_byte_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched))
        if before != after
    ]
    return GraphHitmarkerGatePatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        size_delta=len(patched) - len(data),
        false_constant_index=false_constant_index,
        setter_record_start_indices=[damage_record_start, fatal_record_start],
        target_integer_indices=target_indices,
        target_offsets=[damage_target.start, fatal_target.start],
        original_connection_indices=[578, 577],
        replacement_connection_index=false_constant_index,
        changed_byte_offsets=changed_byte_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification="compiled-repeater-hit-and-fatal-setters-fed-constant-false",
    )


def patch_blaster_load_canary(
    input_path: Path,
    output_path: Path,
) -> GraphLoadCanaryPatchReport:
    """Combine the setter experiment with an unmistakable UI:Root-off canary."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified Kay-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    _, false_constant_index, setter_target = (
        locate_blaster_has_dealt_damage_setter(layout)
    )
    _, root_enabled_target = locate_blaster_root_enabled(layout)
    setter_replacement = encode_low_tag_varint32(false_constant_index << 2)
    root_replacement = encode_low_tag_varint32(11)
    if len(setter_replacement) != setter_target.end - setter_target.start:
        raise TargetTraceError("setter canary replacement would change file size")
    if len(root_replacement) != root_enabled_target.end - root_enabled_target.start:
        raise TargetTraceError("UI:Root canary replacement would change file size")

    patched = bytearray(data)
    patched[setter_target.start : setter_target.end] = setter_replacement
    patched[root_enabled_target.start : root_enabled_target.end] = root_replacement
    patched = bytes(patched)
    verified = parse_layout_bytes(patched, str(output_path))

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    expected_indices = sorted([setter_target.index, root_enabled_target.index])
    if differing_values != expected_indices:
        raise TargetTraceError(
            f"canary altered unexpected integer records: {differing_values}"
        )
    if verified.integers[setter_target.index].value != false_constant_index << 2:
        raise TargetTraceError("canary setter did not decode to constant false")
    if verified.integers[root_enabled_target.index].value != 11:
        raise TargetTraceError("canary UI:Root myEnabled did not decode to false")

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not layout_counts_unchanged or not descriptors_unchanged:
        raise TargetTraceError("canary changed layout counts or descriptors")
    if layout.strings != verified.strings or len(data) != len(patched):
        raise TargetTraceError("canary changed strings or file size")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched)) if before != after
    ]
    return GraphLoadCanaryPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        setter_integer_index=setter_target.index,
        setter_original_connection_index=551,
        setter_replacement_connection_index=false_constant_index,
        root_enabled_integer_index=root_enabled_target.index,
        root_enabled_original_token=7,
        root_enabled_replacement_token=11,
        changed_byte_offsets=changed_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification="compiled-setter-plus-ui-root-disabled-load-canary",
    )


def patch_blaster_root_canary(
    input_path: Path,
    output_path: Path,
) -> GraphRootCanaryPatchReport:
    """Disable only optimized UI:Root in an otherwise-vanilla graph."""
    input_path = input_path.resolve()
    output_path = output_path.resolve()
    if input_path == output_path:
        raise TargetTraceError("output path must differ from the input path")
    if output_path.exists():
        raise TargetTraceError(f"refusing to overwrite existing output: {output_path}")

    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != BLASTER_GRAPH_SHA256:
        raise TargetTraceError(
            "input SHA-256 is not the verified Kay-blaster graph revision"
        )

    layout = parse_layout_bytes(data, str(input_path))
    _, target = locate_blaster_root_enabled(layout)
    replacement = encode_low_tag_varint32(11)
    if len(replacement) != target.end - target.start:
        raise TargetTraceError("UI:Root canary replacement would change file size")
    patched = data[: target.start] + replacement + data[target.end :]
    verified = parse_layout_bytes(patched, str(output_path))

    original_values = [entry.value for entry in layout.integers]
    patched_values = [entry.value for entry in verified.integers]
    differing_values = [
        index
        for index, (before, after) in enumerate(zip(original_values, patched_values))
        if before != after
    ]
    if differing_values != [target.index]:
        raise TargetTraceError(
            f"root canary altered unexpected integer records: {differing_values}"
        )
    if verified.integers[target.index].value != 11:
        raise TargetTraceError("root canary did not decode to false")

    layout_counts_unchanged = (
        layout.string_count == verified.string_count
        and layout.value_count == verified.value_count
        and layout.integer_count == verified.integer_count
        and layout.root_value_index == verified.root_value_index
    )
    descriptors_unchanged = (
        [entry.value for entry in layout.descriptors]
        == [entry.value for entry in verified.descriptors]
    )
    if not layout_counts_unchanged or not descriptors_unchanged:
        raise TargetTraceError("root canary changed layout counts or descriptors")
    if layout.strings != verified.strings or len(data) != len(patched):
        raise TargetTraceError("root canary changed strings or file size")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patched)
    changed_offsets = [
        index for index, (before, after) in enumerate(zip(data, patched)) if before != after
    ]
    return GraphRootCanaryPatchReport(
        input_path=str(input_path),
        output_path=str(output_path),
        input_sha256=digest,
        output_sha256=hashlib.sha256(patched).hexdigest(),
        input_size=len(data),
        output_size=len(patched),
        root_enabled_integer_index=target.index,
        root_enabled_original_token=7,
        root_enabled_replacement_token=11,
        changed_byte_offsets=changed_offsets,
        layout_counts_unchanged=layout_counts_unchanged,
        descriptors_unchanged=descriptors_unchanged,
        verification="otherwise-vanilla-compiled-ui-root-disabled-canary",
    )
