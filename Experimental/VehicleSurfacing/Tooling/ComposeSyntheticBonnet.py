#=======================================================================================================================================
#                                                   COMPOSESYNTHETICBONNET.PY
#=======================================================================================================================================
# 🧩 Writes the vehicle-plan samples from known analytic shapes, standing in for edge loops exported from Blender (limit
#    points, quad spacing, 0.05 mm noise, mirror snap at x = 0): SyntheticBonnet.toml (V1–V4 sections, unchanged),
#    SyntheticBonnetNetwork.toml (V5: 6 sections + 4 spines that cross them) and SyntheticPod.toml (V7: 5 planar half hoops
#    from the top of x = 0 round to the bottom of x = 0 — loft, mirror, close → solid).

#-----------------------------------------------------------------------------------------------------------------------
#                                                    IMPORTS
#-----------------------------------------------------------------------------------------------------------------------

import math
import pathlib
import random

#-----------------------------------------------------------------------------------------------------------------------
#                                                    SURFACE
#-----------------------------------------------------------------------------------------------------------------------

HalfWidth   = 0.75                                                                              # [m] - x across (mirror plane x = 0)
Length      = 1.1                                                                               # [m] - y along (front y = 0)
NoiseLimit  = 5.0e-5                                                                            # [m] - |noise| ≤ 0.05 mm
SectionEtas = [0.0, 0.17, 0.38, 0.6, 0.81, 1.0]                                                 # [-] - Loft sections
BetweenEtas = [0.08, 0.27, 0.5, 0.7, 0.9]                                                       # [-] - Check loops (never lofted)
SpineXis    = [0.0, 0.35, 0.7, 1.0]                                                             # [-] - Network spines (V5)
PodLength   = 1.6                                                                               # [m] - Pod along y
PodStations = 5                                                                                 # [-] - Half hoops


def BonnetHeight(X, Y):
    """📝 H(x, y) = 0.78 + 0.16·η − 0.06·η² − 0.085·ξ² + 0.012·sin(πη)·(1 − ξ²), ξ = x / 0.75, η = y / 1.1 (as the kernel gate)."""
    Xi, Eta = X / HalfWidth, Y / Length
    return 0.78 + 0.16 * Eta - 0.06 * Eta * Eta - 0.085 * Xi * Xi + 0.012 * math.sin(math.pi * Eta) * (1.0 - Xi * Xi)


def IrregularStations(Count):
    """Quad-model spacing: monotone, irregular, ends exact."""
    Stations = [(Index + 0.3 * math.sin(2.3 * Index)) / (Count - 1) for Index in range(Count)]
    Stations[0], Stations[-1] = 0.0, 1.0
    return Stations


def Loop(Eta, Generator):
    Points = []
    Scale  = NoiseLimit / math.sqrt(3.0)
    for Station in IrregularStations(26):
        X, Y = HalfWidth * Station, Length * Eta
        Point = [X + Scale * Generator.uniform(-1, 1), Y + Scale * Generator.uniform(-1, 1), BonnetHeight(X, Y) + Scale * Generator.uniform(-1, 1)]
        if Station == 0.0:
            Point[0] = 0.0                                                                      # Mirror snap
        Points.append(Point)
    return Points

def Spine(Xi, Generator):
    """Loop along y at ξ (30 points); the ξ = 0 spine lies in the mirror plane."""
    Points = []
    Scale  = NoiseLimit / math.sqrt(3.0)
    for Station in IrregularStations(30):
        X, Y = HalfWidth * Xi, Length * Station
        Point = [X + Scale * Generator.uniform(-1, 1), Y + Scale * Generator.uniform(-1, 1), BonnetHeight(X, Y) + Scale * Generator.uniform(-1, 1)]
        if Xi == 0.0:
            Point[0] = 0.0                                                                      # Mirror snap
        Points.append(Point)
    return Points


def PodHalfWidth(Y):
    return 0.4 + 0.1 * math.sin(math.pi * Y / PodLength)                                        # [m] - a(y)


def PodHalfHeight(Y):
    return 0.3 + 0.05 * Y                                                                       # [m] - b(y)


def Hoop(Y, Generator):
    """Half ellipse x = a·cos θ, z = b·sin θ, θ from π/2 (top, x = 0) to −π/2 (bottom, x = 0): 28 points."""
    Points = []
    Scale  = NoiseLimit / math.sqrt(3.0)
    for Station in IrregularStations(28):
        Theta = 0.5 * math.pi - math.pi * Station
        Point = [PodHalfWidth(Y) * math.cos(Theta) + Scale * Generator.uniform(-1, 1), Y + Scale * Generator.uniform(-1, 1),
                 PodHalfHeight(Y) * math.sin(Theta) + Scale * Generator.uniform(-1, 1)]
        if Station in (0.0, 1.0):
            Point[0] = 0.0                                                                      # Mirror snap (both ends)
        Points.append(Point)
    return Points


def WriteSample(FileName, Header, Source, Curves, PlanarCondition=False):
    """Curves: [(name, points)] → Samples/FileName in the V1 format."""
    Lines = [Header, 'Format = "VehicleCurves"', 'Units  = "m"', f'Source = "{Source}"', '']
    for Name, Points in Curves:
        Lines += ['[[Curves]]', f'Name   = "{Name}"', 'Closed = false'] + (['Planar = true'] if PlanarCondition else []) + ['Points = [']
        Lines += [f'    [{Point[0]!r}, {Point[1]!r}, {Point[2]!r}],' for Point in Points]
        Lines += [']', '']
    Target = pathlib.Path(__file__).resolve().parent.parent / 'Samples' / FileName
    Target.write_text('\n'.join(Lines), encoding='utf-8')
    print(f"wrote {Target} ({len(Curves)} curves)")

#-----------------------------------------------------------------------------------------------------------------------
#                                                    ENTRY
#-----------------------------------------------------------------------------------------------------------------------

def Main():
    Generator = random.Random(0xB0AA37)
    Lines = ['# Vehicle plan V1 sample: synthetic bonnet (analytic H(x, y), see ComposeSyntheticBonnet.py) — 6 sections + 5 check loops',
             'Format = "VehicleCurves"', 'Units  = "m"', 'Source = "ComposeSyntheticBonnet.py · seed 0xB0AA37 · noise ≤ 0.05 mm"', '']
    Curves = [(f"Section{Index + 1}", Eta) for Index, Eta in enumerate(SectionEtas)] + [(f"Between{Index + 1}", Eta) for Index, Eta in enumerate(BetweenEtas)]
    for Name, Eta in Curves:
        Lines += ['[[Curves]]', f'Name   = "{Name}"', 'Closed = false', 'Points = [']
        Lines += [f'    [{Point[0]!r}, {Point[1]!r}, {Point[2]!r}],' for Point in Loop(Eta, Generator)]
        Lines += [']', '']
    Target = pathlib.Path(__file__).resolve().parent.parent / 'Samples' / 'SyntheticBonnet.toml'
    Target.write_text('\n'.join(Lines), encoding='utf-8')
    print(f"wrote {Target} ({len(Curves)} curves)")

    # V5 network: sections and spines from their own generator (SyntheticBonnet.toml stays bit-identical).
    Generator = random.Random(0xB0AA38)
    Network   = [(f"Section{Index + 1}", Loop(Eta, Generator)) for Index, Eta in enumerate(SectionEtas)]
    Network  += [(f"Spine{Index + 1}", Spine(Xi, Generator)) for Index, Xi in enumerate(SpineXis)]
    WriteSample('SyntheticBonnetNetwork.toml', '# Vehicle plan V5 sample: synthetic bonnet network — 6 sections across + 4 spines along (they cross: L builds a Gordon surface)',
                'ComposeSyntheticBonnet.py · seed 0xB0AA38 · noise ≤ 0.05 mm', Network)

    # V7 pod: planar half hoops (Planar = true: fitted in their best plane, so the rims cap exactly).
    Generator = random.Random(0x90D5)
    Pod = [(f"Hoop{Index + 1}", Hoop(PodLength * Index / (PodStations - 1), Generator)) for Index in range(PodStations)]
    WriteSample('SyntheticPod.toml', '# Vehicle plan V7 sample: synthetic pod — 5 planar half hoops a(y) = 0.4 + 0.1·sin(πy/1.6), b(y) = 0.3 + 0.05·y (loft → Alt+X → Close)',
                'ComposeSyntheticBonnet.py · seed 0x90D5 · noise ≤ 0.05 mm', Pod, True)


if __name__ == '__main__':
    Main()
