# FM-1 Mutant v0.9.1-dev — live performance controls

**Status: experimental / stacked development branch.** This branch builds upon the still-pending v0.9 STUDIO/MUTANT and v0.8 ANIMAL VOX branches; accepted `main` remains on v0.7. Do not merge without browser and physical FM-1 validation.

## New in 0.9.1

### REC now has a complete lifecycle

- **Tap REC** (<650 ms) captures the exact currently audible patch into RAM; LED stays lit after release.
- **Hold REC** (~650 ms to <2 seconds), **then release**, to stop mutation and restore the captured patch.
- **Hold REC** for **2 seconds or longer** to **CLEAR** the snapshot and turn the REC LED off. Release does nothing further.
- Captures are shared between STUDIO and MUTANT, intentionally. They are not audio recordings or flash presets. No snapshots survive power-off.
- An explicit short vs long vs extra-long gesture avoids accidental restore while trying to clear.

### OP-XY MIDI performance controls

The device's existing model/engine/MIDI CC controls are retained. Added *dedicated* controls that do not take over the four engine sound parameter knobs:

| MIDI CC | Parameter | Values |
|---|---|---|
| **40** | Mutation DEPTH | 0–127 mapped to 0–100 |
| **41** | Mutation SPEED | 0–127 mapped to 0–100 |
| **42** | INSTABILITY | 0–127 mapped to 0–100 |
| **43** | STYLE | 0–42 DRIFT, 43–85 HAUNT, 86–127 DECAY |
| **44** | RUN in MUTANT | 0–63 stop, 64–127 start; ignored in STUDIO |
| **45** | Manual mutation action | Rising edge from <64 to ≥64. RANDOM in STUDIO, small step in MUTANT |
| **46** | REC CAPTURE | Rising edge from <64 to ≥64 |
| **47** | REC RESTORE | Rising edge from <64 to ≥64 |
| **48** | STUDIO/MUTANT | 0–63 STUDIO, 64–127 MUTANT |
| **49** | REC CLEAR | Rising edge from <64 to ≥64 |

**OP-XY recommendation:** map its eight assignable CC controls in the external MIDI track on M2/M3, with M2's four encoders set to 40/41/42/43. M3 could still control the selected synthesis engine using CC16/17/18/23, or be reassigned to 44–47 for a mutation performance. OP-XY CC sequences and knob automation work without adding a second MIDI sequencer to Mutant.

Buttons still work as before: ARP=RANDOM/STEP, PLAY=MUTATE/RUN, GLO tap toggles machine, GLO long GLOBAL, SEQ opens mutation settings, EDIT keeps the existing GATOR modulation routes.

**Important:** remote mutation CCs obey the **selected MIDI receive channel**. CCs on other channels are ignored, including all action and model-select CCs. This is important when OP-XY controls multiple external devices.

### MIDI channel and LED backlight are independent persistent device settings

The hardware has three existing LED driver layers (full, dim glow, and configurable background); this update exposes timed hardware dimming, not simulated slow LED blinking.

- **GLO hold** (~650 ms) opens existing GLOBAL settings.
- **GLO hold again**, while viewing GLOBAL, opens the new **DEVICE** page.
- **DEVICE K1: BUTTON LIGHT** 0=off, 1=dim, 2=medium, 3=brighter inactive button labels. Active buttons remain full brightness. This applies **only to 14 control buttons**, not the note keyboard.
- **DEVICE K2: MIDI CH** 0=OMNI, 1–16=receive only on that channel. The same MIDI CH is still editable as GLOBAL K4.
- Press **SAVE while on GLOBAL or DEVICE** to save **MIDI CH + BUTTON LIGHT** as an independent persistent device configuration, with an on-screen success or error message. SAVE on either page does NOT overwrite a SOUND preset.
- That config is stored in a tiny, tagged, CRC-protected record in the existing *second* firmware flash storage object, separate from the original 16-slot GATOR sound bank (slot 0). Existing preset banks are not reformatted or migrated.
- Press SAVE from the regular sound page as before to save a sound. Global MIDI CH/LED setting does not require sacrificing a sound slot.
- On boot: load original sound bank first, then apply optional device config if its magic/version/data are valid. Otherwise default to saved bank MIDI channel and medium button lighting.
- New LED setting is hardware-dependent: actual readability and relative brightness need verification on physical FM-1. At off=0 full-bright active buttons still light normally.

Global LEGATO / bend range / fine tune and other existing GATOR controls are still present; this patch specifically saves **MIDI CH** and **BUTTON LIGHT** independently. GATOR patch save still stores its existing global struct for backward compatibility.

### Alternate expressive controllers to explore after this build

The existing MIDI note/velocity/CC path can be played from more than the OP-XY. Useful future tests:

- **Moog Theremini:** Moog confirms its USB interface sends **CC messages, not conventional MIDI note-on/off or gate events** ([official Moog support](https://moogmusic-help.freshdesk.com/en/support/solutions/articles/69000840674-can-the-theremini-be-used-to-play-an-external-synth-using-midi-)). By default pitch antenna sends CC20 and volume CC2 on channel 1, configurable on the Theremini. For v0.9.1 assign those antenna controls to **CC40 DEPTH** and **CC41 SPEED** (on the same MIDI channel as Mutant) while holding a note from OP-XY. True theremin-style note/gate/pitch performance will require a separate translator or a future explicit CC-to-note/gate mode; **do not assume Theremini directly triggers notes**. USB hosting/routing may require OP-XY or another MIDI host. No Theremini-specific gate logic is included.
- **Roland SPD-SX:** each pad can transmit a chosen note number/channel and configure gate lengths. Use its pads to trigger sound events on Mutant, potentially mapping shoulder pads to mutation actions after a separate MIDI routing/translation review. No SPD-SX-specific note mapping or drum quantization was added.

Neither controller requires changing Mutant's engine DSP. If using multiple controllers, choose routing, MIDI channel, and USB/DIN connections deliberately; MIDI data from different channels won't accidentally trigger Mutant when MIDI CH is specific.

## First acceptance checks

1. Load v0.9.1 `mutant.fwsc` from the passing experimental GitHub Actions run in the full browser FM-1 emulator. Test REC tap, REC hold released between 650ms and 2s, and 2s CLEAR LED off.
2. Try STUDIO ARP/RANDOM, MUTANT RUN and speed/depth. In the emulator inspect the new DEVICE page from GLO hold twice.
3. Test a specific MIDI channel from DEVICE K2, SAVE there, reboot, and verify MIDI CH and button lighting persisted; no preset overwritten.
4. On OP-XY's matching channel, send CC40–43 while playing. Check 44=RUN, 45=RANDOM/STEP, 46=CAPTURE, 47=RESTORE, 48=MODE, 49=CLEAR. For momentary action CCs, send low (0) between high (127) triggers.
5. Send control CCs on a **different channel**; none should change Mutant. The primary MIDI engine selection remains CC27 zero-based model number.
6. After emulator validation install the **same local file** on physical FM-1 through [Mutant's Web MIDI updater](https://bwatanabe.github.io/fm1-mutant/) → **Install selected file**, NOT **Install published build** (still accepted v0.7). Confirm inactive button brightness is useful and no audio/UI timing problems under rapid CC automation.

The simulator suite includes explicit snapshots, REC clearing, GLO GLOBAL→DEVICE, device brightness, MIDI channel gating and CC actions, and all preexisting 29-engine audio regressions. Host simulator cannot assess physical LED luminosity, persistence across actual power cycles, or physical CPU headroom; these need the hardware test.

## Design deferred to later versions

No tempo/clock changes in this release. MIDI clock, internal BPM and beat/bar mutation sync remain candidates. Also deferred: migration from 16 to 32 sound/scene slots, Program Change preset recall, true mutation parameter-selection mask, on-device clip sequencing, Theremini-specific continuous-pitch mapping, and Roland SPD-SX shoulder pad gesture mapping.
