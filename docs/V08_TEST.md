# FM-1 Mutant v0.8-dev — ANIMAL VOX (first animal/formant hybrid)

**Experimental.** All 29 models compiled and all automated simulations passed on the initial code revision. The newly introduced ANIMAL VOX must still be auditioned in the browser emulator and on physical FM-1 + OP-XY, particularly for CPU headroom under complex creature sounds. Do not merge into `main` until user acceptance.

## What it is

Model **29 (ANIMAL VOX / D5, zero-based model 28)** feeds *actual output* from JIANT's five ANIMAL source algorithms through the same four Klatt-style resonators already proven in CHIP VOX. No new oscillator or vocal-filter algorithm is invented.

Five sources: **CMOS** (bird-machine chirps), **PULSE** (insect/frog/clicks), **WHSTL** (bird whistles), **WING** (flying insect pulses), **GLOTT** (irregular growls/mammal vocal source). These modes remain independent in the standalone ANIMAL model 28 (index 27). The new sound is not intended to form identifiable spoken letters or numbers.

The existing tested v0.7 remains published on `main`. v0.8 has a separate, selectable patch and resonator state; changing engines restores each family's temporary edits. Storage still uses GATOR's existing sixteen-slot patch structure.

## Controls

| Page | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| HOME | **MODE**: CMOS/PULSE/WHSTL/WING/GLOTT | **CALL**: touch, click, pressure, buzz or mouth | **VOWEL**: A-E-I-O-U | Transpose |
| SCL | Formant **SHIFT** (±12 semitones) | Formant **Q** | Source **CHAOS** / coupled instability | — |
| FX | **WET %**: 0=dry original creature, 100=vocal-filtered creature | **RATE**: source chirp/pulse rate | **GROUP**: call grouping | **TEMP/FM**: source temperature/air/FM |

Other ANIMAL parameters (SIZE/CAP and SWEEP/HYST) are set at fixed neutral defaults for this first hybrid, because the priority is making its essential vocal filtering and creature behavior playable with the existing FM-1 screen.

**Starting preset:** CMOS BIRD source, A vowel, 85% wet, moderate rate, GROUP=hold. Experiment with fully wet first, then pull back the WET knob to introduce the original creature sound.

**MIDI routing:** CC27=28 selects ANIMAL VOX exactly, irrespective of how many new engines are added. CC19=127 also selects the final model but CC19's proportional thresholds change with each added model. On ANIMAL VOX, CC16–18 map to MODE, CALL, VOWEL; CC20–22 map to SHIFT, Q, CHAOS; CC23–24 map to WET and RATE; CC25–26 map to GROUP and TEMP/FM. Note-on/off, velocity and pitch bend use the existing MIDI channel settings.

In the browser emulator (where holding HOME and pressing a key is inconvenient), press HOME repeatedly while already on HOME to cycle Plaits → LOFI → VOICE → CHIP VOX → ANIMAL → ANIMAL VOX → Plaits. From another page, first tap HOME to navigate to HOME. Physical SELECT also navigates to D5.

## Provenance

- `firmware/apps/gator/animal_dsp.c` is the unchanged GPL-3.0-only JIANT ANIMAL DSP source, pinned upstream revision `f2fd85865ae4910a85069e848dd9f92bf2645996`.
- `firmware/apps/gator/voice_dsp.c` retains the JIANT original Klatt-derived four-formant coefficient functions and vowel lookup.
- `firmware/apps/gator/hybrid_host.c` now accepts a separate filter-state pointer and active MIDI pitch, retaining the exact filter-equation and per-block update math from CHIP VOX.
- `firmware/apps/gator/animal_vox_host.c` is glue: remaps patch parameters to the unchanged ANIMAL renderer, holds independent filter state, mixes wet and dry PCM, and controls notes.
- Original Plaits and Felucca LOFI engines remain untouched. No sequencer, lyrics or workstation features were imported.

## Automated validation

The new tests `host/animalvox-{cmos,pulse,whstl,wing,glott}.sim` render all five modes through the filters. `animalvox-cmos-u.sim` and `animalvox-cmos-dry.sim` compare changes in vowel and wet/dry relative to `animalvox-cmos.sim`. `tools/check_animal_vox_wav.py` rejects silence, digital clipping, indistinguishable source modes or a bypassed filter.

In the first successful workflow, all modes produced sound without clipping, the CMOS A/U sound differed by **0.8979** relative PCM and wet/dry by **0.4738**. This confirms signal routing, not that every mode will sound musically convincing on actual FM-1 hardware. Peak amplitudes were CMOS 26,580, PULSE 11,663, WHSTL 9,683, WING 30,623, GLOTT 25,873 out of 32,767. The old ANIMAL, CHIP VOX motion, LOFI, VOICE and Plaits regressions also passed.

## Try in emulator, then device

1. Open the latest **successful** [v0.8 GitHub Actions workflow](https://github.com/bwatanabe/fm1-mutant/actions/workflows/mutant-v08-test.yml), select the run for branch `feature/animal-formant`, and download **mutant-v08-firmware-test** under Artifacts.
2. Extract `mutant.fwsc` and open it in [the full FM-1 emulator](https://joakimeriksson.github.io/fm1-emu/) using **Open .fwsc…**, then Start.
3. Select **ANIMAL VOX (D5)** by tapping HOME five times from a Plaits model on HOME, or send MIDI CC27=28. Start with CMOS and turn HOME K3 VOWEL while holding a note; on FX sweep WET from 0 to 100%. Change MODE to WHSTL or GLOTT and see if each source retains its distinctive sound.
4. Check plain ANIMAL (CC27=27) and CHIP VOX (CC27=26) still work. Confirm note releases and the ability to go back to Plaits.
5. **After** a good emulator test, use **Install selected file** on [Mutant's installer website](https://bwatanabe.github.io/fm1-mutant/) to flash that same v0.8 `.fwsc` to the actual FM-1. **Do not choose Install published build**: that deliberately still installs hardware-tested v0.7.
6. On device with the OP-XY, test sustained animal notes, rapid MODE changes, the FX wet/dry transition, and fast vowel sweeps. Listen for dropouts, DSP overload and unexpected muting. Actual FM-1 CPU/audio timing cannot be verified solely through CI or emulation.

## Regroup after acceptance

After the physical v0.8 test, evaluate the existing engines as an instrument—what produces distinct expressive textures, which parameters matter, whether the most valuable next development is a small modulation-routing system, improved sound preset navigation, or another donor engine. Deliberately pause new engine imports while deciding that with the user.
