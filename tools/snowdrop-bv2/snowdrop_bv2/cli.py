from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any, Iterable

from . import __version__
from .binary import BV2FormatError, iter_bv2_files, probe_file
from .layout import parse_layout
from .patching import (
    patch_blaster_feedback_alpha,
    patch_blaster_feedback_renderer,
    patch_blaster_has_dealt_damage_gate,
    patch_blaster_hit_feedback,
    patch_blaster_hostile_reticle_color,
    patch_blaster_load_canary,
    patch_blaster_root_canary,
    patch_hit_feedback,
    patch_kay_blaster_batch,
    patch_kay_hitmarker_gates,
    patch_repeater_has_dealt_damage_gate,
    patch_repeater_hitmarker_gates,
    patch_weapon_hitmarker_gates,
    patch_weapon_hitmarkers_and_crosshair,
    patch_verified_weapon_batch,
    patch_verified_weapon_batch_no_crosshair,
)
from .planning import plan_hit_feedback
from .sidecar import probe_sidecar, sidecar_for
from .targeting import TargetTraceError, trace_hit_feedback


def _emit(payload: Any, output: Path = None) -> None:
    rendered = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    if output is None:
        sys.stdout.write(rendered)
        return
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(rendered, encoding="utf-8")


def _inspect(path: Path) -> dict:
    binary = probe_file(path).to_dict()
    sidecar_path = sidecar_for(path)
    return {
        "tool_version": __version__,
        "binary": binary,
        "hunter_sidecar": probe_sidecar(sidecar_path).to_dict() if sidecar_path else None,
    }


def _corpus(root: Path) -> dict:
    assets = []
    errors = []
    for path in iter_bv2_files(root):
        try:
            assets.append(_inspect(path))
        except (BV2FormatError, UnicodeError, OSError) as error:
            errors.append({"path": str(path.resolve()), "error": str(error)})
    return {
        "tool_version": __version__,
        "root": str(root.resolve()),
        "asset_count": len(assets),
        "error_count": len(errors),
        "assets": assets,
        "errors": errors,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="snowdrop-bv2",
        description="Snowdrop FFBV2 inspection and guarded patch generation",
    )
    parser.add_argument("--version", action="version", version=__version__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    inspect_parser = subparsers.add_parser("inspect", help="inspect one BV2 asset")
    inspect_parser.add_argument("path", type=Path)
    inspect_parser.add_argument("--output", type=Path)

    corpus_parser = subparsers.add_parser("corpus", help="inventory a BV2 asset corpus")
    corpus_parser.add_argument("root", type=Path)
    corpus_parser.add_argument("--output", type=Path)

    plan_parser = subparsers.add_parser(
        "plan-hit-feedback",
        help="produce a read-only semantic target plan from Hunter sidecars",
    )
    plan_parser.add_argument("root", type=Path)
    plan_parser.add_argument("--output", type=Path)

    trace_parser = subparsers.add_parser(
        "trace-hit-feedback",
        help="locate the exact shared-component semantic connection run",
    )
    trace_parser.add_argument("path", type=Path)
    trace_parser.add_argument("--output", type=Path)

    layout_parser = subparsers.add_parser(
        "layout", help="decode the FFBV2 string, integer, and descriptor pools"
    )
    layout_parser.add_argument("path", type=Path)
    layout_parser.add_argument("--output", type=Path)

    patch_parser = subparsers.add_parser(
        "patch-hit-feedback",
        help="write a guarded shared-component patch for the Reduced value",
    )
    patch_parser.add_argument("path", type=Path)
    patch_parser.add_argument("patched_path", type=Path)
    patch_parser.add_argument(
        "--value",
        type=float,
        default=0.0,
        help="replacement Reduced multiplier (default: 0.0)",
    )
    patch_parser.add_argument("--report", type=Path)

    blaster_patch_parser = subparsers.add_parser(
        "patch-blaster-hit-feedback",
        help="write a guarded Kay-blaster graph patch for the Reduced value",
    )
    blaster_patch_parser.add_argument("path", type=Path)
    blaster_patch_parser.add_argument("patched_path", type=Path)
    blaster_patch_parser.add_argument(
        "--value",
        type=float,
        default=0.0,
        help="replacement Reduced multiplier (default: 0.0)",
    )
    blaster_patch_parser.add_argument("--report", type=Path)

    renderer_patch_parser = subparsers.add_parser(
        "patch-blaster-feedback-renderer",
        help="disable Kay-blaster hit-feedback sector rendering",
    )
    renderer_patch_parser.add_argument("path", type=Path)
    renderer_patch_parser.add_argument("patched_path", type=Path)
    renderer_patch_parser.add_argument("--report", type=Path)

    alpha_patch_parser = subparsers.add_parser(
        "patch-blaster-feedback-alpha",
        help="make Kay-blaster damage-feedback colors transparent",
    )
    alpha_patch_parser.add_argument("path", type=Path)
    alpha_patch_parser.add_argument("patched_path", type=Path)
    alpha_patch_parser.add_argument("--report", type=Path)

    reticle_color_parser = subparsers.add_parser(
        "patch-blaster-hostile-reticle-color",
        help="replace the hostile center-reticle color with the default color",
    )
    reticle_color_parser.add_argument("path", type=Path)
    reticle_color_parser.add_argument("patched_path", type=Path)
    reticle_color_parser.add_argument("--report", type=Path)

    damage_gate_parser = subparsers.add_parser(
        "patch-blaster-has-dealt-damage-gate",
        help="feed Kay-blaster Has Dealt Damage setter an explicit false",
    )
    damage_gate_parser.add_argument("path", type=Path)
    damage_gate_parser.add_argument("patched_path", type=Path)
    damage_gate_parser.add_argument("--report", type=Path)

    repeater_damage_gate_parser = subparsers.add_parser(
        "patch-repeater-has-dealt-damage-gate",
        help="feed repeater-blaster Has Dealt Damage setter an explicit false",
    )
    repeater_damage_gate_parser.add_argument("path", type=Path)
    repeater_damage_gate_parser.add_argument("patched_path", type=Path)
    repeater_damage_gate_parser.add_argument("--report", type=Path)

    repeater_hitmarker_parser = subparsers.add_parser(
        "patch-repeater-hitmarker-gates",
        help="force repeater-blaster hit and fatal feedback gates false",
    )
    repeater_hitmarker_parser.add_argument("path", type=Path)
    repeater_hitmarker_parser.add_argument("patched_path", type=Path)
    repeater_hitmarker_parser.add_argument("--report", type=Path)

    kay_graph_parser = subparsers.add_parser(
        "patch-kay-hitmarker-gates",
        help="force one verified Kay-blaster graph's hit and fatal gates false",
    )
    kay_graph_parser.add_argument("path", type=Path)
    kay_graph_parser.add_argument("patched_path", type=Path)
    kay_graph_parser.add_argument("--report", type=Path)

    kay_batch_parser = subparsers.add_parser(
        "patch-kay-blaster-batch",
        help="patch all verified Kay-blaster graphs into a new directory",
    )
    kay_batch_parser.add_argument("input_graphs_directory", type=Path)
    kay_batch_parser.add_argument("output_graphs_directory", type=Path)
    kay_batch_parser.add_argument("--report", type=Path)

    weapon_graph_parser = subparsers.add_parser(
        "patch-weapon-hitmarker-gates",
        help="force one verified weapon graph's hit and fatal gates false",
    )
    weapon_graph_parser.add_argument("path", type=Path)
    weapon_graph_parser.add_argument("patched_path", type=Path)
    weapon_graph_parser.add_argument("--report", type=Path)

    weapon_batch_parser = subparsers.add_parser(
        "patch-verified-weapon-batch",
        help="patch all verified Kay and pickup-weapon graphs",
    )
    weapon_batch_parser.add_argument("input_graphs_directory", type=Path)
    weapon_batch_parser.add_argument("output_graphs_directory", type=Path)
    weapon_batch_parser.add_argument("--report", type=Path)

    no_crosshair_batch_parser = subparsers.add_parser(
        "patch-verified-weapon-batch-no-crosshair",
        help="patch verified hitmarkers and center crosshairs while retaining bars",
    )
    no_crosshair_batch_parser.add_argument("input_graphs_directory", type=Path)
    no_crosshair_batch_parser.add_argument("output_graphs_directory", type=Path)
    no_crosshair_batch_parser.add_argument("--report", type=Path)

    no_crosshair_graph_parser = subparsers.add_parser(
        "patch-weapon-hitmarkers-and-crosshair",
        help="patch one verified weapon graph's hitmarkers and center crosshair",
    )
    no_crosshair_graph_parser.add_argument("path", type=Path)
    no_crosshair_graph_parser.add_argument("patched_path", type=Path)
    no_crosshair_graph_parser.add_argument("--report", type=Path)

    load_canary_parser = subparsers.add_parser(
        "patch-blaster-load-canary",
        help="combine the hitmarker setter experiment with a UI:Root-off canary",
    )
    load_canary_parser.add_argument("path", type=Path)
    load_canary_parser.add_argument("patched_path", type=Path)
    load_canary_parser.add_argument("--report", type=Path)

    root_canary_parser = subparsers.add_parser(
        "patch-blaster-root-canary",
        help="disable only optimized UI:Root in an otherwise-vanilla graph",
    )
    root_canary_parser.add_argument("path", type=Path)
    root_canary_parser.add_argument("patched_path", type=Path)
    root_canary_parser.add_argument("--report", type=Path)
    return parser


def main(argv: Iterable[str] = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.command == "inspect":
            _emit(_inspect(args.path), args.output)
        elif args.command == "corpus":
            _emit(_corpus(args.root), args.output)
        elif args.command == "plan-hit-feedback":
            payload = {
                "tool_version": __version__,
                "root": str(args.root.resolve()),
                "targets": [target.to_dict() for target in plan_hit_feedback(args.root)],
            }
            _emit(payload, args.output)
        elif args.command == "trace-hit-feedback":
            _emit(trace_hit_feedback(args.path).to_dict(), args.output)
        elif args.command == "layout":
            _emit(parse_layout(args.path).summary(), args.output)
        elif args.command == "patch-hit-feedback":
            report = patch_hit_feedback(
                args.path,
                args.patched_path,
                replacement_value=args.value,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-blaster-hit-feedback":
            report = patch_blaster_hit_feedback(
                args.path,
                args.patched_path,
                replacement_value=args.value,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-blaster-feedback-renderer":
            report = patch_blaster_feedback_renderer(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-blaster-feedback-alpha":
            report = patch_blaster_feedback_alpha(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-blaster-hostile-reticle-color":
            report = patch_blaster_hostile_reticle_color(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-blaster-has-dealt-damage-gate":
            report = patch_blaster_has_dealt_damage_gate(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-repeater-has-dealt-damage-gate":
            report = patch_repeater_has_dealt_damage_gate(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-repeater-hitmarker-gates":
            report = patch_repeater_hitmarker_gates(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-kay-hitmarker-gates":
            report = patch_kay_hitmarker_gates(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-kay-blaster-batch":
            reports = patch_kay_blaster_batch(
                args.input_graphs_directory,
                args.output_graphs_directory,
            )
            _emit(
                {
                    "verification": "all-kay-blaster-hitmarker-gates-patched",
                    "files": [report.to_dict() for report in reports],
                },
                args.report,
            )
        elif args.command == "patch-weapon-hitmarker-gates":
            report = patch_weapon_hitmarker_gates(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-verified-weapon-batch":
            reports = patch_verified_weapon_batch(
                args.input_graphs_directory,
                args.output_graphs_directory,
            )
            _emit(
                {
                    "verification": "all-verified-weapon-hitmarker-gates-patched",
                    "files": [report.to_dict() for report in reports],
                },
                args.report,
            )
        elif args.command == "patch-verified-weapon-batch-no-crosshair":
            reports = patch_verified_weapon_batch_no_crosshair(
                args.input_graphs_directory,
                args.output_graphs_directory,
            )
            _emit(
                {
                    "verification": (
                        "all-verified-weapon-hitmarker-gates-and-"
                        "center-crosshair-colors-and-kay-residual-rings-patched"
                    ),
                    "files": [report.to_dict() for report in reports],
                },
                args.report,
            )
        elif args.command == "patch-weapon-hitmarkers-and-crosshair":
            report = patch_weapon_hitmarkers_and_crosshair(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-blaster-load-canary":
            report = patch_blaster_load_canary(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        elif args.command == "patch-blaster-root-canary":
            report = patch_blaster_root_canary(
                args.path,
                args.patched_path,
            )
            _emit(report.to_dict(), args.report)
        else:
            raise AssertionError(f"unhandled command: {args.command}")
    except (BV2FormatError, TargetTraceError, UnicodeError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 0

