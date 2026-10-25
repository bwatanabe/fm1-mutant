# FM-1 Mutant v0.7-dev — JIANT ANIMAL

**Experimental build.** The user has confirmed v0.6 CHIP VOX in both the web emulator and the real FM-1 with OP-XY; that accepted version is preserved on `main`. v0.7 adds JIANT ANIMAL as **model 28**, while retaining the 24 Plaits models plus LOFI, VOICE and CHIP VOX.

## Original synthesis code

`firmware/apps/gator/animal_dsp.c` is the **unchanged** JIANT ANIMAL source, pinned to JIANT commit `f2fd85865ae4910a85069e848dd9f92bf2645996` (GPL-3.0-only), original authors Leo Kuroshita and Juan Ignacio Antonucci. Five physical-/circuit-inspired modes:

- **CMOS (0):** two Schmitt-trigger/RC oscillators with coupled chirps, gate and phrase capacitor; bird-machine character.
- **PULSE (1):** short strike trains and resonators for crickets, cicadas, frogs, clicks.
- **WHSTL (2):** Hopf-based bird-call whistle with nonlinear pressure/chaos and pitch hooks.
- **WING (3):** flight/wingbeat pulse with wandering pitch and Doppler.
- **GLOTT (4):** irregular mammal-style larynx source with mouth and roughness.

Mutant uses the existing Felucca `voice_t`, oscillator and filter helpers. JIANT-specific `FM6_EXP2` and `LFO_INC` tables are generated from its own `tools/gen_tables.py` formulas at the selected sample rate (48 kHz for FM-1) by `firmware/apps/gator/gen_animal_tables.py`. The original `noise_inc()` pitch extension function is reused from JIANT's `eng_noise.c` inside the host bridge. JIANT's workstation/sequencer and lyric code are not imported.

## Finding the engine

- **From the HOME screen:** each press of HOME cycles Plaits → LOFI → VOICE → CHIP VOX → **ANIMAL** → last Plaits. This uses one key at a time in the web emulator. From a different page, press HOME to return to HOME first.
- **MIDI CC27 = 27** selects ANIMAL by its exact **zero-based** model index; CC27 = 26 CHIP VOX, 25 VOICE, 24 LOFI and 0–23 the Plaits models. **Use CC27 for stable OP-XY model selection**; existing CC19 selects proportionally to the total model count, so its model-selection thresholds change when a model is appended.
- **MIDI CC19 = 127** also selects ANIMAL, as the last model.
- Physical FM-1: SELECT to bank **D4**, or HOME+28th key, if available.

## Controls

The basic Mutant mapping remains 3 HOME parameters, 3 on SCL and 2 on FX:

| Page | Knob | ANIMAL parameter |
|---|---|---|
| HOME | K1 | MODE: CMOS, PULSE, WHSTL, WING, GLOTT |
| HOME | K2 | TOUCH / CLICK / PRESS / BUZZ / MOUTH, depending on mode |
| HOME | K3 | COUPLE / Q / CHAOS / DRIFT / ROUGH, depending on mode |
| HOME | K4 | Transpose |
| SCL | K1 | CAP (CMOS) or SIZE (other modes) |
| SCL | K2 | HYST / DETUNE / SWEEP / DOPPLR (mode-dependent) |
| SCL | K3 | RATE |
| FX | K1 | GROUP (0–15, HOLD/TRILL/chirp groupings) |
| FX | K2 | FM (CMOS), AIR (GLOTT) or TEMP (other modes) |

MIDI CC16, 17, 18 map to the three HOME knobs; CC20, 21, 22 map to SCL, and CC23, 24 to FX. Note on/off, velocity and pitch bend follow the normal GATOR midi-channel setting. The voice is monophonic in this first ANIMAL version. The initial ANIMAL patch matches JIANT's `CMOS BIRD` source parameters.

Original JIANT's preset metadata and sequencing are omitted. In GATOR the original 16 preset storage slots and patch struct remain unchanged; switching to ANIMAL keeps a separate temporary sound cache. Note that the labels are context-dependent, but all five modes share the same parameter fields.

## Browser test

1. Download `mutant-v07-firmware-test` from the **successful** GitHub Actions `mutant-v07-test` run on `feature/jiant-animal`. Extract `mutant.fwsc`.
2. Open the [full FM-1 emulator](https://joakimeriksson.github.io/fm1-emu/), choose **Open .fwsc…**, load the file, and start. The public Mutant website is still on stable v0.6.
3. Select ANIMAL using MIDI CC27=27 or the single-button HOME cycle. On HOME, turn K1 through CMOS, PULSE, WHSTL, WING and GLOTT; play with K2/K3 while holding a note.
4. Explore SCL RATE and FX GROUP for call rhythm; confirm MIDI notes stop on release. Switch back to CHIP VOX (CC27=26) and LOFI (CC27=24) to check regressions.
5. After successful emulator testing, install the **same** v0.7 `.fwsc` using **Install selected file** on the [Mutant firmware updater](https://bwatanabe.github.io/fm1-mutant/). Do not use *Install published build* for v0.7. Listen for audio overload, clicks or stuck notes on physical FM-1 + OP-XY before merging.

## Automatic tests

`host/animal-cmos.sim`, `animal-pulse.sim`, `animal-whstl.sim`, `animal-wing.sim` and `animal-glott.sim` exercise each JIANT algorithm separately. `tools/check_animal_wav.py` checks output length, silence, severe clipping and that adjacent source modes are not identical. Full CI still runs LOFI, VOICE, CHIP VOX, animated vowel and all original Plaits regression tests. Passing a simulator is not a substitute for FM-1 CPU/performance testing.

## After this

The next candidate would be combining ANIMAL's especially interesting CMOS or GLOTT sound sources with the existing JIANT vocal resonator stage. That is **not included** in v0.7; first validate the donor engine in isolation.
