/*====================================================================================================================================
                                                    SOLIDARCDOCUMENTVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Step 9: SolidArc document gates — construct tiles, picks, blends, face edits, parameters, undo, files, variable blends, drawn sketches

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Document/SolidArcDocument.h"
#include "UI/SolidArc/Document/SolidArcMeasurement.h"
#include "UI/SolidArc/Document/SolidArcImagePlane.h"
#include "UI/SolidArc/Document/SolidArcDocumentCodec.h"
#include "UI/SolidArc/Document/SolidArcDrawingSheet.h"
#include "UI/SolidArc/Document/SolidArcExchange.h"
#include "UI/SolidArc/Document/SolidArcScene.h"
#include "UI/SolidArc/Document/SolidArcSpecimen.h"
#include "UI/SolidArc/Document/VehicleCurveImport.h"
#include "UI/SolidArc/Viewport/SolidArcCamera.h"
#include "UI/SolidArc/Viewport/SolidArcGizmo.h"
#include "UI/SolidArc/Viewport/SolidArcSnapping.h"

#include "Engine/GeometryProcessing/Blend/BlendCatalogue.h"
#include "Engine/GeometryProcessing/Drawing/DrawingExport.h"
#include "Engine/GeometryProcessing/Exchange/StepExchange.h"
#include "Engine/GeometryProcessing/Exchange/StlExchange.h"
#include "Engine/GeometryProcessing/Feature/ReplayDigest.h"
#include "Engine/GeometryProcessing/Geometry/NurbsCarrier.h"
#include "Engine/GeometryProcessing/Measurement/MassProperties.h"
#include "Engine/GeometryProcessing/Sketch/SketchProfile.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace Frontier;
using namespace Frontier::SolidArc;
using namespace Frontier::GeometryProcessing;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double Pi = std::numbers::pi;

    struct GateTally
    {
        const char* Code     = "";                                                           // [-] - W1 …
        const char* Subject  = "";                                                           // [-] - What is measured
        const char* Limit    = "";                                                           // [-] - Pass condition (text)
        double      Worst    = 0.0;                                                          // [-] - Worst value
        long        Checks   = 0;                                                            // [-] - Measurements
        long        Failures = 0;                                                            // [-] - Measurements over the limit
    };

    GateTally ConstructGate   = { "W1", "Construct tiles (block, cylinder, sphere, torus, union, difference): |V − closed form| / V, sound audit", "≤ 10⁻⁹" };
    GateTally PickGate        = { "W2", "CPU ray pick: face under the ray / edge within reach resolves to the expected persistent name", "all equal" };
    GateTally BlendGate       = { "W3", "Fillet / chamfer on picked edges: |V − closed form| / V", "≤ 10⁻⁹" };
    GateTally FaceGate        = { "W4", "Face edit on a picked face (push): |V − closed form| / V", "≤ 10⁻⁹" };
    GateTally ParameterGate   = { "W5", "Parameter edit regenerates downstream steps; selection / references survive by name", "≤ 10⁻⁹, names kept" };
    GateTally AtomicGate      = { "W6", "Refused command (oversize fillet, empty selection) leaves tree, body and history untouched", "all equal" };
    GateTally HistoryGate     = { "W7", "Undo / redo restore exact volumes; delete removes a whole tile", "≤ 10⁻¹²" };
    GateTally FileGate        = { "W8", "TOML save → load → save is byte-equal; reloaded body volume and names equal", "equal" };
    GateTally VariableGate    = { "W9", "Variable blend (V / F): volume vs closed form; each size at its anchored corner through flip, parameter edit and reload", "≤ 10⁻⁹" };
    GateTally SurfacingGate   = { "W11", "Vehicle curves: VehicleCurves.toml round trip (bit-equal) → SpaceCurve steps → loft sheet vs analytic bonnet and limit points; pole move + undo; reload", "≤ 0.5 mm, equal" };
    GateTally PanelGate       = { "W12", "Vehicle panels: loft + loft (G2) → Alt+X mirror → 4-panel sheet; pod loft → mirror → Close → solid vs ∫ πab dy; crossing curves → network; reload byte-equal", "sound · rel. ≤ 2·10⁻³" };
    GateTally EvaluationGate  = { "W13", "Surface evaluation: deviation needles are true closest points (residual ∥ normal, ≤ 61² lattice) and ≤ 0.2 mm on the fitted bonnet; zebra phase jumps at a G0 crease and is continuous at G2; zebra density tessellation", "≤ 0.2 mm · G2 ≤ 10⁻⁶ stripe" };
    GateTally BlueprintGate   = { "W14", "Blueprint study: image planes (BMW calibration landmarks → world, picture decode, codec round trip, refusals, re-import by name) and x / y-axis Construct cylinders", "≤ 10⁻⁵ m · ≤ 10⁻⁹" };
    GateTally MatcapGate      = { "W15", "Matcaps: built-in library optics (tinted metal / white plastic highlights, brushed anisotropy, roughness), picture fitting, per-face / body assignment without regeneration, undo, regeneration by persistent name, codec, refusals", "exact · 1 / 255" };
    GateTally BodiesGate      = { "W16", "Multi-body: New body (R) tiles keep exact separate volumes; per-body matcaps resolve face → body → default without regeneration; Combine (Q) difference consumes the tool and its delete restores it; targeted Construct / blend touch one body; cross-body refusals; scene body spans; undo; codec", "≤ 10⁻⁹ · exact" };
    GateTally TrimGate        = { "W18", "Curve trim (Plasticity T): a pentagram drawn as one chain refuses to extrude, T on the five inner pieces leaves the star (area vs shoelace, extrude exact); preview leaves the document alone; cross-sketch trims (circle → arc, side split) with undo / redo; a trim that would break an extrude detaches (W46); an emptied sketch goes; codec", "≤ 10⁻¹² · ≤ 10⁻⁹ · exact" };
    GateTally CutGate         = { "W17", "Trim / split: C cuts a block by a drawn line (2 + 6 exact) and a circle (π/4), the outside piece keeps key and matcap; X removes picked bodies (undo / redo); vehicle loft cut by a character line along −z → two sound sheets, X the piece; refusals; codec", "≤ 10⁻⁹ · exact" };
    GateTally MoveGate        = { "W19", "Move / rotate / scale (G / R / S): Transform steps keep volume (×8 for scale 2), the scene and picking follow the body, face selection survives by name, undo exact; refusals untouched", "≤ 10⁻⁹ · exact" };
    GateTally CopyGate        = { "W20", "Duplicate (Shift+D) and copy / paste (Ctrl+C / Ctrl+V): moved copy = new active selected body with its own caption; paste after X copies the body as it stood at copy time; refusals", "≤ 10⁻⁹ · exact" };
    GateTally HideGate        = { "W21", "Hide family (H, Shift+H, Alt+H, Ctrl+H): scene bodies and picking follow, selection drops hidden bodies, undo, codec round trip, refusals", "exact" };
    GateTally CurveGate       = { "W22", "Join (J) two open drawings into a closed square (extrudes to 1); offset (O) it by ¼ (2.25); pipe (P) along a drawn line (π r² L); refusals; codec byte-equal with kinds 13 / 14 / 15", "≤ 10⁻⁹ · exact" };
    GateTally RevolveGate     = { "W29", "Revolve (Alt+R): annulus 3π about the proposed y-axis (New); a plate about its own edge line united → 4.625π, undo / redo; x-axis fallback quarter turn 3π/2; refusals (angle, crossing axis, no sketch, nothing to subtract); codec writes the point axis only without a line", "≤ 10⁻⁹ · exact" };
    GateTally EvolutionGate   = { "W30", "Evolution (Shift+P / Alt+E): rectangle along a proposed line sketch = prism 1; circle along a quarter arc = Pappus π²/16; a square swept through a block as Difference = 1.875; refusals; codec Kind 16 + Path", "≤ 10⁻⁹ planar · ≤ 10⁻⁶ NURBS skin" };
    GateTally ReplayGate      = { "W31", "Incremental replay: on a 27-step studded plate and the vehicle bonnet every command / undo / redo / preview clone equals a full replay with the cache off (volumes, names, tokens, step reports, saved file); appends resume at the new step, a parameter edit at its first binding step, undo / redo / clone restore every step; only the 8 newest history entries keep checkpoints", "identical · undo / redo / clone ≥ 10× · tail edit ≥ 2×" };
    GateTally XRayGate        = { "W32", "X-ray (Alt+Z) pick-through and box edges on a 4 × 2 × 1 block in the isometric view: the ray at the hidden back-bottom edge picks the top face opaque and that edge in x-ray, √(1.25² + 1) = 1.6008 farther along the ray; a visible edge still picks in x-ray; the full-viewport box takes the 9 edges off the far corner (0, 2, 0) opaque and all 12 in x-ray; an empty box takes none", "all equal" };
    GateTally MeasureGate     = { "W33", "Measure (I) and Section (Alt+S) on a 4 × 2 × 1 block and a separate 2 × 2 × 1 body: body mass properties, face area / perimeter, edge length, minimum distances and angles between faces and edges, totals, X / Z / face-plane sections with second moments, two-body sections by the parallel-axis theorem, hidden bodies skipped, refusals", "≤ 1e-9 relative" };
    GateTally SectionGate     = { "W34", "Section clip (Alt+S) on a 4 × 2 × 1 block cut at x = 1 (kept x ≤ 1): rays wholly on the removed side miss; a ray through the opening meets the cap at the back face x = 0 (distance 5, not the plane's 4); a kept face in front of the plane crossing still wins; removed edges are skipped, kept edges pick; the cap has no name; sheet faces show their back instead of a cap; the box takes only edges reaching the kept side; ComposeSolidArcSectionClip marks sheet bodies", "exact" };
    GateTally SnapGate        = { "W35", "Object snapping on a 4 × 2 × 1 block, a separate cylinder r 0.5 and coplanar sketches (isometric camera, pointer at projected pixels): endpoint, midpoint, edge / face / circle centres, on edge, on face, on circle, sketch crossing, tangent and perpendicular points from a reference, arc-length midpoint of a spline; hidden corners need x-ray, the section and excluded bodies drop candidates, the filter disables kinds, a dragged sketch excludes itself, direction lock lines", "≤ 1e-12 (on carrier)" };
    GateTally JointGate       = { "W36", "Joints (Alt+J) + interference on the bracket specimen: hover origins (rim centres, face centroids); a pin r 0.25 jointed revolute, flipped, offset 0.25 into the hole r 0.3 → centroid (3, 1.25, 0.5); a lever on the pin cap at 90° → exact bounds; measure lever + bracket clear 0.25, bracket + pin clear 0.05; pin offset −0.1 → lever overlaps (0.8 − 2 (0.2 √0.05 + 0.09 asin ⅔)) · 0.1; moving the bracket carries both; refusals; codec (joints, geometry, fixed point); undo", "≤ 1e-9" };
    GateTally SheetGate       = { "W37", "Drawing sheet (Alt+D) of the bracket specimen: kernel views, hidden lines and dimensions in mm (metres × 1000), automatic ISO 5455 scale (next larger refused), unit invariance against the millimetre kernel sheet, A4 third-angle placement, section A-A cap = exact section 4.116587, SVG / DXF export byte-equal beside the document, [DrawingSheet] codec round trip + refusals, hidden bodies refused", "≤ 1e-9" };
    GateTally BoundGate       = { "W39", "Bevel previews from the first Windows run: an upright fillet beside a 0.4 bottom fillet (the cap foot took the wrong root of the cylinder, the guide got a random tangent at the tangential corner, C′ blew up and the volume quadrature bisected for up to 334 s) builds every radius in a fraction of a second with |V − closed form| / V; a fillet / chamfer as wide as the neighbouring face (crashed) is refused with the document untouched", "≤ 10⁻⁹, ≤ 20 s, refused" };
    GateTally ExchangeGate    = { "W38", "STEP / STL exchange (Import… / Export…, Open / Save as by file type): bracket + sphere exported as STEP (the kernel writer's bytes, read back × 10⁹ mm³) and STL (watertight, \\|ΔV\\| ≤ sag · A); selection / hidden bodies choose what is written; Import makes one embedded Import step per solid (one undo entry) with equal volumes and faces; canonical fixed point; save → load → save byte-equal with equal digests; OCCT FilletedBoss (loop repaired) and InchBlock 1 × ½ × 2 in (inch → m) imported; a fillet on an imported edge = (1 − π/4) r² L; refusals atomic", "≤ 1e-9 (STL: \\|ΔV\\| ≤ sag · A)" };
    GateTally PrimitiveGate   = { "W40", "Construct catalogue primitives drawn as new bodies: cone (up and down), wedge, 3 / 6 / 8-sided prisms, tube and the drawn polygon against closed forms; eight tiles side by side as Separate bodies keep their own exact volumes (no Boolean); prism Radius and tube bore edits regenerate exactly; codec byte-equal; refusals atomic", "≤ 10⁻⁹ · exact" };
    GateTally InsetGate       = { "W41", "Inset (Plasticity O on faces · I): block / cylinder / triangle-prism tops split flat, pocketed and bossed against closed forms with exact face counts; a depth edit regenerates; the pocket floor takes a second inset; codec byte-equal; too-wide / zero / curved refusals atomic", "≤ 10⁻⁹ · exact" };
    GateTally SectionLoftGate = { "W42", "Loft through drawn sketches (line / polyline / rectangle / circle / slot / spline) + gizmo verbs (sketch place, face offset / tweak / rotate / scale): |V − closed form| / V, codec byte-equal, rigid-only sketch refusals", "≤ 10⁻⁹" };
    GateTally GizmoGate       = { "W43", "Transform gizmo (Slate GizmoPRO): every handle of each mode set T / R / S / T+R / R+S / T+R+S is picked at its projected centre, a far pixel misses; axis parameter and rotation angle recovered from pixel rays", "≤ 10⁻⁹ · exact" };
    GateTally ShellGate       = { "W45", "Solid shells, sketch on a face, hide / lock: closed sections loft a capped solid of its own (two in one file), open sections refuse the solid and loft a sheet; sketches drawn on a level and an upright body face extrude onto it; sketch / body hide and lock are undoable appearance entries that survive the codec byte-equal", "≤ 10⁻⁹ · exact" };
    GateTally CornerGate      = { "W47", "Sketch corner fillet / chamfer and fan lofts: every corner of an extruded rectangle rounded (the extrude follows, V = A − (4 − π) r²), one picked corner chamfered, the kept corner (virtual sharp) dragged with the fillets following, refusals leave the file untouched; two lofts out of one section sew into one sheet (one turned); codec byte-equal", "≤ 10⁻⁹ · exact" };
    GateTally DetachGate      = { "W46", "Trim / join on sketches a body reads: the trim detaches (original kept hidden for its extrude, the trimmed curves a new sketch, the body unchanged, one undo entry); a whole-curve trim only hides; picked sketches join (disjoint profiles extrude together), a read one stays hidden; codec byte-equal", "≤ 10⁻⁹ · exact" };
    GateTally PointGate       = { "W44", "Mode-1 sketch points (direct drag, gizmo on picked points): a rectangle corner dragged keeps H / V and the extrude follows; two corners moved together; Bezier poles turned 30° and a triangle scaled × 2 along x about their centre land exactly; one undo entry per drag; codec byte-equal; mismatched / missing / out-of-range refusals atomic", "≤ 10⁻⁹ · exact" };
    GateTally DrawingGate     = { "W10", "Drawn sketches (line, rectangle, circle, arc, slot, 4 curve forms) → extrude / Boolean: |V − closed form| / V; refusals; reload; delete", "≤ 10⁻⁹" };

    std::vector<std::string> Journal;                                                        // [-] - Narrative rows for the exhibit

    void Tally(GateTally& Gate, double Value, bool PassedCondition, const std::string& Note)
    {
        Gate.Worst = std::max(Gate.Worst, Value);
        ++Gate.Checks;
        if (!PassedCondition) ++Gate.Failures;
        std::string Escaped;                                                                 // A '|' in a name would split the table cell
        for (const char Glyph : Note) Escaped += Glyph == '|' ? std::string("\\|") : std::string(1, Glyph);
        char Row[1024];
        std::snprintf(Row, sizeof(Row), "| %s | %s | %.3g | %s |", Gate.Code, Escaped.c_str(), Value, PassedCondition ? "🟢" : "🔴");
        Journal.push_back(Row);
    }

    void TallyVolume(GateTally& Gate, const SolidArcDocument& Document, double Expected, const std::string& Note)
    {
        const double Relative = std::abs(Document.Summary.Volume - Expected) / std::max(1.0e-12, std::abs(Expected));
        Tally(Gate, Relative, Relative <= 1.0e-9 && Document.Summary.SoundCondition, Note + " (V = " + std::to_string(Document.Summary.Volume) + ")");
    }

    [[nodiscard]] OrbitRay RayBetween(std::array<double, 3> From, std::array<double, 3> Towards)
    {
        std::array<double, 3> Direction = { Towards[0] - From[0], Towards[1] - From[1], Towards[2] - From[2] };
        const double Length = std::sqrt(Direction[0] * Direction[0] + Direction[1] * Direction[1] + Direction[2] * Direction[2]);
        for (double& Component : Direction) Component /= Length;
        return { From, Direction };
    }

    // Pick through the scene and select the hit (as the viewport does on a click).
    std::string PickAndSelect(SolidArcDocument& Document, const OrbitRay& Ray, bool AdditiveCondition)
    {
        const SolidArcScene               Scene = BuildSolidArcScene(Document);
        const std::optional<SolidArcPick> Hit   = PickSolidArcScene(Scene, Ray, 0.02);
        if (!Hit) return {};
        const std::string Name = NameSolidArcIdentity(Scene, Hit->Identity);
        SelectSolidArcName(Document, SolidArcIdentityKind(Hit->Identity), Name, AdditiveCondition);
        return Name;
    }

    // Position of an end of the named edge on the document's current body (start, or end when reversed).
    [[nodiscard]] Vector3 EdgeEndPosition(const SolidArcDocument& Document, const std::string& EdgeName, bool ReversedCondition)
    {
        for (const auto& [Slot, Name] : Document.Regeneration.Naming.Edges)
        {
            if (Name != EdgeName) continue;
            const EdgeProfile& Profile = Document.Partition->Edges.Resolve(Document.Partition->Edges.TokenAt(Slot));
            return Document.Partition->Vertices.Resolve(ReversedCondition ? Profile.End : Profile.Start).Position;
        }
        return { std::nan(""), std::nan(""), std::nan("") };
    }

    // Worst distance from the expected cap vertices of a variable fillet on the front-top edge (y = 0, z = Top) to the
    // body's vertices: the rails end Size from the anchored corner (x = AnchorX) and EndSize from the far one.
    [[nodiscard]] double MeasureCapGap(const SolidArcDocument& Document, double AnchorX, double FarX, double Top, double Size, double EndSize)
    {
        const Vector3 Expected[4] = { { AnchorX, Size, Top }, { AnchorX, 0.0, Top - Size }, { FarX, EndSize, Top }, { FarX, 0.0, Top - EndSize } };
        double Worst = 0.0;
        for (const Vector3& Point : Expected)
        {
            double Nearest = std::numeric_limits<double>::infinity();
            for (const auto& [Slot, Name] : Document.Regeneration.Naming.Vertices)
                Nearest = std::min(Nearest, Distance(Document.Partition->Vertices.Resolve(Document.Partition->Vertices.TokenAt(Slot)).Position, Point));
            Worst = std::max(Worst, Nearest);
        }
        return Worst;
    }

    [[nodiscard]] SolidArcDrawingRequest DrawingOf(SolidArcDrawingKind Kind, int GridPlane, std::vector<Vector2> Points, bool ClosedCondition = false,
                                                   bool CentreCondition = false, SketchSplineForm Form = SketchSplineForm::Hermite)
    {
        SolidArcDrawingRequest Request;
        Request.Kind = Kind; Request.GridPlane = GridPlane; Request.Points = std::move(Points);
        Request.ClosedCondition = ClosedCondition; Request.CentreCondition = CentreCondition; Request.Form = Form;
        return Request;
    }

    // Exact profile area of a drawing (Green's theorem on the solved sketch; independent of the solid's volume integral).
    [[nodiscard]] double MeasureDrawingArea(const SolidArcDrawingRequest& Request)
    {
        SketchModel Sketch;
        if (!ComposeSolidArcSketch(Request, Sketch).empty()) return std::nan("");
        (void)SolveSketch(Sketch);
        return ExtractSketchProfile(Sketch).Area;
    }

    void Expect(SolidArcCommandOutcome Outcome)
    {
        if (!Outcome.AcceptedCondition) std::printf("⚠️ command \"%s\" refused: %s\n", Outcome.Label.c_str(), Outcome.Refusal.c_str());
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    MAIN
//------------------------------------------------------------------------------------------------------------------------

int main()
{
    const auto Start = std::chrono::steady_clock::now();

    // ---- W1 single tiles against closed forms
    {
        SolidArcDocument Document = CreateSolidArcDocument("Tiles");
        SolidArcConstructRequest Request;
        Request.Kind = SolidArcConstructKind::Sphere; Request.Position = { 0.3, -0.2, 1.0 }; Request.Radius = 0.8;
        Expect(ConstructSolidArcPrimitive(Document, Request));
        TallyVolume(ConstructGate, Document, 4.0 / 3.0 * Pi * 0.8 * 0.8 * 0.8, "Sphere r 0.8 at (0.3, −0.2, 1)");
        SolidArcDocument Ring = CreateSolidArcDocument("Ring");
        Request.Kind = SolidArcConstructKind::Torus; Request.Position = { 1.0, 2.0, 0.5 }; Request.Major = 1.5; Request.Minor = 0.4;
        Expect(ConstructSolidArcPrimitive(Ring, Request));
        TallyVolume(ConstructGate, Ring, 2.0 * Pi * Pi * 1.5 * 0.4 * 0.4, "Torus R 1.5 r 0.4 at (1, 2, 0.5)");
        SolidArcDocument Drilled = CreateSolidArcDocument("Drilled");
        Request = {};
        Request.Kind = SolidArcConstructKind::Block; Request.Position = { 0.0, 0.0, 0.0 }; Request.Width = 3.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Drilled, Request));
        Request.Kind = SolidArcConstructKind::Cylinder; Request.Position = { 1.0, 1.0, -0.5 }; Request.Radius = 0.4; Request.Height = 2.0;
        Request.Combination = FeatureCombination::Difference;
        Expect(ConstructSolidArcPrimitive(Drilled, Request));
        TallyVolume(ConstructGate, Drilled, 6.0 - Pi * 0.16, "Block 3 × 2 × 1 − through cylinder r 0.4");
    }

    // ---- W1 … W7 on one modelling session (the parity walkthrough)
    SolidArcDocument Document = CreateSolidArcDocument("Bracket");
    SolidArcConstructRequest Block;
    Block.Kind = SolidArcConstructKind::Block; Block.Width = 4.0; Block.Depth = 2.0; Block.Height = 1.0;
    Expect(ConstructSolidArcPrimitive(Document, Block));
    TallyVolume(ConstructGate, Document, 8.0, "Block1 4 × 2 × 1");
    SolidArcConstructRequest Boss;
    Boss.Kind = SolidArcConstructKind::Cylinder; Boss.Position = { 1.0, 1.0, 1.0 }; Boss.Radius = 0.5; Boss.Height = 0.75;
    Boss.Combination = FeatureCombination::Union;
    Expect(ConstructSolidArcPrimitive(Document, Boss));
    const double BossVolume = Pi * 0.25 * 0.75;
    TallyVolume(ConstructGate, Document, 8.0 + BossVolume, "+ Cylinder1 boss r 0.5 h 0.75 (union on the top face)");

    // W2: pick the top face beside the boss, then the right vertical-face / top-face edge (x = 4, z = 1)
    const std::string TopFace = PickAndSelect(Document, RayBetween({ 3.0, 1.0, 5.0 }, { 3.0, 1.0, 0.0 }), false);
    Tally(PickGate, 0.0, TopFace.find("Cap(End)") != std::string::npos, "ray ↓ at (3, 1) → " + TopFace);
    const std::string RightEdge = PickAndSelect(Document, RayBetween({ 5.0, 1.0, 2.0 }, { 4.0, 1.0, 1.0 }), false);
    const bool RightEdgeCondition = RightEdge.rfind("Edge(", 0) == 0 && RightEdge.find("Cap(End)") != std::string::npos;
    Tally(PickGate, 0.0, RightEdgeCondition, "ray at the x = 4 top edge → " + RightEdge);
    const std::string Missed = PickAndSelect(Document, RayBetween({ 9.0, 9.0, 9.0 }, { 9.0, 9.0, 20.0 }), false);
    Tally(PickGate, 0.0, Missed.empty(), "ray away from the body → no pick");

    // W3: fillet r 0.25 on the picked edge (length 2): removed (1 − π/4)·r²·L
    SelectSolidArcName(Document, SolidArcSelectionKind::Edge, RightEdge, false);
    Expect(BlendSolidArcSelection(Document, BlendProfile::Fillet, 0.25));
    const double FilletLoss = (1.0 - Pi / 4.0) * 0.0625 * 2.0;
    TallyVolume(BlendGate, Document, 8.0 + BossVolume - FilletLoss, "Fillet1 r 0.25 on the picked edge");
    // chamfer setback 0.2 on the opposite (x = 0) top edge: removed c²/2·L
    const std::string LeftEdge = PickAndSelect(Document, RayBetween({ -1.0, 1.0, 2.0 }, { 0.0, 1.0, 1.0 }), false);
    Expect(BlendSolidArcSelection(Document, BlendProfile::Chamfer, 0.2));
    TallyVolume(BlendGate, Document, 8.0 + BossVolume - FilletLoss - 0.02 * 2.0, "Chamfer1 0.2 on the x = 0 top edge");

    // W4: push the front face (y = 0) out by 0.3: + 0.3 × front area (rectangle minus the blended corners)
    const std::string FrontFace = PickAndSelect(Document, RayBetween({ 2.0, -5.0, 0.5 }, { 2.0, 0.0, 0.5 }), false);
    Tally(PickGate, 0.0, !FrontFace.empty() && FrontFace.rfind("Edge(", 0) != 0, "ray → front face → " + FrontFace);
    const double FrontArea = 4.0 * 1.0 - (1.0 - Pi / 4.0) * 0.0625 - 0.02;
    Expect(ChangeSolidArcSelection(Document, FaceOperationKind::Push, 0.3));
    const double PushedVolume = 8.0 + BossVolume - FilletLoss - 0.04 + 0.3 * FrontArea;
    TallyVolume(FaceGate, Document, PushedVolume, "Push1 0.3 on the picked front face");

    // W5: Block1.Depth 2 → 2.5: every blend follows (edge length 2.5), the push still applies to the renamed-same face
    const double BeforeEdit = Document.Summary.Volume;
    Expect(AssignSolidArcParameter(Document, "Block1.Depth", 2.5));
    const double EditedVolume = 4.0 * 2.5 + BossVolume - (1.0 - Pi / 4.0) * 0.0625 * 2.5 - 0.02 * 2.5 + 0.3 * FrontArea;
    TallyVolume(ParameterGate, Document, EditedVolume, "Block1.Depth = 2.5 → fillet, chamfer, push regenerate");
    Tally(ParameterGate, 0.0, Document.Regeneration.Refused == 0, "no step refused after the edit");
    Tally(ParameterGate, 0.0, Document.Selection.Faces.size() == 1 && Document.Selection.Faces[0] == FrontFace, "selection still names " + FrontFace);

    // W6: atomic refusals
    const size_t StepsBefore = Document.Tree.Steps.size();
    const size_t UndoBefore  = Document.UndoEntries.size();
    const double VolumeBefore = Document.Summary.Volume;
    const std::string BackEdge = PickAndSelect(Document, RayBetween({ 2.0, 3.5, 2.0 }, { 2.0, 2.5, 1.0 }), false);
    const SolidArcCommandOutcome Oversize = BlendSolidArcSelection(Document, BlendProfile::Fillet, 5.0);
    Tally(AtomicGate, 0.0, !Oversize.AcceptedCondition && Document.Tree.Steps.size() == StepsBefore && Document.UndoEntries.size() == UndoBefore &&
                           Document.Summary.Volume == VolumeBefore, "fillet r 5 on " + BackEdge + " refused: " + Oversize.Refusal.substr(0, 90));
    ClearSolidArcSelection(Document);
    const SolidArcCommandOutcome Empty = ChangeSolidArcSelection(Document, FaceOperationKind::Push, 0.1);
    Tally(AtomicGate, 0.0, !Empty.AcceptedCondition && Document.Tree.Steps.size() == StepsBefore, "push with no faces refused: " + Empty.Refusal);

    // W7: undo the edit, redo it, delete the boss tile
    UndoSolidArcCommand(Document);
    Tally(HistoryGate, std::abs(Document.Summary.Volume - BeforeEdit), Document.Summary.Volume == BeforeEdit, "undo Block1.Depth → exact earlier volume");
    RedoSolidArcCommand(Document);
    Tally(HistoryGate, std::abs(Document.Summary.Volume - EditedVolume) / EditedVolume, std::abs(Document.Summary.Volume - EditedVolume) <= 1.0e-12 * EditedVolume, "redo → edited volume");
    const std::string BossLabel = Document.Tree.Steps[2].Label;
    Expect(DeleteSolidArcStep(Document, BossLabel));
    const bool WholeTileCondition = std::none_of(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [](const FeatureStep& Step)
                                                 { return Step.Label == "Sketch#3" || Step.Label == "Extrude#4"; });
    TallyVolume(HistoryGate, Document, EditedVolume - BossVolume, "delete Cylinder1 (sketch + extrude) → volume without the boss");
    Tally(HistoryGate, 0.0, WholeTileCondition && Document.Tree.Parameters.end() == std::find_if(Document.Tree.Parameters.begin(), Document.Tree.Parameters.end(),
          [](const FeatureParameter& Parameter) { return Parameter.Name.rfind("Cylinder1.", 0) == 0; }), "both steps and every Cylinder1.* parameter removed");

    // W8: file round trip
    const std::string Saved = EncodeSolidArcDocument(Document);
    SolidArcDocumentFile Loaded = DecodeSolidArcDocument(Saved);
    SolidArcDocument Reloaded = CreateSolidArcDocument(Loaded.Title);
    const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Reloaded, Loaded.Tree, Loaded.Captions, "Open");
    Tally(FileGate, std::abs(Reloaded.Summary.Volume - Document.Summary.Volume), Loaded.Refusal.empty() && Opened.AcceptedCondition &&
          Reloaded.Summary.Volume == Document.Summary.Volume, "reloaded volume equal (" + std::to_string(Saved.size()) + " bytes of TOML)");
    Tally(FileGate, 0.0, Reloaded.Regeneration.Naming.Faces == Document.Regeneration.Naming.Faces &&
          Reloaded.Regeneration.Naming.Edges == Document.Regeneration.Naming.Edges, "reloaded persistent names equal");
    Tally(FileGate, 0.0, EncodeSolidArcDocument(Reloaded) == Saved, "save → load → save byte-equal");
    const SolidArcDocumentFile Broken = DecodeSolidArcDocument("Format = \"SolidArc\"\nSteps = [ { Kind = 0 } ]\n");
    Tally(FileGate, 0.0, !Broken.Refusal.empty(), "malformed file refused: " + Broken.Refusal);
    // 📝 Documents written before the SolidArc rename carry Format = "DraftingWorkspace" and still open unchanged.
    std::string Legacy = Saved;
    const std::string Current = "Format = 'SolidArc'";                                     // toml++ writes literal strings
    const size_t      FormatAt = Legacy.find(Current);
    if (FormatAt != std::string::npos) Legacy.replace(FormatAt, Current.size(), "Format = 'DraftingWorkspace'");
    const SolidArcDocumentFile LegacyFile = DecodeSolidArcDocument(Legacy);
    SolidArcDocument LegacyDocument = CreateSolidArcDocument(LegacyFile.Title);
    const SolidArcCommandOutcome LegacyOpened = ReplaceSolidArcTree(LegacyDocument, LegacyFile.Tree, LegacyFile.Captions, "Open");
    Tally(FileGate, std::abs(LegacyDocument.Summary.Volume - Document.Summary.Volume), FormatAt != std::string::npos && LegacyFile.Refusal.empty() &&
          LegacyOpened.AcceptedCondition && LegacyDocument.Summary.Volume == Document.Summary.Volume, "pre-rename Format = \"DraftingWorkspace\" opens, same volume");
    const SolidArcDocumentFile Foreign = DecodeSolidArcDocument("Format = \"Other\"\n");
    Tally(FileGate, 0.0, !Foreign.Refusal.empty(), "foreign format refused: " + Foreign.Refusal);

    // ---- W9 variable blend: pick the front-top edge (y = 0, z = 1) of a 4 × 2 × 1 block, fillet 0.1 → 0.3, flip, edit, reload
    for (int Flip = 0; Flip < 2; ++Flip)
    {
        const bool       ReversedCondition = Flip == 1;
        SolidArcDocument Variable = CreateSolidArcDocument("Variable");
        Expect(ConstructSolidArcPrimitive(Variable, Block));
        const std::string FrontEdge = PickAndSelect(Variable, RayBetween({ 2.0, -1.0, 2.0 }, { 2.0, 0.0, 1.0 }), false);
        const double AnchorX = EdgeEndPosition(Variable, FrontEdge, ReversedCondition).X;      // [len] - Corner that takes Size
        const double FarX    = 4.0 - AnchorX;
        SolidArcBlendRequest Request;
        Request.Shape = BlendProfile::Fillet; Request.Size = 0.1; Request.EndSize = 0.3; Request.ReversedCondition = ReversedCondition;
        const SolidArcCommandOutcome Applied = BlendSolidArcSelection(Variable, Request);
        Expect(Applied);
        const std::string Case = std::string(ReversedCondition ? "flipped" : "unflipped") + ", anchor x = " + std::to_string(AnchorX).substr(0, 3);
        TallyVolume(VariableGate, Variable, 8.0 - MeasureVariableFilletLoss(4.0, 0.1, 0.3), "Fillet 0.1 → 0.3 on " + FrontEdge + " (" + Case + ")");
        const double Gap = MeasureCapGap(Variable, AnchorX, FarX, 1.0, 0.1, 0.3);
        Tally(VariableGate, Gap, Gap <= 1.0e-9, "rails 0.1 from the anchored corner, 0.3 from the far one (" + Case + ")");
        Tally(VariableGate, 0.0, Applied.Label.find("0.1 → 0.3") != std::string::npos, "history caption: " + Applied.Label);

        // Width 4 → 5: the anchor is a named vertex, so 0.1 stays at its corner (x = 0 stays, x = 4 moves to 5).
        Expect(AssignSolidArcParameter(Variable, "Block1.Width", 5.0));
        const double MovedAnchor = AnchorX < 2.0 ? 0.0 : 5.0, MovedFar = 5.0 - MovedAnchor;
        TallyVolume(VariableGate, Variable, 10.0 - MeasureVariableFilletLoss(5.0, 0.1, 0.3), "Block1.Width = 5 regenerates the variable fillet (" + Case + ")");
        const double MovedGap = MeasureCapGap(Variable, MovedAnchor, MovedFar, 1.0, 0.1, 0.3);
        Tally(VariableGate, MovedGap, MovedGap <= 1.0e-9 && Variable.Regeneration.Refused == 0, "after the edit 0.1 is still at its corner, x = " + std::to_string(MovedAnchor).substr(0, 3) + " (" + Case + ")");

        // Save → load: the end size and the named anchor survive the TOML round trip.
        const std::string Saved = EncodeSolidArcDocument(Variable);
        SolidArcDocumentFile Loaded = DecodeSolidArcDocument(Saved);
        SolidArcDocument Reloaded = CreateSolidArcDocument(Loaded.Title);
        const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Reloaded, Loaded.Tree, Loaded.Captions, "Open");
        const double ReloadGap = Opened.AcceptedCondition ? MeasureCapGap(Reloaded, MovedAnchor, MovedFar, 1.0, 0.1, 0.3) : 1.0;
        Tally(VariableGate, ReloadGap, Opened.AcceptedCondition && Reloaded.Summary.Volume == Variable.Summary.Volume && ReloadGap <= 1.0e-9 &&
              Saved.find("SizeAnchors") != std::string::npos && EncodeSolidArcDocument(Reloaded) == Saved, "reload: same volume bits, anchored rails, byte-equal re-save (" + Case + ")");
    }

    // ---- W10 drawing: the workspace tools as the viewport commits them, extruded with the Plasticity Boolean choices
    {
        using Kind = SolidArcDrawingKind;
        const FeatureCombination Union = FeatureCombination::Union, Difference = FeatureCombination::Difference, New = FeatureCombination::New;
        auto DrawAndExtrude = [&](SolidArcDocument& Target, const SolidArcDrawingRequest& Request, double Distance, FeatureCombination Combination)
        {
            Expect(DrawSolidArcSketch(Target, Request));
            const SolidArcCommandOutcome Outcome = ExtrudeSolidArcSketch(Target, Target.Tree.Steps.back().Label, Distance, Combination);
            Expect(Outcome);
            return Outcome;
        };
        const SolidArcDrawingRequest Box = DrawingOf(Kind::Rectangle, 0, { { 0.0, 0.0 }, { 2.0, 1.0 } });
        const std::vector<Vector2> Square = { { 0.2, 0.2 }, { 0.5, 0.2 }, { 0.5, 0.8 }, { 0.2, 0.8 } };
        {
            SolidArcDocument Drawn = CreateSolidArcDocument("Drawn");
            DrawAndExtrude(Drawn, Box, 1.5, New);
            TallyVolume(DrawingGate, Drawn, 3.0, "Rectangle (0,0)–(2,1) on XY, extrude 1.5 (New)");
            DrawAndExtrude(Drawn, DrawingOf(Kind::Circle, 0, { { 1.0, 0.5 }, { 1.4, 0.5 } }, false, true), 1.5, Difference);
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi, "− centre circle r 0.4 through (Difference)");
            DrawAndExtrude(Drawn, DrawingOf(Kind::Line, 0, { { 1.5, 0.0 }, { 3.0, 0.0 }, { 1.5, 1.0 } }, true), 1.0, Union);
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi + 1.0 / 3.0, "+ closed line triangle, height 1 (Union; 1/3 outside the box)");
            DrawAndExtrude(Drawn, DrawingOf(Kind::Curve, 0, Square, true), 0.5, Difference);
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi + 1.0 / 3.0 - 0.123, "− closed Hermite pocket 0.5 (area 0.246, coplanar floor: closed spline section)");

            // Reload: spline points and forms round-trip; same volume bits, byte-equal re-save.
            const std::string Saved = EncodeSolidArcDocument(Drawn);
            SolidArcDocumentFile Loaded = DecodeSolidArcDocument(Saved);
            SolidArcDocument Reloaded = CreateSolidArcDocument(Loaded.Title);
            const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Reloaded, Loaded.Tree, Loaded.Captions, "Open");
            Tally(DrawingGate, std::abs(Reloaded.Summary.Volume - Drawn.Summary.Volume), Opened.AcceptedCondition && Reloaded.Summary.Volume == Drawn.Summary.Volume &&
                  EncodeSolidArcDocument(Reloaded) == Saved, "reload of the drawn document: same volume bits, byte-equal re-save");

            // Refusals leave the document untouched.
            const size_t Steps = Drawn.Tree.Steps.size();
            const double Volume = Drawn.Summary.Volume;
            const SolidArcCommandOutcome Collinear = DrawSolidArcSketch(Drawn, DrawingOf(Kind::Arc, 0, { { 0.0, 2.0 }, { 1.0, 2.0 }, { 2.0, 2.0 } }));
            const SolidArcCommandOutcome Coincident = DrawSolidArcSketch(Drawn, DrawingOf(Kind::Curve, 0, { { 0.0, 2.0 }, { 0.0, 2.0 }, { 1.0, 2.0 } }));
            const SolidArcCommandOutcome SecondBody = ExtrudeSolidArcSketch(Drawn, Drawn.Tree.Steps.front().Label, 1.0, New);
            Tally(DrawingGate, 0.0, !Collinear.AcceptedCondition && !Coincident.AcceptedCondition && !SecondBody.AcceptedCondition && Drawn.Tree.Steps.size() == Steps &&
                  Drawn.Summary.Volume == Volume, "refused untouched: \"" + Collinear.Refusal + "\" · \"" + Coincident.Refusal + "\" · \"" + SecondBody.Refusal + "\"");
            Expect(DrawSolidArcSketch(Drawn, DrawingOf(Kind::Arc, 0, { { 0.0, 2.0 }, { 2.0, 2.0 }, { 1.0, 2.5 } })));
            const SolidArcCommandOutcome OpenArc = ExtrudeSolidArcSketch(Drawn, Drawn.Tree.Steps.back().Label, 1.0, Union);
            Tally(DrawingGate, 0.0, !OpenArc.AcceptedCondition && Drawn.Summary.Volume == Volume, "open arc profile refused: \"" + OpenArc.Refusal + "\"");

            // Delete the pocket's sketch: its extrude goes with it.
            std::string CurveLabel;
            for (const FeatureStep& Step : Drawn.Tree.Steps) if (Drawn.Captions[Step.Label] == "Curve1") CurveLabel = Step.Label;
            Expect(DeleteSolidArcStep(Drawn, CurveLabel));
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi + 1.0 / 3.0, "delete Curve1: its extrude goes with it");
        }
        {
            SolidArcDocument Slotted = CreateSolidArcDocument("Slotted");
            DrawAndExtrude(Slotted, DrawingOf(Kind::Rectangle, 0, { { 1.0, 0.5 }, { 2.0, 1.0 } }, false, true), 1.5, New);
            TallyVolume(DrawingGate, Slotted, 3.0, "centre rectangle about (1, 0.5) to (2, 1), extrude 1.5");
            DrawAndExtrude(Slotted, DrawingOf(Kind::Slot, 0, { { 0.4, 0.5 }, { 1.2, 0.5 }, { 0.8, 0.7 } }), 1.5, Difference);
            TallyVolume(DrawingGate, Slotted, 3.0 - (0.8 * 0.4 + 0.04 * Pi) * 1.5, "− slot (0.4,0.5)–(1.2,0.5) r 0.2 through");
        }
        {
            // Upright plane: rectangle on XZ extrudes along −y.
            SolidArcDocument Upright = CreateSolidArcDocument("Upright");
            DrawAndExtrude(Upright, DrawingOf(Kind::Rectangle, 1, { { 0.0, 0.0 }, { 2.0, 1.0 } }), 0.5, New);
            TallyVolume(DrawingGate, Upright, 1.0, "rectangle on XZ, extrude 0.5");
            DrawAndExtrude(Upright, DrawingOf(Kind::Circle, 1, { { 0.6, 0.5 }, { 1.4, 0.5 } }), 0.5, Difference);
            TallyVolume(DrawingGate, Upright, 1.0 - 0.08 * Pi, "− two-point circle (0.6,0.5)–(1.4,0.5) on XZ");
        }
        // Each curve form alone: volume = exact profile area × height.
        const std::vector<Vector2> Wave = { { 0.0, 0.0 }, { 1.0, -0.3 }, { 2.0, 0.4 }, { 2.2, 1.5 }, { 0.8, 1.9 }, { -0.4, 1.0 } };
        for (const SketchSplineForm Form : { SketchSplineForm::Hermite, SketchSplineForm::Bezier, SketchSplineForm::Nurbs, SketchSplineForm::CubicSpline })
        {
            SolidArcDocument Freeform = CreateSolidArcDocument("Freeform");
            const SolidArcDrawingRequest Request = DrawingOf(Kind::Curve, 0, Wave, true, false, Form);
            const double Area = MeasureDrawingArea(Request);
            DrawAndExtrude(Freeform, Request, 0.7, New);
            TallyVolume(DrawingGate, Freeform, Area * 0.7, std::string("closed ") + SketchSplineFormLabel(Form) + " through 6 clicks, extrude 0.7 = area " + std::to_string(Area) + " × 0.7");
            DrawAndExtrude(Freeform, DrawingOf(Kind::Circle, 0, { { 1.0, 0.8 }, { 1.3, 0.8 } }, false, true), 0.7, Difference);
            TallyVolume(DrawingGate, Freeform, (Area - 0.09 * Pi) * 0.7, std::string("  − circle r 0.3 inside the ") + SketchSplineFormLabel(Form));
        }
    }

    // ---- W11 vehicle surfacing (plan V1 import, V3 curve steps, V4 loft) on the synthetic bonnet sample.
    {
        const VehicleCurveFile Sample = ReadVehicleCurves(SOLIDARC_VEHICLE_SAMPLE);
        size_t PointCount = 0;
        for (const VehicleCurve& Curve : Sample.Curves) PointCount += Curve.Points.size();
        Tally(SurfacingGate, 0.0, Sample.Refusal.empty() && Sample.Curves.size() == 11, "SyntheticBonnet.toml decoded: " + std::to_string(Sample.Curves.size()) + " curves · "
              + std::to_string(PointCount) + " points " + Sample.Refusal);
        const VehicleCurveFile Again = DecodeVehicleCurves(EncodeVehicleCurves(Sample));
        bool Equal = Again.Refusal.empty() && Again.Curves.size() == Sample.Curves.size();
        for (size_t Index = 0; Equal && Index < Sample.Curves.size(); ++Index)
        {
            const VehicleCurve& First = Sample.Curves[Index];
            const VehicleCurve& Second = Again.Curves[Index];
            Equal = First.Name == Second.Name && First.ClosedCondition == Second.ClosedCondition && First.Points.size() == Second.Points.size()
                 && std::memcmp(First.Points.data(), Second.Points.data(), First.Points.size() * sizeof(Vector3)) == 0;
        }
        Tally(SurfacingGate, 0.0, Equal, "V1 round trip decode → encode → decode: names, closed flags, every coordinate bit-equal");
        const VehicleCurveFile Millimetres = DecodeVehicleCurves("Format = \"VehicleCurves\"\nUnits = \"mm\"\n[[Curves]]\nName = \"A\"\nPoints = [[1000.0, 0.0, 0.0], [0.0, 500.0, 0.0]]\n");
        Tally(SurfacingGate, 0.0, Millimetres.Refusal.empty() && Millimetres.Curves.size() == 1 && Millimetres.Curves[0].Points[0].X == 1.0 && Millimetres.Curves[0].Points[1].Y == 0.5,
              "Units = \"mm\" scaled to metres");
        const std::pair<const char*, const char*> Malformed[] = {
            { "Format = \"Mesh\"\n", "Format is not" },
            { "Format = \"VehicleCurves\"\nUnits = \"in\"\n[[Curves]]\nName = \"A\"\nPoints = [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]\n", "unknown Units" },
            { "Format = \"VehicleCurves\"\n[[Curves]]\nName = \"A\"\nPoints = [[0.0, 0.0, 0.0]]\n", "needs at least 2 points" },
            { "Format = \"VehicleCurves\"\n[[Curves]]\nName = \"A\"\nPoints = [[0.0, 0.0, 0.0], [1.0, \"x\", 0.0]]\n", "point 1 is not [x, y, z]" },
            { "Format = \"VehicleCurves\"\n[[Curves]]\nPoints = [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]\n", "has no Name" },
        };
        for (const auto& [Text, Expected] : Malformed)
        {
            const VehicleCurveFile Refused = DecodeVehicleCurves(Text);
            Tally(SurfacingGate, 0.0, Refused.Refusal.find(Expected) != std::string::npos && Refused.Curves.empty(), "malformed file refused: " + Refused.Refusal);
        }

        // Import: 11 SpaceCurve steps in one undo entry, all within the 0.2 mm tolerance.
        SolidArcDocument Vehicle = CreateSolidArcDocument("Bonnet");
        const SolidArcCommandOutcome Imported = ImportSolidArcCurves(Vehicle, Sample);
        double WorstFit = 0.0;
        int    Poles = 0;
        for (const auto& [Label, Fit] : Vehicle.Regeneration.Curves) { WorstFit = std::max(WorstFit, Fit.WorstDeviation); Poles += static_cast<int>(Fit.Curve.Definition->Poles.size()); }
        Tally(SurfacingGate, WorstFit * 1000.0, Imported.AcceptedCondition && Vehicle.Tree.Steps.size() == 11 && Vehicle.Regeneration.Refused == 0 && Vehicle.UndoEntries.size() == 1
              && Vehicle.Regeneration.Curves.size() == 11 && WorstFit <= 2.0e-4, "import → 11 SpaceCurve steps, 1 undo entry, " + std::to_string(PointCount) + " points → " + std::to_string(Poles)
              + " poles, worst fit [mm] (value)");
        const SolidArcCommandOutcome Refused = ImportSolidArcCurves(Vehicle, DecodeVehicleCurves("Format = \"Mesh\"\n"));
        Tally(SurfacingGate, 0.0, !Refused.AcceptedCondition && Vehicle.Tree.Steps.size() == 11 && Vehicle.UndoEntries.size() == 1, "refused file leaves the document untouched: " + Refused.Refusal);

        // Loft through the 6 sections (by caption) → one open NURBS sheet.
        std::vector<std::string> Sections;
        for (const FeatureStep& Step : Vehicle.Tree.Steps) if (Vehicle.Captions[Step.Label].rfind("Section", 0) == 0) Sections.push_back(Step.Label);
        const SolidArcCommandOutcome Single = LoftSolidArcCurves(Vehicle, { Sections.front() });
        Tally(SurfacingGate, 0.0, !Single.AcceptedCondition && Single.Refusal.find("at least 2") != std::string::npos, "loft through one curve refused: " + Single.Refusal);
        const SolidArcCommandOutcome Lofted = LoftSolidArcCurves(Vehicle, Sections);
        const SolidArcBodySummary& Sheet = Vehicle.Summary;
        Tally(SurfacingGate, 0.0, Lofted.AcceptedCondition && Sheet.PresentCondition && Sheet.SheetCondition && Sheet.SoundCondition && Sheet.Faces == 1 && Sheet.Edges == 4 && Sheet.Vertices == 4,
              "loft 6 sections → sheet: " + std::to_string(Sheet.Faces) + " face · " + std::to_string(Sheet.Edges) + " edges · audit " + (Sheet.SoundCondition ? "sound" : "findings") + " " + Lofted.Refusal);
        auto SheetSurface = [](const SolidArcDocument& Document)
        {
            const FaceToken Face = Document.Partition->Faces.TokenAt(Document.Regeneration.Naming.Faces.begin()->first);
            return Document.Partition->Surfaces.Resolve(Document.Partition->Faces.Resolve(Face).Surface);
        };
        if (Lofted.AcceptedCondition)
        {
            const SurfaceGeometry Surface = SheetSurface(Vehicle);
            double WorstPoint = 0.0, WorstAnalytic = 0.0;
            for (const VehicleCurve& Curve : Sample.Curves)
            {
                for (const Vector3& Point : Curve.Points)
                {
                    const Vector2 Foot = ProjectOntoSurface(Surface, Point, { Point.X / 0.75, Point.Y / 1.1 });
                    WorstPoint = std::max(WorstPoint, Length(EvaluateSurface(Surface, Foot).Position - Point));
                }
            }
            for (int First = 0; First <= 60; ++First)
            {
                for (int Second = 0; Second <= 60; ++Second)
                {
                    const Vector3 Point = EvaluateSurface(Surface, { First / 60.0, Second / 60.0 }).Position;
                    const double  Xi = Point.X / 0.75, Eta = Point.Y / 1.1;
                    const double  Height = 0.78 + 0.16 * Eta - 0.06 * Eta * Eta - 0.085 * Xi * Xi + 0.012 * std::sin(Pi * Eta) * (1.0 - Xi * Xi);
                    const double  Across = (-0.17 * Xi - 0.024 * Xi * std::sin(Pi * Eta)) / 0.75;
                    const double  Along  = (0.16 - 0.12 * Eta + 0.012 * Pi * std::cos(Pi * Eta) * (1.0 - Xi * Xi)) / 1.1;
                    WorstAnalytic = std::max(WorstAnalytic, std::abs(Point.Z - Height) / std::sqrt(1.0 + Across * Across + Along * Along));
                }
            }
            Tally(SurfacingGate, WorstPoint * 1000.0, WorstPoint <= 5.0e-4, "sheet vs all " + std::to_string(PointCount) + " limit points (6 sections + 5 loops never lofted), worst [mm] (value)");
            Tally(SurfacingGate, WorstAnalytic * 1000.0, WorstAnalytic <= 5.0e-4, "sheet vs the analytic bonnet H(x, y), 61 × 61 normal distance, worst [mm] (value)");

            // Reload: points and pole edits bit-equal, save → load → save byte-equal, same face name.
            const std::string Text = EncodeSolidArcDocument(Vehicle);
            const SolidArcDocumentFile File = DecodeSolidArcDocument(Text);
            SolidArcDocument Reopened = CreateSolidArcDocument(File.Title);
            const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Reopened, File.Tree, File.Captions, "Open");
            bool SameSteps = File.Tree.Steps.size() == Vehicle.Tree.Steps.size();
            for (size_t Index = 0; SameSteps && Index < File.Tree.Steps.size(); ++Index)
            {
                const FeatureStep& First = Vehicle.Tree.Steps[Index];
                const FeatureStep& Second = File.Tree.Steps[Index];
                SameSteps = First.Kind == Second.Kind && First.Sections == Second.Sections && First.Points.size() == Second.Points.size()
                         && std::memcmp(First.Points.data(), Second.Points.data(), First.Points.size() * sizeof(Vector3)) == 0;
            }
            Tally(SurfacingGate, 0.0, Opened.AcceptedCondition && SameSteps && EncodeSolidArcDocument(Reopened) == Text && Reopened.Regeneration.Naming.Faces == Vehicle.Regeneration.Naming.Faces,
                  "reload: points bit-equal, sections equal, save → load → save byte-equal, face name " + Vehicle.Regeneration.Naming.Faces.begin()->second);

            // Pole move (mode 1 drag): +10 mm on the middle pole of Section3, then undo / redo.
            const std::string Third = Sections[2];
            const NurbsCurveDefinition Before = *Vehicle.Regeneration.Curves.at(Third).Curve.Definition;
            const int     Pole   = static_cast<int>(Before.Poles.size()) / 2;
            const Vector3 Middle = EvaluateSurface(Surface, { 0.5, 0.38 }).Position;
            const SolidArcCommandOutcome Moved = MoveSolidArcPole(Vehicle, Third, Pole, Before.Poles[Pole] + Vector3{ 0.0, 0.0, 0.01 });
            const double Shift = Moved.AcceptedCondition ? Length(EvaluateSurface(SheetSurface(Vehicle), { 0.5, 0.38 }).Position - Middle) : 0.0;
            Tally(SurfacingGate, 0.0, Moved.AcceptedCondition && Vehicle.Summary.SheetCondition && Shift > 1.0e-3 && Vehicle.Regeneration.Naming.Faces == Reopened.Regeneration.Naming.Faces,
                  "move pole " + std::to_string(Pole) + " of Section3 by +10 mm → loft regenerates, panel at (0.5, 0.38) moved " + std::to_string(Shift * 1000.0).substr(0, 5) + " mm, face name kept");
            const SolidArcCommandOutcome Missing = MoveSolidArcPole(Vehicle, Third, 99, Vector3{});
            Tally(SurfacingGate, 0.0, !Missing.AcceptedCondition && Missing.Refusal.find("does not exist") != std::string::npos, "pole 99 refused: " + Missing.Refusal);
            const bool Undone = UndoSolidArcCommand(Vehicle);
            const double Back = Length(EvaluateSurface(SheetSurface(Vehicle), { 0.5, 0.38 }).Position - Middle);
            const bool Redone = RedoSolidArcCommand(Vehicle);
            const double Reapplied = Length(EvaluateSurface(SheetSurface(Vehicle), { 0.5, 0.38 }).Position - Middle);
            Tally(SurfacingGate, Back, Undone && Redone && Back == 0.0 && std::abs(Reapplied - Shift) <= 1.0e-15, "undo → panel exactly back · redo → moved again");
        }
    }

    // ---- W12 vehicle panels (plan V5–V7): continuity, mirror, close, network through the document commands.
    {
        auto SamplePath = [](const char* FileName)
        {
            std::string Path = SOLIDARC_VEHICLE_SAMPLE;
            return Path.substr(0, Path.rfind('/') + 1) + FileName;
        };
        auto ByCaption = [](SolidArcDocument& Document, const std::string& Stem)
        {
            std::vector<std::string> Labels;
            for (const FeatureStep& Step : Document.Tree.Steps) if (Document.Captions[Step.Label].rfind(Stem, 0) == 0) Labels.push_back(Step.Label);
            return Labels;
        };
        auto Reloads = [](const SolidArcDocument& Document)
        {
            const std::string Text = EncodeSolidArcDocument(Document);
            const SolidArcDocumentFile File = DecodeSolidArcDocument(Text);
            SolidArcDocument Reopened = CreateSolidArcDocument(File.Title);
            const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Reopened, File.Tree, File.Captions, "Open");
            return Opened.AcceptedCondition && EncodeSolidArcDocument(Reopened) == Text && Reopened.Regeneration.Naming.Faces == Document.Regeneration.Naming.Faces
                && Reopened.Summary.Volume == Document.Summary.Volume;
        };

        // Bonnet: front loft, rear loft G2 to it, mirror through x = 0.
        SolidArcDocument Bonnet = CreateSolidArcDocument("Bonnet panels");
        Expect(ImportSolidArcCurves(Bonnet, ReadVehicleCurves(SOLIDARC_VEHICLE_SAMPLE)));
        const std::vector<std::string> Sections = ByCaption(Bonnet, "Section");
        const SolidArcCommandOutcome Front = LoftSolidArcCurves(Bonnet, { Sections[0], Sections[1], Sections[2] });
        const SolidArcCommandOutcome Rear  = LoftSolidArcCurves(Bonnet, { Sections[2], Sections[3], Sections[4], Sections[5] }, 2);
        Tally(PanelGate, 0.0, Front.AcceptedCondition && Rear.AcceptedCondition && Bonnet.Regeneration.Panels.size() == 2 && Bonnet.Summary.SheetCondition && Bonnet.Summary.SoundCondition
              && Bonnet.Summary.Faces == 2, "front loft + rear loft G2 → 2 panels sewn on Section3, sheet sound " + Front.Refusal + Rear.Refusal);
        const SolidArcCommandOutcome Mirrored = MirrorSolidArcPanels(Bonnet, 0);
        const auto& Names = Bonnet.Regeneration.Panels;
        const std::string FrontName = Bonnet.Tree.Steps[11].Label + ".Side(Surface)";
        const std::string ImageName = Bonnet.Tree.Steps.back().Label + ".Image(" + FrontName + ")";
        Tally(PanelGate, 0.0, Mirrored.AcceptedCondition && Names.size() == 4 && Names.contains(ImageName) && Bonnet.Summary.SheetCondition && Bonnet.Summary.SoundCondition && Bonnet.Summary.Faces == 4,
              "Alt+X → 4 panels, one sound sheet, face " + ImageName + " " + Mirrored.Refusal);
        Tally(PanelGate, 0.0, Reloads(Bonnet), "bonnet reload: continuity + mirror steps survive, save → load → save byte-equal, names equal");
        const SolidArcCommandOutcome Capped = CloseSolidArcPanels(Bonnet);
        Tally(PanelGate, 0.0, !Capped.AcceptedCondition && Capped.Refusal.find("not planar") != std::string::npos && Names.size() == 4, "Close on the open bonnet rim refused, document kept: " + Capped.Refusal);
        const bool Undone = UndoSolidArcCommand(Bonnet);
        Tally(PanelGate, 0.0, Undone && Bonnet.Regeneration.Panels.size() == 2 && Bonnet.Summary.Faces == 2, "undo mirror → 2 panels");

        // Pod: planar hoops, loft, mirror, close → solid.
        SolidArcDocument Pod = CreateSolidArcDocument("Pod");
        const VehicleCurveFile Hoops = ReadVehicleCurves(SamplePath("SyntheticPod.toml"));
        const bool Planar = !Hoops.Curves.empty() && std::all_of(Hoops.Curves.begin(), Hoops.Curves.end(), [](const VehicleCurve& Curve) { return Curve.PlanarCondition; })
                         && DecodeVehicleCurves(EncodeVehicleCurves(Hoops)).Curves.front().PlanarCondition;
        Expect(ImportSolidArcCurves(Pod, Hoops));
        const SolidArcCommandOutcome Loft = LoftSolidArcCurves(Pod, ByCaption(Pod, "Hoop"));
        const SolidArcCommandOutcome Image = MirrorSolidArcPanels(Pod, 0);
        const SolidArcCommandOutcome Closed = CloseSolidArcPanels(Pod);
        double Exact = 0.0;
        for (int Index = 0; Index <= 2000; ++Index)
        {
            const double Y = 1.6 * Index / 2000.0, Weight = (Index == 0 || Index == 2000) ? 1.0 : (Index % 2 ? 4.0 : 2.0);
            Exact += Weight * Pi * (0.4 + 0.1 * std::sin(Pi * Y / 1.6)) * (0.3 + 0.05 * Y) * 1.6 / 6000.0;
        }
        const double Relative = std::abs(Pod.Summary.Volume - Exact) / Exact;
        Tally(PanelGate, Relative, Planar && Loft.AcceptedCondition && Image.AcceptedCondition && Closed.AcceptedCondition && !Pod.Summary.SheetCondition && Pod.Summary.SoundCondition && Relative <= 2.0e-3
              && Pod.Summary.Faces == 4, "pod: Planar = true round trip · loft → mirror → Close → solid V = " + std::to_string(Pod.Summary.Volume).substr(0, 8) + " vs " + std::to_string(Exact).substr(0, 8)
              + ", rel. (value) " + Loft.Refusal + Image.Refusal + Closed.Refusal);
        const SolidArcCommandOutcome Again = CloseSolidArcPanels(Pod);
        Tally(PanelGate, 0.0, !Again.AcceptedCondition && Again.Refusal.find("already closed") != std::string::npos, "second Close refused: " + Again.Refusal);
        Tally(PanelGate, 0.0, Reloads(Pod), "pod reload: planar flags + close step, byte-equal, same volume");

        // Network: 6 sections + 4 spines cross → L makes a Gordon panel; G1 on a network refuses.
        SolidArcDocument Network = CreateSolidArcDocument("Network");
        Expect(ImportSolidArcCurves(Network, ReadVehicleCurves(SamplePath("SyntheticBonnetNetwork.toml"))));
        std::vector<std::string> Curves;
        for (const FeatureStep& Step : Network.Tree.Steps) Curves.push_back(Step.Label);
        const SolidArcCommandOutcome Refused = LoftSolidArcCurves(Network, Curves, 1);
        Tally(PanelGate, 0.0, !Refused.AcceptedCondition && Refused.Refusal.find("network") != std::string::npos, "G1 on crossing curves refused: " + Refused.Refusal);
        const SolidArcCommandOutcome Gordon = LoftSolidArcCurves(Network, Curves);
        double WorstAnalytic = 0.0;
        if (Gordon.AcceptedCondition)
        {
            const NurbsSurface& Surface = Network.Regeneration.Panels.begin()->second;
            for (int First = 0; First <= 60; ++First)
            {
                for (int Second = 0; Second <= 60; ++Second)
                {
                    const Vector3 Point = EvaluateNurbsSurface(Surface, { First / 60.0, Second / 60.0 }).Position;
                    const double  Xi = Point.X / 0.75, Eta = Point.Y / 1.1;
                    const double  Height = 0.78 + 0.16 * Eta - 0.06 * Eta * Eta - 0.085 * Xi * Xi + 0.012 * std::sin(Pi * Eta) * (1.0 - Xi * Xi);
                    const double  Across = (-0.17 * Xi - 0.024 * Xi * std::sin(Pi * Eta)) / 0.75;
                    const double  Along  = (0.16 - 0.12 * Eta + 0.012 * Pi * std::cos(Pi * Eta) * (1.0 - Xi * Xi)) / 1.1;
                    WorstAnalytic = std::max(WorstAnalytic, std::abs(Point.Z - Height) / std::sqrt(1.0 + Across * Across + Along * Along));
                }
            }
        }
        const bool NetworkStep = !Network.Tree.Steps.empty() && Network.Tree.Steps.back().Kind == FeatureStepKind::Network;
        Tally(PanelGate, WorstAnalytic * 1000.0, Gordon.AcceptedCondition && NetworkStep && Network.Summary.SheetCondition && Network.Summary.SoundCondition && Network.Summary.Faces == 1 && WorstAnalytic <= 5.0e-4,
              "10 crossing curves → Network step (Gordon), sheet sound, vs analytic H 61 × 61, worst [mm] (value) " + Gordon.Refusal);
        Tally(PanelGate, 0.0, Reloads(Network), "network reload byte-equal");
    }

    // ---- W13 surface evaluation (plan V8): deviation map and zebra phase through the document and scene functions.
    {
        auto Sections = [](SolidArcDocument& Document)
        {
            std::vector<std::string> Labels;
            for (const FeatureStep& Step : Document.Tree.Steps) if (Document.Captions[Step.Label].rfind("Section", 0) == 0) Labels.push_back(Step.Label);
            return Labels;
        };
        auto Domain = [](const NurbsSurface& Surface, bool AlongU) { return MeasureNurbsSurfaceDomain(*Surface.Definition, AlongU); };
        auto Compact = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3g", Value); return std::string(Text); };

        // ① deviation map on the six-section bonnet: every imported point, closest-point residual, lattice bound.
        SolidArcDocument Fitted = CreateSolidArcDocument("Deviation");
        Expect(ImportSolidArcCurves(Fitted, ReadVehicleCurves(SOLIDARC_VEHICLE_SAMPLE)));
        Expect(LoftSolidArcCurves(Fitted, Sections(Fitted)));
        const SolidArcDeviationMap Map = MeasureSolidArcDeviation(Fitted);
        size_t Imported = 0;
        for (const FeatureStep& Step : Fitted.Tree.Steps) if (Step.Kind == FeatureStepKind::SpaceCurve) Imported += Step.Points.size();
        const NurbsSurface& Sheet = Fitted.Regeneration.Panels.begin()->second;
        const NurbsDomain   AlongU = Domain(Sheet, true), AlongV = Domain(Sheet, false);
        double WorstResidual = 0.0, WorstLattice = 0.0;
        for (const SolidArcDeviationNeedle& Needle : Map.Needles)
        {
            const Vector2 Foot   = ProjectOntoNurbsSurface(Sheet, Needle.Point, { 0.5 * (AlongU.Low + AlongU.High), 0.5 * (AlongV.Low + AlongV.High) });
            const bool    Inside = Foot.X > AlongU.Low + 1.0e-9 && Foot.X < AlongU.High - 1.0e-9 && Foot.Y > AlongV.Low + 1.0e-9 && Foot.Y < AlongV.High - 1.0e-9;
            const Vector3 Normal = EvaluateNurbsNormal(Sheet, Foot);
            const Vector3 Gap    = Needle.Point - Needle.Foot;
            if (Inside && Needle.Distance > 1.0e-12) WorstResidual = std::max(WorstResidual, Length(Cross(Gap, Normal)) / Needle.Distance);
            double Lattice = std::numeric_limits<double>::infinity();
            for (int First = 0; First <= 60; ++First)
                for (int Second = 0; Second <= 60; ++Second)
                    Lattice = std::min(Lattice, Distance(EvaluateNurbsSurface(Sheet, { AlongU.Low + (AlongU.High - AlongU.Low) * First / 60.0,
                                                                                         AlongV.Low + (AlongV.High - AlongV.Low) * Second / 60.0 }).Position, Needle.Point));
            WorstLattice = std::max(WorstLattice, Needle.Distance - Lattice);
        }
        Tally(EvaluationGate, Map.Worst * 1000.0, Map.Needles.size() == Imported && Imported == 286 && Map.Worst <= 2.0e-4 && Map.Revision == Fitted.Revision,
              "deviation map, six-section loft: " + std::to_string(Map.Needles.size()) + " needles (one per imported point), worst [mm] (value), mean "
              + std::to_string(Map.Mean * 1000.0) + " mm — W11 measures the same sheet");
        Tally(EvaluationGate, 0.0, WorstResidual <= 1.0e-6, "interior needles run along the foot normal: |gap × N| / |gap| = " + Compact(WorstResidual) + " (≤ 10⁻⁶)");
        Tally(EvaluationGate, 0.0, WorstLattice <= 1.0e-9, "no needle longer than the best of a 61 × 61 lattice: excess " + Compact(WorstLattice * 1000.0) + " mm");

        // ② zebra: lift pole 2 of Section3 by 20 mm, front + rear lofts; phase across the shared Section3 at G0 and G2.
        auto SeamJump = [&](const SolidArcDocument& Document)
        {
            const Vector3 Forward = Normalize(Vector3{ -1.0, 1.0, -1.0 }), Up = Normalize(Vector3{ -1.0, 1.0, 2.0 });
            std::vector<const NurbsSurface*> Panels;
            for (const auto& [Name, Surface] : Document.Regeneration.Panels) Panels.push_back(&Surface);
            if (Panels.size() != 2) return std::numeric_limits<double>::infinity();
            double Worst = 0.0;
            for (int Edge = 0; Edge < 2; ++Edge)
            {
                const NurbsDomain Across = Domain(*Panels[0], false), Along = Domain(*Panels[0], true);
                const double      Fixed  = Edge ? Across.High : Across.Low;
                const Vector3     Probe  = EvaluateNurbsSurface(*Panels[0], { 0.5 * (Along.Low + Along.High), Fixed }).Position;
                const Vector2     Near   = ProjectOntoNurbsSurface(*Panels[1], Probe, { 0.5, 0.5 });
                if (Distance(EvaluateNurbsSurface(*Panels[1], Near).Position, Probe) > 1.0e-6) continue;  // not the shared edge
                for (int Index = 1; Index < 40; ++Index)
                {
                    const Vector2 Here  = { Along.Low + (Along.High - Along.Low) * Index / 40.0, Fixed };
                    const Vector3 Point = EvaluateNurbsSurface(*Panels[0], Here).Position;
                    const Vector2 There = ProjectOntoNurbsSurface(*Panels[1], Point, Near);
                    Worst = std::max(Worst, std::abs(SolidArcZebraPhase(EvaluateNurbsNormal(*Panels[0], Here), Forward, Up, 64.0)
                                                   - SolidArcZebraPhase(EvaluateNurbsNormal(*Panels[1], There), Forward, Up, 64.0)));
                }
            }
            return Worst;
        };
        SolidArcDocument Creased = CreateSolidArcDocument("Zebra");
        Expect(ImportSolidArcCurves(Creased, ReadVehicleCurves(SOLIDARC_VEHICLE_SAMPLE)));
        const std::vector<std::string> Labels = Sections(Creased);
        const Vector3 Pole = Creased.Regeneration.Curves.at(Labels[2]).Curve.Definition->Poles[2];
        Expect(MoveSolidArcPole(Creased, Labels[2], 2, Pole + Vector3{ 0.0, 0.0, 0.02 }));
        Expect(LoftSolidArcCurves(Creased, { Labels[0], Labels[1], Labels[2] }));
        const SolidArcCommandOutcome Plain = LoftSolidArcCurves(Creased, { Labels[2], Labels[3], Labels[4], Labels[5] }, 0);
        const double PlainJump = SeamJump(Creased);
        Tally(EvaluationGate, 0.0, Plain.AcceptedCondition && PlainJump > 0.05 && PlainJump < 1.0e3,
              "G0 rear loft on the lifted Section3: zebra phase jump across the seam " + Compact(PlainJump) + " stripes of 64 (> 0.05) — visibly broken stripes");
        (void)UndoSolidArcCommand(Creased);
        const SolidArcCommandOutcome Smooth = LoftSolidArcCurves(Creased, { Labels[2], Labels[3], Labels[4], Labels[5] }, 2);
        const double SmoothJump = SeamJump(Creased);
        Tally(EvaluationGate, 0.0, Smooth.AcceptedCondition && SmoothJump <= 1.0e-6, "G2 rear loft: zebra phase jump " + Compact(SmoothJump) + " stripes of 64 (≤ 10⁻⁶) — continuous stripes");

        // ③ zebra density tessellation: interpolated normals need short sides on the sheet.
        const SolidArcScene Coarse = BuildSolidArcScene(Creased);
        const SolidArcScene Fine   = BuildSolidArcScene(Creased, 1.5e-3, true);
        const size_t CoarseCount = Coarse.Corners.size() / 3, FineCount = Fine.Corners.size() / 3;
        Tally(EvaluationGate, 0.0, FineCount >= 20 * CoarseCount && Fine.Generation != Coarse.Generation && Fine.Revision == Coarse.Revision,
              "zebra tessellation " + std::to_string(FineCount) + " vs " + std::to_string(CoarseCount) + " triangles (≥ 20×); distinct scene generation re-uploads the GPU buffers");
    }

    // ---- W14 blueprint study: image planes from the committed BMW trace, picture decode, codec, refusals, axis cylinders.
    {
        auto Compact = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3g", Value); return std::string(Text); };
        auto Find    = [](const std::vector<SolidArcImagePlane>& Planes, const std::string& Name) -> const SolidArcImagePlane*
        {
            for (const SolidArcImagePlane& Plane : Planes) if (Plane.Name == Name) return &Plane;
            return nullptr;
        };
        // Crop pixel (column, row) → world through the plane's parallelogram.
        auto Pixel = [](const SolidArcImagePlane& Plane, double Column, double Row)
        {
            return LocateSolidArcImagePoint(Plane, (Column - Plane.Crop[0]) / double(Plane.Crop[2] - Plane.Crop[0]), (Row - Plane.Crop[1]) / double(Plane.Crop[3] - Plane.Crop[1]));
        };

        // ① landmarks of BmwM3E90Calibration.toml land on the car frame (nose y 0, tail y 4.58, ground z 0, mirror plane x 0).
        std::string Sample = SOLIDARC_VEHICLE_SAMPLE;
        Sample = Sample.substr(0, Sample.find_last_of("/\\") + 1) + "BmwM3E90Sections.toml";
        const VehicleCurveFile Bmw = ReadVehicleCurves(Sample);
        const SolidArcImagePlane* Side  = Find(Bmw.ImagePlanes, "Side");
        const SolidArcImagePlane* Top   = Find(Bmw.ImagePlanes, "Top");
        const SolidArcImagePlane* Front = Find(Bmw.ImagePlanes, "Front");
        // The side view sets the scale (4580 mm over its bumper lines), so its landmarks are exact; the top / front views of
        // the sheet agree with it to about one pixel (2.08 mm) — the drawing's own alignment, gated at 1.5 px.
        constexpr double Millimetre = 2.0771e-3;                                             // [m/px] - Calibration scale
        double Landmark = INFINITY, Alignment = INFINITY;
        if (Side && Top && Front)
        {
            Landmark  = std::max({ std::abs(Pixel(*Side, 30, 1642).Y - 0.0), std::abs(Pixel(*Side, 2235, 1642).Y - 4.58), std::abs(Pixel(*Side, 30, 1642).Z) });
            Alignment = std::max({ std::abs(Pixel(*Top, 25, 2388).Y), std::abs(Pixel(*Top, 2231, 2388).Y - 4.58), std::abs(Pixel(*Top, 25, 2388).X),
                                   std::abs(Pixel(*Front, 510, 727).X), std::abs(Pixel(*Front, 510, 727).Z) });
        }
        Tally(BlueprintGate, Landmark, Bmw.Refusal.empty() && Bmw.Curves.size() == 24 && Bmw.ImagePlanes.size() == 3 && Landmark <= 1.0e-5,
              "BMW trace: 24 sections + 3 planes; side bumper / ground pixels → y 0, y 4.58, z 0, worst [m] (value)");
        Tally(BlueprintGate, 0.0, Alignment <= 1.5 * Millimetre,
              "top nose / tail / centre line and front centre / ground pixels → world within " + Compact(Alignment * 1000.0) + " mm (sheet alignment, ≤ 1.5 px)");
        const bool Facing = Side && Top && Front && std::abs(Side->Corner.X + 1.2) <= 1.0e-12 && Side->Right.X == 0.0 && Side->Down.X == 0.0
                         && Top->Right.Z == 0.0 && Top->Down.Z == 0.0 && Front->Right.Y == 0.0 && Front->Down.Y == 0.0 && Top->Corner.Z < 0.0 && Front->Corner.Y > 4.58;
        Tally(BlueprintGate, 0.0, Facing, "Side ⟂ x at x = −1.2 (behind, seen from Right) · Top ⟂ z under the ground · Front ⟂ y behind the tail");

        // ② picture decode: a 3 × 2 24-bit BMP written here (bottom-up rows) → RGBA8, top row first; a missing file refuses.
        const std::string Bitmap = (std::filesystem::temp_directory_path() / "SolidArcImagePlaneValidation.bmp").string();
        {
            const uint8_t Colours[2][3][3] = { { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 } }, { { 10, 20, 30 }, { 40, 50, 60 }, { 70, 80, 90 } } };   // [row][column] RGB, top row first
            std::vector<uint8_t> Bytes(54, 0);
            const uint32_t Stride = 12, Size = 54 + Stride * 2;
            auto Put = [&](size_t At, uint32_t Value, int Count) { for (int Index = 0; Index < Count; ++Index) Bytes[At + Index] = uint8_t(Value >> (8 * Index)); };
            Bytes[0] = 'B'; Bytes[1] = 'M'; Put(2, Size, 4); Put(10, 54, 4); Put(14, 40, 4); Put(18, 3, 4); Put(22, 2, 4); Put(26, 1, 2); Put(28, 24, 2); Put(34, Stride * 2, 4);
            for (int Row = 1; Row >= 0; --Row)
            {
                for (int Column = 0; Column < 3; ++Column) for (int Channel = 2; Channel >= 0; --Channel) Bytes.push_back(Colours[Row][Column][Channel]);
                for (uint32_t Pad = 9; Pad < Stride; ++Pad) Bytes.push_back(0);
            }
            std::ofstream(Bitmap, std::ios::binary).write(reinterpret_cast<const char*>(Bytes.data()), std::streamsize(Bytes.size()));
            const SolidArcPicture Picture = DecodeSolidArcPicture(Bitmap);
            bool Exact = Picture.Refusal.empty() && Picture.Width == 3 && Picture.Height == 2 && Picture.Pixels.size() == 24;
            for (int Row = 0; Exact && Row < 2; ++Row)
                for (int Column = 0; Column < 3; ++Column)
                    for (int Channel = 0; Channel < 3; ++Channel) Exact = Exact && Picture.Pixels[(Row * 3 + Column) * 4 + Channel] == Colours[Row][Column][Channel];
            Tally(BlueprintGate, 0.0, Exact && Picture.Pixels[3] == 255, "3 × 2 BMP → RGBA8, every pixel exact, top row first, alpha 255");
            std::filesystem::remove(Bitmap);
            const SolidArcPicture Missing = DecodeSolidArcPicture(Bitmap);
            Tally(BlueprintGate, 0.0, !Missing.Refusal.empty() && Missing.Pixels.empty(), "missing picture refused (no throw)");
        }

        // ③ import keeps the planes beside the curves; a second import replaces planes by Name; codec round trip byte-equal.
        SolidArcDocument Study = CreateSolidArcDocument("Blueprint");
        const SolidArcCommandOutcome First  = ImportSolidArcCurves(Study, Bmw);
        const size_t                 Steps  = Study.Tree.Steps.size();
        const SolidArcCommandOutcome Second = ImportSolidArcCurves(Study, Bmw);
        Tally(BlueprintGate, 0.0, First.AcceptedCondition && Second.AcceptedCondition && Steps == 24 && Study.Tree.Steps.size() == 48 && Study.ImagePlanes.size() == 3,
              "import → 24 curves + 3 planes; re-import adds curves, replaces planes by Name (still 3)");
        Study.ImagePlanes[1].VisibleCondition = false;
        Study.ImagePlanes[2].Opacity          = 0.3;
        const std::string            Encoded = EncodeSolidArcDocument(Study);
        const SolidArcDocumentFile   Decoded = DecodeSolidArcDocument(Encoded);
        SolidArcDocument             Again   = CreateSolidArcDocument(Decoded.Title);
        const SolidArcCommandOutcome Opened  = ReplaceSolidArcTree(Again, Decoded.Tree, Decoded.Captions, "Open");
        Again.ImagePlanes = Decoded.ImagePlanes;
        Tally(BlueprintGate, 0.0, Decoded.Refusal.empty() && Opened.AcceptedCondition && Decoded.ImagePlanes.size() == 3 && !Decoded.ImagePlanes[1].VisibleCondition
              && Decoded.ImagePlanes[2].Opacity == 0.3 && EncodeSolidArcDocument(Again) == Encoded, "document with planes (one hidden, one at ink 0.3): save → load → save byte-equal");
        const std::string Plain = EncodeSolidArcDocument(CreateSolidArcDocument("Plain"));
        Tally(BlueprintGate, 0.0, Plain.find("ImagePlanes") == std::string::npos, "documents without planes write no ImagePlanes key (older files unchanged)");

        // ④ refusals: a degenerate plane or an opacity outside [0, 1] rejects the whole curves file (no partial import).
        const std::string Curve = "Format = \"VehicleCurves\"\nUnits = \"m\"\n[[Curves]]\nName = \"Line\"\nPoints = [[0, 0, 0], [1, 0, 0], [2, 0, 0.5]]\n";
        auto PlaneRow = [](const std::string& Right, double Opacity)
        {
            return "[[ImagePlanes]]\nName = \"Side\"\nPath = \"Side.png\"\nCorner = [0, 0, 1]\nRight = " + Right + "\nDown = [0, 0, -1]\nOpacity = " + std::to_string(Opacity) + "\n";
        };
        const VehicleCurveFile Fine     = DecodeVehicleCurves(Curve + PlaneRow("[0, 4, 0]", 0.5));
        const VehicleCurveFile Flat     = DecodeVehicleCurves(Curve + PlaneRow("[0, 0, -2]", 0.5));
        const VehicleCurveFile Overdone = DecodeVehicleCurves(Curve + PlaneRow("[0, 4, 0]", 1.5));
        Tally(BlueprintGate, 0.0, Fine.Refusal.empty() && Fine.ImagePlanes.size() == 1 && !Flat.Refusal.empty() && Flat.Curves.empty() && !Overdone.Refusal.empty() && Overdone.Curves.empty(),
              "zero-area plane refused (" + Flat.Refusal + ") · opacity 1.5 refused — no curves kept");

        // ⑤ Construct · Cylinder · Axis x / y (wheel arches): exact volume πr²h and the extent along +axis from Position.
        for (int Axis = 0; Axis < 2; ++Axis)
        {
            SolidArcDocument Wheel = CreateSolidArcDocument("Wheel");
            SolidArcConstructRequest Request;
            Request.Kind     = SolidArcConstructKind::Cylinder;
            Request.Axis     = Axis;
            Request.Position = { 0.62, 0.805, 0.33 };
            Request.Radius   = 0.37;
            Request.Height   = 0.5;
            const SolidArcCommandOutcome Built = ConstructSolidArcPrimitive(Wheel, Request);
            const double Exact    = std::numbers::pi * 0.37 * 0.37 * 0.5;
            const double Relative = std::abs(Wheel.Summary.Volume - Exact) / Exact;
            const SolidArcScene Scene = BuildSolidArcScene(Wheel, 1.5e-3);
            const double Base   = Axis == 0 ? 0.62 : 0.805;
            const double Low    = Scene.BoundsLow[Axis], High = Scene.BoundsHigh[Axis];
            const double Across = Scene.BoundsHigh[2] - Scene.BoundsLow[2];
            const double Extent = std::max({ std::abs(Low - Base), std::abs(High - Base - 0.5), std::abs(Across - 0.74) });
            Tally(BlueprintGate, Relative, Built.AcceptedCondition && Wheel.Summary.SoundCondition && Relative <= 1.0e-9 && Extent <= 2.0e-3,
                  std::string(Axis == 0 ? "Axis x" : "Axis y") + " cylinder r 0.37 h 0.5: |V − πr²h| / V (value); " + Compact(Low) + " … " + Compact(High) + " along the axis, Ø " + Compact(Across) + " in z");
        }
    }

    // ---- W15 matcaps: built-in library optics, picture fitting, per-face / body assignment without regeneration, undo,
    //      persistence through regeneration, codec, refusals.
    {
        auto Compact = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3g", Value); return std::string(Text); };
        auto Luminance = [](const SolidArcPicture& Picture, size_t Texel)
        {
            return 0.2126 * Picture.Pixels[Texel * 4] + 0.7152 * Picture.Pixels[Texel * 4 + 1] + 0.0722 * Picture.Pixels[Texel * 4 + 2];
        };
        // Texels inside 0.9 of the disc radius.
        auto Inside = [](const SolidArcPicture& Picture, uint32_t Column, uint32_t Row)
        {
            const double X = 2.0 * (Column + 0.5) / Picture.Width - 1.0, Y = 2.0 * (Row + 0.5) / Picture.Height - 1.0;
            return X * X + Y * Y < 0.81;
        };
        // Mean |∂L/∂x| over mean |∂L/∂y| inside the disc (brushed metal blurs along x).
        auto Anisotropy = [&](const SolidArcPicture& Picture)
        {
            double Across = 0.0, Down = 0.0;
            for (uint32_t Row = 1; Row < Picture.Height; ++Row)
                for (uint32_t Column = 1; Column < Picture.Width; ++Column)
                {
                    if (!Inside(Picture, Column, Row)) continue;
                    const size_t Texel = size_t(Row) * Picture.Width + Column;
                    Across += std::abs(Luminance(Picture, Texel) - Luminance(Picture, Texel - 1));
                    Down   += std::abs(Luminance(Picture, Texel) - Luminance(Picture, Texel - Picture.Width));
                }
            return Across / std::max(Down, 1e-9);
        };
        // Colour of the brightest texel inside the disc.
        auto Brightest = [&](const SolidArcPicture& Picture)
        {
            size_t Best = 0; double Peak = -1.0;
            for (uint32_t Row = 0; Row < Picture.Height; ++Row)
                for (uint32_t Column = 0; Column < Picture.Width; ++Column)
                    if (Inside(Picture, Column, Row) && Luminance(Picture, size_t(Row) * Picture.Width + Column) > Peak)
                    {
                        Peak = Luminance(Picture, size_t(Row) * Picture.Width + Column);
                        Best = size_t(Row) * Picture.Width + Column;
                    }
            return std::array<double, 3>{ double(Picture.Pixels[Best * 4]), double(Picture.Pixels[Best * 4 + 1]), double(Picture.Pixels[Best * 4 + 2]) };
        };
        // Luminance spread (standard deviation) inside the disc: sharp reflections spread wide.
        auto Spread = [&](const SolidArcPicture& Picture)
        {
            double Sum = 0.0, Squares = 0.0; size_t Count = 0;
            for (uint32_t Row = 0; Row < Picture.Height; ++Row)
                for (uint32_t Column = 0; Column < Picture.Width; ++Column)
                    if (Inside(Picture, Column, Row))
                    {
                        const double Value = Luminance(Picture, size_t(Row) * Picture.Width + Column);
                        Sum += Value; Squares += Value * Value; ++Count;
                    }
            const double Mean = Sum / double(Count);
            return std::sqrt(std::max(0.0, Squares / double(Count) - Mean * Mean));
        };

        // ① the library: 13 built-ins, square RGBA8, opaque, deterministic; unknown names refuse
        const std::vector<SolidArcMatcapDescription>& BuiltIn = SolidArcBuiltInMatcaps();
        bool Shaped = BuiltIn.size() == 13, Repeatable = true;
        std::map<std::string, SolidArcPicture> Composed;
        for (const SolidArcMatcapDescription& Description : BuiltIn)
        {
            const SolidArcPicture Picture = ComposeSolidArcMatcap(Description.Name);
            Shaped = Shaped && Picture.Refusal.empty() && Picture.Width == SolidArcMatcapExtent && Picture.Height == SolidArcMatcapExtent
                  && Picture.Pixels.size() == size_t(SolidArcMatcapExtent) * SolidArcMatcapExtent * 4;
            for (size_t Index = 3; Shaped && Index < Picture.Pixels.size(); Index += 4) Shaped = Picture.Pixels[Index] == 255;
            Repeatable = Repeatable && ComposeSolidArcMatcap(Description.Name).Pixels == Picture.Pixels;
            Composed[Description.Name] = Picture;
        }
        const SolidArcPicture Unknown = ComposeSolidArcMatcap("Unobtainium");
        const SolidArcMatcapLibrary Plain = AssembleSolidArcMatcaps({});
        Tally(MatcapGate, 0.0, Shaped && Repeatable && !Unknown.Refusal.empty() && Plain.Names.size() == 14 && Plain.Names[0] == SolidArcStudioMatcap && Plain.Pictures.size() == 13
                               && SolidArcMatcapLayer(Plain, "Brass") == 5 && SolidArcMatcapLayer(Plain, "Unobtainium") == 0,
              "13 built-ins 256 × 256 RGBA8, opaque, composed twice bit-equal; library Studio + 13 (Brass = layer 5); unknown name refused / layer 0");

        // ② optics: metal highlights carry the tint, plastic highlights stay white; brushed blurs along x; roughness spreads less
        const std::array<double, 3> BrassPeak = Brightest(Composed["Brass"]);
        const std::array<double, 3> RedPeak   = Brightest(Composed["RedPlastic"]);
        const double BrassTint = BrassPeak[2] / BrassPeak[0];
        const double RedWhite  = std::min({ RedPeak[0], RedPeak[1], RedPeak[2] }) / std::max({ RedPeak[0], RedPeak[1], RedPeak[2] });
        Tally(MatcapGate, 0.0, BrassTint < 0.75 && RedWhite > 0.8,
              "brightest texel: Brass blue / red " + Compact(BrassTint) + " (< 0.75, tinted metal) · Red plastic min / max " + Compact(RedWhite) + " (> 0.8, white Fresnel)");
        const double Brushed = Anisotropy(Composed["BrushedSteel"]), Mirror = Anisotropy(Composed["Chrome"]);
        Tally(MatcapGate, 0.0, Brushed < 0.6 * Mirror, "|∂L/∂x| / |∂L/∂y|: Brushed steel " + Compact(Brushed) + " < 0.6 × Chrome " + Compact(Mirror) + " (anisotropic)");
        const double Sharp = Spread(Composed["Chrome"]), Satin = Spread(Composed["SatinAluminium"]), Matte = Spread(Composed["Clay"]);
        Tally(MatcapGate, 0.0, Sharp > Satin && Satin > Matte,
              "luminance spread inside the disc: Chrome " + Compact(Sharp) + " > Satin aluminium " + Compact(Satin) + " > Clay " + Compact(Matte));

        // ③ user pictures: centre square crop + bilinear fit; a 4 × 2 picture keeps its middle two columns
        const std::string Bitmap = (std::filesystem::temp_directory_path() / "SolidArcMatcapValidation.bmp").string();
        const uint8_t Columns[4][3] = { { 200, 0, 0 }, { 0, 200, 0 }, { 0, 0, 200 }, { 200, 200, 0 } };
        {
            std::vector<uint8_t> Bytes(54, 0);
            const uint32_t Stride = 12, Size = 54 + Stride * 2;
            auto Put = [&](size_t At, uint32_t Value, int Count) { for (int Index = 0; Index < Count; ++Index) Bytes[At + Index] = uint8_t(Value >> (8 * Index)); };
            Bytes[0] = 'B'; Bytes[1] = 'M'; Put(2, Size, 4); Put(10, 54, 4); Put(14, 40, 4); Put(18, 4, 4); Put(22, 2, 4); Put(26, 1, 2); Put(28, 24, 2); Put(34, Stride * 2, 4);
            for (int Row = 1; Row >= 0; --Row)
                for (int Column = 0; Column < 4; ++Column) for (int Channel = 2; Channel >= 0; --Channel) Bytes.push_back(Columns[Column][Channel]);
            std::ofstream(Bitmap, std::ios::binary).write(reinterpret_cast<const char*>(Bytes.data()), std::streamsize(Bytes.size()));
        }
        const SolidArcPicture Fitted = FitSolidArcMatcapPicture(DecodeSolidArcPicture(Bitmap));
        auto Near = [&](uint32_t Column, uint32_t Row, const uint8_t* Expected)
        {
            const uint8_t* Pixel = &Fitted.Pixels[(size_t(Row) * Fitted.Width + Column) * 4];
            return std::abs(Pixel[0] - Expected[0]) <= 1 && std::abs(Pixel[1] - Expected[1]) <= 1 && std::abs(Pixel[2] - Expected[2]) <= 1 && Pixel[3] == 255;
        };
        Tally(MatcapGate, 0.0, Fitted.Refusal.empty() && Fitted.Width == 256 && Fitted.Height == 256 && Near(64, 10, Columns[1]) && Near(192, 200, Columns[2]),
              "4 × 2 BMP → 256 × 256: quarter columns show source columns 2 and 3 (centre crop), within 1 / 255, opaque");

        // ④ assignment on a block: two faces Brass, body Brushed steel — no regeneration, one undo entry each
        SolidArcDocument Shop = CreateSolidArcDocument("Matcaps");
        SolidArcConstructRequest Stock;
        Stock.Kind = SolidArcConstructKind::Block; Stock.Width = 2.0; Stock.Depth = 1.0; Stock.Height = 0.5;
        Expect(ConstructSolidArcPrimitive(Shop, Stock));
        SolidArcScene ShopScene = BuildSolidArcScene(Shop);
        const std::string First = ShopScene.Faces.size() == 6 ? ShopScene.Faces[0].Name : "", Second = ShopScene.Faces.size() == 6 ? ShopScene.Faces[1].Name : "";
        const uint64_t Revision = Shop.Revision;
        const size_t   Entries  = Shop.UndoEntries.size();
        SelectSolidArcName(Shop, SolidArcSelectionKind::Face, First, false);
        SelectSolidArcName(Shop, SolidArcSelectionKind::Face, Second, true);
        const SolidArcCommandOutcome FaceOutcome = AssignSolidArcMatcap(Shop, "Brass");
        ClearSolidArcSelection(Shop);
        const SolidArcCommandOutcome BodyOutcome = AssignSolidArcMatcap(Shop, "BrushedSteel");
        const SolidArcMatcapLibrary Library = AssembleSolidArcMatcaps(Shop.Appearance);
        auto LayerCounts = [&](const SolidArcScene& Scene)
        {
            std::map<uint32_t, int> Counts;
            for (const uint32_t Word : ComposeSolidArcMatcapLayers(Scene, Shop.Appearance, Library)) Counts[Word >> SolidArcMatcapLayerShift] += 1;
            return Counts;
        };
        std::map<uint32_t, int> Counts = LayerCounts(ShopScene);
        const uint32_t BrassLayer = SolidArcMatcapLayer(Library, "Brass"), SteelLayer = SolidArcMatcapLayer(Library, "BrushedSteel");
        Tally(MatcapGate, 0.0, FaceOutcome.AcceptedCondition && BodyOutcome.AcceptedCondition && Shop.Revision == Revision && Shop.UndoEntries.size() == Entries + 2
                               && Shop.UndoEntries.back().AppearanceCondition && Counts[BrassLayer] == 2 && Counts[SteelLayer] == 4 && Counts.size() == 2,
              "2 faces → Brass, body → Brushed steel: state words 2 × layer " + std::to_string(BrassLayer) + " + 4 × layer " + std::to_string(SteelLayer)
              + "; revision unchanged (no regeneration); 2 matcap-only undo entries");

        // ⑤ refusals leave the appearance and the history alone
        const SolidArcAppearance Kept = Shop.Appearance;
        const SolidArcCommandOutcome Unnamed = AssignSolidArcMatcap(Shop, "Unobtainium");
        const SolidArcCommandOutcome Again   = AssignSolidArcMatcap(Shop, "BrushedSteel");
        const bool Untouched = Shop.Appearance.BodyMatcap == Kept.BodyMatcap && Shop.Appearance.FaceMatcaps == Kept.FaceMatcaps && Shop.UndoEntries.size() == Entries + 2;
        Tally(MatcapGate, 0.0, !Unnamed.AcceptedCondition && !Again.AcceptedCondition && Untouched,
              "unknown matcap refused (" + Unnamed.Refusal + ") · repeat refused (" + Again.Refusal + ") · appearance and history unchanged");

        // ⑥ undo / redo restore the appearance exactly and never regenerate
        UndoSolidArcCommand(Shop);
        const bool BodyUndone = Shop.Appearance.BodyMatcap == SolidArcStudioMatcap && Shop.Appearance.FaceMatcaps.size() == 2;
        UndoSolidArcCommand(Shop);
        const bool FacesUndone = Shop.Appearance.FaceMatcaps.empty() && Shop.Revision == Revision;
        RedoSolidArcCommand(Shop);
        RedoSolidArcCommand(Shop);
        Tally(MatcapGate, 0.0, BodyUndone && FacesUndone && Shop.Appearance.BodyMatcap == Kept.BodyMatcap && Shop.Appearance.FaceMatcaps == Kept.FaceMatcaps && Shop.Revision == Revision,
              "undo → body Studio, undo → no face matcaps, redo × 2 → identical appearance; revision unchanged throughout");

        // ⑦ a parameter edit regenerates: assignments follow the persistent names (none dangling)
        const SolidArcCommandOutcome Deeper = AssignSolidArcParameter(Shop, "Block1.Depth", 1.5);
        ShopScene = BuildSolidArcScene(Shop);
        Counts    = LayerCounts(ShopScene);
        Tally(MatcapGate, 0.0, Deeper.AcceptedCondition && Shop.Revision != Revision && Counts[BrassLayer] == 2 && Counts[SteelLayer] == 4 && CountDanglingMatcaps(ShopScene, Shop.Appearance) == 0,
              "Block1.Depth 1 → 1.5 regenerates; still 2 Brass + 4 Brushed steel faces, 0 dangling assignments");

        // ⑧ Alt+M: the selected face returns to the body matcap; with nothing selected the body returns to Studio
        SelectSolidArcName(Shop, SolidArcSelectionKind::Face, First, false);
        const SolidArcCommandOutcome FaceRemoval = RemoveSolidArcMatcap(Shop);
        ClearSolidArcSelection(Shop);
        const SolidArcCommandOutcome BodyRemoval = RemoveSolidArcMatcap(Shop);
        const SolidArcCommandOutcome Nothing     = RemoveSolidArcMatcap(Shop);
        Tally(MatcapGate, 0.0, FaceRemoval.AcceptedCondition && BodyRemoval.AcceptedCondition && !Nothing.AcceptedCondition && Shop.Appearance.FaceMatcaps.size() == 1
                               && Shop.Appearance.FaceMatcaps.count(Second) == 1 && Shop.Appearance.BodyMatcap == SolidArcStudioMatcap,
              "remove on 1 face → 1 override left; remove on the body → Studio; a third remove refused (" + Nothing.Refusal + ")");

        // ⑨ user picture: added by name (layer 15), refused when missing / named twice; saved → loaded → saved byte-equal
        const SolidArcCommandOutcome Added     = AddSolidArcMatcapPicture(Shop, "ShopStripes", Bitmap);
        const SolidArcCommandOutcome Twice     = AddSolidArcMatcapPicture(Shop, "ShopStripes", Bitmap);
        const SolidArcCommandOutcome BuiltName = AddSolidArcMatcapPicture(Shop, "Chrome", Bitmap);
        const SolidArcCommandOutcome Absent    = AddSolidArcMatcapPicture(Shop, "Absent", Bitmap + ".missing");
        ClearSolidArcSelection(Shop);
        const SolidArcCommandOutcome Striped   = AssignSolidArcMatcap(Shop, "ShopStripes");
        const SolidArcMatcapLibrary  Custom    = AssembleSolidArcMatcaps(Shop.Appearance);
        Tally(MatcapGate, 0.0, Added.AcceptedCondition && !Twice.AcceptedCondition && !BuiltName.AcceptedCondition && !Absent.AcceptedCondition && Striped.AcceptedCondition
                               && Custom.Names.size() == 15 && SolidArcMatcapLayer(Custom, "ShopStripes") == 14 && Custom.Refusals.back().empty() && Custom.Pictures.back().Pixels == Fitted.Pixels,
              "picture ShopStripes → layer 14 of 15, fitted pixels equal; twice / built-in name / missing file refused (" + Absent.Refusal + ")");
        const std::string Written = EncodeSolidArcDocument(Shop);
        const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
        SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
        Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
        RegenerateSolidArcDocument(Copy, true);
        SolidArcDocument Bare = CreateSolidArcDocument("Bare");
        Expect(ConstructSolidArcPrimitive(Bare, Stock));
        Tally(MatcapGate, 0.0, Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Written.find("[Appearance]") != std::string::npos
                               && Copy.Appearance.FaceMatcaps == Shop.Appearance.FaceMatcaps && EncodeSolidArcDocument(Bare).find("Appearance") == std::string::npos,
              "save → load → save byte-equal with [Appearance]; documents without matcaps write no Appearance key");
        std::string Corrupt = Written;
        const size_t Brass = Corrupt.find("Matcap = 'Brass'");
        if (Brass != std::string::npos) Corrupt.replace(Brass, 16, "Matcap = 'Unobtainium'");
        const SolidArcDocumentFile Rejected = DecodeSolidArcDocument(Corrupt);
        Tally(MatcapGate, 0.0, Brass != std::string::npos && !Rejected.Refusal.empty(), "a file naming an unknown matcap is refused: " + Rejected.Refusal);
        std::filesystem::remove(Bitmap);
    }

    // ---- W16 multi-body: Plasticity R (new body) tiles, per-body matcaps, Q Boolean, targeted commands, refusals, codec.
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Figures  = [](const SolidArcDocument& Target, const std::string& Key) -> const SolidArcBodyFigures*
        {
            for (const SolidArcBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return &Body;
            return nullptr;
        };
        auto VolumeOf = [&](const SolidArcDocument& Target, const std::string& Key) { const SolidArcBodyFigures* Body = Figures(Target, Key); return Body ? Body->Volume : -1.0; };

        // ① three bodies: Block1 4 × 2 × 1, Cylinder1 r 0.5 h 2 through it (New body), Block2 unit cube at x = 6 (New body)
        SolidArcDocument Parts = CreateSolidArcDocument("Parts");
        SolidArcConstructRequest Request;
        Request.Kind = SolidArcConstructKind::Block; Request.Width = 4.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Parts, Request));
        Request = {};
        Request.Kind = SolidArcConstructKind::Cylinder; Request.Position = { 1.0, 1.0, -0.5 }; Request.Radius = 0.5; Request.Height = 2.0;
        Request.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Parts, Request));
        Request = {};
        Request.Kind = SolidArcConstructKind::Block; Request.Position = { 6.0, 0.0, 0.0 }; Request.Width = 1.0; Request.Depth = 1.0; Request.Height = 1.0;
        Request.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Parts, Request));
        const bool Three = Parts.Summary.Bodies.size() == 3;
        const std::string BlockKey    = Three ? Parts.Summary.Bodies[0].Key : std::string{};
        const std::string CylinderKey = Three ? Parts.Summary.Bodies[1].Key : std::string{};
        const std::string CubeKey     = Three ? Parts.Summary.Bodies[2].Key : std::string{};
        const double CylinderVolume = Pi * 0.25 * 2.0;
        const double Worst = std::max({ Relative(VolumeOf(Parts, BlockKey), 8.0), Relative(VolumeOf(Parts, CylinderKey), CylinderVolume), Relative(VolumeOf(Parts, CubeKey), 1.0),
                                        Relative(Parts.Summary.Volume, 9.0 + CylinderVolume) });
        std::string Captions;
        for (const SolidArcBodyFigures& Body : Parts.Summary.Bodies) Captions += (Captions.empty() ? "" : " · ") + Body.Caption + " (" + Body.Key + ")";
        Tally(BodiesGate, Worst, Three && Worst <= 1.0e-9 && Parts.Summary.SoundCondition && Parts.Regeneration.Active == CubeKey,
              "3 bodies " + Captions + ": 8 · π/2 · 1 exact, overlapping solids stay separate, all sound, active = the last created");

        // ② scene: one span per body, faces and edges tagged; picking the cylinder body marks exactly its faces
        SolidArcScene PartsScene = BuildSolidArcScene(Parts);
        bool Spans = PartsScene.Bodies.size() == 3;
        uint32_t FaceSum = 0;
        for (size_t Index = 0; Spans && Index < PartsScene.Bodies.size(); ++Index)
        {
            const SolidArcSceneBody& Body = PartsScene.Bodies[Index];
            Spans = Spans && Body.Key == Parts.Summary.Bodies[Index].Key && int(Body.FaceCount) == Parts.Summary.Bodies[Index].Faces && Body.FirstFace == FaceSum;
            for (uint32_t Face = Body.FirstFace; Face < Body.FirstFace + Body.FaceCount; ++Face) Spans = Spans && PartsScene.Faces[Face].Body == Index;
            FaceSum += Body.FaceCount;
        }
        SelectSolidArcName(Parts, SolidArcSelectionKind::Body, CylinderKey, false);
        const SolidArcSceneStates Picked = ComposeSolidArcStates(PartsScene, Parts.Selection, 0u);
        const long Marked = std::count_if(Picked.Faces.begin(), Picked.Faces.end(), [](uint32_t Word) { return (Word & 1u) != 0; });
        Tally(BodiesGate, 0.0, Spans && FaceSum == PartsScene.Faces.size() && Three && Marked == Parts.Summary.Bodies[1].Faces,
              "scene: 3 body spans in creation order, face counts equal the audit, every face tagged; body pick marks its " + std::to_string(Marked) + " faces only");

        // ③ per-body matcaps: Block1 → Brushed steel, Cylinder1 → Brass, one Block1 face → Red plastic; Block2 keeps the default
        const uint64_t Revision = Parts.Revision;
        SelectSolidArcName(Parts, SolidArcSelectionKind::Body, BlockKey, false);
        const SolidArcCommandOutcome Steel = AssignSolidArcMatcap(Parts, "BrushedSteel");
        SelectSolidArcName(Parts, SolidArcSelectionKind::Body, CylinderKey, false);
        const SolidArcCommandOutcome Brass = AssignSolidArcMatcap(Parts, "Brass");
        const std::string RedFace = PartsScene.Faces[PartsScene.Bodies[0].FirstFace].Name;
        SelectSolidArcName(Parts, SolidArcSelectionKind::Face, RedFace, false);
        const SolidArcCommandOutcome Red = AssignSolidArcMatcap(Parts, "RedPlastic");
        ClearSolidArcSelection(Parts);
        const SolidArcMatcapLibrary Library = AssembleSolidArcMatcaps(Parts.Appearance);
        auto LayerCounts = [&](const SolidArcScene& Scene)
        {
            std::map<std::string, int> Counts;
            for (const uint32_t Word : ComposeSolidArcMatcapLayers(Scene, Parts.Appearance, Library)) Counts[Library.Names[Word >> SolidArcMatcapLayerShift]] += 1;
            return Counts;
        };
        std::map<std::string, int> Counts = LayerCounts(PartsScene);
        const int CylinderFaces = Parts.Summary.Bodies[1].Faces, CubeFaces = Parts.Summary.Bodies[2].Faces;
        Tally(BodiesGate, 0.0, Steel.AcceptedCondition && Brass.AcceptedCondition && Red.AcceptedCondition && Parts.Revision == Revision && Counts["BrushedSteel"] == 5
                               && Counts["RedPlastic"] == 1 && Counts["Brass"] == CylinderFaces && Counts[SolidArcStudioMatcap] == CubeFaces && Parts.Appearance.BodyMatcaps.size() == 2
                               && SolidArcFaceMatcap(Parts.Appearance, BlockKey, RedFace) == "RedPlastic" && SolidArcBodyMatcap(Parts.Appearance, CubeKey) == SolidArcStudioMatcap,
              Steel.Label + " · " + Brass.Label + " · " + Red.Label + ": 5 steel + 1 red + " + std::to_string(CylinderFaces) + " brass + " + std::to_string(CubeFaces)
              + " Studio faces; revision unchanged");

        // ④ Q Boolean: Block1 − Cylinder1. The tool is consumed (its Brass override is kept for undo / delete, so it dangles);
        //    the hole faces belong to Block1 and show its matcap.
        const SolidArcCommandOutcome Drill = CombineSolidArcBodies(Parts, BlockKey, { CylinderKey }, FeatureCombination::Difference);
        PartsScene = BuildSolidArcScene(Parts);
        Counts = LayerCounts(PartsScene);
        const double Drilled = 8.0 - Pi * 0.25;
        const double DrillError = std::max(Relative(VolumeOf(Parts, BlockKey), Drilled), Relative(Parts.Summary.Volume, Drilled + 1.0));
        Tally(BodiesGate, DrillError, Drill.AcceptedCondition && DrillError <= 1.0e-9 && Parts.Summary.Bodies.size() == 2 && !Figures(Parts, CylinderKey) && Parts.Summary.SoundCondition
                                      && Parts.Tree.Steps.back().Kind == FeatureStepKind::Combine && Parts.Tree.Steps.back().Tool == CylinderKey && Parts.Regeneration.Active == BlockKey
                                      && CountDanglingMatcaps(PartsScene, Parts.Appearance) == 1 && Counts["Brass"] == 0 && Counts["BrushedSteel"] == Parts.Summary.Bodies[0].Faces - 1,
              Drill.Label + ": Block1 = 8 − π/4 exact, 2 bodies, the hole wears Block1's steel; the tool's Brass override dangles (1)");

        // ⑤ deleting the Combine step brings Cylinder1 back with its Brass; undo re-applies the Boolean
        const std::string CombineLabel = Parts.Tree.Steps.back().Label;
        const SolidArcCommandOutcome Restored = DeleteSolidArcStep(Parts, CombineLabel);
        PartsScene = BuildSolidArcScene(Parts);
        Counts = LayerCounts(PartsScene);
        const bool Back = Restored.AcceptedCondition && Parts.Summary.Bodies.size() == 3 && Relative(VolumeOf(Parts, CylinderKey), CylinderVolume) <= 1.0e-9 && Counts["Brass"] == CylinderFaces && CountDanglingMatcaps(PartsScene, Parts.Appearance) == 0;
        UndoSolidArcCommand(Parts);
        Tally(BodiesGate, Relative(VolumeOf(Parts, BlockKey), Drilled), Back && Parts.Summary.Bodies.size() == 2 && Relative(VolumeOf(Parts, BlockKey), Drilled) <= 1.0e-9,
              "delete " + CombineLabel + " → Cylinder1 returns (π/2, Brass, 0 dangling); undo → drilled again");

        // ⑥ targeted commands while Block1 is active: fillet one vertical Block2 edge (r 0.2, closed form), then a boss on
        //    Block2 (Target). Vertical edges join two sides, so their names carry no "Cap".
        std::string CubeEdge;
        for (const RegeneratedBody& Body : Parts.Regeneration.Bodies)
            if (Body.Key == CubeKey)
                for (const auto& Entry : Body.Naming.Edges)
                    if (CubeEdge.empty() && Entry.second.find("Cap") == std::string::npos) CubeEdge = Entry.second;
        SelectSolidArcName(Parts, SolidArcSelectionKind::Edge, CubeEdge, false);
        const SolidArcCommandOutcome Rounded = BlendSolidArcSelection(Parts, BlendProfile::Fillet, 0.2);
        const double RoundedCube = 1.0 - 0.04 * (1.0 - Pi / 4.0);
        Request = {};
        Request.Kind = SolidArcConstructKind::Cylinder; Request.Position = { 6.5, 0.5, 1.0 }; Request.Radius = 0.25; Request.Height = 0.5;
        Request.Combination = FeatureCombination::Union; Request.Target = CubeKey;
        const SolidArcCommandOutcome Bossed = ConstructSolidArcPrimitive(Parts, Request);
        const double BossedCube = RoundedCube + Pi * 0.0625 * 0.5;
        const double TargetError = std::max({ Relative(VolumeOf(Parts, CubeKey), BossedCube), Relative(VolumeOf(Parts, BlockKey), Drilled) });
        const FeatureStep* Blend = nullptr;
        for (const FeatureStep& Step : Parts.Tree.Steps) if (Step.Kind == FeatureStepKind::Blend) Blend = &Step;
        Tally(BodiesGate, TargetError, Rounded.AcceptedCondition && Bossed.AcceptedCondition && TargetError <= 1.0e-9 && Blend && Blend->Target == CubeKey && Parts.Summary.SoundCondition,
              "fillet r 0.2 on " + CubeEdge + " (Target " + CubeKey + ") + Cylinder boss with Target = Block2: 1 − 0.04(1 − π/4) + π/32 exact; Block1 untouched");

        // ⑦ refusals leave tree, bodies and history untouched
        const size_t Steps = Parts.Tree.Steps.size(), Entries = Parts.UndoEntries.size();
        const double Volume = Parts.Summary.Volume;
        PartsScene = BuildSolidArcScene(Parts);
        SelectSolidArcName(Parts, SolidArcSelectionKind::Edge, PartsScene.Edges[PartsScene.Bodies.front().FirstEdge].Name, false);
        SelectSolidArcName(Parts, SolidArcSelectionKind::Edge, PartsScene.Edges[PartsScene.Bodies.back().FirstEdge].Name, true);
        const SolidArcCommandOutcome Spanning = BlendSolidArcSelection(Parts, BlendProfile::Fillet, 0.1);
        ClearSolidArcSelection(Parts);
        const SolidArcCommandOutcome Self     = CombineSolidArcBodies(Parts, BlockKey, { BlockKey }, FeatureCombination::Union);
        const SolidArcCommandOutcome Lonely   = CombineSolidArcBodies(Parts, BlockKey, {}, FeatureCombination::Union);
        const SolidArcCommandOutcome Ghost    = CombineSolidArcBodies(Parts, BlockKey, { "Extrude#99" }, FeatureCombination::Union);
        const SolidArcCommandOutcome Separate = CombineSolidArcBodies(Parts, BlockKey, { CubeKey }, FeatureCombination::Separate);
        Request = {};
        Request.Kind = SolidArcConstructKind::Sphere; Request.Radius = 0.3; Request.Combination = FeatureCombination::New;
        const SolidArcCommandOutcome Second = ConstructSolidArcPrimitive(Parts, Request);
        const bool Refused = !Spanning.AcceptedCondition && !Self.AcceptedCondition && !Lonely.AcceptedCondition && !Ghost.AcceptedCondition && !Separate.AcceptedCondition && !Second.AcceptedCondition;
        Tally(BodiesGate, 0.0, Refused && Parts.Tree.Steps.size() == Steps && Parts.UndoEntries.size() == Entries && Parts.Summary.Volume == Volume,
              "refused: blend across bodies (" + Spanning.Refusal + ") · self-combine (" + Self.Refusal + ") · no tool · unknown body (" + Ghost.Refusal + ") · Separate as Q · second New; untouched");

        // ⑧ intersection keeps only the common part: Block2 ∩ a fresh New-body block overlapping its lower half
        Request = {};
        Request.Kind = SolidArcConstructKind::Block; Request.Position = { 5.5, -0.5, -0.5 }; Request.Width = 2.0; Request.Depth = 2.0; Request.Height = 1.0;
        Request.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Parts, Request));
        const std::string SlabKey = Parts.Summary.Bodies.back().Key;
        const SolidArcCommandOutcome Common = CombineSolidArcBodies(Parts, CubeKey, { SlabKey }, FeatureCombination::Intersection);
        const double CommonVolume = 0.5 - 0.04 * (1.0 - Pi / 4.0) * 0.5;                    // Lower half of the cube with half the vertical fillet
        const double CommonError  = Relative(VolumeOf(Parts, CubeKey), CommonVolume);
        Tally(BodiesGate, CommonError, Common.AcceptedCondition && CommonError <= 1.0e-9 && Parts.Summary.Bodies.size() == 2 && Parts.Summary.SoundCondition,
              Common.Label + ": Block2 = its lower half (V = " + std::to_string(VolumeOf(Parts, CubeKey)) + "), the slab consumed");
        UndoSolidArcCommand(Parts);
        UndoSolidArcCommand(Parts);

        // ⑨ save → load → save byte-equal (Target / Tool / Combine / [[Appearance.Bodies]]); same body volumes after reload
        const std::string Written = EncodeSolidArcDocument(Parts);
        const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
        SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
        Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
        RegenerateSolidArcDocument(Copy, true);
        bool SameBodies = Copy.Summary.Bodies.size() == Parts.Summary.Bodies.size();
        for (size_t Index = 0; SameBodies && Index < Copy.Summary.Bodies.size(); ++Index)
            SameBodies = Copy.Summary.Bodies[Index].Key == Parts.Summary.Bodies[Index].Key && Copy.Summary.Bodies[Index].Volume == Parts.Summary.Bodies[Index].Volume;
        Tally(BodiesGate, 0.0, Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && SameBodies && Copy.Appearance.BodyMatcaps == Parts.Appearance.BodyMatcaps
                               && Written.find("Tool = '") != std::string::npos && Written.find("Target = '") != std::string::npos && Written.find("[[Appearance.Bodies]]") != std::string::npos,
              "save → load → save byte-equal with Combine Tool / Target and [[Appearance.Bodies]]; " + std::to_string(Copy.Summary.Bodies.size()) + " bodies, volumes bit-equal");
        std::string Future = Written;
        const size_t KindAt = Future.find("Kind = 10");
        if (KindAt != std::string::npos) Future.replace(KindAt, 9, "Kind = 25");
        const SolidArcDocumentFile Newer = DecodeSolidArcDocument(Future);
        Tally(BodiesGate, 0.0, KindAt != std::string::npos && !Newer.Refusal.empty(), "a future step kind (25, beyond Evolution 16) is refused: " + Newer.Refusal);
    }

    // ---- W17 trim / split: Plasticity C (command:cut) and X on picked bodies.
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Figures  = [](const SolidArcDocument& Target, const std::string& Key) -> const SolidArcBodyFigures*
        {
            for (const SolidArcBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return &Body;
            return nullptr;
        };
        auto VolumeOf = [&](const SolidArcDocument& Target, const std::string& Key) { const SolidArcBodyFigures* Body = Figures(Target, Key); return Body ? Body->Volume : -1.0; };
        auto Draw = [](SolidArcDocument& Target, SolidArcDrawingKind Kind, std::vector<Vector2> Points, bool CentreCondition = false)
        {
            SolidArcDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points); Drawing.CentreCondition = CentreCondition;
            const SolidArcCommandOutcome Outcome = DrawSolidArcSketch(Target, Drawing);
            return Outcome.AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };

        // ① Block1 4 × 2 × 1 (Brass) cut by a drawn line x = 1 drawn upward: the inside piece (left, x < 1) is 2, the rest 6
        SolidArcDocument Part = CreateSolidArcDocument("Cut");
        SolidArcConstructRequest Request;
        Request.Kind = SolidArcConstructKind::Block; Request.Width = 4.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Part, Request));
        const std::string BlockKey = Part.Regeneration.Active;
        SelectSolidArcName(Part, SolidArcSelectionKind::Body, BlockKey, false);
        Expect(AssignSolidArcMatcap(Part, "Brass"));
        const std::string LineSketch = Draw(Part, SolidArcDrawingKind::Line, { { 1.0, -1.0 }, { 1.0, 3.0 } });
        SolidArcCutRequest Cut;
        Cut.Target = BlockKey; Cut.Cutter = LineSketch;
        const SolidArcCommandOutcome Split = CutSolidArcBody(Part, Cut);
        const std::string PieceKey = Split.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        const double SplitError = std::max(Relative(VolumeOf(Part, PieceKey), 2.0), Relative(VolumeOf(Part, BlockKey), 6.0));
        Tally(CutGate, SplitError, Split.AcceptedCondition && SplitError <= 1.0e-9 && Part.Summary.Bodies.size() == 2 && Part.Summary.SoundCondition && Part.Regeneration.Active == PieceKey
                                   && SolidArcBodyCaption(Part, PieceKey) == "Cut1" && SolidArcBodyMatcap(Part.Appearance, BlockKey) == "Brass" && SolidArcBodyMatcap(Part.Appearance, PieceKey) == SolidArcStudioMatcap,
              Split.Label + ": " + PieceKey + " (Cut1) = 2, Block1 = 6 exact, both sound; Block1 keeps its key and Brass, the piece wears Studio");

        // ② X on the picked piece removes it; undo brings it back, redo removes it again
        const SolidArcCommandOutcome Removed = RemoveSolidArcBodies(Part, { PieceKey });
        const bool Gone = Removed.AcceptedCondition && Part.Summary.Bodies.size() == 1 && !Figures(Part, PieceKey) && Relative(VolumeOf(Part, BlockKey), 6.0) <= 1.0e-9
                       && Part.Tree.Steps.back().Kind == FeatureStepKind::Remove && Part.Regeneration.Active == BlockKey;
        UndoSolidArcCommand(Part);
        const bool Undone = Part.Summary.Bodies.size() == 2 && Relative(VolumeOf(Part, PieceKey), 2.0) <= 1.0e-9;
        RedoSolidArcCommand(Part);
        Tally(CutGate, 0.0, Gone && Undone && Part.Summary.Bodies.size() == 1, Removed.Label + ": 1 body left (6), Block1 active; undo → 2 bodies, redo → 1");

        // ③ a drawn circle r ½ at (2, 1) cuts a cylinder out of what is left: π/4 inside, 6 − π/4 outside
        const std::string CircleSketch = Draw(Part, SolidArcDrawingKind::Circle, { { 2.0, 1.0 }, { 2.5, 1.0 } }, true);
        Cut = {};
        Cut.Cutter = CircleSketch;                                                           // Target empty → the active body
        const SolidArcCommandOutcome Core = CutSolidArcBody(Part, Cut);
        const std::string CoreKey = Core.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        const double CoreError = std::max(Relative(VolumeOf(Part, CoreKey), Pi / 4.0), Relative(VolumeOf(Part, BlockKey), 6.0 - Pi / 4.0));
        Tally(CutGate, CoreError, Core.AcceptedCondition && CoreError <= 1.0e-9 && Part.Summary.Bodies.size() == 2 && Part.Summary.SoundCondition && Figures(Part, BlockKey) && Figures(Part, BlockKey)->Genus == 1,
              Core.Label + ": core π/4 and the holed block 6 − π/4 exact (genus 1), cut from the active body");

        // ④ vehicle flow: 3 parabolic sections lofted into a sheet, a wavy character line seen from the Top view (−z)
        //    splits it into two sound sheets, X removes one (the BMW workflow: panel → C → X)
        SolidArcDocument Bonnet = CreateSolidArcDocument("Bonnet");
        VehicleCurveFile Curves;
        for (int Section = 0; Section < 3; ++Section)
        {
            VehicleCurve Curve;
            Curve.Name = "Section" + std::to_string(Section + 1);
            for (int Index = 0; Index <= 10; ++Index)
            {
                const double X = -1.0 + 0.2 * Index, Y = 0.5 * Section;
                Curve.Points.push_back({ X, Y, 0.25 * (1.0 - X * X) + 0.08 * Y * Y });
            }
            Curves.Curves.push_back(Curve);
        }
        VehicleCurve Character;
        Character.Name = "CharacterLine";
        for (int Index = 0; Index <= 12; ++Index)
        {
            const double X = -1.6 + 3.2 * Index / 12.0;
            Character.Points.push_back({ X, 0.5 + 0.18 * std::sin(2.2 * X / 1.6), 1.0 });
        }
        Curves.Curves.push_back(Character);
        Expect(ImportSolidArcCurves(Bonnet, Curves));
        std::vector<std::string> Labels;
        for (const FeatureStep& Step : Bonnet.Tree.Steps) if (Step.Kind == FeatureStepKind::SpaceCurve) Labels.push_back(Step.Label);
        const bool Imported = Labels.size() == 4;
        if (Imported) Expect(LoftSolidArcCurves(Bonnet, { Labels[0], Labels[1], Labels[2] }));
        const std::string PanelKey = Bonnet.Regeneration.Active;
        Cut = {};
        Cut.Target = PanelKey; Cut.Cutter = Imported ? Labels[3] : std::string{}; Cut.Direction = { 0.0, 0.0, -1.0 };
        const SolidArcCommandOutcome Trim = CutSolidArcBody(Bonnet, Cut);
        const std::string TrimKey = Trim.AcceptedCondition ? Bonnet.Tree.Steps.back().Label : std::string{};
        const SolidArcBodyFigures* Kept  = Figures(Bonnet, PanelKey);
        const SolidArcBodyFigures* Piece = Figures(Bonnet, TrimKey);
        const bool Sheets = Trim.AcceptedCondition && Kept && Piece && Kept->SheetCondition && Piece->SheetCondition && Kept->SoundCondition && Piece->SoundCondition && Bonnet.Summary.Bodies.size() == 2;
        const SolidArcCommandOutcome Discard = RemoveSolidArcBodies(Bonnet, { TrimKey });
        Tally(CutGate, 0.0, Imported && Sheets && Discard.AcceptedCondition && Bonnet.Summary.Bodies.size() == 1 && Figures(Bonnet, PanelKey) && Figures(Bonnet, PanelKey)->SheetCondition,
              Trim.Label + " seen along −z: 2 sound sheets (" + std::to_string(Kept ? Kept->Faces : 0) + " + " + std::to_string(Piece ? Piece->Faces : 0) + " faces); " + Discard.Label + " keeps the trimmed panel");

        // ⑤ refusals leave tree, bodies and history untouched
        const size_t Steps = Part.Tree.Steps.size(), Entries = Part.UndoEntries.size();
        const double Volume = Part.Summary.Volume;
        Cut = {};
        const SolidArcCommandOutcome NoCutter = CutSolidArcBody(Part, Cut);
        const std::string Beside = Draw(Part, SolidArcDrawingKind::Line, { { 9.0, -1.0 }, { 9.0, 3.0 } });
        const size_t StepsWithBeside = Part.Tree.Steps.size(), EntriesWithBeside = Part.UndoEntries.size();
        Cut.Target = BlockKey; Cut.Cutter = Beside;
        const SolidArcCommandOutcome Misses = CutSolidArcBody(Part, Cut);
        Cut.Target = "Extrude#99";
        const SolidArcCommandOutcome Ghost = CutSolidArcBody(Part, Cut);
        Cut.Target = BlockKey; Cut.Cutter = BlockKey;                                         // An extrude step is no cutter
        const SolidArcCommandOutcome Wrong = CutSolidArcBody(Part, Cut);
        const SolidArcCommandOutcome Phantom = RemoveSolidArcBodies(Part, { "Ghost#1" });
        const bool Refused = !NoCutter.AcceptedCondition && !Misses.AcceptedCondition && !Ghost.AcceptedCondition && !Wrong.AcceptedCondition && !Phantom.AcceptedCondition;
        Tally(CutGate, 0.0, Refused && Part.Tree.Steps.size() == StepsWithBeside && Part.UndoEntries.size() == EntriesWithBeside && Part.Summary.Volume == Volume && StepsWithBeside == Steps + 1 && EntriesWithBeside == Entries + 1,
              "refused: no cutter (" + NoCutter.Refusal + ") · misses (" + Misses.Refusal + ") · unknown body (" + Ghost.Refusal + ") · extrude as cutter · Remove unknown (" + Phantom.Refusal + "); untouched");
        Expect(DeleteSolidArcStep(Part, Beside));

        // ⑥ save → load → save byte-equal with Cut (Profile) / Remove and a curve Cut (Sections + plane); volumes bit-equal
        bool RoundTrips = true;
        std::string Notes;
        for (SolidArcDocument* Source : { &Part, &Bonnet })
        {
            const std::string Written = EncodeSolidArcDocument(*Source);
            const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            bool Same = Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Summary.Bodies.size() == Source->Summary.Bodies.size();
            for (size_t Index = 0; Same && Index < Copy.Summary.Bodies.size(); ++Index)
                Same = Copy.Summary.Bodies[Index].Key == Source->Summary.Bodies[Index].Key && Copy.Summary.Bodies[Index].Volume == Source->Summary.Bodies[Index].Volume;
            RoundTrips = RoundTrips && Same && Written.find("Kind = 11") != std::string::npos && Written.find("Kind = 12") != std::string::npos;
            Notes += (Notes.empty() ? "" : " · ") + Source->Title + " " + std::to_string(Copy.Summary.Bodies.size()) + " bodies";
        }
        Tally(CutGate, 0.0, RoundTrips, "save → load → save byte-equal with Cut (11) and Remove (12): " + Notes + ", volumes bit-equal");
    }

    // ---- W18: curve trim (Plasticity T) on sketches
    {
        auto Format = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.12g", Value); return std::string(Text); };
        auto Draw = [](SolidArcDocument& Target, SolidArcDrawingKind Kind, std::vector<Vector2> Points, bool ClosedCondition = false, bool CentreCondition = false)
        {
            SolidArcDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points); Drawing.ClosedCondition = ClosedCondition; Drawing.CentreCondition = CentreCondition;
            const SolidArcCommandOutcome Outcome = DrawSolidArcSketch(Target, Drawing);
            return Outcome.AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto CurvesOf = [](const SolidArcDocument& Target, const std::string& Label)
        {
            const auto Solved = Target.Regeneration.Sketches.find(Label);
            return Solved != Target.Regeneration.Sketches.end() ? Solved->second.Curves.size() : size_t(0);
        };
        auto Meet = [](Vector2 First, Vector2 FirstEnd, Vector2 Second, Vector2 SecondEnd)
        {
            const Vector2 Along = FirstEnd - First, Across = SecondEnd - Second;
            const double  Share = Cross(Second - First, Across) / Cross(Along, Across);
            return First + Along * Share;
        };

        // ① pentagram A → C → E → B → D → A as one closed Line chain: the crossing chain cannot be extruded
        const std::vector<Vector2> Tips = { { 1.0, 0.0 }, { 1.6, 1.9 }, { 0.0, 0.7 }, { 2.0, 0.7 }, { 0.4, 1.9 } };
        SolidArcDocument Star = CreateSolidArcDocument("Star");
        const std::string StarSketch = Draw(Star, SolidArcDrawingKind::Line, Tips, true);
        const SolidArcCommandOutcome Early = ExtrudeSolidArcSketch(Star, StarSketch, 1.0, FeatureCombination::New);
        std::vector<Vector2> Outline = Tips;                                                 // Independent star: 5 tips + 5 inner crossings
        for (int Chord = 0; Chord < 5; ++Chord)
        {
            const Vector2 From = Tips[Chord], To = Tips[(Chord + 1) % 5];
            const Vector2 Next = Tips[(Chord + 2) % 5], NextEnd = Tips[(Chord + 3) % 5];
            Outline.push_back(Meet(From, To, Next, NextEnd));
        }
        Vector2 Middle = {};
        for (const Vector2& Tip : Tips) Middle = Middle + Tip * 0.2;
        std::sort(Outline.begin(), Outline.end(), [&](Vector2 Left, Vector2 Right) { return std::atan2(Left.Y - Middle.Y, Left.X - Middle.X) < std::atan2(Right.Y - Middle.Y, Right.X - Middle.X); });
        double StarArea = 0.0;
        for (size_t Index = 0; Index < Outline.size(); ++Index) StarArea += 0.5 * Cross(Outline[Index], Outline[(Index + 1) % Outline.size()]);

        // ② preview of the first trim: removed ends on the two crossings of chord A → C, the document untouched
        const std::string BeforePreview = EncodeSolidArcDocument(Star);
        const SketchTrimOutcome Preview = PreviewSolidArcTrim(Star, { StarSketch, 0, (Tips[0] + Tips[1]) * 0.5 });
        Vector2 FirstCrossing  = Meet(Tips[0], Tips[1], Tips[3], Tips[4]);                   // A → C meets B → D and E → B,
        Vector2 SecondCrossing = Meet(Tips[0], Tips[1], Tips[2], Tips[3]);                   // ordered along A → C
        if (Dot(FirstCrossing - Tips[0], Tips[1] - Tips[0]) > Dot(SecondCrossing - Tips[0], Tips[1] - Tips[0])) std::swap(FirstCrossing, SecondCrossing);
        const double  PreviewError   = Preview.Removed.size() >= 2 ? std::max(Length(Preview.Removed.front() - FirstCrossing), Length(Preview.Removed.back() - SecondCrossing)) : 1.0;
        Tally(TrimGate, PreviewError, Preview.Refusal.empty() && PreviewError <= 1.0e-12 && EncodeSolidArcDocument(Star) == BeforePreview && Preview.Pieces == 2,
              "preview T on Line1 of the pentagram: removed piece between its two crossings (off " + Format(PreviewError) + "), 2 pieces left; document unchanged");

        // ③ T on the five inner pieces: the star outline (10 lines) is one loop of the shoelace area; extrude 1 is exact
        bool Trimmed = true;
        for (int Chord = 0; Chord < 5; ++Chord)
            Trimmed = Trimmed && TrimSolidArcSketch(Star, { StarSketch, Chord, (Tips[Chord] + Tips[(Chord + 1) % 5]) * 0.5 }).AcceptedCondition;
        const auto   Solved      = Star.Regeneration.Sketches.find(StarSketch);
        const SketchProfile Loop = Solved != Star.Regeneration.Sketches.end() ? ExtractSketchProfile(Solved->second) : SketchProfile{};
        const double AreaError   = std::abs(Loop.Area - StarArea);
        const SolidArcCommandOutcome Raised = ExtrudeSolidArcSketch(Star, StarSketch, 1.0, FeatureCombination::New);
        const double VolumeError = std::abs(Star.Summary.Volume - StarArea) / StarArea;
        Tally(TrimGate, std::max(AreaError, VolumeError), !Early.AcceptedCondition && Trimmed && CurvesOf(Star, StarSketch) == 10 && Loop.Refusal.empty() && Loop.Loops.size() == 1
              && AreaError <= 1.0e-12 && Raised.AcceptedCondition && VolumeError <= 1.0e-9 && Star.Summary.SoundCondition && Star.UndoEntries.size() == 7,
              "pentagram: extrude refused first (" + Early.Refusal.substr(0, 60) + "…); 5 × T → 10 lines, area " + Format(Loop.Area) + " vs shoelace " + Format(StarArea) + ", extrude 1 → V " + Format(Star.Summary.Volume) + ", sound");

        // ④ a trim that would open the extruded star detaches (W46): the extrude keeps the star (hidden), the opened
        //    chain is a new sketch of 9 lines; undo returns the file byte-equal
        const std::string BeforeBreak = EncodeSolidArcDocument(Star);
        const size_t      Entries     = Star.UndoEntries.size();
        const double      StarVolume  = Star.Summary.Volume;
        const SolidArcCommandOutcome Break = TrimSolidArcSketch(Star, { StarSketch, 0, Tips[0] + (Tips[1] - Tips[0]) * 0.1 });
        const std::string Opened = Star.Tree.Steps.back().Label;
        const bool Detached = Break.AcceptedCondition && SolidArcCurveHiddenCondition(Star, StarSketch) && Opened != StarSketch && CurvesOf(Star, Opened) == 9
                           && Star.Summary.Volume == StarVolume && Star.UndoEntries.size() == Entries + 1;
        UndoSolidArcCommand(Star);
        Tally(TrimGate, 0.0, Detached && EncodeSolidArcDocument(Star) == BeforeBreak && Star.UndoEntries.size() == Entries,
              "T on a star tip after the extrude: detached (" + Break.Label + "), the extrude keeps the star, " + Opened + " holds 9 lines; undo byte-equal");

        // ⑤ cross-sketch: circle r 0.3 at (2, 0.5) over the right side of a 2 × 1 rectangle (two sketches)
        SolidArcDocument Plan = CreateSolidArcDocument("Plan");
        const std::string Box    = Draw(Plan, SolidArcDrawingKind::Rectangle, { { 0.0, 0.0 }, { 2.0, 1.0 } });
        const std::string Circle = Draw(Plan, SolidArcDrawingKind::Circle, { { 2.0, 0.5 }, { 2.3, 0.5 } }, false, true);
        const SolidArcCommandOutcome Outer = TrimSolidArcSketch(Plan, { Circle, 0, { 2.3, 0.5 } });
        const auto   Arc      = Plan.Regeneration.Sketches.find(Circle);
        const bool   ArcCondition = Arc != Plan.Regeneration.Sketches.end() && Arc->second.Curves.size() == 1 && Arc->second.Curves[0].Kind == SketchCurveKind::Arc;
        double ArcError = 1.0;
        if (ArcCondition)
        {
            const SketchModel& Model = Arc->second;
            ArcError = std::max(Length(Model.Points[Model.Curves[0].Start].Position - Vector2{ 2.0, 0.8 }), Length(Model.Points[Model.Curves[0].End].Position - Vector2{ 2.0, 0.2 }));
        }
        UndoSolidArcCommand(Plan);
        const bool Restored = Plan.Regeneration.Sketches.at(Circle).Curves[0].Kind == SketchCurveKind::Circle;
        RedoSolidArcCommand(Plan);
        const SolidArcCommandOutcome Side = TrimSolidArcSketch(Plan, { Box, 1, { 2.0, 0.5 } });
        Tally(TrimGate, ArcError, Outer.AcceptedCondition && ArcCondition && ArcError <= 1.0e-12 && Restored && Plan.Regeneration.Sketches.at(Circle).Curves[0].Kind == SketchCurveKind::Arc
              && Side.AcceptedCondition && CurvesOf(Plan, Box) == 5,
              Outer.Label + ": the circle (another sketch than the rectangle) becomes the arc (2, 0.8) → (2, 0.2) inside it (off " + Format(ArcError) + "); undo → circle, redo → arc; " + Side.Label + " splits the right side (5 curves)");

        // ⑥ a lone unused line with nothing crossing it: T erases it and its sketch step goes; undo brings it back
        const std::string Lone  = Draw(Plan, SolidArcDrawingKind::Line, { { 5.0, 5.0 }, { 6.0, 5.0 } });
        const size_t      Steps = Plan.Tree.Steps.size();
        const SolidArcCommandOutcome Erase = TrimSolidArcSketch(Plan, { Lone, 0, { 5.5, 5.0 } });
        const bool Gone = Erase.AcceptedCondition && Plan.Tree.Steps.size() == Steps - 1 && !Plan.Captions.contains(Lone);
        UndoSolidArcCommand(Plan);
        const bool Back = Plan.Tree.Steps.size() == Steps && CurvesOf(Plan, Lone) == 1;
        Tally(TrimGate, 0.0, Gone && Back, Erase.Label + ": nothing crosses it → the curve and its emptied sketch step go; undo restores");

        // ⑦ save → load → save byte-equal with the trimmed sketches (new points, PointOnLine pins = constraint kind 14)
        bool RoundTrips = true;
        for (SolidArcDocument* Source : { &Star, &Plan })
        {
            const std::string Written = EncodeSolidArcDocument(*Source);
            const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            RoundTrips = RoundTrips && Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Summary.Volume == Source->Summary.Volume;
        }
        RoundTrips = RoundTrips && EncodeSolidArcDocument(Star).find("Kind = 14") != std::string::npos;
        Tally(TrimGate, 0.0, RoundTrips, "save → load → save byte-equal for the star (pins kept) and the plan; volumes bit-equal");
    }

    GateTally ConstrainGate   = { "W23", "Sketch constraints (K): a drawn rectangle goes DOF 4 → Lock 2 → D 1 → D 0, each dimension drives the extrude volume; literal and parameter-bound dimension edits regenerate downstream; undo / redo; remove frees a degree", "≤ 10⁻⁹ · exact" };
    GateTally ConflictGate    = { "W24", "Constraint refusals: redundant (named partner) and conflicting (named partners) constraints, non-dimension / non-positive edits, unknown sketch — the document, history and file stay untouched", "exact" };
    GateTally DragGate        = { "W25", "Point drag: a free corner follows the cursor and the extrude follows it; a locked point refuses; the preview is pure; save → load → save byte-equal with constraint labels", "≤ 10⁻⁹ · exact" };
    GateTally ReferenceGate   = { "W26", "Reference dimensions (Shift+D / E): a driven diagonal reads √13 then 5 without moving anything (DOF, positions, volume bit-equal); editing it or making it driving while implied is refused; E makes it driving at its reading and back; undo / redo; codec", "≤ 10⁻¹² · exact" };
    GateTally SplineGate      = { "W27", "Spline constraints: a line and a NURBS curve joined → L (G1) → C (G2, replaces the G1) → H on the far end, each solve ≤ 10⁻¹² with DOF −1 / −1 / −1; undo restores the G1; G2 on a natural cubic refused; codec", "≤ 10⁻¹² · exact" };
    GateTally LinkGate        = { "W28", "Cross-sketch links: a later sketch's line linked to an earlier plate's corner and top line (one undo entry, construction curve on linked points) follows the plate's width edit; backward links, linked drags, bad indices refused; codec; unlink frees", "≤ 10⁻¹² · exact" };
    // ---- W19 … W22: the editing keys (G / R / S, Shift+D, Ctrl+C / Ctrl+V, H family, J / O / P)
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto VolumeOf = [](const SolidArcDocument& Target, const std::string& Key)
        {
            for (const SolidArcBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return Body.Volume;
            return -1.0;
        };
        auto RoundTrip = [](const SolidArcDocument& Source)
        {
            const std::string Written = EncodeSolidArcDocument(Source);
            const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            return Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Summary.Volume == Source.Summary.Volume && Copy.Appearance.HiddenBodies == Source.Appearance.HiddenBodies;
        };
        auto Draw = [](SolidArcDocument& Target, SolidArcDrawingKind Kind, std::vector<Vector2> Points)
        {
            SolidArcDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points);
            const SolidArcCommandOutcome Outcome = DrawSolidArcSketch(Target, Drawing);
            return Outcome.AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto Hits = [](const SolidArcDocument& Target, std::array<double, 3> At)
        {
            const SolidArcScene Scene = BuildSolidArcScene(Target);
            const std::optional<SolidArcPick> Hit = PickSolidArcScene(Scene, RayBetween({ At[0], At[1], 20.0 }, At), 0.02);
            if (!Hit) return std::string{};
            const uint32_t Index = (Hit->Identity & ~IdentityEdgeBit) - 1u;
            return Scene.Bodies[(Hit->Identity & IdentityEdgeBit) ? Scene.Edges[Index].Body : Scene.Faces[Index].Body].Key;
        };

        // W19 ① G: Block1 2 × 2 × 1 moved by (10, 0, 0): volume 4, the old spot is empty and the new one picks Block1
        SolidArcDocument Part = CreateSolidArcDocument("Editing");
        SolidArcConstructRequest Request;
        Request.Kind = SolidArcConstructKind::Block; Request.Width = 2.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Part, Request));
        const std::string BlockKey = Part.Regeneration.Active;
        const std::string TopFace  = PickAndSelect(Part, RayBetween({ 1.0, 1.0, 20.0 }, { 1.0, 1.0, 0.0 }), false);
        const SolidArcCommandOutcome Moved = TransformSolidArcBodies(Part, { BlockKey }, TranslationMap({ 10.0, 0.0, 0.0 }), "Move");
        const bool Followed = Hits(Part, { 1.0, 1.0, 0.0 }).empty() && Hits(Part, { 11.0, 1.0, 0.0 }) == BlockKey;
        const bool Kept = Part.Selection.Faces.size() == 1 && Part.Selection.Faces.front() == TopFace;
        Tally(MoveGate, Relative(Part.Summary.Volume, 4.0), Moved.AcceptedCondition && Relative(Part.Summary.Volume, 4.0) <= 1.0e-9 && Part.Summary.SoundCondition && Followed && Kept
                                                            && Part.Tree.Steps.back().Kind == FeatureStepKind::Transform && SolidArcBodyCaption(Part, Part.Tree.Steps.back().Label) == "Move1",
              Moved.Label + ": V 4 kept, sound; (1, 1) now empty, (11, 1) picks Block1; the picked top face " + TopFace + " stays selected");

        // W19 ② R 90° about z through (11, 1, 0), then S ×2 about the same point: V 4 → 32; undo → 4
        const SolidArcCommandOutcome Turned = TransformSolidArcBodies(Part, { BlockKey }, RotationMap({ 11.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, Pi / 2.0), "Rotate");
        const SolidArcCommandOutcome Grown  = TransformSolidArcBodies(Part, { BlockKey }, ScalingMap({ 11.0, 1.0, 0.0 }, { 2.0, 2.0, 2.0 }), "Scale");
        const double GrownVolume = Part.Summary.Volume;
        UndoSolidArcCommand(Part);
        const double UndoneVolume = Part.Summary.Volume;
        RedoSolidArcCommand(Part);
        Tally(MoveGate, std::max(Relative(GrownVolume, 32.0), Relative(UndoneVolume, 4.0)), Turned.AcceptedCondition && Grown.AcceptedCondition && Relative(GrownVolume, 32.0) <= 1.0e-9 && Relative(UndoneVolume, 4.0) <= 1.0e-9
                                                                                         && Part.Summary.SoundCondition && Hits(Part, { 11.0, 1.0, 0.0 }) == BlockKey,
              Turned.Label + " + " + Grown.Label + ": V 32 = 8 × 4, sound; undo → 4, redo → 32");

        // W19 ③ refusals leave the document alone
        const std::string Before = EncodeSolidArcDocument(Part);
        const SolidArcCommandOutcome Empty = TransformSolidArcBodies(Part, {}, TranslationMap({ 1.0, 0.0, 0.0 }), "Move");
        const SolidArcCommandOutcome Ghost = TransformSolidArcBodies(Part, { "Ghost#9" }, TranslationMap({ 1.0, 0.0, 0.0 }), "Move");
        AffineMap Flat;
        Flat.ColumnZ = {};
        const SolidArcCommandOutcome Singular = TransformSolidArcBodies(Part, { BlockKey }, Flat, "Scale");
        Tally(MoveGate, 0.0, !Empty.AcceptedCondition && !Ghost.AcceptedCondition && !Singular.AcceptedCondition && Singular.Refusal.find("singular") != std::string::npos && EncodeSolidArcDocument(Part) == Before,
              "refused: nothing picked · " + Ghost.Refusal + " · " + Singular.Refusal + " — document unchanged");

        // W20 ① Shift+D: a copy moved by (0, 6, 0) is a new body, active and selected, captioned Block1Copy1
        const SolidArcCommandOutcome Copied = DuplicateSolidArcBodies(Part, { BlockKey }, TranslationMap({ 0.0, 6.0, 0.0 }));
        const std::string CopyKey = Copied.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        Tally(CopyGate, Relative(VolumeOf(Part, CopyKey), 32.0), Copied.AcceptedCondition && Part.Summary.Bodies.size() == 2 && Relative(VolumeOf(Part, CopyKey), 32.0) <= 1.0e-9 && Relative(VolumeOf(Part, BlockKey), 32.0) <= 1.0e-9
                                                                 && Part.Regeneration.Active == CopyKey && Part.Selection.Bodies == std::vector<std::string>{ CopyKey } && SolidArcBodyCaption(Part, CopyKey) == "Block1Copy1"
                                                                 && Hits(Part, { 11.0, 7.0, 0.0 }) == CopyKey,
              Copied.Label + ": 2 bodies of 32; " + CopyKey + " (Block1Copy1) active, selected and picked at (11, 7)");

        // W20 ② Ctrl+C on Block1 (clipboard = key + the last step), X on Block1, Ctrl+V: the pasted copy is Block1 as copied
        const std::string Clipboard = Part.Tree.Steps.back().Label;
        Expect(RemoveSolidArcBodies(Part, { BlockKey }));
        const SolidArcCommandOutcome Pasted = DuplicateSolidArcBodies(Part, { BlockKey }, TranslationMap({}), Clipboard);
        const std::string PasteKey = Pasted.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        Tally(CopyGate, Relative(VolumeOf(Part, PasteKey), 32.0), Pasted.AcceptedCondition && Part.Summary.Bodies.size() == 2 && Relative(VolumeOf(Part, PasteKey), 32.0) <= 1.0e-9 && Hits(Part, { 11.0, 1.0, 0.0 }) == PasteKey,
              Pasted.Label + ": after X on Block1 the paste brings it back as " + PasteKey + " (V 32 at the copied place)");

        // W20 ③ refusals
        const SolidArcCommandOutcome Stale = DuplicateSolidArcBodies(Part, { BlockKey }, TranslationMap({}), "Nowhere#99");
        const SolidArcCommandOutcome None  = DuplicateSolidArcBodies(Part, {}, TranslationMap({}));
        Tally(CopyGate, 0.0, !Stale.AcceptedCondition && !None.AcceptedCondition && Stale.Refusal.find("gone") != std::string::npos, "refused: " + Stale.Refusal + " · " + None.Refusal);

        // W21 H family on three bodies: Block1Copy1 (copy), the pasted body, and a new cylinder
        SolidArcConstructRequest Post;
        Post.Kind = SolidArcConstructKind::Cylinder; Post.Position = { -5.0, 0.0, 0.0 }; Post.Radius = 0.5; Post.Height = 1.0; Post.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Part, Post));
        const std::string PostKey = Part.Regeneration.Active;
        auto Shown = [](const SolidArcDocument& Target) { std::set<std::string> Keys; for (const SolidArcSceneBody& Body : BuildSolidArcScene(Target).Bodies) Keys.insert(Body.Key); return Keys; };
        ClearSolidArcSelection(Part);
        const SolidArcCommandOutcome NoPick = ChangeSolidArcVisibility(Part, SolidArcVisibilityVerb::HideSelected);
        SelectSolidArcName(Part, SolidArcSelectionKind::Body, CopyKey, false);
        const SolidArcCommandOutcome Hide = ChangeSolidArcVisibility(Part, SolidArcVisibilityVerb::HideSelected);
        const bool HideOk = Hide.AcceptedCondition && Shown(Part) == std::set<std::string>{ PasteKey, PostKey } && Hits(Part, { 11.0, 7.0, 0.0 }).empty() && Part.Selection.Bodies.empty();
        SelectSolidArcName(Part, SolidArcSelectionKind::Body, PostKey, false);
        const SolidArcCommandOutcome Isolate = ChangeSolidArcVisibility(Part, SolidArcVisibilityVerb::HideUnselected);
        const bool IsolateOk = Isolate.AcceptedCondition && Shown(Part) == std::set<std::string>{ PostKey };
        const SolidArcCommandOutcome Invert = ChangeSolidArcVisibility(Part, SolidArcVisibilityVerb::InvertHidden);
        const bool InvertOk = Invert.AcceptedCondition && Shown(Part) == std::set<std::string>{ CopyKey, PasteKey } && Part.Selection.Bodies.empty();
        const bool Saved = RoundTrip(Part);
        const SolidArcCommandOutcome Reveal = ChangeSolidArcVisibility(Part, SolidArcVisibilityVerb::UnhideAll);
        const bool RevealOk = Reveal.AcceptedCondition && Shown(Part).size() == 3;
        UndoSolidArcCommand(Part);
        const bool UndoOk = Shown(Part) == std::set<std::string>{ CopyKey, PasteKey } && Part.Summary.Bodies.size() == 3;
        RedoSolidArcCommand(Part);
        const SolidArcCommandOutcome Again = ChangeSolidArcVisibility(Part, SolidArcVisibilityVerb::UnhideAll);
        Tally(HideGate, 0.0, HideOk && IsolateOk && InvertOk && RevealOk && UndoOk && Saved && !NoPick.AcceptedCondition && !Again.AcceptedCondition,
              "H hides Block1Copy1 (scene 2, unpickable, deselected) · Shift+H on the cylinder leaves it alone · Ctrl+H swaps → 2 shown · saved with Hidden, reload equal · Alt+H shows 3; undo → 2 · refused: " + NoPick.Refusal + " · " + Again.Refusal);
        Tally(HideGate, 0.0, Part.Summary.Bodies.size() == 3 && Relative(Part.Summary.Volume, 64.0 + Pi * 0.25) <= 1.0e-9, "hiding never touches the model: 3 bodies, V 64 + π/4 throughout");
        Tally(MoveGate, 0.0, RoundTrip(Part) && EncodeSolidArcDocument(Part).find("Kind = 13") != std::string::npos && EncodeSolidArcDocument(Part).find("Kind = 14") != std::string::npos,
              "save → load → save byte-equal with Transform (13) and Duplicate (14) steps (placements and the paste source kept)");

        // W22 ① J: an L and its complement drawn as two sketches; the L alone cannot extrude, joined they make a unit square
        SolidArcDocument Plan = CreateSolidArcDocument("Curves");
        const std::string Lower = Draw(Plan, SolidArcDrawingKind::Line, { { 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, 1.0 } });
        const std::string Upper = Draw(Plan, SolidArcDrawingKind::Line, { { 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0 } });
        const SolidArcCommandOutcome Open   = ExtrudeSolidArcSketch(Plan, Lower, 1.0, FeatureCombination::Separate);
        const size_t                 Steps  = Plan.Tree.Steps.size();
        const SolidArcCommandOutcome Joined = JoinSolidArcSketches(Plan, { Lower, Upper });
        const bool Merged = Joined.AcceptedCondition && Plan.Tree.Steps.size() == Steps - 1 && !Plan.Captions.contains(Upper);
        const SolidArcCommandOutcome Square = ExtrudeSolidArcSketch(Plan, Lower, 1.0, FeatureCombination::Separate);
        const std::string SquareKey = Square.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        Tally(CurveGate, Relative(VolumeOf(Plan, SquareKey), 1.0), !Open.AcceptedCondition && Merged && Square.AcceptedCondition && Relative(VolumeOf(Plan, SquareKey), 1.0) <= 1.0e-9,
              Joined.Label + ": the open L refuses to extrude; joined (one sketch fewer) the square extrudes to V 1");

        // W22 ② O: the joined square offset by ¼ outward → 1.5 × 1.5 (mitred corners), extruded 1 → 2.25; preview is pure
        const std::string BeforePreview = EncodeSolidArcDocument(Plan);
        const SketchOffsetOutcome Preview = PreviewSolidArcOffset(Plan, Lower, 0.25);
        const bool Pure = EncodeSolidArcDocument(Plan) == BeforePreview && Preview.Refusal.empty() && Preview.Chains == 1;
        const SolidArcCommandOutcome Offset = OffsetSolidArcSketch(Plan, Lower, 0.25);
        const std::string OffsetSketch = Offset.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        const SolidArcCommandOutcome Grow = ExtrudeSolidArcSketch(Plan, OffsetSketch, 1.0, FeatureCombination::Separate);
        const std::string GrowKey = Grow.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        Tally(CurveGate, Relative(VolumeOf(Plan, GrowKey), 2.25), Pure && Offset.AcceptedCondition && Grow.AcceptedCondition && Relative(VolumeOf(Plan, GrowKey), 2.25) <= 1.0e-9 && SolidArcBodyCaption(Plan, OffsetSketch) == "Offset1",
              Offset.Label + ": new sketch Offset1, extruded V 2.25 = 1.5²; the preview left the document alone");

        // W22 ③ P: a drawn line (0, 3) → (2, 3) piped with r ¼ → new body π r² L
        const std::string Rail = Draw(Plan, SolidArcDrawingKind::Line, { { 0.0, 3.0 }, { 2.0, 3.0 } });
        const SolidArcCommandOutcome Piped = PipeSolidArcPath(Plan, Rail, 0.25);
        const std::string PipeKey = Piped.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        Tally(CurveGate, Relative(VolumeOf(Plan, PipeKey), Pi * 0.0625 * 2.0), Piped.AcceptedCondition && Relative(VolumeOf(Plan, PipeKey), Pi * 0.0625 * 2.0) <= 1.0e-9 && Plan.Summary.SoundCondition && Plan.Regeneration.Active == PipeKey,
              Piped.Label + ": tube V π r² L = π/8 exact, sound, active");

        // W22 ④ refusals and codec
        const SolidArcCommandOutcome Lone     = JoinSolidArcSketches(Plan, { Lower });
        // The pipe's rail joins too since W46: kept hidden for the pipe (volume unchanged), the join a new step; undone.
        const double                 Tube     = VolumeOf(Plan, PipeKey);
        const SolidArcCommandOutcome Consumed = JoinSolidArcSketches(Plan, { Rail, Lower });
        const bool RailKept = Consumed.AcceptedCondition && SolidArcCurveHiddenCondition(Plan, Rail) && VolumeOf(Plan, PipeKey) == Tube;
        if (Consumed.AcceptedCondition) UndoSolidArcCommand(Plan);
        const SolidArcCommandOutcome Vanish   = OffsetSolidArcSketch(Plan, Lower, -0.6);
        const SolidArcCommandOutcome Fat      = PipeSolidArcPath(Plan, Rail, 0.0);
        const SolidArcCommandOutcome NoPath   = PipeSolidArcPath(Plan, PipeKey, 0.1);
        Tally(CurveGate, 0.0, !Lone.AcceptedCondition && RailKept && !Vanish.AcceptedCondition && !Fat.AcceptedCondition && !NoPath.AcceptedCondition,
              "refused: " + Lone.Refusal + " · " + Vanish.Refusal + " · " + Fat.Refusal + " · " + NoPath.Refusal + "; the pipe's rail joins detached (kept hidden, tube unchanged), undone");
        Tally(CurveGate, 0.0, RoundTrip(Plan) && EncodeSolidArcDocument(Plan).find("Kind = 15") != std::string::npos, "save → load → save byte-equal with the joined / offset sketches and the Pipe (15) step");
    }

    // ---- W23 … W25: sketch constraints (K)
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Propose = [](const SolidArcDocument& Target, const std::string& Sketch, std::vector<SketchPick> Picks, SketchConstraintVerb Verb, double Value = std::numeric_limits<double>::quiet_NaN())
        {
            const SolidArcSketchDiagnosis Diagnosis = DiagnoseSolidArcSketch(Target, Sketch);
            SketchConstraintProposal Proposal = ProposeSketchConstraint(Diagnosis.Sketch, Picks, Verb);
            if (!std::isnan(Value)) Proposal.Constraint.Value = Value;
            return Proposal.Constraint;
        };
        auto Freedom = [](const SolidArcDocument& Target, const std::string& Sketch) { return DiagnoseSolidArcSketch(Target, Sketch).Solve.DegreesOfFreedom; };
        const SketchPick Bottom = { SketchPickKind::Curve, 0 };
        const SketchPick Side   = { SketchPickKind::Curve, 1 };
        const SketchPick Top    = { SketchPickKind::Curve, 2 };

        // W23 ① a drawn 2 × 1 rectangle extruded by 1: DOF 4; ] locks its first corner (2), D on the bottom = 3 (1), D on the side = 2 (0)
        SolidArcDocument Plate = CreateSolidArcDocument("Constraints");
        SolidArcDrawingRequest Drawing;
        Drawing.Kind = SolidArcDrawingKind::Rectangle; Drawing.Points = { { 0.0, 0.0 }, { 2.0, 1.0 } };
        const SolidArcCommandOutcome Drawn = DrawSolidArcSketch(Plate, Drawing);
        const std::string Sketch = Drawn.AcceptedCondition ? Plate.Tree.Steps.back().Label : std::string{};
        const SolidArcCommandOutcome Raised = ExtrudeSolidArcSketch(Plate, Sketch, 1.0, FeatureCombination::Separate);
        const std::string Key = Raised.AcceptedCondition ? Plate.Tree.Steps.back().Label : std::string{};
        const int Free = Freedom(Plate, Sketch);
        const SolidArcCommandOutcome Locked = ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { { SketchPickKind::Point, 0 } }, SketchConstraintVerb::Lock));
        const int AfterLock = Freedom(Plate, Sketch);
        const SolidArcCommandOutcome Wide = ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Dimension, 3.0));
        const int AfterWide = Freedom(Plate, Sketch);
        const double WideVolume = Plate.Summary.Volume;
        const SolidArcCommandOutcome Tall = ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Side }, SketchConstraintVerb::Dimension, 2.0));
        const int AfterTall = Freedom(Plate, Sketch);
        const std::vector<SketchConstraint>& Authored = Plate.Tree.Steps[0].Sketch.Constraints;
        const bool Labelled = Authored.size() == 7 && Authored[4].Label == "F1" && Authored[5].Label == "D1" && Authored[6].Label == "D2";
        Tally(ConstrainGate, Relative(Plate.Summary.Volume, 6.0) + Relative(WideVolume, 3.0),
              Free == 4 && Locked.AcceptedCondition && AfterLock == 2 && Wide.AcceptedCondition && AfterWide == 1 && Tall.AcceptedCondition && AfterTall == 0 && Labelled
                  && Relative(WideVolume, 3.0) <= 1.0e-9 && Relative(Plate.Summary.Volume, 6.0) <= 1.0e-9 && Plate.Regeneration.Active == Key,
              "DOF " + std::to_string(Free) + " → " + Locked.Label + " " + std::to_string(AfterLock) + " → " + Wide.Label + " " + std::to_string(AfterWide) + " (V 3) → " + Tall.Label + " " + std::to_string(AfterTall) + " (V 6); labels F1 D1 D2");

        // W23 ② D1 = 4 → V 8 (the solved corners are written back: the step's own points move); undo → 6, redo → 8
        const SolidArcCommandOutcome Edit = ChangeSolidArcDimension(Plate, Sketch, 5, 4.0);
        const double Edited = Plate.Summary.Volume;
        const Vector2 Corner = Plate.Tree.Steps[0].Sketch.Points[2].Position;
        const bool Undone = UndoSolidArcCommand(Plate);
        const double Restored = Plate.Summary.Volume;
        const bool Redone = RedoSolidArcCommand(Plate);
        Tally(ConstrainGate, Relative(Edited, 8.0) + Relative(Restored, 6.0) + std::abs(Corner.X - 4.0) + std::abs(Corner.Y - 2.0),
              Edit.AcceptedCondition && Relative(Edited, 8.0) <= 1.0e-9 && std::abs(Corner.X - 4.0) <= 1.0e-9 && std::abs(Corner.Y - 2.0) <= 1.0e-9 && Undone && Relative(Restored, 6.0) <= 1.0e-12 && Redone && Relative(Plate.Summary.Volume, 8.0) <= 1.0e-9,
              Edit.Label + ": V 8, far corner written back at (4, 2); undo V 6, redo V 8");

        // W23 ③ a Block tile is fully dimensioned by parameters: its Width dimension changes the parameter (and the volume)
        SolidArcDocument Tile = CreateSolidArcDocument("Tile");
        Expect(ConstructSolidArcPrimitive(Tile, SolidArcConstructRequest{}));
        const std::string TileSketch = Tile.Tree.Steps[0].Label;
        int WidthIndex = -1;
        for (size_t Index = 0; Index < Tile.Tree.Steps[0].Sketch.Constraints.size(); ++Index)
            if (Tile.Tree.Steps[0].Sketch.Constraints[Index].Parameter.ends_with("Width")) WidthIndex = static_cast<int>(Index);
        const int TileFree = Freedom(Tile, TileSketch);
        const SolidArcCommandOutcome Widen = ChangeSolidArcDimension(Tile, TileSketch, WidthIndex, 3.0);
        double WidthValue = 0.0;
        for (const FeatureParameter& Parameter : Tile.Tree.Parameters) if (Parameter.Name.ends_with("Width")) WidthValue = Parameter.Value;
        Tally(ConstrainGate, Relative(Tile.Summary.Volume, 4.5), TileFree == 0 && WidthIndex >= 0 && Widen.AcceptedCondition && WidthValue == 3.0 && Relative(Tile.Summary.Volume, 4.5) <= 1.0e-9,
              "Block tile DOF 0; " + Widen.Label + " sets its Width parameter to 3 → V 3 × 1.5 × 1 = 4.5");

        // W23 ④ removing D2 frees the height (DOF 1); the body stays (positions unchanged)
        const SolidArcCommandOutcome Freed = RemoveSolidArcConstraint(Plate, Sketch, 6);
        Tally(ConstrainGate, Relative(Plate.Summary.Volume, 8.0), Freed.AcceptedCondition && Freedom(Plate, Sketch) == 1 && Relative(Plate.Summary.Volume, 8.0) <= 1.0e-12 && Plate.Tree.Steps[0].Sketch.Constraints.size() == 6,
              Freed.Label + ": DOF 1, V 8 kept");

        // W24 refusals (with D2 back): each leaves the file and the history untouched
        Expect(ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Side }, SketchConstraintVerb::Dimension, 2.0)));
        const std::string File = EncodeSolidArcDocument(Plate);
        const size_t History = Plate.UndoEntries.size();
        const SolidArcCommandOutcome Again     = ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Horizontal));
        const SolidArcCommandOutcome Implied   = ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Top }, SketchConstraintVerb::Dimension));
        const SolidArcCommandOutcome Clash     = ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Top }, SketchConstraintVerb::Dimension, 5.0));
        const SolidArcCommandOutcome Valueless = ChangeSolidArcDimension(Plate, Sketch, 0, 1.0);
        const SolidArcCommandOutcome Negative  = ChangeSolidArcDimension(Plate, Sketch, 5, -1.0);
        const SolidArcCommandOutcome Missing   = ConstrainSolidArcSketch(Plate, Key, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Horizontal));
        const SolidArcCommandOutcome Beyond    = RemoveSolidArcConstraint(Plate, Sketch, 40);
        const bool Untouched = EncodeSolidArcDocument(Plate) == File && Plate.UndoEntries.size() == History;
        Tally(ConflictGate, 0.0, !Again.AcceptedCondition && Again.Refusal.find("already implied by H1") != std::string::npos,
              "second H on Line1 refused: " + Again.Refusal);
        Tally(ConflictGate, 0.0, !Implied.AcceptedCondition && Implied.Refusal.find("already implied by") != std::string::npos && Implied.Refusal.find("D1") != std::string::npos,
              "Length 4 on the top refused: " + Implied.Refusal);
        Tally(ConflictGate, 0.0, !Clash.AcceptedCondition && Clash.Refusal.find("conflicts with") != std::string::npos && Clash.Refusal.find("D1") != std::string::npos,
              "Length 5 on the top refused: " + Clash.Refusal);
        Tally(ConflictGate, 0.0, !Valueless.AcceptedCondition && !Negative.AcceptedCondition && !Missing.AcceptedCondition && !Beyond.AcceptedCondition && Untouched,
              "refused: " + Valueless.Refusal + " · " + Negative.Refusal + " · " + Missing.Refusal + " · " + Beyond.Refusal + "; file and history untouched");

        // W25 ① D2 removed again: the free far corner dragged to (4, 3) carries the top up → V 12; the preview is pure
        Expect(RemoveSolidArcConstraint(Plate, Sketch, 6));
        const std::string BeforePreview = EncodeSolidArcDocument(Plate);
        const SketchDragOutcome Preview = PreviewSolidArcDrag(Plate, Sketch, 2, { 4.0, 3.0 });
        const bool Pure = EncodeSolidArcDocument(Plate) == BeforePreview && Preview.Refusal.empty() && Preview.PinnedCondition;
        const SolidArcCommandOutcome Dragged = DragSolidArcSketchPoint(Plate, Sketch, 2, { 4.0, 3.0 });
        const Vector2 Far = Plate.Tree.Steps[0].Sketch.Points[2].Position;
        const Vector2 Near = Plate.Tree.Steps[0].Sketch.Points[3].Position;
        const double Miss = std::abs(Far.X - 4.0) + std::abs(Far.Y - 3.0) + std::abs(Near.X) + std::abs(Near.Y - 3.0);
        Tally(DragGate, Miss + Relative(Plate.Summary.Volume, 12.0), Pure && Dragged.AcceptedCondition && Miss <= 1.0e-9 && Relative(Plate.Summary.Volume, 12.0) <= 1.0e-9,
              Dragged.Label + ": corner at (4, 3), Line3.Start follows to (0, 3), V 12; the preview left the document alone");

        // W25 ② dragging the locked corner refuses; ③ codec byte-equal with F1 D1 and the dragged positions
        const SolidArcCommandOutcome Stuck = DragSolidArcSketchPoint(Plate, Sketch, 0, { -1.0, -1.0 });
        Tally(DragGate, 0.0, !Stuck.AcceptedCondition && Stuck.Refusal.find("fully constrained") != std::string::npos, "refused: " + Stuck.Refusal);
        const std::string Written = EncodeSolidArcDocument(Plate);
        const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
        SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
        Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
        RegenerateSolidArcDocument(Copy, true);
        Tally(DragGate, 0.0, Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Summary.Volume == Plate.Summary.Volume && Written.find("'F1'") != std::string::npos && Freedom(Copy, Sketch) == 1,
              "save → load → save byte-equal; F1 / D1 labels and DOF 1 survive the reload");
    }

    // ---- W26 … W28: reference dimensions (Shift+D / E), spline constraints (L / C / H), cross-sketch links
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Propose = [](const SolidArcDocument& Target, const std::string& Sketch, std::vector<SketchPick> Picks, SketchConstraintVerb Verb, double Value = std::numeric_limits<double>::quiet_NaN())
        {
            SketchConstraintProposal Proposal = ProposeSketchConstraint(DiagnoseSolidArcSketch(Target, Sketch).Sketch, Picks, Verb);
            if (!std::isnan(Value)) Proposal.Constraint.Value = Value;
            return Proposal.Constraint;
        };
        auto Freedom = [](const SolidArcDocument& Target, const std::string& Sketch) { return DiagnoseSolidArcSketch(Target, Sketch).Solve.DegreesOfFreedom; };
        auto RoundTrip = [](const SolidArcDocument& Source)
        {
            const std::string Written = EncodeSolidArcDocument(Source);
            const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            return Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Summary.Volume == Source.Summary.Volume;
        };
        auto DrawRectangle = [](SolidArcDocument& Target, Vector2 Low, Vector2 High)
        {
            SolidArcDrawingRequest Drawing;
            Drawing.Kind = SolidArcDrawingKind::Rectangle; Drawing.Points = { Low, High };
            return DrawSolidArcSketch(Target, Drawing).AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto Scientific = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3e", Value); return std::string(Text); };
        const SketchPick Bottom = { SketchPickKind::Curve, 0 };
        const SketchPick Side   = { SketchPickKind::Curve, 1 };

        // W26 ① a fully dimensioned 2 × 1 → 3 × 2 plate (DOF 0, V 6); Shift+D on its diagonal: D3 (reference) reads √13,
        //        DOF stays 0, the positions and the volume are bit-equal
        SolidArcDocument Plate = CreateSolidArcDocument("References");
        const std::string Sketch = DrawRectangle(Plate, { 0.0, 0.0 }, { 2.0, 1.0 });
        Expect(ExtrudeSolidArcSketch(Plate, Sketch, 1.0, FeatureCombination::Separate));
        Expect(ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { { SketchPickKind::Point, 0 } }, SketchConstraintVerb::Lock)));
        Expect(ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Dimension, 3.0)));
        Expect(ConstrainSolidArcSketch(Plate, Sketch, Propose(Plate, Sketch, { Side }, SketchConstraintVerb::Dimension, 2.0)));
        const std::vector<Vector2> Corners = [&] { std::vector<Vector2> Positions; for (const SketchPoint& Point : Plate.Tree.Steps[0].Sketch.Points) Positions.push_back(Point.Position); return Positions; }();
        const double Before = Plate.Summary.Volume;
        const SketchConstraintProposal Diagonal = ProposeSketchReference(DiagnoseSolidArcSketch(Plate, Sketch).Sketch, { { SketchPickKind::Point, 0 }, { SketchPickKind::Point, 2 } });
        const SolidArcCommandOutcome Referenced = ConstrainSolidArcSketch(Plate, Sketch, Diagonal.Constraint);
        const SketchConstraint& Reading = Plate.Tree.Steps[0].Sketch.Constraints.back();
        bool Unmoved = Plate.Summary.Volume == Before;
        for (size_t Point = 0; Point < Corners.size(); ++Point) Unmoved = Unmoved && Plate.Tree.Steps[0].Sketch.Points[Point].Position.X == Corners[Point].X && Plate.Tree.Steps[0].Sketch.Points[Point].Position.Y == Corners[Point].Y;
        const SolidArcSketchDiagnosis Diagnosed = DiagnoseSolidArcSketch(Plate, Sketch);
        const int ReadingIndex = static_cast<int>(Plate.Tree.Steps[0].Sketch.Constraints.size()) - 1;
        Tally(ReferenceGate, std::abs(Reading.Value - std::sqrt(13.0)),
              Diagonal.Refusal.empty() && Referenced.AcceptedCondition && Reading.DrivenCondition && Reading.Label == "D3" && std::abs(Reading.Value - std::sqrt(13.0)) <= 1.0e-12 && Diagnosed.Solve.DegreesOfFreedom == 0
                  && Diagnosed.Solve.Constraints[ReadingIndex].Status == SketchConstraintStatus::Driven && Unmoved,
              Referenced.Label + ": " + DescribeSketchConstraint(Plate.Tree.Steps[0].Sketch, Reading) + " · DOF 0 · positions and V bit-equal");

        // W26 ② D2 = 4 → the reading follows to 5 (written back); editing the reading and making it driving are refused
        const SolidArcCommandOutcome Taller = ChangeSolidArcDimension(Plate, Sketch, 6, 4.0);
        const double Followed = Plate.Tree.Steps[0].Sketch.Constraints[ReadingIndex].Value;
        const std::string File = EncodeSolidArcDocument(Plate);
        const size_t History = Plate.UndoEntries.size();
        const SolidArcCommandOutcome Typed   = ChangeSolidArcDimension(Plate, Sketch, ReadingIndex, 6.0);
        const SolidArcCommandOutcome Driving = ToggleSolidArcReference(Plate, Sketch, ReadingIndex);
        const SolidArcCommandOutcome Plain   = ToggleSolidArcReference(Plate, Sketch, 0);
        const bool Untouched = EncodeSolidArcDocument(Plate) == File && Plate.UndoEntries.size() == History;
        Tally(ReferenceGate, std::abs(Followed - 5.0) + Relative(Plate.Summary.Volume, 12.0),
              Taller.AcceptedCondition && std::abs(Followed - 5.0) <= 1.0e-12 && Relative(Plate.Summary.Volume, 12.0) <= 1.0e-9 && !Typed.AcceptedCondition && Typed.Refusal.find("reference dimension") != std::string::npos
                  && !Driving.AcceptedCondition && Driving.Refusal.find("stays a reference") != std::string::npos && !Plain.AcceptedCondition && Untouched,
              Taller.Label + " → D3 reads 5 · refused: " + Typed.Refusal + " · " + Driving.Refusal + " · " + Plain.Refusal);

        // W26 ③ D2 removed (DOF 1): E makes D3 driving at its reading √(9 + 16) = 5 (DOF 0, nothing moves), E again frees it;
        //        undo / redo; save → load → save byte-equal with Reference = true
        Expect(RemoveSolidArcConstraint(Plate, Sketch, 6));
        const int Loose = Freedom(Plate, Sketch);
        const double LooseVolume = Plate.Summary.Volume;
        const int Index = static_cast<int>(Plate.Tree.Steps[0].Sketch.Constraints.size()) - 1;
        const SolidArcCommandOutcome Drive = ToggleSolidArcReference(Plate, Sketch, Index);
        const int Driven = Freedom(Plate, Sketch);
        const double DrivenValue = Plate.Tree.Steps[0].Sketch.Constraints[Index].Value;
        const bool DrivenStill = Plate.Summary.Volume == LooseVolume;
        const SolidArcCommandOutcome Release = ToggleSolidArcReference(Plate, Sketch, Index);
        const int Released = Freedom(Plate, Sketch);
        const bool Undone = UndoSolidArcCommand(Plate) && Freedom(Plate, Sketch) == 0 && !Plate.Tree.Steps[0].Sketch.Constraints[Index].DrivenCondition;
        const bool Redone = RedoSolidArcCommand(Plate) && Freedom(Plate, Sketch) == 1 && Plate.Tree.Steps[0].Sketch.Constraints[Index].DrivenCondition;
        Tally(ReferenceGate, std::abs(DrivenValue - 5.0),
              Loose == 1 && Drive.AcceptedCondition && Driven == 0 && std::abs(DrivenValue - 5.0) <= 1.0e-12 && DrivenStill && Release.AcceptedCondition && Released == 1 && Undone && Redone,
              "DOF " + std::to_string(Loose) + " → " + Drive.Label + " " + std::to_string(Driven) + " (V bit-equal) → " + Release.Label + " " + std::to_string(Released) + "; undo / redo");
        const std::string Written = EncodeSolidArcDocument(Plate);
        Tally(ReferenceGate, 0.0, RoundTrip(Plate) && Written.find("Reference = true") != std::string::npos, "save → load → save byte-equal; Reference = true written for D3 only");

        // W27 ① a NURBS curve drawn from (0, 0) and a line (−2, 0.5) → (0, 0), joined (ends welded): L gives T1 (G1), C gives
        //        K1 that replaces it (G2), H on the far end; each solve converges ≤ 10⁻¹², undo brings T1 back
        SolidArcDocument Joint = CreateSolidArcDocument("Spline joint");
        SolidArcDrawingRequest Lead;
        Lead.Kind = SolidArcDrawingKind::Line; Lead.Points = { { -2.0, 0.5 }, { 0.0, 0.0 } };
        Expect(DrawSolidArcSketch(Joint, Lead));
        const std::string LineSketch = Joint.Tree.Steps.back().Label;
        SolidArcDrawingRequest Sweep;
        Sweep.Kind = SolidArcDrawingKind::Curve; Sweep.Form = SketchSplineForm::Nurbs; Sweep.Points = { { 0.0, 0.0 }, { 1.0, 1.0 }, { 2.0, 1.2 }, { 3.0, 0.4 } };
        Expect(DrawSolidArcSketch(Joint, Sweep));
        const std::string CurveSketch = Joint.Tree.Steps.back().Label;
        const SolidArcCommandOutcome Joined = JoinSolidArcSketches(Joint, { LineSketch, CurveSketch });
        int LineCurve = -1, SplineCurve = -1;
        const SketchModel& Merged = Joint.Tree.Steps[0].Sketch;
        for (size_t Curve = 0; Curve < Merged.Curves.size(); ++Curve)
            (Merged.Curves[Curve].Kind == SketchCurveKind::Spline ? SplineCurve : LineCurve) = static_cast<int>(Curve);
        const bool Welded = SplineCurve >= 0 && LineCurve >= 0 && (Merged.Curves[SplineCurve].Points.front() == Merged.Curves[LineCurve].End || Merged.Curves[SplineCurve].Points.front() == Merged.Curves[LineCurve].Start);
        const int FarEnd = SplineCurve >= 0 ? Merged.Curves[SplineCurve].Points.back() : -1;
        const int Open = Freedom(Joint, LineSketch);
        const SolidArcCommandOutcome Tangent = ConstrainSolidArcSketch(Joint, LineSketch, Propose(Joint, LineSketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Curve, LineCurve } }, SketchConstraintVerb::Parallel));
        const SolidArcSketchDiagnosis AfterTangent = DiagnoseSolidArcSketch(Joint, LineSketch);
        const SolidArcCommandOutcome Curvature = ConstrainSolidArcSketch(Joint, LineSketch, Propose(Joint, LineSketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Curve, LineCurve } }, SketchConstraintVerb::Curvature));
        const SolidArcSketchDiagnosis AfterCurvature = DiagnoseSolidArcSketch(Joint, LineSketch);
        const std::vector<SketchConstraint>& Joints = Joint.Tree.Steps[0].Sketch.Constraints;
        const bool Replaced = !Joints.empty() && Joints.back().Kind == SketchConstraintKind::SplineCurvature && Joints.back().Label == "K1"
                           && std::none_of(Joints.begin(), Joints.end(), [](const SketchConstraint& Constraint) { return Constraint.Kind == SketchConstraintKind::SplineTangent; });
        Tally(SplineGate, std::max(AfterTangent.Solve.MaxResidual, AfterCurvature.Solve.MaxResidual),
              Joined.AcceptedCondition && Welded && Tangent.AcceptedCondition && AfterTangent.Solve.Converged && AfterTangent.Solve.MaxResidual <= 1.0e-12 && AfterTangent.Solve.DegreesOfFreedom == Open - 1
                  && Curvature.AcceptedCondition && Curvature.Label.find("replaces T1") != std::string::npos && Replaced && AfterCurvature.Solve.Converged && AfterCurvature.Solve.MaxResidual <= 1.0e-12
                  && AfterCurvature.Solve.DegreesOfFreedom == Open - 2,
              "DOF " + std::to_string(Open) + " → " + Tangent.Label + " " + std::to_string(AfterTangent.Solve.DegreesOfFreedom) + " → " + Curvature.Label + " " + std::to_string(AfterCurvature.Solve.DegreesOfFreedom));
        const SolidArcCommandOutcome Level = ConstrainSolidArcSketch(Joint, LineSketch, Propose(Joint, LineSketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Point, FarEnd } }, SketchConstraintVerb::Horizontal));
        const SolidArcSketchDiagnosis AfterLevel = DiagnoseSolidArcSketch(Joint, LineSketch);
        const std::string LevelLabel = Joint.Tree.Steps[0].Sketch.Constraints.back().Label;
        const bool Back = UndoSolidArcCommand(Joint) && UndoSolidArcCommand(Joint)
                       && std::any_of(Joint.Tree.Steps[0].Sketch.Constraints.begin(), Joint.Tree.Steps[0].Sketch.Constraints.end(), [](const SketchConstraint& Constraint) { return Constraint.Kind == SketchConstraintKind::SplineTangent && Constraint.Label == "T1"; });
        const bool Forward = RedoSolidArcCommand(Joint) && RedoSolidArcCommand(Joint) && Joint.Tree.Steps[0].Sketch.Constraints.back().Label == LevelLabel;
        Tally(SplineGate, AfterLevel.Solve.MaxResidual,
              Level.AcceptedCondition && LevelLabel == "H1" && AfterLevel.Solve.Converged && AfterLevel.Solve.MaxResidual <= 1.0e-12 && AfterLevel.Solve.DegreesOfFreedom == Open - 3 && Back && Forward,
              Level.Label + " · DOF " + std::to_string(AfterLevel.Solve.DegreesOfFreedom) + "; undo × 2 brings T1 back, redo × 2 restores K1 + H1");

        // W27 ② refusals: G2 on a natural cubic spline, L on a spline and a curve that do not meet; save → load → save byte-equal
        SolidArcDocument Natural = CreateSolidArcDocument("Natural");
        Expect(DrawSolidArcSketch(Natural, Lead));
        const std::string NaturalSketch = Natural.Tree.Steps.back().Label;
        SolidArcDrawingRequest Cubic = Sweep;
        Cubic.Form = SketchSplineForm::CubicSpline;
        Expect(DrawSolidArcSketch(Natural, Cubic));
        Expect(JoinSolidArcSketches(Natural, { NaturalSketch, Natural.Tree.Steps.back().Label }));
        const SketchConstraintProposal NaturalProposal = ProposeSketchConstraint(DiagnoseSolidArcSketch(Natural, NaturalSketch).Sketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Curve, LineCurve } }, SketchConstraintVerb::Curvature);
        Tally(SplineGate, 0.0, !NaturalProposal.Refusal.empty() && NaturalProposal.Refusal.find("zero curvature") != std::string::npos, "refused: " + NaturalProposal.Refusal);
        Tally(SplineGate, 0.0, RoundTrip(Joint) && EncodeSolidArcDocument(Joint).find("Kind = 22") != std::string::npos && EncodeSolidArcDocument(Joint).find("Kind = 24") != std::string::npos,
              "save → load → save byte-equal with SplineCurvature (22) and SplineDirection (24)");

        // W28 ① Sketch A: a 3 × 2 plate (DOF 0, V 6); Sketch B (later, same plane): a free line (5, 3) → (6, 4). O on B's start
        //        and A's far corner links the corner into B (one undo entry); O on B's end and A's top line links the line
        //        (construction, on linked points); B's start sits on (3, 2), its end on y = 2
        SolidArcDocument Linked = CreateSolidArcDocument("Links");
        const std::string Source = DrawRectangle(Linked, { 0.0, 0.0 }, { 2.0, 1.0 });
        Expect(ExtrudeSolidArcSketch(Linked, Source, 1.0, FeatureCombination::Separate));
        Expect(ConstrainSolidArcSketch(Linked, Source, Propose(Linked, Source, { { SketchPickKind::Point, 0 } }, SketchConstraintVerb::Lock)));
        Expect(ConstrainSolidArcSketch(Linked, Source, Propose(Linked, Source, { Bottom }, SketchConstraintVerb::Dimension, 3.0)));
        Expect(ConstrainSolidArcSketch(Linked, Source, Propose(Linked, Source, { Side }, SketchConstraintVerb::Dimension, 2.0)));
        SolidArcDrawingRequest Spoke;
        Spoke.Kind = SolidArcDrawingKind::Line; Spoke.Points = { { 5.0, 3.0 }, { 6.0, 4.0 } };
        Expect(DrawSolidArcSketch(Linked, Spoke));
        const std::string Follower = Linked.Tree.Steps.back().Label;
        const std::vector<std::string> Sources = SolidArcLinkSources(Linked, Follower);
        const int Unlinked = Freedom(Linked, Follower);
        const std::vector<SolidArcSketchPick> CornerPicks = { { {}, { SketchPickKind::Point, 0 } }, { Source, { SketchPickKind::Point, 2 } } };
        const SolidArcLinkedPicks CornerLinked = LinkSolidArcPicks(Linked, Follower, CornerPicks);
        const SketchConstraintProposal CornerProposal = ProposeSketchConstraint(CornerLinked.Sketch, CornerLinked.Picks, SketchConstraintVerb::On);
        const size_t BeforeCorner = Linked.UndoEntries.size();
        const SolidArcCommandOutcome Cornered = ConstrainSolidArcSketch(Linked, Follower, CornerProposal.Constraint, CornerPicks);
        const bool OneEntry = Linked.UndoEntries.size() == BeforeCorner + 1;
        const int AfterCorner = Freedom(Linked, Follower);
        const std::vector<SolidArcSketchPick> EdgePicks = { { {}, { SketchPickKind::Point, 1 } }, { Source, { SketchPickKind::Curve, 2 } } };
        const SolidArcLinkedPicks EdgeLinked = LinkSolidArcPicks(Linked, Follower, EdgePicks);
        const SketchConstraintProposal EdgeProposal = ProposeSketchConstraint(EdgeLinked.Sketch, EdgeLinked.Picks, SketchConstraintVerb::On);
        const SolidArcCommandOutcome Edged = ConstrainSolidArcSketch(Linked, Follower, EdgeProposal.Constraint, EdgePicks);
        const int AfterEdge = Freedom(Linked, Follower);
        const SketchModel& Solved = Linked.Regeneration.Sketches.at(Follower);
        const double CornerGap = std::abs(Solved.Points[0].Position.X - 3.0) + std::abs(Solved.Points[0].Position.Y - 2.0);
        const double EdgeGap   = std::abs(Solved.Points[1].Position.Y - 2.0);
        const SketchModel& Authored = Linked.Tree.Steps.back().Sketch;
        const bool Shapes = Authored.Points.size() == 4 && Authored.Points[2].Link.Sketch == Source && Authored.Points[2].Link.Index == 2 && Authored.Curves.size() == 2 && Authored.Curves[1].Construction
                         && Authored.Curves[1].Link.Sketch == Source && Authored.Curves[1].Link.Index == 2 && CornerLinked.Appended == 1 && EdgeLinked.Appended == 2;
        Tally(LinkGate, CornerGap + EdgeGap,
              Sources.size() == 1 && Sources[0] == Source && CornerLinked.Refusal.empty() && CornerProposal.Refusal.empty() && Cornered.AcceptedCondition && OneEntry && Edged.AcceptedCondition && Shapes
                  && Unlinked == 4 && AfterCorner == 2 && AfterEdge == 1 && CornerGap <= 1.0e-12 && EdgeGap <= 1.0e-12,
              "DOF " + std::to_string(Unlinked) + " → " + Cornered.Label + " " + std::to_string(AfterCorner) + " → " + Edged.Label + " " + std::to_string(AfterEdge) + "; corner gap " + Scientific(CornerGap) + ", edge gap " + Scientific(EdgeGap));

        // W28 ② A's D1 = 4: B regenerates with its start on (4, 2) and its end still on the top line; the K session's view
        //        (DiagnoseSolidArcSketch) shows the same before B is touched
        const SolidArcCommandOutcome Widened = ChangeSolidArcDimension(Linked, Source, 5, 4.0);
        const SketchModel& Moved = Linked.Regeneration.Sketches.at(Follower);
        const SolidArcSketchDiagnosis Viewed = DiagnoseSolidArcSketch(Linked, Follower);
        const double MovedGap = std::abs(Moved.Points[0].Position.X - 4.0) + std::abs(Moved.Points[0].Position.Y - 2.0) + std::abs(Moved.Points[1].Position.Y - 2.0);
        const double ViewGap  = std::abs(Viewed.Sketch.Points[0].Position.X - 4.0) + std::abs(Viewed.Sketch.Points[0].Position.Y - 2.0);
        bool Quiet = true;
        for (const FeatureStepReport& Report : Linked.Regeneration.Steps) Quiet = Quiet && Report.Notes.empty();
        Tally(LinkGate, MovedGap + ViewGap, Widened.AcceptedCondition && MovedGap <= 1.0e-12 && ViewGap <= 1.0e-12 && Quiet && Relative(Linked.Summary.Volume, 8.0) <= 1.0e-9,
              Widened.Label + ": B's start follows to (4, 2), end on y = 2 (gap " + Scientific(MovedGap) + "), K view gap " + Scientific(ViewGap) + ", no notes, V 8");

        // W28 ③ refusals: A linking to the later B, dragging the linked point, a non-existent source point — untouched
        const std::string LinkedFile = EncodeSolidArcDocument(Linked);
        const size_t LinkedHistory = Linked.UndoEntries.size();
        const SolidArcLinkedPicks Later = LinkSolidArcPicks(Linked, Source, { { {}, { SketchPickKind::Point, 0 } }, { Follower, { SketchPickKind::Point, 0 } } });
        const SolidArcCommandOutcome Backward = ConstrainSolidArcSketch(Linked, Source, CornerProposal.Constraint, { { {}, { SketchPickKind::Point, 0 } }, { Follower, { SketchPickKind::Point, 0 } } });
        const SolidArcCommandOutcome Held = DragSolidArcSketchPoint(Linked, Follower, 2, { 9.0, 9.0 });
        const SolidArcLinkedPicks Beyond = LinkSolidArcPicks(Linked, Follower, { { Source, { SketchPickKind::Point, 40 } } });
        const bool LinkedUntouched = EncodeSolidArcDocument(Linked) == LinkedFile && Linked.UndoEntries.size() == LinkedHistory;
        Tally(LinkGate, 0.0, Later.Refusal.find("comes after") != std::string::npos && !Backward.AcceptedCondition && !Held.AcceptedCondition && Held.Refusal.find("is linked to") != std::string::npos
                  && Beyond.Refusal.find("has no point 40") != std::string::npos && LinkedUntouched,
              "refused: " + Later.Refusal + " · " + Held.Refusal + " · " + Beyond.Refusal + "; file and history untouched");

        // W28 ④ save → load → save byte-equal (Links, LinkSketch); Delete unlinks the corner: B's start stays at (4, 2), free,
        //        and no longer follows A (D1 = 5 leaves it); undo relinks it
        Tally(LinkGate, 0.0, RoundTrip(Linked) && LinkedFile.find("[[Steps.Sketch.Links]]") != std::string::npos && LinkedFile.find("LinkSketch = ") != std::string::npos,
              "save → load → save byte-equal with [[Steps.Sketch.Links]] { Point, Sketch, Index } and the construction line's LinkSketch / LinkIndex");
        const SolidArcCommandOutcome Unlink = UnlinkSolidArcGeometry(Linked, Follower, { SketchPickKind::Point, 2 });
        const int Freed = Freedom(Linked, Follower);
        Expect(ChangeSolidArcDimension(Linked, Source, 5, 5.0));
        const SketchModel& Stayed = Linked.Regeneration.Sketches.at(Follower);
        const double StayGap = std::abs(Stayed.Points[0].Position.X - 4.0) + std::abs(Stayed.Points[0].Position.Y - 2.0);
        const bool Relinked = UndoSolidArcCommand(Linked) && UndoSolidArcCommand(Linked) && !Linked.Tree.Steps.back().Sketch.Points[2].Link.Sketch.empty();
        Tally(LinkGate, StayGap, Unlink.AcceptedCondition && Freed == AfterEdge + 2 && StayGap <= 1.0e-12 && Relinked,
              Unlink.Label + ": DOF " + std::to_string(Freed) + ", start stays at (4, 2) when D1 = 5 (gap " + Scientific(StayGap) + "); undo relinks");
    }

    // ---- W29 … W30: Revolve (Alt+R) and Evolution (Shift+P / Alt+E)
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Figures  = [](const SolidArcDocument& Target, const std::string& Key) -> const SolidArcBodyFigures*
        {
            for (const SolidArcBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return &Body;
            return nullptr;
        };
        auto VolumeOf = [&](const SolidArcDocument& Target, const std::string& Key) { const SolidArcBodyFigures* Body = Figures(Target, Key); return Body ? Body->Volume : -1.0; };
        auto Draw = [](SolidArcDocument& Target, SolidArcDrawingKind Kind, std::vector<Vector2> Points, int GridPlane = 0)
        {
            SolidArcDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points); Drawing.GridPlane = GridPlane;
            return DrawSolidArcSketch(Target, Drawing).AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto RoundTrip = [](const SolidArcDocument& Source)
        {
            const std::string Written = EncodeSolidArcDocument(Source);
            const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            return Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Summary.Volume == Source.Summary.Volume;
        };
        auto Scientific = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3e", Value); return std::string(Text); };
        auto Verdict = [](const SolidArcCommandOutcome& Outcome) { return Outcome.AcceptedCondition ? Outcome.Label : Outcome.Label + " REFUSED (" + Outcome.Refusal + ")"; };
        auto Occurrences = [](const std::string& Text, const std::string& Word) { int Count = 0; for (size_t At = Text.find(Word); At != std::string::npos; At = Text.find(Word, At + 1)) ++Count; return Count; };

        // W29 ① a 1 × 1 square at x ∈ [1, 2] on XY: the proposed axis is the sketch y-axis (the profile lies right of it);
        //        a full turn → the first body (New) is an annulus π(R² − r²)h = 3π
        SolidArcDocument Turned = CreateSolidArcDocument("Revolve");
        const std::string Ring = Draw(Turned, SolidArcDrawingKind::Rectangle, { { 1.0, 0.0 }, { 2.0, 1.0 } });
        const SolidArcRevolveAxis Proposed = ProposeSolidArcRevolveAxis(Turned, Ring);
        const SolidArcCommandOutcome Annulus = RevolveSolidArcSketch(Turned, Ring, Proposed, 2.0 * Pi, FeatureCombination::New);
        const std::string RingKey = Annulus.AcceptedCondition ? Turned.Tree.Steps.back().Label : std::string{};
        const double RingVolume = VolumeOf(Turned, RingKey);
        Tally(RevolveGate, Relative(RingVolume, 3.0 * Pi),
              Annulus.AcceptedCondition && Proposed.Line < 0 && Proposed.Direction.X == 0.0 && Proposed.Direction.Y == 1.0 && Turned.Tree.Steps.back().Combination == FeatureCombination::New
                  && Relative(RingVolume, 3.0 * Pi) <= 1.0e-9 && Turned.Summary.SoundCondition,
              Verdict(Annulus) + ": proposed axis " + DescribeSolidArcRevolveAxis(Turned, Ring, Proposed) + ", V 3π (rel. " + Scientific(Relative(RingVolume, 3.0 * Pi)) + "), New, sound");

        // W29 ② a 1.5 × 1 plate on the y-axis (x ∈ [0, 1.5], y ∈ [−½, ½]) turned about its own left edge (a line of the
        //        sketch, by identifier) and united with the annulus: 3π + 2.25π − overlap 0.625π = 4.625π; undo / redo exact
        const std::string Plate = Draw(Turned, SolidArcDrawingKind::Rectangle, { { 0.0, -0.5 }, { 1.5, 0.5 } });
        SolidArcRevolveAxis Edge;
        for (const SketchCurve& Curve : Turned.Regeneration.Sketches.at(Plate).Curves)
        {
            const SketchModel& Model = Turned.Regeneration.Sketches.at(Plate);
            if (Curve.Kind == SketchCurveKind::Line && Model.Points[Curve.Start].Position.X == 0.0 && Model.Points[Curve.End].Position.X == 0.0) Edge.Line = Curve.Identifier;
        }
        const SolidArcCommandOutcome United = RevolveSolidArcSketch(Turned, Plate, Edge, 2.0 * Pi, FeatureCombination::Union, RingKey);
        const double UnionVolume = VolumeOf(Turned, RingKey);
        const bool   Rewound = UndoSolidArcCommand(Turned) && VolumeOf(Turned, RingKey) == RingVolume && RedoSolidArcCommand(Turned) && VolumeOf(Turned, RingKey) == UnionVolume;
        Tally(RevolveGate, Relative(UnionVolume, 4.625 * Pi),
              United.AcceptedCondition && Edge.Line >= 0 && Relative(UnionVolume, 4.625 * Pi) <= 1.0e-9 && Turned.Summary.Bodies.size() == 1 && Turned.Summary.SoundCondition && Rewound,
              Verdict(United) + ": axis by line identifier " + std::to_string(Edge.Line) + ", V 4.625π (rel. " + Scientific(Relative(UnionVolume, 4.625 * Pi)) + "), one sound body; undo / redo bit-equal");

        // W29 ③ a 2 × 1 plate across the y-axis (x ∈ [−1, 1], y ∈ [1, 2]): the proposal falls back to the sketch x-axis;
        //        a quarter turn → (θ/2)(R² − r²)w = 3π/2
        SolidArcDocument Quarter = CreateSolidArcDocument("Quarter");
        const std::string Bar = Draw(Quarter, SolidArcDrawingKind::Rectangle, { { -1.0, 1.0 }, { 1.0, 2.0 } });
        const SolidArcRevolveAxis Across = ProposeSolidArcRevolveAxis(Quarter, Bar);
        const SolidArcCommandOutcome Bent = RevolveSolidArcSketch(Quarter, Bar, Across, 0.5 * Pi, FeatureCombination::Separate);
        const double BentVolume = VolumeOf(Quarter, Bent.AcceptedCondition ? Quarter.Tree.Steps.back().Label : std::string{});
        Tally(RevolveGate, Relative(BentVolume, 1.5 * Pi),
              Bent.AcceptedCondition && Across.Line < 0 && Across.Direction.X == 1.0 && Across.Direction.Y == 0.0 && Relative(BentVolume, 1.5 * Pi) <= 1.0e-9 && Quarter.Summary.SoundCondition,
              Verdict(Bent) + ": proposed " + DescribeSolidArcRevolveAxis(Quarter, Bar, Across) + ", V 3π/2 (rel. " + Scientific(Relative(BentVolume, 1.5 * Pi)) + "), sound");

        // W29 ④ refusals leave file and history alone: no angle, more than a turn, a profile across the axis, no sketch,
        //        Difference with nothing to subtract from
        SolidArcDocument Empty = CreateSolidArcDocument("Refusals");
        const std::string Across2 = Draw(Empty, SolidArcDrawingKind::Rectangle, { { -1.0, 0.0 }, { 1.0, 1.0 } });
        const std::string Before  = EncodeSolidArcDocument(Empty);
        const SolidArcRevolveAxis YAxis{ -1, { 0.0, 0.0 }, { 0.0, 1.0 } };
        const SolidArcCommandOutcome Nothing  = RevolveSolidArcSketch(Empty, Across2, Across, 0.0);
        const SolidArcCommandOutcome Overturn = RevolveSolidArcSketch(Empty, Across2, Across, 2.5 * Pi);
        const SolidArcCommandOutcome Crossing = RevolveSolidArcSketch(Empty, Across2, YAxis, Pi);
        const SolidArcCommandOutcome Missing  = RevolveSolidArcSketch(Empty, "Sketch#99", YAxis, Pi);
        const SolidArcCommandOutcome Subtract = RevolveSolidArcSketch(Empty, Across2, Across, Pi, FeatureCombination::Difference);
        Tally(RevolveGate, 0.0, !Nothing.AcceptedCondition && !Overturn.AcceptedCondition && !Crossing.AcceptedCondition && !Missing.AcceptedCondition && !Subtract.AcceptedCondition
                                && EncodeSolidArcDocument(Empty) == Before && Crossing.Refusal.find("crosses the axis") != std::string::npos,
              "refused: " + Nothing.Refusal + " · " + Overturn.Refusal + " · " + Crossing.Refusal + " · " + Missing.Refusal + " · " + Subtract.Refusal + "; file untouched");

        // W29 ⑤ codec: the point + direction axis is written only without a line (older files stay byte-equal)
        const std::string TurnedFile = EncodeSolidArcDocument(Turned), QuarterFile = EncodeSolidArcDocument(Quarter);
        Tally(RevolveGate, 0.0, RoundTrip(Turned) && RoundTrip(Quarter) && Occurrences(TurnedFile, "AxisDirection") == 1 && QuarterFile.find("AxisDirection = [ 1.0, 0.0 ]") != std::string::npos,
              "save → load → save byte-equal; AxisOrigin / AxisDirection only on the y- / x-axis revolves (" + std::to_string(Occurrences(TurnedFile, "AxisDirection")) + " in the union file)");

        // W30 ① a 1 × ½ rectangle on XY and a line (0, 0) → (0, 2) on XZ (world +z): the proposal takes the line sketch;
        //        the rigid sweep is a prism A·L = 1
        SolidArcDocument Swept = CreateSolidArcDocument("Evolution");
        const std::string Slab = Draw(Swept, SolidArcDrawingKind::Rectangle, { { 0.0, 0.0 }, { 1.0, 0.5 } });
        const std::string Rail = Draw(Swept, SolidArcDrawingKind::Line, { { 0.0, 0.0 }, { 0.0, 2.0 } }, 1);
        const std::string Chosen = ProposeSolidArcEvolutionPath(Swept, Slab);
        const SolidArcCommandOutcome Prism = EvolveSolidArcProfile(Swept, Slab, Chosen, FeatureCombination::New);
        const std::string PrismKey = Prism.AcceptedCondition ? Swept.Tree.Steps.back().Label : std::string{};
        Tally(EvolutionGate, Relative(VolumeOf(Swept, PrismKey), 1.0),
              Prism.AcceptedCondition && Chosen == Rail && Relative(VolumeOf(Swept, PrismKey), 1.0) <= 1.0e-9 && Swept.Summary.SoundCondition && Swept.Tree.Steps.back().Combination == FeatureCombination::New,
              Verdict(Prism) + ": path proposed " + Chosen + ", V 1 (rel. " + Scientific(Relative(VolumeOf(Swept, PrismKey), 1.0)) + "), New, sound");

        // W30 ② a circle r ¼ about the origin swept along a quarter arc of radius 2 on XZ (start ⟂ the profile): Pappus
        //        πr² · Rπ/2 = π²/16 (the side is a NURBS skin within 10⁻⁶ of the canal)
        SolidArcDocument Bend = CreateSolidArcDocument("Bend");
        const std::string Disc = Draw(Bend, SolidArcDrawingKind::Circle, { { -0.25, 0.0 }, { 0.25, 0.0 } });
        const std::string Arc  = Draw(Bend, SolidArcDrawingKind::Arc, { { 0.0, 0.0 }, { 2.0, 2.0 }, { 2.0 - std::sqrt(2.0), std::sqrt(2.0) } }, 1);
        const SolidArcCommandOutcome Elbow = EvolveSolidArcProfile(Bend, Disc, Arc, FeatureCombination::Separate);
        const double ElbowVolume = VolumeOf(Bend, Elbow.AcceptedCondition ? Bend.Tree.Steps.back().Label : std::string{});
        Tally(EvolutionGate, Relative(ElbowVolume, Pi * Pi / 16.0),
              Elbow.AcceptedCondition && Relative(ElbowVolume, Pi * Pi / 16.0) <= 1.0e-6 && Bend.Summary.SoundCondition,
              Verdict(Elbow) + ": the arc is stored from (2, 2) (counter-clockwise), the sweep starts at its end nearer the profile; V π²/16 (rel. " + Scientific(Relative(ElbowVolume, Pi * Pi / 16.0)) + "), sound");

        // W30 ③ groove: a 2 × 1 × 1 block (XZ rectangle x ∈ [−1, 1], z ∈ [½, 1½] extruded 1 along −y) minus a ½ × ½
        //        square swept along +z through it: the block keeps 2 − ½ · ¼ · 1 = 1.875 (the square straddles y = 0)
        SolidArcDocument Groove = CreateSolidArcDocument("Groove");
        const std::string Face = Draw(Groove, SolidArcDrawingKind::Rectangle, { { -1.0, 0.5 }, { 1.0, 1.5 } }, 1);
        Expect(ExtrudeSolidArcSketch(Groove, Face, 1.0, FeatureCombination::Separate));
        const std::string BlockKey = Groove.Tree.Steps.back().Label;
        const double BlockVolume = VolumeOf(Groove, BlockKey);
        const std::string Square = Draw(Groove, SolidArcDrawingKind::Rectangle, { { -0.25, -0.25 }, { 0.25, 0.25 } });
        const std::string Spine  = Draw(Groove, SolidArcDrawingKind::Line, { { 0.0, 0.0 }, { 0.0, 2.0 } }, 1);
        const SolidArcCommandOutcome Cut = EvolveSolidArcProfile(Groove, Square, Spine, FeatureCombination::Difference, BlockKey);
        const double GrooveVolume = VolumeOf(Groove, BlockKey);
        const double GrooveExpected = BlockVolume - 0.125;
        Tally(EvolutionGate, Relative(GrooveVolume, GrooveExpected),
              Cut.AcceptedCondition && Relative(BlockVolume, 2.0) <= 1.0e-12 && Relative(GrooveVolume, GrooveExpected) <= 1.0e-6 && Groove.Summary.Bodies.size() == 1 && Groove.Summary.SoundCondition,
              Verdict(Cut) + ": block 2 → " + std::to_string(GrooveVolume) + " vs 1.875 (rel. " + Scientific(Relative(GrooveVolume, GrooveExpected)) + "), one sound body");

        // W30 ④ refusals (path = profile, no path, a 4-curve path, a path leaving along the profile plane, Difference with
        //        no body) leave the file alone; codec byte-equal with Kind 16 and its Path
        const std::string Flat = Draw(Swept, SolidArcDrawingKind::Line, { { 3.0, 0.0 }, { 5.0, 0.0 } });
        const std::string SweptBefore = EncodeSolidArcDocument(Swept);
        const SolidArcCommandOutcome Self   = EvolveSolidArcProfile(Swept, Slab, Slab);
        const SolidArcCommandOutcome NoPath = EvolveSolidArcProfile(Swept, Slab, "");
        const SolidArcCommandOutcome Many   = EvolveSolidArcProfile(Swept, Rail, Slab);
        const SolidArcCommandOutcome InPlane = EvolveSolidArcProfile(Swept, Slab, Flat);
        SolidArcDocument Bare = CreateSolidArcDocument("Bare");
        const std::string BareSlab = Draw(Bare, SolidArcDrawingKind::Rectangle, { { 0.0, 0.0 }, { 1.0, 0.5 } });
        const std::string BareRail = Draw(Bare, SolidArcDrawingKind::Line, { { 0.0, 0.0 }, { 0.0, 2.0 } }, 1);
        const SolidArcCommandOutcome Hollow = EvolveSolidArcProfile(Bare, BareSlab, BareRail, FeatureCombination::Difference);
        const std::string SweptAfter = EncodeSolidArcDocument(Swept);
        Tally(EvolutionGate, 0.0, !Self.AcceptedCondition && !NoPath.AcceptedCondition && !Many.AcceptedCondition && !InPlane.AcceptedCondition && !Hollow.AcceptedCondition && SweptAfter == SweptBefore,
              "refused: " + Self.Refusal + " · " + NoPath.Refusal + " · " + Many.Refusal + " · " + InPlane.Refusal + " · " + Hollow.Refusal + "; file untouched");
        Tally(EvolutionGate, 0.0, RoundTrip(Swept) && RoundTrip(Bend) && RoundTrip(Groove) && SweptAfter.find("Kind = 16") != std::string::npos && SweptAfter.find("Path = '" + Rail + "'") != std::string::npos,
              "save → load → save byte-equal with the Evolution (16) steps and Path = '" + Rail + "'");
    }

    // ---- W31 incremental replay: identical to full replays, resume points, speed
    {
        // A copy regenerated from scratch (cache off): the reference every incremental state must equal.
        auto FullReplay = [](const SolidArcDocument& Source)
        {
            SolidArcDocument Copy = CreateSolidArcDocument(Source.Title);
            Copy.Tree      = Source.Tree;
            Copy.Captions  = Source.Captions;
            Copy.Selection = Source.Selection;
            Copy.Replay.EnabledCondition = false;
            RegenerateSolidArcDocument(Copy);
            return Copy;
        };
        auto Identical = [](const SolidArcDocument& First, const SolidArcDocument& Second)
        {
            const SolidArcBodySummary& A = First.Summary;
            const SolidArcBodySummary& B = Second.Summary;
            if (A.Volume != B.Volume || A.Faces != B.Faces || A.Edges != B.Edges || A.Vertices != B.Vertices || A.Genus != B.Genus || A.Bodies.size() != B.Bodies.size()) return false;
            for (size_t Index = 0; Index < A.Bodies.size(); ++Index) if (A.Bodies[Index].Key != B.Bodies[Index].Key || A.Bodies[Index].Volume != B.Bodies[Index].Volume) return false;
            const RegenerationOutcome& P = First.Regeneration;
            const RegenerationOutcome& Q = Second.Regeneration;
            if (P.Bodies.size() != Q.Bodies.size() || P.Steps.size() != Q.Steps.size() || P.Active != Q.Active || P.Refused != Q.Refused || P.Curves.size() != Q.Curves.size()) return false;
            for (size_t Index = 0; Index < P.Bodies.size(); ++Index)
            {
                const RegeneratedBody& X = P.Bodies[Index];
                const RegeneratedBody& Y = Q.Bodies[Index];
                if (X.Key != Y.Key || X.Body.Slot != Y.Body.Slot || X.Body.Generation != Y.Body.Generation) return false;
                if (X.Naming.Faces != Y.Naming.Faces || X.Naming.Edges != Y.Naming.Edges || X.Naming.Vertices != Y.Naming.Vertices) return false;
            }
            for (size_t Index = 0; Index < P.Steps.size(); ++Index)
            {
                if (P.Steps[Index].Status != Q.Steps[Index].Status || P.Steps[Index].Refusal != Q.Steps[Index].Refusal || P.Steps[Index].Body.Slot != Q.Steps[Index].Body.Slot) return false;
            }
            return EncodeSolidArcDocument(First) == EncodeSolidArcDocument(Second);
        };
        auto Milliseconds = [](auto&& Action)
        {
            const auto Started = std::chrono::steady_clock::now();
            Action();
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Started).count();
        };
        auto Ms = [](double Value) { char Text[32]; std::snprintf(Text, sizeof Text, "%.1f ms", Value); return std::string(Text); };
        auto Times = [](double Value) { char Text[32]; std::snprintf(Text, sizeof Text, "%.1f×", Value); return std::string(Text); };

        // Build: block + 8 bosses (union) + 4 holes (difference) + a front fillet, every command checked against a full replay.
        SolidArcDocument Studded = CreateSolidArcDocument("Studded");
        int  Commands = 0, IdenticalCount = 0, AppendResumes = 0;
        auto Command = [&](SolidArcCommandOutcome Outcome)
        {
            const size_t Before = Studded.UndoEntries.empty() ? 0 : Studded.UndoEntries.back().Tree.Steps.size();
            Expect(Outcome);
            ++Commands;
            IdenticalCount += Identical(Studded, FullReplay(Studded)) ? 1 : 0;
            AppendResumes  += static_cast<size_t>(Studded.Regeneration.Resumed) == Before ? 1 : 0;
        };
        SolidArcConstructRequest Plate;
        Plate.Kind = SolidArcConstructKind::Block; Plate.Width = 8.0; Plate.Depth = 3.0; Plate.Height = 0.6;
        Command(ConstructSolidArcPrimitive(Studded, Plate));
        for (int Boss = 0; Boss < 8; ++Boss)
        {
            SolidArcConstructRequest Stud;
            Stud.Kind = SolidArcConstructKind::Cylinder; Stud.Position = { 0.75 + 0.9 * Boss, 2.1, 0.3 }; Stud.Radius = 0.25 + 0.01 * Boss; Stud.Height = 0.5 + 0.05 * Boss;
            Stud.Combination = FeatureCombination::Union;
            Command(ConstructSolidArcPrimitive(Studded, Stud));
        }
        for (int Hole = 0; Hole < 4; ++Hole)
        {
            SolidArcConstructRequest Drill;
            Drill.Kind = SolidArcConstructKind::Cylinder; Drill.Position = { 1.3 + 1.8 * Hole, 0.9, -0.5 }; Drill.Radius = 0.2; Drill.Height = 2.0;
            Drill.Combination = FeatureCombination::Difference;
            Command(ConstructSolidArcPrimitive(Studded, Drill));
        }
        const std::string FrontEdge = PickAndSelect(Studded, RayBetween({ 4.0, -1.0, 1.6 }, { 4.0, 0.0, 0.6 }), false);
        SelectSolidArcName(Studded, SolidArcSelectionKind::Edge, FrontEdge, false);
        Command(BlendSolidArcSelection(Studded, BlendProfile::Fillet, 0.15));
        const int Steps = static_cast<int>(Studded.Tree.Steps.size());
        Tally(ReplayGate, 0.0, IdenticalCount == Commands && AppendResumes == Commands && Steps == 27 && Studded.Regeneration.Refused == 0,
              std::to_string(Commands) + " commands → " + std::to_string(Steps) + " steps: " + std::to_string(IdenticalCount) + " identical to a full replay, " + std::to_string(AppendResumes) + " resumed at the new step");

        // Speed: full replay vs the incremental paths (each result identical to a full replay of the same tree).
        double Full = 1.0e300;
        for (int Run = 0; Run < 3; ++Run) Full = std::min(Full, Milliseconds([&]() { (void)FullReplay(Studded); }));
        auto Timed = [&](const std::string& Caption, auto&& Action, int ExpectedResume, double Least)
        {
            const double Time = Milliseconds(Action);
            const bool Same = Identical(Studded, FullReplay(Studded));
            const int Resumed = Studded.Regeneration.Resumed;
            Tally(ReplayGate, Full / Time, Same && Resumed == ExpectedResume && Full / Time >= Least,
                  Caption + ": resumed at " + std::to_string(Resumed) + "/" + std::to_string(Studded.Tree.Steps.size()) + " · " + Ms(Time) + " vs full " + Ms(Full) + " · " + Times(Full / Time) + " · identical");
        };
        int BindingStep = -1;
        for (int Index = 0; Index < Steps && BindingStep < 0; ++Index)
        {
            for (const SketchConstraint& Constraint : Studded.Tree.Steps[Index].Sketch.Constraints) if (Constraint.Parameter == "Cylinder6.Radius") BindingStep = Index;
        }
        Timed("tail edit Fillet1.Size 0.15 → 0.12", [&]() { Expect(AssignSolidArcParameter(Studded, "Fillet1.Size", 0.12)); }, Steps - 1, 2.0);
        Timed("middle edit Cylinder6.Radius 0.30 → 0.32", [&]() { Expect(AssignSolidArcParameter(Studded, "Cylinder6.Radius", 0.32)); }, BindingStep, 0.0);
        Timed("undo (Cylinder6.Radius back)", [&]() { (void)UndoSolidArcCommand(Studded); }, Steps, 10.0);
        Timed("redo", [&]() { (void)RedoSolidArcCommand(Studded); }, Steps, 10.0);
        Timed("undo again", [&]() { (void)UndoSolidArcCommand(Studded); }, Steps, 10.0);
        {
            std::unique_ptr<SolidArcDocument> Preview;
            const double Time = Milliseconds([&]() { Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(Studded)); });
            Tally(ReplayGate, Full / Time, Identical(*Preview, Studded) && Preview->Regeneration.Resumed == Steps && Full / Time >= 10.0,
                  "preview clone (bevel / move / sweep staging): resumed at " + std::to_string(Preview->Regeneration.Resumed) + "/" + std::to_string(Steps) + " · " + Ms(Time) + " · " + Times(Full / Time));
            SolidArcBlendRequest Wider;
            Wider.Size = 0.2;
            const std::string BackEdge = PickAndSelect(*Preview, RayBetween({ 4.0, 4.0, 1.6 }, { 4.0, 3.0, 0.6 }), false);
            SelectSolidArcName(*Preview, SolidArcSelectionKind::Edge, BackEdge, false);
            const double Staged = Milliseconds([&]() { Expect(BlendSolidArcSelection(*Preview, Wider)); });
            // Fillet1 was last replayed by the (non-capturing) parameter edit: this authoring command re-captures its edge
            // signature in place from the checkpoint before it, so only the new fillet replays (27).
            Tally(ReplayGate, 0.0, Identical(*Preview, FullReplay(*Preview)) && Preview->Regeneration.Resumed == Steps && Identical(Studded, FullReplay(Studded)),
                  "preview fillets the back edge " + BackEdge + ": resumed at " + std::to_string(Preview->Regeneration.Resumed) + " (Fillet1 re-captured in place) · " + Ms(Staged) + " · source untouched");
        }

        // History depth: 10 more edits; only the 8 newest undo entries keep checkpoints; undoing all 10 stays identical.
        for (int Edit = 0; Edit < 10; ++Edit) Expect(AssignSolidArcParameter(Studded, "Fillet1.Size", 0.1 + 0.005 * Edit));
        int Kept = 0;
        for (const SolidArcHistoryEntry& Entry : Studded.UndoEntries) Kept += Entry.Replay ? 1 : 0;
        int UndoIdentical = 0, UndoRestored = 0;
        for (int Edit = 0; Edit < 10; ++Edit)
        {
            (void)UndoSolidArcCommand(Studded);
            UndoIdentical += Identical(Studded, FullReplay(Studded)) ? 1 : 0;
            UndoRestored  += Studded.Regeneration.Resumed == Steps ? 1 : 0;
        }
        Tally(ReplayGate, 0.0, Kept == 8 && Studded.UndoEntries.size() > 8 && UndoIdentical == 10 && UndoRestored >= 8,
              std::to_string(Studded.UndoEntries.size() + 10) + " undo entries, " + std::to_string(Kept) + " with checkpoints; 10 undos identical, " + std::to_string(UndoRestored) + " restored every step (older ones resume at the edited step)");

        // Vehicle bonnet: 11 SpaceCurves + loft; a pole move on one section resumes at that curve.
        SolidArcDocument Bonnet = CreateSolidArcDocument("Bonnet");
        Expect(ImportSolidArcCurves(Bonnet, ReadVehicleCurves(SOLIDARC_VEHICLE_SAMPLE)));
        std::vector<std::string> Sections;
        for (const FeatureStep& Step : Bonnet.Tree.Steps) if (Bonnet.Captions[Step.Label].rfind("Section", 0) == 0) Sections.push_back(Step.Label);
        Expect(LoftSolidArcCurves(Bonnet, Sections));
        int SectionStep = -1;
        for (size_t Index = 0; Index < Bonnet.Tree.Steps.size(); ++Index) if (Bonnet.Tree.Steps[Index].Label == Sections.back()) SectionStep = static_cast<int>(Index);
        const Vector3 Pole = Bonnet.Regeneration.Curves.at(Sections.back()).Curve.Definition->Poles[2];
        double BonnetFull = 1.0e300;
        for (int Run = 0; Run < 3; ++Run) BonnetFull = std::min(BonnetFull, Milliseconds([&]() { (void)FullReplay(Bonnet); }));
        const double Moved = Milliseconds([&]() { Expect(MoveSolidArcPole(Bonnet, Sections.back(), 2, Pole + Vector3{ 0.0, 0.0, 0.01 })); });
        const bool MovedSame = Identical(Bonnet, FullReplay(Bonnet));
        const int MovedResume = Bonnet.Regeneration.Resumed;
        const double Undone = Milliseconds([&]() { (void)UndoSolidArcCommand(Bonnet); });
        Tally(ReplayGate, BonnetFull / Undone, MovedSame && MovedResume == SectionStep && Identical(Bonnet, FullReplay(Bonnet)) && Bonnet.Regeneration.Resumed == static_cast<int>(Bonnet.Tree.Steps.size()),
              "bonnet (" + std::to_string(Bonnet.Tree.Steps.size()) + " steps, full " + Ms(BonnetFull) + "): pole move resumed at " + std::to_string(MovedResume) + " · " + Ms(Moved) + "; undo restored all · " + Ms(Undone) + " · identical");
    }

    // ---- W32 x-ray: pick-through and box edges (closed form: the isometric eye sits at +x, −y, +z, so exactly the 3 edges
    //      meeting the far corner (0, 2, 0) are hidden)
    {
        SolidArcDocument Ghost = CreateSolidArcDocument("Ghost");
        SolidArcConstructRequest Brick;
        Brick.Kind = SolidArcConstructKind::Block; Brick.Width = 4.0; Brick.Depth = 2.0; Brick.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Ghost, Brick));
        const SolidArcScene Scene = BuildSolidArcScene(Ghost);
        auto AtCorner = [](const Vector3& Point, double X, double Y, double Z)
        {
            return std::abs(Point.X - X) + std::abs(Point.Y - Y) + std::abs(Point.Z - Z) < 1.0e-9;
        };
        auto TouchesFarCorner = [&](const std::string& Name)
        {
            return AtCorner(EdgeEndPosition(Ghost, Name, false), 0.0, 2.0, 0.0) || AtCorner(EdgeEndPosition(Ghost, Name, true), 0.0, 2.0, 0.0);
        };
        auto LiesOn = [&](const std::string& Name, double Y, double Z)                       // Edge along x at (y, z)
        {
            const Vector3 Start = EdgeEndPosition(Ghost, Name, false);
            const Vector3 End   = EdgeEndPosition(Ghost, Name, true);
            return std::abs(Start.Y - Y) + std::abs(End.Y - Y) + std::abs(Start.Z - Z) + std::abs(End.Z - Z) < 1.0e-9 && std::abs(Start.X - End.X) > 1.0;
        };
        auto NameOf = [&](const std::optional<SolidArcPick>& Hit) { return Hit ? NameSolidArcIdentity(Scene, Hit->Identity) : std::string("(miss)"); };

        // Pick-through: from the front-top towards the back-bottom edge (2, 2, 0)
        const OrbitRay Behind = RayBetween({ 2.0, -3.0, 4.0 }, { 2.0, 2.0, 0.0 });
        const std::optional<SolidArcPick> Opaque = PickSolidArcScene(Scene, Behind, 0.02, false);
        const std::optional<SolidArcPick> Through = PickSolidArcScene(Scene, Behind, 0.02, true);
        const bool OpaqueFace = Opaque && (Opaque->Identity & IdentityEdgeBit) == 0;
        const bool ThroughEdge = Through && (Through->Identity & IdentityEdgeBit) != 0 && LiesOn(NameOf(Through), 2.0, 0.0);
        const double Farther = (Opaque && Through) ? Through->Distance - Opaque->Distance : 0.0;
        Tally(XRayGate, 0.0, OpaqueFace && ThroughEdge && Farther > 0.5,
              "ray at the hidden back-bottom edge: opaque → " + NameOf(Opaque) + " · x-ray → " + NameOf(Through) + " (" + std::to_string(Farther) + " farther)");

        // A visible edge (front-top, y = 0, z = 1) still picks in x-ray, and the same with x-ray off
        const OrbitRay Front = RayBetween({ 2.0, -3.0, 4.0 }, { 2.0, 0.0, 1.0 });
        const std::optional<SolidArcPick> FrontOpaque  = PickSolidArcScene(Scene, Front, 0.02, false);
        const std::optional<SolidArcPick> FrontThrough = PickSolidArcScene(Scene, Front, 0.02, true);
        Tally(XRayGate, 0.0, FrontOpaque && FrontThrough && FrontOpaque->Identity == FrontThrough->Identity && (FrontThrough->Identity & IdentityEdgeBit) != 0 && LiesOn(NameOf(FrontThrough), 0.0, 1.0),
              "ray at the visible front-top edge: opaque = x-ray → " + NameOf(FrontThrough));

        // Box edges over the whole viewport in the isometric view
        OrbitCamera Camera;
        Camera.Target = { 2.0, 1.0, 0.5 }; Camera.Distance = 10.0; Camera.SceneRadius = 2.5;
        const OrbitCameraFrame Frame = FrameOrbitCamera(Camera, 800.0, 600.0);
        const std::vector<uint32_t> Visible = EncloseSolidArcEdges(Scene, Frame, { 0.0, 0.0 }, { 800.0, 600.0 }, false);
        const std::vector<uint32_t> All     = EncloseSolidArcEdges(Scene, Frame, { 0.0, 0.0 }, { 800.0, 600.0 }, true);
        long OffCorner = 0;
        for (const SolidArcSceneEdge& Edge : Scene.Edges) OffCorner += TouchesFarCorner(Edge.Name) ? 0 : 1;
        bool VisibleOffCorner = true;
        for (const uint32_t Index : Visible) VisibleOffCorner = VisibleOffCorner && !TouchesFarCorner(Scene.Edges[Index].Name);
        const bool Subset = std::includes(All.begin(), All.end(), Visible.begin(), Visible.end());
        Tally(XRayGate, 0.0, Scene.Edges.size() == 12 && OffCorner == 9 && Visible.size() == 9 && VisibleOffCorner,
              "full box without x-ray → " + std::to_string(Visible.size()) + " edges, none at the far corner (closed form 9 of " + std::to_string(Scene.Edges.size()) + ")");
        Tally(XRayGate, 0.0, All.size() == 12 && Subset, "full box with x-ray → " + std::to_string(All.size()) + " edges (the opaque 9 among them)");

        // An empty corner of the viewport (the block projects around the centre)
        const std::vector<uint32_t> NoneOpaque  = EncloseSolidArcEdges(Scene, Frame, { 0.0, 0.0 }, { 40.0, 40.0 }, false);
        const std::vector<uint32_t> NoneThrough = EncloseSolidArcEdges(Scene, Frame, { 0.0, 0.0 }, { 40.0, 40.0 }, true);
        Tally(XRayGate, 0.0, NoneOpaque.empty() && NoneThrough.empty(), "empty 40 × 40 px corner box → " + std::to_string(NoneOpaque.size()) + " / " + std::to_string(NoneThrough.size()) + " edges");
    }

    // ---- W33 measure + section: exact figures of a 4 × 2 × 1 block (corner at the origin) and a second 2 × 2 × 1 body at
    //      x ∈ [6, 8], against closed forms (unit density)
    {
        SolidArcDocument Gauge = CreateSolidArcDocument("Gauge");
        SolidArcConstructRequest Brick;
        Brick.Kind = SolidArcConstructKind::Block; Brick.Width = 4.0; Brick.Depth = 2.0; Brick.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Gauge, Brick));
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0, std::abs(Expected)); };
        auto Near = [](const Vector3& Point, double X, double Y, double Z) { return std::abs(Point.X - X) + std::abs(Point.Y - Y) + std::abs(Point.Z - Z); };
        auto Row = [](const SolidArcMeasurement& Measurement, const std::string& Caption)
        {
            for (const SolidArcMeasureRow& Each : Measurement.Rows) if (Each.Caption == Caption) return Each.Number;
            return std::nan("");
        };
        auto Select = [&](std::initializer_list<OrbitRay> Rays)
        {
            ClearSolidArcSelection(Gauge);
            std::string Names;
            for (const OrbitRay& Ray : Rays) Names += (Names.empty() ? "" : " + ") + PickAndSelect(Gauge, Ray, true);
            return Names;
        };
        const OrbitRay Top        = RayBetween({ 1.3, 0.7, 5.0 }, { 1.3, 0.7, 0.0 });
        const OrbitRay Bottom     = RayBetween({ 1.3, 0.7, -5.0 }, { 1.3, 0.7, 0.0 });
        const OrbitRay Front      = RayBetween({ 1.3, -5.0, 0.4 }, { 1.3, 0.0, 0.4 });
        const OrbitRay FrontTop   = RayBetween({ 2.0, -3.0, 4.0 }, { 2.0, 0.0, 1.0 });
        const OrbitRay BackBottom = RayBetween({ 2.0, 5.0, -3.0 }, { 2.0, 2.0, 0.0 });

        // Nothing selected → the body: V = 8, A = 28, centroid (2, 1, ½), principal moments m(b² + c²)/12 = 10/3, 34/3, 40/3
        ClearSolidArcSelection(Gauge);
        const SolidArcMeasurement Body = MeasureSolidArcSelection(Gauge);
        const double BodyError = std::max({ Relative(Body.Volume, 8.0), Relative(Body.Area, 28.0), Near(Body.Centroid, 2.0, 1.0, 0.5),
                                            Relative(Row(Body, "Principal I1 I2 I3"), 10.0 / 3.0) });
        Tally(MeasureGate, BodyError, Body.Refusal.empty() && BodyError <= 1.0e-9 && Body.CentroidCondition,
              "nothing selected → " + Body.Subject + " · V " + std::to_string(Body.Volume) + " · A " + std::to_string(Body.Area) + " · I₁ " + std::to_string(Row(Body, "Principal I1 I2 I3")));

        // One face: top, area 8, perimeter 12, centroid (2, 1, 1)
        const std::string TopName = Select({ Top });
        const SolidArcMeasurement Face = MeasureSolidArcSelection(Gauge);
        const double FaceError = std::max({ Relative(Face.Area, 8.0), Relative(Face.Length, 12.0), Near(Face.Centroid, 2.0, 1.0, 1.0) });
        Tally(MeasureGate, FaceError, Face.Refusal.empty() && FaceError <= 1.0e-9, "face " + TopName + " → area " + std::to_string(Face.Area) + " · perimeter " + std::to_string(Face.Length));

        // One edge: front-top, length 4
        const std::string EdgeName = Select({ FrontTop });
        const SolidArcMeasurement Edge = MeasureSolidArcSelection(Gauge);
        Tally(MeasureGate, Relative(Edge.Length, 4.0), Edge.Refusal.empty() && Relative(Edge.Length, 4.0) <= 1.0e-9, "edge " + EdgeName + " → length " + std::to_string(Edge.Length));

        // Two faces: top + bottom → distance 1, normals 180°; top + front → touching (0), 90°
        const std::string Opposite = Select({ Top, Bottom });
        const SolidArcMeasurement Apart = MeasureSolidArcSelection(Gauge);
        const double ApartError = std::max({ Relative(Apart.Distance, 1.0), std::abs(Apart.Angle - 180.0) / 180.0, std::abs(Apart.Second.Z - Apart.First.Z) - 1.0 });
        Tally(MeasureGate, ApartError, Apart.DistanceCondition && ApartError <= 1.0e-9, Opposite + " → distance " + std::to_string(Apart.Distance) + " · angle " + std::to_string(Apart.Angle) + "°");
        const std::string Adjacent = Select({ Top, Front });
        const SolidArcMeasurement Touching = MeasureSolidArcSelection(Gauge);
        const double TouchingError = std::max(std::abs(Touching.Distance), std::abs(Touching.Angle - 90.0) / 90.0);
        Tally(MeasureGate, TouchingError, Touching.DistanceCondition && TouchingError <= 1.0e-9, Adjacent + " → distance " + std::to_string(Touching.Distance) + " · angle " + std::to_string(Touching.Angle) + "°");

        // Edge + face: front-top edge ∥ bottom face → distance 1, line-to-plane angle 0°; two skew-parallel edges → √5, 0°
        const std::string EdgeFace = Select({ Bottom, FrontTop });
        const SolidArcMeasurement Parallel = MeasureSolidArcSelection(Gauge);
        const double ParallelError = std::max(Relative(Parallel.Distance, 1.0), std::abs(Parallel.Angle));
        Tally(MeasureGate, ParallelError, Parallel.DistanceCondition && ParallelError <= 1.0e-9, EdgeFace + " → distance " + std::to_string(Parallel.Distance) + " · angle " + std::to_string(Parallel.Angle) + "°");
        const std::string TwoEdges = Select({ FrontTop, BackBottom });
        const SolidArcMeasurement Diagonal = MeasureSolidArcSelection(Gauge);
        const double DiagonalError = std::max(Relative(Diagonal.Distance, std::sqrt(5.0)), std::abs(Diagonal.Angle));
        Tally(MeasureGate, DiagonalError, Diagonal.DistanceCondition && DiagonalError <= 1.0e-9, TwoEdges + " → distance " + std::to_string(Diagonal.Distance) + " (√5) · angle " + std::to_string(Diagonal.Angle) + "°");

        // Three faces → total area 8 + 8 + 4 = 20
        Select({ Top, Bottom, Front });
        const SolidArcMeasurement Total = MeasureSolidArcSelection(Gauge);
        Tally(MeasureGate, Relative(Total.Area, 20.0), Total.Refusal.empty() && Relative(Total.Area, 20.0) <= 1.0e-9, Total.Subject + " → total area " + std::to_string(Total.Area));

        // Section x = 1.5 (n = −x̂, u = ŷ, v = −ẑ): 2 × 1 rectangle → A 2, P 6, centroid (1.5, 1, ½), Iuu = 2·1³/12, Ivv = 1·2³/12
        auto SectionError = [&](const SolidArcSection& Section, double Area, double Perimeter, Vector3 Centroid, double Iuu, double Ivv)
        {
            return std::max({ Relative(Section.Area, Area), Relative(Section.Perimeter, Perimeter), Near(Section.Centroid, Centroid.X, Centroid.Y, Centroid.Z),
                              Relative(Section.SecondMoments[0], Iuu), Relative(Section.SecondMoments[1], Ivv), std::abs(Section.SecondMoments[2]) });
        };
        ClearSolidArcSelection(Gauge);
        const SolidArcSection Across = SectionSolidArcDocument(Gauge, { SolidArcSectionAxis::X, 1.5, false, {} });
        const double AcrossError = SectionError(Across, 2.0, 6.0, { 1.5, 1.0, 0.5 }, 2.0 / 12.0, 8.0 / 12.0);
        Tally(MeasureGate, AcrossError, Across.Refusal.empty() && AcrossError <= 1.0e-9 && Across.Regions == 1 && Across.Loops == 1 && Across.Plane.ZAxis.X == -1.0,
              "section x = 1.5 → A " + std::to_string(Across.Area) + " · P " + std::to_string(Across.Perimeter) + " · Iuu " + std::to_string(Across.SecondMoments[0]) + " · Ivv " + std::to_string(Across.SecondMoments[1]));

        // Section z = 0.25: 4 × 2 → A 8, P 12, Iuu = 4·2³/12, Ivv = 2·4³/12
        const SolidArcSection Level = SectionSolidArcDocument(Gauge, { SolidArcSectionAxis::Z, 0.25, false, {} });
        const double LevelError = SectionError(Level, 8.0, 12.0, { 2.0, 1.0, 0.25 }, 32.0 / 12.0, 128.0 / 12.0);
        Tally(MeasureGate, LevelError, Level.Refusal.empty() && LevelError <= 1.0e-9, "section z = 0.25 → A " + std::to_string(Level.Area) + " · Ivv " + std::to_string(Level.SecondMoments[1]));

        // Face plane: the top face moved −¼ along its normal (z = ¾), kept side below
        Select({ Top });
        const SolidArcSection Facing = SectionSolidArcDocument(Gauge, { SolidArcSectionAxis::Face, -0.25, false, TopName });
        const double FacingError = std::max(SectionError(Facing, 8.0, 12.0, { 2.0, 1.0, 0.75 }, 32.0 / 12.0, 128.0 / 12.0), std::abs(Facing.Plane.ZAxis.Z + 1.0));
        Tally(MeasureGate, FacingError, Facing.Refusal.empty() && FacingError <= 1.0e-9, "section on " + TopName + " at −0.25 → plane z " + std::to_string(Facing.Plane.Origin.Z) + " · A " + std::to_string(Facing.Area));

        // Refusals: a plane past the body; a Face request naming no planar face
        const SolidArcSection Past = SectionSolidArcDocument(Gauge, { SolidArcSectionAxis::X, 10.0, false, {} });
        const SolidArcSection Nameless = SectionSolidArcDocument(Gauge, { SolidArcSectionAxis::Face, 0.0, false, "F-none" });
        Tally(MeasureGate, 0.0, Past.Refusal.find("misses") != std::string::npos && !Nameless.PlaneCondition && !Nameless.Refusal.empty(),
              "x = 10 → \"" + Past.Refusal + "\" · face F-none → \"" + Nameless.Refusal + "\"");

        // Second body 2 × 2 × 1 at x = 6: section z = ½ adds; centroid area-weighted; Ivv by the parallel-axis theorem
        SolidArcConstructRequest Second = Brick;
        Second.Combination = FeatureCombination::Separate; Second.Position = { 6.0, 0.0, 0.0 }; Second.Width = 2.0;
        Expect(ConstructSolidArcPrimitive(Gauge, Second));
        ClearSolidArcSelection(Gauge);
        const SolidArcSection Pair = SectionSolidArcDocument(Gauge, { SolidArcSectionAxis::Z, 0.5, false, {} });
        const double CentreX = (8.0 * 2.0 + 4.0 * 7.0) / 12.0;
        const double PairIvv = 2.0 * 64.0 / 12.0 + 8.0 * (2.0 - CentreX) * (2.0 - CentreX) + 2.0 * 8.0 / 12.0 + 4.0 * (7.0 - CentreX) * (7.0 - CentreX);
        const double PairIuu = 4.0 * 8.0 / 12.0 + 2.0 * 8.0 / 12.0;
        const double PairError = SectionError(Pair, 12.0, 20.0, { CentreX, 1.0, 0.5 }, PairIuu, PairIvv);
        Tally(MeasureGate, PairError, Pair.Refusal.empty() && PairError <= 1.0e-9 && Pair.Bodies == 2 && Pair.Regions == 2,
              "two bodies, z = ½ → A " + std::to_string(Pair.Area) + " · x̄ " + std::to_string(Pair.Centroid.X) + " · Ivv " + std::to_string(Pair.SecondMoments[1]) + " (parallel axis " + std::to_string(PairIvv) + ")");

        // Mode-4 body pick → that body's mass (V 4); hiding it removes it from the section
        const std::string SecondKey = Gauge.Regeneration.Bodies.back().Key;
        ClearSolidArcSelection(Gauge);
        SelectSolidArcName(Gauge, SolidArcSelectionKind::Body, SecondKey, false);
        const SolidArcMeasurement Picked = MeasureSolidArcSelection(Gauge);
        const double PickedError = std::max(Relative(Picked.Volume, 4.0), Near(Picked.Centroid, 7.0, 1.0, 0.5));
        Tally(MeasureGate, PickedError, Picked.Refusal.empty() && PickedError <= 1.0e-9, "body " + SecondKey + " picked → " + Picked.Subject + " · V " + std::to_string(Picked.Volume));
        Expect(ChangeSolidArcVisibility(Gauge, SolidArcVisibilityVerb::HideSelected));
        const SolidArcSection Alone = SectionSolidArcDocument(Gauge, { SolidArcSectionAxis::Z, 0.5, false, {} });
        Tally(MeasureGate, Relative(Alone.Area, 8.0), Alone.Refusal.empty() && Relative(Alone.Area, 8.0) <= 1.0e-9 && Alone.Bodies == 1,
              "second body hidden → section A " + std::to_string(Alone.Area) + " over " + std::to_string(Alone.Bodies) + " body");
    }

    // ---- W34 section clip: CPU pick and box selection of the Alt+S view (closed form on a 4 × 2 × 1 block, plane x = 1,
    //      normal −x̂ so the kept side is x ≤ 1)
    {
        SolidArcDocument Cut = CreateSolidArcDocument("Cut");
        SolidArcConstructRequest Brick;
        Brick.Kind = SolidArcConstructKind::Block; Brick.Width = 4.0; Brick.Depth = 2.0; Brick.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Cut, Brick));
        const SolidArcScene       Scene = BuildSolidArcScene(Cut);
        const SolidArcSectionClip Clip  = ComposeSolidArcSectionClip(Scene, Cut, { -1.0, 0.0, 0.0, 1.0 });
        auto NameOf = [&](const std::optional<SolidArcPick>& Hit)
        {
            if (!Hit) return std::string("(miss)");
            return Hit->Identity == IdentitySectionCap ? std::string("(cap)") : NameSolidArcIdentity(Scene, Hit->Identity);
        };
        auto EdgeCondition = [](const std::optional<SolidArcPick>& Hit) { return Hit && Hit->Identity != IdentitySectionCap && (Hit->Identity & IdentityEdgeBit) != 0; };

        Tally(SectionGate, 0.0, Clip.Plane == std::array<double, 4>{ -1.0, 0.0, 0.0, 1.0 } && Clip.SheetFaces.size() == Scene.Faces.size() &&
                               std::count(Clip.SheetFaces.begin(), Clip.SheetFaces.end(), uint8_t(1)) == 0,
              "clip of a solid block: plane copied, " + std::to_string(Clip.SheetFaces.size()) + " faces, none marked sheet");

        // Straight down at x = 3: top and bottom both lie on the removed side → nothing
        const OrbitRay Removed = RayBetween({ 3.0, 1.0, 5.0 }, { 3.0, 1.0, 0.0 });
        const std::optional<SolidArcPick> RemovedOpen = PickSolidArcScene(Scene, Removed, 0.0);
        const std::optional<SolidArcPick> RemovedCut  = PickSolidArcScene(Scene, Removed, 0.0, false, &Clip);
        Tally(SectionGate, 0.0, RemovedOpen && !RemovedCut, "down at x = 3: unsectioned → " + NameOf(RemovedOpen) + " · sectioned → " + NameOf(RemovedCut));

        // Along −x at (y, z) = (1, ½): x = 4 is removed, the back of x = 0 is the first kept surface → the cap at distance 5
        const OrbitRay Opening = RayBetween({ 5.0, 1.0, 0.5 }, { 0.0, 1.0, 0.5 });
        const std::optional<SolidArcPick> Capped = PickSolidArcScene(Scene, Opening, 0.0, false, &Clip);
        const double CapError = Capped ? std::abs(Capped->Distance - 5.0) : 1.0;
        Tally(SectionGate, CapError, Capped && Capped->Identity == IdentitySectionCap && CapError <= 1.0e-9 && NameSolidArcIdentity(Scene, IdentitySectionCap).empty(),
              "along −x through the opening → " + NameOf(Capped) + " at " + std::to_string(Capped ? Capped->Distance : 0.0) + " (back face x = 0: 5; plane crossing: 4) · cap name \"\"");

        // Oblique from (3, 1, 3) towards (½, 1, 1): it crosses the plane above the block (z = 1.4), then meets the kept top
        const OrbitRay Oblique = RayBetween({ 3.0, 1.0, 3.0 }, { 0.5, 1.0, 1.0 });
        const std::optional<SolidArcPick> Over     = PickSolidArcScene(Scene, Oblique, 0.0, false, &Clip);
        const std::optional<SolidArcPick> OverOpen = PickSolidArcScene(Scene, Oblique, 0.0);
        const double OverError = Over ? std::abs(Over->Distance - std::sqrt(2.5 * 2.5 + 2.0 * 2.0)) : 1.0;
        Tally(SectionGate, OverError, Over && OverOpen && Over->Identity == OverOpen->Identity && OverError <= 1.0e-9,
              "oblique ray crossing the plane outside the material → " + NameOf(Over) + " (the kept top, not the cap) at " + std::to_string(Over ? Over->Distance : 0.0));

        // Edges: the x = 4 top edge is removed (the ray continues through removed faces → miss); the x = 0 top edge picks
        const OrbitRay FarEdge  = RayBetween({ 5.0, 1.0, 2.0 }, { 4.0, 1.0, 1.0 });
        const OrbitRay NearEdge = RayBetween({ -1.0, 1.0, 2.0 }, { 0.0, 1.0, 1.0 });
        const std::optional<SolidArcPick> FarOpen  = PickSolidArcScene(Scene, FarEdge, 0.02);
        const std::optional<SolidArcPick> FarCut   = PickSolidArcScene(Scene, FarEdge, 0.02, false, &Clip);
        const std::optional<SolidArcPick> NearOpen = PickSolidArcScene(Scene, NearEdge, 0.02);
        const std::optional<SolidArcPick> NearCut  = PickSolidArcScene(Scene, NearEdge, 0.02, false, &Clip);
        Tally(SectionGate, 0.0, EdgeCondition(FarOpen) && !FarCut, "x = 4 top edge: unsectioned → " + NameOf(FarOpen) + " · sectioned → " + NameOf(FarCut));
        Tally(SectionGate, 0.0, EdgeCondition(NearCut) && NearOpen && NearCut->Identity == NearOpen->Identity, "x = 0 top edge: sectioned = unsectioned → " + NameOf(NearCut));

        // Sheet faces have no inside: a clip marking every face as sheet shows the back of x = 0 itself
        SolidArcSectionClip Sheets = Clip;
        Sheets.SheetFaces.assign(Scene.Faces.size(), 1);
        const std::optional<SolidArcPick> Backside = PickSolidArcScene(Scene, Opening, 0.0, false, &Sheets);
        Tally(SectionGate, 0.0, Backside && Backside->Identity != IdentitySectionCap && (Backside->Identity & IdentityEdgeBit) == 0 && std::abs(Backside->Distance - 5.0) <= 1.0e-9,
              "same ray with sheet faces → " + NameOf(Backside) + " (its back, no cap)");
        for (SolidArcBodyFigures& Figures : Cut.Summary.Bodies) Figures.SheetCondition = true;  // Summary only; restored below
        const SolidArcSectionClip Marked = ComposeSolidArcSectionClip(Scene, Cut, Clip.Plane);
        for (SolidArcBodyFigures& Figures : Cut.Summary.Bodies) Figures.SheetCondition = false;
        Tally(SectionGate, 0.0, std::count(Marked.SheetFaces.begin(), Marked.SheetFaces.end(), uint8_t(1)) == long(Scene.Faces.size()),
              "the body summarised as a sheet → " + std::to_string(std::count(Marked.SheetFaces.begin(), Marked.SheetFaces.end(), uint8_t(1))) + " / " + std::to_string(Scene.Faces.size()) + " faces marked");

        // Box over the whole viewport in x-ray: the 4 edges at x = 4 are removed, the 4 at x = 0 and the 4 along x remain
        OrbitCamera Camera;
        Camera.Target = { 2.0, 1.0, 0.5 }; Camera.Distance = 10.0; Camera.SceneRadius = 2.5;
        const OrbitCameraFrame   Frame  = FrameOrbitCamera(Camera, 800.0, 600.0);
        const std::vector<uint32_t> Boxed  = EncloseSolidArcEdges(Scene, Frame, { 0.0, 0.0 }, { 800.0, 600.0 }, true, &Clip);
        bool KeptCondition = true;
        for (const uint32_t Index : Boxed)
        {
            const Vector3 Start = EdgeEndPosition(Cut, Scene.Edges[Index].Name, false);
            const Vector3 End   = EdgeEndPosition(Cut, Scene.Edges[Index].Name, true);
            KeptCondition = KeptCondition && std::min(Start.X, End.X) <= 1.0;
        }
        Tally(SectionGate, 0.0, Boxed.size() == 8 && KeptCondition, "full box, x-ray, sectioned → " + std::to_string(Boxed.size()) + " edges, all reaching x ≤ 1 (closed form 8 of 12)");
    }

    // ---- W35 object snapping: exact snap points under the pointer (closed forms on a block, a cylinder, sketches)
    {
        SolidArcDocument Snaps = CreateSolidArcDocument("Snaps");
        SolidArcConstructRequest Brick;
        Brick.Kind = SolidArcConstructKind::Block; Brick.Width = 4.0; Brick.Depth = 2.0; Brick.Height = 1.0;
        Expect(ConstructSolidArcPrimitive(Snaps, Brick));
        SolidArcConstructRequest Post;
        Post.Kind = SolidArcConstructKind::Cylinder; Post.Position = { 6.0, 1.0, 0.0 }; Post.Radius = 0.5; Post.Height = 1.0;
        Post.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Snaps, Post));
        Expect(DrawSolidArcSketch(Snaps, DrawingOf(SolidArcDrawingKind::Line, 0, { { -3.0, -2.0 }, { -1.0, -4.0 } })));
        Expect(DrawSolidArcSketch(Snaps, DrawingOf(SolidArcDrawingKind::Line, 0, { { -3.0, -4.0 }, { -1.0, -2.0 } })));
        Expect(DrawSolidArcSketch(Snaps, DrawingOf(SolidArcDrawingKind::Circle, 0, { { -5.0, -3.0 }, { -4.2, -3.0 } }, false, true)));
        Expect(DrawSolidArcSketch(Snaps, DrawingOf(SolidArcDrawingKind::Curve, 0, { { -8.0, -1.0 }, { -7.0, 0.5 }, { -6.0, -0.5 }, { -5.0, 1.0 } })));
        const SolidArcScene   Scene     = BuildSolidArcScene(Snaps);
        SolidArcSnapCatalogue Catalogue = CatalogueSolidArcSnaps(Snaps, Scene);
        OrbitCamera Camera;
        Camera.Target = { 0.0, -1.0, 0.5 }; Camera.Distance = 22.0; Camera.SceneRadius = 9.0;
        const OrbitCameraFrame Frame = FrameOrbitCamera(Camera, 1200.0, 900.0);
        auto Query = [&](Vector3 At, double NudgeX = 0.0, double NudgeY = 0.0)
        {
            SolidArcSnapQuery Request;
            Request.Frame = Frame;
            (void)ProjectSolidArcPoint(Frame, At, Request.PixelX, Request.PixelY);
            Request.PixelX += NudgeX;
            Request.PixelY += NudgeY;
            return Request;
        };
        auto Describe = [](const SolidArcSnap& Snap)
        {
            char Text[256];
            std::snprintf(Text, sizeof(Text), "%s (%.12g, %.12g, %.12g) · %s · %.1f px", Snap.Kind == SolidArcSnapKind::None ? "none" : SolidArcSnapLabel(Snap.Kind),
                          Snap.Position.X, Snap.Position.Y, Snap.Position.Z, Snap.Source.c_str(), Snap.Pixels);
            return std::string(Text);
        };
        auto Point = [&](const char* Note, Vector3 At, SolidArcSnapKind Kind, Vector3 Expected, double NudgeX = 3.0, double NudgeY = -2.0)
        {
            const SolidArcSnap Snap  = FindSolidArcSnap(Catalogue, Snaps, Scene, Query(At, NudgeX, NudgeY));
            const double       Error = Distance(Snap.Position, Expected);
            Tally(SnapGate, Error, Snap.Kind == Kind && Error <= 1.0e-12, std::string(Note) + " → " + Describe(Snap));
        };
        Point("block corner (4, 0, 1), pointer 3.6 px off", { 4.0, 0.0, 1.0 }, SolidArcSnapKind::Endpoint, { 4.0, 0.0, 1.0 });
        Point("front top edge midpoint (2, 0, 1)", { 2.0, 0.0, 1.0 }, SolidArcSnapKind::Midpoint, { 2.0, 0.0, 1.0 });
        Point("top face centre (2, 1, 1) (planar face centroid)", { 2.0, 1.0, 1.0 }, SolidArcSnapKind::Centre, { 2.0, 1.0, 1.0 });
        Point("cylinder top rim centre (6, 1, 1)", { 6.0, 1.0, 1.0 }, SolidArcSnapKind::Centre, { 6.0, 1.0, 1.0 });
        Point("sketch lines cross at (−2, −3, 0)", { -2.0, -3.0, 0.0 }, SolidArcSnapKind::Intersection, { -2.0, -3.0, 0.0 });
        Point("sketch line end (−1, −4, 0)", { -1.0, -4.0, 0.0 }, SolidArcSnapKind::Endpoint, { -1.0, -4.0, 0.0 });
        Point("sketch circle centre (−5, −3, 0)", { -5.0, -3.0, 0.0 }, SolidArcSnapKind::Centre, { -5.0, -3.0, 0.0 });
        {
            // On the front top edge at x = 1.3 (y = 0, z = 1 exactly), on the top face, on the rim, on the cylinder side
            const SolidArcSnap Edge = FindSolidArcSnap(Catalogue, Snaps, Scene, Query({ 1.3, 0.0, 1.0 }, 1.0, 1.0));
            const double EdgeError = std::abs(Edge.Position.Y) + std::abs(Edge.Position.Z - 1.0);
            Tally(SnapGate, EdgeError, Edge.Kind == SolidArcSnapKind::Curve && EdgeError <= 1.0e-12 && std::abs(Edge.Position.X - 1.3) < 0.05, "on edge near (1.3, 0, 1): |y| + |z − 1| → " + Describe(Edge));
            const SolidArcSnap Top = FindSolidArcSnap(Catalogue, Snaps, Scene, Query({ 2.9, 1.4, 1.0 }));
            const double TopError = std::abs(Top.Position.Z - 1.0);
            Tally(SnapGate, TopError, Top.Kind == SolidArcSnapKind::Face && TopError <= 1.0e-12 && Distance(Top.Normal, Vector3{ 0.0, 0.0, 1.0 }) <= 1.0e-12, "on top face near (2.9, 1.4, 1): |z − 1|, outward normal ẑ → " + Describe(Top));
            const Vector3 RimAt{ 6.0 + 0.5 * std::cos(-0.6), 1.0 + 0.5 * std::sin(-0.6), 1.0 };
            const SolidArcSnap Rim = FindSolidArcSnap(Catalogue, Snaps, Scene, Query(RimAt, 1.0, -1.0));
            const double RimError = std::abs(std::hypot(Rim.Position.X - 6.0, Rim.Position.Y - 1.0) - 0.5) + std::abs(Rim.Position.Z - 1.0);
            Tally(SnapGate, RimError, Rim.Kind == SolidArcSnapKind::Curve && RimError <= 1.0e-12, "on top rim near angle −0.6: ||ρ| − 0.5| + |z − 1| → " + Describe(Rim));
            const Vector3 SideAt{ 6.0 + 0.5 * std::cos(-0.9), 1.0 + 0.5 * std::sin(-0.9), 0.45 };
            const SolidArcSnap Side = FindSolidArcSnap(Catalogue, Snaps, Scene, Query(SideAt, 0.0, 0.0));
            const double SideError = std::abs(std::hypot(Side.Position.X - 6.0, Side.Position.Y - 1.0) - 0.5);
            Tally(SnapGate, SideError, Side.Kind == SolidArcSnapKind::Face && SideError <= 1.0e-12, "on cylinder side near angle −0.9, z 0.45: ||ρ| − 0.5| (exact ray hit, not the facets) → " + Describe(Side));
        }
        {
            // Tangent from P = (−5, −0.5, 0) to the circle r 0.8 at (−5, −3): cos α = r / d = 0.32
            const Vector3 Reference{ -5.0, -0.5, 0.0 };
            const double  Sine = std::sqrt(1.0 - 0.32 * 0.32);
            const Vector3 Expected{ -5.0 + 0.8 * Sine, -3.0 + 0.8 * 0.32, 0.0 };
            SolidArcSnapQuery Request = Query(Expected, 2.0, 1.0);
            Request.Reference = Reference;
            const SolidArcSnap Snap = FindSolidArcSnap(Catalogue, Snaps, Scene, Request);
            const Vector3 Centre{ -5.0, -3.0, 0.0 };
            const double Error = std::abs(Distance(Snap.Position, Centre) - 0.8) + std::abs(Dot(Snap.Position - Reference, Snap.Position - Centre)) + std::abs(Snap.Position.Z);
            Tally(SnapGate, Error, Snap.Kind == SolidArcSnapKind::Tangent && Error <= 1.0e-12 && Distance(Snap.Position, Expected) <= 1.0e-12,
                  "tangent from (−5, −0.5, 0) to circle r 0.8: ||T − C| − r| + |(T − P)·(T − C)| → " + Describe(Snap));
            // Perpendicular from P = (−4.5, −4.5, 0) to the line (−3, −2) → (−1, −4): foot (−2.5, −2.5, 0)
            Request = Query({ -2.5, -2.5, 0.0 }, 2.0, 2.0);
            Request.Reference = Vector3{ -4.5, -4.5, 0.0 };
            const SolidArcSnap Foot = FindSolidArcSnap(Catalogue, Snaps, Scene, Request);
            const double FootError = Distance(Foot.Position, Vector3{ -2.5, -2.5, 0.0 });
            Tally(SnapGate, FootError, Foot.Kind == SolidArcSnapKind::Perpendicular && FootError <= 1.0e-12, "perpendicular from (−4.5, −4.5, 0) to Line (−3, −2) → (−1, −4): foot (−2.5, −2.5, 0) → " + Describe(Foot));
        }
        {
            // Arc-length midpoint of the Hermite curve: both halves equal (independent 200 000-chord sums)
            const SolidArcSnapCurve* Spline = nullptr;
            for (const SolidArcSnapCurve& Curve : Catalogue.Curves) if (Curve.Segment && Curve.Segment->Kind == ProfileSegmentKind::Spline) Spline = &Curve;
            const SolidArcSnapPoint* Middle = nullptr;
            for (const SolidArcSnapPoint& Each : Catalogue.Points) if (Spline && Each.Kind == SolidArcSnapKind::Midpoint && Each.Source == Spline->Source) Middle = &Each;
            double Before = 0.0, After = 0.0;
            if (Spline && Middle)
            {
                const Vector2 Local{ Dot(Middle->Position - Spline->Plane.Origin, Spline->Plane.XAxis), Dot(Middle->Position - Spline->Plane.Origin, Spline->Plane.YAxis) };
                const double Split = LocateProfileFraction(*Spline->Segment, Local);
                constexpr int Chords = 200000;
                for (int Index = 0; Index < Chords; ++Index)
                {
                    const double From = double(Index) / Chords, To = double(Index + 1) / Chords;
                    const auto Chord = [&](double Low, double High) { return Distance(EvaluateProfileSegment(*Spline->Segment, Low), EvaluateProfileSegment(*Spline->Segment, High)); };
                    if (To <= Split) Before += Chord(From, To);
                    else if (From >= Split) After += Chord(From, To);
                    else { Before += Chord(From, Split); After += Chord(Split, To); }
                }
            }
            const double Imbalance = std::abs(Before - After) / std::max(1.0e-12, Before + After);
            Tally(SnapGate, Imbalance, Middle && Imbalance <= 1.0e-9, "Hermite curve midpoint: |L₁ − L₂| / L (L₁ " + std::to_string(Before) + ", L₂ " + std::to_string(After) + ")");
        }
        {
            // Hidden back corner (0, 2, 0): no endpoint there unless x-ray
            SolidArcSnapQuery Hidden = Query({ 0.0, 2.0, 0.0 }, 1.0, 1.0);
            const SolidArcSnap Plain = FindSolidArcSnap(Catalogue, Snaps, Scene, Hidden);
            Hidden.XRayEnabled = true;
            const SolidArcSnap Through = FindSolidArcSnap(Catalogue, Snaps, Scene, Hidden);
            const bool PlainCondition = !(Plain.Kind == SolidArcSnapKind::Endpoint && Distance(Plain.Position, Vector3{ 0.0, 2.0, 0.0 }) <= 1.0e-12);
            Tally(SnapGate, Distance(Through.Position, Vector3{ 0.0, 2.0, 0.0 }), PlainCondition && Through.Kind == SolidArcSnapKind::Endpoint && Distance(Through.Position, Vector3{ 0.0, 2.0, 0.0 }) <= 1.0e-12,
                  "hidden corner (0, 2, 0): plain → " + Describe(Plain) + " · x-ray → " + Describe(Through));
            // Section x ≤ 3 (n = −x̂, w = 3): the corner (4, 0, 1) is cut away
            const SolidArcSectionClip Clip = ComposeSolidArcSectionClip(Scene, Snaps, { -1.0, 0.0, 0.0, 3.0 });
            SolidArcSnapQuery Cut = Query({ 4.0, 0.0, 1.0 }, 1.0, 1.0);
            Cut.Section = &Clip;
            const SolidArcSnap Sectioned = FindSolidArcSnap(Catalogue, Snaps, Scene, Cut);
            Tally(SnapGate, 0.0, !(Sectioned.Kind == SolidArcSnapKind::Endpoint && Distance(Sectioned.Position, Vector3{ 4.0, 0.0, 1.0 }) <= 1.0e-9), "corner (4, 0, 1) behind the section x ≤ 3 → " + Describe(Sectioned));
            // Move excludes the moving body: the block's corner does not snap
            SolidArcSnapCatalogue Others = CatalogueSolidArcSnaps(Snaps, Scene, { Snaps.Regeneration.Bodies.front().Key });
            const SolidArcSnap Excluded = FindSolidArcSnap(Others, Snaps, Scene, Query({ 4.0, 0.0, 1.0 }));
            Tally(SnapGate, 0.0, !(Excluded.Kind == SolidArcSnapKind::Endpoint && Distance(Excluded.Position, Vector3{ 4.0, 0.0, 1.0 }) <= 1.0e-9), "block excluded (Move) at its corner → " + Describe(Excluded));
            // A dragged sketch excludes itself by its step label: the circle's centre is gone, the block's corner stays
            std::string CircleLabel;
            for (const FeatureStep& Step : Snaps.Tree.Steps)
            {
                const auto Caption = Snaps.Captions.find(Step.Label);
                if (Step.Kind == FeatureStepKind::Sketch && Caption != Snaps.Captions.end() && Caption->second == "Circle1") CircleLabel = Step.Label;
            }
            SolidArcSnapCatalogue WithoutCircle = CatalogueSolidArcSnaps(Snaps, Scene, { CircleLabel });
            const SolidArcSnap Centre = FindSolidArcSnap(WithoutCircle, Snaps, Scene, Query({ -5.0, -3.0, 0.0 }, 1.0, 1.0));
            const SolidArcSnap Corner = FindSolidArcSnap(WithoutCircle, Snaps, Scene, Query({ 4.0, 0.0, 1.0 }, 1.0, 1.0));
            Tally(SnapGate, 0.0, !CircleLabel.empty() && Centre.Kind != SolidArcSnapKind::Centre && Corner.Kind == SolidArcSnapKind::Endpoint && WithoutCircle.Curves.size() + 1 == Catalogue.Curves.size(),
                  "sketch " + CircleLabel + " excluded (its point being dragged): centre → " + Describe(Centre) + " · corner → " + Describe(Corner));
            // Filter: endpoints off → the corner pointer lands on an edge instead (still exact)
            SolidArcSnapQuery Filtered = Query({ 4.0, 0.0, 1.0 }, 1.0, 1.0);
            Filtered.Filter.EndpointEnabled = false;
            const SolidArcSnap Edge = FindSolidArcSnap(Catalogue, Snaps, Scene, Filtered);
            Tally(SnapGate, 0.0, Edge.Kind != SolidArcSnapKind::Endpoint && Edge.Kind != SolidArcSnapKind::None, "endpoints disabled at the corner → " + Describe(Edge));
            // Transient point (the drawing chain)
            SolidArcSnapQuery Chain = Query({ -6.0, -6.0, 0.0 }, 2.0, 2.0);
            Chain.Transient.push_back({ SolidArcSnapKind::Endpoint, { -6.0, -6.0, 0.0 }, {}, {}, "Drawing point 1", {} });
            const SolidArcSnap Placed = FindSolidArcSnap(Catalogue, Snaps, Scene, Chain);
            Tally(SnapGate, Distance(Placed.Position, Vector3{ -6.0, -6.0, 0.0 }), Placed.Kind == SolidArcSnapKind::Endpoint && Distance(Placed.Position, Vector3{ -6.0, -6.0, 0.0 }) == 0.0, "placed drawing point (−6, −6, 0) → " + Describe(Placed));
            // Direction lock: the pointer over (5.5, 0, 1) meets the x line through (4, 0, 1) at t = 1.5
            const SolidArcSnapQuery Lock = Query({ 5.5, 0.0, 1.0 });
            const std::optional<double> Along = ApproachSolidArcLine(CastOrbitRay(Frame, Lock.PixelX, Lock.PixelY), { 4.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 });
            Tally(SnapGate, Along ? std::abs(*Along - 1.5) : 1.0, Along && std::abs(*Along - 1.5) <= 1.0e-9, "x lock through (4, 0, 1), pointer over (5.5, 0, 1) → t " + std::to_string(Along ? *Along : -1.0));
            // Catalogue census: 12 + 2 block / cylinder edges' vertices … reported for the record
            Tally(SnapGate, 0.0, Catalogue.Curves.size() >= 12 + 3 + 4 && !Catalogue.Points.empty(),
                  "catalogue: " + std::to_string(Catalogue.Curves.size()) + " curves, " + std::to_string(Catalogue.Points.size()) + " fixed points");
        }
    }

    // ---- report
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    std::printf("# SolidArcDocumentValidation\n\n");
    std::printf("Document layer of SolidArc over the GeometryProcessing feature tree (%.2f s).\n\n", Seconds);
    std::printf("| Gate | Subject | Limit | Worst | Checks | Verdict |\n|------|---------|-------|-------|--------|---------|\n");
    long Failures = 0;
    // ---- W36 joints (Alt+J) + interference: a pin into the bracket hole, a lever on the pin, every number closed form
    {
        SolidArcDocument Rig = BuildSolidArcSpecimen();
        SolidArcConstructRequest Pin;
        Pin.Kind = SolidArcConstructKind::Cylinder; Pin.Position = { 6.0, 1.25, 0.0 }; Pin.Radius = 0.25; Pin.Height = 1.5;
        Pin.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Rig, Pin));
        SolidArcConstructRequest Lever;
        Lever.Kind = SolidArcConstructKind::Block; Lever.Position = { 6.0, 4.0, 0.0 }; Lever.Width = 2.0; Lever.Depth = 0.4; Lever.Height = 0.2;
        Lever.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Rig, Lever));
        Tally(JointGate, 0.0, Rig.Regeneration.Bodies.size() == 3, "bracket + pin + lever: " + std::to_string(Rig.Regeneration.Bodies.size()) + " bodies");
        const std::string BracketKey = Rig.Regeneration.Bodies[0].Key, PinKey = Rig.Regeneration.Bodies[1].Key, LeverKey = Rig.Regeneration.Bodies[2].Key;
        // The entity of a body whose joint origin sits at a point (and reads Description) — what the hover offers there.
        auto OriginAt = [&](const SolidArcDocument& Document, const std::string& Key, SolidArcSelectionKind Kind, Vector3 At, const char* Description)
        {
            for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
            {
                if (Body.Key != Key) continue;
                for (const auto& [Slot, Name] : Kind == SolidArcSelectionKind::Edge ? Body.Naming.Edges : Body.Naming.Faces)
                {
                    const SolidArcJointOrigin Origin = ResolveSolidArcJointOrigin(Document, Kind, Name);
                    if (Origin.Refusal.empty() && Origin.Description == Description && Distance(Origin.Frame.Origin, At) <= 1.0e-9) return Name;
                }
            }
            return std::string();
        };
        auto Centroid = [&](const SolidArcDocument& Document, const std::string& Key)
        {
            for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
                if (Body.Key == Key) return MeasureBodyMass(*Document.Partition, Body.Body).Centroid;
            return Vector3{ std::nan(""), 0.0, 0.0 };
        };
        auto Bounds = [&](const SolidArcDocument& Document, const std::string& Key, Vector3& Low, Vector3& High)
        {
            Low = { 1.0e9, 1.0e9, 1.0e9 }; High = { -1.0e9, -1.0e9, -1.0e9 };
            for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
            {
                if (Body.Key != Key) continue;
                for (const VertexToken Vertex : Document.Partition->CollectInventory(Body.Body).Vertices)
                {
                    const Vector3 Point = Document.Partition->Vertices.Resolve(Vertex).Position;
                    Low  = { std::min(Low.X, Point.X), std::min(Low.Y, Point.Y), std::min(Low.Z, Point.Z) };
                    High = { std::max(High.X, Point.X), std::max(High.Y, Point.Y), std::max(High.Z, Point.Z) };
                }
            }
        };
        auto Gap = [](const SolidArcDocument& Document)
        {
            double Worst = 0.0;
            for (const RegeneratedJoint& Joint : Document.Regeneration.Joints) Worst = std::max(Worst, Distance(Joint.MovingOrigin.Origin, Joint.FixedOrigin.Origin));
            return Worst;
        };
        auto Triple = [](Vector3 Point) { char Text[96]; std::snprintf(Text, sizeof(Text), "(%.12g, %.12g, %.12g)", Point.X, Point.Y, Point.Z); return std::string(Text); };
        // ① the hover origins: circles a quarter turn off their seams are probed by centre (the rim's own centre)
        const std::string PinTop   = OriginAt(Rig, PinKey, SolidArcSelectionKind::Edge, { 6.0, 1.25, 1.5 }, "circle centre");
        const std::string HoleTop  = OriginAt(Rig, BracketKey, SolidArcSelectionKind::Edge, { 3.0, 1.25, 1.0 }, "circle centre");
        const std::string LeverTop = OriginAt(Rig, LeverKey, SolidArcSelectionKind::Face, { 7.0, 4.2, 0.2 }, "plane centroid");
        const std::string PinCap   = OriginAt(Rig, PinKey, SolidArcSelectionKind::Face, { 6.0, 1.25, 1.5 }, "plane centroid");
        Tally(JointGate, 0.0, !PinTop.empty() && !HoleTop.empty() && !LeverTop.empty() && !PinCap.empty(),
              "origins: pin top rim → circle centre (6, 1.25, 1.5); hole top rim → (3, 1.25, 1); lever top → plane centroid (7, 4.2, 0.2); pin cap → (6, 1.25, 1.5)");
        {
            const SolidArcJointOrigin Rim = ResolveSolidArcJointOrigin(Rig, SolidArcSelectionKind::Edge, HoleTop);
            Tally(JointGate, Distance(Rim.Frame.ZAxis, Vector3{ 0.0, 0.0, 1.0 }), Rim.Body == BracketKey && Distance(Rim.Frame.ZAxis, Vector3{ 0.0, 0.0, 1.0 }) <= 1.0e-12,
                  "hole rim z axis leaves the bracket top (+z): " + Triple(Rim.Frame.ZAxis) + " on " + Rim.Body);
        }
        // ② revolute, flipped (origins aligned) + offset 0.25: the pin drops into the hole, 0.25 proud — centroid (3, 1.25, 0.5)
        SolidArcJointRequest Insert;
        Insert.MovingKind = SolidArcSelectionKind::Edge; Insert.MovingName = PinTop;
        Insert.FixedKind  = SolidArcSelectionKind::Edge; Insert.FixedName  = HoleTop;
        Insert.Kind = JointKind::Revolute; Insert.Flipped = true; Insert.Offset = 0.25;
        const size_t Entries = Rig.UndoEntries.size();
        const SolidArcCommandOutcome Inserted = JointSolidArcBodies(Rig, Insert);
        Expect(Inserted);
        const Vector3 PinCentre = Centroid(Rig, PinKey);
        Tally(JointGate, Distance(PinCentre, Vector3{ 3.0, 1.25, 0.5 }), Inserted.AcceptedCondition && Distance(PinCentre, Vector3{ 3.0, 1.25, 0.5 }) <= 1.0e-9,
              "pin into the hole (revolute, flipped, offset 0.25): centroid " + Triple(PinCentre) + " = (3, 1.25, 0.5)");
        Tally(JointGate, std::abs(Gap(Rig) - 0.25), Rig.Regeneration.Joints.size() == 1 && std::abs(Gap(Rig) - 0.25) <= 1.0e-9 && Rig.UndoEntries.size() == Entries + 1,
              "1 regenerated joint, origins 0.25 apart (the offset), 1 undo entry; step " + Rig.Tree.Steps.back().Label + " captioned " + Rig.Captions[Rig.Tree.Steps.back().Label]);
        {
            std::string Names;
            for (const FeatureParameter& Parameter : Rig.Tree.Parameters) if (Parameter.Name.starts_with(Rig.Captions[Rig.Tree.Steps.back().Label] + ".")) Names += Parameter.Name + " ";
            Tally(JointGate, 0.0, Names.find(".Angle") != std::string::npos && Names.find(".Offset") != std::string::npos && Names.find(".Slide") == std::string::npos,
                  "revolute parameters: " + Names);
        }
        // ③ the lever top onto the pin cap (facing: the lever turns over onto it), revolute 90°: centred on the pin axis, long side along y
        SolidArcJointRequest Swing;
        Swing.MovingKind = SolidArcSelectionKind::Face; Swing.MovingName = LeverTop;
        Swing.FixedKind  = SolidArcSelectionKind::Face; Swing.FixedName  = OriginAt(Rig, PinKey, SolidArcSelectionKind::Face, { 3.0, 1.25, 1.25 }, "plane centroid");
        Swing.Kind = JointKind::Revolute; Swing.Angle = 90.0;
        Expect(JointSolidArcBodies(Rig, Swing));
        const std::string SwingLabel = Rig.Tree.Steps.back().Label;
        Vector3 Low, High;
        Bounds(Rig, LeverKey, Low, High);
        const double BoundsError = Distance(Low, Vector3{ 2.8, 0.25, 1.25 }) + Distance(High, Vector3{ 3.2, 2.25, 1.45 });
        Tally(JointGate, BoundsError, BoundsError <= 1.0e-9, "lever on the pin cap at 90°: bounds " + Triple(Low) + " … " + Triple(High) + " = (2.8, 0.25, 1.25) … (3.2, 2.25, 1.45)");
        // ④ measure: lever + bracket clear by 0.25 (lever bed over the bracket top), bracket + pin clear by 0.05 (hole r 0.3, pin r 0.25)
        auto Measure = [&](const SolidArcDocument& Document, const std::string& First, const std::string& Second)
        {
            SolidArcDocument Copy = CloneSolidArcDocument(Document);
            Copy.Selection = {};
            Copy.Selection.Bodies = { First, Second };
            Copy.Selection.LastKind = SolidArcSelectionKind::Body;
            return MeasureSolidArcSelection(Copy);
        };
        {
            const SolidArcMeasurement Above = Measure(Rig, LeverKey, BracketKey);
            Tally(JointGate, std::abs(Above.Distance - 0.25), Above.Interference.starts_with("Clear") && std::abs(Above.Distance - 0.25) <= 1.0e-9,
                  "lever + bracket: " + Above.Interference + ", clearance " + std::to_string(Above.Distance));
            const SolidArcMeasurement Inside = Measure(Rig, BracketKey, PinKey);
            Tally(JointGate, std::abs(Inside.Distance - 0.05), Inside.Interference.starts_with("Clear") && std::abs(Inside.Distance - 0.05) <= 1.0e-9,
                  "bracket + pin: " + Inside.Interference + ", clearance " + std::to_string(Inside.Distance) + " (0.3 − 0.25)");
        }
        // ⑤ the pin offset −0.1 (one undo entry): replay resumes at the pin joint, the lever rides down — it now overlaps the
        //    bracket top slab 0.1 deep minus the strip |x − 3| ≤ 0.2 of the hole disc (⌀ 0.6 > lever width 0.4)
        const std::string InsertCaption = Rig.Captions[Rig.Regeneration.Joints.front().Label];             // Parameters go by caption: Revolute1.Offset
        Expect(AssignSolidArcParameter(Rig, InsertCaption + ".Offset", -0.1));
        {
            const double              Expected = (0.8 - 2.0 * (0.2 * std::sqrt(0.05) + 0.09 * std::asin(2.0 / 3.0))) * 0.1;
            const SolidArcMeasurement Clash    = Measure(Rig, LeverKey, BracketKey);
            Tally(JointGate, std::abs(Clash.OverlapVolume - Expected), Clash.Interference.starts_with("Overlap") && std::abs(Clash.OverlapVolume - Expected) <= 1.0e-9 && !Clash.Overlap.empty(),
                  "pin offset −0.1 → lever + bracket: " + Clash.Interference + ", overlap " + std::to_string(Clash.OverlapVolume) + " = (0.8 − 2 (0.2 √0.05 + 0.09 asin ⅔)) · 0.1, " + std::to_string(Clash.Overlap.size()) + " overlap edges drawn");
            Bounds(Rig, LeverKey, Low, High);
            Tally(JointGate, std::abs(Low.Z - 0.9), std::abs(Low.Z - 0.9) <= 1.0e-9, "the lever followed the pin: bed at z " + std::to_string(Low.Z) + " = 0.9");
        }
        // ⑥ moving the bracket +1 x carries the pin and the lever (bodies jointed to it follow)
        Expect(TransformSolidArcBodies(Rig, { BracketKey }, TranslationMap({ 1.0, 0.0, 0.0 }), "Move"));
        {
            const Vector3 Carried = Centroid(Rig, PinKey);
            Bounds(Rig, LeverKey, Low, High);
            const double Error = Distance(Carried, Vector3{ 4.0, 1.25, 0.15 }) + std::abs(Low.X - 3.8);
            Tally(JointGate, Error, Error <= 1.0e-9 && std::abs(Gap(Rig) - 0.1) <= 1.0e-9,
                  "bracket moved +1 x: pin centroid " + Triple(Carried) + " = (4, 1.25, 0.15), lever x from " + std::to_string(Low.X) + " = 3.8, joints still assembled");
        }
        // ⑦ refusals leave the document untouched: both origins on one body, a pin jointed twice
        {
            const size_t Before = Rig.UndoEntries.size();
            SolidArcJointRequest Self = Insert;
            Self.FixedKind = SolidArcSelectionKind::Face; Self.FixedName = OriginAt(Rig, PinKey, SolidArcSelectionKind::Face, { 4.0, 1.25, 0.9 }, "plane centroid");
            const SolidArcCommandOutcome Lonely = JointSolidArcBodies(Rig, Self);
            SolidArcJointRequest Twice = Insert;
            Twice.MovingName = OriginAt(Rig, PinKey, SolidArcSelectionKind::Edge, { 4.0, 1.25, 0.9 }, "circle centre");
            Twice.FixedName  = OriginAt(Rig, BracketKey, SolidArcSelectionKind::Edge, { 4.0, 1.25, 1.0 }, "circle centre");
            const SolidArcCommandOutcome Again = JointSolidArcBodies(Rig, Twice);
            Tally(JointGate, 0.0, !Lonely.AcceptedCondition && !Again.AcceptedCondition && Rig.UndoEntries.size() == Before,
                  "refused: \"" + Lonely.Refusal + "\" · \"" + Again.Refusal + "\"; no undo entry");
        }
        // ⑧ undo takes back the move, then the offset; the file keeps both joints byte-equal
        {
            const std::string Written = EncodeSolidArcDocument(Rig);
            const SolidArcDocumentFile Read = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            // Reference signatures are re-captured on load (the lever's face probe moved with the pin edit), so the first
            // rewrite refreshes them; a second round trip is the fixed point. Geometry and joint values must match.
            const std::string          Rewritten = EncodeSolidArcDocument(Copy);
            const SolidArcDocumentFile Again     = DecodeSolidArcDocument(Rewritten);
            SolidArcDocument Second = CreateSolidArcDocument(Again.Title);
            Second.Tree = Again.Tree; Second.Captions = Again.Captions; Second.Appearance = Again.Appearance;
            RegenerateSolidArcDocument(Second, true);
            const auto SameValue = [](const FeatureValue& Left, const FeatureValue& Right)
            {
                return Left.Literal == Right.Literal && Left.Parameter == Right.Parameter && Left.Factor == Right.Factor && Left.Offset == Right.Offset
                    && Left.SecondParameter == Right.SecondParameter && Left.SecondFactor == Right.SecondFactor;
            };
            bool SameJoints = Copy.Tree.Steps.size() == Rig.Tree.Steps.size();
            for (size_t Index = 0; SameJoints && Index < Rig.Tree.Steps.size(); ++Index)
            {
                const FeatureStep& Left = Rig.Tree.Steps[Index];
                const FeatureStep& Right = Copy.Tree.Steps[Index];
                if (Left.Kind != FeatureStepKind::Joint) continue;
                SameJoints = Right.Kind == FeatureStepKind::Joint && Left.Joint.Kind == Right.Joint.Kind && Left.Joint.FlippedCondition == Right.Joint.FlippedCondition
                          && SameValue(Left.Joint.Angle, Right.Joint.Angle) && SameValue(Left.Joint.Offset, Right.Joint.Offset) && Left.Target == Right.Target && Left.Tool == Right.Tool;
            }
            const double Moved = Distance(Centroid(Copy, PinKey), Centroid(Rig, PinKey)) + Distance(Centroid(Copy, LeverKey), Centroid(Rig, LeverKey));
            Tally(JointGate, Moved, SameJoints && Moved <= 1.0e-12 && EncodeSolidArcDocument(Second) == Rewritten && Copy.Regeneration.Joints.size() == 2 && Written.find("JointFlipped = true") != std::string::npos,
                  "save → load: 2 joints with equal kind / flip / values / bodies, pin and lever centroids within " + std::to_string(Moved) + "; the second round trip is byte-equal (JointFlipped = true for the pin)");
            const bool Undone = UndoSolidArcCommand(Rig) && UndoSolidArcCommand(Rig);
            const Vector3 Back = Centroid(Rig, PinKey);
            Tally(JointGate, Distance(Back, Vector3{ 3.0, 1.25, 0.5 }), Distance(Back, Vector3{ 3.0, 1.25, 0.5 }) <= 1.0e-9 && Undone && Rig.Regeneration.Joints.back().Label == SwingLabel,
                  "2 × undo → pin centroid " + Triple(Back) + " again");
        }
    }

    // ---- W37 drawing sheet (Alt+D): the bracket on paper through the kernel sheet, metres → millimetres, saved settings
    {
        const auto Format = [](const char* Pattern, auto... Values) { char Text[256]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        SolidArcDocument Rig = BuildSolidArcSpecimen();
        SolidArcDrawingSheetSettings Settings;
        const SolidArcDrawingSheet Sheet = ComposeSolidArcDrawingSheet(Rig, Settings);
        std::string Views, Texts;
        for (const Frontier::GeometryProcessing::PlacedDrawingView& View : Sheet.Sheet.Views) Views += (Views.empty() ? "" : " ") + View.Label;
        for (const Frontier::GeometryProcessing::DrawingDimension& Dimension : Sheet.Sheet.Dimensions) Texts += (Texts.empty() ? "" : " ") + Dimension.Text;
        Tally(SheetGate, 0.0, Sheet.Refusal.empty() && Sheet.Bodies == 1 && Views == "Front Top Right Isometric" && Sheet.Sheet.Width == 420.0 && Sheet.Sheet.Height == 297.0
              && Sheet.Sheet.Angle == Frontier::GeometryProcessing::DrawingProjectionAngle::First,
              "default: A3 first angle, " + Views + ", scale " + Sheet.Sheet.ScaleLabel + ", " + std::to_string(Sheet.Sheet.Lines.size()) + " lines, "
              + std::to_string(Sheet.Sheet.Dimensions.size()) + " dimensions, " + Format("%.0f ms", Sheet.Milliseconds));

        // Automatic scale: the largest ISO 5455 scale that fits — the next larger one is refused.
        const std::vector<double> Scales = Frontier::GeometryProcessing::EnumerateStandardScales();
        const auto Chosen = std::find_if(Scales.begin(), Scales.end(), [&](double Scale) { return std::fabs(Scale - Sheet.Sheet.Scale) <= 1.0e-15; });
        bool LargerRefused = Chosen != Scales.end();
        if (Chosen != Scales.begin() && Chosen != Scales.end())
        {
            SolidArcDrawingSheetSettings Larger = Settings;
            Larger.Scale = *(Chosen - 1);
            LargerRefused = !ComposeSolidArcDrawingSheet(Rig, Larger).Refusal.empty();
        }
        Tally(SheetGate, 0.0, LargerRefused, "automatic " + Sheet.Sheet.ScaleLabel + " is standard and the next larger scale is refused");

        // Dimension texts in millimetres; every value × 1000 prints as its text and the linear ones span value × scale on paper.
        double Worst = 0.0;
        bool   Printed = true;
        for (const Frontier::GeometryProcessing::DrawingDimension& Dimension : Sheet.Sheet.Dimensions)
        {
            const std::string Digits = Frontier::GeometryProcessing::FormatDimensionValue(Dimension.Value * SolidArcModelUnitMillimetres);
            const std::string Prefix = Dimension.Kind == Frontier::GeometryProcessing::DrawingDimensionKind::Diameter ? "⌀" : Dimension.Kind == Frontier::GeometryProcessing::DrawingDimensionKind::Radius ? "R" : "";
            Printed = Printed && Dimension.Text == Prefix + Digits;
            if (Dimension.Kind == Frontier::GeometryProcessing::DrawingDimensionKind::Horizontal || Dimension.Kind == Frontier::GeometryProcessing::DrawingDimensionKind::Vertical)
                Worst = std::max(Worst, std::fabs(std::hypot(Dimension.LineEnd.X - Dimension.LineStart.X, Dimension.LineEnd.Y - Dimension.LineStart.Y)
                                                  - Dimension.Value * SolidArcModelUnitMillimetres * Sheet.Sheet.Scale));
        }
        const auto Shows = [&](const char* Text) { return (" " + Texts + " ").find(std::string(" ") + Text + " ") != std::string::npos; };
        Tally(SheetGate, Worst, Printed && Worst <= 1.0e-9 && Shows("4000") && Shows("2500") && Shows("1750") && Shows("⌀600") && Shows("⌀1000"),
              "dimensions in mm: " + Texts + Format(" — span vs value × scale %.1e mm", Worst));

        // Unit invariance: the kernel sheet of the same body in millimetre units at scale × 1000 strokes the same paper.
        std::vector<Frontier::GeometryProcessing::BodyToken> Bodies;
        for (const RegeneratedBody& Body : Rig.Regeneration.Bodies) Bodies.push_back(Body.Body);
        Frontier::GeometryProcessing::DrawingSheetRequest Direct;
        Direct.Paper = Settings.Paper; Direct.Angle = Settings.Angle; Direct.Scale = Sheet.Sheet.Scale * SolidArcModelUnitMillimetres; Direct.ModelUnitMillimetres = 1.0;
        Direct.Title = { Rig.Title, Settings.Title.DrawingNumber, "", "", "", Settings.Title.Revision, Settings.Title.Company };
        const Frontier::GeometryProcessing::DrawingSheet Reference = Frontier::GeometryProcessing::ComposeDrawingSheet(*Rig.Partition, Bodies, Direct);
        // Dimension texts are 1000× longer numbers ("4000" vs "4"), so footprints and the layout differ; each view's
        // strokes relative to its view origin (and the frame / title block) must coincide.
        double Stroke = Reference.Lines.size() == Sheet.Sheet.Lines.size() && Reference.Views.size() == Sheet.Sheet.Views.size() ? 0.0 : 1.0e300;
        double Shifted = 0.0;                                                                // [mm] - Largest view-origin move
        size_t Compared = 0;
        for (size_t View = 0; Stroke < 1.0e300 && View < Reference.Views.size(); ++View)
            Shifted = std::max(Shifted, std::hypot(Sheet.Sheet.Views[View].Origin.X - Reference.Views[View].Origin.X, Sheet.Sheet.Views[View].Origin.Y - Reference.Views[View].Origin.Y));
        for (size_t Line = 0; Stroke < 1.0e300 && Line < Reference.Lines.size(); ++Line)
        {
            const Frontier::GeometryProcessing::SheetPolyline& Ours = Sheet.Sheet.Lines[Line];
            const Frontier::GeometryProcessing::SheetPolyline& Theirs = Reference.Lines[Line];
            if (Theirs.Layer != Ours.Layer || Theirs.Points.size() != Ours.Points.size()) { Stroke = 1.0e300; break; }
            if (Theirs.Layer == Frontier::GeometryProcessing::SheetLayer::Dimension || Theirs.Layer == Frontier::GeometryProcessing::SheetLayer::CuttingPlane || Theirs.Points.empty()) continue;
            Frontier::GeometryProcessing::Vector2 Offset{ 0.0, 0.0 };
            for (size_t View = 0; View < Reference.Views.size(); ++View)
            {
                const Frontier::GeometryProcessing::PlacedDrawingView& Placed = Reference.Views[View];
                const Frontier::GeometryProcessing::Vector2 Point = Theirs.Points.front();
                if (Point.X >= Placed.Low.X - 1.0e-6 && Point.X <= Placed.High.X + 1.0e-6 && Point.Y >= Placed.Low.Y - 1.0e-6 && Point.Y <= Placed.High.Y + 1.0e-6)
                {
                    Offset = { Sheet.Sheet.Views[View].Origin.X - Placed.Origin.X, Sheet.Sheet.Views[View].Origin.Y - Placed.Origin.Y };
                    break;
                }
            }
            for (size_t Point = 0; Point < Theirs.Points.size(); ++Point)
                Stroke = std::max(Stroke, std::hypot(Theirs.Points[Point].X + Offset.X - Ours.Points[Point].X, Theirs.Points[Point].Y + Offset.Y - Ours.Points[Point].Y));
            ++Compared;
        }
        Tally(SheetGate, Stroke, Stroke <= 1.0e-9, Format("metres × 1000 at 1:%g = millimetres at ", 1.0 / Sheet.Sheet.Scale) + Reference.ScaleLabel
              + Format(": %zu of %zu strokes (all but dimensions) coincide per view, worst %.1e mm (views moved ≤ %.2f mm by the longer texts)", Compared, Reference.Lines.size(), Stroke, Shifted));

        // Paper and angle: A4 picks a scale no larger than A3; third angle mirrors the Top / Right placement about Front.
        SolidArcDrawingSheetSettings Small = Settings;
        Small.Paper = Frontier::GeometryProcessing::DrawingPaperSize::A4;
        Small.Angle = Frontier::GeometryProcessing::DrawingProjectionAngle::Third;
        const SolidArcDrawingSheet Third = ComposeSolidArcDrawingSheet(Rig, Small);
        const auto Centre = [](const Frontier::GeometryProcessing::PlacedDrawingView& View) { return Frontier::GeometryProcessing::Vector2{ 0.5 * (View.Low.X + View.High.X), 0.5 * (View.Low.Y + View.High.Y) }; };
        const bool Placed = Third.Refusal.empty() && Third.Sheet.Views.size() >= 3 && Sheet.Sheet.Views.size() >= 3
                         && Centre(Sheet.Sheet.Views[1]).Y < Centre(Sheet.Sheet.Views[0]).Y && Centre(Third.Sheet.Views[1]).Y > Centre(Third.Sheet.Views[0]).Y
                         && Centre(Sheet.Sheet.Views[2]).X < Centre(Sheet.Sheet.Views[0]).X && Centre(Third.Sheet.Views[2]).X > Centre(Third.Sheet.Views[0]).X;
        Tally(SheetGate, 0.0, Placed && Third.Sheet.Scale <= Sheet.Sheet.Scale && Third.Sheet.Width == 297.0,
              "A4 third angle at " + Third.Sheet.ScaleLabel + ": Top above / Right right of Front (first angle: below / left)");

        // Section A-A along the Section card's y = 1.25 plane: the kernel frame keeps −ZAxis (= the card's removed
        // side flipped), the cap equals the exact Section figure 4.116587.
        SolidArcDrawingSheetSettings Cut = Settings;
        Cut.SectionEnabled = true;
        Cut.Section        = { SolidArcSectionAxis::Y, 1.25, false, {} };
        const SolidArcDrawingSheet Sectioned = ComposeSolidArcDrawingSheet(Rig, Cut);
        const SolidArcSection      Exact     = SectionSolidArcDocument(Rig, Cut.Section);
        double Cap = 0.0;
        std::string CutViews;
        for (const Frontier::GeometryProcessing::PlacedDrawingView& View : Sectioned.Sheet.Views) { Cap += View.View.SectionArea; CutViews += (CutViews.empty() ? "" : " ") + View.Label; }
        const Frontier::GeometryProcessing::OrthonormalFrame Frame = SolidArcSheetSectionFrame(Exact.Plane);
        Tally(SheetGate, std::fabs(Cap - Exact.Area), Sectioned.Refusal.empty() && CutViews == "Front Top Right Section A-A" && std::fabs(Cap - Exact.Area) <= 1.0e-9
              && std::fabs(Exact.Area - 4.116587) <= 5.0e-7 && Dot(Frame.ZAxis, Exact.Plane.ZAxis) <= -1.0 + 1.0e-15,
              CutViews + Format(": cap %.9f m² = section %.9f m², %.0f ms", Cap, Exact.Area, Sectioned.Milliseconds));

        // Export: paths beside the document; the files are the writers' bytes.
        const std::filesystem::path Folder = std::filesystem::temp_directory_path() / "SolidArcSheetValidation";
        std::filesystem::create_directories(Folder);
        const std::string Svg = SolidArcDrawingSheetExportPath((Folder / "Bracket.solidarc").string(), ".svg");
        const std::string Dxf = SolidArcDrawingSheetExportPath((Folder / "Bracket.solidarc").string(), ".dxf");
        const auto Bytes = [](const std::string& Path) { std::ifstream Stream(Path, std::ios::binary); std::ostringstream Text; Text << Stream.rdbuf(); return Text.str(); };
        const bool Written = ExportSolidArcDrawingSheet(Sheet, Svg).empty() && ExportSolidArcDrawingSheet(Sheet, Dxf).empty();
        Tally(SheetGate, 0.0, Written && Svg == (Folder / "Bracket.svg").string() && Dxf == (Folder / "Bracket.dxf").string() && SolidArcDrawingSheetExportPath("", ".svg") == "Untitled.svg"
              && Bytes(Svg) == Frontier::GeometryProcessing::WriteDrawingSvg(Sheet.Sheet) && Bytes(Dxf) == Frontier::GeometryProcessing::WriteDrawingDxf(Sheet.Sheet) && Bytes(Svg).find("⌀600") != std::string::npos,
              Format("Bracket.svg %zu B + Bracket.dxf %zu B byte-equal to the writers", Bytes(Svg).size(), Bytes(Dxf).size()));

        // Saved with the document: defaults write no table; revised settings round trip (fixed point); bad labels refused.
        const std::string Plain = EncodeSolidArcDocument(Rig);
        SolidArcDocument Saved = CloneSolidArcDocument(Rig);
        Saved.DrawingSheet.Paper = Frontier::GeometryProcessing::DrawingPaperSize::A2;
        Saved.DrawingSheet.Angle = Frontier::GeometryProcessing::DrawingProjectionAngle::Third;
        Saved.DrawingSheet.Scale = 0.02;
        Saved.DrawingSheet.SectionEnabled = true;
        Saved.DrawingSheet.Section = { SolidArcSectionAxis::X, 1.0, true, {} };
        Saved.DrawingSheet.HiddenLinesEnabled = false;
        Saved.DrawingSheet.Title = { "Bracket \"B\" Ø", "DWG-0042", "Frontier", "2026-10-23", "AlSi10Mg", "C", "Frontier" };
        const std::string Written1 = EncodeSolidArcDocument(Saved);
        const SolidArcDocumentFile File = DecodeSolidArcDocument(Written1);
        SolidArcDocument Copy = CloneSolidArcDocument(Rig);
        Copy.DrawingSheet = File.DrawingSheet;
        const bool RoundTrip = File.Refusal.empty() && SolidArcDrawingSheetKey(File.DrawingSheet) == SolidArcDrawingSheetKey(Saved.DrawingSheet)
                            && SolidArcSectionKey(File.DrawingSheet.Section) == SolidArcSectionKey(Saved.DrawingSheet.Section) && EncodeSolidArcDocument(Copy) == Written1;
        // toml++ writes 'literal' or "basic" strings; either is swapped.
        const auto Swap = [](std::string Text, const std::string& Key, const std::string& From, const std::string& To)
        {
            for (const char* Quote : { "'", "\"" })
            {
                const std::string Old = Key + " = " + Quote + From + Quote;
                if (const size_t At = Text.find(Old); At != std::string::npos) return Text.replace(At, Old.size(), Key + " = \"" + To + "\"");
            }
            return std::string();
        };
        const std::string BadPaper = Swap(Written1, "Paper", "A2", "A5"), BadAngle = Swap(Written1, "Angle", "Third", "Fifth");
        const std::string PaperRefusal = DecodeSolidArcDocument(BadPaper).Refusal, AngleRefusal = DecodeSolidArcDocument(BadAngle).Refusal;
        Tally(SheetGate, 0.0, Plain.find("[DrawingSheet]") == std::string::npos && Written1.find("[DrawingSheet]") != std::string::npos && RoundTrip
              && PaperRefusal.find("A5") != std::string::npos && AngleRefusal.find("Fifth") != std::string::npos,
              "codec: no table at the defaults, A2 / third / 1:50 / section x flipped / title fields round trip (fixed point), " + PaperRefusal + " · " + AngleRefusal);

        // Refusals: every body hidden, a face section without a face; hiding is part of the cache key.
        SolidArcDocument Hidden = CloneSolidArcDocument(Rig);
        Hidden.Appearance.HiddenBodies.insert(Rig.Regeneration.Bodies[0].Key);
        const SolidArcDrawingSheet None = ComposeSolidArcDrawingSheet(Hidden, Settings);
        SolidArcDrawingSheetSettings Faceless = Settings;
        Faceless.SectionEnabled = true;
        Faceless.Section        = { SolidArcSectionAxis::Face, 0.0, false, {} };
        const SolidArcDrawingSheet NoFace = ComposeSolidArcDrawingSheet(Rig, Faceless);
        Tally(SheetGate, 0.0, None.Refusal == "Drawing sheet: no shown solid body" && NoFace.Refusal.find("section A-A") != std::string::npos
              && SolidArcDrawingSheetCacheKey(Hidden, Settings) != SolidArcDrawingSheetCacheKey(Rig, Settings) && DefaultSolidArcDrawingSheetCondition(Settings) && !DefaultSolidArcDrawingSheetCondition(Cut),
              "refusals: " + None.Refusal + " · " + NoFace.Refusal);
    }

    // ---- W38 STEP / STL exchange: the bracket + a sphere out and back in, OCCT fixtures in, a fillet on an imported edge
    {
        const auto Format   = [](const char* Pattern, auto... Values) { char Text[512]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        const auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        const auto Bytes    = [](const std::string& Path) { std::ifstream Stream(Path, std::ios::binary); std::ostringstream Text; Text << Stream.rdbuf(); return Text.str(); };
        const std::filesystem::path Folder = std::filesystem::temp_directory_path() / "SolidArcExchangeValidation";
        std::filesystem::remove_all(Folder);
        std::filesystem::create_directories(Folder);
        const std::string StepPath = (Folder / "Bracket.step").string(), StlPath = (Folder / "Bracket.stl").string();

        // ① Bracket specimen + a sphere r 0.5 at x = 6 (New body): STEP of both = the kernel writer's bytes.
        SolidArcDocument Rig = BuildSolidArcSpecimen();
        SolidArcConstructRequest Request;
        Request.Kind = SolidArcConstructKind::Sphere; Request.Position = { 6.0, 0.0, 0.5 }; Request.Radius = 0.5;
        Request.Combination = FeatureCombination::Separate;
        Expect(ConstructSolidArcPrimitive(Rig, Request));
        Rig.Selection = {};
        std::vector<BodyToken> Tokens;
        for (const RegeneratedBody& Body : Rig.Regeneration.Bodies) Tokens.push_back(Body.Body);
        const SolidArcExportOutcome Step = ExportSolidArcBodies(Rig, StepPath);
        Frontier::GeometryProcessing::StepRequest Direct;
        Direct.ModelUnitMillimetres = SolidArcModelUnitMillimetres;
        Direct.Name                 = "Bracket";
        const std::string Expected = Frontier::GeometryProcessing::WriteStep(*Rig.Partition, Tokens, Direct).Bytes;
        const std::string Written  = Bytes(StepPath);
        Tally(ExchangeGate, 0.0, Step.Refusal.empty() && Step.Bodies.size() == 2 && Tokens.size() == 2 && Written == Expected && Step.Bytes == double(Written.size())
              && Written.find("FILE_NAME('Bracket.step'") != std::string::npos && SolidArcExportPath((Folder / "Bracket.solidarc").string(), ".step") == StepPath,
              Format("Export… Bracket.step: %zu bodies, %d faces, %d stand-in(s), %.0f B = WriteStep bytes, %.0f ms", Step.Bodies.size(), Step.Faces, Step.Approximated, Step.Bytes, Step.Milliseconds));

        // Read back in millimetres by the kernel reader: volumes × 10⁹.
        ModelPartition Millimetres;
        StepImportRequest MillimetreRequest;
        const StepImport Back = ReadStep(Millimetres, Written, MillimetreRequest);
        double WorstBack = Back.Bodies.size() == 2 ? 0.0 : 1.0;
        for (size_t Index = 0; Index < Back.Bodies.size() && Index < Rig.Summary.Bodies.size(); ++Index)
            WorstBack = std::max(WorstBack, Relative(MeasureBodyMass(Millimetres, Back.Bodies[Index]).Volume, Rig.Summary.Bodies[Index].Volume * 1.0e9));
        Tally(ExchangeGate, WorstBack, Back.Refusal.empty() && WorstBack <= 1.0e-9 && Back.FileUnitMillimetres == 1.0,
              Format("read back in mm: %zu solids, volumes = metres³ × 10⁹ (worst %.1e)", Back.Bodies.size(), WorstBack));

        // ② STL: the writer's bytes, watertight per body, |ΔV| ≤ sag · A (mm).
        const SolidArcExportOutcome Facets = ExportSolidArcBodies(Rig, StlPath);
        StlRequest Tessellated;
        Tessellated.ModelUnitMillimetres = SolidArcModelUnitMillimetres;
        Tessellated.Name                 = "Bracket";
        const StlExport    Reference = WriteStl(*Rig.Partition, Tokens, Tessellated);
        const StlReading   Reading   = ReadStl(Bytes(StlPath));
        const StlSoundness Sound     = MeasureStlSoundness(Reading);
        const double Exact = Rig.Summary.Volume * 1.0e9;
        const double Gap   = std::abs(Sound.Volume - Exact);
        Tally(ExchangeGate, Gap / Exact, Facets.Refusal.empty() && Bytes(StlPath) == Reference.Bytes && Sound.WatertightCondition && Sound.Components == 2 && Gap <= Reference.Sag * SolidArcModelUnitMillimetres * Sound.Area,
              Format("Export… Bracket.stl: %d facets, watertight, 2 components, |ΔV| %.3f mm³ ≤ sag %.2f mm × A %.0f mm²", Facets.Triangles, Gap, Reference.Sag * SolidArcModelUnitMillimetres, Sound.Area));

        // ③ What is written: the selection, else every shown body (hidden ones stay out).
        const std::string SphereKey = Rig.Summary.Bodies.size() == 2 ? Rig.Summary.Bodies[1].Key : std::string{};
        SolidArcDocument Chosen = CloneSolidArcDocument(Rig);
        SelectSolidArcName(Chosen, SolidArcSelectionKind::Body, SphereKey, false);
        const std::vector<std::string> Picked = SolidArcExportBodies(Chosen);
        SolidArcDocument Veiled = CloneSolidArcDocument(Rig);
        Veiled.Appearance.HiddenBodies.insert(Rig.Summary.Bodies.front().Key);
        const std::vector<std::string> Shown = SolidArcExportBodies(Veiled);
        const SolidArcExportOutcome Single = ExportSolidArcBodies(Chosen, (Folder / "Sphere.stp").string());
        Tally(ExchangeGate, 0.0, Picked == std::vector<std::string>{ SphereKey } && Shown == std::vector<std::string>{ SphereKey } && Single.Refusal.empty() && Single.Bodies.size() == 1
              && Bytes(Single.Path).find("PRODUCT('Sphere'") != std::string::npos,
              "selected sphere → 1 body; bracket hidden → 1 body; Sphere.stp names its PRODUCT 'Sphere'");

        // ④ Import… Bracket.step into a fresh document: one Import step per solid, one undo entry, equal figures.
        SolidArcDocument Received = CreateSolidArcDocument("Received");
        const SolidArcStepReading    Read     = ReadSolidArcStepFile(StepPath);
        const SolidArcCommandOutcome Imported = ImportSolidArcStep(Received, Read);
        double WorstVolume = Received.Summary.Bodies.size() == 2 ? 0.0 : 1.0;
        bool   Figures     = Received.Summary.Bodies.size() == 2;
        std::string Captions;
        for (size_t Index = 0; Figures && Index < 2; ++Index)
        {
            const SolidArcBodyFigures& Ours = Received.Summary.Bodies[Index];
            const SolidArcBodyFigures& Theirs = Rig.Summary.Bodies[Index];
            WorstVolume = std::max(WorstVolume, Relative(Ours.Volume, Theirs.Volume));
            Figures = Figures && Ours.Faces == Theirs.Faces && Ours.Edges == Theirs.Edges && Ours.Vertices == Theirs.Vertices && Ours.Genus == Theirs.Genus && Ours.SoundCondition;
            Captions += (Captions.empty() ? "" : " · ") + Ours.Caption + " (" + Ours.Key + Format(", %d faces)", Ours.Faces);
        }
        const bool Steps = Received.Tree.Steps.size() == 2 && Received.Tree.Steps[0].Kind == FeatureStepKind::Import && Received.Tree.Steps[0].ExchangeUnit == SolidArcModelUnitMillimetres
                        && Received.UndoEntries.size() == 1;
        Tally(ExchangeGate, WorstVolume, Read.Refusal.empty() && Imported.AcceptedCondition && Steps && Figures && WorstVolume <= 1.0e-9 && Read.Texts.size() == 2,
              Imported.Label + ": " + Captions + Format(" · volumes, faces, edges, vertices, genus equal (worst %.1e) · %.0f ms", WorstVolume, Imported.Milliseconds));

        // Faces are named in file order; the names reach the selection.
        const std::string FirstFace = Received.Regeneration.Bodies.empty() || Received.Regeneration.Bodies[0].Naming.Faces.empty() ? std::string{} : Received.Regeneration.Bodies[0].Naming.Faces.begin()->second;
        bool Named = !FirstFace.empty();
        for (const RegeneratedBody& Body : Received.Regeneration.Bodies)
            for (const auto& [Slot, Name] : Body.Naming.Faces) Named = Named && Name.rfind(Body.Key + ".Face(", 0) == 0;
        Tally(ExchangeGate, 0.0, Named, "every imported face is named <Import#k>.Face(n), e.g. " + FirstFace);

        // ⑤ Canonical writing: export the imported bodies, import that, export again — the same file up to the last digit
        //    of the metre ↔ millimetre scaling (numbers compared relatively, everything else byte for byte).
        const std::string Second = (Folder / "Second" / "Bracket.step").string(), Third = (Folder / "Third" / "Bracket.step").string();
        std::filesystem::create_directories(Folder / "Second");
        std::filesystem::create_directories(Folder / "Third");
        const SolidArcExportOutcome Again = ExportSolidArcBodies(Received, Second);
        SolidArcDocument Echo = CreateSolidArcDocument("Echo");
        const SolidArcCommandOutcome Echoed = ImportSolidArcStep(Echo, ReadSolidArcStepFile(Second));
        const SolidArcExportOutcome Last = ExportSolidArcBodies(Echo, Third);
        const std::string SecondText = Bytes(Second), ThirdText = Bytes(Third);
        double Drift = 0.0;
        bool   Same  = true;
        int    Numbers = 0, Differing = 0;
        for (size_t First = 0, Other = 0; Same && (First < SecondText.size() || Other < ThirdText.size());)
        {
            const auto Numeric = [](const std::string& Text, size_t At) { return At < Text.size() && (std::isdigit(static_cast<unsigned char>(Text[At])) || Text[At] == '-'); };
            if (Numeric(SecondText, First) && Numeric(ThirdText, Other) && First > 0 && Other > 0 && (SecondText[First - 1] == '(' || SecondText[First - 1] == ','))
            {
                char* EndFirst = nullptr;
                char* EndOther = nullptr;
                const double A = std::strtod(SecondText.c_str() + First, &EndFirst), B = std::strtod(ThirdText.c_str() + Other, &EndOther);
                Drift = std::max(Drift, std::abs(A - B) / std::max(1.0, std::abs(A)));
                ++Numbers;
                Differing += A != B;
                First = size_t(EndFirst - SecondText.c_str());
                Other = size_t(EndOther - ThirdText.c_str());
                continue;
            }
            Same = First < SecondText.size() && Other < ThirdText.size() && SecondText[First] == ThirdText[Other];
            ++First;
            ++Other;
        }
        Tally(ExchangeGate, Drift, Again.Refusal.empty() && Echoed.AcceptedCondition && Last.Refusal.empty() && Same && Drift <= 1.0e-14 && Read.Texts[0] == Received.Tree.Steps[0].Exchange,
              Format("export → import → export: same entities, %d of %d numbers differ in the last digit (worst %.1e, mm ↔ m scaling); the embedded text is the canonical writing", Differing, Numbers, Drift));

        // ⑥ Saved with the document: the STEP text rides along (multi-line string); save → load → save byte-equal, digests equal.
        const std::string Saved1 = EncodeSolidArcDocument(Received);
        const SolidArcDocumentFile Loaded = DecodeSolidArcDocument(Saved1);
        SolidArcDocument Reloaded = CreateSolidArcDocument(Loaded.Title);
        const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Reloaded, Loaded.Tree, Loaded.Captions, "Open");
        bool Digests = Reloaded.Tree.Steps.size() == Received.Tree.Steps.size();
        for (size_t Index = 0; Digests && Index < Received.Tree.Steps.size(); ++Index)
            Digests = DigestFeatureStep(Reloaded.Tree, Reloaded.Tree.Steps[Index]) == DigestFeatureStep(Received.Tree, Received.Tree.Steps[Index]);
        FeatureTree Altered = Received.Tree;
        if (!Altered.Steps.empty()) Altered.Steps[0].Exchange += " ";
        const bool Sensitive = !Altered.Steps.empty() && DigestFeatureStep(Altered, Altered.Steps[0]) != DigestFeatureStep(Received.Tree, Received.Tree.Steps[0]);
        Tally(ExchangeGate, std::abs(Reloaded.Summary.Volume - Received.Summary.Volume), Loaded.Refusal.empty() && Opened.AcceptedCondition && Reloaded.Summary.Volume == Received.Summary.Volume
              && EncodeSolidArcDocument(Reloaded) == Saved1 && Digests && Sensitive && Reloaded.Regeneration.Naming.Faces == Received.Regeneration.Naming.Faces,
              Format("codec: %zu B of TOML carry both files; save → load → save byte-equal, volumes and names equal, digests equal (and change with the text)", Saved1.size()));

        // ⑦ OCCT-written fixtures: FilletedBoss (a loop running against its mates is repaired), InchBlock (inch → metres).
        const std::string Fixtures = SOLIDARC_STEP_FIXTURES;
        SolidArcDocument Boss = CreateSolidArcDocument("Boss");
        const SolidArcStepReading BossRead = ReadSolidArcStepFile(Fixtures + "/FilletedBoss.step");
        const SolidArcCommandOutcome BossIn = ImportSolidArcStep(Boss, BossRead);
        Tally(ExchangeGate, Relative(Boss.Summary.Volume, 23897.5876482e-9), BossIn.AcceptedCondition && Boss.Summary.SoundCondition && Relative(Boss.Summary.Volume, 23897.5876482e-9) <= 1.0e-9
              && Boss.Summary.Faces == 9 && BossRead.FileUnitMillimetres == 1.0,
              Format("FilletedBoss.step (OCCT): 9 faces, V %.10e m³, %d loop(s) reoriented, %d tolerant edge(s)", Boss.Summary.Volume, BossRead.ReorientedFaces, BossRead.TolerantEdges));
        SolidArcDocument Inch = CreateSolidArcDocument("Inch");
        const SolidArcStepReading InchRead = ReadSolidArcStepFile(Fixtures + "/InchBlock.step");
        const SolidArcCommandOutcome InchIn = ImportSolidArcStep(Inch, InchRead);
        const double Side = 0.0254;
        Tally(ExchangeGate, Relative(Inch.Summary.Volume, Side * Side * Side), InchIn.AcceptedCondition && Relative(Inch.Summary.Volume, Side * Side * Side) <= 1.0e-9 && InchRead.FileUnitMillimetres == 25.4,
              Format("InchBlock.step (OCCT, INCH, 1 × ½ × 2 in): V = 0.0254³ m³ (%.10e), caption %s", Inch.Summary.Volume, Inch.Summary.Bodies.empty() ? "?" : Inch.Summary.Bodies[0].Caption.c_str()));

        // ⑧ The imported body is a body like any other: fillet r 5 mm on an edge by its persistent name. OpenCascade writes
        //    the box's faces x−, x+, y−, y+, z−, z+, so Face(1) | Face(6) is the ½ in edge along y of the 1 × ½ × 2 in block.
        const std::string Edge = Inch.Regeneration.Bodies.empty() || Inch.Regeneration.Bodies[0].Naming.Edges.empty() ? std::string{} : Inch.Regeneration.Bodies[0].Naming.Edges.begin()->second;
        SelectSolidArcName(Inch, SolidArcSelectionKind::Edge, Edge, false);
        const double Radius = 0.005;
        const SolidArcCommandOutcome Rounded = BlendSolidArcSelection(Inch, BlendProfile::Fillet, Radius);
        const double Removed = (1.0 - Pi / 4.0) * Radius * Radius * (Side / 2.0);
        Tally(ExchangeGate, 0.0, Edge == "Edge(Import#1.Face(1)|Import#1.Face(6))", "the edge picked by name: " + Edge);
        TallyVolume(ExchangeGate, Inch, Side * Side * Side - Removed, "fillet r 5 mm on imported edge " + Edge + (Rounded.AcceptedCondition ? "" : " refused: " + Rounded.Refusal) + ": V = 0.0254³ − (1 − π/4) r² · 0.0127");

        // ⑨ Refusals leave the document as it was: a missing file, a damaged solid, a non-STEP name, nothing to export.
        std::string Damaged = Written;
        const size_t Shell = Damaged.find("CLOSED_SHELL('',(#");
        if (Shell != std::string::npos) Damaged.insert(Shell + 17, "999999,");
        const std::string DamagedPath = (Folder / "Damaged.step").string();
        { std::ofstream Stream(DamagedPath, std::ios::binary); Stream << Damaged; }
        SolidArcDocument& Kept   = Received;
        const size_t      Undos  = Received.UndoEntries.size();
        const std::string Before = EncodeSolidArcDocument(Kept);
        const SolidArcStepReading    Missing  = ReadSolidArcStepFile((Folder / "Absent.step").string());
        const SolidArcStepReading    Broken   = ReadSolidArcStepFile(DamagedPath);
        const SolidArcCommandOutcome Refused1 = ImportSolidArcStep(Kept, Missing);
        const SolidArcCommandOutcome Refused2 = ImportSolidArcStep(Kept, Broken);
        const SolidArcExportOutcome  Wrong    = ExportSolidArcBodies(Rig, (Folder / "Bracket.obj").string());
        const SolidArcExportOutcome  Empty    = ExportSolidArcBodies(CreateSolidArcDocument("Empty"), (Folder / "Empty.step").string());
        Tally(ExchangeGate, 0.0, !Refused1.AcceptedCondition && !Refused2.AcceptedCondition && EncodeSolidArcDocument(Kept) == Before && Kept.UndoEntries.size() == Undos
              && !Broken.Refusal.empty() && !Wrong.Refusal.empty() && !Empty.Refusal.empty() && !std::filesystem::exists(Folder / "Bracket.obj") && !std::filesystem::exists(Folder / "Empty.step"),
              "refused, document unchanged: " + Refused1.Refusal + " · " + Refused2.Refusal + " · " + Wrong.Refusal + " · " + Empty.Refusal);

        // A saved Import step without its text is refused by the codec.
        std::string Hollow = Saved1;
        const size_t Text = Hollow.find("Exchange = ");
        if (Text != std::string::npos) Hollow.replace(Text, 11, "Exchanged = ");
        const SolidArcDocumentFile Emptied = DecodeSolidArcDocument(Hollow);
        Tally(ExchangeGate, 0.0, Text != std::string::npos && !Emptied.Refusal.empty(), "codec: an Import step without its STEP text refused: " + Emptied.Refusal);

        // Undo removes both imports (one entry), redo restores them.
        const double Whole = Received.Summary.Volume;
        const bool Back1 = UndoSolidArcCommand(Received) && Received.Tree.Steps.empty() && Received.Summary.Bodies.empty();
        const bool Forth = RedoSolidArcCommand(Received) && Received.Summary.Volume == Whole && Received.Summary.Bodies.size() == 2;
        Tally(ExchangeGate, 0.0, Back1 && Forth, "undo removes the import (both bodies), redo restores it");
    }

    // ---- W39 bevel previews after the first Windows run: an upright fillet beside a 0.4 bottom fillet (froze / refused /
    //      off by 10⁻⁶), and a blend as wide as the neighbouring face (crashed)
    {
        const auto Format = [](const char* Pattern, auto... Values) { char Text[512]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        constexpr double Bottom = 0.4;                                                       // [len] - First fillet radius R
        constexpr double Notch  = 1.0 - std::numbers::pi / 4.0;                              // [-]   - Fillet cross-section / r²
        // 📝 Closed form: V = 2·1.5·1 − Notch·(R²·2 + r²·1) + overlap of the two notch prisms. In the corner the upright
        //    notch (offsets 𝑢, 𝑣 from the two side faces, 𝑢 < r − √(r² − (r − 𝑣)²)) reaches down to the bottom fillet at
        //    height h(𝑣) = R − √(R² − (R − 𝑣)²), so overlap = ∫₀ʳ 𝑢max(𝑣)·h(𝑣) d𝑣 — Simpson in 𝑠 with 𝑣 = r𝑠² (the √
        //    end points become smooth), converged to rounding at 40 000 panels.
        const auto Overlap = [&](double Radius)
        {
            constexpr int Panels = 40000;                                                    // [-] - Even
            double Sum = 0.0;                                                                // [len³] - Simpson sum
            for (int Index = 0; Index <= Panels; ++Index)
            {
                const double Unit   = static_cast<double>(Index) / Panels;                   // [-]   - 𝑠
                const double Offset = Radius * Unit * Unit;                                  // [len] - 𝑣
                const double Height = Bottom - std::sqrt(std::max(0.0, Bottom * Bottom - (Bottom - Offset) * (Bottom - Offset)));
                const double Reach  = Radius - std::sqrt(std::max(0.0, Radius * Radius - (Radius - Offset) * (Radius - Offset)));
                const double Weight = (Index == 0 || Index == Panels) ? 1.0 : (Index % 2 == 1 ? 4.0 : 2.0);
                Sum += Weight * Reach * Height * 2.0 * Radius * Unit;
            }
            return Sum / (3.0 * Panels);
        };
        SolidArcDocument Base = CreateSolidArcDocument("Bounded");
        Expect(ConstructSolidArcPrimitive(Base, SolidArcConstructRequest{}));
        SelectSolidArcName(Base, SolidArcSelectionKind::Edge, "Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))", false);
        SolidArcBlendRequest First;
        First.Size = Bottom;
        Expect(BlendSolidArcSelection(Base, First));
        // 0.1137 = the screenshot; 0.1 bisected for 334 s; 0.01 / 0.03 / 0.05 / 0.12 / 0.3 were refused; 0.02 / 0.1137 were 10⁻⁶ off.
        for (const double Radius : { 0.01, 0.02, 0.03, 0.05, 0.1, 0.1137, 0.12, 0.3 })
        {
            SolidArcDocument Preview = CloneSolidArcDocument(Base);
            ClearSolidArcSelection(Preview);
            SelectSolidArcName(Preview, SolidArcSelectionKind::Edge, "Edge(Extrude#2.Side(Line1)|Extrude#2.Side(Line2))", false);
            SolidArcBlendRequest Upright;
            Upright.Size = Radius;
            const auto   Begun    = std::chrono::steady_clock::now();
            const SolidArcCommandOutcome Outcome = BlendSolidArcSelection(Preview, Upright);
            const double Seconds  = std::chrono::duration<double>(std::chrono::steady_clock::now() - Begun).count();
            const double Expected = 3.0 - Notch * (Bottom * Bottom * 2.0 + Radius * Radius) + Overlap(Radius);   // [len³]
            const double Relative = std::abs(Preview.Summary.Volume - Expected) / Expected;
            Tally(BoundGate, Relative, Outcome.AcceptedCondition && Preview.Summary.SoundCondition && Relative <= 1.0e-9 && Seconds <= 20.0,
                  Format("upright fillet r %.4g beside the 0.4 bottom fillet: V %.12f vs %.12f, %.2f s%s", Radius, Preview.Summary.Volume, Expected, Seconds,
                         Outcome.AcceptedCondition ? "" : (" — refused: " + Outcome.Refusal).c_str()));
        }
        for (const BlendProfile Shape : { BlendProfile::Fillet, BlendProfile::Chamfer })
        {
            SolidArcDocument Plain = CreateSolidArcDocument("Width");
            Expect(ConstructSolidArcPrimitive(Plain, SolidArcConstructRequest{}));
            const std::string Before = EncodeSolidArcDocument(Plain);
            SelectSolidArcName(Plain, SolidArcSelectionKind::Edge, "Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))", false);
            SolidArcBlendRequest Wide;
            Wide.Shape = Shape;
            Wide.Size  = 1.5;
            const SolidArcCommandOutcome Outcome = BlendSolidArcSelection(Plain, Wide);
            Plain.Selection = {};
            Tally(BoundGate, 0.0, !Outcome.AcceptedCondition && EncodeSolidArcDocument(Plain) == Before,
                  std::string(Shape == BlendProfile::Fillet ? "fillet" : "chamfer") + " 1.5 = the bottom cap's depth: refused, document unchanged: " + Outcome.Refusal);
        }
    }

    // ---- W40 Construct catalogue: the new primitives against closed forms, every tile a body of its own
    {
        constexpr double π = std::numbers::pi;
        const auto Format = [](const char* Pattern, auto... Values) { char Text[512]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        const auto PrismVolume = [&](int Sides, double Radius, double Height) { return 0.5 * Sides * Radius * Radius * std::sin(2.0 * π / Sides) * Height; };
        struct Case { const char* Name; SolidArcConstructRequest Request; double Expected; };
        std::vector<Case> Cases;
        {
            SolidArcConstructRequest Request;
            Request.Kind = SolidArcConstructKind::Cone; Request.Position = { 1.0, 2.0, 0.3 }; Request.Radius = 0.5; Request.Height = 1.2;
            Cases.push_back({ "cone r 0.5 h 1.2", Request, π * 0.25 * 1.2 / 3.0 });
            Request.Height = -0.8;
            Cases.push_back({ "cone r 0.5 h −0.8 (points down)", Request, π * 0.25 * 0.8 / 3.0 });
            Request = {};
            Request.Kind = SolidArcConstructKind::Wedge; Request.Position = { -1.0, 0.5, 0.0 }; Request.Width = 2.0; Request.Depth = 1.5; Request.Height = 1.0;
            Cases.push_back({ "wedge 2 × 1.5 × 1", Request, 1.5 });
            for (const int Sides : { 3, 6, 8 })
            {
                Request = {};
                Request.Kind = SolidArcConstructKind::Prism; Request.Position = { 0.5, -0.5, 0.2 }; Request.Radius = 0.6; Request.Height = 0.9; Request.Sides = Sides; Request.Angle = 0.3;
                Cases.push_back({ Sides == 3 ? "prism 3 sides" : Sides == 6 ? "prism 6 sides" : "prism 8 sides", Request, PrismVolume(Sides, 0.6, 0.9) });
            }
            Request = {};
            Request.Kind = SolidArcConstructKind::Tube; Request.Position = { 0.0, 0.0, 0.0 }; Request.Radius = 0.5; Request.Inner = 0.3; Request.Height = 1.0;
            Cases.push_back({ "tube 0.5 / 0.3 h 1", Request, π * (0.25 - 0.09) });
        }
        for (const Case& Entry : Cases)
        {
            SolidArcDocument Single = CreateSolidArcDocument("Primitive");
            SolidArcConstructRequest Request = Entry.Request;
            Request.Combination = FeatureCombination::Separate;
            const SolidArcCommandOutcome Outcome = ConstructSolidArcPrimitive(Single, Request);
            const double Relative = std::abs(Single.Summary.Volume - Entry.Expected) / Entry.Expected;
            Tally(PrimitiveGate, Relative, Outcome.AcceptedCondition && Single.Summary.SoundCondition && Relative <= 1.0e-9 && Single.Summary.Bodies.size() == 1,
                  Format("%s: V %.12f vs %.12f%s", Entry.Name, Single.Summary.Volume, Entry.Expected, Outcome.AcceptedCondition ? "" : (" — refused: " + Outcome.Refusal).c_str()));
        }

        // ① eight tiles in a row, each Separate: eight bodies, each exact, total = Σ (no Boolean merged anything)
        SolidArcDocument Row = CreateSolidArcDocument("Catalogue");
        std::vector<std::pair<std::string, double>> Expected;
        double Total = 0.0;
        const SolidArcConstructKind Kinds[] = { SolidArcConstructKind::Block, SolidArcConstructKind::Cylinder, SolidArcConstructKind::Sphere, SolidArcConstructKind::Torus,
                                                SolidArcConstructKind::Cone, SolidArcConstructKind::Wedge, SolidArcConstructKind::Prism, SolidArcConstructKind::Tube };
        for (int Index = 0; Index < 8; ++Index)
        {
            SolidArcConstructRequest Request;
            Request.Kind        = Kinds[Index];
            Request.Combination = FeatureCombination::Separate;
            const double Across = 4.0 * Index;
            double       Volume = 0.0;
            switch (Request.Kind)
            {
            case SolidArcConstructKind::Block:    Request.Position = { Across, 0.0, 0.0 }; Volume = 2.0 * 1.5 * 1.0; break;
            case SolidArcConstructKind::Cylinder: Request.Position = { Across, 0.0, 0.0 }; Request.Radius = 0.6; Request.Height = 1.1; Volume = π * 0.36 * 1.1; break;
            case SolidArcConstructKind::Sphere:   Request.Position = { Across, 0.0, 0.7 }; Request.Radius = 0.7; Volume = 4.0 / 3.0 * π * 0.343; break;
            case SolidArcConstructKind::Torus:    Request.Position = { Across, 0.0, 0.4 }; Request.Major = 1.2; Request.Minor = 0.4; Volume = 2.0 * π * π * 1.2 * 0.16; break;
            case SolidArcConstructKind::Cone:     Request.Position = { Across, 0.0, 0.0 }; Request.Radius = 0.8; Request.Height = 1.5; Volume = π * 0.64 * 1.5 / 3.0; break;
            case SolidArcConstructKind::Wedge:    Request.Position = { Across, 0.0, 0.0 }; Request.Width = 1.8; Request.Depth = 1.2; Request.Height = 0.9; Volume = 0.5 * 1.8 * 1.2 * 0.9; break;
            case SolidArcConstructKind::Prism:    Request.Position = { Across, 0.0, 0.0 }; Request.Radius = 0.7; Request.Height = 1.0; Request.Sides = 5; Volume = PrismVolume(5, 0.7, 1.0); break;
            case SolidArcConstructKind::Tube:     Request.Position = { Across, 0.0, 0.0 }; Request.Radius = 0.7; Request.Inner = 0.45; Request.Height = 1.3; Volume = π * (0.49 - 0.2025) * 1.3; break;
            }
            Expect(ConstructSolidArcPrimitive(Row, Request));
            Expected.push_back({ Row.Regeneration.Active, Volume });
            Total += Volume;
        }
        int Matched = 0;
        double Worst = 0.0;
        for (const auto& [Key, Volume] : Expected)
            for (const SolidArcBodyFigures& Body : Row.Summary.Bodies)
                if (Body.Key == Key)
                {
                    const double Relative = std::abs(Body.Volume - Volume) / Volume;
                    Worst = std::max(Worst, Relative);
                    Matched += Relative <= 1.0e-9 && Body.SoundCondition ? 1 : 0;
                }
        Tally(PrimitiveGate, Worst, Row.Summary.Bodies.size() == 8 && Matched == 8 && Row.Summary.SoundCondition,
              Format("8 Separate tiles (block, cylinder, sphere, torus, cone, wedge, prism, tube): %d bodies, %d exact, worst relative %.3g", int(Row.Summary.Bodies.size()), Matched, Worst));
        const double TotalRelative = std::abs(Row.Summary.Volume - Total) / Total;
        Tally(PrimitiveGate, TotalRelative, TotalRelative <= 1.0e-9, Format("part volume = Σ tiles: %.12f vs %.12f", Row.Summary.Volume, Total));

        // ② parameter edits regenerate: prism Radius 0.7 → 0.9, tube bore 0.45 → 0.2
        auto ParameterEndingIn = [&](const char* Stem, const char* Suffix)
        {
            for (const auto& Parameter : Row.Tree.Parameters)
                if (Parameter.Name.rfind(Stem, 0) == 0 && Parameter.Name.size() > std::strlen(Suffix) && Parameter.Name.compare(Parameter.Name.size() - std::strlen(Suffix), std::string::npos, Suffix) == 0)
                    return Parameter.Name;
            return std::string();
        };
        const std::string PrismRadius = ParameterEndingIn("Prism", ".Radius");
        const SolidArcCommandOutcome Grown = AssignSolidArcParameter(Row, PrismRadius, 0.9);
        const double PrismExpected = Total - PrismVolume(5, 0.7, 1.0) + PrismVolume(5, 0.9, 1.0);
        const double PrismRelative = std::abs(Row.Summary.Volume - PrismExpected) / PrismExpected;
        Tally(PrimitiveGate, PrismRelative, Grown.AcceptedCondition && PrismRelative <= 1.0e-9 && Row.Summary.Bodies.size() == 8,
              Format("%s 0.7 → 0.9 (equal sides + on-circle constraints follow): V %.12f vs %.12f", PrismRadius.c_str(), Row.Summary.Volume, PrismExpected));
        const std::string TubeBore = ParameterEndingIn("Tube", ".Inner");
        const SolidArcCommandOutcome Bored = AssignSolidArcParameter(Row, TubeBore, 0.2);
        const double TubeExpected = PrismExpected - π * (0.49 - 0.2025) * 1.3 + π * (0.49 - 0.04) * 1.3;
        const double TubeRelative = std::abs(Row.Summary.Volume - TubeExpected) / TubeExpected;
        Tally(PrimitiveGate, TubeRelative, Bored.AcceptedCondition && TubeRelative <= 1.0e-9,
              Format("%s 0.45 → 0.2: V %.12f vs %.12f", TubeBore.c_str(), Row.Summary.Volume, TubeExpected));

        // ③ codec: save → load → save byte-equal, same volume
        const std::string RowSaved = EncodeSolidArcDocument(Row);
        SolidArcDocumentFile RowFile = DecodeSolidArcDocument(RowSaved);
        SolidArcDocument     RowLoaded = CreateSolidArcDocument(RowFile.Title);
        const SolidArcCommandOutcome RowOpened = ReplaceSolidArcTree(RowLoaded, RowFile.Tree, RowFile.Captions, "Open");
        Tally(PrimitiveGate, std::abs(RowLoaded.Summary.Volume - Row.Summary.Volume), RowOpened.AcceptedCondition && EncodeSolidArcDocument(RowLoaded) == RowSaved &&
              RowLoaded.Summary.Volume == Row.Summary.Volume && RowLoaded.Summary.Bodies.size() == 8, Format("codec: %d bytes, save → load → save byte-equal, 8 bodies", int(RowSaved.size())));

        // ④ drawn polygon (5 sides, R 0.8) extruded 0.5 as a new body = (5/2) R² sin 72° · 0.5
        SolidArcDocument Drawn = CreateSolidArcDocument("Polygon");
        SolidArcDrawingRequest Pentagon;
        Pentagon.Kind = SolidArcDrawingKind::Polygon; Pentagon.Sides = 5; Pentagon.Points = { { 1.0, 1.0 }, { 1.0, 1.8 } };
        Expect(DrawSolidArcSketch(Drawn, Pentagon));
        const std::string PentagonLabel = Drawn.Tree.Steps.back().Label;
        const SolidArcCommandOutcome Pushed = ExtrudeSolidArcSketch(Drawn, PentagonLabel, 0.5, FeatureCombination::Separate, {});
        const double PentagonExpected = PrismVolume(5, 0.8, 0.5);
        const double PentagonRelative = std::abs(Drawn.Summary.Volume - PentagonExpected) / PentagonExpected;
        Tally(PrimitiveGate, PentagonRelative, Pushed.AcceptedCondition && PentagonRelative <= 1.0e-9 && std::abs(MeasureDrawingArea(Pentagon) - 2.0 * PentagonExpected) <= 1.0e-12,
              Format("drawn polygon 5 sides R 0.8 extruded 0.5: V %.12f vs %.12f", Drawn.Summary.Volume, PentagonExpected));

        // ⑤ refusals leave the document untouched
        const std::string Before = EncodeSolidArcDocument(Row);
        SolidArcConstructRequest Bad;
        Bad.Kind = SolidArcConstructKind::Tube; Bad.Radius = 0.4; Bad.Inner = 0.4; Bad.Combination = FeatureCombination::Separate;
        const SolidArcCommandOutcome BoreRefused = ConstructSolidArcPrimitive(Row, Bad);
        Bad.Kind = SolidArcConstructKind::Prism; Bad.Sides = 2;
        const SolidArcCommandOutcome SidesRefused = ConstructSolidArcPrimitive(Row, Bad);
        Pentagon.Sides = 2;
        SketchModel Probe;
        const std::string PolygonRefusal = ComposeSolidArcSketch(Pentagon, Probe);
        Tally(PrimitiveGate, 0.0, !BoreRefused.AcceptedCondition && !SidesRefused.AcceptedCondition && !PolygonRefusal.empty() && EncodeSolidArcDocument(Row) == Before,
              "refused, document unchanged: " + BoreRefused.Refusal + " · " + SidesRefused.Refusal + " · " + PolygonRefusal);
    }

    // ---- W41 Inset (Plasticity O on faces · I): border ring + inner face, optional boss / pocket depth
    {
        constexpr double π = std::numbers::pi;
        const auto Format = [](const char* Pattern, auto... Values) { char Text[512]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        // A fresh document with one primitive and the face under a downward ray at (X, Y) selected.
        const auto Prepared = [&](SolidArcConstructRequest Request, double X, double Y, std::string& Face)
        {
            SolidArcDocument Document = CreateSolidArcDocument("Inset");
            Request.Combination = FeatureCombination::Separate;
            Expect(ConstructSolidArcPrimitive(Document, Request));
            Face = PickAndSelect(Document, RayBetween({ X, Y, 5.0 }, { X, Y, -5.0 }), false);
            return Document;
        };
        struct Case { const char* Name; SolidArcConstructRequest Request; double X, Y, Distance, Depth, Expected; int Faces; };
        SolidArcConstructRequest Block;
        Block.Kind = SolidArcConstructKind::Block; Block.Width = 2.0; Block.Depth = 1.5; Block.Height = 1.0;
        SolidArcConstructRequest Cylinder;
        Cylinder.Kind = SolidArcConstructKind::Cylinder; Cylinder.Radius = 0.6; Cylinder.Height = 1.0;
        SolidArcConstructRequest Triangle;
        Triangle.Kind = SolidArcConstructKind::Prism; Triangle.Sides = 3; Triangle.Radius = 0.6; Triangle.Height = 0.9;
        const double TriangleArea  = 0.5 * 3.0 * 0.36 * std::sin(2.0 * π / 3.0);            // [len²] - Inradius R/2 = 0.3
        const double TriangleScale = (0.3 - 0.1) / 0.3;                                      // [-]    - Inset 0.1 shrinks the inradius
        const Case Cases[] = {
            { "block top, inset 0.25, depth 0 (flat split)", Block, 1.0, 0.75, 0.25, 0.0, 3.0, 7 },
            { "block top, inset 0.25, depth −0.2 (pocket)", Block, 1.0, 0.75, 0.25, -0.2, 3.0 - 1.5 * 1.0 * 0.2, 11 },
            { "block top, inset 0.25, depth +0.3 (boss)", Block, 1.0, 0.75, 0.25, 0.3, 3.0 + 1.5 * 1.0 * 0.3, 11 },
            { "cylinder top, inset 0.2, depth 0 (annulus + disc)", Cylinder, 0.1, 0.1, 0.2, 0.0, π * 0.36, 4 },
            { "cylinder top, inset 0.2, depth −0.3 (round pocket)", Cylinder, 0.1, 0.1, 0.2, -0.3, π * 0.36 - π * 0.16 * 0.3, 5 },
            { "triangle prism top, inset 0.1 (60° corners), depth −0.2", Triangle, 0.05, 0.05, 0.1, -0.2, TriangleArea * 0.9 - TriangleArea * TriangleScale * TriangleScale * 0.2, 9 },
        };
        for (const Case& Entry : Cases)
        {
            std::string Face;
            SolidArcDocument Document = Prepared(Entry.Request, Entry.X, Entry.Y, Face);
            const SolidArcCommandOutcome Outcome = InsetSolidArcSelection(Document, Entry.Distance, Entry.Depth);
            const double Relative = std::abs(Document.Summary.Volume - Entry.Expected) / Entry.Expected;
            Tally(InsetGate, Relative, Outcome.AcceptedCondition && Document.Summary.SoundCondition && Relative <= 1.0e-9 && Document.Summary.Faces == Entry.Faces,
                  Format("%s on %s: V %.12f vs %.12f · %d faces (expected %d)%s", Entry.Name, Face.c_str(), Document.Summary.Volume, Entry.Expected, Document.Summary.Faces, Entry.Faces,
                         Outcome.AcceptedCondition ? "" : (" — refused: " + Outcome.Refusal).c_str()));
        }

        // ① parameter edit regenerates (depth −0.2 → −0.5), the inner face of the inset takes a second inset, codec byte-equal
        std::string Face;
        SolidArcDocument Edited = Prepared(Block, 1.0, 0.75, Face);
        Expect(InsetSolidArcSelection(Edited, 0.25, -0.2));
        const SolidArcCommandOutcome Deeper = AssignSolidArcParameter(Edited, "Inset1.Depth", -0.5);
        const double DeeperExpected = 3.0 - 1.5 * 1.0 * 0.5;
        const double DeeperRelative = std::abs(Edited.Summary.Volume - DeeperExpected) / DeeperExpected;
        Tally(InsetGate, DeeperRelative, Deeper.AcceptedCondition && DeeperRelative <= 1.0e-9, Format("Inset1.Depth −0.2 → −0.5: V %.12f vs %.12f", Edited.Summary.Volume, DeeperExpected));
        const std::string Floor = PickAndSelect(Edited, RayBetween({ 1.0, 0.75, 5.0 }, { 1.0, 0.75, -5.0 }), false);
        const SolidArcCommandOutcome Again = InsetSolidArcSelection(Edited, 0.1, 0.2);
        const double AgainExpected = DeeperExpected + 1.3 * 0.8 * 0.2;
        const double AgainRelative = std::abs(Edited.Summary.Volume - AgainExpected) / AgainExpected;
        Tally(InsetGate, AgainRelative, Again.AcceptedCondition && AgainRelative <= 1.0e-9 && Edited.Summary.SoundCondition,
              Format("pocket floor %s inset 0.1, boss +0.2: V %.12f vs %.12f", Floor.c_str(), Edited.Summary.Volume, AgainExpected));
        const std::string Saved = EncodeSolidArcDocument(Edited);
        SolidArcDocumentFile File = DecodeSolidArcDocument(Saved);
        SolidArcDocument     Loaded = CreateSolidArcDocument(File.Title);
        const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Loaded, File.Tree, File.Captions, "Open");
        Tally(InsetGate, std::abs(Loaded.Summary.Volume - Edited.Summary.Volume), Opened.AcceptedCondition && EncodeSolidArcDocument(Loaded) == Saved && Loaded.Summary.Volume == Edited.Summary.Volume,
              Format("codec: %d bytes, save → load → save byte-equal (depth kept)", int(Saved.size())));

        // ② refusals leave the document untouched: too wide (1.5 deep face, inset 0.8), a curved face, zero distance
        SolidArcDocument Refused = Prepared(Block, 1.0, 0.75, Face);
        const std::string Before = EncodeSolidArcDocument(Refused);
        const SolidArcCommandOutcome Wide = InsetSolidArcSelection(Refused, 0.8, 0.0);
        const SolidArcCommandOutcome Zero = InsetSolidArcSelection(Refused, 0.0, 0.0);
        SolidArcDocument Round = Prepared(Cylinder, 0.0, 0.0, Face);
        PickAndSelect(Round, RayBetween({ 0.0, 3.0, 0.5 }, { 0.0, 0.0, 0.5 }), false);
        const std::string RoundBefore = EncodeSolidArcDocument(Round);
        const SolidArcCommandOutcome Curved = InsetSolidArcSelection(Round, 0.1, 0.0);
        Tally(InsetGate, 0.0, !Wide.AcceptedCondition && !Zero.AcceptedCondition && !Curved.AcceptedCondition && Curved.Refusal.find("not planar") != std::string::npos && EncodeSolidArcDocument(Refused) == Before && EncodeSolidArcDocument(Round) == RoundBefore,
              "refused, document unchanged: " + Wide.Refusal + " · " + Zero.Refusal + " · " + Curved.Refusal);
    }

    // ---- W42 Loft through drawn sketches (line, polyline, rectangle, circle, slot, spline) + gizmo document verbs
    {
        constexpr double π = std::numbers::pi;
        const auto Format = [](const char* Pattern, auto... Values) { char Text[640]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        // One drawn sketch on the XY grid, lifted by Height with the gizmo verb; returns its step label.
        const auto Drawn = [&](SolidArcDocument& Document, SolidArcDrawingRequest Request, double Height)
        {
            Request.GridPlane = 0;
            Expect(DrawSolidArcSketch(Document, Request));
            const std::string Label = Document.Tree.Steps.back().Label;
            if (Height != 0.0) Expect(PlaceSolidArcSketches(Document, { Label }, TranslationMap({ 0.0, 0.0, Height }), "Move"));
            return Label;
        };
        const auto Circle = [](double Radius)
        {
            SolidArcDrawingRequest Request;
            Request.Kind = SolidArcDrawingKind::Circle; Request.CentreCondition = true; Request.Points = { { 0.0, 0.0 }, { Radius, 0.0 } };
            return Request;
        };
        const auto Rectangle = [](double HalfX, double HalfY)
        {
            SolidArcDrawingRequest Request;
            Request.Kind = SolidArcDrawingKind::Rectangle; Request.CentreCondition = true; Request.Points = { { 0.0, 0.0 }, { HalfX, HalfY } };
            return Request;
        };
        const auto Slot = [](double Half, double Radius)
        {
            SolidArcDrawingRequest Request;
            Request.Kind = SolidArcDrawingKind::Slot; Request.Points = { { -Half, 0.0 }, { Half, 0.0 }, { 0.0, Radius } };
            return Request;
        };
        const auto Frustum = [](double Low, double High, double Height) { return Height / 3.0 * (Low + High + std::sqrt(Low * High)); };
        const double SlotArea = [&](double Half, double Radius) { return 4.0 * Half * Radius + π * Radius * Radius; }(0.5, 0.3);

        // ① closed sections with closed forms: loft (Plasticity L) then Close (plane caps) → solid
        struct Case { const char* Name; SolidArcDrawingRequest Low, High; double Height, Expected; };
        const Case Cases[] = {
            { "circle r 1 → circle r 1 lifted 2 (cylinder 2π)", Circle(1.0), Circle(1.0), 2.0, 2.0 * π },
            { "circle r 1 → circle r 0.5 lifted 1.5 (cone frustum)", Circle(1.0), Circle(0.5), 1.5, Frustum(π, π * 0.25, 1.5) },
            { "rectangle 2 × 1 → rectangle 1 × 0.5 lifted 1 (pyramid frustum)", Rectangle(1.0, 0.5), Rectangle(0.5, 0.25), 1.0, Frustum(2.0, 0.5, 1.0) },
            { "slot (2 lines + 2 arcs) → slot × 0.5 lifted 0.8 (similar sections)", Slot(0.5, 0.3), Slot(0.25, 0.15), 0.8, Frustum(SlotArea, SlotArea * 0.25, 0.8) },
        };
        for (const Case& Entry : Cases)
        {
            SolidArcDocument Document = CreateSolidArcDocument("Loft");
            const std::string First  = Drawn(Document, Entry.Low, 0.0);
            const std::string Second = Drawn(Document, Entry.High, Entry.Height);
            const SolidArcCommandOutcome Loft   = LoftSolidArcCurves(Document, { First, Second });
            const SolidArcCommandOutcome Closed = CloseSolidArcPanels(Document);
            const double Relative = std::abs(Document.Summary.Volume - Entry.Expected) / Entry.Expected;
            Tally(SectionLoftGate, Relative, Loft.AcceptedCondition && Closed.AcceptedCondition && Document.Summary.SoundCondition && Relative <= 1.0e-9,
                  Format("%s: %s → %s, V %.12f vs %.12f%s", Entry.Name, First.c_str(), Second.c_str(), Document.Summary.Volume, Entry.Expected,
                         Loft.AcceptedCondition ? (Closed.AcceptedCondition ? "" : (" — close refused: " + Closed.Refusal).c_str()) : (" — loft refused: " + Loft.Refusal).c_str()));
        }

        // ② rectangle → circle (no closed form): sound, 3 faces, between the circle and rectangle prisms; then the top
        //    section moved up 1 with the gizmo verb regenerates the loft (volume scales with the height for parallel sections)
        {
            SolidArcDocument Document = CreateSolidArcDocument("Loft");
            const std::string Square = Drawn(Document, Rectangle(1.0, 1.0), 0.0);
            const std::string Round  = Drawn(Document, Circle(1.0), 1.0);
            const SolidArcCommandOutcome Loft   = LoftSolidArcCurves(Document, { Square, Round });
            const SolidArcCommandOutcome Closed = CloseSolidArcPanels(Document);
            const double Volume = Document.Summary.Volume;
            Tally(SectionLoftGate, 0.0, Loft.AcceptedCondition && Closed.AcceptedCondition && Document.Summary.SoundCondition && Document.Summary.Faces == 3 && Volume > π && Volume < 4.0,
                  Format("square 2 × 2 → circle r 1 lifted 1: sound %d, %d faces, π < V %.9f < 4%s", int(Document.Summary.SoundCondition), Document.Summary.Faces, Volume,
                         Loft.AcceptedCondition ? "" : (" — refused: " + Loft.Refusal).c_str()));
            const SolidArcCommandOutcome Lifted = PlaceSolidArcSketches(Document, { Round }, TranslationMap({ 0.0, 0.0, 1.0 }), "Move");
            const double Relative = std::abs(Document.Summary.Volume - 2.0 * Volume) / (2.0 * Volume);
            Tally(SectionLoftGate, Relative, Lifted.AcceptedCondition && Relative <= 1.0e-9 && Document.Summary.SoundCondition,
                  Format("gizmo move of %s by +1 z regenerates the loft: V %.12f vs 2 × %.12f", Round.c_str(), Document.Summary.Volume, Volume));
            // codec: sketch sections and their moved planes survive save → load → save
            const std::string Saved = EncodeSolidArcDocument(Document);
            SolidArcDocumentFile File = DecodeSolidArcDocument(Saved);
            SolidArcDocument     Loaded = CreateSolidArcDocument(File.Title);
            const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Loaded, File.Tree, File.Captions, "Open");
            Tally(SectionLoftGate, std::abs(Loaded.Summary.Volume - Document.Summary.Volume), Opened.AcceptedCondition && EncodeSolidArcDocument(Loaded) == Saved && Loaded.Summary.Volume == Document.Summary.Volume,
                  Format("codec: %d bytes, save → load → save byte-equal, same volume", int(Saved.size())));
        }

        // ③ open sections: a line → a 3-point polyline lifted 1 → an open sheet; a closed spline (4 points) → a circle
        {
            SolidArcDocument Document = CreateSolidArcDocument("Loft");
            SolidArcDrawingRequest Line;
            Line.Kind = SolidArcDrawingKind::Line; Line.Points = { { -1.0, 0.0 }, { 1.0, 0.0 } };
            SolidArcDrawingRequest Polyline;
            Polyline.Kind = SolidArcDrawingKind::Line; Polyline.Points = { { -1.0, 0.5 }, { 0.0, 1.0 }, { 1.0, 0.5 } };
            const std::string Bottom = Drawn(Document, Line, 0.0);
            const std::string Top    = Drawn(Document, Polyline, 1.0);
            const SolidArcCommandOutcome Loft = LoftSolidArcCurves(Document, { Bottom, Top });
            Tally(SectionLoftGate, 0.0, Loft.AcceptedCondition && Document.Regeneration.Panels.size() == 1,
                  Format("line %s → polyline %s (2 segments) lifted 1: open panel, %d panel(s)%s", Bottom.c_str(), Top.c_str(), int(Document.Regeneration.Panels.size()),
                         Loft.AcceptedCondition ? "" : (" — refused: " + Loft.Refusal).c_str()));

            SolidArcDocument Spline = CreateSolidArcDocument("Loft");
            SolidArcDrawingRequest Curve;
            Curve.Kind = SolidArcDrawingKind::Curve; Curve.ClosedCondition = true; Curve.Points = { { 1.0, 0.0 }, { 0.0, 0.8 }, { -1.0, 0.0 }, { 0.0, -0.8 } };
            const std::string Freeform = Drawn(Spline, Curve, 0.0);
            const std::string Disc     = Drawn(Spline, Circle(0.6), 1.2);
            const SolidArcCommandOutcome Skin   = LoftSolidArcCurves(Spline, { Freeform, Disc });
            const SolidArcCommandOutcome Closed = CloseSolidArcPanels(Spline);
            Tally(SectionLoftGate, 0.0, Skin.AcceptedCondition && Closed.AcceptedCondition && Spline.Summary.SoundCondition && Spline.Summary.Volume > 0.0,
                  Format("closed spline (4 points) → circle r 0.6 lifted 1.2: closed solid, V %.9f, sound %d%s", Spline.Summary.Volume, int(Spline.Summary.SoundCondition),
                         Skin.AcceptedCondition ? (Closed.AcceptedCondition ? "" : (" — close refused: " + Closed.Refusal).c_str()) : (" — refused: " + Skin.Refusal).c_str()));
        }

        // ④ refusals leave the document untouched: open + closed mixed, a scaled or mirrored sketch, an unknown label
        {
            SolidArcDocument Document = CreateSolidArcDocument("Loft");
            SolidArcDrawingRequest Line;
            Line.Kind = SolidArcDrawingKind::Line; Line.Points = { { -1.0, 0.0 }, { 1.0, 0.0 } };
            const std::string Open   = Drawn(Document, Line, 0.0);
            const std::string Closed = Drawn(Document, Circle(1.0), 1.0);
            const std::string Before = EncodeSolidArcDocument(Document);
            const SolidArcCommandOutcome Mixed    = LoftSolidArcCurves(Document, { Open, Closed });
            const SolidArcCommandOutcome Scaled   = PlaceSolidArcSketches(Document, { Closed }, ScalingMap({ 0.0, 0.0, 0.0 }, { 2.0, 2.0, 2.0 }), "Scale");
            const SolidArcCommandOutcome Mirrored = PlaceSolidArcSketches(Document, { Closed }, ReflectionMap({ 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }), "Mirror");
            const SolidArcCommandOutcome Unknown  = PlaceSolidArcSketches(Document, { "Nothing#9" }, TranslationMap({ 0.0, 0.0, 1.0 }), "Move");
            Tally(SectionLoftGate, 0.0, !Mixed.AcceptedCondition && !Scaled.AcceptedCondition && !Mirrored.AcceptedCondition && !Unknown.AcceptedCondition && EncodeSolidArcDocument(Document) == Before,
                  "refused, document unchanged: " + Mixed.Refusal + " · " + Scaled.Refusal + " · " + Mirrored.Refusal + " · " + Unknown.Refusal);
            // a quarter turn about x through the origin carries the plane exactly: x̂ stays, ẑ → −ŷ
            const SolidArcCommandOutcome Turned = PlaceSolidArcSketches(Document, { Closed }, RotationMap({ 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, π / 2.0), "Rotate");
            const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Closed; });
            const double Gap = Found == Document.Tree.Steps.end() ? 1.0 : Length(Found->Plane.XAxis - Vector3{ 1.0, 0.0, 0.0 }) + Length(Found->Plane.ZAxis - Vector3{ 0.0, -1.0, 0.0 })
                                                                          + Length(Found->Plane.Origin - Vector3{ 0.0, -1.0, 0.0 });
            Tally(SectionLoftGate, Gap, Turned.AcceptedCondition && Gap <= 1.0e-12, Format("gizmo rotate of %s 90° about x: plane x̂ (1, 0, 0), ẑ (0, −1, 0), origin (0, −1, 0), gap %.3g", Closed.c_str(), Gap));
        }

        // ⑤ face gizmo verbs on a block 2 × 1.5 × 1: tweak along a direction, rotate about an axis, scale, push / pull (offset)
        {
            const auto Block = [&](const OrbitRay& Ray, std::string& Face)
            {
                SolidArcDocument Document = CreateSolidArcDocument("Gizmo");
                SolidArcConstructRequest Request;
                Request.Kind = SolidArcConstructKind::Block; Request.Width = 2.0; Request.Depth = 1.5; Request.Height = 1.0; Request.Combination = FeatureCombination::Separate;
                Expect(ConstructSolidArcPrimitive(Document, Request));
                Face = PickAndSelect(Document, Ray, false);
                return Document;
            };
            const OrbitRay Side = RayBetween({ 5.0, 0.75, 0.5 }, { 0.0, 0.75, 0.5 });
            const OrbitRay Top  = RayBetween({ 1.0, 0.75, 5.0 }, { 1.0, 0.75, 0.0 });
            struct Verb { const char* Name; OrbitRay Ray; FaceOperationKind Operation; double Amount; std::optional<Vector3> Direction; double Expected; };
            const Verb Verbs[] = {
                { "side x = 2 pulled +0.3 along its normal (offset)", Side, FaceOperationKind::Offset, 0.3, std::nullopt, 3.0 + 1.5 * 0.3 },
                { "side x = 2 tweaked along (1, 0, 1) by 0.2√2 (x moves 0.2)", Side, FaceOperationKind::Tweak, 0.2 * std::sqrt(2.0), Vector3{ 1.0, 0.0, 1.0 }, 3.0 + 1.5 * 0.2 },
                { "top rotated 0.1 rad about x through its centre (volume kept)", Top, FaceOperationKind::Rotate, 0.1, Vector3{ 1.0, 0.0, 0.0 }, 3.0 },
                { "top scaled × 0.5 about its centre (pyramid frustum)", Top, FaceOperationKind::Scale, 0.5, std::nullopt, Frustum(3.0 / 1.0, 0.75, 1.0) },
            };
            std::string Saved;
            for (const Verb& Entry : Verbs)
            {
                std::string Face;
                SolidArcDocument Document = Block(Entry.Ray, Face);
                const SolidArcCommandOutcome Outcome = ChangeSolidArcSelection(Document, Entry.Operation, Entry.Amount, Entry.Direction);
                const double Relative = std::abs(Document.Summary.Volume - Entry.Expected) / Entry.Expected;
                Tally(SectionLoftGate, Relative, Outcome.AcceptedCondition && Document.Summary.SoundCondition && Relative <= 1.0e-9,
                      Format("%s on %s: V %.12f vs %.12f%s", Entry.Name, Face.c_str(), Document.Summary.Volume, Entry.Expected, Outcome.AcceptedCondition ? "" : (" — refused: " + Outcome.Refusal).c_str()));
                if (Entry.Operation == FaceOperationKind::Tweak)
                {
                    Saved = EncodeSolidArcDocument(Document);
                    SolidArcDocumentFile File = DecodeSolidArcDocument(Saved);
                    SolidArcDocument     Loaded = CreateSolidArcDocument(File.Title);
                    const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Loaded, File.Tree, File.Captions, "Open");
                    Tally(SectionLoftGate, std::abs(Loaded.Summary.Volume - Document.Summary.Volume), Opened.AcceptedCondition && EncodeSolidArcDocument(Loaded) == Saved && Loaded.Summary.Volume == Document.Summary.Volume
                          && Saved.find("Points") != std::string::npos, "codec: the tweak direction is saved (Points) and replays byte-equal");
                }
            }
        }
    }

    // ---- W43 Transform gizmo: the CPU triangles the GPU draws are the ones picked; drag maths from pixel rays
    {
        const auto Format = [](const char* Pattern, auto... Values) { char Text[640]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        const OrbitCameraFrame Frame = FrameOrbitCamera(OrbitCamera{}, 1600.0, 900.0);
        const auto Project = [&](Vector3 Point, double& X, double& Y)
        {
            double Clip[4] = {};
            for (int Row = 0; Row < 4; ++Row)
                Clip[Row] = Frame.ClipFromWorld[Row] * Point.X + Frame.ClipFromWorld[4 + Row] * Point.Y + Frame.ClipFromWorld[8 + Row] * Point.Z + Frame.ClipFromWorld[12 + Row];
            X = (Clip[0] / Clip[3] * 0.5 + 0.5) * Frame.Width;
            Y = (Clip[1] / Clip[3] * 0.5 + 0.5) * Frame.Height;
        };
        struct ModeSet { uint32_t Modes; const char* Name; };
        const ModeSet Sets[] = { { GizmoTranslate, "T" }, { GizmoRotate, "R" }, { GizmoScale, "S" }, { GizmoTranslate | GizmoRotate, "T+R" },
                                 { GizmoRotate | GizmoScale, "R+S" }, { GizmoTranslate | GizmoRotate | GizmoScale, "T+R+S" } };
        for (const ModeSet& Set : Sets)
        {
            SolidArcGizmoPose Pose;
            Pose.Modes = Set.Modes;
            Pose.Size  = MeasureGizmoSize(Frame, Pose.Origin, GizmoPixels);
            const SolidArcGizmoGeometry Geometry = BuildSolidArcGizmo(Pose, Frame, SolidArcGizmoHandle::None, SolidArcGizmoHandle::None);
            // 📝 Slate proportions (L = 1, tip 0.95): cone at the tip, cylinder 0.28 inside, plane quad at (u + v)(tip − 0.08),
            //    arc on the 45° bisector at 0.62 · tip.
            const double Tip = 0.95, Arc = 0.62 * Tip / std::sqrt(2.0), Size = Pose.Size;
            struct Probe { Vector3 Point; SolidArcGizmoHandle Handle; uint32_t Mode; };
            const Probe Probes[] = {
                { { Tip * Size, 0.0, 0.0 }, SolidArcGizmoHandle::TranslateX, GizmoTranslate }, { { 0.0, Tip * Size, 0.0 }, SolidArcGizmoHandle::TranslateY, GizmoTranslate },
                { { 0.0, 0.0, Tip * Size }, SolidArcGizmoHandle::TranslateZ, GizmoTranslate },
                { { (Tip - 0.08) * Size, (Tip - 0.08) * Size, 0.0 }, SolidArcGizmoHandle::PlaneXY, GizmoTranslate },
                // 📝 In the exact isometric view the x–z quad centre lies on the eye ray through the y cone (their offset
                //    (0.87, −0.95, 0.87) runs along the view axis), and the opaque cone rightly wins; probe the quad's free corner.
                { { (Tip - 0.03) * Size, 0.0, (Tip - 0.13) * Size }, SolidArcGizmoHandle::PlaneXZ, GizmoTranslate },
                { { (Tip - 0.28) * Size, 0.0, 0.0 }, SolidArcGizmoHandle::ScaleX, GizmoScale }, { { 0.0, (Tip - 0.28) * Size, 0.0 }, SolidArcGizmoHandle::ScaleY, GizmoScale },
                { { 0.0, 0.0, (Tip - 0.28) * Size }, SolidArcGizmoHandle::ScaleZ, GizmoScale },
                { { Arc * Size, Arc * Size, 0.0 }, SolidArcGizmoHandle::RotateZ, GizmoRotate }, { { 0.0, Arc * Size, Arc * Size }, SolidArcGizmoHandle::RotateX, GizmoRotate },
                { { Arc * Size, 0.0, Arc * Size }, SolidArcGizmoHandle::RotateY, GizmoRotate } };
            int Picks = 0, Misses = 0;
            std::string Wrong;
            for (const Probe& Entry : Probes)
            {
                if (!(Set.Modes & Entry.Mode)) continue;
                double X = 0.0, Y = 0.0;
                Project(Entry.Point, X, Y);
                const SolidArcGizmoHandle Found = PickSolidArcGizmo(Geometry, Frame, X, Y, 6.0);
                ++Picks;
                if (Found != Entry.Handle) { ++Misses; Wrong += std::string(" ") + GizmoHandleLabel(Entry.Handle) + "→" + GizmoHandleLabel(Found); }
            }
            double FarX = 0.0, FarY = 0.0;
            Project({ 0.0, 0.0, 0.0 }, FarX, FarY);
            const bool Far = PickSolidArcGizmo(Geometry, Frame, FarX + 400.0, FarY, 6.0) == SolidArcGizmoHandle::None;
            Tally(GizmoGate, double(Misses), Misses == 0 && Far && Geometry.OpaqueCount <= Geometry.Vertices.size() && !Geometry.Vertices.empty(),
                  Format("%s: %d handle(s) picked at their projected centres, far pixel misses, %zu vertices (%u opaque)%s", Set.Name, Picks, Geometry.Vertices.size(), Geometry.OpaqueCount, Wrong.c_str()));
        }
        double Worst = 0.0;
        for (const double Parameter : { 0.3, 1.7, -0.4 })
        {
            double X = 0.0, Y = 0.0;
            Project({ 0.0, 0.0, Parameter }, X, Y);
            const std::optional<double> Found = MeasureAxisParameter(CastOrbitRay(Frame, X, Y), { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 });
            Worst = std::max(Worst, Found ? std::abs(*Found - Parameter) : 1.0);
        }
        Tally(GizmoGate, Worst, Worst <= 1.0e-9, Format("axis drag: pointer ray → parameter on the z axis (0.3, 1.7, −0.4), worst |Δt| %.3g", Worst));
        Worst = 0.0;
        for (const double Angle : { 0.3, 2.0, -1.0 })
        {
            double X = 0.0, Y = 0.0;
            Project({ 0.0, std::cos(Angle), std::sin(Angle) }, X, Y);
            const std::optional<double> Found = MeasureGizmoAngle(CastOrbitRay(Frame, X, Y), { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 });
            Worst = std::max(Worst, Found ? std::abs(*Found - Angle) : 1.0);
        }
        Tally(GizmoGate, Worst, Worst <= 1.0e-9, Format("rotate drag: pointer ray → angle about x from y (0.3, 2.0, −1.0 rad), worst |Δθ| %.3g", Worst));
    }

    // ---- W44 Mode-1 sketch points: DragSolidArcSketchPoint / DragSolidArcSketchPoints carry the steps built on the sketch
    {
        using Kind = SolidArcDrawingKind;
        const auto Format = [](const char* Pattern, auto... Values) { char Text[640]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        const auto Gap    = [](Vector2 Point, Vector2 Expected) { return std::abs(Point.X - Expected.X) + std::abs(Point.Y - Expected.Y); };
        const auto Solved = [](const SolidArcDocument& Document, const std::string& Label) -> const SketchModel& { return Document.Regeneration.Sketches.at(Label); };

        // ① a 2 × 1 rectangle extruded 1: the corner (2, 1) dragged to (3, 1) (one point: the direct drag and the gizmo agree)
        SolidArcDocument Plate = CreateSolidArcDocument("Points");
        Expect(DrawSolidArcSketch(Plate, DrawingOf(Kind::Rectangle, 0, { { 0.0, 0.0 }, { 2.0, 1.0 } })));
        const std::string Sketch = Plate.Tree.Steps.back().Label;
        Expect(ExtrudeSolidArcSketch(Plate, Sketch, 1.0, FeatureCombination::New));
        const size_t History = Plate.UndoEntries.size();
        const SolidArcCommandOutcome One = DragSolidArcSketchPoints(Plate, Sketch, { 2 }, { { 3.0, 1.0 } });
        const SketchModel& AfterOne = Solved(Plate, Sketch);
        double Miss = Gap(AfterOne.Points[2].Position, { 3.0, 1.0 }) + Gap(AfterOne.Points[1].Position, { 3.0, 0.0 }) + Gap(AfterOne.Points[0].Position, { 0.0, 0.0 });
        double Relative = std::abs(Plate.Summary.Volume - 3.0) / 3.0;
        Tally(PointGate, Miss + Relative, One.AcceptedCondition && Miss <= 1.0e-9 && Relative <= 1.0e-9 && Plate.UndoEntries.size() == History + 1,
              Format("%s: corner (3, 1), its H / V neighbour follows to (3, 0), V %.12f vs 3, one undo entry", One.Label.c_str(), Plate.Summary.Volume));

        // ② both top corners moved up 0.5 together (the gizmo's xy square on two picked points) → 3 × 1.5 × 1
        const SolidArcCommandOutcome Two = DragSolidArcSketchPoints(Plate, Sketch, { 2, 3 }, { { 3.0, 1.5 }, { 0.0, 1.5 } });
        const SketchModel& AfterTwo = Solved(Plate, Sketch);
        Miss     = Gap(AfterTwo.Points[2].Position, { 3.0, 1.5 }) + Gap(AfterTwo.Points[3].Position, { 0.0, 1.5 }) + Gap(AfterTwo.Points[1].Position, { 3.0, 0.0 });
        Relative = std::abs(Plate.Summary.Volume - 4.5) / 4.5;
        Tally(PointGate, Miss + Relative, Two.AcceptedCondition && Miss <= 1.0e-9 && Relative <= 1.0e-9 && Plate.UndoEntries.size() == History + 2 && Plate.Summary.SoundCondition,
              Format("%s: (3, 1.5) and (0, 1.5), V %.12f vs 4.5, one undo entry", Two.Label.c_str(), Plate.Summary.Volume));

        // ③ codec: the dragged positions survive save → load → save byte-equal
        const std::string Saved = EncodeSolidArcDocument(Plate);
        SolidArcDocumentFile File   = DecodeSolidArcDocument(Saved);
        SolidArcDocument     Loaded = CreateSolidArcDocument(File.Title);
        const SolidArcCommandOutcome Opened = ReplaceSolidArcTree(Loaded, File.Tree, File.Captions, "Open");
        Tally(PointGate, std::abs(Loaded.Summary.Volume - Plate.Summary.Volume), Opened.AcceptedCondition && EncodeSolidArcDocument(Loaded) == Saved && Loaded.Summary.Volume == Plate.Summary.Volume,
              Format("codec: %d bytes, save → load → save byte-equal, same volume", int(Saved.size())));

        // ④ refusals leave the file and the history untouched
        const SolidArcCommandOutcome Unpaired = DragSolidArcSketchPoints(Plate, Sketch, { 0, 1 }, { { 1.0, 1.0 } });
        const SolidArcCommandOutcome Missing  = DragSolidArcSketchPoints(Plate, "Sketch#99", { 0, 1 }, { { 1.0, 1.0 }, { 2.0, 2.0 } });
        const SolidArcCommandOutcome Beyond   = DragSolidArcSketchPoints(Plate, Sketch, { 0, 40 }, { { 1.0, 1.0 }, { 2.0, 2.0 } });
        Tally(PointGate, 0.0, !Unpaired.AcceptedCondition && !Missing.AcceptedCondition && !Beyond.AcceptedCondition && EncodeSolidArcDocument(Plate) == Saved && Plate.UndoEntries.size() == History + 2,
              "refused: " + Unpaired.Refusal + " · " + Missing.Refusal + " · " + Beyond.Refusal + "; file and history untouched");

        // ⑤ Bezier poles (no constraints) turned 30° about their centre: every pole lands exactly (the gizmo z arc)
        {
            SolidArcDocument Curves = CreateSolidArcDocument("Points");
            const std::vector<Vector2> Poles = { { 0.0, 0.0 }, { 0.5, 1.0 }, { 1.5, 1.0 }, { 2.0, 0.0 } };
            Expect(DrawSolidArcSketch(Curves, DrawingOf(Kind::Curve, 0, Poles, false, false, SketchSplineForm::Bezier)));
            const std::string Label  = Curves.Tree.Steps.back().Label;
            const SketchModel Before = Solved(Curves, Label);
            std::vector<int>     Picked;
            std::vector<Vector2> Targets;
            Vector2 Centre;
            for (int Point = 0; Point < int(Before.Points.size()); ++Point) { Centre.X += Before.Points[Point].Position.X / double(Before.Points.size()); Centre.Y += Before.Points[Point].Position.Y / double(Before.Points.size()); }
            const double Turn = std::numbers::pi / 6.0;
            for (int Point = 0; Point < int(Before.Points.size()); ++Point)
            {
                const Vector2 Arm = { Before.Points[Point].Position.X - Centre.X, Before.Points[Point].Position.Y - Centre.Y };
                Picked.push_back(Point);
                Targets.push_back({ Centre.X + std::cos(Turn) * Arm.X - std::sin(Turn) * Arm.Y, Centre.Y + std::sin(Turn) * Arm.X + std::cos(Turn) * Arm.Y });
            }
            const SolidArcCommandOutcome Turned = DragSolidArcSketchPoints(Curves, Label, Picked, Targets);
            double Worst = 0.0;
            for (size_t Pick = 0; Pick < Picked.size(); ++Pick) Worst = std::max(Worst, Gap(Solved(Curves, Label).Points[Picked[Pick]].Position, Targets[Pick]));
            Tally(PointGate, Worst, Turned.AcceptedCondition && Worst <= 1.0e-9 && Before.Points.size() == 4,
                  Format("%s: %d Bezier poles turned 30° about (%.3f, %.3f), worst miss %.3g", Turned.Label.c_str(), int(Picked.size()), Centre.X, Centre.Y, Worst));
        }

        // ⑥ a closed triangle (0, 0) (2, 0) (1, 1.5) extruded 1 (V 1.5), its points scaled × 2 along x about (1, 0.5) → V 3
        {
            SolidArcDocument Prism = CreateSolidArcDocument("Points");
            Expect(DrawSolidArcSketch(Prism, DrawingOf(Kind::Line, 0, { { 0.0, 0.0 }, { 2.0, 0.0 }, { 1.0, 1.5 } }, true)));
            const std::string Label = Prism.Tree.Steps.back().Label;
            Expect(ExtrudeSolidArcSketch(Prism, Label, 1.0, FeatureCombination::New));
            const double      Low    = Prism.Summary.Volume;
            const SketchModel Before = Solved(Prism, Label);
            std::vector<int>     Picked;
            std::vector<Vector2> Targets;
            for (int Point = 0; Point < int(Before.Points.size()); ++Point)
            {
                Picked.push_back(Point);
                Targets.push_back({ 1.0 + 2.0 * (Before.Points[Point].Position.X - 1.0), Before.Points[Point].Position.Y });
            }
            const SolidArcCommandOutcome Scaled = DragSolidArcSketchPoints(Prism, Label, Picked, Targets);
            Relative = std::abs(Prism.Summary.Volume - 3.0) / 3.0;
            Tally(PointGate, Relative + std::abs(Low - 1.5), Scaled.AcceptedCondition && Relative <= 1.0e-9 && std::abs(Low - 1.5) <= 1.0e-9 && Prism.Summary.SoundCondition,
                  Format("%s: %d points × 2 along x, V %.12f → %.12f vs 3", Scaled.Label.c_str(), int(Picked.size()), Low, Prism.Summary.Volume));
        }
    }

    // ---- W45 Solid loft shells, sketches drawn on body faces, hide / lock of sketches and bodies
    {
        constexpr double π = std::numbers::pi;
        const auto Format  = [](const char* Pattern, auto... Values) { char Text[640]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        const auto Frustum = [](double Low, double High, double Height) { return Height / 3.0 * (Low + High + std::sqrt(Low * High)); };
        const auto Disc    = [](SolidArcDocument& Document, Vector2 Centre, double Radius, double Height)
        {
            SolidArcDrawingRequest Request;
            Request.Kind = SolidArcDrawingKind::Circle; Request.CentreCondition = true; Request.Points = { Centre, { Centre.X + Radius, Centre.Y } };
            Expect(DrawSolidArcSketch(Document, Request));
            const std::string Label = Document.Tree.Steps.back().Label;
            if (Height != 0.0) Expect(PlaceSolidArcSketches(Document, { Label }, TranslationMap({ 0.0, 0.0, Height }), "Move"));
            return Label;
        };

        // ① closed sections: circle r 1 → circle r 0.5 lifted 1.5 lofts a capped solid of its own (no Close step)
        SolidArcDocument Shells = CreateSolidArcDocument("Shells");
        const std::string Low  = Disc(Shells, { 0.0, 0.0 }, 1.0, 0.0);
        const std::string High = Disc(Shells, { 0.0, 0.0 }, 0.5, 1.5);
        const SolidArcCommandOutcome First = LoftSolidArcCurves(Shells, { Low, High }, 0, true);
        const double Cone     = Frustum(π, 0.25 * π, 1.5);
        double       Relative = std::abs(Shells.Summary.Volume - Cone) / Cone;
        Tally(ShellGate, Relative, First.AcceptedCondition && Relative <= 1.0e-9 && Shells.Summary.SoundCondition && !Shells.Summary.SheetCondition && Shells.Summary.Faces == 3 && Shells.Summary.Bodies.size() == 1,
              Format("%s: V %.12f vs frustum %.12f, %d faces (skin + 2 caps), sound, 1 body", First.Label.c_str(), Shells.Summary.Volume, Cone, Shells.Summary.Faces));

        // ② a second solid loft in the same file (beside x = 4): two bodies, volumes add (a panel Close allows only one)
        const std::string Left  = Disc(Shells, { 4.0, 0.0 }, 1.0, 0.0);
        const std::string Right = Disc(Shells, { 4.0, 0.0 }, 1.0, 2.0);
        const SolidArcCommandOutcome Second = LoftSolidArcCurves(Shells, { Left, Right }, 0, true);
        const double Sum = Cone + 2.0 * π;
        Relative = std::abs(Shells.Summary.Volume - Sum) / Sum;
        Tally(ShellGate, Relative, Second.AcceptedCondition && Relative <= 1.0e-9 && Shells.Summary.Bodies.size() == 2 && Shells.Summary.SoundCondition,
              Format("%s: 2 bodies, V %.12f vs frustum + 2π %.12f", Second.Label.c_str(), Shells.Summary.Volume, Sum));

        // ③ codec: Closed = true on the solid lofts survives save → load → save byte-equal
        {
            const std::string          Written = EncodeSolidArcDocument(Shells);
            const SolidArcDocumentFile Read    = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            Tally(ShellGate, std::abs(Copy.Summary.Volume - Shells.Summary.Volume), Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Summary.Volume == Shells.Summary.Volume
                  && Written.find("Closed = true") != std::string::npos, Format("codec: %d bytes byte-equal, same volume, solid lofts carry Closed = true", int(Written.size())));
        }

        // ④ open sections (two lines) refuse the solid with the file and history untouched, then loft a sheet
        {
            SolidArcDocument Open = CreateSolidArcDocument("Shells");
            Expect(DrawSolidArcSketch(Open, DrawingOf(SolidArcDrawingKind::Line, 0, { { 0.0, 0.0 }, { 2.0, 0.0 } })));
            const std::string Near = Open.Tree.Steps.back().Label;
            Expect(DrawSolidArcSketch(Open, DrawingOf(SolidArcDrawingKind::Line, 0, { { 0.0, 1.0 }, { 2.0, 1.0 } })));
            const std::string Far = Open.Tree.Steps.back().Label;
            Expect(PlaceSolidArcSketches(Open, { Far }, TranslationMap({ 0.0, 0.0, 1.0 }), "Move"));
            const std::string Before  = EncodeSolidArcDocument(Open);
            const size_t      History = Open.UndoEntries.size();
            const SolidArcCommandOutcome Solid = LoftSolidArcCurves(Open, { Near, Far }, 0, true);
            const bool Untouched = EncodeSolidArcDocument(Open) == Before && Open.UndoEntries.size() == History;
            const SolidArcCommandOutcome Sheet = LoftSolidArcCurves(Open, { Near, Far });
            Tally(ShellGate, 0.0, !Solid.AcceptedCondition && Untouched && Sheet.AcceptedCondition && Open.Summary.SheetCondition,
                  "open sections: solid refused (" + Solid.Refusal + "), file untouched; the plain loft makes a sheet");
        }

        // ⑤ sketch on a face: a 2 × 2 × 1 block; a 1 × 1 square on its top face (z = 1) and a 1 × 0.5 one on its x = 2 face,
        //    each extruded 0.5 onto the block → V 4 + 0.5 + 0.25; a skewed frame refuses atomically
        {
            SolidArcDocument Block = CreateSolidArcDocument("Faces");
            Expect(DrawSolidArcSketch(Block, DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.0, 0.0 }, { 2.0, 2.0 } })));
            Expect(ExtrudeSolidArcSketch(Block, Block.Tree.Steps.back().Label, 1.0, FeatureCombination::New));
            SolidArcDrawingRequest Top = DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.5, 0.5 }, { 1.5, 1.5 } });
            Top.PlaneCondition = true;
            Top.Plane          = { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } };
            const SolidArcCommandOutcome OnTop = DrawSolidArcSketch(Block, Top);
            const std::string TopLabel = Block.Tree.Steps.back().Label;
            const SolidArcCommandOutcome Raised = ExtrudeSolidArcSketch(Block, TopLabel, 0.5, FeatureCombination::Union);
            Relative = std::abs(Block.Summary.Volume - 4.5) / 4.5;
            Tally(ShellGate, Relative, OnTop.AcceptedCondition && Raised.AcceptedCondition && Relative <= 1.0e-9 && Block.Summary.SoundCondition && Block.Tree.Steps[Block.Tree.Steps.size() - 2].Plane.Origin.Z == 1.0,
                  Format("%s on the top face (plane z = 1) → %s: V %.12f vs 4.5", OnTop.Label.c_str(), Raised.Label.c_str(), Block.Summary.Volume));
            SolidArcDrawingRequest Side = DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.5, 0.25 }, { 1.5, 0.75 } });
            Side.PlaneCondition = true;
            Side.Plane          = { { 2.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } };   // x̂ = ẑ × n̂, ŷ = n̂ × x̂
            const SolidArcCommandOutcome OnSide = DrawSolidArcSketch(Block, Side);
            const SolidArcCommandOutcome Out    = ExtrudeSolidArcSketch(Block, Block.Tree.Steps.back().Label, 0.5, FeatureCombination::Union);
            Relative = std::abs(Block.Summary.Volume - 4.75) / 4.75;
            Tally(ShellGate, Relative, OnSide.AcceptedCondition && Out.AcceptedCondition && Relative <= 1.0e-9 && Block.Summary.SoundCondition,
                  Format("%s on the x = 2 face (n̂ = x̂) → %s: V %.12f vs 4.75", OnSide.Label.c_str(), Out.Label.c_str(), Block.Summary.Volume));
            const std::string Before = EncodeSolidArcDocument(Block);
            SolidArcDrawingRequest Skewed = Side;
            Skewed.Plane.XAxis = { 0.0, 1.0, 0.1 };
            const SolidArcCommandOutcome Refused = DrawSolidArcSketch(Block, Skewed);
            Tally(ShellGate, 0.0, !Refused.AcceptedCondition && EncodeSolidArcDocument(Block) == Before, "skewed face frame refused, file untouched: " + Refused.Refusal);
        }

        // ⑥ hide / lock: Outliner eye on a sketch, lock on a sketch and on a body — undoable appearance entries, codec byte-equal
        {
            SolidArcDocument Figures = CreateSolidArcDocument("Figures");
            Expect(DrawSolidArcSketch(Figures, DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.0, 0.0 }, { 1.0, 1.0 } })));
            const std::string Plate = Figures.Tree.Steps.back().Label;
            Expect(ExtrudeSolidArcSketch(Figures, Plate, 1.0, FeatureCombination::New));
            const std::string Body = Figures.Summary.Bodies.front().Key;
            Expect(DrawSolidArcSketch(Figures, DrawingOf(SolidArcDrawingKind::Circle, 0, { { 3.0, 0.0 }, { 3.5, 0.0 } }, false, true)));
            const std::string Round = Figures.Tree.Steps.back().Label;
            SelectSolidArcCurve(Figures, Round, false);
            SelectSolidArcName(Figures, SolidArcSelectionKind::Body, Body, true);
            const double Volume  = Figures.Summary.Volume;
            const size_t History = Figures.UndoEntries.size();
            const SolidArcCommandOutcome Hide     = ToggleSolidArcCurveVisibility(Figures, Round);
            const SolidArcCommandOutcome LockOne  = ToggleSolidArcLock(Figures, Plate);
            const SolidArcCommandOutcome LockBody = ToggleSolidArcLock(Figures, Body);
            const bool Applied = Hide.AcceptedCondition && LockOne.AcceptedCondition && LockBody.AcceptedCondition && SolidArcCurveHiddenCondition(Figures, Round)
                              && SolidArcLockedCondition(Figures, Plate) && SolidArcLockedCondition(Figures, Body) && Figures.Selection.Curves.empty() && Figures.Selection.Bodies.empty()
                              && Figures.UndoEntries.size() == History + 3 && Figures.Summary.Volume == Volume;
            Tally(ShellGate, 0.0, Applied, "hide " + Round + ", lock " + Plate + " and body " + Body + ": 3 undo entries, both leave the selection, geometry unchanged");
            const std::string          Written = EncodeSolidArcDocument(Figures);
            const SolidArcDocumentFile Read    = DecodeSolidArcDocument(Written);
            SolidArcDocument Copy = CreateSolidArcDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Copy, true);
            Tally(ShellGate, 0.0, Read.Refusal.empty() && EncodeSolidArcDocument(Copy) == Written && Copy.Appearance.HiddenCurves == Figures.Appearance.HiddenCurves
                  && Copy.Appearance.LockedFigures == Figures.Appearance.LockedFigures && Written.find("HiddenCurves") != std::string::npos && Written.find("Locked") != std::string::npos,
                  Format("codec: HiddenCurves %d · Locked %d survive save → load → save byte-equal", int(Copy.Appearance.HiddenCurves.size()), int(Copy.Appearance.LockedFigures.size())));
            const bool Undone = UndoSolidArcCommand(Figures) && UndoSolidArcCommand(Figures) && UndoSolidArcCommand(Figures);
            const SolidArcCommandOutcome Unknown = ToggleSolidArcLock(Figures, "Sketch#99");
            Tally(ShellGate, 0.0, Undone && Figures.Appearance.HiddenCurves.empty() && Figures.Appearance.LockedFigures.empty() && !Unknown.AcceptedCondition && Figures.UndoEntries.size() == History,
                  "three undos clear hide and locks; unknown figure refused: " + Unknown.Refusal);
        }
    }

    // ---- W47 Sketch corner fillet / chamfer (B on 2D sketches), fan lofts out of one section
    {
        constexpr double π = std::numbers::pi;
        const auto Format = [](const char* Pattern, auto... Values) { char Text[640]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        const auto CurvesIn = [](SolidArcDocument& Document, const std::string& Label) -> size_t
        {
            const auto Found = Document.Regeneration.Sketches.find(Label);
            return Found != Document.Regeneration.Sketches.end() ? Found->second.Curves.size() : 0;
        };

        // ① every corner of a 2 × 1 rectangle extruded 1, filleted r 0.25 afterwards: the extrude follows, V = 2 − (4 − π)/16
        SolidArcDocument Rounded = CreateSolidArcDocument("Rounded");
        Expect(DrawSolidArcSketch(Rounded, DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.0, 0.0 }, { 2.0, 1.0 } })));
        const std::string Plate = Rounded.Tree.Steps.back().Label;
        Expect(ExtrudeSolidArcSketch(Rounded, Plate, 1.0, FeatureCombination::New));
        const size_t History = Rounded.UndoEntries.size();
        SolidArcCornerRequest Every;
        Every.Sketches = { Plate };
        Every.Size     = 0.25;
        const SolidArcCommandOutcome Filleted = CutSolidArcSketchCorners(Rounded, Every);
        const double Expected = 2.0 - (4.0 - π) * 0.0625;
        double Relative = std::abs(Rounded.Summary.Volume - Expected) / Expected;
        Tally(CornerGate, Relative, Filleted.AcceptedCondition && Relative <= 1.0e-9 && CurvesIn(Rounded, Plate) == 8 && Rounded.Summary.SoundCondition && Rounded.Summary.Faces == 10
              && Rounded.UndoEntries.size() == History + 1,
              Format("%s: 4 lines + 4 tangent arcs, the extrude follows: V %.12f vs 2 − (4 − π)/16 %.12f, %d faces, sound, one undo entry", Filleted.Label.c_str(), Rounded.Summary.Volume, Expected, Rounded.Summary.Faces));

        // ② the corner (2, 1) stays as the virtual sharp: dragged to (3, 1) the rectangle widens and the fillets follow
        {
            const auto Found = Rounded.Regeneration.Sketches.find(Plate);
            int Sharp = -1;
            if (Found != Rounded.Regeneration.Sketches.end())
                for (size_t Index = 0; Index < Found->second.Points.size(); ++Index)
                    if (Length(Found->second.Points[Index].Position - Vector2{ 2.0, 1.0 }) <= 1.0e-12) Sharp = static_cast<int>(Index);
            const SolidArcCommandOutcome Dragged = Sharp >= 0 ? DragSolidArcSketchPoints(Rounded, Plate, { Sharp }, { { 3.0, 1.0 } }) : SolidArcCommandOutcome{};
            const double Wider = 3.0 - (4.0 - π) * 0.0625;
            const double Error = std::abs(Rounded.Summary.Volume - Wider) / Wider;
            Tally(CornerGate, Error, Dragged.AcceptedCondition && Error <= 1.0e-9 && CurvesIn(Rounded, Plate) == 8 && Rounded.Summary.SoundCondition,
                  Format("virtual sharp point %d dragged (2, 1) to (3, 1): V %.12f vs 3 − (4 − π)/16, the arcs stay tangent with r 0.25", Sharp, Rounded.Summary.Volume));
            UndoSolidArcCommand(Rounded);
            UndoSolidArcCommand(Rounded);
            Tally(CornerGate, std::abs(Rounded.Summary.Volume - 2.0), std::abs(Rounded.Summary.Volume - 2.0) <= 1.0e-12 && CurvesIn(Rounded, Plate) == 4, "undo × 2: the sharp rectangle and V 2 return");
        }

        // ③ one picked corner (0, 0) chamfered 0.3: 5 lines, area 2 − 0.045, Chamfer distances on the sketch
        {
            SolidArcDocument Cut = CreateSolidArcDocument("Chamfered");
            Expect(DrawSolidArcSketch(Cut, DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.0, 0.0 }, { 2.0, 1.0 } })));
            const std::string Sketch = Cut.Tree.Steps.back().Label;
            int Origin = -1;
            const SketchModel& Model = Cut.Regeneration.Sketches.at(Sketch);
            for (size_t Index = 0; Index < Model.Points.size(); ++Index) if (Length(Model.Points[Index].Position) <= 1.0e-12) Origin = static_cast<int>(Index);
            SolidArcCornerRequest One;
            One.Sketches = { Sketch };
            One.Points   = { Origin };
            One.Size     = 0.3;
            One.Shape    = SketchCornerShape::Chamfer;
            const SolidArcCommandOutcome Chamfered = CutSolidArcSketchCorners(Cut, One);
            int Labelled = 0;
            for (const FeatureStep& Step : Cut.Tree.Steps)
                if (Step.Label == Sketch) for (const SketchConstraint& Constraint : Step.Sketch.Constraints) Labelled += Constraint.Label == "Chamfer" ? 1 : 0;
            Expect(ExtrudeSolidArcSketch(Cut, Sketch, 1.0, FeatureCombination::New));
            const double Error = std::abs(Cut.Summary.Volume - 1.955) / 1.955;
            Tally(CornerGate, Error, Chamfered.AcceptedCondition && CurvesIn(Cut, Sketch) == 5 && Labelled == 2 && Error <= 1.0e-9 && Cut.Summary.SoundCondition,
                  Format("%s: 5 lines, 2 Chamfer distances; extruded 1 → V %.12f vs 2 − 0.3²/2 = 1.955", Chamfered.Label.c_str(), Cut.Summary.Volume));

            // ④ refusals, file untouched: r 0.6 (the second corner on the 1-long side has only 0.4 left), a circle centre, size 0
            const std::string Before = EncodeSolidArcDocument(Cut);
            SolidArcCornerRequest Large = Every;
            Large.Sketches = { Sketch };
            Large.Size     = 0.6;
            const SolidArcCommandOutcome TooLarge = CutSolidArcSketchCorners(Cut, Large);
            Expect(DrawSolidArcSketch(Cut, DrawingOf(SolidArcDrawingKind::Circle, 0, { { 5.0, 0.0 }, { 5.5, 0.0 } }, false, true)));
            const std::string Ring = Cut.Tree.Steps.back().Label;
            const std::string WithRing = EncodeSolidArcDocument(Cut);
            SolidArcCornerRequest Centre;
            Centre.Sketches = { Ring };
            Centre.Points   = { 0 };
            const SolidArcCommandOutcome NoCorner = CutSolidArcSketchCorners(Cut, Centre);
            Large.Size = 0.0;
            const SolidArcCommandOutcome Zero = CutSolidArcSketchCorners(Cut, Large);
            Tally(CornerGate, 0.0, !TooLarge.AcceptedCondition && !NoCorner.AcceptedCondition && !Zero.AcceptedCondition && EncodeSolidArcDocument(Cut) == WithRing && WithRing != Before,
                  "refused: " + TooLarge.Refusal + " · " + NoCorner.Refusal + " · " + Zero.Refusal);
        }

        // ⑤ fan lofts (the Windows report): A → B and A → C both start on line A, so they run along it the same way; the
        //    second panel turns and both sew into one sheet
        {
            SolidArcDocument Fan = CreateSolidArcDocument("Fan");
            Expect(DrawSolidArcSketch(Fan, DrawingOf(SolidArcDrawingKind::Line, 0, { { 0.0, 0.0 }, { 1.0, 0.0 } })));
            const std::string A = Fan.Tree.Steps.back().Label;
            Expect(DrawSolidArcSketch(Fan, DrawingOf(SolidArcDrawingKind::Line, 1, { { 0.0, 1.0 }, { 1.0, 1.0 } })));
            const std::string B = Fan.Tree.Steps.back().Label;
            Expect(DrawSolidArcSketch(Fan, DrawingOf(SolidArcDrawingKind::Line, 0, { { 0.0, 1.0 }, { 1.0, 1.0 } })));
            const std::string C = Fan.Tree.Steps.back().Label;
            const SolidArcCommandOutcome Upright = LoftSolidArcCurves(Fan, { A, B }, 0, false);
            const SolidArcCommandOutcome Level   = LoftSolidArcCurves(Fan, { A, C }, 0, false);
            Tally(CornerGate, 0.0, Upright.AcceptedCondition && Level.AcceptedCondition && Fan.Summary.SheetCondition && Fan.Summary.Faces == 2 && Fan.Summary.SoundCondition && Fan.Regeneration.Panels.size() == 2,
                  Upright.Label + " and " + Level.Label + ": one sheet of 2 faces sharing " + A + (Level.AcceptedCondition ? "" : " · refused: " + Level.Refusal));
            RedoSolidArcCommand(Rounded);                                                    // The four fillets again
            const std::string          Written = EncodeSolidArcDocument(Rounded);
            const SolidArcDocumentFile Read    = DecodeSolidArcDocument(Written);
            SolidArcDocument Loaded = CreateSolidArcDocument(Read.Title);
            Loaded.Tree = Read.Tree; Loaded.Captions = Read.Captions; Loaded.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Loaded, true);
            const double Drift = std::abs(Loaded.Summary.Volume - Expected) / Expected;
            Tally(CornerGate, Drift, Read.Refusal.empty() && EncodeSolidArcDocument(Loaded) == Written && Drift <= 1.0e-9 && CurvesIn(Rounded, Plate) == 8,
                  Format("codec: %d bytes byte-equal with the filleted sketch (arcs, Tangent / Radius / PointOnLine), V %.12f after reload", int(Written.size()), Loaded.Summary.Volume));
        }
    }

    // ---- W46 Trim and join on sketches some later step reads (detach), join of picked sketches
    {
        constexpr double π = std::numbers::pi;
        const auto Format = [](const char* Pattern, auto... Values) { char Text[640]; std::snprintf(Text, sizeof(Text), Pattern, Values...); return std::string(Text); };
        const auto Steps  = [](const SolidArcDocument& Document, const std::string& Label)
        {
            return std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; });
        };

        // ① a circle r 0.5 at the corner (2, 1) of a 2 × 1 rectangle, extruded 1: trimming its outer ¾ detaches — the
        //    extrude keeps the whole circle (V π/4), the original is hidden, the inner quarter arc is a new sketch
        SolidArcDocument Detach = CreateSolidArcDocument("Detach");
        Expect(DrawSolidArcSketch(Detach, DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.0, 0.0 }, { 2.0, 1.0 } })));
        Expect(DrawSolidArcSketch(Detach, DrawingOf(SolidArcDrawingKind::Circle, 0, { { 2.0, 1.0 }, { 2.5, 1.0 } }, false, true)));
        const std::string Round = Detach.Tree.Steps.back().Label;
        Expect(ExtrudeSolidArcSketch(Detach, Round, 1.0, FeatureCombination::New));
        const size_t StepCount = Detach.Tree.Steps.size();
        const size_t History   = Detach.UndoEntries.size();
        const double Puck      = 0.25 * π;
        const SolidArcCommandOutcome Trimmed = TrimSolidArcSketch(Detach, { Round, 0, { 2.0 + 0.5 * std::sqrt(0.5), 1.0 + 0.5 * std::sqrt(0.5) } });
        const FeatureStep& Copy   = Detach.Tree.Steps.back();
        const auto         Solved = Detach.Regeneration.Sketches.find(Copy.Label);
        double             Sweep  = 0.0;
        if (Solved != Detach.Regeneration.Sketches.end() && Solved->second.Curves.size() == 1 && Solved->second.Curves[0].Kind == SketchCurveKind::Arc)
        {
            const SketchModel& Arc   = Solved->second;
            const Vector2      Centre = Arc.Points[Arc.Curves[0].Centre].Position;
            const Vector2      From   = Arc.Points[Arc.Curves[0].Start].Position - Centre;
            const Vector2      To     = Arc.Points[Arc.Curves[0].End].Position - Centre;
            Sweep = std::atan2(From.X * To.Y - From.Y * To.X, From.X * To.X + From.Y * To.Y);
            if (Sweep < 0.0) Sweep += 2.0 * π;
        }
        double Relative = std::abs(Detach.Summary.Volume - Puck) / Puck;
        Tally(DetachGate, std::max(Relative, std::abs(Sweep - 0.5 * π)), Trimmed.AcceptedCondition && Relative <= 1.0e-9 && std::abs(Sweep - 0.5 * π) <= 1.0e-9
              && SolidArcCurveHiddenCondition(Detach, Round) && Detach.Tree.Steps.size() == StepCount + 1 && Copy.Kind == FeatureStepKind::Sketch && Detach.UndoEntries.size() == History + 1
              && SolidArcSketchConsumedCondition(Detach, Round) && !SolidArcSketchReferencedCondition(Detach, Copy.Label),
              Format("%s: V %.12f vs π/4 kept, %s hidden, new %s (%s) = quarter arc, sweep %.12f vs π/2, one undo entry", Trimmed.Label.c_str(), Detach.Summary.Volume,
                     Round.c_str(), Copy.Label.c_str(), SolidArcBodyCaption(Detach, Copy.Label).c_str(), Sweep));

        // ② the copy is free: trimming it again edits it in place (no further copy); undo × 2 restores the circle, shown
        {
            const size_t Count = Detach.Tree.Steps.size();
            const std::string Free = Copy.Label;
            const SolidArcCommandOutcome Again = TrimSolidArcSketch(Detach, { Free, 0, { 2.0 - 0.5 * std::sqrt(0.5), 1.0 - 0.5 * std::sqrt(0.5) } });
            const bool InPlace = Again.AcceptedCondition && Detach.Tree.Steps.size() <= Count && Steps(Detach, Round) != Detach.Tree.Steps.end();
            const bool Undone  = UndoSolidArcCommand(Detach) && UndoSolidArcCommand(Detach);
            Tally(DetachGate, 0.0, InPlace && Undone && Detach.Tree.Steps.size() == StepCount && !SolidArcCurveHiddenCondition(Detach, Round) && Detach.UndoEntries.size() == History,
                  "a second trim on " + Free + " edits it in place (" + Again.Label + "); two undos restore the shown circle and the step count");
        }

        // ③ a lone circle some extrude reads, no crossings: the whole curve goes, so the trim only hides it
        {
            Expect(DrawSolidArcSketch(Detach, DrawingOf(SolidArcDrawingKind::Circle, 0, { { 6.0, 0.0 }, { 6.5, 0.0 } }, false, true)));
            const std::string Lone = Detach.Tree.Steps.back().Label;
            Expect(ExtrudeSolidArcSketch(Detach, Lone, 1.0, FeatureCombination::Separate));
            const size_t Count  = Detach.Tree.Steps.size();
            const double Volume = Detach.Summary.Volume;
            const SolidArcCommandOutcome Whole = TrimSolidArcSketch(Detach, { Lone, 0, { 6.5, 0.0 } });
            Tally(DetachGate, std::abs(Detach.Summary.Volume - Volume), Whole.AcceptedCondition && Detach.Tree.Steps.size() == Count && SolidArcCurveHiddenCondition(Detach, Lone)
                  && Detach.Summary.Volume == Volume, "whole-curve trim of " + Lone + " (read by an extrude): hidden only, no new step, volume unchanged");
        }

        // ④ picked sketches join in pick order, touching or not: a 1 × 1 square and a disjoint circle r 0.4 become one
        //    sketch of 5 curves; extruded 1 → V 1 + 0.16π
        {
            SolidArcDocument Picked = CreateSolidArcDocument("Join");
            Expect(DrawSolidArcSketch(Picked, DrawingOf(SolidArcDrawingKind::Rectangle, 0, { { 0.0, 0.0 }, { 1.0, 1.0 } })));
            const std::string Square = Picked.Tree.Steps.back().Label;
            Expect(DrawSolidArcSketch(Picked, DrawingOf(SolidArcDrawingKind::Circle, 0, { { 3.0, 0.5 }, { 3.4, 0.5 } }, false, true)));
            const std::string Disc = Picked.Tree.Steps.back().Label;
            const SolidArcCommandOutcome Joined = JoinSolidArcSketches(Picked, { Square, Disc });
            const auto   Kept  = Steps(Picked, Square);
            const size_t Curves = Kept != Picked.Tree.Steps.end() ? Picked.Regeneration.Sketches[Square].Curves.size() : 0;
            const SolidArcCommandOutcome Raised = ExtrudeSolidArcSketch(Picked, Square, 1.0, FeatureCombination::New);
            const double Expected = 1.0 + 0.16 * π;
            Relative = std::abs(Picked.Summary.Volume - Expected) / Expected;
            Tally(DetachGate, Relative, Joined.AcceptedCondition && Curves == 5 && Steps(Picked, Disc) == Picked.Tree.Steps.end() && Raised.AcceptedCondition && Relative <= 1.0e-9,
                  Format("%s: one sketch of %d curves, %s removed; %s → V %.12f vs 1 + 0.16π %.12f", Joined.Label.c_str(), int(Curves), Disc.c_str(), Raised.Label.c_str(),
                         Picked.Summary.Volume, Expected));

            // ⑤ join into a read sketch: the extruded square stays (hidden) for its extrude, the join is a new last step
            //    holding square + a new circle; the free circle merges away; volume unchanged; codec byte-equal
            Expect(DrawSolidArcSketch(Picked, DrawingOf(SolidArcDrawingKind::Circle, 0, { { 0.5, 3.0 }, { 0.8, 3.0 } }, false, true)));
            const std::string Ring   = Picked.Tree.Steps.back().Label;
            const size_t      Count  = Picked.Tree.Steps.size();
            const double      Volume = Picked.Summary.Volume;
            const SolidArcCommandOutcome Into = JoinSolidArcSketches(Picked, { Square, Ring });
            const FeatureStep& Last = Picked.Tree.Steps.back();
            const size_t Held = Picked.Regeneration.Sketches.contains(Last.Label) ? Picked.Regeneration.Sketches[Last.Label].Curves.size() : 0;
            Tally(DetachGate, std::abs(Picked.Summary.Volume - Volume), Into.AcceptedCondition && SolidArcCurveHiddenCondition(Picked, Square) && Steps(Picked, Ring) == Picked.Tree.Steps.end()
                  && Picked.Tree.Steps.size() == Count && Last.Label != Square && Held == 6 && Picked.Summary.Volume == Volume,
                  Format("%s: %s kept hidden for its extrude, %s merged away, new %s holds %d curves, V unchanged", Into.Label.c_str(), Square.c_str(), Ring.c_str(), Last.Label.c_str(), int(Held)));
            const std::string          Written = EncodeSolidArcDocument(Picked);
            const SolidArcDocumentFile Read    = DecodeSolidArcDocument(Written);
            SolidArcDocument Loaded = CreateSolidArcDocument(Read.Title);
            Loaded.Tree = Read.Tree; Loaded.Captions = Read.Captions; Loaded.Appearance = Read.Appearance;
            RegenerateSolidArcDocument(Loaded, true);
            Tally(DetachGate, 0.0, Read.Refusal.empty() && EncodeSolidArcDocument(Loaded) == Written && Loaded.Summary.Volume == Picked.Summary.Volume,
                  Format("codec: %d bytes byte-equal after the detached join", int(Written.size())));
        }
    }

    for (const GateTally* Gate : { &ConstructGate, &PickGate, &BlendGate, &FaceGate, &ParameterGate, &AtomicGate, &HistoryGate, &FileGate, &VariableGate, &DrawingGate, &SurfacingGate, &PanelGate, &EvaluationGate, &BlueprintGate, &MatcapGate, &BodiesGate, &CutGate, &TrimGate, &MoveGate, &CopyGate, &HideGate, &CurveGate, &ConstrainGate, &ConflictGate, &DragGate, &ReferenceGate, &SplineGate, &LinkGate, &RevolveGate, &EvolutionGate, &ReplayGate, &XRayGate, &MeasureGate, &SectionGate, &SnapGate, &JointGate, &SheetGate, &ExchangeGate, &BoundGate, &PrimitiveGate, &InsetGate, &SectionLoftGate, &GizmoGate, &PointGate, &ShellGate, &DetachGate, &CornerGate })
    {
        std::printf("| %s | %s | %s | %.3g | %ld | %s |\n", Gate->Code, Gate->Subject, Gate->Limit, Gate->Worst, Gate->Checks, Gate->Failures ? "🔴" : "🟢");
        Failures += Gate->Failures;
    }
    std::printf("\n## Walkthrough\n\n| Gate | Check | Value | Verdict |\n|------|-------|-------|---------|\n");
    for (const std::string& Row : Journal) std::printf("%s\n", Row.c_str());
    std::printf("\n## Saved document (TOML, first 40 lines)\n\n```toml\n");
    size_t Cursor = 0;
    for (int Line = 0; Line < 40 && Cursor < Saved.size(); ++Line)
    {
        const size_t Next = Saved.find('\n', Cursor);
        std::printf("%s\n", Saved.substr(Cursor, Next - Cursor).c_str());
        Cursor = Next == std::string::npos ? Saved.size() : Next + 1;
    }
    std::printf("```\n");
    return Failures == 0 ? 0 : 1;
}
