# FM-1 Mutant combined firmware — Plaits + Felucca LOFI

This experimental build is the **same firmware application** with the original 24 GATOR/Plaits synthesis models and an additional **LOFI CHIP** engine at index 24 (model **25**, bank **D1**).

The synthesizer remains a sound module, not a groovebox: the OP-XY, iPad or computer can supply notes and CC automation. This version does **not yet** combine chip and formant synthesis in an audio chain.

## Selecting an engine

1. On the built-in FM-1 keys, hold **HOME** and press the **25th key** (index 24) for LOFI CHIP.
2. Alternatively, send **MIDI CC19 = 127** on the active channel, or adjust the SELECT encoder until **D1 — LOFI CHIP** appears.
3. Return to any of the 24 original Plaits models by selecting another HOME+key or turning SELECT.
4. The PRESET knob still accesses GATOR's original 16 patch slots. Changing the engine and saving writes the existing GATOR patch format — **no storage migration**.

## LOFI controls

LOFI uses the existing GATOR pages. The existing DSP module is copied unchanged from Felucca-Engines; its processing types include pulse, triangle, saw, shift-register noise and 4-bit wavetable.

| Page | Controls |
| --- | --- |
| HOME | K1 CHIP (0–4); K2 WAVE (pulse / triangle / saw / noise / WRAM); K3 DUTY; K4 transpose |
| SCL / ATTN | FM → CRSH; TIMBRE → SWP; MORPH → VIB |
| FX / LPG | DECAY → ARP; COLOUR → TONE. OUT/AUX and STEREO controls are currently inactive for LOFI's mono output |
| Other pages | Remain GATOR controls; the Plaits modulation matrix and LFO pages are **not yet connected to the LOFI renderer** |

MIDI CCs: CC16=CHIP, CC17=WAVE, CC18=DUTY (original GATOR main-knob mappings); CC19 chooses the model; when LOFI is selected CC20=CRSH, 21=SWP, 22=VIB, 23=ARP, 24=TONE. CC120/123 silence notes. Note-on/off and pitch bend are implemented. Channel filtering still follows GATOR's GLOBAL MIDI CH setting.

Notes: LOFI remains **monophonic** in this build, and we use a deliberately simple host envelope to avoid replacing its DSP. Switching synth families silences the previously active voice. Do not expect all GATOR modes, modulation routes or Plaits-specific controls to behave identically on LOFI.

## Browser testing

At https://bwatanabe.github.io/fm1-mutant/ download the published `mutant.fwsc`. Load it in the full browser FM-1 emulator using **Open .fwsc…**, then Start. Try HOME+key25 or CC19=127, play from the on-screen keyboard or MIDI, turn K1–K3 and use the SCL/FX pages. Switch back to Plaits and confirm notes still play.

**Do not assume emulator CPU timing represents physical FM-1 performance.** After basic browser testing, install through the same site and test the OP-XY over USB MIDI on real hardware, listening for glitches and confirming the ability to return to known-good GATOR.

## Provenance

- Firmware framework, MIDI, audio transport, preset persistence, UI: GATOR (GPL-3.0-only)
- 24 original synthesis models: Emilie Gillet's Plaits DSP (MIT)
- LOFI oscillator, filters, arpeggiation, chip noise, synthesis data and DSP helpers: **unchanged** Felucca-Engines at `04ffe903eb87a752305d1e5b10da01b53e4ab004` (MIT)
- The new `firmware/apps/gator/lofi_host.c` contains only glue: note handling, basic envelope, parameter scaling and PCM audio conversion.

## Next milestone

Once this integration passes browser and device checks, adapt the JIANT VOICE/formant engine using the same small-host approach and then experiment with chip oscillator output routed into formant filters. Keep the firmware source and tests on GitHub.
