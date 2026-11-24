#!/usr/bin/env python3
"""Compose a varied, loopable guitar, bass and drum rock soundtrack."""

from __future__ import annotations

import math
import random
import struct
import sys
import wave
from array import array
from pathlib import Path

RATE = 22050
BPM = 108
BARS = 32
BEAT = 60.0 / BPM
BAR = BEAT * 4
TOTAL = int(round(BARS * BAR * RATE))
TRACK = array("f", [0.0]) * TOTAL
RNG = random.Random(2606)


def midi(note: float) -> float:
    return 440.0 * (2.0 ** ((note - 69.0) / 12.0))


def add_tone(start: float, duration: float, notes: tuple[float, ...], volume: float, kind: str) -> None:
    begin = max(0, int(start * RATE))
    end = min(TOTAL, int((start + duration) * RATE))
    if end <= begin:
        return
    length = end - begin
    frequencies = tuple(midi(note) for note in notes)
    attack = 0.012
    decay = {"guitar": 5.8, "bass": 4.1}[kind]
    phase_offsets = tuple(RNG.random() * math.tau for _ in frequencies)
    for i in range(length):
        t = i / RATE
        envelope = min(1.0, t / attack) * math.exp(-t * decay)
        sample = 0.0
        for freq, offset in zip(frequencies, phase_offsets):
            phase = math.tau * freq * t + offset
            if kind == "guitar":
                sample += math.sin(phase) + 0.38 * math.sin(phase * 2.0) + 0.15 * math.sin(phase * 3.0)
            else:
                sample += math.sin(phase) + 0.16 * math.sin(phase * 2.0)
        sample /= max(1, len(frequencies))
        if kind == "guitar":
            sample = math.tanh(sample * 2.3)
        TRACK[begin + i] += sample * envelope * volume


def add_kick(start: float, volume: float) -> None:
    begin = max(0, int(start * RATE))
    length = min(int(0.19 * RATE), TOTAL - begin)
    phase = 0.0
    for i in range(length):
        t = i / RATE
        freq = 46.0 + 88.0 * math.exp(-t * 25.0)
        phase += math.tau * freq / RATE
        TRACK[begin + i] += math.sin(phase) * math.exp(-t * 20.0) * volume


def add_snare(start: float, volume: float) -> None:
    begin = max(0, int(start * RATE))
    length = min(int(0.15 * RATE), TOTAL - begin)
    previous = 0.0
    for i in range(length):
        t = i / RATE
        noise = RNG.uniform(-1.0, 1.0)
        high = noise - previous * 0.82
        previous = noise
        body = math.sin(math.tau * (185.0 - 35.0 * t) * t)
        TRACK[begin + i] += (high * 0.72 + body * 0.28) * math.exp(-t * 24.0) * volume


def add_hat(start: float, volume: float, length_seconds: float = 0.055) -> None:
    begin = max(0, int(start * RATE))
    length = min(int(length_seconds * RATE), TOTAL - begin)
    previous = 0.0
    for i in range(length):
        t = i / RATE
        noise = RNG.uniform(-1.0, 1.0)
        high = noise - previous * 0.92
        previous = noise
        TRACK[begin + i] += high * math.exp(-t * 58.0) * volume


progressions = [
    [40, 36, 43, 38],  # Em - C - G - D
    [40, 43, 36, 38],  # variation
    [33, 36, 40, 38],  # Am - C - Em - D
    [43, 38, 36, 40],  # G - D - C - Em
]
for bar in range(BARS):
    section = bar // 8
    chord = progressions[section // 1][bar % 4]
    start = bar * BAR
    variation = bar % 8

    # Palm-muted power chords move between accents instead of repeating one riff.
    patterns = (
        (0.0, 0.75, 1.5, 2.5, 3.0),
        (0.0, 1.0, 1.75, 2.0, 3.25),
        (0.0, 0.5, 1.5, 2.75, 3.0),
        (0.0, 1.25, 2.0, 2.5, 3.5),
    )
    for beat_index in patterns[(bar + section) % len(patterns)]:
        accent = 1.22 if beat_index in (0.0, 2.0) else 0.82
        add_tone(start + beat_index * BEAT, BEAT * (0.30 if beat_index % 1 else 0.38),
                 (chord, chord + 7, chord + 12), 0.24 * accent, "guitar")

    # A separate bass line walks around each chord, with small passing notes.
    bass_steps = (0, 0, 7, 12) if bar % 4 != 3 else (0, 7, 10, 12)
    for beat_index, step in enumerate(bass_steps):
        add_tone(start + beat_index * BEAT, BEAT * 0.72, (chord - 12 + step,), 0.31, "bass")

    # Four-on-the-floor drive, backbeat snare, and eighth-note hats with fills.
    for beat_index in range(4):
        kick = beat_index in (0, 2) or (bar % 4 == 3 and beat_index == 3)
        if kick:
            add_kick(start + beat_index * BEAT, 0.42 if beat_index == 0 else 0.31)
        if beat_index in (1, 3):
            add_snare(start + beat_index * BEAT, 0.22)
    for eighth in range(8):
        if (bar + eighth) % 11 != 0:
            strength = 0.075 if eighth % 2 else 0.12
            add_hat(start + eighth * BEAT / 2.0, strength, 0.045 if eighth % 2 else 0.065)
    if bar % 8 == 7:
        for fill in range(4):
            add_snare(start + 3.0 * BEAT + fill * BEAT / 4.0, 0.09)


# Fade only a few milliseconds at the loop boundary, then softly limit the mix.
peak = max(abs(value) for value in TRACK) or 1.0
gain = 0.72 / peak
fade = int(0.012 * RATE)
pcm = array("h")
for i, value in enumerate(TRACK):
    edge = min(1.0, (i + 1) / fade, (TOTAL - i) / fade)
    limited = math.tanh(value * gain * 1.15) / math.tanh(1.15)
    pcm.append(int(max(-1.0, min(1.0, limited * edge)) * 32767))

output = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("rock-guitarra.wav")
output.parent.mkdir(parents=True, exist_ok=True)
with wave.open(str(output), "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(RATE)
    wav.writeframes(pcm.tobytes())
print(f"Trilha rock de guitarra, baixo e bateria criada: {output} ({BARS} compassos, {TOTAL / RATE:.1f} s)")
