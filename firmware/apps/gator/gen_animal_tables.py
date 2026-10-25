#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Small ANIMAL table subset, formulas from JIANT tools/gen_tables.py.

JIANT commit f2fd85865ae4910a85069e848dd9f92bf2645996.
The sample-rate dependency is explicit (Mutant normally runs at 48kHz).
"""
import argparse
import math
from pathlib import Path


def table(name, values):
    rows = [f"static const uint32_t {name}[{len(values)}] = {{"]
    for i in range(0, len(values), 8):
        rows.append("    " + ", ".join(f"{v}u" for v in values[i:i + 8]) + ",")
    rows.append("};")
    return "\n".join(rows)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fs", type=int, required=True)
    ap.add_argument("out", type=Path)
    args = ap.parse_args()
    if not 32000 <= args.fs <= 96000:
        ap.error("sample rate must be between 32000 and 96000 Hz")
    fs = args.fs
    lfo = [int((0.05 * (800 ** (v / 127))) * 32 / fs * 2**32) for v in range(128)]
    exp2 = [int(round((1 << 30) * (2 ** (i / 1024) - 1))) for i in range(1025)]
    contents = ("/* ANIMAL tables generated from JIANT's original formulas. */\n"
                "#ifndef MUTANT_ANIMAL_TABLES_H\n#define MUTANT_ANIMAL_TABLES_H\n"
                + table("LFO_INC", lfo) + "\n"
                + table("FM6_EXP2", exp2) + "\n#endif\n")
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(contents)


if __name__ == "__main__":
    main()
