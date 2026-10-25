#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Create a static Mutant test/installer website from build/mutant.fwsc.

This never rewrites the original GATOR web site. It packages the exact
same .fwsc image for emulator download and for Web MIDI device update.
"""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WEB = ROOT / "web"
LOADER_MARK = b"FELUCCA-LOADER-1"
REPO = "https://github.com/bwatanabe/fm1-mutant"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware", type=Path, default=ROOT / "build" / "mutant.fwsc")
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "mutant-site")
    parser.add_argument("--version", default="development")
    args = parser.parse_args()
    data = args.firmware.read_bytes()
    if LOADER_MARK not in data:
        raise SystemExit("Mutant package does not contain the known GATOR/Felucca loader")
    # The FWSC identity is also checked in the browser before the updater runs.
    args.out.mkdir(parents=True, exist_ok=True)
    for name, dest in (("mutant.html", "index.html"), ("mutant.js", "mutant.js"),
                       ("fm1ota.js", "fm1ota.js"), ("fm1pkg.js", "fm1pkg.js")):
        shutil.copyfile(WEB / name, args.out / dest)
    fw = args.out / "firmware"
    fw.mkdir(exist_ok=True)
    name = "mutant.fwsc"
    shutil.copyfile(args.firmware, fw / name)
    digest = hashlib.sha256(data).hexdigest()
    payload = {
        "source": REPO,
        "packages": [{
            "id": "mutant", "title": "FM-1 Mutant (experimental)",
            "version": args.version, "file": name,
            "size": len(data), "sha256": digest
        }]
    }
    (fw / "manifest.json").write_text(json.dumps(payload, indent=2) + "\n")
    (fw / "SHA256SUMS").write_text(f"{digest}  {name}\n")
    (args.out / ".nojekyll").write_text("")
    (args.out / "ABOUT.txt").write_text(
        "FM-1 Mutant preview build. Same compiled image for emulator and device.\n"
        + f"Sources: {REPO}\n"
        + "Includes GPL-3.0-only GATOR platform and MIT-licensed Felucca-Engines DSP.\n"
    )
    print(f"mutant site: {args.out} ({len(data)} bytes, sha256 {digest[:16]}...)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
