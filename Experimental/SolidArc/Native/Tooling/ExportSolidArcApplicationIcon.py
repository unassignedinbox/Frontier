#====================================================================================================================================
#                                                 EXPORTSOLIDARCAPPLICATIONICON.PY
#====================================================================================================================================
# 🧩 Draws the SolidArc application icon (a filleted block on a dark tile) and writes the multi-size Windows .ico
#
#    Experimental/SolidArc/Native/Package/SolidArcWorkspace.ico   16 · 24 · 32 · 48 · 64 · 128 · 256 px, 32-bit RGBA
#
#    📝 Linked into SolidArcWorkspace.exe by SolidArcWorkspace.rc (with the version block and the manifest): a Windows
#       program with an icon, version details and a manifest looks like an ordinary application to scanners.
#    Run from the repository root (Pillow required):  python3 Experimental/SolidArc/Native/Tooling/ExportSolidArcApplicationIcon.py

import pathlib

from PIL import Image, ImageDraw

#--------------------------------------------------------------------------------------------------------------------
#                                                    ARTWORK
#--------------------------------------------------------------------------------------------------------------------
Supersample = 4                                                                          # [-]  - Drawn at 4 × then reduced
Canvas      = 256 * Supersample                                                          # [px] - Master edge length
Sizes       = [16, 24, 32, 48, 64, 128, 256]                                             # [px] - Stored icon sizes
Tile        = (32, 36, 44, 255)                                                          # [-]  - Dark slate tile (RGBA)
Front       = (120, 160, 210, 255)                                                       # [-]  - Lit block face
Top         = (176, 206, 240, 255)                                                       # [-]  - Upper face
Side        = (78, 108, 150, 255)                                                        # [-]  - Shaded face
Arc         = (255, 196, 64, 255)                                                        # [-]  - Fillet highlight


def Scale(Points):
    return [(X * Supersample, Y * Supersample) for X, Y in Points]


def DrawMaster():
    Master = Image.new("RGBA", (Canvas, Canvas), (0, 0, 0, 0))
    Pen    = ImageDraw.Draw(Master)
    Pen.rounded_rectangle((8 * Supersample, 8 * Supersample, 248 * Supersample, 248 * Supersample), radius=44 * Supersample, fill=Tile)
    # An isometric block whose front-top edge is rounded: the fillet band reads as the arc in the name.
    Pen.polygon(Scale([(56, 112), (128, 76), (200, 112), (128, 148)]), fill=Top)
    Pen.polygon(Scale([(56, 124), (128, 160), (128, 214), (56, 178)]), fill=Front)
    Pen.polygon(Scale([(128, 160), (200, 124), (200, 178), (128, 214)]), fill=Side)
    Pen.polygon(Scale([(56, 112), (128, 148), (128, 160), (56, 124)]), fill=Arc)
    Pen.polygon(Scale([(128, 148), (200, 112), (200, 124), (128, 160)]), fill=(224, 168, 48, 255))
    return Master


def Main():
    Root   = pathlib.Path(__file__).resolve().parents[4]
    Target = Root / "Experimental/SolidArc/Native/Package/SolidArcWorkspace.ico"
    Master = DrawMaster().resize((256, 256), Image.LANCZOS)
    Master.save(Target, format="ICO", sizes=[(Size, Size) for Size in Sizes])
    print(f"ExportSolidArcApplicationIcon: {Target.relative_to(Root)} ({Target.stat().st_size} bytes)")


if __name__ == "__main__":
    Main()
