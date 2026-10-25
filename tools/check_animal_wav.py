#!/usr/bin/env python3
"""ANIMAL v0.7 five-mode audio sanity; not a real-time CPU/subjective quality test."""
from array import array
import math
import sys
import wave
from pathlib import Path

out = Path("build/host/out")
sounds = {}
for mode in ("cmos", "pulse", "whstl", "wing", "glott"):
    with wave.open(str(out / f"animal-{mode}.wav"), "rb") as f:
        assert f.getnchannels() == 2 and f.getsampwidth() == 2, f"{mode}: unexpected format"
        sample_rate, frames = f.getframerate(), f.getnframes()
        data = f.readframes(frames)
    a = array("h")
    a.frombytes(data)
    if sys.byteorder != "little":
        a.byteswap()
    a = a[::2]
    assert len(a) >= sample_rate, f"{mode}: WAV too short"
    peak = max(abs(v) for v in a)
    rms = math.sqrt(sum(int(x) * int(x) for x in a) / len(a))
    clip = sum(abs(x) >= 32760 for x in a) / len(a)
    print(f"ANIMAL {mode}: peak={peak} RMS={rms:.1f} clipped={clip*100:.3f}%")
    if rms < 3 or peak < 16 or clip > .10:
        raise SystemExit(f"{mode}: silence or excessive clipping")
    sounds[mode] = a
# Broad comparison only: species modes shouldn't all produce the same PCM.
for left, right in (("cmos", "pulse"), ("pulse", "whstl"), ("whstl", "wing"), ("wing", "glott")):
    a, b = sounds[left], sounds[right]
    begin = min(len(a), len(b)) // 4
    end = min(len(a), len(b)) * 3 // 4
    difference = sum(abs(int(a[i])-int(b[i])) for i in range(begin, end))
    amplitude = sum(abs(int(a[i]))+abs(int(b[i])) for i in range(begin, end))
    d = difference/max(1, amplitude)
    print(f"{left}/{right} relative PCM difference={d:.3f}")
    if d < .03:
        raise SystemExit(f"{left} and {right} render nearly identical audio")
