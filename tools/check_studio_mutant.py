#!/usr/bin/env python3
"""Regression checks for two-machine Mutant v0.9 behavior.

Reads host/sim.c STATE snapshots. No special MIDI hardware required.
"""
import re
import sys
from pathlib import Path

path = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/out/studio-mutant.log")
snapshots = {}
label = None
for line in path.read_text().splitlines():
    # host/sim.c's echo command prints the arguments WITHOUT the "echo"
    # prefix. Test labels are unambiguous, uppercase single-word markers.
    if line.split() and line.split()[0] in {
        "STUDIO_BASE", "RANDOM1", "RANDOM2", "STUDIO_STEP", "MUTANT_IDLE",
        "MUTANT_CONTROLS", "MUTANT_RUNNING", "MUTANT_EVOLVED", "MUTANT_CAPTURE",
        "MUTANT_LATER", "MUTANT_RESTORED", "RESTORE_RELEASED",
        "GLO_LONG", "GLO_LONG_RELEASE", "STUDIO_AGAIN"
    }:
        label = line.split()[0]
    if line.startswith("STATE "):
        assert label, "state line without label"
        snapshots[label] = dict(re.findall(r"([a-z_]+)=([A-Za-z0-9]+)", line))

def entry(label):
    assert label in snapshots, f"missing test state {label}"
    return snapshots[label]

initial = entry("STUDIO_BASE")
random1, random2, step = (entry(x) for x in ("RANDOM1", "RANDOM2", "STUDIO_STEP"))
assert initial["machine"] == "STUDIO"
assert random1["engine"] == random2["engine"] == initial["engine"], "RANDOM unexpectedly switched engines"
assert len({initial["hash"], random1["hash"], random2["hash"], step["hash"]}) == 4, "RANDOM/MUTATE did not change the sound"
assert all(s["bank"] == initial["bank"] for s in snapshots.values()), "saved preset bank was mutated"
idle, controls = entry("MUTANT_IDLE"), entry("MUTANT_CONTROLS")
assert idle["machine"] == "MUTANT" and idle["run"] == "0" and idle["page"] == "0"
assert controls["page"] == "8", "SEQ did not open the mutation controls"
running, evolved = entry("MUTANT_RUNNING"), entry("MUTANT_EVOLVED")
assert running["run"] == "1" and evolved["run"] == "1"
assert evolved["hash"] != running["hash"], "autonomous mutation did not move"
capture, later, restored = (entry(x) for x in ("MUTANT_CAPTURE","MUTANT_LATER","MUTANT_RESTORED"))
assert capture["capture"] == "1" and capture["run"] == "1", "REC short did not capture running sound"
assert later["hash"] != capture["hash"], "motion stopped after REC capture"
assert restored["run"] == "0" and restored["hash"] == capture["hash"], "REC hold did not restore exact sound state and stop"
assert entry("RESTORE_RELEASED")["hash"] == capture["hash"], "REC long also triggered a short capture"
assert entry("GLO_LONG")["machine"] == "MUTANT" and entry("GLO_LONG")["page"] == "6", "GLO long did not open Global"
assert entry("GLO_LONG_RELEASE")["machine"] == "MUTANT", "GLO long mistakenly toggled mode"
assert entry("STUDIO_AGAIN")["machine"] == "STUDIO"
print("STUDIO/MUTANT control regression PASS:")
print("- two unrestricted randomizations and one bounded mutation produce distinct patches")
print("- saved bank remains unchanged")
print("- GLO tap/hold switches machines or opens Global correctly")
print("- SEQ opens mutation page, PLAY starts motion")
print("- REC short captures while evolution continues; REC hold restores exact captured sound")
