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
        {"clip": "launch", "len": 5.0, "caption": "FATHOM DYNAMICS SURVEY CREW. LAUNCH DAY."},
        {"clip": "descent", "len": 4.0, "caption": "DESCENT TO THE SEA FLOOR."},
        {"gap": True, "len": 2.5},
        {"clip": "seafloor", "len": 4.5, "caption": "EXPLORATION MISSION SUCCESSFUL."},
        {"gap": True, "len": 2.5},
        {"clip": "lift", "len": 4.5, "caption": "UNIT 7 - DEFECTIVE, DECOMMISSIONED."},
        {"gap": True, "len": 2.5},
    ],
}

with open(OUT, "w") as f:
    json.dump(film, f, separators=(",", ":"))
    f.write("\n")
print("wrote", os.path.normpath(OUT), os.path.getsize(OUT), "bytes")
