/*====================================================================================================================================
                                                        PANELASSEMBLY.CPP
====================================================================================================================================*/
// 🧩 Panel sewing: corner merging, point-for-point boundary pairing, rim loops, plane caps, outward side by signed volume

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Surfacing/PanelAssembly.h"
#include "Construction/BodyAssembly.h"
#include "Geometry/ProjectedCurve.h"
#include "Surfacing/SurfaceAlgebra.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIVATE FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr int    BoundarySamples = 17;                                                   // [-]   - Points compared along two boundaries
    constexpr int    RimSamples      = 64;                                                   // [-]   - Points per rim edge (planarity, area)
    constexpr double SeamGap         = 1.0e-9;                                               // [len] - First = last pole column: closed in 𝑢
    constexpr std::array<double, 6> GaussAbscissae = { -0.9324695142031521, -0.6612093864662645, -0.2386191860831969, 0.2386191860831969, 0.6612093864662645, 0.9324695142031521 };
    constexpr std::array<double, 6> GaussWeights   = { 0.1713244923791704, 0.3607615730481386, 0.4679139345726910, 0.4679139345726910, 0.3607615730481386, 0.1713244923791704 };

    [[nodiscard]] std::string Millimetres(double Value)
    {
        char Text[48];
        std::snprintf(Text, sizeof Text, "%.3g mm", Value * 1000.0);
        return Text;
    }

    // 📝 One boundary of one panel: its iso-curve (τ along the iso parameter), chart line, and the direction the panel's
    //    counter-clockwise loop walks it (bottom and right with τ, top and left against).
    struct PanelSide
    {
        int           Panel     = 0;                                                         // [-] - Panel index
        NurbsCurve    Curve     = {};                                                        // [-] - Iso-curve, τ ∈ [0, 1]
        ParameterLine Chart     = {};                                                        // [-] - τ → (𝑢, 𝑣)
        Orientation   LoopSense = Orientation::Forward;                                      // [-] - Loop direction vs τ
        int           Start     = -1;                                                        // [-] - Vertex at τ = 0
        int           End       = -1;                                                        // [-] - Vertex at τ = 1
        int           Edge      = -1;                                                        // [-] - Blueprint edge
        bool          ReversedCondition = false;                                             // [-] - Edge τ = 1 − side τ
        bool          SeamCondition     = false;                                             // [-] - Closed panel's seam (paired with itself)
        int           SeamOf            = -1;                                                // [-] - Return use of this seam side
    };

    [[nodiscard]] Orientation Opposite(Orientation Sense) { return Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward; }

    [[nodiscard]] Vector3 CurvePoint(const NurbsCurve& Curve, double Parameter) { return EvaluateNurbsCurveDerivatives(*Curve.Definition, Parameter).Position; }

    // ∫∫ ⅓ S·(S𝑢 × S𝑣) d𝑢 d𝑣 over the panel (6-point Gauss per knot span): its share of the enclosed volume.
    [[nodiscard]] double PanelFlux(const NurbsSurfaceDefinition& Definition)
    {
        auto Breaks = [](const std::vector<double>& Knots)
        {
            std::vector<double> Values;
            for (double Knot : Knots) if (Values.empty() || Knot > Values.back()) Values.push_back(Knot);
            return Values;
        };
        const std::vector<double> BreaksU = Breaks(Definition.KnotsU), BreaksV = Breaks(Definition.KnotsV);
        double Flux = 0.0;
        for (size_t SpanU = 0; SpanU + 1 < BreaksU.size(); ++SpanU)
        {
            for (size_t SpanV = 0; SpanV + 1 < BreaksV.size(); ++SpanV)
            {
                const double HalfU = 0.5 * (BreaksU[SpanU + 1] - BreaksU[SpanU]), HalfV = 0.5 * (BreaksV[SpanV + 1] - BreaksV[SpanV]);
                for (size_t IndexU = 0; IndexU < GaussAbscissae.size(); ++IndexU)
                {
                    for (size_t IndexV = 0; IndexV < GaussAbscissae.size(); ++IndexV)
                    {
                        const Vector2 Parameter = { BreaksU[SpanU] + HalfU * (1.0 + GaussAbscissae[IndexU]), BreaksV[SpanV] + HalfV * (1.0 + GaussAbscissae[IndexV]) };
                        const NurbsSurfaceDerivatives Sample = EvaluateNurbsSurfaceDerivatives(Definition, Parameter);
                        Flux += GaussWeights[IndexU] * GaussWeights[IndexV] * HalfU * HalfV * Dot(Sample.Position, Cross(Sample.U, Sample.V)) / 3.0;
                    }
                }
            }
        }
        return Flux;
    }

    struct RimLoop
    {
        std::vector<int>     Sides  = {};                                                    // [-]   - PanelSide indices in cap order
        std::vector<Vector3> Points = {};                                                    // [len] - Samples in cap order
    };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

SurfacePanel MirrorPanel(const SurfacePanel& Panel, Vector3 PlanePoint, Vector3 PlaneNormal)
{
    const Vector3 Normal = PlaneNormal * (1.0 / Length(PlaneNormal));
    const NurbsSurface Image = MapNurbsSurface(*Panel.Surface.Definition, [&](Vector3 Point) { return Point - Normal * (2.0 * Dot(Point - PlanePoint, Normal)); });
    const std::optional<NurbsSurface> Turned = FinishNurbsSurface(ReverseNurbsSurface(*Image.Definition, true));
    return SurfacePanel{ Turned ? *Turned : Image };
}

PanelAssemblyOutcome AssemblePanels(ModelPartition& Partition, const std::vector<SurfacePanel>& Input, const PanelAssemblySettings& Settings, const std::string& Label)
{
    PanelAssemblyOutcome Outcome;
    auto Refuse = [&](const std::string& Reason) { Outcome.Refusal = Label + ": " + Reason; return Outcome; };
    if (Input.empty()) return Refuse("no panels");
    std::vector<SurfacePanel> Panels = Input;

    for (int Attempt = 0; Attempt < 2; ++Attempt)
    {
        // ① Sides and merged corners.
        std::vector<Vector3>   Vertices;
        std::vector<PanelSide> Sides;
        std::vector<std::vector<int>> Loops(Panels.size());                                  // Side indices per panel loop, in loop order
        auto VertexAt = [&](Vector3 Position)
        {
            for (size_t Index = 0; Index < Vertices.size(); ++Index) if (Length(Vertices[Index] - Position) <= Settings.SewingGap) return static_cast<int>(Index);
            Vertices.push_back(Position);
            return static_cast<int>(Vertices.size()) - 1;
        };
        for (size_t Panel = 0; Panel < Panels.size(); ++Panel)
        {
            if (!Panels[Panel].Surface.Definition) return Refuse("panel " + std::to_string(Panel) + " is empty");
            const NurbsSurface& Surface = Panels[Panel].Surface;
            const NurbsSurfaceDefinition& Definition = *Surface.Definition;
            const NurbsDomain DomainU = MeasureNurbsSurfaceDomain(Definition, true), DomainV = MeasureNurbsSurfaceDomain(Definition, false);
            if (DomainU.Low != 0.0 || DomainU.High != 1.0 || DomainV.Low != 0.0 || DomainV.High != 1.0) return Refuse("panel " + std::to_string(Panel) + " is not on [0, 1]²");
            bool ClosedCondition = true;
            for (int Row = 0; Row < Definition.CountV; ++Row)
            {
                ClosedCondition = ClosedCondition && Length(Definition.Poles[Row] - Definition.Poles[static_cast<size_t>(Definition.CountU - 1) * Definition.CountV + Row]) <= SeamGap;
            }
            auto AddSide = [&](bool FixedU, double Value, ParameterLine Chart, Orientation Sense)
            {
                PanelSide Side;
                Side.Panel     = static_cast<int>(Panel);
                Side.Curve     = ExtractNurbsIsoCurve(Surface, FixedU, Value);
                Side.Chart     = Chart;
                Side.LoopSense = Sense;
                Side.Start     = VertexAt(CurvePoint(Side.Curve, 0.0));
                Side.End       = VertexAt(CurvePoint(Side.Curve, 1.0));
                Sides.push_back(std::move(Side));
                return static_cast<int>(Sides.size()) - 1;
            };
            const int Bottom = AddSide(false, 0.0, { { 0.0, 0.0 }, { 1.0, 0.0 } }, Orientation::Forward);
            const int Top    = AddSide(false, 1.0, { { 0.0, 1.0 }, { 1.0, 0.0 } }, Orientation::Reversed);
            if (ClosedCondition)
            {
                // The seam: one side used forward on the 𝑢 = 1 chart line and reversed on the 𝑢 = 0 line.
                const int Seam = AddSide(true, 0.0, { { 1.0, 0.0 }, { 0.0, 1.0 } }, Orientation::Forward);
                Sides[Seam].SeamCondition = true;
                PanelSide Return = Sides[Seam];
                Return.Chart         = { { 0.0, 0.0 }, { 0.0, 1.0 } };
                Return.LoopSense     = Orientation::Reversed;
                Return.SeamCondition = false;
                Return.SeamOf        = Seam;
                Sides.push_back(Return);
                Loops[Panel] = { Bottom, Seam, Top, static_cast<int>(Sides.size()) - 1 };
            }
            else
            {
                const int Right = AddSide(true, 1.0, { { 1.0, 0.0 }, { 0.0, 1.0 } }, Orientation::Forward);
                const int Left  = AddSide(true, 0.0, { { 0.0, 0.0 }, { 0.0, 1.0 } }, Orientation::Reversed);
                Loops[Panel] = { Bottom, Right, Top, Left };
            }
            for (int Side : Loops[Panel])
            {
                if (Sides[Side].Start == Sides[Side].End && !ClosedCondition)
                    return Refuse("panel " + std::to_string(Panel) + " has a collapsed boundary (degenerate corners 🚧)");
            }
        }

        // ② Pair boundaries: same corners, point-for-point at τ or 1 − τ.
        BodyBlueprint Blueprint;
        for (const Vector3& Position : Vertices) Blueprint.Vertices.push_back({ Position });
        std::vector<int> EdgeUses;
        Outcome.SewnEdges = 0;
        for (size_t Index = 0; Index < Sides.size(); ++Index)
        {
            PanelSide& Side = Sides[Index];
            if (Side.SeamOf >= 0) { Side.Edge = Sides[Side.SeamOf].Edge; continue; }      // Seam use counted with the seam
            for (size_t Other = 0; Other < Index && !Side.SeamCondition; ++Other)
            {
                const PanelSide& Candidate = Sides[Other];
                if (Candidate.SeamCondition || Candidate.SeamOf >= 0 || Candidate.Edge < 0 || Candidate.ReversedCondition) continue;
                const bool SameCondition = Candidate.Start == Side.Start && Candidate.End == Side.End;
                const bool SwapCondition = Candidate.Start == Side.End && Candidate.End == Side.Start;
                if (!SameCondition && !SwapCondition) continue;
                double Worst = 0.0, Geometric = 0.0;
                for (int Sample = 0; Sample <= BoundarySamples; ++Sample)
                {
                    const double Tau = static_cast<double>(Sample) / BoundarySamples;
                    const Vector3 Point = CurvePoint(Side.Curve, Tau);
                    Worst = std::max(Worst, Length(Point - CurvePoint(Candidate.Curve, SameCondition ? Tau : 1.0 - Tau)));
                    Geometric = std::max(Geometric, Length(Point - CurvePoint(Candidate.Curve, ProjectOntoNurbsCurve(*Candidate.Curve.Definition, Point, 0.0))));
                }
                if (Worst > Settings.SewingGap)
                {
                    if (Geometric <= Settings.SewingGap)
                        return Refuse("panels " + std::to_string(Candidate.Panel) + " and " + std::to_string(Side.Panel) + " share a boundary parametrised differently (off by "
                                      + Millimetres(Worst) + " at equal τ; reparametrised sewing 🚧)");
                    continue;
                }
                if (EdgeUses[Candidate.Edge] >= 2) return Refuse("a boundary of panel " + std::to_string(Side.Panel) + " is claimed by a third panel (non-manifold)");
                Side.Edge = Candidate.Edge;
                Side.ReversedCondition = SwapCondition;
                ++EdgeUses[Side.Edge];
                ++Outcome.SewnEdges;
                break;
            }
            if (Side.Edge >= 0) continue;
            BlueprintEdge Edge;
            Edge.Curve = Side.Curve;
            Edge.Low = 0.0; Edge.High = 1.0; Edge.Start = Side.Start; Edge.End = Side.End;
            Blueprint.Edges.push_back(std::move(Edge));
            Side.Edge = static_cast<int>(Blueprint.Edges.size()) - 1;
            EdgeUses.push_back(1);
            if (Side.SeamCondition) ++EdgeUses[Side.Edge];                                   // Its return use is the next side
        }
        // ③ Uses: an edge running against its side flips the sense and the chart line.
        auto UseOf = [&](const PanelSide& Side)
        {
            BlueprintUse Use;
            Use.Edge = Side.Edge;
            if (!Side.ReversedCondition) { Use.Sense = Side.LoopSense; Use.SurfaceCurve = Side.Chart; return Use; }
            Use.Sense        = Opposite(Side.LoopSense);
            Use.SurfaceCurve = ParameterLine{ Side.Chart.Origin + Side.Chart.Direction, Side.Chart.Direction * -1.0 };
            return Use;
        };
        for (size_t Panel = 0; Panel < Panels.size(); ++Panel)
        {
            BlueprintFace Face;
            Face.Surface = Panels[Panel].Surface;
            Face.Sense   = Orientation::Forward;
            std::vector<BlueprintUse> Loop;
            for (int Side : Loops[Panel]) Loop.push_back(UseOf(Sides[Side]));
            Face.Loops.push_back(std::move(Loop));
            Blueprint.Faces.push_back(std::move(Face));
        }

        // ④ Rim: edges with one use. Each rim use is walked by the cap the other way: chain them into loops.
        std::vector<int> RimSides;
        for (size_t Index = 0; Index < Sides.size(); ++Index) if (EdgeUses[Sides[Index].Edge] == 1) RimSides.push_back(static_cast<int>(Index));
        Outcome.LaminarEdges = static_cast<int>(RimSides.size());
        Outcome.Caps.clear();
        Outcome.SolidCondition = false;
        if (!Settings.CapCondition || RimSides.empty())
        {
            Blueprint.Category = RimSides.empty() ? BodyCategory::Solid : BodyCategory::Sheet;
            if (Blueprint.Category == BodyCategory::Solid)
            {
                double Flux = 0.0;
                for (const SurfacePanel& Panel : Panels) Flux += PanelFlux(*Panel.Surface.Definition);
                if (Flux < 0.0 && Attempt == 0) { for (SurfacePanel& Panel : Panels) Panel = SurfacePanel{ *FinishNurbsSurface(ReverseNurbsSurface(*Panel.Surface.Definition, true)) }; Outcome.FlippedCondition = true; continue; }
                Outcome.Volume = Flux;
            }
            const AssemblyOutcome Assembly = AssembleBody(Partition, Blueprint, Label);
            if (!Assembly.Refusal.empty()) { Outcome.Refusal = Assembly.Refusal; return Outcome; }
            Outcome.Body  = Assembly.Body;
            Outcome.Faces = Assembly.Faces;
            Outcome.SolidCondition = Blueprint.Category == BodyCategory::Solid;
            return Outcome;
        }
        // Cap direction of a rim side: from the vertex its panel loop reaches last to the one it leaves first.
        auto CapStart = [&](const PanelSide& Side) { return Side.LoopSense == Orientation::Forward ? Side.End : Side.Start; };
        auto CapEnd   = [&](const PanelSide& Side) { return Side.LoopSense == Orientation::Forward ? Side.Start : Side.End; };
        std::map<int, int> Leaving;
        for (int Side : RimSides)
        {
            if (Leaving.count(CapStart(Sides[Side]))) return Refuse("the rim branches at a corner (" + std::to_string(CapStart(Sides[Side])) + ")");
            Leaving[CapStart(Sides[Side])] = Side;
        }
        std::vector<RimLoop> Rims;
        std::vector<bool> Walked(Sides.size(), false);
        for (int First : RimSides)
        {
            if (Walked[First]) continue;
            RimLoop Rim;
            int Current = First;
            while (!Walked[Current])
            {
                Walked[Current] = true;
                Rim.Sides.push_back(Current);
                const PanelSide& Side = Sides[Current];
                for (int Sample = 0; Sample < RimSamples; ++Sample)
                {
                    const double Tau = static_cast<double>(Sample) / RimSamples;
                    Rim.Points.push_back(CurvePoint(Side.Curve, Side.LoopSense == Orientation::Forward ? 1.0 - Tau : Tau));
                }
                const auto Next = Leaving.find(CapEnd(Side));
                if (Next == Leaving.end()) return Refuse("the rim does not close into loops");
                Current = Next->second;
            }
            if (Current != First) return Refuse("the rim does not close into loops");
            Rims.push_back(std::move(Rim));
        }

        // ⑤ Plane caps: Newell normal of the cap-direction polygon is the outward normal; planarity within PlanarGap.
        double Flux = 0.0;
        for (const SurfacePanel& Panel : Panels) Flux += PanelFlux(*Panel.Surface.Definition);
        std::vector<OrthonormalFrame> Frames;
        for (size_t Index = 0; Index < Rims.size(); ++Index)
        {
            const RimLoop& Rim = Rims[Index];
            Vector3 Area = {}, Centre = {};
            for (size_t Point = 0; Point < Rim.Points.size(); ++Point)
            {
                Area   = Area + Cross(Rim.Points[Point], Rim.Points[(Point + 1) % Rim.Points.size()]) * 0.5;
                Centre = Centre + Rim.Points[Point] * (1.0 / Rim.Points.size());
            }
            if (!(Length(Area) > 0.0)) return Refuse("rim loop " + std::to_string(Index) + " encloses no area");
            const Vector3 Normal = Area * (1.0 / Length(Area));
            double OffPlane = 0.0;
            for (const Vector3& Point : Rim.Points) OffPlane = std::max(OffPlane, std::abs(Dot(Point - Centre, Normal)));
            if (OffPlane > Settings.PlanarGap) return Refuse("rim loop " + std::to_string(Index) + " is not planar (off by " + Millimetres(OffPlane) + "; only plane caps 🚧)");
            Flux += Dot(Centre, Area) / 3.0;                                                 // Plane face: ⅓ (n·p)·A with the loop's own area vector
            const Vector3 Reference = std::abs(Normal.X) < 0.9 ? Vector3{ 1.0, 0.0, 0.0 } : Vector3{ 0.0, 1.0, 0.0 };
            Frames.push_back(ConstructFrame(Centre, Normal, Reference));
        }
        if (Flux < 0.0 && Attempt == 0)
        {
            for (SurfacePanel& Panel : Panels) Panel = SurfacePanel{ *FinishNurbsSurface(ReverseNurbsSurface(*Panel.Surface.Definition, true)) };
            Outcome.FlippedCondition = true;
            continue;
        }
        if (Flux < 0.0) return Refuse("the closed panels enclose a negative volume after turning (inconsistent panel normals)");
        Outcome.Volume = Flux;
        for (size_t Index = 0; Index < Rims.size(); ++Index)
        {
            BlueprintFace Cap;
            const PlaneSurface Plane{ Frames[Index] };
            Cap.Surface = Plane;
            Cap.Sense   = Orientation::Forward;
            std::vector<BlueprintUse> Loop;
            for (int SideIndex : Rims[Index].Sides)
            {
                const PanelSide& Side = Sides[SideIndex];
                BlueprintUse Use;
                Use.Edge  = Side.Edge;
                Use.Sense = Opposite(UseOf(Side).Sense);
                const BlueprintEdge& Edge = Blueprint.Edges[Side.Edge];
                Use.SurfaceCurve = FitSurfaceCurve(SurfaceGeometry{ Plane }, Edge.Curve, Edge.Low, Edge.High, { { -1.0e6, -1.0e6 }, { 1.0e6, 1.0e6 } });
                Loop.push_back(std::move(Use));
            }
            Cap.Loops.push_back(std::move(Loop));
            Blueprint.Faces.push_back(std::move(Cap));
        }
        Blueprint.Category = BodyCategory::Solid;
        const AssemblyOutcome Assembly = AssembleBody(Partition, Blueprint, Label);
        if (!Assembly.Refusal.empty()) { Outcome.Refusal = Assembly.Refusal; return Outcome; }
        Outcome.Body = Assembly.Body;
        Outcome.Faces.assign(Assembly.Faces.begin(), Assembly.Faces.begin() + static_cast<long>(Panels.size()));
        Outcome.Caps.assign(Assembly.Faces.begin() + static_cast<long>(Panels.size()), Assembly.Faces.end());
        Outcome.SolidCondition = true;
        return Outcome;
    }
    return Refuse("orientation did not settle");
}

} // namespace Frontier::SolidArcRemastered
