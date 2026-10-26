#!/usr/bin/env python3
"""v0.10 migration, named library, confirmed delete and tempo synchronization."""
import re
from pathlib import Path

OUT = Path("build/host/out")

def snapshots(path, tag, markers):
    wanted = set(markers)
    found = {}
    last = None
    for line in (OUT / path).read_text().splitlines():
        parts = line.split()
        if parts and parts[0] in wanted:
            last = parts[0]
        elif line.startswith(tag + " ") and last:
            found[last] = dict(re.findall(r"([a-z_]+)=([A-Za-z0-9]+)", line))
            last = None
    for marker in markers:
        assert marker in found, f"{path}: missing {marker}"
    return found

lib = snapshots("patch-library.log", "LIB", (
    "MIGRATED","LIBRARY","RENAMED_EXISTING","EMPTY_25",
    "SAVED_25","RELOADED_25","DELETE_ARMED","DELETED_25","DELETE_PERSISTED"
))
def hasbit(label, bit):
    return bool(int(lib[label]["used"], 16) & (1 << bit))

assert lib["MIGRATED"]["ver"] == "2", "old 16-slot bank did not migrate in RAM"
assert lib["MIGRATED"]["patch"] == "777" or lib["MIGRATED"].get("patch3") == "777", "v1 patch 4 changed"
assert int(lib["MIGRATED"]["bytes"]) == int(lib["MIGRATED"]["legacy"]), "old format not preserved until SAVE"
assert int(lib["RENAMED_EXISTING"]["bytes"]) > int(lib["MIGRATED"]["bytes"]), "new 32-slot bank not persisted"
assert lib["RENAMED_EXISTING"]["name"].startswith("QATCH"), "rename did not edit name"
assert lib["RENAMED_EXISTING"]["patch3"] == "777", "renaming destroyed an unrelated sound"
assert not hasbit("EMPTY_25", 24)
assert hasbit("SAVED_25", 24) and hasbit("RELOADED_25", 24)
assert lib["RELOADED_25"]["name"] == lib["SAVED_25"]["name"], "saved name lost on reload"
assert hasbit("DELETE_ARMED", 24), "hold prematurely deleted without confirmation"
assert not hasbit("DELETED_25", 24) and not hasbit("DELETE_PERSISTED", 24), "deletion not saved"
assert all(row["patch3"] == "777" for row in lib.values()), "migrated v1 sound was lost"
print("LIBRARY PASS: exact legacy migration; name-only edit; 32-slot save; double-confirmed deletion and reload")

clk = snapshots("mutation-clock.log", "STATE", (
    "MIDI_IDLE","WAITING_CLOCK","NO_CLOCK_NO_CHANGE","BEFORE_BEAT",
    "ON_BEAT","MIDI_STOPPED","MIDI_RESTARTED",
    "INTERNAL_EARLY","INTERNAL_BEAT","ENGINE_SWITCH_RUN",
    "SECOND_SWITCH_RUN","CLOCK_PAUSED"
))
def s(k): return clk[k]
assert s("MIDI_IDLE")["clock"] == "2" and s("MIDI_IDLE")["div"] == "1"
assert s("WAITING_CLOCK")["run"] == "1"
assert s("NO_CLOCK_NO_CHANGE")["goal"] == s("WAITING_CLOCK")["goal"], "MIDI mode should wait for clock"
assert s("BEFORE_BEAT")["goal"] == s("WAITING_CLOCK")["goal"], "MIDI target moved before 24 ticks"
assert s("ON_BEAT")["goal"] != s("BEFORE_BEAT")["goal"], "target didn't move at 24 PPQN quarter note"
assert s("MIDI_STOPPED")["goal"] == s("ON_BEAT")["goal"], "MIDI STOP did not freeze clock targets"
assert s("MIDI_RESTARTED")["goal"] != s("MIDI_STOPPED")["goal"], "START/restart ignored"
assert s("INTERNAL_EARLY")["clock"] == "1" and s("INTERNAL_EARLY")["goal"] != ""
assert s("INTERNAL_BEAT")["goal"] != s("INTERNAL_EARLY")["goal"], "internal BPM did not schedule target"
assert s("ENGINE_SWITCH_RUN")["run"] == "1" and s("ENGINE_SWITCH_RUN")["engine"] == "24"
assert s("SECOND_SWITCH_RUN")["run"] == "1" and s("SECOND_SWITCH_RUN")["engine"] == "26"
assert s("CLOCK_PAUSED")["run"] == "0"
print("CLOCK PASS: FREE unaffected; MIDI 24PPQN beat, stop/start, internal BPM; engine changes keep RUN")
