#!/usr/bin/env python3
"""Macht aus dem rohen Applikations-Binary ein fertiges Image (siehe common/include/image.h):
auf 4 Byte auffuellen, image_size im Header eintragen, CRC32 anhaengen.

  make_image.py <roh.bin> <image.bin>
"""
import struct
import sys
import zlib

HEADER_OFFSET = 0x200
HEADER_SIZE = 64
MAGIC = 0x31474D49


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    data = bytearray(open(sys.argv[1], "rb").read())
    if len(data) < HEADER_OFFSET + HEADER_SIZE:
        print("Fehler: Binary zu klein", file=sys.stderr)
        return 1
    magic, hver, size, load = struct.unpack_from("<IIII", data, HEADER_OFFSET)
    if magic != MAGIC:
        print("Fehler: Image-Header nicht an Offset 0x200 gefunden", file=sys.stderr)
        return 1
    while len(data) % 4:
        data.append(0xFF)
    total = len(data) + 4
    struct.pack_into("<I", data, HEADER_OFFSET + 8, total)
    data += struct.pack("<I", zlib.crc32(bytes(data)) & 0xFFFFFFFF)
    open(sys.argv[2], "wb").write(data)
    print(f"Image: {total} Byte, CRC32 {zlib.crc32(bytes(data[:-4])) & 0xFFFFFFFF:08X}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
