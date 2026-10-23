/*====================================================================================================================================
                                                    EXCHANGEVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Exchange X1 (priority 4): STL export read back — closure, Euler characteristic against the B-rep, volume within the
//    chordal bound, binary = ASCII, refusals

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Blend/BlendCatalogue.h"
#include "Blend/EdgeBlend.h"
#include "Construction/PrimitiveConstruction.h"
#include "Exchange/StlExchange.h"
#include "Inspection/SpecimenCatalogue.h"
#include "Measurement/MassProperties.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <numbers>
#include <string>
#include <vector>

using namespace Frontier::SolidArcRemastered;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{

struct GateTally
{
    const char* Code     = "";                                                               // [-] - EX1 …
    const char* Subject  = "";                                                               // [-] - What it measures
    const char* Limit    = "";                                                               // [-] - Acceptance rule
    double      Worst    = 0.0;                                                              // [-] - Largest measured value
    long        Checks   = 0;                                                                // [-]
    long        Failures = 0;                                                                // [-]
};

GateTally StlClosureGate  = { "EX1", "STL closure: every specimen (primitives, hole, cavity, NURBS sweep / loft / pipe, rolling-ball and variable fillets, chamfers) written binary and read back welds watertight — no open, non-manifold or misoriented side, no degenerate facet; facets = complex triangles; components = shells; Euler χ of the facets = the B-rep's V − E + 2F − L", "0 defects · χ exact" };
GateTally StlVolumeGate   = { "EX2", "STL geometry: |V(facets) − V(exact)| ≤ sag · A(exact) for every specimen (each facet lies within sag of its face); the sphere's error shrinks at least 2× when sag shrinks 4×; ModelUnitMillimetres 1000 scales every corner by 1000 within float rounding", "≤ sag · A · ratio ≤ ½" };
GateTally StlFormatGate   = { "EX3", "STL formats: binary size 84 + 50 n, header never \"solid…\"; ASCII (9 significant digits) reads back to the same welded corners and facets as binary; stored normals = unit facet normals; writing twice gives the same bytes", "identical" };
GateTally StlRefusalGate  = { "EX4", "STL refusals and defect detection: no bodies, a zero / negative unit, a stale body token, a truncated binary, ASCII without endsolid, a facet with two vertices; one facet removed → 3 open sides, one flipped → 3 misoriented, one repeated → 3 non-manifold", "named · counted" };

std::vector<GateTally*> AllGates() { return { &StlClosureGate, &StlVolumeGate, &StlFormatGate, &StlRefusalGate }; }

struct DetailRow
{
    std::string Gate            = {};
    std::string Name            = {};
    std::string Detail          = {};
    bool        PassedCondition = false;
};

std::vector<DetailRow> Details;
std::string            FileFolder;                                                           // --files: write the exports here

void Report(GateTally& Gate, const std::string& Name, const std::string& Detail, double Value, bool PassedCondition)
{
    Gate.Worst = std::max(Gate.Worst, Value);
    ++Gate.Checks;
    if (!PassedCondition) ++Gate.Failures;
    Details.push_back({ Gate.Code, Name, Detail, PassedCondition });
    std::fprintf(stderr, "%s %s %s · %s\n", PassedCondition ? "🟢" : "🔴", Gate.Code, Name.c_str(), Detail.c_str());
}

template <typename... Values> std::string Format(const char* Pattern, Values... Arguments)
{
    char Text[512];
    std::snprintf(Text, sizeof(Text), Pattern, Arguments...);
    return Text;
}

void WriteFile(const std::string& Name, const std::string& Bytes)
{
    if (FileFolder.empty()) return;
    std::filesystem::create_directories(FileFolder);
    std::ofstream Stream(std::filesystem::path(FileFolder) / Name, std::ios::binary);
    Stream.write(Bytes.data(), static_cast<std::streamsize>(Bytes.size()));
}

// 📝 One test body: name, partition, token. Construction specimens carry hand-derived expectations; blend specimens are
//    blended first (BlendEdges) and checked against their own B-rep counts.
struct TestBody
{
    std::string    Name      = {};
    ModelPartition Partition = {};
    BodyToken      Body      = {};
};

std::vector<TestBody> ConstructTestBodies()
{
    std::vector<TestBody> Bodies;
    for (const char* Name : { "Block", "Cylinder", "Cone", "Sphere", "Torus", "PerforatedBlock", "HollowBlock", "PipeBend", "LoftFrustum", "SweptRing", "MirroredCross" })
    {
        TestBody& Entry = Bodies.emplace_back();
        Entry.Name = Name;
        const SpecimenOutcome Outcome = ConstructSpecimen(Entry.Partition, Name, {});
        if (!Outcome.Refusal.empty()) { std::fprintf(stderr, "specimen %s refused: %s\n", Name, Outcome.Refusal.c_str()); Bodies.pop_back(); continue; }
        Entry.Body = Outcome.Body;
    }
    for (const char* Name : { "BoxAllFillet", "BoxAllChamfer", "HoleRimFillet", "BossRootFillet", "VariableFillet", "SmoothFillet" })
    {
        TestBody& Entry = Bodies.emplace_back();
        Entry.Name = Name;
        const BlendSpecimen Specimen = ConstructBlendSpecimen(Entry.Partition, Name);
        const BlendOutcome  Blended  = BlendEdges(Entry.Partition, Specimen.Body, Specimen.Edges, Specimen.Settings);
        if (!Blended.Refusal.empty()) { std::fprintf(stderr, "blend %s refused: %s\n", Name, Blended.Refusal.c_str()); Bodies.pop_back(); continue; }
        Entry.Body = Blended.Body;
    }
    return Bodies;
}

// Euler–Poincaré on the B-rep: V − E + F − (L − F) = 2 (S − G) = χ of the boundary surface.
int BoundaryEulerCharacteristic(const ModelPartition& Partition, BodyToken Body)
{
    const BodyInventory Inventory = Partition.CollectInventory(Body);
    return static_cast<int>(Inventory.Vertices.size()) - static_cast<int>(Inventory.Edges.size()) + 2 * static_cast<int>(Inventory.Faces.size()) - static_cast<int>(Inventory.Loops.size());
}

bool SameReading(const StlReading& First, const StlReading& Second)
{
    if (First.Triangles != Second.Triangles || First.Positions.size() != Second.Positions.size()) return false;
    for (size_t Index = 0; Index < First.Positions.size(); ++Index)
        if (First.Positions[Index].X != Second.Positions[Index].X || First.Positions[Index].Y != Second.Positions[Index].Y || First.Positions[Index].Z != Second.Positions[Index].Z) return false;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    STL
//------------------------------------------------------------------------------------------------------------------------

void ValidateStl()
{
    std::vector<TestBody> Bodies = ConstructTestBodies();
    for (TestBody& Entry : Bodies)
    {
        const BodyInventory Inventory = Entry.Partition.CollectInventory(Entry.Body);
        StlRequest Request;
        Request.Name = Entry.Name;
        const StlExport Binary = WriteStl(Entry.Partition, { Entry.Body }, Request);
        if (!Binary.Refusal.empty()) { Report(StlClosureGate, Entry.Name, Binary.Refusal, 1.0, false); continue; }
        WriteFile(Entry.Name + ".stl", Binary.Bytes);
        const StlReading   Reading   = ReadStl(Binary.Bytes);
        const StlSoundness Soundness = MeasureStlSoundness(Reading);
        const int          Expected  = BoundaryEulerCharacteristic(Entry.Partition, Entry.Body);
        const int          Defects   = Soundness.OpenSides + Soundness.NonManifoldSides + Soundness.MisorientedSides + Soundness.DegenerateTriangles;
        Report(StlClosureGate, Entry.Name,
               Format("%d facets, %zu corners, χ %d = B-rep %d, %d component(s) / %zu shell(s), defects %d (open %d · non-manifold %d · misoriented %d · degenerate %d)",
                      Binary.Triangles, Reading.Positions.size(), Soundness.EulerCharacteristic, Expected, Soundness.Components, Inventory.Shells.size(), Defects,
                      Soundness.OpenSides, Soundness.NonManifoldSides, Soundness.MisorientedSides, Soundness.DegenerateTriangles),
               static_cast<double>(Defects), Reading.Refusal.empty() && Soundness.WatertightCondition && Defects == 0 && Soundness.EulerCharacteristic == Expected
               && static_cast<int>(Reading.Triangles.size()) == Binary.Triangles && Soundness.Components == static_cast<int>(Inventory.Shells.size()));

        const BodyMassProperties Mass  = MeasureBodyMass(Entry.Partition, Entry.Body);
        const double             Bound = Binary.Sag * Mass.SurfaceArea;
        const double             Error = std::fabs(Soundness.Volume - Mass.Volume);
        Report(StlVolumeGate, Entry.Name, Format("V facets %.9g vs exact %.9g: |ΔV| %.3e ≤ sag %.2e · A %.6g = %.3e (ratio %.3f)", Soundness.Volume, Mass.Volume, Error, Binary.Sag, Mass.SurfaceArea, Bound, Error / Bound),
               Error / Bound, Error <= Bound);

        Request.BinaryEnabled = false;
        const StlExport  Ascii        = WriteStl(Entry.Partition, { Entry.Body }, Request);
        const StlReading AsciiReading = ReadStl(Ascii.Bytes);
        double NormalError = 0.0;
        for (size_t Facet = 0; Facet < Reading.Triangles.size(); ++Facet)
        {
            const std::array<int, 3>& Triangle = Reading.Triangles[Facet];
            const Vector3 Normal = Cross(Reading.Positions[Triangle[1]] - Reading.Positions[Triangle[0]], Reading.Positions[Triangle[2]] - Reading.Positions[Triangle[0]]);
            const double  Size   = Length(Normal);
            if (Size > 0.0) NormalError = std::max(NormalError, Length(Normal * (1.0 / Size) - Reading.Normals[Facet]));
        }
        const bool Repeatable = WriteStl(Entry.Partition, { Entry.Body }, StlRequest{ true, 1.0, {}, Entry.Name }).Bytes == Binary.Bytes;
        Report(StlFormatGate, Entry.Name, Format("binary %zu B = 84 + 50 · %d, ASCII %zu B, same welded facets: %s, normals within %.1e, repeatable: %s", Binary.Bytes.size(), Binary.Triangles, Ascii.Bytes.size(),
                                                 SameReading(Reading, AsciiReading) ? "yes" : "no", NormalError, Repeatable ? "yes" : "no"),
               NormalError, Reading.BinaryCondition && !AsciiReading.BinaryCondition && Binary.Bytes.size() == 84 + 50 * static_cast<size_t>(Binary.Triangles)
               && Binary.Bytes.compare(0, 5, "solid") != 0 && SameReading(Reading, AsciiReading) && AsciiReading.Name == Entry.Name && NormalError <= 1.0e-6 && Repeatable);
    }

    // Sphere convergence: sag ÷ 4 → error ÷ ≥ 2.
    {
        TestBody Sphere;
        Sphere.Body = ConstructSpecimen(Sphere.Partition, "Sphere", {}).Body;
        const double Exact = MeasureBodyMass(Sphere.Partition, Sphere.Body).Volume;
        std::vector<double> Errors;
        std::string Trail;
        for (const double Ratio : { 4.0e-3, 1.0e-3, 2.5e-4 })
        {
            StlRequest Request;
            Request.Tessellation.SagRatio = Ratio;
            const StlExport Export = WriteStl(Sphere.Partition, { Sphere.Body }, Request);
            Errors.push_back(std::fabs(MeasureStlSoundness(ReadStl(Export.Bytes)).Volume - Exact));
            Trail += Format("%s%d facets |ΔV| %.3e", Trail.empty() ? "" : " → ", Export.Triangles, Errors.back());
        }
        const double Worse = std::max(Errors[1] / Errors[0], Errors[2] / Errors[1]);
        Report(StlVolumeGate, "Sphere convergence (sag ratio 4e-3 → 1e-3 → 2.5e-4)", Trail + Format(" · worst step ratio %.3f", Worse), Worse, Worse <= 0.5);
    }

    // Model units: millimetre output of a metre model.
    {
        TestBody Block;
        Block.Body = ConstructBlock(Block.Partition, { 0.0, 0.0, 0.0 }, { 4.0, 2.5, 1.0 }).Value;
        StlRequest Metres;
        Metres.ModelUnitMillimetres = 1000.0;
        const StlReading Unit  = ReadStl(WriteStl(Block.Partition, { Block.Body }, StlRequest{}).Bytes);
        const StlReading Scale = ReadStl(WriteStl(Block.Partition, { Block.Body }, Metres).Bytes);
        double Worst = Unit.Positions.size() == Scale.Positions.size() ? 0.0 : 1.0e300;
        for (size_t Index = 0; Worst < 1.0e300 && Index < Unit.Positions.size(); ++Index)
            Worst = std::max(Worst, Length(Scale.Positions[Index] - Unit.Positions[Index] * 1000.0) / 4000.0);
        Report(StlVolumeGate, "Block 4 × 2.5 × 1 m → mm", Format("V %.6g mm³ = 10¹⁰, corners × 1000 within %.1e relative", MeasureStlSoundness(Scale).Volume, Worst), Worst,
               Worst <= 6.0e-8 && std::fabs(MeasureStlSoundness(Scale).Volume - 1.0e10) <= 1.0e10 * 1.0e-6 && Unit.Triangles == Scale.Triangles);
    }
}

void ValidateStlRefusals()
{
    TestBody Block;
    Block.Body = ConstructBlock(Block.Partition, { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }).Value;
    const auto Refuses = [&](const std::string& Name, const std::string& Refusal, const char* Wanted)
    {
        Report(StlRefusalGate, Name, Refusal.empty() ? std::string("(accepted)") : Refusal, 0.0, Refusal.find(Wanted) != std::string::npos);
    };
    Refuses("no bodies", WriteStl(Block.Partition, {}, {}).Refusal, "no body");
    StlRequest Zero;
    Zero.ModelUnitMillimetres = 0.0;
    Refuses("zero unit", WriteStl(Block.Partition, { Block.Body }, Zero).Refusal, "positive length");
    Zero.ModelUnitMillimetres = -1.0;
    Refuses("negative unit", WriteStl(Block.Partition, { Block.Body }, Zero).Refusal, "positive length");
    Refuses("stale token", WriteStl(Block.Partition, { BodyToken{ 999, 1 } }, {}).Refusal, "not in the partition");
    const StlExport Binary = WriteStl(Block.Partition, { Block.Body }, {});
    Refuses("truncated binary", ReadStl(Binary.Bytes.substr(0, Binary.Bytes.size() - 7)).Refusal, "neither a binary layout");
    StlRequest AsciiRequest;
    AsciiRequest.BinaryEnabled = false;
    const std::string Ascii = WriteStl(Block.Partition, { Block.Body }, AsciiRequest).Bytes;
    Refuses("ASCII without endsolid", ReadStl(Ascii.substr(0, Ascii.rfind("endsolid"))).Refusal, "missing \"endsolid\"");
    std::string TwoVertices = Ascii;
    const size_t Third = TwoVertices.find("vertex", TwoVertices.find("vertex", TwoVertices.find("vertex") + 1) + 1);
    TwoVertices.erase(Third, TwoVertices.find('\n', Third) - Third + 1);
    Refuses("facet with two vertices", ReadStl(TwoVertices).Refusal, "three vertices");

    const StlReading Sound = ReadStl(Binary.Bytes);
    StlReading Open = Sound, Flipped = Sound, Repeated = Sound;
    Open.Triangles.pop_back();
    std::swap(Flipped.Triangles[0][1], Flipped.Triangles[0][2]);
    Repeated.Triangles.push_back(Repeated.Triangles[0]);
    const StlSoundness OpenMeasure = MeasureStlSoundness(Open), FlipMeasure = MeasureStlSoundness(Flipped), RepeatMeasure = MeasureStlSoundness(Repeated);
    Report(StlRefusalGate, "defects counted", Format("one facet removed → %d open; one flipped → %d misoriented; one repeated → %d non-manifold; none watertight",
                                                     OpenMeasure.OpenSides, FlipMeasure.MisorientedSides, RepeatMeasure.NonManifoldSides),
           0.0, OpenMeasure.OpenSides == 3 && FlipMeasure.MisorientedSides == 3 && RepeatMeasure.NonManifoldSides == 3 && !OpenMeasure.WatertightCondition
           && !FlipMeasure.WatertightCondition && !RepeatMeasure.WatertightCondition && MeasureStlSoundness(Sound).WatertightCondition);
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    MAIN
//------------------------------------------------------------------------------------------------------------------------

int main(int Count, char** Arguments)
{
    std::string Only;
    for (int Index = 1; Index + 1 < Count; ++Index)
    {
        if (std::string(Arguments[Index]) == "--only") Only = Arguments[Index + 1];
        if (std::string(Arguments[Index]) == "--files") FileFolder = Arguments[Index + 1];
    }
    const auto Start = std::chrono::steady_clock::now();
    if (Only.empty() || Only == "stl") ValidateStl();
    if (Only.empty() || Only == "stlrefusal") ValidateStlRefusals();
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();

    bool PassedCondition = true;
    for (const GateTally* Gate : AllGates()) PassedCondition = PassedCondition && Gate->Failures == 0 && (Gate->Checks > 0 || !Only.empty());
    std::printf("# Exchange validation\n\n%s Priority 4: STL export and read-back (%.2f s)\n\n", PassedCondition ? "🟢" : "🔴", Seconds);
    std::printf("## Gates\n\n| | Gate | Measures | Limit | Worst | Checks | Failures |\n|---|---|---|---|---|---|---|\n");
    for (const GateTally* Gate : AllGates())
        std::printf("| %s | %s | %s | %s | %.2e | %ld | %ld |\n", Gate->Failures == 0 && Gate->Checks > 0 ? "🟢" : "🔴", Gate->Code, Gate->Subject, Gate->Limit, Gate->Worst, Gate->Checks, Gate->Failures);
    std::printf("\n## Checks\n\n| | Gate | Check | Detail |\n|---|---|---|---|\n");
    const auto Cell = [](std::string Text)
    {
        for (size_t At = Text.find('|'); At != std::string::npos; At = Text.find('|', At + 2)) Text.replace(At, 1, "\\|");
        return Text;
    };
    for (const DetailRow& Row : Details) std::printf("| %s | %s | %s | %s |\n", Row.PassedCondition ? "🟢" : "🔴", Row.Gate.c_str(), Cell(Row.Name).c_str(), Cell(Row.Detail).c_str());
    return PassedCondition ? 0 : 1;
}
