from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
import re
from typing import Dict, List, Optional, Tuple


_SEMANTIC_PATTERNS = {
    "ui_hit_feedback": re.compile(
        r'\b(?:Data|value)\s*=\s*"UI_HitFeedback"'
    ),
    "user_option_type": re.compile(
        r'\b(?:Type|valueType)\s*=\s*"User Option"'
    ),
    "multi_select_2": re.compile(r'\btype\s*=\s*"native:MultiSelect2"'),
    "damage_dealt": re.compile(r'HC_CombatNode_DamageDealt'),
    "shared_damage_feedback": re.compile(r'crosshair_damage_feedback\.mcompoundnode'),
}


@dataclass(frozen=True)
class SidecarProbe:
    path: str
    marker_lines: Dict[str, List[int]]
    float_constant_lines: Dict[str, List[int]]
    direct_hit_feedback_chains: List[dict]

    def to_dict(self) -> dict:
        return asdict(self)


def _line_numbers(lines: List[str], pattern: re.Pattern) -> List[int]:
    return [index for index, line in enumerate(lines, start=1) if pattern.search(line)]


@dataclass(frozen=True)
class _Node:
    node_id: int
    line: int
    node_type: Optional[str]
    value: Optional[str]
    value_type: Optional[str]


def _parse_simple_nodes(lines: List[str]) -> Dict[int, _Node]:
    starts = re.compile(r"^(?P<indent>\s*)\[(?P<id>\d+)\]\s*=\s*\{\s*$")
    type_pattern = re.compile(r'^\s*type\s*=\s*"([^"]+)",\s*$')
    value_pattern = re.compile(r'^\s*value\s*=\s*(.+),\s*$')
    value_type_pattern = re.compile(r'^\s*valueType\s*=\s*"([^"]+)",\s*$')
    nodes: Dict[int, _Node] = {}

    for index, line in enumerate(lines):
        match = starts.match(line)
        if not match:
            continue
        indent = match.group("indent")
        body: List[str] = []
        for candidate in lines[index + 1 :]:
            if candidate == indent + "},":
                break
            body.append(candidate)
        else:
            continue

        node_type = None
        value = None
        value_type = None
        for body_line in body:
            type_match = type_pattern.match(body_line)
            value_match = value_pattern.match(body_line)
            value_type_match = value_type_pattern.match(body_line)
            if type_match:
                node_type = type_match.group(1)
            if value_match:
                value = value_match.group(1)
            if value_type_match:
                value_type = value_type_match.group(1)
        node_id = int(match.group("id"))
        nodes[node_id] = _Node(
            node_id=node_id,
            line=index + 1,
            node_type=node_type,
            value=value,
            value_type=value_type,
        )
    return nodes


def _parse_connections(lines: List[str]) -> List[Tuple[int, int, int, int, int]]:
    pattern = re.compile(
        r"^\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+),\s*$"
    )
    result = []
    for line_number, line in enumerate(lines, start=1):
        match = pattern.match(line)
        if match:
            result.append(tuple(int(value) for value in match.groups()) + (line_number,))
    return result


def _extract_direct_hit_feedback_chains(lines: List[str]) -> List[dict]:
    nodes = _parse_simple_nodes(lines)
    connections = _parse_connections(lines)
    chains: List[dict] = []

    ui_constants = [
        node
        for node in nodes.values()
        if node.node_type == "native:Constant"
        and node.value == '"UI_HitFeedback"'
        and node.value_type == "User Option"
    ]
    for ui_node in ui_constants:
        option_edges = [edge for edge in connections if edge[0] == ui_node.node_id]
        option_nodes = [
            nodes.get(edge[2])
            for edge in option_edges
            if nodes.get(edge[2]) is not None
            and nodes[edge[2]].node_type == "HC_UserOptionsNodes_GetOptionInteger"
        ]
        for option_node in option_nodes:
            select_edges = [edge for edge in connections if edge[0] == option_node.node_id]
            select_nodes = [
                nodes.get(edge[2])
                for edge in select_edges
                if nodes.get(edge[2]) is not None
                and nodes[edge[2]].node_type == "native:MultiSelect2"
            ]
            for select_node in select_nodes:
                constants_by_pin = {}
                connection_lines = []
                for edge in connections:
                    source_id, _source_pin, target_id, target_pin, edge_line = edge
                    if target_id != select_node.node_id or target_pin not in (2, 3, 4):
                        continue
                    source = nodes.get(source_id)
                    if source is None or source.node_type != "native:Constant":
                        continue
                    constants_by_pin[target_pin] = source
                    connection_lines.append(edge_line)
                if set(constants_by_pin) != {2, 3, 4}:
                    continue
                chains.append(
                    {
                        "user_option_constant_id": ui_node.node_id,
                        "get_option_node_id": option_node.node_id,
                        "multi_select_node_id": select_node.node_id,
                        "reduced_constant_id": constants_by_pin[2].node_id,
                        "reduced_value": constants_by_pin[2].value,
                        "normal_constant_id": constants_by_pin[3].node_id,
                        "normal_value": constants_by_pin[3].value,
                        "large_constant_id": constants_by_pin[4].node_id,
                        "large_value": constants_by_pin[4].value,
                        "node_lines": {
                            "user_option": ui_node.line,
                            "get_option": option_node.line,
                            "multi_select": select_node.line,
                            "reduced": constants_by_pin[2].line,
                            "normal": constants_by_pin[3].line,
                            "large": constants_by_pin[4].line,
                        },
                        "constant_connection_lines": sorted(connection_lines),
                    }
                )
    return chains


def probe_sidecar(path: Path) -> SidecarProbe:
    path = path.resolve()
    lines = path.read_text(encoding="utf-8", errors="strict").splitlines()
    marker_lines = {
        name: _line_numbers(lines, pattern)
        for name, pattern in _SEMANTIC_PATTERNS.items()
    }
    float_constant_lines = {
        value: _line_numbers(
            lines,
            re.compile(
                rf"\b(?:Data|value)\s*=\s*{re.escape(value)}(?:0*)?\s*,\s*$"
            ),
        )
        for value in ("0", "0.75", "1", "1.5")
    }
    return SidecarProbe(
        path=str(path),
        marker_lines=marker_lines,
        float_constant_lines=float_constant_lines,
        direct_hit_feedback_chains=_extract_direct_hit_feedback_chains(lines),
    )


def sidecar_for(asset_path: Path) -> Optional[Path]:
    candidate = Path(str(asset_path) + ".decompiled")
    return candidate if candidate.is_file() else None

