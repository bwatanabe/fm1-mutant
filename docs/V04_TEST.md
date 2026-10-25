# FM-1 Mutant v0.4-dev — JIANT VOICE (experimental)

**Status:** prototype. Do not merge to `main` or install on real FM-1 until cross-compiler, host simulator and browser emulator checks have passed.

This version preserves the 24 original GATOR / Mutable Plaits models, Felucca LOFI (model 25), and adds **JIANT VOICE (model 26 / D2)**.

## Browser-friendly sound selection

When already on the HOME screen, one tap of HOME advances through **Plaits → LOFI → VOICE → the previous Plaits model**, repeating. From another page press HOME first to return to HOME, then use the shortcut. The original HOME+key selection also works.

Alternatively, turn SELECT to D2, or send **MIDI CC19 value 127** to choose VOICE. MIDI CC19 value **120** selects LOFI, **0** returns to Plaits model 1. These CC ranges differ from v0.3 because there are now 26 models.

## JIANT VOICE controls

Based on JIANT's original LANG-OFF formant synthesizer (not a rewrite). Its glottal oscillator, VOWEL tables, four serial Klatt resonators, and breath source are reused. The singing/WORDS code is deliberately excluded.

| GATOR page | VOICE controls |
|---|---|
| HOME K1 | VOWEL A–E–I–O–U (`0..127`) |
| HOME K2 | VOWL2 destination vowel (`0..127`) |
| HOME K3 | TALK vowel sweep rate (`0..127`); 0 disables |
| HOME K4 | Transpose |
| SCL / ATTN K1–K3 | SHIFT (`-12..12`), BUZZ, BREATH |
| FX / LPG K1–K2 | Resonator Q, per-note vowel RAND |
| MIDI CC16–18 | VOWEL, VOWL2, TALK |
| MIDI CC20–24 | SHIFT, BUZZ, BREATH, Q, RAND |

The initial VOICE sound maps from JIANT's existing VOX LEAD preset. VOICE is monophonic at present. Plaits/LOFI/VOICE each retain temporary sound settings independently when changing engines. GATOR's patch file format is unchanged; its Plaits modulation matrix isn't yet wired into the imported DSP.

## Automated checks

```sh
./build.sh
python3 tools/sim.py host/voice.sim
python3 tools/sim.py host/models.sim
python3 tools/sim.py --test
```

The first new test renders `build/host/out/voice.wav` and a VOICE screen plus `voice-3engines.wav` after returning through LOFI to Plaits.

## Provenance and differences

JIANT `firmware/src/eng_formant.c`, pinned upstream commit `f2fd85865ae4910a85069e848dd9f92bf2645996` (Oct 25, 2026), original authors Leo Kuroshita and Juan Ignacio Antonucci, GPL-3.0-only.
Vendored **adapted extract**: `firmware/apps/gator/voice_dsp.c` (see its header). Only LANG-OFF VOWEL synthesis was extracted; the source's glottal wave shaping, four serial resonators, vowel interpolation, and filter coefficients are retained. Adaptations are the sample-rate constants (48 kHz), TALK interpolation increment (replacing JIANT's generated ENV_LIN), and a simple note reset from its non-WORDS path.
Host adaptation: `firmware/apps/gator/voice_host.c`; it uses the same Felucca-defined voice data as LOFI.

**Deferred:** JIANT WORDS / syllables / lyrics; four-voice polyphony; feeding LOFI audio *into* the formant resonators; additional modulation routing. Do not claim existing VOICE is a reusable audio-in effect yet.
