#!/usr/bin/env python3
"""Offline mirror of the underwater cut rule (brief 02). Reads Content/Data/deepcut.json.

    python3 Tools/cutmap.py            ASCII map for every shot pair
    python3 Tools/cutmap.py --alpha 1  with drifting plates at the far end of their travel (0..1)

For each pair A -> B it floods the water the robot can reach in A from where it arrives, and tests the cut
from every reachable spot. Legend:
    #  rock in A                 o  reachable in A and the cut to B works
    .  reachable, cut fails      (space) water in A the robot cannot reach
    S  start mark of A           P part, C crew robot (in A)
The rule mirrored here (see MatchCutGameMode.cpp, EvalCut): same screen position, same distance to the camera
clamped to B's depth range, fail if outside B's swim rectangle or if the robot's sphere touches a rock of B.
Rocks are boxes facing the camera, so everything is 2D in camera-plane units.
"""
import json
import math
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = json.load(open(os.path.join(ROOT, "Content", "Data", "deepcut.json")))
FOV = DATA.get("fov", 60)
TAN_H = math.tan(math.radians(FOV) / 2)
TAN_V = TAN_H / (16 / 9)
RADIUS = DATA.get("heroRadius", 36)
PICTURE = 0.75
DEFAULT_SWIM = [-0.95, -0.68, 0.95, 0.68]
W, H = 96, 30


def rocks_world(shot, alpha):
    out = []
    ref = shot.get("ref", sum(shot["depth"]) / 2)
    for rock in shot.get("rocks", []):
        x0, y0, x1, y1 = rock["r"]
        if "move" in rock:
            dx, dy = rock["move"]
            x0, x1, y0, y1 = x0 + dx * alpha, x1 + dx * alpha, y0 + dy * alpha, y1 + dy * alpha
        out.append((x0 * TAN_H * ref, y0 * TAN_V * ref, x1 * TAN_H * ref, y1 * TAN_V * ref))
    return out


def blocked(shot, ndc, depth, alpha):
    px, py = ndc[0] * TAN_H * depth, ndc[1] * TAN_V * depth
    for x0, y0, x1, y1 in rocks_world(shot, alpha):
        cx, cy = min(max(px, x0), x1), min(max(py, y0), y1)
        if (px - cx) ** 2 + (py - cy) ** 2 < RADIUS ** 2:
            return True
    return False


def in_swim(shot, ndc):
    s = shot.get("swim", DEFAULT_SWIM)
    return s[0] <= ndc[0] <= s[2] and s[1] <= ndc[1] <= s[3]


def cut_ok(b, ndc, depth, alpha):
    d = min(max(depth, b["depth"][0]), b["depth"][1])
    if abs(ndc[0]) > 1 or abs(ndc[1]) > PICTURE:
        return False
    return in_swim(b, ndc) and not blocked(b, ndc, d, alpha)


def cell(x, y):
    return (-1 + (x + 0.5) * 2 / W, PICTURE - (y + 0.5) * 2 * PICTURE / H)


def nearest(ndc):
    return (min(W - 1, max(0, int((ndc[0] + 1) / 2 * W))), min(H - 1, max(0, int((PICTURE - ndc[1]) / (2 * PICTURE) * H))))


def main():
    alpha = 0.0
    if "--alpha" in sys.argv:
        alpha = float(sys.argv[sys.argv.index("--alpha") + 1])
    shots = DATA["shots"]
    depth = DATA.get("startDepth", 900)
    arrive = None  # cells the robot can arrive in (None = the start mark)
    for i, a in enumerate(shots):
        depth = min(max(depth, a["depth"][0]), a["depth"][1])
        free = [[in_swim(a, cell(x, y)) and not blocked(a, cell(x, y), depth, alpha) for x in range(W)] for y in range(H)]
        seeds = [nearest(a["start"])] if arrive is None else list(arrive) + [nearest(a["start"])]
        reach = [[False] * W for _ in range(H)]
        stack = [s for s in seeds if free[s[1]][s[0]]]
        for s in stack:
            reach[s[1]][s[0]] = True
        while stack:
            x, y = stack.pop()
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < W and 0 <= ny < H and free[ny][nx] and not reach[ny][nx]:
                    reach[ny][nx] = True
                    stack.append((nx, ny))
        b = shots[i + 1] if i + 1 < len(shots) else None
        good = set()
        rows = []
        marks = {nearest(a["start"]): "S"}
        if "part" in a:
            marks[nearest(a["part"])] = "P"
        if "crew" in a:
            marks[nearest(a["crew"])] = "C"
        n_reach = 0
        for y in range(H):
            row = ""
            for x in range(W):
                ch = "#"
                if free[y][x]:
                    ch = " "
                    if reach[y][x]:
                        n_reach += 1
                        ch = "."
                        if b and cut_ok(b, cell(x, y), depth, alpha):
                            ch = "o"
                            good.add((x, y))
                ch = marks.get((x, y), ch)
                row += ch
            rows.append(row)
        title = "shot %d %s (depth %d%s)" % (i, a["name"], depth, ", dark" if a.get("dark") else "")
        if b:
            title += "  ->  shot %d %s%s: cut works from %.1f %% of the reachable water (plates at alpha %.2f)" % (
                i + 1, b["name"], " (dark: lamp needed)" if b.get("dark") else "", 100.0 * len(good) / max(n_reach, 1), alpha)
        print(title)
        print("+" + "-" * W + "+")
        for row in rows:
            print("|" + row + "|")
        print("+" + "-" * W + "+")
        if "part" in a:
            t = DATA.get("restore", [0, 0])
            c = nearest(t)
            print("  restore mark at (%.2f, %.2f): %s" % (t[0], t[1], "reachable" if reach[c[1]][c[0]] else "NOT REACHABLE"))
        if "crew" in a:
            c = a["crew"]
            near = any(reach[y][x] and math.hypot((cell(x, y)[0] - c[0]) * TAN_H * depth, (cell(x, y)[1] - c[1]) * TAN_V * depth) < RADIUS * 2 + 40
                       for y in range(H) for x in range(W))
            print("  crew robot at (%.2f, %.2f): %s" % (c[0], c[1], "can be touched" if near else "CANNOT BE TOUCHED"))
        if b and not good:
            print("  !! no cut possible at this plate position")
        print()
        arrive = good


if __name__ == "__main__":
    main()
