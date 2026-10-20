#====================================================================================================================================
#                                                     SHOWCASECOMPOSER.PY
#====================================================================================================================================
# 🧩 Composes Exhibits/Showcase.html: one self-contained page (no external resources) with kernel models, evidence tables and stills

"""
Run:  python Experimental/SolidArcRemastered/Tooling/ShowcaseComposer.py --meshes DIR [--output Exhibits/Showcase.html]

--meshes   directory written by `PatternStressValidation --export DIR` (one JSON tessellation per built row)
--output   the composed page (default Exhibits/Showcase.html)

Models    each listed row is un-welded per face (smooth shading inside a face, creases between faces), quantised to
          16 bits over its bounding box and embedded as base64 typed arrays, with the shared B-rep edge samplings as lines.
Tables    the pattern-stress rows are parsed from Exhibits/SolidArcRemastered/PatternStressValidation.md.
Stills    gallery panels, walkthrough captures and viewport frames are re-encoded as WebP data URIs. Requires Pillow.
"""

import argparse
import base64
import html
import io
import json
import re
import struct
import subprocess
from pathlib import Path

from PIL import Image

#------------------------------------------------------------------------------------------------------------------------
#                                                    SETTINGS
#------------------------------------------------------------------------------------------------------------------------

RepositoryRoot  = Path(__file__).resolve().parents[3]
ExhibitRoot     = RepositoryRoot / "Exhibits"
TemplatePath    = Path(__file__).resolve().parent / "ShowcaseTemplate.html"
PayloadMarker   = "/*@ShowcasePayload@*/"
Categories      = ["Plane", "Cylinder", "Cone", "Sphere", "Torus", "Other"]
GalleryWidth    = 640                                                                     # [px] - Gallery panel width
CaptureWidth    = 1280                                                                    # [px] - Walkthrough capture width
ViewportWidth   = 640                                                                     # [px] - Viewport gate frame width
GalleryQuality  = 72                                                                      # [-]  - WebP quality
CaptureQuality  = 78                                                                      # [-]

Models = [                                                                                # Code, title, caption
    ("T4",  "Rounded block, filleted loop",   "4 verticals r 0.5, then the top loop r 0.2: lines meet quarter tori at smooth joints"),
    ("T5",  "Rounded block, chamfered loop",  "the same loop as a chamfer: planes meet cones at tangent joints"),
    ("T8",  "Tangent chain from one edge",    "one seed line grows to all 8 loop edges; equals T5 exactly"),
    ("T3",  "Stadium slot, both rims",        "top and bottom rims r 0.1 in one operation"),
    ("T6",  "Boss foot loop",                 "rounded boss verticals, then the concave foot loop r 0.1"),
    ("P1",  "Slot cusps",                     "fillets on arcs that meet at cusps: mitred by the bisector section"),
    ("P2",  "Scallops, filleted",             "asymmetric mitres from the general A ∩ B section"),
    ("P3",  "Scallops, chamfered",            "chamfer contacts that do not meet, closed through the far face"),
    ("P4",  "Spoke–hub concave fillets",      "concave ends extend the neighbouring edge instead of consuming it"),
    ("P5",  "Spoke tops ending on the hub",   "blend caps are sections of the blend by the curved hub"),
    ("P6",  "Ring of cylinders",              "convex outer and concave inner cusps, every top edge r 0.05"),
    ("P9",  "Touching bosses, feet",          "6 foot circles r 0.1 on bosses with coplanar contact"),
    ("B13", "Six blocks merged, all filleted", "needs maximal faces: 6 unit blocks fuse to one, then every edge r 0.2"),
    ("B18", "Cross-bores with rim fillets",   "M5 cross-bores + 12 bore entry rims r 0.05"),
    ("M5",  "3 × 3 Steinmetz cross-bores",    "9 equal-radius crossings, 18 singular points, genus 15"),
    ("C6",  "Radial blind bores",             "8 traced cylinder ∩ cylinder bores into a disc"),
]

CaptureCaptions = {
    "01Workspace"       : "Workspace layout with the specimen: outliner, feature tree, viewport, inspector",
    "02FacesSelected"   : "Face mode (3), Shift adds a second cap face",
    "04BevelPreview"    : "B opens the bevel session; the live preview adds 1 face before committing",
    "05FilletApplied"   : "Fillet 0.3 committed: 12 faces, audit sound, undo depth 6",
    "06BevelRefusal"    : "An honest refusal: a chamfer beside the fillet would consume its edge (overflow is next)",
    "07Navigated"       : "Plasticity navigation: middle orbit, right pan, wheel dolly",
    "09FrontParallel"   : "Front view, orthographic (Numpad 1, Numpad 5)",
    "09RightParallel"   : "Right view, orthographic (Numpad 3)",
    "10FaceEdit"        : "Face edit on the selected cap",
    "13Constructed"     : "Construct dialog result: a new primitive in the tree",
    "16WelcomeBack"     : "Welcome back: restore the previous session",
    "17Recovered"       : "Recovered document after an unclean exit",
    "19BevelSignChamfer": "Inside the bevel session C switches to chamfer, D back to fillet",
    "20ShortcutExtrude" : "E opens the extrude session; Enter commits",
    "21TangentChain"    : "Alt+click extends the edge selection along the tangent chain (5 edges)",
}

#------------------------------------------------------------------------------------------------------------------------
#                                                    MODELS
#------------------------------------------------------------------------------------------------------------------------

def EncodeArray(Format, Values):
    return base64.b64encode(struct.pack("<%d%s" % (len(Values), Format), *Values)).decode("ascii")


def ComposeModel(TessellationDirectory, Code, Title, Caption):
    Source    = json.loads((TessellationDirectory / (Code + ".json")).read_text())
    Positions = Source["Positions"]
    Triangles = Source["Triangles"]
    Owners    = Source["TriangleFaces"]
    FaceKinds = Source["Faces"]
    Low       = [min(Positions[Axis::3]) for Axis in range(3)]
    High      = [max(Positions[Axis::3]) for Axis in range(3)]
    Span      = [max(High[Axis] - Low[Axis], 1e-12) for Axis in range(3)]

    def Quantise(Vertex):
        return [round((Positions[3 * Vertex + Axis] - Low[Axis]) / Span[Axis] * 65535) for Axis in range(3)]

    Remap, Quantised, Kinds, FaceIndices, Indices = {}, [], [], [], []
    for Triangle in range(len(Triangles) // 3):
        Face = Owners[Triangle]
        for Corner in range(3):
            Vertex = Triangles[3 * Triangle + Corner]
            Key    = (Face, Vertex)
            if Key not in Remap:
                Remap[Key] = len(Kinds)
                Quantised.extend(Quantise(Vertex))
                Kinds.append(Categories.index(FaceKinds[Face]) if FaceKinds[Face] in Categories else len(Categories) - 1)
                FaceIndices.append(Face)
            Indices.append(Remap[Key])
    EdgeQuantised = []
    for Polyline in Source["Edges"]:
        for Start, Finish in zip(Polyline, Polyline[1:]):
            EdgeQuantised.extend(Quantise(Start) + Quantise(Finish))
    return {
        "Code": Code, "Title": Title, "Caption": Caption, "Low": Low, "Span": Span, "FaceKinds": FaceKinds,
        "Positions": EncodeArray("H", Quantised), "Indices": EncodeArray("I", Indices), "Kinds": EncodeArray("B", Kinds),
        "FaceIndices": EncodeArray("H", FaceIndices), "EdgePositions": EncodeArray("H", EdgeQuantised),
    }

#------------------------------------------------------------------------------------------------------------------------
#                                                    TABLES
#------------------------------------------------------------------------------------------------------------------------

def InlineMarkdown(Text):
    Text = html.escape(Text.replace("\\|", "|"), quote=False)
    Text = re.sub(r"`([^`]+)`", r"<code>\1</code>", Text)
    return re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", Text)


def ComposeStress():
    Lines   = (ExhibitRoot / "SolidArcRemastered" / "PatternStressValidation.md").read_text(encoding="utf-8").splitlines()
    Summary = next(Line for Line in Lines if Line.startswith("**") and "rows" in Line)
    Rows    = []
    for Line in Lines:
        Pieces = re.split(r"(?<!\\)\|", Line)[1:-1]
        if len(Pieces) < 14:
            continue
        Cells = [Cell.strip() for Cell in Pieces[:13]] + ["|".join(Pieces[13:]).strip()]   # Findings may hold a bare |ΔV|
        if Cells[1] in ("Code", "") or set(Cells[1]) <= set("-"):
            continue
        Verdict, Code, Scene, Ops, Topology, Genus, Lumps, Volume, Closed, Error, Oracle, Triangles, Milliseconds, Finding = Cells
        Rows.append({
            "Verdict": Verdict, "Code": Code, "Family": re.match(r"[A-Z]+", Code).group(0), "Scene": InlineMarkdown(Scene), "Ops": Ops,
            "Topology": Topology, "Genus": Genus, "Lumps": Lumps, "Volume": Volume, "Closed": Closed, "Error": Error, "Oracle": Oracle,
            "Triangles": Triangles, "Milliseconds": Milliseconds, "Finding": InlineMarkdown(Finding),
        })
    return { "Summary": InlineMarkdown(Summary), "Rows": Rows }

#------------------------------------------------------------------------------------------------------------------------
#                                                    STILLS
#------------------------------------------------------------------------------------------------------------------------

def EncodeStill(Path, Width, Quality):
    Picture = Image.open(Path).convert("RGB")
    if Picture.width > Width:
        Picture = Picture.resize((Width, round(Picture.height * Width / Picture.width)), Image.LANCZOS)
    Buffer = io.BytesIO()
    Picture.save(Buffer, "WEBP", quality=Quality, method=6)
    return "data:image/webp;base64," + base64.b64encode(Buffer.getvalue()).decode("ascii")


def SpacedName(Stem):
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", Stem)


def ComposeStills():
    Galleries = {}
    for Folder in sorted((ExhibitRoot / "SolidArcRemastered").glob("*Gallery")):
        Label = SpacedName(Folder.name.replace("Gallery", ""))
        Galleries[Label] = [{ "Name": SpacedName(Path.stem), "Image": EncodeStill(Path, GalleryWidth, GalleryQuality) } for Path in sorted(Folder.glob("*.png"))]
    Order     = ["Blend", "Boolean", "Face Operation", "Feature", "Intersection", "Model", "Tessellation"]
    Galleries = { Name: Galleries[Name] for Name in Order if Name in Galleries }
    Captures  = []
    for Path in sorted((ExhibitRoot / "DraftingWorkspace" / "Walkthrough").glob("*.png")):
        Captures.append({ "Name": SpacedName(re.sub(r"^\d+", "", Path.stem)), "Caption": CaptureCaptions.get(Path.stem, ""), "Image": EncodeStill(Path, CaptureWidth, CaptureQuality) })
    Viewport  = [{ "Name": SpacedName(Path.stem.replace("Viewport", "")), "Image": EncodeStill(Path, ViewportWidth, GalleryQuality) }
                 for Path in sorted((ExhibitRoot / "DraftingWorkspace" / "Viewport").glob("*.png"))]
    return Galleries, Captures, Viewport

#------------------------------------------------------------------------------------------------------------------------
#                                                    COMPOSITION
#------------------------------------------------------------------------------------------------------------------------

def Main():
    Parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    Parser.add_argument("--meshes", required=True, type=Path)
    Parser.add_argument("--output", default=ExhibitRoot / "Showcase.html", type=Path)
    Arguments = Parser.parse_args()

    Commit, Dated = subprocess.run(["git", "log", "-1", "--format=%h %cs"], cwd=RepositoryRoot, capture_output=True, text=True).stdout.split()
    Galleries, Captures, Viewport = ComposeStills()
    Payload = {
        "Composed" : Dated,
        "Commit"   : Commit,
        "Models"   : [ComposeModel(Arguments.meshes, Code, Title, Caption) for Code, Title, Caption in Models],
        "Stress"   : ComposeStress(),
        "Galleries": Galleries,
        "Captures" : Captures,
        "Viewport" : Viewport,
    }
    Text = json.dumps(Payload, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    Page = TemplatePath.read_text(encoding="utf-8")
    assert PayloadMarker in Page
    Arguments.output.write_text(Page.replace(PayloadMarker, Text), encoding="utf-8")
    Sizes = { Name: len(json.dumps(Payload[Name])) for Name in ("Models", "Stress", "Galleries", "Captures", "Viewport") }
    print("%s · %.2f MB · %s" % (Arguments.output, Arguments.output.stat().st_size / 1e6, " · ".join("%s %.2f MB" % (Name, Size / 1e6) for Name, Size in Sizes.items())))


if __name__ == "__main__":
    Main()
