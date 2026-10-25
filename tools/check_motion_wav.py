#!/usr/bin/env python3
"""CHIP VOX v0.6 audio sanity: NOIS carrier + vowel sweep/CC1 tests."""
import math
import sys
import wave
from array import array
from pathlib import Path

OUT = Path("build/host/out")

def load(name):
    with wave.open(str(OUT / name), "rb") as f:
        assert f.getnchannels() == 2 and f.getsampwidth() == 2
        rate = f.getframerate()
        raw = f.readframes(f.getnframes())
    samples = array("h")
    samples.frombytes(raw)
    if sys.byteorder != "little":
        samples.byteswap()
    return rate, samples[::2]

def validate(name):
    rate, data = load(name)
    assert len(data) >= rate, f"{name}: less than a second of audio"
    peak = max(abs(v) for v in data)
    rms = math.sqrt(sum(int(v) * int(v) for v in data) / len(data))
    clip = sum(abs(v) >= 32760 for v in data) / len(data)
    print(f"{name}: peak={peak}, RMS={rms:.1f}, clipped={clip*100:.3f}%")
    if rms < 3 or peak < 16 or clip > .10:
        raise SystemExit(f"{name}: silence or severe clipping")
    return data

def relative(a, b):
    # Ignore startup and release; same MIDI stream, differing only in vowel motion.
    length = min(len(a), len(b))
    start, stop = length // 5, length * 4 // 5
    difference = sum(abs(int(a[i]) - int(b[i])) for i in range(start, stop))
    magnitude = sum(abs(int(a[i])) + abs(int(b[i])) for i in range(start, stop))
    return difference / max(1, magnitude)

fixed_a = validate("motion-a.wav")
fixed_u = validate("motion-u.wav")
moving = validate("motion-auto.wav")
validate("motion-live.wav")
vowels = relative(fixed_a, fixed_u)
sweep = relative(fixed_a, moving)
print(f"CC1 manual A/U relative difference: {vowels:.4f}")
print(f"Auto-vowel motion versus fixed A relative difference: {sweep:.4f}")
if vowels < .025:
    raise SystemExit("CC1 manual morph did not change noise/formant audio")
if sweep < .025:
    raise SystemExit("SPEED automation did not change noise/formant audio")
