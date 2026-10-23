/*====================================================================================================================================
                                                     DRAWINGVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Drawings D1 (priority 11): orthographic views, exact silhouettes, ray-cast hidden lines, section views with hatching — every
//    value against a closed form

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Blend/BlendCatalogue.h"
#include "Blend/EdgeBlend.h"
#include "Boolean/GeneralFuse.h"
#include "Construction/PrimitiveConstruction.h"
#include "Drawing/DrawingProjection.h"
#include "Geometry/CurveGeometry.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <numbers>
#include <set>
#include <string>
#include <vector>

using namespace Frontier::SolidArcRemastered;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double Pi = std::numbers::pi;

    struct GateTally
    {
        const char* Code     = "";                                                           // [-] - DR1 …
        const char* Subject  = "";                                                           // [-] - What it measures
        const char* Limit    = "";                                                           // [-] - Acceptance rule
        double      Worst    = 0.0;                                                          // [-] - Largest measured value
        long        Checks   = 0;                                                            // [-]
        long        Failures = 0;                                                            // [-]
    };

    GateTally BlockGate      = { "DR1", "A block 4 × 2.5 × 1 in the six principal views and the isometric: visible length = both coincident outlines (2 × perimeter), nothing hidden; isometric: the three edges at the far corner (0, 2.5, 0) hidden, 9 visible, every edge foreshortened by √(2/3); view box = the projected extent", "≤ 10⁻¹²" };
    GateTally HoleGate       = { "DR2", "The block minus a through hole r 0.3 at (3, 1.25): top view — both hole circles visible, every point at r from the centre, angular cover 2 × 2π; front / right view — exactly two hidden silhouettes of length 1 at x = 2.7, 3.3 / y = 0.95, 1.55, total hidden = 2", "≤ 10⁻¹²" };
    GateTally SilhouetteGate = { "DR3", "Silhouettes: a cylinder r 0.5 (front: two visible lines at x = ±0.5 of length 2; top: none), a sphere r 1 in front, top and isometric views (every point at radius 1, angular cover 2π), the cylinder in the isometric view (two lines at view x = ±0.5)", "≤ 10⁻¹² · cover ≤ 10⁻⁶" };
    GateTally OcclusionGate  = { "DR4", "Occlusion between bodies: block B [1, 4] × [3, 4] × [0.5, 1.5] behind block A [0, 2] × [0, 1] × [0, 1] in the front view — hidden length 3 (bottom x ∈ [1, 2], left z ∈ [0.5, 1], front and back), visible 25, every hidden stroke exactly (1, 0.5)→(2, 0.5) or (1, 0.5)→(1, 1); hidden lines switched off drops them all", "≤ 10⁻¹²" };
    GateTally SectionGate    = { "DR5", "Section view through the hole axis (plane y = 1.25 toward the front viewer): cap area 4 × 1 − 0.6 × 1 = 3.4, every hatch end on the cap boundary, every hatch midpoint inside, hatch at 45° and the requested spacing, the section outline closed, the source partition untouched", "≤ 10⁻¹²" };
    GateTally TangentGate    = { "DR6", "Tangent edges: a box edge filleted r 0.3 — exactly its two run-out edges flagged tangent (a chamfer: none), switched off they vanish; the right view along the fillet shows its quarter-circle caps at r 0.3 about (0.3, 1.7) and no silhouette on the fillet", "≤ 10⁻¹² · count exact" };
    GateTally RefuseGate     = { "DR7", "Refusals name the cause: no bodies, a view frame that is not orthonormal, a left-handed frame, a section plane that misses every body, a section plane with every body in front of it", "named" };
    GateTally TimingGate     = { "DR8", "Projection time of every view above (block, hole, sphere, occlusion, section, fillet)", "≤ 5 s per view" };

    std::vector<GateTally*> AllGates() { return { &BlockGate, &HoleGate, &SilhouetteGate, &OcclusionGate, &SectionGate, &TangentGate, &RefuseGate, &TimingGate }; }

    struct DetailRow
    {
        std::string Gate            = {};
        std::string Name            = {};
        std::string Detail          = {};
        bool        PassedCondition = false;
    };
    std::vector<DetailRow> Details;
    std::string            SvgFolder;

    void Report(GateTally& Gate, const std::string& Name, const std::string& Detail, double Value, bool PassedCondition)
    {
        Gate.Worst = std::max(Gate.Worst, Value);
        ++Gate.Checks;
        if (!PassedCondition) ++Gate.Failures;
        Details.push_back({ Gate.Code, Name, Detail, PassedCondition });
        std::fprintf(stderr, "%s %s %s · %s\n", PassedCondition ? "🟢" : "🔴", Gate.Code, Name.c_str(), Detail.c_str());
    }

    std::string Format(const char* Pattern, double First, double Second = 0.0, double Third = 0.0)
    {
        char Text[512];
        std::snprintf(Text, sizeof(Text), Pattern, First, Second, Third);
        return Text;
    }

    void Timing(const std::string& Name, const DrawingView& View)
    {
        Report(TimingGate, Name, Format("%.1f ms, %.0f strokes, %.0f rays", View.Milliseconds, static_cast<double>(View.Strokes.size()), static_cast<double>(View.VisibilityProbes)), View.Milliseconds / 1000.0, View.Milliseconds <= 5000.0);
    }

    // Σ stroke lengths of one visibility (optionally one kind).
    double SumLength(const DrawingView& View, DrawingVisibility Visibility, std::optional<DrawingStrokeKind> Kind = {})
    {
        double Total = 0.0;
        for (const DrawingStroke& Stroke : View.Strokes)
            if (Stroke.Visibility == Visibility && (!Kind || Stroke.Kind == *Kind) && Stroke.Kind != DrawingStrokeKind::Hatch) Total += MeasureStrokeLength(Stroke);
        return Total;
    }

    // Angle swept by strokes about a centre (sum of the angle increments along each polyline).
    double AngularCover(const std::vector<const DrawingStroke*>& Strokes, Vector2 Centre)
    {
        double Cover = 0.0;
        for (const DrawingStroke* Stroke : Strokes)
            for (size_t Index = 1; Index < Stroke->Points.size(); ++Index)
            {
                const double First  = std::atan2(Stroke->Points[Index - 1].Y - Centre.Y, Stroke->Points[Index - 1].X - Centre.X);
                const double Second = std::atan2(Stroke->Points[Index].Y - Centre.Y, Stroke->Points[Index].X - Centre.X);
                double Turn = Second - First;
                while (Turn > Pi) Turn -= 2.0 * Pi;
                while (Turn < -Pi) Turn += 2.0 * Pi;
                Cover += std::fabs(Turn);
            }
        return Cover;
    }

    double RadiusError(const std::vector<const DrawingStroke*>& Strokes, Vector2 Centre, double Radius)
    {
        double Worst = 0.0;
        for (const DrawingStroke* Stroke : Strokes)
            for (const Vector2& Point : Stroke->Points) Worst = std::max(Worst, std::fabs(std::hypot(Point.X - Centre.X, Point.Y - Centre.Y) - Radius));
        return Worst;
    }

    std::vector<const DrawingStroke*> Select(const DrawingView& View, const std::function<bool(const DrawingStroke&)>& Filter)
    {
        std::vector<const DrawingStroke*> Chosen;
        for (const DrawingStroke& Stroke : View.Strokes) if (Filter(Stroke)) Chosen.push_back(&Stroke);
        return Chosen;
    }

    // Strokes whose every point lies within Precision of the circle about Centre.
    std::vector<const DrawingStroke*> OnCircle(const DrawingView& View, Vector2 Centre, double Radius, double Precision)
    {
        return Select(View, [&](const DrawingStroke& Stroke)
        {
            if (Stroke.Kind == DrawingStrokeKind::Hatch) return false;
            for (const Vector2& Point : Stroke.Points) if (std::fabs(std::hypot(Point.X - Centre.X, Point.Y - Centre.Y) - Radius) > Precision) return false;
            return true;
        });
    }

    // 📝 Evidence: visible strokes solid, hidden dashed, silhouettes blue, section outlines heavy, hatch thin.
    void WriteSvg(const std::string& Name, const DrawingView& View)
    {
        if (SvgFolder.empty() || View.Strokes.empty()) return;
        const std::string Path = SvgFolder + "/" + Name + ".svg";
        std::FILE* File = std::fopen(Path.c_str(), "w");
        if (!File) return;
        const double Width = View.High.X - View.Low.X, Height = View.High.Y - View.Low.Y;
        const double Pad   = 0.08 * std::max(Width, Height) + 1.0e-3;
        const double Scale = 520.0 / std::max(Width + 2.0 * Pad, Height + 2.0 * Pad);
        const double PixelsX = (Width + 2.0 * Pad) * Scale, PixelsY = (Height + 2.0 * Pad) * Scale;
        std::fprintf(File, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\">\n", PixelsX, PixelsY + 24.0, PixelsX, PixelsY + 24.0);
        std::fprintf(File, "<rect width=\"100%%\" height=\"100%%\" fill=\"#fbfbf8\"/>\n<text x=\"8\" y=\"17\" font-family=\"monospace\" font-size=\"13\" fill=\"#333\">%s · %zu strokes · %.0f ms</text>\n", Name.c_str(), View.Strokes.size(), View.Milliseconds);
        for (int Layer = 0; Layer < 3; ++Layer)                                              // Hatch, hidden, visible
        {
            for (const DrawingStroke& Stroke : View.Strokes)
            {
                const bool Hatch = Stroke.Kind == DrawingStrokeKind::Hatch, Hidden = Stroke.Visibility == DrawingVisibility::Hidden;
                if ((Layer == 0) != Hatch || (!Hatch && (Layer == 1) != Hidden)) continue;
                const char* Colour = Hatch ? "#8a8a8a" : Stroke.Kind == DrawingStrokeKind::Silhouette ? "#1f5fbf" : Stroke.Kind == DrawingStrokeKind::TangentEdge ? "#6c8f3a" : "#111";
                const double Weight = Hatch ? 0.7 : Stroke.Kind == DrawingStrokeKind::SectionOutline ? 2.6 : Hidden ? 1.0 : 1.6;
                std::fprintf(File, "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"%.1f\" stroke-linecap=\"round\"%s points=\"", Colour, Weight, Hidden ? " stroke-dasharray=\"6 4\"" : "");
                for (const Vector2& Point : Stroke.Points) std::fprintf(File, "%.2f,%.2f ", (Point.X - View.Low.X + Pad) * Scale, 24.0 + (View.High.Y - Point.Y + Pad) * Scale);
                std::fprintf(File, "\"/>\n");
            }
        }
        std::fprintf(File, "</svg>\n");
        std::fclose(File);
    }

    DrawingViewRequest Standard(DrawingStandardView Kind)
    {
        DrawingViewRequest Request;
        Request.Frame = StandardDrawingFrame(Kind);
        return Request;
    }

    // The bracket: block 4 × 2.5 × 1 minus a through hole r 0.3 at (3, 1.25) (the drill longer than the block).
    BodyToken ConstructBracket(ModelPartition& Partition)
    {
        const BodyToken Block = ConstructBlock(Partition, {}, { 4.0, 2.5, 1.0 }).Value;
        const BodyToken Drill = ConstructCylinder(Partition, ConstructFrame({ 3.0, 1.25, -0.5 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }), 0.3, 2.0).Value;
        return ComputeBoolean(Partition, Block, Drill, BooleanOperation::Difference).Body;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                GATES
    //--------------------------------------------------------------------------------------------------------------------

    void ValidateBlock()
    {
        ModelPartition  Partition;
        const BodyToken Block = ConstructBlock(Partition, {}, { 4.0, 2.5, 1.0 }).Value;
        struct Expectation { DrawingStandardView Kind; double Perimeter; Vector2 Low, High; };
        const Expectation Views[] = {
            { DrawingStandardView::Front, 10.0, { 0.0, 0.0 }, { 4.0, 1.0 } },   { DrawingStandardView::Back, 10.0, { -4.0, 0.0 }, { 0.0, 1.0 } },
            { DrawingStandardView::Top, 13.0, { 0.0, 0.0 }, { 4.0, 2.5 } },     { DrawingStandardView::Bottom, 13.0, { 0.0, -2.5 }, { 4.0, 0.0 } },
            { DrawingStandardView::Right, 7.0, { 0.0, 0.0 }, { 2.5, 1.0 } },    { DrawingStandardView::Left, 7.0, { -2.5, 0.0 }, { 0.0, 1.0 } },
        };
        for (const Expectation& Expected : Views)
        {
            const DrawingView View = ProjectDrawingView(Partition, { Block }, Standard(Expected.Kind));
            const double Visible = SumLength(View, DrawingVisibility::Visible), Hidden = SumLength(View, DrawingVisibility::Hidden);
            const double Box = std::max({ std::fabs(View.Low.X - Expected.Low.X), std::fabs(View.Low.Y - Expected.Low.Y), std::fabs(View.High.X - Expected.High.X), std::fabs(View.High.Y - Expected.High.Y) });
            const double Error = std::max({ std::fabs(Visible - 2.0 * Expected.Perimeter), Hidden, Box });
            Report(BlockGate, std::string("Block · ") + DrawingStandardViewLabel(Expected.Kind), Format("visible %.15g (2 × %.4g), hidden %.3g", Visible, Expected.Perimeter, Hidden) + Format(", view box error %.2e", Box), Error, View.Refusal.empty() && Error <= 1.0e-12);
            Timing(std::string("Block · ") + DrawingStandardViewLabel(Expected.Kind), View);
            if (Expected.Kind == DrawingStandardView::Front) WriteSvg("BlockFront", View);
        }
        const DrawingView View = ProjectDrawingView(Partition, { Block }, Standard(DrawingStandardView::Isometric));
        const double Foreshorten = std::sqrt(2.0 / 3.0);
        const double Visible = SumLength(View, DrawingVisibility::Visible), Hidden = SumLength(View, DrawingVisibility::Hidden);
        const double Error = std::max(std::fabs(Visible - 3.0 * 7.5 * Foreshorten), std::fabs(Hidden - 7.5 * Foreshorten));
        std::set<uint32_t> HiddenEdges;
        for (const DrawingStroke& Stroke : View.Strokes) if (Stroke.Visibility == DrawingVisibility::Hidden) HiddenEdges.insert(Stroke.Source);
        Report(BlockGate, "Block · Isometric", Format("visible %.15g (22.5·√⅔), hidden %.15g (7.5·√⅔), hidden edges %.0f", Visible, Hidden, static_cast<double>(HiddenEdges.size())), Error, Error <= 1.0e-12 && HiddenEdges.size() == 3);
        Timing("Block · Isometric", View);
        WriteSvg("BlockIsometric", View);
    }

    void ValidateHole()
    {
        ModelPartition  Partition;
        const BodyToken Bracket = ConstructBracket(Partition);
        const DrawingView Top = ProjectDrawingView(Partition, { Bracket }, Standard(DrawingStandardView::Top));
        const std::vector<const DrawingStroke*> Circles = OnCircle(Top, { 3.0, 1.25 }, 0.3, 1.0e-9);
        double Hidden = SumLength(Top, DrawingVisibility::Hidden);
        bool   AllVisible = true;
        for (const DrawingStroke* Stroke : Circles) AllVisible = AllVisible && Stroke->Visibility == DrawingVisibility::Visible;
        const double Radius = RadiusError(Circles, { 3.0, 1.25 }, 0.3), Cover = AngularCover(Circles, { 3.0, 1.25 });
        const double Silhouettes = static_cast<double>(Select(Top, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::Silhouette; }).size());
        Report(HoleGate, "Top view · hole circles", Format("radius error %.2e, cover %.15g (4π = %.15g)", Radius, Cover, 4.0 * Pi) + Format(", hidden %.3g, silhouettes %.0f", Hidden, Silhouettes),
               std::max({ Radius, std::fabs(Cover - 4.0 * Pi), Hidden }), AllVisible && Radius <= 1.0e-12 && std::fabs(Cover - 4.0 * Pi) <= 1.0e-12 && Hidden == 0.0 && Silhouettes == 0.0);
        Timing("Bracket · Top", Top);
        WriteSvg("BracketTop", Top);

        struct Side { DrawingStandardView Kind; double First, Second; const char* Name; };
        for (const Side& Expected : { Side{ DrawingStandardView::Front, 2.7, 3.3, "Front view · hidden hole silhouettes" }, Side{ DrawingStandardView::Right, 0.95, 1.55, "Right view · hidden hole silhouettes" } })
        {
            const DrawingView View = ProjectDrawingView(Partition, { Bracket }, Standard(Expected.Kind));
            const std::vector<const DrawingStroke*> Lines = Select(View, [](const DrawingStroke& Stroke) { return Stroke.Visibility == DrawingVisibility::Hidden; });
            double Error = std::fabs(SumLength(View, DrawingVisibility::Hidden) - 2.0);
            bool   Kinds = Lines.size() == 2;
            std::set<long> Places;
            for (const DrawingStroke* Line : Lines)
            {
                Kinds = Kinds && Line->Kind == DrawingStrokeKind::Silhouette;
                for (const Vector2& Point : Line->Points)
                {
                    const double Near = std::min(std::fabs(Point.X - Expected.First), std::fabs(Point.X - Expected.Second));
                    Error = std::max(Error, Near);
                }
                Places.insert(std::lround(Line->Points.front().X * 100.0));
                Error = std::max({ Error, std::fabs(MeasureStrokeLength(*Line) - 1.0) });
            }
            Report(HoleGate, Expected.Name, Format("%.0f hidden silhouettes, total %.15g, worst offset from x = {%.4g, ", static_cast<double>(Lines.size()), SumLength(View, DrawingVisibility::Hidden), Expected.First) + Format("%.4g} %.2e", Expected.Second, Error),
                   Error, Kinds && Places.size() == 2 && Error <= 1.0e-12);
            Timing(std::string("Bracket · ") + DrawingStandardViewLabel(Expected.Kind), View);
            WriteSvg(std::string("Bracket") + DrawingStandardViewLabel(Expected.Kind), View);
        }
        const DrawingView Isometric = ProjectDrawingView(Partition, { Bracket }, Standard(DrawingStandardView::Isometric));
        Report(HoleGate, "Isometric view · projects", Format("%.0f strokes, %.0f hidden", static_cast<double>(Isometric.Strokes.size()), static_cast<double>(Select(Isometric, [](const DrawingStroke& Stroke) { return Stroke.Visibility == DrawingVisibility::Hidden; }).size())), 0.0, Isometric.Refusal.empty() && !Isometric.Strokes.empty());
        Timing("Bracket · Isometric", Isometric);
        WriteSvg("BracketIsometric", Isometric);
    }

    void ValidateSilhouettes()
    {
        ModelPartition  Partition;
        const BodyToken Cylinder = ConstructCylinder(Partition, ConstructFrame({}, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }), 0.5, 2.0).Value;
        {
            const DrawingView View = ProjectDrawingView(Partition, { Cylinder }, Standard(DrawingStandardView::Front));
            const std::vector<const DrawingStroke*> Lines = Select(View, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::Silhouette; });
            double Error = 0.0;
            std::set<long> Sides;
            bool Visible = Lines.size() == 2;
            for (const DrawingStroke* Line : Lines)
            {
                Visible = Visible && Line->Visibility == DrawingVisibility::Visible;
                for (const Vector2& Point : Line->Points) Error = std::max(Error, std::fabs(std::fabs(Point.X) - 0.5));
                Error = std::max(Error, std::fabs(MeasureStrokeLength(*Line) - 2.0));
                Sides.insert(Line->Points.front().X > 0.0 ? 1 : -1);
            }
            Report(SilhouetteGate, "Cylinder r 0.5 · front", Format("%.0f silhouettes, worst |x| − 0.5 / length − 2 = %.2e", static_cast<double>(Lines.size()), Error), Error, Visible && Sides.size() == 2 && Error <= 1.0e-12);
            Timing("Cylinder · Front", View);
            WriteSvg("CylinderFront", View);
        }
        {
            const DrawingView View = ProjectDrawingView(Partition, { Cylinder }, Standard(DrawingStandardView::Top));
            const double Silhouettes = static_cast<double>(Select(View, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::Silhouette; }).size());
            const std::vector<const DrawingStroke*> Circles = OnCircle(View, {}, 0.5, 1.0e-9);
            const double Cover = AngularCover(Circles, {});
            Report(SilhouetteGate, "Cylinder r 0.5 · top (seen along its axis)", Format("%.0f silhouettes, rims cover %.15g (4π)", Silhouettes, Cover), std::fabs(Cover - 4.0 * Pi), Silhouettes == 0.0 && std::fabs(Cover - 4.0 * Pi) <= 1.0e-12);
        }
        {
            const DrawingView View = ProjectDrawingView(Partition, { Cylinder }, Standard(DrawingStandardView::Isometric));
            const std::vector<const DrawingStroke*> Lines = Select(View, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::Silhouette; });
            double Error = 0.0;
            std::set<long> Sides;
            for (const DrawingStroke* Line : Lines)
            {
                for (const Vector2& Point : Line->Points) Error = std::max(Error, std::fabs(std::fabs(Point.X) - 0.5));
                Sides.insert(Line->Points.front().X > 0.0 ? 1 : -1);
            }
            Report(SilhouetteGate, "Cylinder r 0.5 · isometric", Format("%.0f silhouettes, worst | |x| − 0.5 | = %.2e", static_cast<double>(Lines.size()), Error), Error, Lines.size() == 2 && Sides.size() == 2 && Error <= 1.0e-12);
            Timing("Cylinder · Isometric", View);
            WriteSvg("CylinderIsometric", View);
        }
        const BodyToken Sphere = ConstructSphere(Partition, ConstructFrame({ 5.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }), 1.0).Value;
        for (DrawingStandardView Kind : { DrawingStandardView::Front, DrawingStandardView::Top, DrawingStandardView::Isometric })
        {
            const DrawingView View   = ProjectDrawingView(Partition, { Sphere }, Standard(Kind));
            const Vector2     Centre = ProjectDrawingPoint(StandardDrawingFrame(Kind), { 5.0, 0.0, 0.0 });
            const std::vector<const DrawingStroke*> Lines = Select(View, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::Silhouette; });
            const double Radius = RadiusError(Lines, Centre, 1.0), Cover = AngularCover(Lines, Centre);
            bool Visible = !Lines.empty();
            for (const DrawingStroke* Line : Lines) Visible = Visible && Line->Visibility == DrawingVisibility::Visible;
            Report(SilhouetteGate, std::string("Sphere r 1 · ") + DrawingStandardViewLabel(Kind), Format("%.0f silhouette runs, radius error %.2e, cover %.12g (2π)", static_cast<double>(Lines.size()), Radius, Cover), std::max(Radius, std::fabs(Cover - 2.0 * Pi) * 1.0e-6),
                   Visible && Radius <= 1.0e-12 && std::fabs(Cover - 2.0 * Pi) <= 1.0e-6);
            Timing(std::string("Sphere · ") + DrawingStandardViewLabel(Kind), View);
            if (Kind == DrawingStandardView::Isometric) WriteSvg("SphereIsometric", View);
        }
    }

    void ValidateOcclusion()
    {
        ModelPartition  Partition;
        const BodyToken Front = ConstructBlock(Partition, {}, { 2.0, 1.0, 1.0 }).Value;
        const BodyToken Back  = ConstructBlock(Partition, { 1.0, 3.0, 0.5 }, { 3.0, 1.0, 1.0 }).Value;
        DrawingViewRequest Request = Standard(DrawingStandardView::Front);
        const DrawingView View = ProjectDrawingView(Partition, { Front, Back }, Request);
        const double Visible = SumLength(View, DrawingVisibility::Visible), Hidden = SumLength(View, DrawingVisibility::Hidden);
        double Ends = 0.0;
        bool   Owners = true;
        for (const DrawingStroke& Stroke : View.Strokes)
        {
            if (Stroke.Visibility != DrawingVisibility::Hidden) continue;
            Owners = Owners && Stroke.Body == Back;
            // Exactly one of the two hidden pieces: bottom (1, 0.5) → (2, 0.5) or left (1, 0.5) → (1, 1), either direction.
            const Vector2 First = Stroke.Points.front(), Last = Stroke.Points.back();
            double Near = 1.0e300;
            for (const Vector2 Far : { Vector2{ 2.0, 0.5 }, Vector2{ 1.0, 1.0 } })
            {
                const double Forward  = std::max(std::hypot(First.X - 1.0, First.Y - 0.5), std::hypot(Last.X - Far.X, Last.Y - Far.Y));
                const double Backward = std::max(std::hypot(Last.X - 1.0, Last.Y - 0.5), std::hypot(First.X - Far.X, First.Y - Far.Y));
                Near = std::min({ Near, Forward, Backward });
            }
            Ends = std::max(Ends, Near);
        }
        const double Error = std::max({ std::fabs(Visible - 25.0), std::fabs(Hidden - 3.0), Ends });
        Report(OcclusionGate, "Block behind block · front", Format("visible %.15g (25), hidden %.15g (3), worst hidden piece off (1, 0.5)→(2, 0.5) / (1, 0.5)→(1, 1) by %.2e", Visible, Hidden, Ends), Error, Owners && Error <= 1.0e-12);
        Timing("Occlusion · Front", View);
        WriteSvg("OcclusionFront", View);
        Request.HiddenLinesEnabled = false;
        const DrawingView Plain = ProjectDrawingView(Partition, { Front, Back }, Request);
        Report(OcclusionGate, "Hidden lines off", Format("hidden %.3g, visible %.15g (25)", SumLength(Plain, DrawingVisibility::Hidden), SumLength(Plain, DrawingVisibility::Visible)), std::fabs(SumLength(Plain, DrawingVisibility::Visible) - 25.0),
               SumLength(Plain, DrawingVisibility::Hidden) == 0.0 && std::fabs(SumLength(Plain, DrawingVisibility::Visible) - 25.0) <= 1.0e-12);
    }

    void ValidateSection()
    {
        ModelPartition  Partition;
        const BodyToken Bracket   = ConstructBracket(Partition);
        const size_t    BodiesBefore = Partition.Bodies.OccupiedCount(), FacesBefore = Partition.Faces.OccupiedCount();
        DrawingViewRequest Request = Standard(DrawingStandardView::Front);
        Request.Section      = OrthonormalFrame{ { 0.0, 1.25, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, -1.0, 0.0 } };
        Request.HatchSpacing = 0.1;
        const DrawingView View = ProjectDrawingView(Partition, { Bracket }, Request);
        Report(SectionGate, "Cap area", Format("%.15g (3.4) over %.0f caps", View.SectionArea, static_cast<double>(View.SectionCaps)), std::fabs(View.SectionArea - 3.4), View.Refusal.empty() && std::fabs(View.SectionArea - 3.4) <= 1.0e-12);

        // The cap: x ∈ [0, 2.7] ∪ [3.3, 4], z ∈ [0, 1] in front-view coordinates.
        const auto Boundary = [](Vector2 Point)
        {
            const double Across = std::min({ std::fabs(Point.X), std::fabs(Point.X - 2.7), std::fabs(Point.X - 3.3), std::fabs(Point.X - 4.0) });
            const double Up     = std::min(std::fabs(Point.Y), std::fabs(Point.Y - 1.0));
            return std::min(Across, Up);
        };
        const auto Inside = [](Vector2 Point) { return Point.Y > 0.0 && Point.Y < 1.0 && ((Point.X > 0.0 && Point.X < 2.7) || (Point.X > 3.3 && Point.X < 4.0)); };
        double EndError = 0.0, AngleError = 0.0, SpacingError = 0.0;
        bool   Middles = true;
        std::vector<double> Offsets;
        const std::vector<const DrawingStroke*> Hatch = Select(View, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::Hatch; });
        for (const DrawingStroke* Line : Hatch)
        {
            const Vector2 Start = Line->Points.front(), End = Line->Points.back();
            EndError   = std::max({ EndError, Boundary(Start), Boundary(End) });
            Middles    = Middles && Inside({ 0.5 * (Start.X + End.X), 0.5 * (Start.Y + End.Y) });
            AngleError = std::max(AngleError, std::fabs(std::fabs((End.Y - Start.Y) / (End.X - Start.X)) - 1.0));
            Offsets.push_back((Start.Y - Start.X) / std::sqrt(2.0));                         // n·p with n = (−sin 45°, cos 45°)
        }
        std::sort(Offsets.begin(), Offsets.end());
        Offsets.erase(std::unique(Offsets.begin(), Offsets.end(), [](double First, double Second) { return Second - First < 1.0e-9; }), Offsets.end());
        for (const double Offset : Offsets) SpacingError = std::max(SpacingError, std::fabs(Offset / 0.1 - 0.5 - std::round(Offset / 0.1 - 0.5)) * 0.1);
        Report(SectionGate, "Hatching", Format("%.0f hatch lines, worst end off the cap boundary %.2e, worst slope error %.2e", static_cast<double>(Hatch.size()), EndError, AngleError) + Format(", spacing error %.2e, ", SpacingError) + (Middles ? "midpoints inside" : "a midpoint outside"),
               std::max({ EndError, AngleError, SpacingError }), !Hatch.empty() && Middles && EndError <= 1.0e-12 && AngleError <= 1.0e-12 && SpacingError <= 1.0e-12);

        // The section outline: the cap boundary, every point on it, total = 2·(2.7 + 1) + 2·(0.7 + 1) = 10.8.
        const std::vector<const DrawingStroke*> Outline = Select(View, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::SectionOutline; });
        double Off = 0.0, Total = 0.0;
        for (const DrawingStroke* Line : Outline)
        {
            Total += MeasureStrokeLength(*Line);
            for (const Vector2& Point : Line->Points) Off = std::max(Off, Boundary(Point));
        }
        Report(SectionGate, "Section outline", Format("%.0f strokes, length %.15g (10.8), worst point off the boundary %.2e", static_cast<double>(Outline.size()), Total, Off), std::max(Off, std::fabs(Total - 10.8)), Off <= 1.0e-12 && std::fabs(Total - 10.8) <= 1.0e-12);
        Report(SectionGate, "Source partition untouched", Format("bodies %.0f → %.0f, faces %.0f → ", static_cast<double>(BodiesBefore), static_cast<double>(Partition.Bodies.OccupiedCount()), static_cast<double>(FacesBefore)) + Format("%.0f", static_cast<double>(Partition.Faces.OccupiedCount())), 0.0,
               BodiesBefore == Partition.Bodies.OccupiedCount() && FacesBefore == Partition.Faces.OccupiedCount());
        Timing("Bracket · Section A-A", View);
        WriteSvg("BracketSection", View);
    }

    void ValidateTangent()
    {
        for (const char* Name : { "BoxEdgeFillet", "BoxEdgeChamfer" })
        {
            ModelPartition      Partition;
            const BlendSpecimen Specimen = ConstructBlendSpecimen(Partition, Name);
            const BlendOutcome  Blended  = BlendEdges(Partition, Specimen.Body, Specimen.Edges, Specimen.Settings);
            if (!Blended.Refusal.empty()) { Report(TangentGate, Name, "blend refused: " + Blended.Refusal, 1.0, false); continue; }
            const bool Fillet = std::string(Name) == "BoxEdgeFillet";
            DrawingViewRequest Request = Standard(DrawingStandardView::Isometric);
            const DrawingView View = ProjectDrawingView(Partition, { Blended.Body }, Request);
            std::set<uint32_t> Tangent;
            for (const DrawingStroke& Stroke : View.Strokes) if (Stroke.Kind == DrawingStrokeKind::TangentEdge) Tangent.insert(Stroke.Source);
            // Run-outs of the fillet along x: y = 0.3 on the top (z = 2) and z = 1.7 on the front (y = 0).
            double RunOut = 0.0;
            for (uint32_t Slot : Tangent)
            {
                const EdgeProfile& Edge = Partition.Edges.Resolve(Partition.Edges.TokenAt(Slot));
                const CurveGeometry& Curve = Partition.Curves.Resolve(Edge.Curve);
                for (double Parameter : { Edge.IntervalLow, Edge.IntervalHigh })
                {
                    const Vector3 Point = EvaluateCurve(Curve, Parameter).Position;
                    RunOut = std::max(RunOut, std::min(std::hypot(Point.Y - 0.3, Point.Z - 2.0), std::hypot(Point.Y, Point.Z - 1.7)));
                }
            }
            Request.TangentEdgesEnabled = false;
            const DrawingView Plain = ProjectDrawingView(Partition, { Blended.Body }, Request);
            const double PlainTangent = static_cast<double>(Select(Plain, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::TangentEdge; }).size());
            const size_t Expected = Fillet ? 2 : 0;
            Report(TangentGate, std::string(Name) + " · isometric", Format("%.0f tangent edges (%.0f expected), run-out ends off y = 0.3 / z = 1.7 by %.2e", static_cast<double>(Tangent.size()), static_cast<double>(Expected), RunOut) + Format(", %.0f left when switched off", PlainTangent), RunOut,
                   Tangent.size() == Expected && RunOut <= 1.0e-12 && PlainTangent == 0.0);
            Timing(std::string(Name) + " · Isometric", View);
            WriteSvg(std::string(Name) + "Isometric", View);
            if (!Fillet) continue;
            const DrawingView Right = ProjectDrawingView(Partition, { Blended.Body }, Standard(DrawingStandardView::Right));
            const std::vector<const DrawingStroke*> Caps = OnCircle(Right, { 0.3, 1.7 }, 0.3, 1.0e-9);
            const double Cover = AngularCover(Caps, { 0.3, 1.7 }) / static_cast<double>(std::max<size_t>(1, Caps.size()));
            const double Silhouettes = static_cast<double>(Select(Right, [](const DrawingStroke& Stroke) { return Stroke.Kind == DrawingStrokeKind::Silhouette; }).size());
            Report(TangentGate, "BoxEdgeFillet · right view caps", Format("%.0f quarter arcs, radius error %.2e, mean cover %.15g (π/2)", static_cast<double>(Caps.size()), RadiusError(Caps, { 0.3, 1.7 }, 0.3), Cover) + Format(", %.0f silhouettes", Silhouettes),
                   std::max(RadiusError(Caps, { 0.3, 1.7 }, 0.3), std::fabs(Cover - 0.5 * Pi)), !Caps.empty() && RadiusError(Caps, { 0.3, 1.7 }, 0.3) <= 1.0e-12 && std::fabs(Cover - 0.5 * Pi) <= 1.0e-12 && Silhouettes == 0.0);
            WriteSvg("BoxEdgeFilletRight", Right);
        }
    }

    void ValidateRefusals()
    {
        ModelPartition  Partition;
        const BodyToken Bracket = ConstructBracket(Partition);
        const auto Expect = [&](const std::string& Name, const DrawingView& View, const char* Fragment)
        {
            Report(RefuseGate, Name, View.Refusal.empty() ? std::string("not refused") : View.Refusal, 0.0, View.Refusal.find(Fragment) != std::string::npos);
        };
        Expect("No bodies", ProjectDrawingView(Partition, {}, Standard(DrawingStandardView::Top)), "no bodies");
        DrawingViewRequest Skewed = Standard(DrawingStandardView::Top);
        Skewed.Frame.XAxis = { 1.0, 0.1, 0.0 };
        Expect("Skewed frame", ProjectDrawingView(Partition, { Bracket }, Skewed), "not a right-handed orthonormal");
        DrawingViewRequest Mirrored = Standard(DrawingStandardView::Top);
        Mirrored.Frame.ZAxis = { 0.0, 0.0, -1.0 };
        Expect("Left-handed frame", ProjectDrawingView(Partition, { Bracket }, Mirrored), "not a right-handed orthonormal");
        DrawingViewRequest Beyond = Standard(DrawingStandardView::Front);
        Beyond.Section = OrthonormalFrame{ { 0.0, 10.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, -1.0, 0.0 } };
        Expect("Plane beyond the body", ProjectDrawingView(Partition, { Bracket }, Beyond), "misses every body");
        DrawingViewRequest Before = Standard(DrawingStandardView::Front);
        Before.Section = OrthonormalFrame{ { 0.0, -5.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, -1.0, 0.0 } };
        Expect("Plane in front of the body", ProjectDrawingView(Partition, { Bracket }, Before), "misses every body");
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENTRY POINT
//------------------------------------------------------------------------------------------------------------------------

int main(int Count, char** Arguments)
{
    std::string Only;
    for (int Index = 1; Index + 1 < Count; ++Index)
    {
        if (std::string(Arguments[Index]) == "--only") Only = Arguments[Index + 1];
        if (std::string(Arguments[Index]) == "--svg") SvgFolder = Arguments[Index + 1];
    }
    const auto Start = std::chrono::steady_clock::now();
    if (Only.empty() || Only == "block") ValidateBlock();
    if (Only.empty() || Only == "hole") ValidateHole();
    if (Only.empty() || Only == "silhouette") ValidateSilhouettes();
    if (Only.empty() || Only == "occlusion") ValidateOcclusion();
    if (Only.empty() || Only == "section") ValidateSection();
    if (Only.empty() || Only == "tangent") ValidateTangent();
    if (Only.empty() || Only == "refusal") ValidateRefusals();
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();

    bool PassedCondition = true;
    for (const GateTally* Gate : AllGates()) PassedCondition = PassedCondition && Gate->Failures == 0 && (Gate->Checks > 0 || !Only.empty());
    std::printf("# Drawing validation\n\n%s Priority 11 D1: orthographic views, exact silhouettes, ray-cast hidden lines and section views with hatching (%.2f s)\n\n", PassedCondition ? "🟢" : "🔴", Seconds);
    std::printf("## Gates\n\n| | Gate | Measures | Limit | Worst | Checks | Failures |\n|---|---|---|---|---|---|---|\n");
    for (const GateTally* Gate : AllGates())
    {
        std::printf("| %s | %s | %s | %s | %.2e | %ld | %ld |\n", Gate->Failures == 0 && Gate->Checks > 0 ? "🟢" : "🔴", Gate->Code, Gate->Subject, Gate->Limit, Gate->Worst, Gate->Checks, Gate->Failures);
    }
    std::printf("\n## Checks\n\n| | Gate | Check | Detail |\n|---|---|---|---|\n");
    const auto Cell = [](std::string Text)                                                   // Details carry |x| and the like
    {
        for (size_t At = Text.find('|'); At != std::string::npos; At = Text.find('|', At + 2)) Text.replace(At, 1, "\\|");
        return Text;
    };
    for (const DetailRow& Row : Details) std::printf("| %s | %s | %s | %s |\n", Row.PassedCondition ? "🟢" : "🔴", Row.Gate.c_str(), Cell(Row.Name).c_str(), Cell(Row.Detail).c_str());
    return PassedCondition ? 0 : 1;
}
