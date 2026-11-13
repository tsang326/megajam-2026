#!/usr/bin/env python3
"""Writes Content/Data/tests/full12.txt from stage1.txt and stage2.txt, so the three scripts cannot drift apart."""
import os
T = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Content", "Data", "tests")
s1 = open(os.path.join(T, "stage1.txt")).read().split("\n")
s2 = open(os.path.join(T, "stage2.txt")).read().split("\n")
head = [
    "# Stages 1 and 2 in one run: the stage1 script, C on the end card, then the stage2 script. Made by Tools/make_full12.py.",
    "# Editor:   Tools/run_offscreen.sh full12 Saved/DeepCut/full12",
    "# Packaged: Tools/run_packaged.sh full12 Saved/DeepCut/full12_pkg",
]
body1 = s1[3:]
while body1 and body1[-1].strip() in ("", "quit", "stat"):
    body1.pop()
link = ["# --- C on the end card: stage 2", "wait 1.0", "snap s5e_end_card_c_next_stage", "next"]
start = next(i for i, l in enumerate(s2) if l.startswith("skipto")) + 1
open(os.path.join(T, "full12.txt"), "w").write("\n".join(head + body1 + link + s2[start:]).rstrip("\n") + "\n")
print("full12.txt:", len(head + body1 + link + s2[start:]), "lines")
