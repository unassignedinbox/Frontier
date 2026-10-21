#=======================================================================================================================================
#                                                   TRACEBLUEPRINTSECTIONS.PY
#=======================================================================================================================================
# 🧩 Blueprint (side / top / front line drawing) → silhouettes → planar half sections → VehicleCurves.toml with [[ImagePlanes]].
#    The car is built the way a designer starts from a three-view: the side view gives the roof line and the underside, the top
#    view the plan half-width, the front view the half-width at each height. Each station y gets one planar half section from
#    the top of x = 0 round to the bottom of x = 0 (as SyntheticPod.toml), so the workspace can loft → Alt+X → Close → solid.
#    Usage: python TraceBlueprintSections.py <Calibration.toml> <Output.toml> [<Overlay.png>]
#    Needs Pillow. The blueprint itself is not committed (third-party drawing); the calibration names its path.

#-----------------------------------------------------------------------------------------------------------------------
#                                                    IMPORTS
#-----------------------------------------------------------------------------------------------------------------------

import math
import pathlib
import sys
import tomllib

from PIL import Image, ImageDraw

#-----------------------------------------------------------------------------------------------------------------------
#                                                    SILHOUETTES
#-----------------------------------------------------------------------------------------------------------------------

DarkLevel = 128                                                                                 # [0-255] - Line pixels


def Opening(Values, Width):
    """Morphological opening of a profile (erode = min, dilate = max): removes peaks narrower than Width samples (antenna, mirrors)."""
    Half  = Width // 2
    Count = len(Values)
    Eroded  = [min(Values[max(0, Index - Half):Index + Half + 1]) for Index in range(Count)]
    return [max(Eroded[max(0, Index - Half):Index + Half + 1]) for Index in range(Count)]


def Smooth(Values, Width):
    """Moving average (edges shrink the window)."""
    Half = Width // 2
    return [sum(Values[max(0, Index - Half):Index + Half + 1]) / len(Values[max(0, Index - Half):Index + Half + 1]) for Index in range(len(Values))]


def Sample(Table, Position):
    """Linear interpolation in a [(position, value)] table sorted by position (clamped)."""
    if Position <= Table[0][0]: return Table[0][1]
    if Position >= Table[-1][0]: return Table[-1][1]
    for (Low, LowValue), (High, HighValue) in zip(Table, Table[1:]):
        if Low <= Position <= High:
            Ratio = (Position - Low) / (High - Low) if High > Low else 0.0
            return LowValue + Ratio * (HighValue - LowValue)
    return Table[-1][1]


def TraceSide(Pixels, Calibration, Scale):
    """Roof line top(y) and underside bottom(y) from the side view; wheels bridged over the marked spans."""
    Side = Calibration["Side"]
    Front, Rear, Ground, Ceiling = Side["FrontColumn"], Side["RearColumn"], Side["GroundRow"], Side["CeilingRow"]
    Columns = list(range(Front + 2, Rear - 1))
    Tops, Bottoms = [], []
    for Column in Columns:
        Rows = [Row for Row in range(Ceiling, Ground - 2) if Pixels[Column, Row] < DarkLevel]
        Tops.append((Ground - Rows[0]) * Scale if Rows else 0.0)
        Bottoms.append((Ground - Rows[-1]) * Scale if Rows else 0.0)
    Tops = Opening(Tops, int(Side["AntennaWidth"] / Scale))
    Stations = [(Column - Front) * Scale for Column in Columns]
    Floor = Side["FloorHeight"]
    Under = []
    for Station, Bottom in zip(Stations, Bottoms):
        InsideWheel = any(abs(Station - Axle) <= Side["WheelSpan"] for Axle in Side["Axles"])
        Under.append(None if InsideWheel else max(Bottom, Floor))
    Known = [(Station, Value) for Station, Value in zip(Stations, Under) if Value is not None]
    Under = [Value if Value is not None else Sample(Known, Station) for Station, Value in zip(Stations, Under)]
    return list(zip(Stations, Smooth(Tops, 9))), list(zip(Stations, Smooth(Under, 31)))


def TracePlan(Pixels, Calibration, Scale):
    """Plan half-width w(y) from the top view (outermost line either side of the centre row, mirrors opened away)."""
    Top = Calibration["Top"]
    Front, Rear, Centre, Low, High = Top["FrontColumn"], Top["RearColumn"], Top["CentreRow"], Top["FirstRow"], Top["LastRow"]
    Columns = list(range(Front + 1, Rear))
    Widths = []
    for Column in Columns:
        Rows = [Row for Row in range(Low, High) if Pixels[Column, Row] < DarkLevel]
        Widths.append(0.5 * (Rows[-1] - Rows[0]) * Scale if Rows else 0.0)
    Widths = Opening(Widths, int(Top["MirrorWidth"] / Scale))
    Widths = [min(Width, Calibration["Car"]["Width"] / 2.0) for Width in Widths]
    return list(zip([(Column - Front) * Scale for Column in Columns], Smooth(Widths, 9)))


def TraceFront(Pixels, Calibration, Scale):
    """Front-view half-width F(z) per height; the mirror rows are bridged linearly (designer mark)."""
    View = Calibration["Front"]
    Centre, Ground, Left, Right, Ceiling = View["CentreColumn"], View["GroundRow"], View["FirstColumn"], View["LastColumn"], View["CeilingRow"]
    Table = []
    for Row in range(Ceiling, Ground - 2):
        Columns = [Column for Column in range(Left, Right) if Pixels[Column, Row] < DarkLevel]
        if not Columns: continue
        Table.append(((Ground - Row) * Scale, max(Centre - Columns[0], Columns[-1] - Centre) * Scale))
    Table.sort()
    Low, High = View["MirrorHeights"]
    Below = Sample([Entry for Entry in Table if Entry[0] < Low], Low)
    Above = Sample([Entry for Entry in Table if Entry[0] > High], High)
    Table = [(Height, Width if not Low <= Height <= High else Below + (Above - Below) * (Height - Low) / (High - Low)) for Height, Width in Table]
    Heights = [Height for Height, _ in Table]
    return list(zip(Heights, Smooth([Width for _, Width in Table], 7)))

#-----------------------------------------------------------------------------------------------------------------------
#                                                    SECTIONS
#-----------------------------------------------------------------------------------------------------------------------


def Resample(Points, Count):
    """Even arc-length resampling of a polyline."""
    Lengths = [0.0]
    for First, Second in zip(Points, Points[1:]): Lengths.append(Lengths[-1] + math.dist(First, Second))
    Result, Cursor = [], 0
    for Index in range(Count):
        Target = Lengths[-1] * Index / (Count - 1)
        while Cursor < len(Lengths) - 2 and Lengths[Cursor + 1] < Target: Cursor += 1
        Span  = Lengths[Cursor + 1] - Lengths[Cursor]
        Ratio = (Target - Lengths[Cursor]) / Span if Span > 0.0 else 0.0
        First, Second = Points[Cursor], Points[Cursor + 1]
        Result.append((First[0] + Ratio * (Second[0] - First[0]), First[1] + Ratio * (Second[1] - First[1])))
    Result[0], Result[-1] = (0.0, Result[0][1]), (0.0, Result[-1][1])
    return Result


def Fair(Points, Sigma, Spacing=0.004):
    """Arc-length Gaussian fairing of a half section: the polyline and its mirror image (x → −x) form one closed loop, so
    the centre-line ends stay on x = 0 with horizontal tangents (the mirror plane needs them). Rounds the beltline and
    roof-edge corners to radii of the order of Sigma; a convex corner moves inward by about 0.4·Sigma·turn."""
    Half  = Resample(Points, max(16, int(sum(math.dist(First, Second) for First, Second in zip(Points, Points[1:])) / Spacing)))
    Loop  = Half + [(-X, Z) for X, Z in reversed(Half[1:-1])]
    Count = len(Loop)
    Reach = int(3.0 * Sigma / Spacing)
    Kernel = [math.exp(-0.5 * (Offset * Spacing / Sigma) ** 2) for Offset in range(-Reach, Reach + 1)]
    Total  = sum(Kernel)
    Faired = []
    for Index in range(len(Half)):
        X = sum(Weight * Loop[(Index + Offset - Reach) % Count][0] for Offset, Weight in enumerate(Kernel)) / Total
        Z = sum(Weight * Loop[(Index + Offset - Reach) % Count][1] for Offset, Weight in enumerate(Kernel)) / Total
        Faired.append((X, Z))
    Faired[0], Faired[-1] = (0.0, Faired[0][1]), (0.0, Faired[-1][1])
    return Faired


def ComposeSection(Top, Bottom, Width, Front, Car):
    """Half section (x, z) at one station: crowned top → side wall S(h) = min(w, F(h)) → flat floor, corners rounded."""
    Crown = Car["Crown"] * min(1.0, Width / 0.5)
    Rows  = 24
    Wall  = []
    for Index in range(Rows + 1):
        Height = Top - Crown - (Top - Crown - Bottom) * Index / Rows
        Wall.append((min(Width, Sample(Front, Height)), Height))
    Shoulder = Wall[0][0]
    Crest = [(Shoulder * Index / 8.0, Top - Crown * (Index / 8.0) ** 2) for Index in range(8)]
    Floor = [(Wall[-1][0] * (1.0 - Index / 6.0), Bottom) for Index in range(1, 7)]
    return Resample(Fair(Crest + Wall + Floor, Car["Fairing"]), Car["SectionPoints"])


def Stations(Car):
    """Section stations: denser at the nose, the windscreen, the backlight and the tail (where the roof line turns)."""
    Explicit = Car.get("Stations")
    if Explicit: return Explicit
    Start, End, Count = Car["FirstStation"], Car["Length"] - Car["FirstStation"], Car["StationCount"]
    return [Start + (End - Start) * (0.5 - 0.5 * math.cos(math.pi * Index / (Count - 1))) for Index in range(Count)]

#-----------------------------------------------------------------------------------------------------------------------
#                                                    OUTPUT
#-----------------------------------------------------------------------------------------------------------------------


def ImagePlaneRows(Calibration, Scale):
    """[[ImagePlanes]] rows: crop (pixels) and the world corner / right / down spans of that crop."""
    Path   = Calibration["Blueprint"]
    Side, Top, View = Calibration["Side"], Calibration["Top"], Calibration["Front"]
    Planes = []
    SideCrop = Side["Crop"]
    Planes.append(("Side", SideCrop, ((Side["ImageOffset"]), (SideCrop[0] - Side["FrontColumn"]) * Scale, (Side["GroundRow"] - SideCrop[1]) * Scale),
                   (0.0, (SideCrop[2] - SideCrop[0]) * Scale, 0.0), (0.0, 0.0, -(SideCrop[3] - SideCrop[1]) * Scale)))
    TopCrop = Top["Crop"]
    Planes.append(("Top", TopCrop, ((Top["CentreRow"] - TopCrop[1]) * Scale, (TopCrop[0] - Top["FrontColumn"]) * Scale, Top["ImageOffset"]),
                   (0.0, (TopCrop[2] - TopCrop[0]) * Scale, 0.0), (-(TopCrop[3] - TopCrop[1]) * Scale, 0.0, 0.0)))
    FrontCrop = View["Crop"]
    Planes.append(("Front", FrontCrop, ((FrontCrop[0] - View["CentreColumn"]) * Scale, View["ImageOffset"], (View["GroundRow"] - FrontCrop[1]) * Scale),
                   ((FrontCrop[2] - FrontCrop[0]) * Scale, 0.0, 0.0), (0.0, 0.0, -(FrontCrop[3] - FrontCrop[1]) * Scale)))
    Text = ""
    for Name, Crop, Corner, Right, Down in Planes:
        Text += f'\n[[ImagePlanes]]\nName    = "{Name}"\nPath    = "{Path}"\nCrop    = [{Crop[0]}, {Crop[1]}, {Crop[2]}, {Crop[3]}]\n'
        Text += f"Corner  = [{Corner[0]:.6f}, {Corner[1]:.6f}, {Corner[2]:.6f}]\nRight   = [{Right[0]:.6f}, {Right[1]:.6f}, {Right[2]:.6f}]\n"
        Text += f"Down    = [{Down[0]:.6f}, {Down[1]:.6f}, {Down[2]:.6f}]\nOpacity = {Calibration['Opacity']}\n"
    return Text


def main():
    if len(sys.argv) < 3:
        print("usage: TraceBlueprintSections.py <Calibration.toml> <Output.toml> [<Overlay.png>]")
        return 2
    CalibrationPath = pathlib.Path(sys.argv[1])
    Calibration = tomllib.loads(CalibrationPath.read_text(encoding="utf-8"))
    Car   = Calibration["Car"]
    Image_ = Image.open(Calibration["Blueprint"]).convert("L")
    Pixels = Image_.load()
    Side  = Calibration["Side"]
    Scale = Car["Length"] / (Side["RearColumn"] - Side["FrontColumn"])                          # [m/px] - Overall length sets the scale

    Roof, Under = TraceSide(Pixels, Calibration, Scale)
    Plan        = TracePlan(Pixels, Calibration, Scale)
    Front       = TraceFront(Pixels, Calibration, Scale)

    Text  = f"# Blueprint trace: {Calibration['Title']} — {len(Stations(Car))} planar half sections from the side / top / front silhouettes\n"
    Text += 'Format = "VehicleCurves"\nUnits  = "m"\n'
    Text += f'Source = "TraceBlueprintSections.py · {CalibrationPath.name} · {Scale * 1000.0:.4f} mm/px"\n'
    Text += ImagePlaneRows(Calibration, Scale)
    Sections = []
    for Index, Station in enumerate(Stations(Car)):
        Section = ComposeSection(Sample(Roof, Station), Sample(Under, Station), Sample(Plan, Station), Front, Car)
        Sections.append((Station, Section))
        Text += f'\n[[Curves]]\nName   = "Station{Index + 1:02d}"\nClosed = false\nPlanar = true\nPoints = [\n'
        Text += "".join(f"    [{X:.6f}, {Station:.6f}, {Z:.6f}],\n" for X, Z in Section) + "]\n"
    pathlib.Path(sys.argv[2]).write_text(Text, encoding="utf-8")
    print(f"scale {Scale * 1000.0:.4f} mm/px · roof max {max(Value for _, Value in Roof):.3f} m · plan max {max(Value for _, Value in Plan):.3f} m · "
          f"front max {max(Value for _, Value in Front):.3f} m · {len(Sections)} sections → {sys.argv[2]}")

    if len(sys.argv) > 3:
        # 📝 Evidence: the traced silhouettes and the sections drawn back over the blueprint (red roof / underside, blue plan, green front).
        Overlay = Image.open(Calibration["Blueprint"]).convert("RGB")
        Draw = ImageDraw.Draw(Overlay)
        Ground, FrontColumn = Side["GroundRow"], Side["FrontColumn"]
        ToSide = lambda Station, Height: (FrontColumn + Station / Scale, Ground - Height / Scale)
        Draw.line([ToSide(S, H) for S, H in Roof], fill=(220, 30, 30), width=5)
        Draw.line([ToSide(S, H) for S, H in Under], fill=(220, 30, 30), width=5)
        for Station, _ in Sections: Draw.line([ToSide(Station, Sample(Under, Station)), ToSide(Station, Sample(Roof, Station))], fill=(240, 140, 0), width=3)
        Top = Calibration["Top"]
        ToTop = lambda Station, Offset: (Top["FrontColumn"] + Station / Scale, Top["CentreRow"] - Offset / Scale)
        Draw.line([ToTop(S, W) for S, W in Plan], fill=(30, 80, 230), width=5)
        Draw.line([ToTop(S, -W) for S, W in Plan], fill=(30, 80, 230), width=5)
        View = Calibration["Front"]
        ToFront = lambda Offset, Height: (View["CentreColumn"] + Offset / Scale, View["GroundRow"] - Height / Scale)
        Draw.line([ToFront(W, H) for H, W in Front], fill=(20, 170, 60), width=5)
        Draw.line([ToFront(-W, H) for H, W in Front], fill=(20, 170, 60), width=5)
        for Station, Section in Sections[::3]: Draw.line([ToFront(X, Z) for X, Z in Section], fill=(240, 140, 0), width=2)
        Overlay.save(sys.argv[3])
    return 0


if __name__ == "__main__":
    sys.exit(main())
