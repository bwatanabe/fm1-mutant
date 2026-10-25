# FM-1 Mutant / LOFI integration

This branch adds a **separate** `mutant` application inside the existing GATOR/Felucca hardware platform. The upstream GATOR application, bootloader and hardware drivers are not modified. This is an experimental stepping stone; Plaits and LOFI are not yet switchable inside one firmware.

## Build and preview

```sh
python3 tools/sim.py --app mutant
./build.sh --app mutant
```

GATOR's build tools generate the Felucca 48 kHz tables automatically for the Mutant app. The simulator creates `build/host/out/mutant.wav` and `mutant.png`. Run the same CI checks before trying hardware. Flashing any unofficial firmware has recovery risks; preserve your known-good GATOR installer.

## Controls

* Local keys or USB MIDI note-on/off (any channel): monophonic LOFI engine.
* EDIT switches between four CHIP and four MOTION parameters; knobs 1–4 edit values.
* SELECT adjusts CHIP, ALGO adjusts WAVE, PRESET cycles the five upstream LOFI presets.
* CC1 controls TONE; CC16–23 control the eight Felucca LOFI parameters; CC120 / CC123 sends all-notes-off.

## Provenance

The upstream engine code and table generator are pinned, **unchanged**, from Felucca-Engines commit `04ffe903eb87a752305d1e5b10da01b53e4ab004` by Leo Kuroshita (@kurogedelic), Hügelton Instruments; see `LICENSES/MIT-Felucca-Engines.txt`. The host adapter uses the public `felucca_engine.h` API. Tests and real-hardware timing are not yet confirmed.

## Next

First get GATOR, demo and Mutant builds all passing. Then test the FM-1 and OP-XY on hardware. After that, combine Plaits + LOFI into a single switchable synth with a stable shared engine interface; JIANT formants come later.
