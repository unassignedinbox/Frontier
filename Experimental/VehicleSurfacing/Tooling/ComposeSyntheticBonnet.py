#=======================================================================================================================================
#                                                   COMPOSESYNTHETICBONNET.PY
#=======================================================================================================================================
# 🧩 Writes Samples/SyntheticBonnet.toml: the vehicle plan V1 format filled from a known analytic bonnet, standing in for
#    edge loops exported from Blender (limit points, quad spacing, 0.05 mm noise, mirror snap at x = 0).

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


if __name__ == '__main__':
    Main()
