import os
from pathlib import Path
import sys
import tempfile
import unittest


TOOL_ROOT = Path(__file__).resolve().parents[1]
CORPUS_ROOT = Path(os.environ.get("OUTLAWS_BV2_CORPUS", TOOL_ROOT / "corpus"))
sys.path.insert(0, str(TOOL_ROOT))

from snowdrop_bv2.binary import iter_bv2_files, probe_file  # noqa: E402
from snowdrop_bv2.planning import plan_hit_feedback  # noqa: E402
from snowdrop_bv2.layout import (  # noqa: E402
    decode_inline_float_token,
    encode_inline_float_token,
    parse_layout,
    parse_layout_bytes,
)
from snowdrop_bv2.patching import (  # noqa: E402
    BLASTER_GRAPH_SHA256,
    KAY_BLASTER_GRAPH_SHA256,
    REPEATER_BLASTER_GRAPH_SHA256,
    VERIFIED_WEAPON_GRAPH_SHA256,
    locate_blaster_feedback_alpha_values,
    locate_blaster_feedback_sector_count,
    locate_blaster_has_dealt_damage_connection,
    locate_blaster_has_dealt_damage_setter,
    locate_blaster_hostile_reticle_color,
    locate_blaster_root_enabled,
    locate_repeater_has_dealt_damage_connection,
    locate_repeater_has_dealt_damage_setter,
    locate_repeater_fatal_setter,
    locate_weapon_hitmarker_gate_sets,
    locate_weapon_crosshair_color_setter,
    locate_kay_residual_ring_renderers,
    locate_kay_faint_center_ring_renderer,
    locate_graph_reduced_value,
    locate_kay_hitmarker_gates,
    locate_reduced_value,
    patch_verified_weapon_batch_no_crosshair,
)
from snowdrop_bv2.sidecar import probe_sidecar  # noqa: E402
from snowdrop_bv2.targeting import (  # noqa: E402
    SHARED_COMPONENT_SHA256,
    trace_hit_feedback,
)


@unittest.skipUnless(CORPUS_ROOT.is_dir(), "Outlaws research corpus is unavailable")
class OutlawsCorpusTests(unittest.TestCase):
    def test_all_21_assets_are_valid_bv2(self):
        files = list(iter_bv2_files(CORPUS_ROOT))
        self.assertEqual(len(files), 21)
        for path in files:
            probe = probe_file(path)
            self.assertEqual(probe.version_label, "FFBV2")
            self.assertIsNotNone(probe.string_table)

    def test_all_21_assets_have_fully_consumed_pool_layouts(self):
        files = list(iter_bv2_files(CORPUS_ROOT))
        self.assertEqual(len(files), 21)
        for path in files:
            layout = parse_layout(path)
            self.assertEqual(layout.descriptors_end, path.stat().st_size)

    def test_hit_feedback_plan_is_conservative(self):
        plan = plan_hit_feedback(CORPUS_ROOT)
        candidates = [item for item in plan if item.status == "candidate"]
        excluded = [item for item in plan if item.status == "excluded"]
        self.assertEqual(len(candidates), 17)
        self.assertEqual(len(excluded), 4)
        self.assertTrue(
            any("grenadelauncher" in item.path for item in excluded),
            "the grenade launcher must remain outside the patch plan",
        )

    def test_blaster_does_not_contain_plain_target_float_encodings(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        probe = probe_file(blaster)
        self.assertEqual(probe.raw_float32_offsets["0.75"], [])
        self.assertEqual(probe.raw_float32_offsets["1.5"], [])

    def test_blaster_compiled_reduced_token_is_uniquely_located(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        layout = parse_layout(blaster)
        target = locate_graph_reduced_value(layout)
        self.assertEqual(target.index, 1828)
        self.assertEqual(target.start, 23166)
        self.assertEqual(target.end, 23171)
        self.assertEqual(target.encoding_hex, "03 01 00 00 3d")
        self.assertEqual(decode_inline_float_token(target.value), 0.75)

    def test_blaster_hash_gate_matches_current_extraction(self):
        import hashlib

        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        self.assertEqual(hashlib.sha256(blaster.read_bytes()).hexdigest(), BLASTER_GRAPH_SHA256)

    def test_blaster_feedback_sector_count_is_uniquely_located(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        layout = parse_layout(blaster)
        target = locate_blaster_feedback_sector_count(layout)
        self.assertEqual(target.index, 2908)
        self.assertEqual(target.start, 25190)
        self.assertEqual(target.end, 25191)
        self.assertEqual(target.encoding_hex, "40")
        self.assertEqual(target.value, 16)

    def test_blaster_damage_feedback_alpha_pattern_is_uniquely_located(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        layout = parse_layout(blaster)
        targets = locate_blaster_feedback_alpha_values(layout)
        self.assertEqual([target.index for target in targets], [3248, 3250, 3252, 3254])
        self.assertEqual(
            [target.start for target in targets],
            [25806, 25812, 25818, 25824],
        )
        self.assertTrue(all(target.encoding_hex == "03 01 00 00 3e" for target in targets))
        self.assertTrue(all(decode_inline_float_token(target.value) == 1.0 for target in targets))

    def test_blaster_hostile_reticle_color_is_uniquely_located(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        layout = parse_layout(blaster)
        hostile, default = locate_blaster_hostile_reticle_color(layout)
        self.assertEqual([target.index for target in hostile], [2016, 2017, 2018, 2019])
        self.assertEqual([target.start for target in hostile], [23524, 23529, 23534, 23539])
        self.assertEqual([target.value for target in hostile], [1605619105, -1249785144, 1578440476, 75])
        self.assertEqual([target.index for target in default], [2003, 2004, 2005, 2006])
        self.assertEqual([target.value for target in default], [1605619021, -1249785144, -774866124, 66])

    def test_blaster_has_dealt_damage_connection_is_uniquely_located(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        layout = parse_layout(blaster)
        record_start, target = locate_blaster_has_dealt_damage_connection(layout)
        self.assertEqual(record_start, 16789)
        self.assertEqual(target.index, 16795)
        self.assertEqual(target.value, 551 << 2)
        self.assertEqual(target.encoding_hex, "71 22")

    def test_blaster_has_dealt_damage_setter_and_false_constant_are_unique(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        layout = parse_layout(blaster)
        record_start, false_constant_index, target = (
            locate_blaster_has_dealt_damage_setter(layout)
        )
        self.assertEqual(record_start, 7721)
        self.assertEqual(false_constant_index, 200)
        self.assertEqual(target.index, 7727)
        self.assertEqual(target.start, 34197)
        self.assertEqual(target.encoding_hex, "71 22")

    def test_repeater_hash_gate_matches_current_extraction(self):
        import hashlib

        repeater = (
            CORPUS_ROOT
            / "helix"
            / "ui"
            / "graphs"
            / "crosshair_pistol_blaster_repeater_overheat.muigraph"
        )
        self.assertEqual(
            hashlib.sha256(repeater.read_bytes()).hexdigest(),
            REPEATER_BLASTER_GRAPH_SHA256,
        )

    def test_repeater_has_dealt_damage_connection_is_uniquely_located(self):
        repeater = (
            CORPUS_ROOT
            / "helix"
            / "ui"
            / "graphs"
            / "crosshair_pistol_blaster_repeater_overheat.muigraph"
        )
        layout = parse_layout(repeater)
        record_start, target = locate_repeater_has_dealt_damage_connection(layout)
        self.assertEqual(record_start, 17396)
        self.assertEqual(target.index, 17402)
        self.assertEqual(target.value, 578 << 2)
        self.assertEqual(target.encoding_hex, "21 24")

    def test_repeater_has_dealt_damage_setter_and_false_constant_are_unique(self):
        repeater = (
            CORPUS_ROOT
            / "helix"
            / "ui"
            / "graphs"
            / "crosshair_pistol_blaster_repeater_overheat.muigraph"
        )
        layout = parse_layout(repeater)
        record_start, false_constant_index, target = (
            locate_repeater_has_dealt_damage_setter(layout)
        )
        self.assertEqual(record_start, 7931)
        self.assertEqual(false_constant_index, 210)
        self.assertEqual(target.index, 7937)
        self.assertEqual(target.start, 35021)
        self.assertEqual(target.encoding_hex, "21 24")

    def test_repeater_fatal_setter_is_uniquely_located(self):
        repeater = (
            CORPUS_ROOT
            / "helix"
            / "ui"
            / "graphs"
            / "crosshair_pistol_blaster_repeater_overheat.muigraph"
        )
        layout = parse_layout(repeater)
        record_start, target = locate_repeater_fatal_setter(layout)
        self.assertEqual(record_start, 7940)
        self.assertEqual(target.index, 7946)
        self.assertEqual(target.start, 35037)
        self.assertEqual(target.value, 577 << 2)
        self.assertEqual(target.encoding_hex, "11 24")

    def test_all_kay_graph_hashes_and_feedback_gates_are_verified(self):
        expected = {
            "crosshair_blaster.muigraph": (16789, 7721, 7730, 200, 551, 549),
            "crosshair_pistol_blaster_heavy_overheat.muigraph": (
                17403, 7938, 7947, 210, 574, 575,
            ),
            "crosshair_pistol_blaster_overheat.muigraph": (
                17396, 7931, 7940, 210, 578, 577,
            ),
            "crosshair_pistol_blaster_repeater_overheat.muigraph": (
                17396, 7931, 7940, 210, 578, 577,
            ),
            "crosshair_pistol_chargeup_concussion_blast_overheat.muigraph": (
                20915, 10993, 11002, 253, 679, 678,
            ),
            "crosshair_pistol_chargeup_overheat.muigraph": (
                20915, 10993, 11002, 253, 679, 678,
            ),
            "crosshair_pistol_chargeup_phasepulse_overheat.muigraph": (
                20915, 10993, 11002, 253, 679, 678,
            ),
            "crosshair_pistol_ion_blast_overheat.muigraph": (
                18773, 9441, 9450, 231, 625, 629,
            ),
            "crosshair_pistol_ion_overheat.muigraph": (
                18773, 9441, 9450, 231, 625, 629,
            ),
        }
        import hashlib

        self.assertEqual(set(KAY_BLASTER_GRAPH_SHA256), set(expected))
        for filename, values in expected.items():
            path = CORPUS_ROOT / "helix" / "ui" / "graphs" / filename
            self.assertEqual(
                hashlib.sha256(path.read_bytes()).hexdigest(),
                KAY_BLASTER_GRAPH_SHA256[filename],
            )
            gates = locate_kay_hitmarker_gates(parse_layout(path))
            self.assertEqual(gates["damage_record_start"], values[0])
            self.assertEqual(gates["has_record_start"], values[1])
            self.assertEqual(gates["fatal_record_start"], values[2])
            self.assertEqual(gates["false_constant_index"], values[3])
            self.assertEqual(gates["has_dealt_connection"], values[4])
            self.assertEqual(gates["fatal_connection"], values[5])

    def test_e11_assault_rifle_graph_hash_and_feedback_gates_are_verified(self):
        import hashlib

        filename = "crosshair_assault_rifle_ammunition.muigraph"
        path = CORPUS_ROOT / "helix" / "ui" / "graphs" / filename
        self.assertEqual(
            hashlib.sha256(path.read_bytes()).hexdigest(),
            VERIFIED_WEAPON_GRAPH_SHA256[filename],
        )
        gates = locate_kay_hitmarker_gates(parse_layout(path))
        self.assertEqual(gates["damage_record_start"], 15243)
        self.assertEqual(gates["has_record_start"], 8458)
        self.assertEqual(gates["fatal_record_start"], 8467)
        self.assertEqual(gates["false_constant_index"], 176)
        self.assertEqual(gates["has_dealt_connection"], 528)
        self.assertEqual(gates["fatal_connection"], 525)
        self.assertEqual(gates["has_target"].start, 33788)
        self.assertEqual(gates["fatal_target"].start, 33804)

    def test_remaining_pickup_weapon_graphs_are_verified(self):
        import hashlib

        expected = {
            "crosshair_burst_assault_rifle_ammunition.muigraph": (
                176, [(15243, 8458, 8467, 528, 525)],
            ),
            "crosshair_marksmanrifle_ammunition.muigraph": (
                204, [(16894, 11739, 11748, 657, 658)],
            ),
            "crosshair_marksmanrifle_chargeup_ammunition.muigraph": (
                291,
                [
                    (23991, 6733, 6742, 854, 853),
                    (24021, 18836, 18845, 860, 857),
                ],
            ),
            "crosshair_myrmite_rifle_ammunition.muigraph": (
                126, [(12028, 5243, 5252, 434, 437)],
            ),
            "crosshair_rifle_ammunition.muigraph": (
                176, [(15243, 8458, 8467, 528, 525)],
            ),
            "crosshair_rotarycannon.muigraph": (
                122, [(11396, 4611, 4620, 412, 419)],
            ),
        }
        for filename, (false_index, expected_sets) in expected.items():
            path = CORPUS_ROOT / "helix" / "ui" / "graphs" / filename
            self.assertEqual(
                hashlib.sha256(path.read_bytes()).hexdigest(),
                VERIFIED_WEAPON_GRAPH_SHA256[filename],
            )
            gate_sets, located_false = locate_weapon_hitmarker_gate_sets(
                parse_layout(path)
            )
            self.assertEqual(located_false, false_index)
            observed = [
                (
                    gates["damage_record_start"],
                    gates["has_record_start"],
                    gates["fatal_record_start"],
                    gates["has_dealt_connection"],
                    gates["fatal_connection"],
                )
                for gates in gate_sets
            ]
            self.assertEqual(observed, expected_sets)

    def test_all_verified_crosshair_color_setters_are_semantically_located(self):
        graphs = CORPUS_ROOT / "helix" / "ui" / "graphs"
        expected = {
            "crosshair_blaster.muigraph": ("Main Color", 5270, 344),
            "crosshair_pistol_blaster_heavy_overheat.muigraph": ("Main Color", 5487, 361),
            "crosshair_pistol_blaster_overheat.muigraph": ("Main Color", 5480, 360),
            "crosshair_pistol_blaster_repeater_overheat.muigraph": ("Main Color", 5480, 360),
            "crosshair_pistol_chargeup_concussion_blast_overheat.muigraph": ("Crosshair Color", 6169, 408),
            "crosshair_pistol_chargeup_overheat.muigraph": ("Crosshair Color", 6169, 408),
            "crosshair_pistol_chargeup_phasepulse_overheat.muigraph": ("Crosshair Color", 6169, 408),
            "crosshair_pistol_ion_blast_overheat.muigraph": ("Crosshair Color", 5605, 380),
            "crosshair_pistol_ion_overheat.muigraph": ("Crosshair Color", 5605, 380),
            "crosshair_assault_rifle_ammunition.muigraph": ("Crosshair Color", 3987, 299),
            "crosshair_burst_assault_rifle_ammunition.muigraph": ("Crosshair Color", 3987, 299),
            "crosshair_marksmanrifle_ammunition.muigraph": ("Crosshair Color", 4522, 350),
            "crosshair_marksmanrifle_chargeup_ammunition.muigraph": ("Crosshair Color", 13699, 434),
            "crosshair_myrmite_rifle_ammunition.muigraph": ("Crosshair Color", 3296, 228),
            "crosshair_rifle_ammunition.muigraph": ("Crosshair Color", 3987, 299),
            "crosshair_rotarycannon.muigraph": ("Crosshair Color", 3691, 226),
        }
        self.assertEqual(set(expected), set(VERIFIED_WEAPON_GRAPH_SHA256))
        for filename, (variable_name, record_start, constant_index) in expected.items():
            path = graphs / filename
            target = locate_weapon_crosshair_color_setter(parse_layout(path), path)
            self.assertEqual(target["variable_name"], variable_name)
            self.assertEqual(target["setter_record_start"], record_start)
            self.assertEqual(target["transparent_constant_index"], constant_index)
            self.assertEqual(
                target["setter_record"],
                [
                    target["setter_state_index"],
                    target["setter_node_id"],
                    -1,
                    target["setter_record"][3],
                    2,
                    target["setter_record"][5],
                    target["original_connection_index"],
                ],
            )
            sidecar_text = path.with_name(path.name + ".decompiled").read_text()
            self.assertIn("Data = 0 0 1 0", sidecar_text)

    def test_all_kay_residual_ring_renderers_are_semantically_located(self):
        graphs = CORPUS_ROOT / "helix" / "ui" / "graphs"
        expected = {
            "crosshair_blaster.muigraph": ([13425, 13526], [13433, 13534], 220, 344, 271),
            "crosshair_pistol_blaster_heavy_overheat.muigraph": ([13642, 13743], [13650, 13751], 230, 361, 281),
            "crosshair_pistol_blaster_overheat.muigraph": ([13635, 13736], [13643, 13744], 230, 360, 281),
            "crosshair_pistol_blaster_repeater_overheat.muigraph": ([13635, 13736], [13643, 13744], 230, 360, 281),
            "crosshair_pistol_chargeup_concussion_blast_overheat.muigraph": ([17154, 17255], [17162, 17263], 273, 408, 325),
            "crosshair_pistol_chargeup_overheat.muigraph": ([17154, 17255], [17162, 17263], 273, 408, 325),
            "crosshair_pistol_chargeup_phasepulse_overheat.muigraph": ([17154, 17255], [17162, 17263], 273, 408, 325),
            "crosshair_pistol_ion_blast_overheat.muigraph": ([15012, 15113], [15020, 15121], 251, 380, 303),
            "crosshair_pistol_ion_overheat.muigraph": ([15012, 15113], [15020, 15121], 251, 380, 303),
        }
        self.assertEqual(set(expected), set(KAY_BLASTER_GRAPH_SHA256))
        for filename, expected_values in expected.items():
            path = graphs / filename
            located = locate_kay_residual_ring_renderers(parse_layout(path), path)
            starts, targets, white, transparent, sector_count = expected_values
            self.assertEqual(located["renderer_node_ids"], [405370, 405656])
            self.assertEqual(located["renderer_record_starts"], starts)
            self.assertEqual(
                [target.index for target in located["targets"]], targets
            )
            self.assertEqual(located["white_constant_index"], white)
            self.assertEqual(located["transparent_constant_index"], transparent)
            self.assertEqual(located["sector_count_constant_index"], sector_count)

    def test_all_kay_faint_center_ring_renderers_are_semantically_located(self):
        graphs = CORPUS_ROOT / "helix" / "ui" / "graphs"
        expected = {
            "crosshair_blaster.muigraph": (274, 344, 271, 335, 336),
            "crosshair_pistol_blaster_heavy_overheat.muigraph": (284, 361, 281, 352, 353),
            "crosshair_pistol_blaster_overheat.muigraph": (284, 360, 281, 351, 352),
            "crosshair_pistol_blaster_repeater_overheat.muigraph": (284, 360, 281, 351, 352),
            "crosshair_pistol_chargeup_concussion_blast_overheat.muigraph": (328, 408, 325, 404, 405),
            "crosshair_pistol_chargeup_overheat.muigraph": (328, 408, 325, 404, 405),
            "crosshair_pistol_chargeup_phasepulse_overheat.muigraph": (328, 408, 325, 404, 405),
            "crosshair_pistol_ion_blast_overheat.muigraph": (306, 380, 303, 377, 378),
            "crosshair_pistol_ion_overheat.muigraph": (306, 380, 303, 377, 378),
        }
        self.assertEqual(set(expected), set(KAY_BLASTER_GRAPH_SHA256))
        for filename, expected_constants in expected.items():
            path = graphs / filename
            located = locate_kay_faint_center_ring_renderer(
                parse_layout(path), path
            )
            self.assertEqual(located["renderer_node_ids"], [407200])
            self.assertEqual(len(located["renderer_record_starts"]), 1)
            self.assertEqual(len(located["targets"]), 1)
            self.assertEqual(
                (
                    located["original_color_constant_index"],
                    located["transparent_constant_index"],
                    located["sector_count_constant_index"],
                    located["inner_radius_constant_index"],
                    located["outer_radius_constant_index"],
                ),
                expected_constants,
            )

    def test_no_crosshair_batch_changes_only_guarded_setter_sources(self):
        graphs = CORPUS_ROOT / "helix" / "ui" / "graphs"
        with tempfile.TemporaryDirectory() as temporary_directory:
            reports = patch_verified_weapon_batch_no_crosshair(
                graphs,
                Path(temporary_directory),
            )
            self.assertEqual(len(reports), 16)
            for report in reports:
                self.assertEqual(report.input_size, report.output_size)
                self.assertTrue(report.layout_counts_unchanged)
                self.assertTrue(report.descriptors_unchanged)
                self.assertGreaterEqual(len(report.changed_byte_offsets), 3)
                expected_integer_changes = (
                    len(report.hitmarker_target_integer_indices)
                    + 1
                    + len(report.residual_ring_target_integer_indices)
                )
                before = parse_layout(Path(report.input_path))
                after = parse_layout(Path(report.output_path))
                changed = [
                    index
                    for index, (left, right) in enumerate(
                        zip(before.integers, after.integers)
                    )
                    if left.value != right.value
                ]
                self.assertEqual(len(changed), expected_integer_changes)
                self.assertIn(report.crosshair_color_target_integer_index, changed)
                if Path(report.input_path).name in KAY_BLASTER_GRAPH_SHA256:
                    self.assertEqual(
                        len(report.residual_ring_target_integer_indices), 1
                    )
                    for target in report.residual_ring_target_integer_indices:
                        self.assertIn(target, changed)
                else:
                    self.assertEqual(report.residual_ring_target_integer_indices, [])

    def test_blaster_optimized_root_enabled_canary_is_uniquely_located(self):
        blaster = CORPUS_ROOT / "helix" / "ui" / "graphs" / "crosshair_blaster.muigraph"
        layout = parse_layout(blaster)
        record_start, target = locate_blaster_root_enabled(layout)
        self.assertEqual(record_start, 16898)
        self.assertEqual(target.index, 16918)
        self.assertEqual(target.start, 49619)
        self.assertEqual(target.value, 7)
        self.assertEqual(target.encoding_hex, "1c")

    def test_shared_component_has_one_exact_direct_chain(self):
        sidecar = probe_sidecar(
            CORPUS_ROOT
            / "helix"
            / "nodes"
            / "compounds"
            / "crosshair_damage_feedback.mcompoundnode.decompiled"
        )
        self.assertEqual(len(sidecar.direct_hit_feedback_chains), 1)
        chain = sidecar.direct_hit_feedback_chains[0]
        self.assertEqual(chain["user_option_constant_id"], 462866)
        self.assertEqual(chain["get_option_node_id"], 462867)
        self.assertEqual(chain["multi_select_node_id"], 462868)
        self.assertEqual(chain["reduced_constant_id"], 462869)
        self.assertEqual(chain["reduced_value"], "0.75")
        self.assertEqual(chain["normal_value"], "1")
        self.assertEqual(chain["large_value"], "1.5")

    def test_shared_component_binary_connection_run_is_unique(self):
        asset = (
            CORPUS_ROOT
            / "helix"
            / "nodes"
            / "compounds"
            / "crosshair_damage_feedback.mcompoundnode"
        )
        trace = trace_hit_feedback(asset)
        self.assertEqual(trace.sha256, SHARED_COMPONENT_SHA256)
        self.assertEqual(trace.hash_gate, "matched")
        self.assertEqual(trace.connection_run_start, 11947)
        self.assertEqual(trace.connection_run_end, 12006)
        self.assertEqual(trace.reduced_connection_source_offset, 11977)
        self.assertEqual(trace.encoded_node_ids["reduced"].offsets, [11977])
        self.assertEqual(trace.encoded_node_ids["normal"].offsets, [11987])
        self.assertEqual(trace.encoded_node_ids["large"].offsets, [11997])
        self.assertEqual(
            trace.decoded_connections,
            [
                {"source_id": 462866, "source_pin": 0, "target_id": 462867, "target_pin": 0},
                {"source_id": 462867, "source_pin": -1, "target_id": 462868, "target_pin": 1},
                {"source_id": 462868, "source_pin": 0, "target_id": 462865, "target_pin": 1},
                {"source_id": 462869, "source_pin": 0, "target_id": 462868, "target_pin": 2},
                {"source_id": 462870, "source_pin": 0, "target_id": 462868, "target_pin": 3},
                {"source_id": 462871, "source_pin": 0, "target_id": 462868, "target_pin": 4},
            ],
        )

    def test_shared_component_layout_is_fully_consumed(self):
        asset = (
            CORPUS_ROOT
            / "helix"
            / "nodes"
            / "compounds"
            / "crosshair_damage_feedback.mcompoundnode"
        )
        layout = parse_layout(asset)
        self.assertEqual(layout.version, 4)
        self.assertEqual(layout.string_count, 219)
        self.assertEqual(layout.value_count, 1833)
        self.assertEqual(layout.integer_count, 12843)
        self.assertEqual(layout.root_value_index, 7343)
        self.assertEqual(layout.strings_start, 277)
        self.assertEqual(layout.strings_end, 5101)
        self.assertEqual(layout.integers_start, 5101)
        self.assertEqual(layout.integers_end, 36279)
        self.assertEqual(layout.descriptors_start, 36279)
        self.assertEqual(layout.descriptors_end, asset.stat().st_size)
        self.assertEqual(layout.strings[81], "UI_HitFeedback")
        self.assertEqual(layout.strings[84], "native:MultiSelect2")

    def test_reduced_inline_float_token_is_uniquely_located(self):
        asset = (
            CORPUS_ROOT
            / "helix"
            / "nodes"
            / "compounds"
            / "crosshair_damage_feedback.mcompoundnode"
        )
        layout = parse_layout(asset)
        target = locate_reduced_value(layout)
        self.assertEqual(target.index, 2991)
        self.assertEqual(target.start, 12090)
        self.assertEqual(target.end, 12095)
        self.assertEqual(target.encoding_hex, "03 01 00 00 3d")
        self.assertEqual(decode_inline_float_token(target.value), 0.75)
        self.assertEqual(target.value, encode_inline_float_token(0.75))

    def test_size_preserving_zero_token_changes_only_target_value(self):
        asset = (
            CORPUS_ROOT
            / "helix"
            / "nodes"
            / "compounds"
            / "crosshair_damage_feedback.mcompoundnode"
        )
        data = asset.read_bytes()
        layout = parse_layout_bytes(data)
        target = locate_reduced_value(layout)
        replacement = b"\x03\x00\x00\x00\x00"
        patched = data[: target.start] + replacement + data[target.end :]
        verified = parse_layout_bytes(patched)
        changed = [
            index
            for index, (before, after) in enumerate(zip(data, patched))
            if before != after
        ]
        differing_values = [
            index
            for index, (before, after) in enumerate(
                zip(layout.integers, verified.integers)
            )
            if before.value != after.value
        ]
        self.assertEqual(len(patched), len(data))
        self.assertEqual(changed, [12091, 12094])
        self.assertEqual(differing_values, [2991])
        self.assertEqual(verified.integers[2991].value, 0)

    def test_size_preserving_three_x_token_changes_only_target_value(self):
        asset = (
            CORPUS_ROOT
            / "helix"
            / "nodes"
            / "compounds"
            / "crosshair_damage_feedback.mcompoundnode"
        )
        data = asset.read_bytes()
        layout = parse_layout_bytes(data)
        target = locate_reduced_value(layout)
        replacement = bytes.fromhex("03 01 00 00 41")
        patched = data[: target.start] + replacement + data[target.end :]
        verified = parse_layout_bytes(patched)
        changed = [
            index
            for index, (before, after) in enumerate(zip(data, patched))
            if before != after
        ]
        differing_values = [
            index
            for index, (before, after) in enumerate(
                zip(layout.integers, verified.integers)
            )
            if before.value != after.value
        ]
        self.assertEqual(len(patched), len(data))
        self.assertEqual(changed, [12094])
        self.assertEqual(differing_values, [2991])
        self.assertEqual(
            decode_inline_float_token(verified.integers[2991].value),
            3.0,
        )


if __name__ == "__main__":
    unittest.main()

