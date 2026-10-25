#!/usr/bin/env python3
"""v0.8 ANIMAL VOX sound checks. Not a listening or physical CPU benchmark."""
import math
import sys
import wave
from array import array
from pathlib import Path

out_dir = Path("build/host/out")

def load(label):
    path = out_dir / f"animalvox-{label}.wav"
    with wave.open(str(path), "rb") as wf:
        assert wf.getnchannels() == 2 and wf.getsampwidth() == 2
        sr = wf.getframerate()
        raw = wf.readframes(wf.getnframes())
    samples = array("h")
    samples.frombytes(raw)
    if sys.byteorder != "little":
        samples.byteswap()
    return sr, samples[::2]

def check(label):
    sr, data = load(label)
    assert len(data) >= sr, f"{label}: too short"
    peak = max(abs(x) for x in data)
    rms = math.sqrt(sum(int(x) ** 2 for x in data) / len(data))
    clip = sum(abs(x) >= 32760 for x in data) / len(data)
    print(f"{label}: peak={peak} RMS={rms:.1f} clipped={clip*100:.3f}%")
    if peak < 15 or rms < 3 or clip > .10:
        raise SystemExit(f"{label}: silent or severely clipped")
    return data

def difference(a, b):
    n = min(len(a), len(b))
    start, end = n // 4, n * 3 // 4
    numerator = sum(abs(int(a[i])-int(b[i])) for i in range(start,end))
    denominator = sum(abs(int(a[i]))+abs(int(b[i])) for i in range(start,end))
    return numerator / max(denominator,1)

sounds = {m:check(m) for m in ("cmos","pulse","whstl","wing","glott","cmos-u","cmos-dry")}
vowel = difference(sounds["cmos"], sounds["cmos-u"])
mix = difference(sounds["cmos"], sounds["cmos-dry"])
print(f"CMOS A/U vowel relative PCM difference={vowel:.4f}")
print(f"CMOS wet/dry relative PCM difference={mix:.4f}")
if vowel < .03 or mix < .03:
    raise SystemExit("ANIMAL VOX filter/vowel/wet mix may be bypassed")
for a,b in (("cmos","pulse"),("pulse","whstl"),("whstl","wing"),("wing","glott")):
    d = difference(sounds[a], sounds[b])
    print(f"ANIMAL VOX {a}/{b} relative PCM difference={d:.4f}")
    if d < .03:
        raise SystemExit(f"{a} and {b} nearly identical, possible source selection bug")
