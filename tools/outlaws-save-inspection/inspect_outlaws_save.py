from __future__ import annotations

import ctypes
import pathlib
import sys


ZSTD_CONTENTSIZE_UNKNOWN = (1 << 64) - 1
ZSTD_CONTENTSIZE_ERROR = (1 << 64) - 2


def decompress_save(source: pathlib.Path, destination: pathlib.Path, dll_path: pathlib.Path) -> None:
    data = source.read_bytes()
    frame_magic = b"\x28\xb5\x2f\xfd"
    frame_offsets = [index for index in range(len(data)) if data.startswith(frame_magic, index)]
    if not frame_offsets:
        raise ValueError(f"No Zstandard frame found in {source.name}")
    zstd = ctypes.CDLL(str(dll_path))
    zstd.ZSTD_getFrameContentSize.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    zstd.ZSTD_getFrameContentSize.restype = ctypes.c_ulonglong
    zstd.ZSTD_findFrameCompressedSize.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    zstd.ZSTD_findFrameCompressedSize.restype = ctypes.c_size_t
    zstd.ZSTD_decompress.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
    zstd.ZSTD_decompress.restype = ctypes.c_size_t
    zstd.ZSTD_isError.argtypes = [ctypes.c_size_t]
    zstd.ZSTD_isError.restype = ctypes.c_uint
    zstd.ZSTD_getErrorName.argtypes = [ctypes.c_size_t]
    zstd.ZSTD_getErrorName.restype = ctypes.c_char_p

    decompressed_parts = []
    frame_details = []
    for frame_offset in frame_offsets:
        payload = data[frame_offset:]
        source_buffer = ctypes.create_string_buffer(payload)
        frame_size = zstd.ZSTD_findFrameCompressedSize(source_buffer, len(payload))
        if zstd.ZSTD_isError(frame_size):
            message = zstd.ZSTD_getErrorName(frame_size).decode("utf-8", errors="replace")
            raise ValueError(f"Could not size {source.name}: {message}")
        output_size = zstd.ZSTD_getFrameContentSize(source_buffer, len(payload))
        if output_size in (ZSTD_CONTENTSIZE_UNKNOWN, ZSTD_CONTENTSIZE_ERROR):
            raise ValueError(f"Zstandard frame does not declare a usable size in {source.name}")

        output_buffer = ctypes.create_string_buffer(output_size)
        result = zstd.ZSTD_decompress(output_buffer, output_size, source_buffer, frame_size)
        if zstd.ZSTD_isError(result):
            message = zstd.ZSTD_getErrorName(result).decode("utf-8", errors="replace")
            raise ValueError(f"Could not decompress {source.name}: {message}")
        decompressed_parts.append(output_buffer.raw[:result])
        frame_details.append(f"{frame_offset}:{frame_size}->{result}")

    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(b"".join(decompressed_parts))
    print(f"{source.name}\tframes={','.join(frame_details)}")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        raise SystemExit("usage: inspect_outlaws_save.py DLL OUTPUT_DIR SAVE [SAVE ...]")
    dll = pathlib.Path(sys.argv[1])
    output_dir = pathlib.Path(sys.argv[2])
    for save_path in map(pathlib.Path, sys.argv[3:]):
        decompress_save(save_path, output_dir / f"{save_path.name}.raw", dll)
