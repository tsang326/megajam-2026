#!/usr/bin/env python3
"""Writes Content/Data/film.json: the opening film and the removed scene as key poses.

Set space is the same as shot space: x right, y up, z away from the set's camera (cm).
A key is [t, x, y, z, headX, headY, roll, eye]. The game interpolates between keys.
Actor names: "unit7..." is painted as Unit 7, a name ending in "lamp" carries the lamp,
"cable..." is a plain cable, anything else is a crew robot.
Edit the functions below and run this again; no rebuild is needed.
"""
import json, math, os

TAN_H = math.tan(math.radians(30.0))
TAN_V = TAN_H / (16.0 / 9.0)
STEP = 0.25
OUT = os.path.join(os.path.dirname(__file__), "..", "Content", "Data", "film.json")


def keys(length, fn):
    out, t = [], 0.0
    while t <= length + 1e-6:
        k = fn(t)
        out.append([round(t, 2)] + [round(v, 2) for v in k])
        t += STEP
    return out


def launch(i):   # they hang under the platform and drop one by one
    def f(t):
        tau = max(0.0, t - 0.6 - 0.45 * i)
        return [(i - 3) * 150.0, 150.0 - 130.0 * tau * tau - 20.0 * tau, 900.0, 1.0, -0.6 if tau > 0 else 0.0, 0.0, 1.0]
    return f


def descent(i):
    def f(t):
        return [(i - 3) * 135.0 + 12.0 * math.sin(t * 1.3 + i), 330.0 - 95.0 * t + 60.0 * math.sin(i * 1.7),
                900.0 + 160.0 * math.sin(i * 2.3), 1.0 if i % 2 else -1.0, -0.6, 0.0, 1.0]
    return f


def seafloor(i):  # on the sea floor, sweeping right
    def f(t):
        return [(i - 3) * 150.0 - 120.0 + 50.0 * t, -95.0 + 14.0 * math.sin(t * 2.2 + i), 900.0 + 120.0 * math.sin(i * 1.9),
                1.0, 0.1 * math.sin(t + i), 0.0, 1.0]
    return f


def lift(t):      # Unit 7 alone, dark, lifted away on a cable
    return [0.0, -150.0 + 42.0 * t, 900.0, 1.0, -0.35, 28.0 + 6.0 * math.sin(t * 0.9), 0.0]


def cable(t):
    k = lift(t)
    return [k[0], k[1] + 385.0, k[2]]


# removed scene 1: Unit 7 holds its lamp on a dark gap while three of the crew pass through the beam
U7 = (-0.2, -0.15, 900.0)  # screen x, screen y, depth. The restore-by-cut frame is read from this first key.


def unit7(t):
    return [U7[0] * U7[2] * TAN_H, U7[1] * U7[2] * TAN_V + 6.0 * math.sin(t * 2.0), U7[2], 1.0, 0.22 + 0.12 * math.sin(t * 1.4), 0.0, 1.0]


def passer(i):
    def f(t):
        ti = t - 1.2 - 2.4 * i
        nx = U7[0] + 0.2 - 1.25 + 0.36 * max(ti, 0.0)
        ny = U7[1] + 0.42 + 0.05 * math.sin(ti * 2.0 + i)
        d = U7[2] + 140.0 + 60.0 * i
        return [nx * d * TAN_H, ny * d * TAN_V, d, 1.0, 0.1 * math.sin(ti * 2.0), 0.0, 1.0]
    return f


lamp, t = [[0.0, 60000]], 9.2
while t < 9.9:  # the lamp stutters once near the end
    lamp.append([round(t, 3), 6000 if math.fmod(t * 14.0, 2.0) < 1.0 else 60000])
    t += 1.0 / 14.0
lamp.append([9.9, 60000])

HW, HH = TAN_H * 1000.0, TAN_V * 1000.0
film = {
    "_comment": "Made by Tools/make_film_json.py. Sets, clips of key poses and the reel order. Captions are player-facing text.",
    "flashback": "removed1",
    "sets": {
        "launch": {"solids": [
            {"p": [0, 215, 900], "s": [1150, 50, 260], "kind": "metal"},
            {"p": [-480, 420, 900], "s": [40, 400, 40], "kind": "metal"},
            {"p": [480, 420, 900], "s": [40, 400, 40], "kind": "metal"}]},
        "descent": {"solids": []},
        "seafloor": {"solids": [
            {"p": [0, -200, 1500], "s": [4000, 60, 4000]},
            {"p": [-380, -110, 1250], "s": [200, 190, 260], "r": [0, 30, 12]},
            {"p": [420, -90, 1400], "s": [240, 260, 300], "r": [8, 60, 0]}]},
        "lift": {"solids": []},
        "gap": {"oldFilm": True, "solids": [
            {"p": [0, round(HH * 0.80, 1), 1150], "s": [round(HW * 2.6, 1), round(HH * 0.50, 1), 900]},
            {"p": [0, round(-HH * 0.82, 1), 1150], "s": [round(HW * 2.6, 1), round(HH * 0.46, 1), 900]},
            {"p": [round(HW * 0.75, 1), 0, 1500], "s": [round(HW * 0.5, 1), round(HH * 1.4, 1), 300], "r": [0, 12, 0]}]},
    },
    "clips": {
        "launch": {"set": "launch", "length": 5.0, "tracks": {("unit7" if i == 6 else "crew%d" % i): keys(5.0, launch(i)) for i in range(7)}},
        "descent": {"set": "descent", "length": 4.0, "tracks": {("unit7" if i == 6 else "crew%d" % i): keys(4.0, descent(i)) for i in range(7)}},
        "seafloor": {"set": "seafloor", "length": 4.5, "tracks": {("unit7" if i == 6 else "crew%d" % i): keys(4.5, seafloor(i)) for i in range(7)}},
        "lift": {"set": "lift", "length": 4.5, "tracks": {"unit7": keys(4.5, lift), "cable": keys(4.5, cable)}},
        "removed1": {"set": "gap", "length": 12.0, "lamp": lamp,
                     "tracks": dict([("unit7lamp", keys(12.0, unit7))] + [("passer%d" % i, keys(12.0, passer(i))) for i in range(3)])},
    },
    "reel": [
        {"clip": "launch", "len": 2.2, "caption": "FATHOM DYNAMICS SURVEY CREW. LAUNCH DAY."},
        {"clip": "descent", "len": 1.8, "caption": "DESCENT TO THE SEA FLOOR."},
        {"gap": True, "len": 0.8},
        {"clip": "seafloor", "len": 2.0, "caption": "EXPLORATION MISSION SUCCESSFUL."},
        {"gap": True, "len": 0.8},
        {"clip": "lift", "len": 2.0, "caption": "UNIT 7 - DEFECTIVE, DECOMMISSIONED."},
        {"gap": True, "len": 0.8},
    ],
}


# ---------------------------------------------------------------------------------------------
# Stage 2: the diver head-cam clip. One clip; "cuts" are the seconds Fathom removed.
# The official version plays the clip without them, the restored version plays everything.
# Track "cam" moves the head camera: [t, x, y, z, yaw, pitch, roll] (cm, degrees).
# Creature tracks ("gentle", "predator"): [t, x, y, z, 0, 0, roll, scale]; they face the way they move.
# ---------------------------------------------------------------------------------------------
DIVER_LEN = 17.0
DIVER_CUTS = [[4.5, 9.5], [12.5, 16.0]]
DIVER_STATIC = [16.0, 17.0]
HITS = [6.0, 8.7]
MATCH_T = 5.5


def path(points):
    def f(t):
        if t <= points[0][0]:
            return list(points[0][1:])
        for a, b in zip(points, points[1:]):
            if t <= b[0]:
                u = (t - a[0]) / (b[0] - a[0])
                u = u * u * (3.0 - 2.0 * u)
                return [a[i] + (b[i] - a[i]) * u for i in (1, 2, 3)]
        return list(points[-1][1:])
    return f


def divercam(t):
    x, y, z = 10.0 * math.sin(0.8 * t), 6.0 * math.sin(1.1 * t), 30.0 * min(t, 12.5)
    yaw, pitch, roll = 4.0 * math.sin(0.5 * t), -3.0 + 2.0 * math.sin(0.7 * t), 1.5 * math.sin(0.9 * t)
    for h in HITS:
        dt = t - h
        if 0.0 <= dt <= 1.2:
            e = math.exp(-4.0 * dt)
            roll += 28.0 * e * math.cos(18.0 * dt)
            yaw += 14.0 * e * math.sin(22.0 * dt)
            pitch += 9.0 * e * math.sin(15.0 * dt)
            x += 60.0 * e * math.sin(20.0 * dt)
    if 9.5 <= t <= 12.5:
        yaw += 1.5 * math.sin(13.0 * t)
        pitch += 1.0 * math.sin(17.0 * t)
    if t > 13.0:
        y += 110.0 * (t - 13.0)
        pitch += min(14.0, 7.0 * (t - 13.0))
    return [x, y, z, yaw, pitch, roll]


# the predator crosses in front of the camera from the side, so its long body is seen (head-on it is only a dot)
_pred = path([(4.5, -1800, 250, 2600), (5.5, -420, 110, 1250), (6.0, -30, 0, 330), (6.5, 650, -100, 650), (7.4, 1100, 150, 1500),
              (8.1, 500, 200, 1150), (8.7, 40, 20, 410), (9.2, -700, -80, 750), (9.5, -1100, -150, 1300), (12.5, -1100, -150, 1300),
              (12.8, -600, -100, 1000), (13.4, -520, 20, 900), (14.8, -1500, 300, 3200), (17.0, -1700, 320, 3600)])
_gent = path([(0.0, 1300, 200, 4200), (4.5, 1200, 200, 4000), (9.5, 700, 120, 2300), (12.3, 80, -20, 560), (12.5, 90, -40, 540),
              (13.5, 170, -150, 500), (16.0, 120, 190, 520), (17.0, 120, 300, 520)])


def predator(t):
    return _pred(t) + [0.0, 0.0, 10.0 * math.sin(t * 9.0), 1.0]


def gentle(t):
    return _gent(t) + [0.0, 0.0, 3.0 * math.sin(t * 0.8), 1.0]


def diver_ndc(p, t):
    c = divercam(t)
    x, y, z = p[0] - c[0], p[1] - c[1], p[2] - c[2]
    yaw, pit = math.radians(c[3]), math.radians(c[4])
    x1, z1 = x * math.cos(yaw) - z * math.sin(yaw), x * math.sin(yaw) + z * math.cos(yaw)
    y2, z2 = y * math.cos(pit) - z1 * math.sin(pit), y * math.sin(pit) + z1 * math.cos(pit)
    return [round(x1 / (z2 * TAN_H), 3), round(y2 / (z2 * TAN_V), 3)]


film["sets"]["diver"] = {"oldFilm": True, "headCam": True, "torch": 26000, "solids": [
    {"p": [0, -330, 2500], "s": [7000, 120, 6000]},
    {"p": [-700, -160, 1500], "s": [320, 360, 380], "r": [0, 25, 8]},
    {"p": [650, -200, 1100], "s": [260, 240, 300], "r": [6, 50, 0]},
    {"p": [-250, -230, 900], "s": [180, 150, 200], "r": [0, 10, 12]},
    {"p": [300, -140, 2400], "s": [500, 420, 500], "r": [0, 70, 5]}]}
film["clips"]["diver"] = {
    "set": "diver", "length": DIVER_LEN, "cuts": DIVER_CUTS, "static": DIVER_STATIC,
    "match": diver_ndc(_pred(MATCH_T), MATCH_T), "matchT": MATCH_T, "thumbT": 5.85,
    "after": "Fathom cut it out.",
    "captions": [[0.3, 4.3, "DIVE 12.  DIVER CAM.", ""], [9.8, 12.4, "THE CREATURE ATTACKS THE DIVER.", "o"]],
    "tracks": {"cam": keys(DIVER_LEN, divercam), "predator": keys(DIVER_LEN, predator), "gentle": keys(DIVER_LEN, gentle)},
}

with open(OUT, "w") as f:
    json.dump(film, f, separators=(",", ":"))
    f.write("\n")
print("wrote", os.path.normpath(OUT), os.path.getsize(OUT), "bytes")
