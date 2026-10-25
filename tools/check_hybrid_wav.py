#!/usr/bin/env python3
"""Sanity tests for LOFI -> JIANT 4-resonator processing.

This checks non-silence, absence of catastrophic clipping and that moving
VOWEL changes PCM with the same LOFI source. It cannot validate timbre or
real-time CPU performance; that requires emulator and physical FM-1 tests.
"""
import math
import sys
import wave
from array import array
from pathlib import Path

OUT = Path("build/host/out")


def read(name):
    path = OUT / name
    with wave.open(str(path), "rb") as f:
        assert f.getnchannels() == 2 and f.getsampwidth() == 2, f"{path}: bad WAV"
        rate = f.getframerate()
        data = f.readframes(f.getnframes())
    samples = array("h")
    samples.frombytes(data)
    if sys.byteorder != "little":
        samples.byteswap()
    return rate, samples[::2]


def test_sound(name):
    rate, a = read(name)
    if len(a) < rate // 8:
        raise SystemExit(f"{name}: too short")
    peak = max(abs(v) for v in a)
    rms = math.sqrt(sum(int(v) ** 2 for v in a) / len(a))
    clipped = sum(1 for v in a if abs(v) >= 32760) / len(a)
    print(f"{name}: {len(a)} mono frames, peak={peak}, RMS={rms:.1f}, clipped={clipped*100:.3f}%")
    if peak < 16 or rms < 4:
        raise SystemExit(f"{name}: no audible output")
    if clipped > 0.10:
        raise SystemExit(f"{name}: severe clipping")
    return a


a = test_sound("hybrid-a.wav")
u = test_sound("hybrid-u.wav")
test_sound("hybrid-switch.wav")

# Compare the active section, not early silence or release tails. The
# emulator gives deterministic output for identical note/preset settings.
n = min(len(a), len(u))
start = n // 5
end = n * 4 // 5
difference = sum(abs(int(a[i]) - int(u[i])) for i in range(start, end))
reference = sum(abs(int(a[i])) + abs(int(u[i])) for i in range(start, end))
relative = difference / max(reference, 1)
print(f"A vs U vowel PCM relative difference: {relative:.4f}")
if relative < 0.02:
    raise SystemExit("VOWEL parameter doesn't change the filtered sound sufficiently")
