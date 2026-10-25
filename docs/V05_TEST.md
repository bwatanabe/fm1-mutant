# FM-1 Mutant v0.5-dev — CHIP VOX hybrid

**Experimental, not yet tested by the user on FM-1 hardware. Do not merge into `main` until emulator and real hardware checks pass.**

## What is new?

**CHIP VOX (model 27 / D3)** is the first cross-engine audio design in Mutant. It runs Felucca LOFI's actual oscillator, with its selected chip / wave / tone / crushing, and processes the resulting PCM with JIANT's existing four-resonator Klatt-style vocal filter (F1-F4). It is **not** JIANT's glottal sound generator and does not implement speech, letters or numbers. It is a playable digital/chip oscillator with adjustable vocal-tract coloration.

The 24 Plaits models, standalone LOFI (#25), and JIANT VOICE (#26) remain. Patch format and GATOR's sixteen storage slots are unchanged. **Each engine family has its own temporary knob-state cache** during switching.

## Access in the FM-1 emulator

On the HOME page, tap HOME repeatedly to cycle:
`Plaits → LOFI → VOICE → CHIP VOX → last Plaits`.

This works with one button at a time and is easier than HOME + a key in the web emulator. From another page, the first HOME press navigates to HOME. The physical FM-1 also supports HOME + 27th key; SELECT adjusts models directly.

With 27 models, MIDI CC19 now maps as follows:
- **127**: CHIP VOX (model 27).
- **121**: JIANT VOICE (model 26).
- **115**: LOFI CHIP (model 25).
- **0**: Plaits VA VCF (model 1).

Other CC19 values select the remaining models based on GATOR's `floor(CC19*27/128)` mapping.

## Controls

| Screen | Knobs |
| --- | --- |
| HOME | K1 CHIP (five synth styles); K2 WAVE (pulse / triangle / saw / noise / wave RAM); K3 **VOWEL** (A–E–I–O–U) |
| HOME | K4 transpose |
| SCL / ATTN | K1 **SHIFT** (-12 to +12 semitones of vocal tract), K2 **Q** (formant bandwidth), K3 **CRSH** (LOFI sample rate reduction) |
| FX / LPG | K1 **WET %** (0 dry LOFI, 100 fully vocal-filtered), K2 **TONE** (LOFI one-pole brightness) |

Use MIDI CC16, CC17, CC18 for HOME K1–K3; CC20 SHIFT, CC21 Q, CC22 CRSH, CC23 WET, CC24 TONE. MIDI notes and pitch bend follow GATOR's MIDI channel settings.

Initial hybrid patch is a clean pulse, A vowel, full LOFI tone, crushing off, formant shift 0, medium Q, **90% wet**. All synthesis is monophonic for this first version.

## What we actually reused

- **Felucca-Engines**, pinned `04ffe903eb87a752305d1e5b10da01b53e4ab004` (MIT): unchanged `eng_lofi.c` and its wave oscillator, triggered by the existing `lo_render()` source host. No separately reimplemented chip oscillators.
- **JIANT**, pinned `f2fd85865ae4910a85069e848dd9f92bf2645996` (GPL-3.0-only): unchanged vowel table interpolation and resonator-coefficient functions in `voice_dsp.c`; `hy_filter_sample()` preserves its F1→F2→F3→F4 Q30 cascade math and soft-knee saturation, substituting the LOFI PCM for the glottal excitation.
- **Mutant adapter**: `firmware/apps/gator/hybrid_host.c` handles source patch mapping, per-note filter state, dry/wet mixing, and audio routing. It does not implement new chip/formant sound-generation algorithms.

**One documented safety fix in vendored Felucca code:** `firmware/apps/mutant/dsp.c` changed its tiny `mulq15` fixed-point multiply helper from `(a*b)>>15` to a 64-bit intermediate, to prevent undefined signed overflow during full-amplitude chip excitation. This is the only additional change to the pinned original Felucca shared DSP utility and applies to LOFI/VOICE too. It needs physical CPU-load testing because wide multiplies may cost cycles. `eng_lofi.c` remains unchanged.

## Build / simulator test

```sh
./build.sh --app gator --name mutant
python3 tools/sim.py host/hybrid-a.sim
python3 tools/sim.py host/hybrid-u.sim
python3 tools/sim.py host/hybrid-switch.sim
python3 tools/check_hybrid_wav.py
```

The CI workflow and `mutant-v05-test` artifact job run these tests. The A and U simulations use **identical oscillator settings** and change only VOWEL; the checker requires two distinct, audible, non-clipped WAVs. It also checks switching CHIP VOX → VOICE → LOFI → Plaits.

First successful build: <https://github.com/bwatanabe/fm1-mutant/actions/runs/40242044634>.
Results: A peak 29,709 / RMS 15,107; U peak 12,784 / RMS 6,484; clipped samples 0%, waveform relative difference 0.6942. Those numbers prove the routing and vowel parameter change the PCM; they do not prove subjective vocal quality or acceptable FM-1 real-time CPU load.

## Emulator and device acceptance

1. Go to the [v0.5 experimental build](https://github.com/bwatanabe/fm1-mutant/actions/runs/40242044634), download **mutant-v05-firmware-test** under Artifacts and extract `mutant.fwsc`. The public Mutant installer still serves the tested **v0.4**.
2. Open [Joakim Eriksson's FM-1 emulator](https://joakimeriksson.github.io/fm1-emu/) and choose **Open .fwsc…** to load v0.5. Start; tap HOME three times to reach CHIP VOX, or use CC19=127.
3. Play notes; at K3 move between A and U. Change WAVE, then try FX K1 WET from 0 to 100 to distinguish the dry chip output from the vocal resonators. Verify no stuck notes and return to the older synthesis models.
4. Only after emulator test: install that exact local `.fwsc` using **Install selected file** on the [Mutant website](https://bwatanabe.github.io/fm1-mutant/). **Do not use Install published build** if testing v0.5; that installs stable v0.4 instead.
5. On real FM-1, test OP-XY USB MIDI plus sustained notes and fast knob movements, listening for crackles, unexpected muting, overload, or clipping. If overloaded, return to published v0.4 using its established installer.

## Next possibilities

True multi-engine modulation routing and alternate excitation sources (Plaits, noise, physical modeling) are deferred. This stage establishes the **audio-in formant filtering** architecture. We should test both emulator and hardware before expanding it.
