#!/usr/bin/env python3
"""Create a short, soft punch impact for the game's hit feedback."""

from __future__ import annotations

import math
import random
import struct
import sys
import wave
from pathlib import Path


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Uso: python3 tools/compose_sfx.py <saida.wav>")
    output = Path(sys.argv[1])
    sample_rate = 22050
    duration = 0.24
    rng = random.Random(74)
    frames = bytearray()
    for index in range(int(sample_rate * duration)):
        time = index / sample_rate
        body = math.sin(2.0 * math.pi * (88.0 - 22.0 * time) * time) * math.exp(-time * 20.0) * 0.46
        snap = rng.uniform(-1.0, 1.0) * math.exp(-time * 55.0) * 0.22
        click = math.sin(2.0 * math.pi * 1450.0 * time) * math.exp(-time * 85.0) * 0.13
        sample = max(-0.82, min(0.82, body + snap + click))
        frames.extend(struct.pack("<h", int(sample * 32767)))
    output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(output), "wb") as stream:
        stream.setnchannels(1)
        stream.setsampwidth(2)
        stream.setframerate(sample_rate)
        stream.writeframes(frames)
    print(f"Efeito de impacto criado: {output}")


if __name__ == "__main__":
    main()
