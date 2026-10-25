#!/usr/bin/env python3
"""Basic sanity check for imported VOICE synth audio, not a listening test."""
import math
import sys
import wave
from array import array
from pathlib import Path

path = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/out/voice.wav")
with wave.open(str(path), "rb") as f:
    assert f.getnchannels() == 2 and f.getsampwidth() == 2, "expected 16-bit stereo"
    raw = f.readframes(f.getnframes())
samples = array("h")
samples.frombytes(raw)
if sys.byteorder != "little":
    samples.byteswap()
if len(samples) < 8000:
    raise SystemExit(f"Voice WAV too short: {len(samples)} samples")
peak = max(abs(v) for v in samples)
rms = math.sqrt(sum(int(v) * int(v) for v in samples) / len(samples))
clipping = sum(1 for v in samples if abs(v) >= 32760) / len(samples)
print(f"VOICE sanity: {path}, peak={peak}, rms={rms:.1f}, clipped={100*clipping:.3f}%")
if rms < 3 or peak < 16:
    raise SystemExit("VOICE appears silent or inaudibly low")
if clipping > 0.10:
    raise SystemExit("VOICE has severe digital clipping")
