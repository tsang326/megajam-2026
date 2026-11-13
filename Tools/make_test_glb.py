#!/usr/bin/env python3
"""Writes one small GLB (a rounded box, orange, Y up, metres) and a manifest next to it, to test the art pipeline
before real models arrive. No engine and no packages needed.

    python3 Tools/make_test_glb.py        -> SourceAssets/art_test/testbox.glb + manifest.json
"""
import json, math, os, struct

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceAssets", "art_test")
HX, HY, HZ, R, N = 0.65, 0.12, 0.75, 0.08, 8   # half sizes, corner radius, grid per face

pos, nor, idx = [], [], []
for axis in range(3):
    for sign in (-1.0, 1.0):
        base = len(pos)
        for i in range(N + 1):
            for j in range(N + 1):
                u, v = i / N * 2 - 1, j / N * 2 - 1
                p = [0.0, 0.0, 0.0]
                p[axis], p[(axis + 1) % 3], p[(axis + 2) % 3] = sign, u, v
                full = [p[0] * HX, p[1] * HY, p[2] * HZ]
                inner = [max(-h + R, min(h - R, c)) for c, h in zip(full, (HX, HY, HZ))]
                d = [a - b for a, b in zip(full, inner)]
                l = math.sqrt(sum(c * c for c in d)) or 1.0
                n = [c / l for c in d]
                pos.append([a + b * R for a, b in zip(inner, n)])
                nor.append(n)
        for i in range(N):
            for j in range(N):
                a, b, c, d = base + i * (N + 1) + j, base + (i + 1) * (N + 1) + j, base + (i + 1) * (N + 1) + j + 1, base + i * (N + 1) + j + 1
                idx += [a, b, c, a, c, d] if sign > 0 else [a, c, b, a, d, c]

pb = b"".join(struct.pack("<3f", *p) for p in pos)
nb = b"".join(struct.pack("<3f", *n) for n in nor)
ib = struct.pack("<%dH" % len(idx), *idx)
ib += b"\0" * (-len(ib) % 4)
blob = pb + nb + ib
doc = {
    "asset": {"version": "2.0", "generator": "Deep Cut test box"},
    "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": "testbox"}],
    "meshes": [{"name": "testbox", "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}, "indices": 2, "material": 0}]}],
    "materials": [{"name": "testbox_orange", "pbrMetallicRoughness": {"baseColorFactor": [0.95, 0.5, 0.06, 1.0], "metallicFactor": 0.1, "roughnessFactor": 0.7}}],
    "buffers": [{"byteLength": len(blob)}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": len(pb), "target": 34962},
        {"buffer": 0, "byteOffset": len(pb), "byteLength": len(nb), "target": 34962},
        {"buffer": 0, "byteOffset": len(pb) + len(nb), "byteLength": len(idx) * 2, "target": 34963}],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": len(pos), "type": "VEC3", "min": [min(p[k] for p in pos) for k in range(3)], "max": [max(p[k] for p in pos) for k in range(3)]},
        {"bufferView": 1, "componentType": 5126, "count": len(nor), "type": "VEC3"},
        {"bufferView": 2, "componentType": 5123, "count": len(idx), "type": "SCALAR"}],
}
js = json.dumps(doc).encode()
js += b" " * (-len(js) % 4)
os.makedirs(OUT, exist_ok=True)
with open(os.path.join(OUT, "testbox.glb"), "wb") as f:
    f.write(struct.pack("<4sII", b"glTF", 2, 12 + 8 + len(js) + 8 + len(blob)))
    f.write(struct.pack("<I4s", len(js), b"JSON") + js)
    f.write(struct.pack("<I4s", len(blob), b"BIN\0") + blob)
json.dump([{"id": "plate", "file": "testbox.glb", "yUp": True, "notes": "Pipeline test: rounded box 1.3 x 0.24 x 1.5 m, made by Tools/make_test_glb.py."}],
          open(os.path.join(OUT, "manifest.json"), "w"), indent=1)
print("wrote", os.path.join(OUT, "testbox.glb"), len(pos), "vertices,", len(idx) // 3, "triangles")
