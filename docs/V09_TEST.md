# FM-1 Mutant v0.9-dev — STUDIO and MUTANT

**Experimental, not yet hardware-validated.** Built on `feature/studio-mutant-v09`, branched from the still-draft ANIMAL VOX v0.8 branch. The public website and `main` remain on the hardware-accepted v0.7 release. Both v0.8 and v0.9 need emulator/device acceptance before merging to `main`.

## Intent

Two distinct personalities in **one sound instrument**. STUDIO is manual, repeatable exploration of the retro, chip, creature, Plaits and formant engines. MUTANT starts from a sound you like, then changes it autonomously through controllable movement, drift, decay, or occasional unexpected events. Neither mode implements a sequencer, sample player or a Fairlight emulator. The OP-XY continues supplying MIDI notes/patterns and optionally external CC automation.

### Button mapping

| FM-1 control | STUDIO | MUTANT |
| --- | --- | --- |
| **GLO tap** (release within ~650 ms) | Enter MUTANT (idle) | Return to STUDIO and stop motion |
| **GLO hold** (~650 ms) | Open existing GLOBAL settings | Open existing GLOBAL settings, WITHOUT switching machines |
| **ARP** (used as **RANDOM**) | Full-range randomization of sound parameters on the *selected* synthesis engine | One manually triggered small perturbation |
| **PLAY/STOP** | Small, depth-constrained **MUTATE** step | Start/stop autonomous change |
| **REC tap** | Save an in-RAM temporary capture of the current patch | Capture current evolving patch **without stopping** the evolution |
| **REC hold** (~650 ms) | Restore captured sound | Stop movement and restore captured sound |
| **SEQ** (repurposed as **MUT SETTINGS**) | Open 4-knob mutation settings | Open 4-knob mutation settings |
| **EDIT** | Original GATOR modulation matrix | Same original GATOR modulation matrix |
| **SAVE** | Original 16-slot GATOR preset SAVE | Same explicit SAVE, never automatically invoked by mutation |
| **HOME** | Existing model selection / shortcut | Same existing model selection / shortcut |

Existing original play/hold/trigger mode selections are preserved at **GLO hold → GLOBAL → ALGO encoder (KEY MODE)**. GLO tap/hold and REC tap/hold are deliberately distinguished on button release. The control panel's ARP/SEQ legends are repurposed inside Mutant firmware because those legacy sequencer features were not used.

## How random is RANDOM?

**Unrestricted within the current engine.** Press ARP in STUDIO and the full range of the following sound parameters is randomized: three HOME parameters, three attenuverters, both LPG/source tone parameters, transpose, and two additional sound parameters used by hybrid engines for sweep/grouping/mix. Randomization is *not limited to a friendly preset range*. Some results may be silence, very harsh noise, or unpleasant distortion; randomize again or restore a capture when that occurs.

The selected synthesis engine stays the same deliberately so you can explore, for example, repeated **CHIP VOX with NOIS** variations without bouncing into Plaits or another engine. GATOR's original eight EDIT modulation routes, the stored preset bank, MIDI settings and performance note mode aren't randomized.

PLAY in STUDIO executes a smaller two-parameter mutation relative to the currently audible sound. The **DEPTH** knob on the mutation settings page sets the size of that step.

## MUTANT controls

Press GLO briefly to enter MUTANT. Nothing moves until you press PLAY.

Press SEQ to enter **MUTATION** settings. Four regular FM-1 knobs adjust:

- **DEPTH** (0–100): permissible travel away from the patch captured when PLAY begins; 0 keeps the patch essentially unchanged.
- **SPEED** (0–100): how often new target positions are selected, from slower to faster.
- **INSTAB** (0–100): occasional sharper jumps instead of only smooth motion.
- **STYLE:** DRIFT / HAUNT / DECAY.

DRIFT gradually interpolates sound parameters toward changing nearby destinations. HAUNT does the same but often returns toward the original sound, with intermittent anomalies. DECAY widens its possible changes over about twelve seconds as the sound degrades and escapes its starting point.

Mutant's animation uses a lightweight control-rate random target generator; source DSP, sample-rate settings and the original EDIT modulation matrix remain unchanged. Mutation is *anchored* to the patch heard when PLAY was pressed, and never automatically switches engines. Press PLAY again to stop and leave the resulting patch in place. You can edit it, capture it, or save it.

A simple default is DEPTH 45, SPEED 35, INSTAB 25, STYLE DRIFT. Start with these before trying more extreme settings.

## REC capture / restore

REC tap captures a **copy of the entire current patch to RAM**. The evolving sound keeps running. You can continue mutating, then hold REC for around 650ms to stop motion and return to the captured sound. The capture does **not** consume one of the 16 preset slots and disappears after power-off. To persist it, use the normal SAVE procedure.

Opening another preset cancels an active autonomous session; saved presets and GATOR's persistent bank structure are unchanged. Engine switching also stops an active mutation so another engine's parameters aren't unintentionally driven by the old engine's targets.

## OP-XY and other MIDI

MIDI note-on/off, pitch bend and prior controller mappings are still handled normally. **CC27 chooses an exact zero-based model** (24 LOFI, 25 VOICE, 26 CHIP VOX, 27 ANIMAL, 28 ANIMAL VOX). Existing CC19 proportional mapping still works but is less stable when adding engines. The v0.9 mutation interface itself is controlled with FM-1 buttons and knobs; dedicated MIDI CCs for machine/mutation behavior have **not** been added yet. This keeps v0.9 small and avoids clashing with your OP-XY mappings.

Because original GATOR EDIT modulation remains a distinct mechanism, its existing routes still affect Plaits as they did before. We have **not** claimed that EDIT is wired into Felucca, JIANT, or the new mutation controls. That would be a separate engineering decision.

## First testing process

1. Download `mutant-v09-firmware-test` from the successful v0.9 GitHub Actions run under `feature/studio-mutant-v09`; extract `mutant.fwsc`.
2. In the full FM-1 browser emulator, use **Open .fwsc…** and Start. Load a LOFI / CHIP VOX / ANIMAL sound. Tap ARP twice: sound should randomize twice, but engine remain fixed. Press PLAY once in STUDIO for a smaller change.
3. Tap GLO (press then release) to enter MUTANT; screen footer should show MUTANT. Press SEQ to open mutation settings, adjust DEPTH/SPEED/INSTAB/STYLE, then HOME to return.
4. Press PLAY to start moving, play or sustain notes and listen to the changes. Try HAUNT and DECAY after DRIFT. Press REC briefly to capture an interesting moment; let the sound move away; **hold REC** to restore it.
5. Press/hold GLO ~650ms to verify the GLOBAL settings open without switching from MUTANT. In Global, the ALGO encoder controls old GATOR KEY MODE.
6. Make sure a saved preset is only changed after pressing SAVE. Test EDIT, returning to Plaits/LOFI/ANIMAL and switching models with CC27. Test same v0.9 `.fwsc` on real FM-1 + OP-XY **only after** browser validation, watching CPU headroom and discontinuities.

**Caution:** v0.9 initially tests algorithmic control and stability; audible clicks at very fast instability settings or extreme full-range RANDOM values are possible, and are useful test feedback. User hardware acceptance is required before merge.

## Tests

`host/studio-mutant.sim` and `tools/check_studio_mutant.py` assert that RANDOM actually changes parameters on successive clicks, the bank contents stay identical, GLO tap/hold work, SEQ opens the mutation page, PLAY starts motion, motion changes sound configuration, REC short captures without stopping, and REC hold restores the exact stored configuration. Full existing 29-engine DSP/audio/updater regressions run alongside.

## What to decide after device testing

Rather than continually importing engines, evaluate whether the two machines are fun and musically useful. Possible next refinements: selectivity (which parameters may mutate), easing/tempo, interesting mutation-style behaviors, preserving a history of captured states, or mapping mutation controls to OP-XY MIDI—based on what actually feels good to play.
