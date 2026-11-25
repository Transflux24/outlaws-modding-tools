from pathlib import Path
import struct
import sys
import unittest


TOOL_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOL_ROOT))

from snowdrop_bv2.binary import BV2FormatError, probe_bytes  # noqa: E402


class BinaryProbeTests(unittest.TestCase):
    def test_rejects_non_bv2_data(self):
        with self.assertRaises(BV2FormatError):
            probe_bytes(b"not a BV2 file")

    def test_finds_longest_nul_delimited_string_table(self):
        data = (
            b"\xffBV2"
            + b"\x01\x02junk\x00"
            + b"ConstantNodesData\x00Data\x00Type\x00float\x00UI_HitFeedback\x00"
            + b"\x01\x02\x03"
        )
        probe = probe_bytes(data)
        self.assertIsNotNone(probe.string_table)
        self.assertEqual(probe.string_table.count, 5)
        self.assertEqual(probe.string_table.first_strings[0], "ConstantNodesData")
        self.assertEqual(probe.marker_offsets["UI_HitFeedback"], [45])

    def test_reports_raw_float_offsets_without_interpreting_them(self):
        data = b"\xffBV2" + b"A\x00B\x00C\x00D\x00" + struct.pack("<f", 0.75)
        probe = probe_bytes(data)
        self.assertEqual(probe.raw_float32_offsets["0.75"], [12])
        self.assertEqual(probe.raw_float32_offsets["1.5"], [])


if __name__ == "__main__":
    unittest.main()

