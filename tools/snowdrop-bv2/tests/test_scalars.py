from pathlib import Path
import sys
import unittest


TOOL_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOL_ROOT))

from snowdrop_bv2.scalars import (  # noqa: E402
    ScalarDecodeError,
    decode_low_tag_varint32,
    decode_varint32,
    decode_varuint32,
    encode_low_tag_varint32,
    encode_varint32,
    encode_varuint32,
)


class UnsignedScalarTests(unittest.TestCase):
    def test_round_trips_every_encoding_boundary(self):
        values = [
            0,
            1,
            0x3F,
            0x40,
            0x3FFF,
            0x4000,
            0x3FFFFFFF,
            0x40000000,
            0xFFFFFFFF,
        ]
        expected_sizes = [1, 1, 1, 2, 2, 4, 4, 5, 5]
        for value, expected_size in zip(values, expected_sizes):
            with self.subTest(value=value):
                encoded = encode_varuint32(value)
                self.assertEqual(len(encoded), expected_size)
                decoded, end = decode_varuint32(encoded)
                self.assertEqual(decoded, value)
                self.assertEqual(end, len(encoded))

    def test_first_outlaws_value_is_four(self):
        self.assertEqual(decode_varuint32(bytes.fromhex("10")), (4, 1))


class SignedScalarTests(unittest.TestCase):
    def test_round_trips_every_encoding_boundary(self):
        values = [
            -0x80000000,
            -0x20000001,
            -0x20000000,
            -0x2001,
            -0x2000,
            -0x21,
            -0x20,
            -1,
            0,
            0x1F,
            0x20,
            0x1FFF,
            0x2000,
            0x1FFFFFFF,
            0x20000000,
            0x7FFFFFFF,
        ]
        for value in values:
            with self.subTest(value=value):
                encoded = encode_varint32(value)
                decoded, end = decode_varint32(encoded)
                self.assertEqual(decoded, value)
                self.assertEqual(end, len(encoded))

    def test_truncated_values_fail_closed(self):
        with self.assertRaises(ScalarDecodeError):
            decode_varuint32(b"\x03\x00")
        with self.assertRaises(ScalarDecodeError):
            decode_varint32(b"\xC0\x00")


class LowTagSignedScalarTests(unittest.TestCase):
    def test_round_trips_every_encoding_boundary(self):
        values = (
            -0x80000000,
            -0x20000001,
            -0x20000000,
            -0x2001,
            -0x2000,
            -0x21,
            -0x20,
            -1,
            0,
            0x1F,
            0x20,
            0x1FFF,
            0x2000,
            0x1FFFFFFF,
            0x20000000,
            0x7FFFFFFF,
        )
        for value in values:
            with self.subTest(value=value):
                encoded = encode_low_tag_varint32(value)
                decoded, end = decode_low_tag_varint32(encoded)
                self.assertEqual(decoded, value)
                self.assertEqual(end, len(encoded))

    def test_known_connection_pin_encodings(self):
        self.assertEqual(encode_low_tag_varint32(-1), b"\xfc")
        self.assertEqual(encode_low_tag_varint32(2), b"\x08")
        self.assertEqual(decode_low_tag_varint32(b"\xfc"), (-1, 1))
        self.assertEqual(decode_low_tag_varint32(b"\x10"), (4, 1))


if __name__ == "__main__":
    unittest.main()

