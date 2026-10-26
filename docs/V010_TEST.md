# FM-1 Mutant v0.10-dev — named sound library and clocked mutation

**Experimental branch.** Built on hardware-accepted v0.9.1. Sound generation remains the same 29 engines (Plaits, LOFI, VOICE, CHIP VOX, ANIMAL and ANIMAL VOX). No additional DSP algorithms, sequencer, or persistent mutation SCENEs added.

## 1. Named library — 32 sound slots

The bank in firmware flash object 0 now contains **32 sounds**, 11-character names, and an occupancy bitmap. The original 16-sound bank **version 1** is recognized by exact length, magic, and version. Its sixteen patch records and current preset position are kept intact. The sixteen added slots start empty and the migrated bank stays **in RAM** until you intentionally save or rename something.

The new bank is saved in the same power-fail-resistant dual-sector A/B flash object. A compile-time assertion verifies its complete payload fits the existing 3,840-byte limit. Your separate MIDI channel and button brightness still live in object 1, unchanged.

**Important backward-compatibility caution:** newer firmware understands both v1 and v2. **Older v0.9.1 firmware does not understand v2.** After you save or rename in v0.10, flashing back to an older version may result in an initialized factory bank; do not assume older firmware will retain your new library. Test the emulator before flashing; consider recording important sounds or backing up your instrument's settings before adopting the new library as your primary firmware. No on-device bank export/backup facility has been implemented yet.

### Browser operations

- **PRESET knob** on any sound page browses saved sounds. Empty slots do not replace your currently playing patch.
- Press **SAVE** to open the **PATCH LIBRARY** browser (32 slots). Turn SELECT / ALGO / K1 to choose the target slot. Three visible rows show previous, selected and next slots, including name or EMPTY.
- With an existing saved slot selected, press **EDIT** to edit **its name only**. In the name editor, turn SELECT or K1 for the character position and ALGO or K2 to change a character from space, A–Z, 0–9, dash and underscore. Press SAVE to save **only the new label**, without overwriting that saved sound. Press HOME to leave without changing the stored label.
- With an empty slot selected, EDIT similarly stages a name; press SAVE once to accept the name, then SAVE again from the library to save your current **sound and its name**.
- Press SAVE from the library to **save/overwrite the selected sound**. This includes edits you made to a recalled patch, and still preserves the full original GATOR modulation routes.
- To **delete**, select a slot in PATCH LIBRARY, hold REC for **2 seconds** until **REC TO DELETE**, then *release and tap REC* within 8 seconds to confirm. SAVE cancels deletion; choosing a different slot also cancels. The last remaining stored sound cannot be deleted, guaranteeing a playable boot patch.
- Deletion is logical removal from the bank; free slots may be reused. It does not guarantee secure erasure of all historical flash A/B copies.

**Presets store sounds, not mutation-session settings.** As agreed, DEPTH, SPEED, INSTABILITY, STYLE and clock choices remain performance controls rather than presets this round.

## 2. MUTANT may continue through engine changes

With MUTANT RUN enabled, switching engine using HOME, SELECT or **MIDI CC27** now leaves RUN active. Mutation targets are **re-anchored** at the new engine's current patch, so chip-wave parameters cannot inadvertently glide into unrelated vocal/animal parameter meanings. Switching synths still silences the previous engine's active note voice, so the next note-on retriggers the new engine as before. STOP retains the current evolving sound. Recalling a stored preset still stops the mutation session deliberately.

## 3. Three choices for mutation timing

Press **SEQ** once for MUTATION controls, then **SEQ again** for the new CLOCK page. Press SEQ on CLOCK to return to MUTATION. The CLOCK page knobs:

| Knob | Control | Values |
| --- | --- | --- |
| K1 | CLOCK SOURCE | FREE / INTERNAL / MIDI |
| K2 | BPM | 40–240, used when INTERNAL selected |
| K3 | DIVISION | 1/4 beat, 1 beat, 2 beats, 1 bar, 2 bars |

**FREE:** existing v0.9.1 timing, influenced by SPEED.

**INTERNAL:** mutation chooses a new target at the chosen beat/bar division of the internal BPM, with no dependence on another device. Example 120 BPM + 1 bar = a new direction every 2 seconds in 4/4.

**MIDI:** accepts standard MIDI Clock **F8 at 24 PPQN** from OP-XY (or compatible source), and respects MIDI START (FA), CONTINUE (FB), STOP (FC). It works regardless of Mutant's selected MIDI note channel: MIDI clock is channel-less. If no MIDI clock arrives within 1.5 seconds, new targets pause rather than guessing the tempo. Current interpolations are allowed to complete. Timing divisions assume four quarter-note beats per bar.

**Important:** synchronization controls when Mutant picks a new *target*. It **does not quantize the audio or create a step sequencer**; morphs glide continuously between successive targets. The existing GATOR EDIT LFOs are not themselves tempo-synchronized by this change.

Optional MIDI CC50=clock source (0–42 FREE, 43–85 INTERNAL, 86–127 MIDI), CC51=internal BPM (40–240), CC52=division. These operate on the selected Mutant MIDI channel; system realtime clock events operate on all channels.

## 4. Testing before physical installation

The full GitHub CI compiles the original and combined engines and tests:
- Actual 16-slot old-bank image migration (known sound data retained) without writing over old flash until the user SAVEs.
- Independent name-only edit, saving slot 25, reloading new bank and guarded deletion with confirmation.
- FREE/INTERNAL/MIDI clock behavior, 24 ticks per quarter note, STOP/START, and continuing RUN when switching synth engines.
- Previous v0.9.1 REC, GLO, device lighting, MIDI CC40–49, and all 29 synthesis-engine audio tests.

Download **mutant-v010-firmware-test** from a successful Actions run on branch `feature/patch-library-clock-v010`, extract `mutant.fwsc`, load it with **Open .fwsc…** in the [full FM-1 emulator](https://joakimeriksson.github.io/fm1-emu/) first. Test saving/editing an expendable patch before installing it on physical FM-1 using **Install selected file** at https://bwatanabe.github.io/fm1-mutant/. **Install published build** still selects the accepted stable release, not experimental v0.10.

On your OP-XY, enable MIDI clock output for clocked mode and send CC27 to switch engines while a sustained MUTANT session runs. Listen for continuity, timing musicality, envelope note handover, and any audio CPU overload. Verify saved names and sound slots across a real device restart.

## After v0.10

Regroup on whether named storage and tempo make sessions more usable. Potential subsequent work: safe bank import/export (before frequent format changes), MIDI Program Change preset recall, more refined target interpolation, selective mutation targets, or gesture mapping for Theremini antenna CCs and SPD-SX pads. No additional engine imports are planned by default.
