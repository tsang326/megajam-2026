#!/usr/bin/env python3
"""Offline mirror of the match-cut rule, for designing shots without launching Unreal.

Reads Content/MatchCut/shots.json (the same file the game loads) and, for every
shot pair A -> B, prints where on A's surfaces a cut succeeds. Optionally writes
small PNG previews of each shot with the next shot's ghost added on top.

    python3 Tools/cutmap.py                 # ASCII cut maps
    python3 Tools/cutmap.py --png out_dir   # also write previews
    python3 Tools/cutmap.py --t 1.0         # mover clock (seconds)

Pure standard library. The maths must stay identical to AMatchCutGameMode
(ProjectToShot / DeprojectFromShot / TryCut); the engine log line "CUT ..." is
the ground truth if the two ever disagree.
"""
import json
import math
import os
import struct
import sys
import zlib

ASPECT = 16.0 / 9.0
HERE = os.path.dirname(os.path.abspath(__file__))
LAYOUT = os.path.join(HERE, "..", "Content", "MatchCut", "shots.json")


def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def mul(a, k): return (a[0] * k, a[1] * k, a[2] * k)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def norm(a):
    l = math.sqrt(dot(a, a))
    return (a[0] / l, a[1] / l, a[2] / l)


def cam_axes(shot):
    """Forward/right/up of a look-at camera with no roll (Unreal axes: X fwd, Y right, Z up)."""
    f = norm(sub(shot["cam"]["look"], shot["cam"]["pos"]))
    yaw = math.atan2(f[1], f[0])
    pitch = math.asin(f[2])
    r = (-math.sin(yaw), math.cos(yaw), 0.0)
    u = (-math.sin(pitch) * math.cos(yaw), -math.sin(pitch) * math.sin(yaw), math.cos(pitch))
    return f, r, u


def project(shot, fov, p):
    f, r, u = cam_axes(shot)
    d = sub(p, shot["cam"]["pos"])
    depth = dot(d, f)
    if depth <= 1.0:
        return None
    th = math.tan(math.radians(fov) / 2)
    tv = th / ASPECT
    return (dot(d, r) / depth / th, dot(d, u) / depth / tv)


def ray(shot, fov, ndc):
    f, r, u = cam_axes(shot)
    th = math.tan(math.radians(fov) / 2)
    tv = th / ASPECT
    d = add(add(f, mul(r, ndc[0] * th)), mul(u, ndc[1] * tv))
    return shot["cam"]["pos"], norm(d)


def box_center(box, t):
    c = box["c"]
    mv = box.get("move")
    if mv:
        a = 0.5 - 0.5 * math.cos(2 * math.pi * t / max(mv.get("period", 4.0), 0.1))
        c = add(c, mul(mv["to"], a))
    return c


def trace(shot, o, d, t):
    """First box hit: (distance, box, axis, sign) or None."""
    best = None
    for box in shot["boxes"]:
        c = box_center(box, t)
        lo = [c[i] - box["s"][i] / 2 for i in range(3)]
        hi = [c[i] + box["s"][i] / 2 for i in range(3)]
        tmin, tmax, axis, sign = -1e30, 1e30, -1, 0
        ok = True
        for i in range(3):
            if abs(d[i]) < 1e-12:
                if o[i] < lo[i] or o[i] > hi[i]:
                    ok = False
                    break
                continue
            t1 = (lo[i] - o[i]) / d[i]
            t2 = (hi[i] - o[i]) / d[i]
            s = -1
            if t1 > t2:
                t1, t2 = t2, t1
                s = 1
            if t1 > tmin:
                tmin, axis, sign = t1, i, s
            tmax = min(tmax, t2)
        if not ok or tmin > tmax or tmin < 0:
            continue
        if best is None or tmin < best[0]:
            best = (tmin, box, axis, sign)
    return best


def cut(shots, fov, a, b, feet, t):
    """Returns (code, landing point or None). Codes: digit = index of the box landed on, M mover, G goal, o out of frame, . void, x untagged, w wall."""
    ndc = project(shots[a], fov, feet)
    if ndc is None or abs(ndc[0]) > 1 or abs(ndc[1]) > 1:
        return "o", None
    o, d = ray(shots[b], fov, ndc)
    hit = trace(shots[b], o, d, t)
    if hit is None:
        return ".", None
    dist, box, axis, sign = hit
    if not box.get("cut"):
        return "x", None
    if not (axis == 2 and sign > 0):
        return "w", None
    code = "G" if box.get("goal") else ("M" if box.get("move") else str(shots[b]["boxes"].index(box)))
    return code, add(o, mul(d, dist))


def write_png(path, w, h, rows):
    raw = b"".join(b"\x00" + bytes(row) for row in rows)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as fh:
        fh.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def render(shot, fov, t, w=320, h=180):
    img = []
    for y in range(h):
        row = []
        for x in range(w):
            ndc = ((x + 0.5) / w * 2 - 1, 1 - (y + 0.5) / h * 2)
            o, d = ray(shot, fov, ndc)
            hit = trace(shot, o, d, t)
            if hit is None:
                row.append((0, 0, 0))
                continue
            _, box, axis, sign = hit
            base = (40, 90, 40) if box.get("goal") else ((200, 110, 30) if box.get("cut") else (70, 72, 80))
            shade = 1.0 if axis == 2 else (0.7 if axis == 1 else 0.5)
            row.append(tuple(int(c * shade) for c in base))
        img.append(row)
    return img


def main():
    args = sys.argv[1:]
    t = float(args[args.index("--t") + 1]) if "--t" in args else 0.0
    png = args[args.index("--png") + 1] if "--png" in args else None
    step = float(args[args.index("--step") + 1]) if "--step" in args else 50.0

    with open(LAYOUT) as fh:
        data = json.load(fh)
    shots, fov = data["shots"], data.get("fov", 60.0)

    for i, shot in enumerate(shots):
        ndc = project(shot, fov, shot["start"])
        print("shot %d %-12s start ndc=%s" % (i, shot["name"], None if ndc is None else "(%.2f,%.2f)" % ndc))

    for a in range(len(shots) - 1):
        b = a + 1
        print("\n=== cut %d -> %d (%s -> %s), mover clock %.2fs; rows = +X (far) at top, columns = Y ===" % (a, b, shots[a]["name"], shots[b]["name"], t))
        print("    0-9 lands on that box of the next shot   M lands on mover   G lands on goal   . void   x untagged   w wall   o out of frame")
        for bi, box in enumerate(shots[a]["boxes"]):
            c = box_center(box, t)
            sx, sy, sz = box["s"]
            top = c[2] + sz / 2
            nx, ny = max(1, int(sx // step)), max(1, int(sy // step))
            print("  box %d centre=%s size=%s top z=%.0f" % (bi, [round(v) for v in c], box["s"], top))
            for ix in reversed(range(nx)):
                x = c[0] - sx / 2 + (ix + 0.5) * sx / nx
                line = ""
                for iy in range(ny):
                    y = c[1] - sy / 2 + (iy + 0.5) * sy / ny
                    line += cut(shots, fov, a, b, (x, y, top), t)[0]
                print("    x=%6.0f  %s" % (x, line))

    if png:
        os.makedirs(png, exist_ok=True)
        views = [render(s, fov, t) for s in shots]
        for i, img in enumerate(views):
            rows = []
            for y, row in enumerate(img):
                out = []
                for x, px in enumerate(row):
                    if i + 1 < len(views):
                        g = views[i + 1][y][x]
                        px = tuple(min(255, int(px[k] + g[k] * 0.45)) for k in range(3))
                    out.extend(px)
                rows.append(out)
            # mark the start position
            ndc = project(shots[i], fov, shots[i]["start"])
            if ndc:
                px, py = int((ndc[0] + 1) / 2 * 320), int((1 - ndc[1]) / 2 * 180)
                for dy in range(-2, 3):
                    for dx in range(-2, 3):
                        if 0 <= px + dx < 320 and 0 <= py + dy < 180:
                            rows[py + dy][(px + dx) * 3:(px + dx) * 3 + 3] = [255, 255, 255]
            write_png(os.path.join(png, "shot%d.png" % i), 320, 180, rows)
        print("\npreviews written to", png)


if __name__ == "__main__":
    main()
