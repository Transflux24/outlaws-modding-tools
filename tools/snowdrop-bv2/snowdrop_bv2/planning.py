from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
from typing import List, Optional

from .binary import BV2FormatError, iter_bv2_files, probe_file
from .sidecar import probe_sidecar, sidecar_for


@dataclass(frozen=True)
class HitFeedbackTarget:
    path: str
    sha256: Optional[str]
    status: str
    reason: str
    implementation: str
    ui_hit_feedback_lines: List[int]
    multi_select_lines: List[int]
    damage_dealt_lines: List[int]
    reduced_value_lines: List[int]
    direct_semantic_chains: List[dict]

    def to_dict(self) -> dict:
        return asdict(self)


def plan_hit_feedback(root: Path) -> List[HitFeedbackTarget]:
    targets: List[HitFeedbackTarget] = []
    for path in iter_bv2_files(root):
        try:
            binary = probe_file(path)
        except BV2FormatError as error:
            targets.append(
                HitFeedbackTarget(
                    path=str(path.resolve()),
                    sha256=None,
                    status="invalid",
                    reason=str(error),
                    implementation="unknown",
                    ui_hit_feedback_lines=[],
                    multi_select_lines=[],
                    damage_dealt_lines=[],
                    reduced_value_lines=[],
                    direct_semantic_chains=[],
                )
            )
            continue

        sidecar_path = sidecar_for(path)
        if sidecar_path is None:
            targets.append(
                HitFeedbackTarget(
                    path=str(path.resolve()),
                    sha256=binary.sha256,
                    status="needs-sidecar",
                    reason="Hunter .decompiled sidecar is missing",
                    implementation="unknown",
                    ui_hit_feedback_lines=[],
                    multi_select_lines=[],
                    damage_dealt_lines=[],
                    reduced_value_lines=[],
                    direct_semantic_chains=[],
                )
            )
            continue

        sidecar = probe_sidecar(sidecar_path)
        ui_lines = sidecar.marker_lines["ui_hit_feedback"]
        multi_lines = sidecar.marker_lines["multi_select_2"]
        damage_lines = sidecar.marker_lines["damage_dealt"]
        reduced_lines = sidecar.float_constant_lines["0.75"]
        shared_lines = sidecar.marker_lines["shared_damage_feedback"]

        if not ui_lines:
            status = "excluded"
            reason = "No UI_HitFeedback option in Hunter sidecar"
        elif not multi_lines:
            status = "investigate"
            reason = "UI_HitFeedback exists but expected MultiSelect2 is absent"
        elif path.name == "hud_combat_damage_direction.muigraph":
            status = "excluded"
            reason = "Incoming-damage direction UI is outside the hitmarker patch"
        else:
            status = "candidate"
            reason = "Semantic markers present; graph connectivity still requires decoding"

        if path.suffix.lower() == ".mcompoundnode":
            implementation = "shared-component"
        elif shared_lines:
            implementation = "references-shared-component"
        else:
            implementation = "inline-or-unresolved"

        targets.append(
            HitFeedbackTarget(
                path=str(path.resolve()),
                sha256=binary.sha256,
                status=status,
                reason=reason,
                implementation=implementation,
                ui_hit_feedback_lines=ui_lines,
                multi_select_lines=multi_lines,
                damage_dealt_lines=damage_lines,
                reduced_value_lines=reduced_lines,
                direct_semantic_chains=sidecar.direct_hit_feedback_chains,
            )
        )
    return targets

