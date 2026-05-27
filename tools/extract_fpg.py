#!/usr/bin/env python3
"""Extract BennuGD/Fenix FPG image libraries to PNG files."""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from PIL import Image


MAGICS = {
    b"fpg\x1a\r\n\x00\x00": 8,
    b"f16\x1a\r\n\x00\x00": 16,
    b"f32\x1a\r\n\x00\x00": 32,
    b"f01\x1a\r\n\x00\x00": 1,
}


def c_string(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("cp850", errors="replace")


def safe_name(text: str, fallback: str) -> str:
    text = text.strip().replace("\\", "_").replace("/", "_")
    keep = []
    for ch in text:
        keep.append(ch if ch.isalnum() or ch in "._- " else "_")
    value = "".join(keep).strip(" .")
    return value or fallback


def rgb565_to_rgba(data: bytes) -> bytes:
    out = bytearray()
    for (px,) in struct.iter_unpack("<H", data):
        r = (px >> 11) & 0x1F
        g = (px >> 5) & 0x3F
        b = px & 0x1F
        out.extend(
            (
                (r << 3) | (r >> 2),
                (g << 2) | (g >> 4),
                (b << 3) | (b >> 2),
                255,
            )
        )
    return bytes(out)


def rgba8888(data: bytes) -> bytes:
    # Bennu stores 32-bit pixels as little-endian integers. Treat this as BGRA
    # on disk and output RGBA for Pillow.
    out = bytearray()
    for b, g, r, a in struct.iter_unpack("BBBB", data):
        out.extend((r, g, b, a))
    return bytes(out)


def extract_fpg(path: Path, out_dir: Path, limit: int | None = None) -> list[dict]:
    out_dir.mkdir(parents=True, exist_ok=True)
    blob = path.read_bytes()
    if len(blob) < 8:
        raise ValueError(f"{path} is too small to be an FPG")

    bpp = MAGICS.get(blob[:8])
    if not bpp:
        raise ValueError(f"{path} has unsupported FPG magic {blob[:8]!r}")

    pos = 8
    palette = None
    if bpp == 8:
        pal_bytes = blob[pos : pos + 768]
        pos += 768 + 576
        palette = [(v << 2) | (v >> 4) for v in pal_bytes]

    manifest: list[dict] = []
    index = 0
    while pos + 64 <= len(blob):
        code, regsize, name_raw, fpname_raw, width, height, flags = struct.unpack_from(
            "<ii32s12siii", blob, pos
        )
        pos += 64

        if width <= 0 or height <= 0:
            break

        control_points = []
        for _ in range(flags):
            if pos + 4 > len(blob):
                raise ValueError(f"{path}: truncated control point table")
            x, y = struct.unpack_from("<hh", blob, pos)
            pos += 4
            control_points.append([None if x == -1 else x, None if y == -1 else y])

        bytes_per_pixel = {1: 1, 8: 1, 16: 2, 32: 4}[bpp]
        size = width * height * bytes_per_pixel
        if pos + size > len(blob):
            raise ValueError(f"{path}: truncated image data at map {code}")

        pixels = blob[pos : pos + size]
        pos += size

        name = c_string(name_raw) or c_string(fpname_raw)
        filename = f"{index:04d}_{code:04d}_{safe_name(name, 'image')}.png"
        output = out_dir / filename

        if bpp == 8:
            image = Image.frombytes("P", (width, height), pixels)
            image.putpalette(palette)
            image.save(output)
        elif bpp == 16:
            image = Image.frombytes("RGBA", (width, height), rgb565_to_rgba(pixels))
            image.save(output)
        elif bpp == 32:
            image = Image.frombytes("RGBA", (width, height), rgba8888(pixels))
            image.save(output)
        else:
            image = Image.frombytes("1", (width, height), pixels)
            image.save(output)

        manifest.append(
            {
                "index": index,
                "code": code,
                "name": name,
                "fpname": c_string(fpname_raw),
                "width": width,
                "height": height,
                "bpp": bpp,
                "control_points": control_points,
                "png": str(output),
            }
        )
        index += 1
        if limit and index >= limit:
            break

    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("fpg", type=Path)
    parser.add_argument("out_dir", type=Path)
    parser.add_argument("--limit", type=int)
    args = parser.parse_args()

    manifest = extract_fpg(args.fpg, args.out_dir, args.limit)
    print(f"extracted {len(manifest)} images to {args.out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
