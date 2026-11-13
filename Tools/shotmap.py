#!/usr/bin/env python3
"""Draws every shot of Content/Data/deepcut.json as a small map (no engine needed, about a second).

  python3 Tools/shotmap.py            -> Saved/DeepCut/shotmap.svg   (open it in a browser)

Per shot: the screen view at the entry camera (what the player sees, solids drawn as their front faces,
rotation ignored) and a top view (x across, z into the picture). Tape marks of the cameras that cut INTO a shot
are drawn in that shot, in their tape colour. It also prints simple checks: marks inside rock, marks off screen.
"""
import json, math, os, sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
d = json.load(open(os.path.join(ROOT, "Content", "Data", "deepcut.json")))
TAN_H = math.tan(math.radians(d.get("fov", 60) / 2.0)); TAN_V = TAN_H / (16 / 9)
R = d.get("heroRadius", 36)
COL = {"yellow": "#ffd91a", "blue": "#33b8ff", "pink": "#ff59bf", "white": "#f2f2e6"}
shots = d["shots"]; ids = {s["id"]: i for i, s in enumerate(shots)}
W, H, TOPH, PAD = 480, 270, 200, 30
out, warn = [], []


def ndc(s, p):
    t = (s.get("rail") or [0, 0, 0])[2]
    return ((p[0] - t) / (p[2] * TAN_H), p[1] / (p[2] * TAN_V))


def blocked(s, p):
    for b in s.get("solids", []):
        if b.get("r") or b.get("hatch"):
            continue
        c, z = b["p"], b["s"]
        dd = [max(abs(p[k] - c[k]) - z[k] / 2, 0) for k in range(3)]
        if math.sqrt(sum(v * v for v in dd)) < R:
            return True
    return False


for i, s in enumerate(shots):
    ox, oy = PAD + (i % 3) * (W + PAD), PAD + (i // 3) * (H + TOPH + 70)
    out.append(f'<text x="{ox}" y="{oy - 8}" fill="#ccc" font-size="14">{i} {s["name"]}  gate: {s.get("gate", "-")}{"  DARK" if s.get("dark") else ""}</text>')
    out.append(f'<rect x="{ox}" y="{oy}" width="{W}" height="{H}" fill="#0b2226" stroke="#555"/>')
    out.append(f'<rect x="{ox}" y="{oy + H * 0.125}" width="{W}" height="{H * 0.75}" fill="none" stroke="#333"/>')

    def px(p):
        n = ndc(s, p); return ox + (n[0] + 1) * W / 2, oy + (1 - n[1]) * H / 2
    for b in sorted(s.get("solids", []), key=lambda b: -b["p"][2]):
        c, z = b["p"], b["s"]; zf = max(c[2] - z[2] / 2, 300)
        x0, y0 = px([c[0] - z[0] / 2, c[1] + z[1] / 2, zf]); x1, y1 = px([c[0] + z[0] / 2, c[1] - z[1] / 2, zf])
        shade = int(max(40, 130 - (zf - 800) / 12))
        out.append(f'<rect x="{max(x0, ox):.0f}" y="{max(y0, oy):.0f}" width="{max(min(x1, ox + W) - max(x0, ox), 0):.0f}" height="{max(min(y1, oy + H) - max(y0, oy), 0):.0f}" fill="rgb({shade},{shade},{shade})" opacity="0.8"/>')
    for a in shots:
        for e in a.get("exits", []):
            if e["to"] != s["id"]:
                continue
            for m in e["marks"]:
                x, y = px(m); c = COL.get(e.get("color", "yellow"))
                out.append(f'<path d="M{x - 9:.0f} {y - 9:.0f} L{x + 9:.0f} {y + 9:.0f} M{x - 9:.0f} {y + 9:.0f} L{x + 9:.0f} {y - 9:.0f}" stroke="{c}" stroke-width="4"/>')
                out.append(f'<text x="{x + 12:.0f}" y="{y + 4:.0f}" fill="{c}" font-size="10">from {a["id"]} z{m[2]:.0f}{"" if not e.get("anyDepth") else " any"}</text>')
                n = ndc(s, m)
                if abs(n[0]) > 0.93 or abs(n[1]) > 0.64: warn.append(f'{a["id"]}->{s["id"]}: mark {m} is outside the swim frame')
                if blocked(s, m): warn.append(f'{a["id"]}->{s["id"]}: mark {m} is inside rock')
    for k, col in (("props", "#7fd"), ("sockets", "#fb3")):
        for p in s.get(k, []):
            x, y = px(p["p"]); out.append(f'<rect x="{x - 7:.0f}" y="{y - 7:.0f}" width="14" height="14" fill="none" stroke="{col}" stroke-width="2"/><text x="{x + 10:.0f}" y="{y + 4:.0f}" fill="{col}" font-size="10">{p["id"]}</text>')
    for k, col in (("switch", "#f43"), ("part", "#ff8"), ("crew", "#fa0"), ("start", "#fff")):
        if k in s:
            x, y = px(s[k]); out.append(f'<circle cx="{x:.0f}" cy="{y:.0f}" r="6" fill="none" stroke="{col}" stroke-width="2"/><text x="{x + 9:.0f}" y="{y - 6:.0f}" fill="{col}" font-size="10">{k}</text>')
    # top view
    ty = oy + H + 16; vmin, vmax = s.get("volMin", [-900, -400, 650]), s.get("volMax", [900, 400, 1400])
    xa, xb, za, zb = vmin[0] - 200, vmax[0] + 200, 600, vmax[2] + 300

    def tp(x, z): return ox + (x - xa) / (xb - xa) * W, ty + TOPH - (z - za) / (zb - za) * TOPH
    out.append(f'<rect x="{ox}" y="{ty}" width="{W}" height="{TOPH}" fill="#0a1416" stroke="#555"/>')
    for b in s.get("solids", []):
        c, z = b["p"], b["s"]
        if z[1] > 250 and c[1] < -300: continue  # the floor
        x0, y0 = tp(c[0] - z[0] / 2, c[2] + z[2] / 2); x1, y1 = tp(c[0] + z[0] / 2, c[2] - z[2] / 2)
        out.append(f'<rect x="{max(x0, ox):.0f}" y="{max(y0, ty):.0f}" width="{max(min(x1, ox + W) - max(x0, ox), 0):.0f}" height="{max(min(y1, ty + TOPH) - max(y0, ty), 0):.0f}" fill="#777" opacity="0.7"/>')
    x0, y0 = tp(vmin[0], vmax[2]); x1, y1 = tp(vmax[0], vmin[2])
    out.append(f'<rect x="{x0:.0f}" y="{y0:.0f}" width="{x1 - x0:.0f}" height="{y1 - y0:.0f}" fill="none" stroke="#2a8" stroke-dasharray="4 3"/>')
    for c in s.get("currents", []):
        x0, y0 = tp(c["p"][0] - c["s"][0] / 2, c["p"][2] + c["s"][2] / 2); x1, y1 = tp(c["p"][0] + c["s"][0] / 2, c["p"][2] - c["s"][2] / 2)
        out.append(f'<rect x="{x0:.0f}" y="{max(y0, ty):.0f}" width="{x1 - x0:.0f}" height="{min(y1, ty + TOPH) - max(y0, ty):.0f}" fill="#9ff" opacity="0.25"/>')
    for a in shots:
        for e in a.get("exits", []):
            if e["to"] == s["id"]:
                for m in e["marks"]:
                    x, y = tp(m[0], m[2]); out.append(f'<circle cx="{x:.0f}" cy="{y:.0f}" r="5" fill="{COL.get(e.get("color", "yellow"))}"/>')
    for k, col in (("props", "#7fd"), ("sockets", "#fb3")):
        for p in s.get(k, []):
            x, y = tp(p["p"][0], p["p"][2]); out.append(f'<rect x="{x - 5:.0f}" y="{y - 5:.0f}" width="10" height="10" fill="none" stroke="{col}" stroke-width="2"/>')
    out.append(f'<text x="{ox + 4}" y="{ty + 14}" fill="#888" font-size="10">top view: far</text><text x="{ox + 4}" y="{ty + TOPH - 5}" fill="#888" font-size="10">camera side</text>')
    for e in s.get("exits", []):
        if e["to"] not in ids: warn.append(f'{s["id"]}: exit to unknown shot {e["to"]}')

rows = (len(shots) + 2) // 3
svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="{PAD + 3 * (W + PAD)}" height="{PAD + rows * (H + TOPH + 70)}" style="background:#111;font-family:monospace">' + "".join(out) + "</svg>"
dst = os.path.join(ROOT, "Saved", "DeepCut"); os.makedirs(dst, exist_ok=True)
open(os.path.join(dst, "shotmap.svg"), "w").write(svg)
print("wrote", os.path.normpath(os.path.join(dst, "shotmap.svg")))
print("\n".join("WARN " + w for w in warn) if warn else "checks: no mark inside rock or off frame")
