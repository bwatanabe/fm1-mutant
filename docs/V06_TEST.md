# Mutant v0.6-dev — animated CHIP VOX

**Status: experimental branch `feature/chip-vox-motion`.** The prior v0.5 CHIP VOX engine was tested by the user in both browser emulator and actual FM-1 with OP-XY USB MIDI, especially using WAVE 3 (NOIS). That tested v0.5 has been merged to `main`.

v0.6 adds **movement between two vowels** to the same CHIP VOX (model 27), retaining all 27 engine models. It does not replace Felucca's LOFI oscillator or JIANT's four-formant filters. The audio source and DSP stay as in v0.5; the new work is **control-rate modulation of the vowel position**.

## Operation

Select **CHIP VOX / D3** by sending MIDI CC19=127, or while on HOME, tap HOME three times from a Plaits model. From another page, first press HOME to return to HOME.

The four HOME knobs and three SCL knobs are unchanged:

| Screen | Controls |
| --- | --- |
| HOME | K1 CHIP, K2 WAVE (0=PLS,1=TRI,2=SAW,**3=NOIS**,4=WRAM), K3 VOWEL (start), K4 transpose |
| SCL / ATTN | K1 SHIFT, K2 Q, K3 CRSH |
| FX / LPG | K1 WET %, K2 TONE, **K3 VOWL2**, **K4 SPEED** |

VOWL2 selects the vowel destination. SPEED=0 disables automatic movement; SPEED=1..100 automatically ping-pongs between VOWEL and VOWL2. The approximate full A→B→A rates are 0.04–4 Hz, depending on SPEED. The phase restarts on each note-on.

**Manual MIDI vowel motion:** with SPEED=0, MIDI CC1 (mod wheel) smoothly interpolates between the two positions. At CC1=0 you hear VOWEL; at CC1=127 you hear VOWL2. CC1 still retains its GATOR behavior on Plaits models. With SPEED>0 the automatic ping-pong takes precedence over CC1.

**New MIDI assignments (only while on CHIP VOX):** CC25 sets VOWL2, CC26 sets SPEED. MIDI CC16–18 and CC20–24 remain as in v0.5. This keeps the existing OP-XY MIDI mappings intact.

**Suggested patch:** CHIP 0 / WAVE 3 (NOIS), VOWEL A (HOME K3 at 0), VOWL2 U (FX K3 at 100), SPEED ~30 (FX K4), WET 100% (FX K1), TONE at taste (FX K2). Turn SPEED to 0 and send CC1 from your controller for direct manual vocal movement.

For backward compatibility, previously saved v0.5 CHIP VOX patches map their old unused `blend` and `spread` fields to VOWL2 and SPEED. Existing saved patches typically have spread=0, so they'll retain static vowels unless you explicitly enable movement. New CHIP VOX defaults are A→U with SPEED=0.

## Implementation details

* Vowel interpolation and resonator DSP remain JIANT-derived code, called by `hy_coefficients()`.
* A Q32 phase accumulator runs once per 32-sample block and produces a simple triangle control signal. No audio oscillator is reimplemented.
* JIANT's resonator coefficients are recalculated once per 32-sample block so motion affects sustained notes. No per-sample coefficient recalculation.
* FM-1 processor use may be higher than static CHIP VOX. The browser emulator and host WAV test **do not** validate physical CPU cost.
* The old stable preset structure, model indexes, oscillator algorithms, and Web MIDI updater remain unchanged.

## Regression tests

```sh
./build.sh --app gator --name mutant
python3 tools/sim.py host/motion-a.sim
python3 tools/sim.py host/motion-u.sim
python3 tools/sim.py host/motion-auto.sim
python3 tools/sim.py host/motion-live.sim
python3 tools/check_motion_wav.py
```

`motion-a.wav` and `motion-u.wav` should differ despite identical WAVE 3 excitation, proving CC1 manual sweep changes the formant filtering. `motion-auto.wav` should differ from static A when SPEED is enabled, confirming automatic movement. Tests reject silence and severe digital clipping.

## Testing experimental firmware

Download the **mutant-v06-firmware-test** artifact from the passing `mutant-v06-test` GitHub Actions workflow, extract `mutant.fwsc`, and use the [full FM-1 web emulator](https://joakimeriksson.github.io/fm1-emu/) → Open .fwsc….

The public [Mutant updater](https://bwatanabe.github.io/fm1-mutant/) continues to publish accepted v0.5 from `main`. Do not use *Install published build* for v0.6. After the emulator test, use *Install selected file* and choose the extracted v0.6 `.fwsc`, then test physical FM-1 CPU/audio under repeated noise notes and rapid vowel sweeps.

Do not merge v0.6 to `main` until hardware testing passes.

## Next research milestone

Explore JIANT's ANIMAL/CMOS sound engine and potentially animal/creature excitations through the same formant filter. Preserve source attribution and prioritize real-time sound controls over any sequencer/workstation features.
