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
        "MUTANT_CONTROLS", "MUTANT_ADJUSTED", "MUTANT_RUNNING", "MUTANT_EVOLVED", "MUTANT_CAPTURE",
        "MUTANT_LATER", "MUTANT_RESTORED", "RESTORE_RELEASED",
        "GLO_LONG", "GLO_LONG_RELEASE", "DEVICE_PAGE", "DEVICE_CHANGED", "DEVICE_SAVED", "DEVICE_RELOADED",
        "CAP_CLEARED_HELD", "CAP_CLEARED_RELEASED", "STUDIO_AGAIN"
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
adjusted = entry("MUTANT_ADJUSTED")
assert adjusted["page"] == "8", "adjusted page disappeared"
assert adjusted["depth"] != controls["depth"], "depth knob not responding"
assert adjusted["speed"] != controls["speed"], "speed knob not responding"
assert adjusted["instability"] != controls["instability"], "instability knob not responding"
assert adjusted["style"] != controls["style"], "style knob not responding"
running, evolved = entry("MUTANT_RUNNING"), entry("MUTANT_EVOLVED")
assert running["run"] == "1" and evolved["run"] == "1"
assert evolved["hash"] != running["hash"], "autonomous mutation did not move"
capture, later, restored = (entry(x) for x in ("MUTANT_CAPTURE","MUTANT_LATER","MUTANT_RESTORED"))
assert capture["capture"] == "1" and capture["run"] == "1", "REC short did not capture running sound"
assert later["hash"] != capture["hash"], "motion stopped after REC capture"
assert restored["run"] == "0" and restored["hash"] == capture["caphash"], "REC hold did not restore exact captured sound state and stop"
assert entry("RESTORE_RELEASED")["hash"] == capture["caphash"], "REC long also triggered a short capture"
assert entry("GLO_LONG")["machine"] == "MUTANT" and entry("GLO_LONG")["page"] == "6", "GLO long did not open Global"
assert entry("GLO_LONG_RELEASE")["machine"] == "MUTANT", "GLO long mistakenly toggled mode"
assert entry("CAP_CLEARED_HELD")["capture"] == "0"
assert entry("CAP_CLEARED_RELEASED")["capture"] == "0"
assert entry("DEVICE_PAGE")["page"] == "9", "second GLO hold didn't open DEVICE"
assert entry("DEVICE_CHANGED")["glow"] == "3", "BUTTON LIGHT level did not change"
assert entry("DEVICE_CHANGED")["midich"] == "15", "MIDI channel control did not change"
assert entry("DEVICE_SAVED")["page"] == "9", "SAVE from device page should not open preset SAVE"
assert entry("DEVICE_RELOADED")["midich"] == "15" and entry("DEVICE_RELOADED")["glow"] == "3", "device config did not survive host simulated reload"
assert entry("STUDIO_AGAIN")["machine"] == "STUDIO"
print("STUDIO/MUTANT control regression PASS:")
print("- two unrestricted randomizations and one bounded mutation produce distinct patches")
print("- saved bank remains unchanged")
print("- GLO tap/hold switches machines or opens Global correctly")
print("- SEQ opens mutation page, PLAY starts motion")
print("- REC short captures; medium hold restores; long hold clears")
print("- GLO hold opens DEVICE; LED level and channel controls respond")

if len(sys.argv) > 2:
    midi_labels = {
        "SELECTED_MIDI15", "WRONG_CHANNEL", "RIGHT_CHANNEL",
        "MIDI_RUNNING", "MIDI_CAPTURE", "MIDI_CAPTURE_NO_REPEAT",
        "MIDI_RESTORED", "MIDI_CLEARED", "MIDI_STUDIO"
    }
    states, label = {}, None
    for line in Path(sys.argv[2]).read_text().splitlines():
        if line.split() and line.split()[0] in midi_labels:
            label = line.split()[0]
        elif line.startswith("STATE ") and label:
            states[label] = dict(re.findall(r"([a-z_]+)=([A-Za-z0-9]+)", line))
    for key in midi_labels:
        assert key in states, f"missing MIDI test marker: {key}"
    at = lambda label: states[label]
    assert at("SELECTED_MIDI15")["midich"] == "15"
    assert at("WRONG_CHANNEL")["depth"] == "45", "wrong MIDI channel changed DEPTH"
    assert at("RIGHT_CHANNEL")["depth"] == "78", "right MIDI channel didn't set DEPTH"
    assert at("RIGHT_CHANNEL")["style"] == "2", "right MIDI channel didn't set STYLE"
    assert at("MIDI_RUNNING")["machine"] == "MUTANT" and at("MIDI_RUNNING")["run"] == "1"
    assert at("MIDI_CAPTURE")["capture"] == "1"
    assert at("MIDI_CAPTURE_NO_REPEAT")["caphash"] == at("MIDI_CAPTURE")["caphash"], "CC46 retriggered without falling edge"
    assert at("MIDI_RESTORED")["run"] == "0" and at("MIDI_RESTORED")["hash"] == at("MIDI_CAPTURE")["caphash"]
    assert at("MIDI_CLEARED")["capture"] == "0"
    assert at("MIDI_STUDIO")["machine"] == "STUDIO"
    print("- MIDI channel filtering, OP-XY CC40-49, run/capture/restore/clear verified")

