#!/usr/bin/env python3
"""Turn generated RGBA PNG sheets into small BMP files SDL can load on Switch."""

from __future__ import annotations

import argparse
from collections import deque
import struct
import zlib
from pathlib import Path


def read_png(path: Path) -> tuple[int, int, int, bytes]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"Not a PNG file: {path}")

    width = height = bit_depth = color_type = interlace = None
    compressed = bytearray()
    offset = 8
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if compression != 0 or filtering != 0:
                raise ValueError(f"Unsupported PNG encoding: {path}")
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break

    if width is None or height is None or bit_depth != 8 or color_type not in (2, 6):
        raise ValueError(f"Expected 8-bit RGB/RGBA PNG: {path}")
    if interlace != 0:
        raise ValueError(f"Interlaced PNG is not supported: {path}")

    channels = 4 if color_type == 6 else 3
    stride = width * channels
    raw = zlib.decompress(compressed)
    if len(raw) != height * (stride + 1):
        raise ValueError(f"Unexpected PNG data size: {path}")

    pixels = bytearray(height * stride)
    source = 0
    for y in range(height):
        filter_type = raw[source]
        source += 1
        row_start = y * stride
        for x in range(stride):
            value = raw[source + x]
            left = pixels[row_start + x - channels] if x >= channels else 0
            up = pixels[row_start - stride + x] if y else 0
            upper_left = pixels[row_start - stride + x - channels] if y and x >= channels else 0
            if filter_type == 1:
                value = (value + left) & 255
            elif filter_type == 2:
                value = (value + up) & 255
            elif filter_type == 3:
                value = (value + ((left + up) // 2)) & 255
            elif filter_type == 4:
                p = left + up - upper_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - upper_left)
                predictor = left if pa <= pb and pa <= pc else up if pb <= pc else upper_left
                value = (value + predictor) & 255
            elif filter_type != 0:
                raise ValueError(f"Unknown PNG filter {filter_type}: {path}")
            pixels[row_start + x] = value
        source += stride

    return width, height, channels, bytes(pixels)


def pixel_rgba(pixels: bytes, channels: int, width: int, x: int, y: int) -> tuple[int, int, int, int]:
    i = (y * width + x) * channels
    if channels == 4:
        return pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]
    return pixels[i], pixels[i + 1], pixels[i + 2], 255


def remove_jump_shadow(
    pixels: bytes, channels: int, width: int, cell: tuple[int, int, int, int]
) -> bytes:
    """Clear detached, flat ground-shadow components below the jump pose."""
    if channels != 4:
        return pixels
    x0, y0, x1, y1 = cell
    cell_w, cell_h = x1 - x0, y1 - y0
    visited = bytearray(cell_w * cell_h)
    removed: list[list[tuple[int, int]]] = []
    for local_y in range(cell_h):
        for local_x in range(cell_w):
            local_i = local_y * cell_w + local_x
            alpha_i = ((y0 + local_y) * width + x0 + local_x) * channels + 3
            if visited[local_i] or pixels[alpha_i] < 48:
                continue
            visited[local_i] = 1
            queue = deque([(local_x, local_y)])
            component: list[tuple[int, int]] = []
            count = 0
            min_x = max_x = local_x
            min_y = max_y = local_y
            while queue:
                px, py = queue.popleft()
                component.append((px, py))
                count += 1
                min_x, max_x = min(min_x, px), max(max_x, px)
                min_y, max_y = min(min_y, py), max(max_y, py)
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        nx, ny = px + dx, py + dy
                        if not (0 <= nx < cell_w and 0 <= ny < cell_h):
                            continue
                        ni = ny * cell_w + nx
                        ai = ((y0 + ny) * width + x0 + nx) * channels + 3
                        if not visited[ni] and pixels[ai] >= 48:
                            visited[ni] = 1
                            queue.append((nx, ny))
            component_w, component_h = max_x - min_x + 1, max_y - min_y + 1
            low_in_frame = max_y >= cell_h * 0.68
            flat_and_wide = component_w >= cell_w * 0.18 and component_h <= cell_h * 0.10
            if low_in_frame and flat_and_wide and count < cell_w * cell_h * 0.16:
                removed.append(component)

    if not removed:
        return pixels

    result = bytearray(pixels)
    for component in removed:
        for px, py in component:
            i = ((y0 + py) * width + x0 + px) * channels + 3
            result[i] = 0
    return bytes(result)


def write_bmp(
    path: Path,
    width: int,
    height: int,
    channels: int,
    pixels: bytes,
    crop: tuple[int, int, int, int] | None = None,
    target_size: tuple[int, int] | None = None,
) -> None:
    x0, y0, x1, y1 = crop or (0, 0, width, height)
    source_width, source_height = x1 - x0, y1 - y0
    out_width, out_height = target_size or (source_width, source_height)
    row_bytes = ((out_width * 3 + 3) // 4) * 4
    image_size = row_bytes * out_height
    header = struct.pack("<2sIHHI", b"BM", 54 + image_size, 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, out_width, out_height, 1, 24, 0, image_size, 2835, 2835, 0, 0)
    rows = bytearray()
    for out_y in range(out_height - 1, -1, -1):
        source_y = y0 + min(source_height - 1, out_y * source_height // out_height)
        row = bytearray()
        for out_x in range(out_width):
            source_x = x0 + min(source_width - 1, out_x * source_width // out_width)
            r, g, b, a = pixel_rgba(pixels, channels, width, source_x, source_y)
            if channels == 4 and a < 128:
                r, g, b = 255, 0, 255  # SDL color key, no SDL_image dependency.
            row.extend((b, g, r))
        row.extend(b"\0" * (row_bytes - len(row)))
        rows.extend(row)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + dib + rows)


def split_sheet(source: Path, output: Path, prefix: str, rows: int, columns: int, names: list[str]) -> None:
    width, height, channels, pixels = read_png(source)
    for row in range(rows):
        row_top, row_bottom = row * height // rows, (row + 1) * height // rows
        min_cell_h = row_bottom - row_top
        bounds: list[tuple[int, int, int, int]] = []
        min_cell_w = width
        for column in range(columns):
            left, right = column * width // columns, (column + 1) * width // columns
            min_cell_w = min(min_cell_w, right - left)
            top, bottom = row_top, row_bottom
            points = [
                (x, y)
                for y in range(top, bottom)
                for x in range(left, right)
                if pixel_rgba(pixels, channels, width, x, y)[3] >= 48
            ]
            name = names[row * columns + column]
            if not points:
                raise ValueError(f"Empty animation frame {name} in {source}")
            bounds.append((
                min(x for x, _ in points) - left,
                min(y for _, y in points) - top,
                max(x for x, _ in points) - left + 1,
                max(y for _, y in points) - top + 1,
            ))

        # Every frame in an animation row gets the same crop. This prevents
        # enemies from visibly growing and shrinking when their pose changes.
        x0 = max(0, min(bound[0] for bound in bounds) - 2)
        y0 = max(0, min(bound[1] for bound in bounds) - 2)
        x1 = min(min_cell_w, max(bound[2] for bound in bounds) + 2)
        y1 = min(min_cell_h, max(bound[3] for bound in bounds) + 2)
        for column in range(columns):
            left = column * width // columns
            name = names[row * columns + column]
            frame_pixels = pixels
            if prefix == "tamandua" and name == "jump":
                frame_pixels = remove_jump_shadow(
                    pixels, channels, width,
                    (left, row_top, (column + 1) * width // columns, row_bottom),
                )
            write_bmp(output / f"{prefix}_{name}.bmp", width, height, channels, frame_pixels,
                      (left + x0, row_top + y0, left + x1, row_top + y1))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    split_sheet(args.source / "tamandua-sheet.png", args.output, "tamandua", 2, 4,
                ["idle", "run1", "run2", "jump", "crouch", "dig", "ball", "roll"])
    split_sheet(args.source / "tamandua-dig-sheet.png", args.output, "tamandua_dig", 1, 4,
                ["1", "2", "3", "4"])
    split_sheet(args.source / "monstros-sheet.png", args.output, "bruto", 2, 4,
                ["idle", "walk", "attack", "hit", "small_idle", "small_walk", "small_jump", "small_ball"])
    # Rename the second creature family so the C++ runtime has a simple, explicit asset set.
    for pose in ("idle", "walk", "jump", "ball"):
        generated = args.output / f"bruto_small_{pose}.bmp"
        generated.replace(args.output / f"grilo_{pose}.bmp")
    split_sheet(args.source / "formigas-sheet.png", args.output, "formiga", 1, 4,
                ["idle", "walk1", "walk2", "bite"])
    for source_name, output_name in (
        ("mata-pocos-de-caldas.png", "mata.bmp"),
        ("cidade-pocos.png", "cidade.bmp"),
        ("escritorio-programacao.png", "escritorio.bmp"),
    ):
        width, height, channels, pixels = read_png(args.source / source_name)
        write_bmp(args.output / output_name, width, height, channels, pixels, target_size=(1280, 720))
    print(f"Assets preparados em {args.output}")


if __name__ == "__main__":
    main()
