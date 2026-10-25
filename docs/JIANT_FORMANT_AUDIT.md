# JIANT formant import — source audit and v0.4 scope

**Status:** architecture audit / implementation plan; no formant audio code imported yet.
**Branch:** `feature/jiant-formant` (based on accepted Mutant v0.3-dev on `main`).
**Goal:** reuse existing JIANT synthesis DSP as much as possible. The application remains a **MIDI-controlled sound instrument**, not a groovebox.

## Proven foundation

User has verified Mutant v0.3-dev, with 24 Plaits models and Felucca LOFI as model 25, in both the FM-1 browser emulator and on real FM-1 hardware driven by OP-XY USB MIDI. Its cleaner PULSE LD initial state and the other waveforms behave as expected.

Do not modify `main` while importing formants.

## Donor code (pin this revision)

- JIANT <https://github.com/juanjiant-bit/JIANT>, main commit `f2fd85865ae4910a85069e848dd9f92bf2645996` (2026-10-25).
- `firmware/src/eng_formant.c` — approximately 470 lines, the actual Klatt-inspired four-formant singing oscillator and processing, with `vowel_at()`, `fres_coef()`, `formant_note_on()` and `formant_render()`.
- `firmware/src/sing.c` — approximately 1,307 additional lines plus generated `felucca_lyrics.h`, only needed for the WORDS/syllable/lyric behavior.
- JIANT licensing: GPL-3.0-only for `eng_formant.c`. Retain source and attribution in any distributed version.

## Key dependency findings

The **current** JIANT formant module is *not* an independent plug-in like Felucca LOFI:

1. It unconditionally `#include "sing.c"`, which expects generated lyric data and JIANT track state.
2. Its WORDS path relies on `P_VLANG`, `sing_note`, `sing_block`, `sing_t`, and a large associated language/articulation state.
3. Its improved sung glide refers to `voice_t.gfrom` and `voice_t.gacc`; Felucca-Engines' existing reusable `voice_t` definition used by Mutant LOFI does not expose those members.
4. JIANT's full `track_t` and `voice.c` are coupled to the workstation, multi-track mixer, and voice scheduler. Porting that entire stack would undermine the project's purpose.
5. The actual **glottal source, four resonators, vowel tables, and fixed-point DSP** are the sections worth preserving.

GitHub does not expose a verified pre-WORDS source tag named `v0.7.0`, `v0.8.3`, or `v0.8.5` in this repository. Avoid pretending a simpler upstream build is available until identified and verified.

## Chosen v0.4 approach — minimal donor adaptation

**Stage A: VOICE standalone**
- Start from the actual JIANT `eng_formant.c` glottal source, vowel table interpolation, Klatt resonator design, and PCM renderer. Leave the DSP equations unchanged.
- Exclude the lyrics/WORDS branch in a marked, vendor-derived **VOICE-VOWEL-only** adaptation rather than importing the full `sing.c` subsystem.
- Adapt the minimal host-facing state (`track_t`, voice fields, sample-rate tables, note state) to the stable Mutant engine interface. Treat alterations as thin compatibility glue; document what lines changed from JIANT.
- Add **VOICE** as model 26 alongside Plaits 24 + Felucca LOFI 1. Map note on/off, velocity, MIDI pitch, and three or four meaningful vowel/tone controls. Existing GATOR preset struct remains unchanged.
- Keep the same browser emulator, `.fwsc` installer, and GitHub Actions CI. Do not merge until hardware retest.

**Stage B: modulation / cross-engine exploration**
- Refactor the *existing* four-formant resonator stage into a process function accepting external audio samples. Reuse JIANT's exact coefficient and processing approach.
- Feed **LOFI pulse / triangle / noise** output into that resonator stage, while allowing vowel position and vocal-tract shift to be controlled by FM-1 encoders or USB MIDI CC.
- Keep sound-generation and audio-input filtering separate: simple VOA engine first, hybrid filter routing after.
- Profile on hardware for CPU budget and memory size. Do not assume the web emulator predicts FM-1 timing.

## Explicit non-goals

No lyric generator, songs, multitrack sequencer, screen redesign, new synthesis algorithms, comprehensive TTS, or polyphony at the beginning. Reuse donor source rather than simulating the engine in JavaScript.

## First implementation acceptance tests

1. Build under the existing cross compiler; all 25 prior models still work.
2. Run a deterministic browser-host simulation selecting model 26 using MIDI CC19 and playing notes.
3. Verify intelligible vowel morphing and no clicks/stuck notes when switching between Plaits, LOFI and VOICE.
4. Test same `.fwsc` on the physical FM-1 with OP-XY USB MIDI.
5. Only then merge the model-26 addition to `main`.
