/*====================================================================================================================================
                                                           SECTIONLOFT.CPP
====================================================================================================================================*/
// 🧩 Loft through sections: knot merging, chord-averaged 𝑣ₖ, homogeneous interpolation, sheet assembly with iso-line charts

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Surfacing/SectionLoft.h"
#include "Surfacing/LinearSystem.h"
#include "Surfacing/SurfaceAlgebra.h"
#include "Construction/BodyAssembly.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIVATE FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double ClosureGap      = 1.0e-9;                                               // [len] - First = last pole: a closed section
    constexpr double ContinuityMatch = 1.0e-9;                                               // [len] - Shared section vs reference boundary (relative)

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

    [[nodiscard]] bool ClosedSection(const NurbsCurveDefinition& Definition)
    {
        return Length(Definition.Poles.front() - Definition.Poles.back()) <= ClosureGap;
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

    // ① Each section on [0, 1]; all open or all closed; open ones aligned with the first; lower degrees raised exactly.
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

    // ①′ Continuity references: turned so the end section is their 𝑣 = 0 or 𝑣 = 1 row running with 𝑢 (transposed /
    //     reversed as needed); σ records the side. A reference that does not carry the section as a boundary refuses.
    struct ContinuityEnd
    {
        int                    Order     = 0;
        NurbsSurfaceDefinition Reference = {};
        double                 Boundary  = 0.0;                                              // [-] - 𝑣 of the shared row on the reference
    };
    std::array<ContinuityEnd, 2> Ends;
    for (int Side = 0; Side < 2; ++Side)
    {
        const LoftContinuity& Continuity = Side == 0 ? Settings.Start : Settings.End;
        if (Continuity.Order <= 0) continue;
        const char* Name = Side == 0 ? "start" : "end";
        if (Continuity.Order > 2) return Refuse(std::string(Name) + " continuity G" + std::to_string(Continuity.Order) + " (only G0 / G1 / G2)");
        if (!Continuity.Reference.Definition) return Refuse(std::string(Name) + " continuity has no reference panel");
        if (Outcome.ClosedCondition) return Refuse(std::string(Name) + " continuity on closed sections (🚧)");
        for (double Weight : Continuity.Reference.Definition->Weights)
        {
            if (Weight != 1.0) return Refuse(std::string(Name) + " continuity with a rational reference panel (🚧)");
        }
        const NurbsCurveDefinition& Section = Side == 0 ? Mapped.front() : Mapped.back();
        double Scale = 0.0;
        for (const Vector3& Pole : Section.Poles) Scale = std::max(Scale, Length(Pole));
        const double Match = ContinuityMatch * std::max(1.0, Scale);
        bool Found = false;
        for (int Variant = 0; Variant < 8 && !Found; ++Variant)
        {
            // Variant bits: 1 transpose, 2 reverse 𝑢, 4 boundary 𝑣 = 1.
            NurbsSurfaceDefinition Candidate = *Continuity.Reference.Definition;
            if (Variant & 1) Candidate = TransposeNurbsSurface(Candidate);
            if (Variant & 2) Candidate = ReverseNurbsSurface(Candidate, true);
            const double Boundary = (Variant & 4) ? 1.0 : 0.0;
            double Worst = 0.0;
            for (int Sample = 0; Sample <= 16 && Worst <= Match; ++Sample)
            {
                const double U = Sample / 16.0;
                Worst = std::max(Worst, Length(EvaluateNurbsSurfaceDerivatives(Candidate, { U, Boundary }).Position - EvaluateNurbsCurveDerivatives(Section, U).Position));
            }
            if (Worst > Match) continue;
            Ends[Side] = { Continuity.Order, Candidate, Boundary };
            Found = true;
        }
        if (!Found) return Refuse(std::string(Name) + " section is not a boundary of the continuity reference panel");
    }

    // ①″ One degree along 𝑢 for the sections and the references (exact elevation).
    int Degree = 1;
    for (const NurbsCurveDefinition& Definition : Mapped) Degree = std::max(Degree, Definition.Degree);
    for (const ContinuityEnd& End : Ends) if (End.Order > 0) Degree = std::max(Degree, End.Reference.DegreeU);
    for (NurbsCurveDefinition& Definition : Mapped)
    {
        if (Definition.Degree == Degree) continue;
        std::optional<NurbsCurveDefinition> Elevated = ElevateNurbsCurveDegree(Definition, Degree);
        if (!Elevated) return Refuse("degree elevation of a section to " + std::to_string(Degree) + " failed");
        Definition = std::move(*Elevated);
        ++Outcome.Elevated;
    }
    for (ContinuityEnd& End : Ends)
    {
        if (End.Order <= 0 || End.Reference.DegreeU == Degree) continue;
        std::optional<NurbsSurfaceDefinition> Elevated = ElevateNurbsSurfaceDegree(End.Reference, Degree, End.Reference.DegreeV);
        if (!Elevated) return Refuse("degree elevation of a continuity reference failed");
        End.Reference = std::move(*Elevated);
    }

    // ② Merge the knot vectors: every section (and reference) gets the union (largest multiplicity of each value).
    std::vector<std::vector<double>> KnotVectors;
    for (const NurbsCurveDefinition& Definition : Mapped) KnotVectors.push_back(Definition.Knots);
    for (const ContinuityEnd& End : Ends) if (End.Order > 0) KnotVectors.push_back(End.Reference.KnotsU);
    const std::vector<double> Union = UniteKnotVectors(KnotVectors, Degree);
    for (NurbsCurveDefinition& Definition : Mapped) Definition = RefineNurbsCurveKnots(Definition, MissingInteriorKnots(Definition.Knots, Union, Degree));
    for (ContinuityEnd& End : Ends) if (End.Order > 0) End.Reference = RefineNurbsSurfaceKnots(End.Reference, MissingInteriorKnots(End.Reference.KnotsU, Union, Degree), {});
    const int CountU = static_cast<int>(Mapped.front().Poles.size());
    for (const NurbsCurveDefinition& Definition : Mapped)
    {
        if (static_cast<int>(Definition.Poles.size()) != CountU) return Refuse("knot merging failed (a knot exceeds multiplicity p)");
    }
    for (const ContinuityEnd& End : Ends)
    {
        if (End.Order > 0 && End.Reference.CountU != CountU) return Refuse("knot merging with a continuity reference failed");
    }

    // ③ 𝑣ₖ: chord length along each pole column, normalised and averaged over the columns that move.
    const int CountV = static_cast<int>(Mapped.size());
    std::vector<double> Parameters(CountV, 0.0);
    int Columns = 0;
    double Across = 0.0;
    for (int Column = 0; Column < CountU; ++Column)
    {
        std::vector<double> Cumulative(CountV, 0.0);
        for (int Row = 1; Row < CountV; ++Row) Cumulative[Row] = Cumulative[Row - 1] + Length(Mapped[Row].Poles[Column] - Mapped[Row - 1].Poles[Column]);
        Across += Cumulative.back() / CountU;
        if (!(Cumulative.back() > ClosureGap)) continue;
        for (int Row = 1; Row < CountV; ++Row) Parameters[Row] += Cumulative[Row] / Cumulative.back();
        ++Columns;
    }
    if (Settings.Parameters.empty())
    {
        if (Columns == 0) return Refuse("all sections coincide");
        for (double& Parameter : Parameters) Parameter /= Columns;
        Parameters.back() = 1.0;
    }
    else
    {
        // ③″ Caller-fixed 𝑣ₖ: lofts that must share boundary columns bit-for-bit (evolution sides) take one list.
        if (static_cast<int>(Settings.Parameters.size()) != CountV) return Refuse(std::to_string(Settings.Parameters.size()) + " fixed parameters for " + std::to_string(CountV) + " sections");
        if (Settings.Parameters.front() != 0.0 || Settings.Parameters.back() != 1.0) return Refuse("fixed parameters must run from 0 to 1");
        Parameters = Settings.Parameters;
    }
    for (int Row = 1; Row < CountV; ++Row)
    {
        if (!(Parameters[Row] > Parameters[Row - 1] + 1.0e-12)) return Refuse("sections " + std::to_string(Row - 1) + " and " + std::to_string(Row) + " coincide");
    }

    // ③′ End conditions per pole column: ∂𝑣 = σ·α·R𝑣, ∂𝑣𝑣 = α²·R𝑣𝑣 at the shared row (α = across-chord ratio).
    FamilySkinning Skinning;
    Skinning.Degree     = Settings.DegreeAcross;
    Skinning.Parameters = Parameters;
    for (int Side = 0; Side < 2; ++Side)
    {
        const ContinuityEnd& End = Ends[Side];
        if (End.Order <= 0) continue;
        double ReferenceAcross = 0.0;
        for (int Column = 0; Column < CountU; ++Column)
        {
            const NurbsCurveDefinition Pillar = ExtractPoleColumn(End.Reference, Column);
            for (size_t Pole = 1; Pole < Pillar.Poles.size(); ++Pole) ReferenceAcross += Length(Pillar.Poles[Pole] - Pillar.Poles[Pole - 1]) / CountU;
        }
        if (!(ReferenceAcross > ClosureGap)) return Refuse("the continuity reference panel has no extent across");
        const double Scale = Across / ReferenceAcross;
        // Start meets the reference's 𝑣 = 1 row, or end meets its 𝑣 = 0 row: the loft continues the reference (σ = +1).
        const double Sign = (Side == 0) == (End.Boundary == 1.0) ? 1.0 : -1.0;
        FamilyEndCondition& Condition = Side == 0 ? Skinning.Start : Skinning.End;
        for (int Column = 0; Column < CountU; ++Column)
        {
            const NurbsCurveDerivatives Pillar = EvaluateNurbsCurveDerivatives(ExtractPoleColumn(End.Reference, Column), End.Boundary);
            Condition.First.push_back(Pillar.First * (Sign * Scale));
            if (End.Order >= 2) Condition.Second.push_back(Pillar.Second * (Scale * Scale));
        }
        (Side == 0 ? Outcome.StartScale : Outcome.EndScale) = Scale;
        // The shared section takes the reference's boundary row so both panels hold bit-identical poles there.
        NurbsCurveDefinition& Section = Side == 0 ? Mapped.front() : Mapped.back();
        const int Row = End.Boundary == 1.0 ? End.Reference.CountV - 1 : 0;
        const NurbsCurveDefinition Shared = ExtractPoleRow(End.Reference, Row);
        for (int Column = 0; Column < CountU; ++Column)
        {
            if (Length(Shared.Poles[Column] - Section.Poles[Column]) > 1.0e-6) return Refuse("the shared section and the reference boundary differ after knot merging");
        }
        Section.Poles = Shared.Poles;
    }

    // ④ Interpolate every pole column through the sections in homogeneous form (w·P, w), with the end conditions.
    std::string Reason;
    const std::optional<NurbsSurfaceDefinition> Skin = SkinCurveFamily(Mapped, Skinning, Reason);
    if (!Skin) return Refuse(Reason);
    const std::optional<NurbsSurface> Surface = FinishNurbsSurface(*Skin);
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
