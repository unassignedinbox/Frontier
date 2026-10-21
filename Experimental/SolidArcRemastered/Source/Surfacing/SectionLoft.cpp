/*====================================================================================================================================
                                                           SECTIONLOFT.CPP
====================================================================================================================================*/
// 🧩 Loft through sections: knot merging, chord-averaged 𝑣ₖ, homogeneous interpolation, sheet assembly with iso-line charts

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Surfacing/SectionLoft.h"
#include "Surfacing/LinearSystem.h"
#include "Construction/BodyAssembly.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIVATE FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double ClosureGap = 1.0e-9;                                                    // [len] - First = last pole: a closed section

    // Affine τ′ = (τ − Low) / (High − Low): the same curve on [0, 1] (knots move, poles and weights stay).
    [[nodiscard]] NurbsCurveDefinition MapOntoUnit(const NurbsCurveDefinition& Definition)
    {
        NurbsCurveDefinition Result = Definition;
        const NurbsDomain Domain = MeasureNurbsCurveDomain(Definition);
        const double Width = Domain.High - Domain.Low;
        for (double& Knot : Result.Knots) Knot = (Knot - Domain.Low) / Width;
        for (int Index = 0; Index <= Result.Degree; ++Index)
        {
            Result.Knots[Index] = 0.0;
            Result.Knots[Result.Knots.size() - 1 - Index] = 1.0;
        }
        return Result;
    }

    // Interior knots with multiplicity, value → count.
    [[nodiscard]] std::map<double, int> InteriorKnots(const NurbsCurveDefinition& Definition)
    {
        std::map<double, int> Counts;
        for (size_t Index = Definition.Degree + 1; Index + Definition.Degree + 1 < Definition.Knots.size(); ++Index) ++Counts[Definition.Knots[Index]];
        return Counts;
    }

    [[nodiscard]] bool ClosedSection(const NurbsCurveDefinition& Definition)
    {
        return Length(Definition.Poles.front() - Definition.Poles.back()) <= ClosureGap;
    }

    // P&T (9.8): 𝑣-knots by averaging q consecutive parameters (clamped, Schoenberg–Whitney holds by construction).
    [[nodiscard]] std::vector<double> AveragedKnots(const std::vector<double>& Parameters, int Degree)
    {
        const int Count = static_cast<int>(Parameters.size());
        std::vector<double> Knots(Degree + 1, 0.0);
        for (int Index = 1; Index <= Count - Degree - 1; ++Index)
        {
            double Sum = 0.0;
            for (int Offset = Index; Offset < Index + Degree; ++Offset) Sum += Parameters[Offset];
            Knots.push_back(Sum / Degree);
        }
        Knots.insert(Knots.end(), Degree + 1, 1.0);
        return Knots;
    }

    [[nodiscard]] ParameterLine IsoLine(Vector2 Origin, Vector2 Direction) { return ParameterLine{ Origin, Direction }; }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

LoftSurfaceOutcome LoftSectionSurface(const std::vector<NurbsCurve>& Sections, const LoftSettings& Settings)
{
    LoftSurfaceOutcome Outcome;
    auto Refuse = [&](const std::string& Reason) { Outcome.Refusal = "Loft: " + Reason; return Outcome; };
    if (Sections.size() < 2) return Refuse("needs at least 2 sections");

    // ① Each section on [0, 1]; same degree; all open or all closed; open ones aligned with the first.
    std::vector<NurbsCurveDefinition> Mapped;
    for (size_t Index = 0; Index < Sections.size(); ++Index)
    {
        if (!Sections[Index].Definition) return Refuse("section " + std::to_string(Index) + " is empty");
        // A closed carrier flagged periodic but with clamped ends (e.g. a full circle) is used as a closed clamped section.
        const NurbsCurveDefinition& Definition = *Sections[Index].Definition;
        const size_t Last = Definition.Knots.size() - 1;
        const bool   Clamped = Definition.Knots[0] == Definition.Knots[Definition.Degree] && Definition.Knots[Last] == Definition.Knots[Last - Definition.Degree];
        if (Definition.Period > 0.0 && !Clamped) return Refuse("section " + std::to_string(Index) + " is periodic with unclamped knots (🚧)");
        Mapped.push_back(MapOntoUnit(Definition));
        Mapped.back().Period = 0.0;
    }
    const int Degree = Mapped.front().Degree;
    for (size_t Index = 1; Index < Mapped.size(); ++Index)
    {
        if (Mapped[Index].Degree != Degree)
            return Refuse("sections of degree " + std::to_string(Degree) + " and " + std::to_string(Mapped[Index].Degree) + " (degree elevation 🚧)");
    }
    Outcome.ClosedCondition = ClosedSection(Mapped.front());
    for (size_t Index = 1; Index < Mapped.size(); ++Index)
    {
        if (ClosedSection(Mapped[Index]) != Outcome.ClosedCondition) return Refuse("open and closed sections mixed (section " + std::to_string(Index) + ")");
    }
    if (Settings.AlignCondition && !Outcome.ClosedCondition)
    {
        const Vector3 Reference = Mapped.front().Poles.back() - Mapped.front().Poles.front();
        for (size_t Index = 1; Index < Mapped.size(); ++Index)
        {
            const Vector3 Chord = Mapped[Index].Poles.back() - Mapped[Index].Poles.front();
            if (Dot(Chord, Reference) >= 0.0) continue;
            Mapped[Index] = MapOntoUnit(*ReverseNurbsCurve(Mapped[Index]).Definition);
            ++Outcome.Reversed;
        }
    }

    // ② Merge the knot vectors: every section gets the union (largest multiplicity of each value).
    std::map<double, int> Union;
    for (const NurbsCurveDefinition& Definition : Mapped)
    {
        for (const auto& [Value, Count] : InteriorKnots(Definition)) Union[Value] = std::max(Union[Value], Count);
    }
    for (NurbsCurveDefinition& Definition : Mapped)
    {
        const std::map<double, int> Present = InteriorKnots(Definition);
        std::vector<double> Missing;
        for (const auto& [Value, Count] : Union)
        {
            const auto Found = Present.find(Value);
            for (int Copy = Found == Present.end() ? 0 : Found->second; Copy < Count; ++Copy) Missing.push_back(Value);
        }
        Definition = RefineNurbsCurveKnots(Definition, Missing);
    }
    const int CountU = static_cast<int>(Mapped.front().Poles.size());
    for (const NurbsCurveDefinition& Definition : Mapped)
    {
        if (static_cast<int>(Definition.Poles.size()) != CountU) return Refuse("knot merging failed (a knot exceeds multiplicity p)");
    }

    // ③ 𝑣ₖ: chord length along each pole column, normalised and averaged over the columns that move.
    const int CountV = static_cast<int>(Mapped.size());
    std::vector<double> Parameters(CountV, 0.0);
    int Columns = 0;
    for (int Column = 0; Column < CountU; ++Column)
    {
        std::vector<double> Cumulative(CountV, 0.0);
        for (int Row = 1; Row < CountV; ++Row) Cumulative[Row] = Cumulative[Row - 1] + Length(Mapped[Row].Poles[Column] - Mapped[Row - 1].Poles[Column]);
        if (!(Cumulative.back() > ClosureGap)) continue;
        for (int Row = 1; Row < CountV; ++Row) Parameters[Row] += Cumulative[Row] / Cumulative.back();
        ++Columns;
    }
    if (Columns == 0) return Refuse("all sections coincide");
    for (double& Parameter : Parameters) Parameter /= Columns;
    Parameters.back() = 1.0;
    for (int Row = 1; Row < CountV; ++Row)
    {
        if (!(Parameters[Row] > Parameters[Row - 1] + 1.0e-12)) return Refuse("sections " + std::to_string(Row - 1) + " and " + std::to_string(Row) + " coincide");
    }

    // ④ Interpolate every pole column through the sections in homogeneous form (w·P, w).
    const int DegreeV = std::clamp(Settings.DegreeAcross, 1, std::min(NurbsDegreeLimit, CountV - 1));
    const std::vector<double> KnotsV = AveragedKnots(Parameters, DegreeV);
    DenseMatrix Matrix(CountV, CountV), RightSides(CountV, 4 * CountU);
    for (int Row = 0; Row < CountV; ++Row)
    {
        const NurbsBasisSample Basis = EvaluateNurbsBasis(KnotsV, DegreeV, CountV, Parameters[Row]);
        for (int Column = 0; Column <= DegreeV; ++Column) Matrix.At(Row, Basis.First + Column) = Basis.Values[0][Column];
        for (int Pole = 0; Pole < CountU; ++Pole)
        {
            const double  Weight = Mapped[Row].Weights[Pole];
            const Vector3 Point  = Mapped[Row].Poles[Pole];
            RightSides.At(Row, 4 * Pole + 0) = Weight * Point.X;
            RightSides.At(Row, 4 * Pole + 1) = Weight * Point.Y;
            RightSides.At(Row, 4 * Pole + 2) = Weight * Point.Z;
            RightSides.At(Row, 4 * Pole + 3) = Weight;
        }
    }
    const std::optional<DenseMatrix> Solution = SolveDenseSystem(std::move(Matrix), std::move(RightSides));
    if (!Solution) return Refuse("singular interpolation across the sections");
    std::vector<Vector3> Poles(static_cast<size_t>(CountU) * CountV);
    std::vector<double>  Weights(Poles.size());
    for (int Pole = 0; Pole < CountU; ++Pole)
    {
        for (int Row = 0; Row < CountV; ++Row)
        {
            const double Weight = Solution->At(Row, 4 * Pole + 3);
            if (!(Weight > 0.0)) return Refuse("interpolated weight ≤ 0 at pole column " + std::to_string(Pole) + " (sections with very different weights)");
            const size_t Index = static_cast<size_t>(Pole) * CountV + Row;
            Poles[Index]   = Vector3{ Solution->At(Row, 4 * Pole + 0), Solution->At(Row, 4 * Pole + 1), Solution->At(Row, 4 * Pole + 2) } * (1.0 / Weight);
            Weights[Index] = Weight;
        }
    }
    const std::optional<NurbsSurface> Surface = ConstructNurbsSurface(Degree, DegreeV, CountU, CountV, Mapped.front().Knots, KnotsV, std::move(Poles), std::move(Weights));
    if (!Surface) return Refuse("surface construction refused the skinned net");
    Outcome.Surface           = *Surface;
    Outcome.SectionParameters = Parameters;
    for (const NurbsCurveDefinition& Definition : Mapped) Outcome.Sections.push_back(NurbsCurve{ std::make_shared<NurbsCurveDefinition>(Definition) });
    return Outcome;
}

LoftSheetOutcome LoftSectionSheet(ModelPartition& Partition, const std::vector<NurbsCurve>& Sections, const LoftSettings& Settings)
{
    LoftSheetOutcome Outcome;
    Outcome.Loft = LoftSectionSurface(Sections, Settings);
    if (!Outcome.Loft.Refusal.empty()) { Outcome.Sheet.Refusal = Outcome.Loft.Refusal; return Outcome; }
    const NurbsSurface& Surface = Outcome.Loft.Surface;

    // 📝 Chart (𝑢, 𝑣) ∈ [0, 1]²; boundary edges are exact iso-curves sharing the chart parameter, so each SP-curve is a line:
    //    bottom 𝑣 = 0 and top 𝑣 = 1 run in 𝑢, the rails 𝑢 = 0 / 𝑢 = 1 run in 𝑣. Loop CCW in the chart (Forward face).
    BodyBlueprint Blueprint;
    Blueprint.Category = BodyCategory::Sheet;
    auto Corner = [&](double U, double V) { return EvaluateSurface(SurfaceGeometry{ Surface }, { U, V }).Position; };
    auto AddEdge = [&](bool FixedU, double Value, int Start, int End)
    {
        BlueprintEdge Edge;
        Edge.Curve = ExtractNurbsIsoCurve(Surface, FixedU, Value);
        Edge.Low = 0.0; Edge.High = 1.0; Edge.Start = Start; Edge.End = End;
        Blueprint.Edges.push_back(std::move(Edge));
        return static_cast<int>(Blueprint.Edges.size()) - 1;
    };
    auto Use = [](int Edge, Orientation Sense, ParameterLine Chart) { BlueprintUse Usage; Usage.Edge = Edge; Usage.Sense = Sense; Usage.SurfaceCurve = Chart; return Usage; };
    BlueprintFace Face;
    Face.Surface = Surface;
    Face.Sense   = Orientation::Forward;
    if (Outcome.Loft.ClosedCondition)
    {
        Blueprint.Vertices = { { Corner(0.0, 0.0) }, { Corner(0.0, 1.0) } };
        const int Bottom = AddEdge(false, 0.0, 0, 0), Top = AddEdge(false, 1.0, 1, 1), Seam = AddEdge(true, 0.0, 0, 1);
        Face.Loops.push_back({ Use(Bottom, Orientation::Forward,  IsoLine({ 0.0, 0.0 }, { 1.0, 0.0 })),
                               Use(Seam,   Orientation::Forward,  IsoLine({ 1.0, 0.0 }, { 0.0, 1.0 })),
                               Use(Top,    Orientation::Reversed, IsoLine({ 0.0, 1.0 }, { 1.0, 0.0 })),
                               Use(Seam,   Orientation::Reversed, IsoLine({ 0.0, 0.0 }, { 0.0, 1.0 })) });
    }
    else
    {
        Blueprint.Vertices = { { Corner(0.0, 0.0) }, { Corner(1.0, 0.0) }, { Corner(1.0, 1.0) }, { Corner(0.0, 1.0) } };
        const int Bottom = AddEdge(false, 0.0, 0, 1), Right = AddEdge(true, 1.0, 1, 2), Top = AddEdge(false, 1.0, 3, 2), Left = AddEdge(true, 0.0, 0, 3);
        Face.Loops.push_back({ Use(Bottom, Orientation::Forward,  IsoLine({ 0.0, 0.0 }, { 1.0, 0.0 })),
                               Use(Right,  Orientation::Forward,  IsoLine({ 1.0, 0.0 }, { 0.0, 1.0 })),
                               Use(Top,    Orientation::Reversed, IsoLine({ 0.0, 1.0 }, { 1.0, 0.0 })),
                               Use(Left,   Orientation::Reversed, IsoLine({ 0.0, 0.0 }, { 0.0, 1.0 })) });
    }
    Blueprint.Faces.push_back(std::move(Face));
    const AssemblyOutcome Assembly = AssembleBody(Partition, Blueprint, "Loft");
    if (!Assembly.Refusal.empty()) { Outcome.Sheet.Refusal = "Loft: " + Assembly.Refusal; return Outcome; }
    Outcome.Sheet.Body  = Assembly.Body;
    Outcome.Sheet.Faces = Assembly.Faces;
    Outcome.Sheet.Roles = { SweepFace{ SweepFaceRole::Side, "Surface", 0 } };
    return Outcome;
}

}
