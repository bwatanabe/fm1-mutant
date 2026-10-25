# Browser preview and FM-1 updater

The experimental browser page lives in `web/mutant.html`. The hosted version is published by `.github/workflows/mutant-preview.yml` after a successful build.

## How it works

1. `./build.sh --app mutant` compiles the FM-1 firmware to `build/mutant.fwsc`.
2. `python3 tools/sim.py --app mutant` tests the app under GATOR's host simulator.
3. `python3 web/make_mutant_site.py` packages **that same firmware file**, a SHA-256 manifest, the browser page, and the existing GATOR/Felucca update protocol in `build/mutant-site`.
4. `python3 -m http.server 8000 --directory build/mutant-site` runs the site locally at http://localhost:8000.

For remote hosting, go to the repository **Settings → Pages → Build and deployment → Source: GitHub Actions**. The `mutant-preview` Actions workflow builds the app and deploys the static site. **If Actions are disabled on the fork, enable them first in the Actions tab.** The expected URL, once deployment succeeds, is https://bwatanabe.github.io/fm1-mutant/ ; do not assume it is live before deployment succeeds.

## Browser audio preview

The page embeds [Joakim Eriksson's FM-1 emulator](https://joakimeriksson.github.io/fm1-emu/), which runs unmodified firmware in the browser. Unlike a JS approximation, its DSP is the same firmware code that runs on hardware.

**Current limitation:** the embedded emulator does not automatically receive our hosted firmware due to cross-origin isolation; its initial firmware is not Mutant. To test Mutant, click **Download Mutant .fwsc**, open the full emulator, use **Open .fwsc…**, choose the downloaded file, and Start. When a new build is deployed, download and load its new .fwsc file. We can improve this later with an upstream emulator integration or a self-hosted emulator.

## Firmware updating

The same page offers **Install published build** (SHA-256-checked) and **Install selected file** (an explicitly selected local .fwsc). Both use GATOR's existing `fm1ota.js` transport through Web MIDI SysEx, verifying the package uses the known `FM-1_9xx` identity and Felucca loader marker.

Use Chrome or Edge, charge the device, connect a direct USB data cable, close other MIDI applications, and never unplug while writing. A successful browser simulation does **not** prove the binary is safe or performant on actual hardware. Do not install until the target firmware passes CI and the experiment is explicitly approved for hardware testing.

## Copyright

GATOR framework and updater: GPL-3.0-only. Felucca-Engines DSP: MIT. FM-1 emulator: separate MIT project hosted and maintained by its developer; the iframe is remote. The preview site is not affiliated with M-VAVE.
