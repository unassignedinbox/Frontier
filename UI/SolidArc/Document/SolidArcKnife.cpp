/*====================================================================================================================================
                                                          SOLIDARCKNIFE.CPP
====================================================================================================================================*/
// 🧩 Knife insert library: procedural built-ins and library STEP files normalized to the insert frame

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Document/SolidArcKnife.h"

#include "UI/SolidArc/Document/SolidArcDrawingSheet.h"
#include "UI/SolidArc/Document/SolidArcExchange.h"
#include "UI/SolidArc/Document/SolidArcScene.h"

#include "Engine/GeometryProcessing/Exchange/StepExchange.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <numbers>

namespace Frontier::SolidArc
{

using namespace GeometryProcessing;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{

[[nodiscard]] double ElapsedMilliseconds(std::chrono::steady_clock::time_point Start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
}

[[nodiscard]] const RegeneratedBody* FindBody(const SolidArcDocument& Document, const std::string& Key)
{
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies) if (Body.Key == Key) return &Body;
    return nullptr;
}

// Scene span of body Key (nullptr when the scene does not show it).
[[nodiscard]] const SolidArcSceneBody* FindSceneBody(const SolidArcScene& Scene, const std::string& Key)
{
    for (const SolidArcSceneBody& Body : Scene.Bodies) if (Body.Key == Key) return &Body;
    return nullptr;
}

// 📝 ① box of the body's facets ② Transform step: unit width, centred, top at +Lift ③ canonical STEP text of the result
//    ④ its edge polylines. Refusal (empty on success) when the body is missing, flat or the writer refuses it.
[[nodiscard]] std::string NormalizeSolidArcInsert(SolidArcDocument& Scratch, const std::string& Key, SolidArcInsert& Insert)
{
    const SolidArcScene      Before = BuildSolidArcScene(Scratch);
    const SolidArcSceneBody* Span   = FindSceneBody(Before, Key);
    if (!Span) return "the cutter body was not made";
    double Low[3]  = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };
    double High[3] = { -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max() };
    for (uint32_t Face = Span->FirstFace; Face < Span->FirstFace + Span->FaceCount; ++Face)
    {
        const SolidArcSceneFace& Each = Before.Faces[Face];
        for (uint32_t Corner = Each.FirstCorner; Corner < Each.FirstCorner + Each.CornerCount; ++Corner)
            for (int Axis = 0; Axis < 3; ++Axis)
            {
                Low[Axis]  = std::min(Low[Axis], double(Before.Corners[Corner].Position[Axis]));
                High[Axis] = std::max(High[Axis], double(Before.Corners[Corner].Position[Axis]));
            }
    }
    const double Width = std::max(High[0] - Low[0], High[1] - Low[1]);                      // [len]
    if (!(Width > 1.0e-9) || !(High[2] - Low[2] > 1.0e-9)) return "the cutter body is flat";
    const double Factor = 1.0 / Width;
    AffineMap Map;
    Map.ColumnX = { Factor, 0.0, 0.0 };
    Map.ColumnY = { 0.0, Factor, 0.0 };
    Map.ColumnZ = { 0.0, 0.0, Factor };
    Map.Shift   = { -0.5 * (Low[0] + High[0]) * Factor, -0.5 * (Low[1] + High[1]) * Factor, SolidArcInsertLift - High[2] * Factor };
    const SolidArcCommandOutcome Placed = TransformSolidArcBodies(Scratch, { Key }, Map, "Normalize");
    if (!Placed.AcceptedCondition) return Placed.Refusal;
    const RegeneratedBody* Body = FindBody(Scratch, Key);
    if (!Body && Scratch.Regeneration.Bodies.size() == 1) Body = &Scratch.Regeneration.Bodies.front();
    if (!Body || !Body->Body.PresentCondition() || !Scratch.Partition) return "the normalized cutter is missing";
    StepRequest Request;
    Request.ModelUnitMillimetres = SolidArcModelUnitMillimetres;
    Request.Name                 = Insert.Name;
    const StepExport Export = WriteStep(*Scratch.Partition, { Body->Body }, Request);
    if (!Export.Refusal.empty()) return Export.Refusal;
    Insert.Exchange = Export.Bytes;
    Insert.Depth    = (High[2] - Low[2]) * Factor - SolidArcInsertLift;
    const SolidArcScene      After = BuildSolidArcScene(Scratch);
    const SolidArcSceneBody* Shown = FindSceneBody(After, Body->Key);
    if (!Shown) return {};
    for (uint32_t Edge = Shown->FirstEdge; Edge < Shown->FirstEdge + Shown->EdgeCount; ++Edge)
    {
        const SolidArcSceneEdge& Each = After.Edges[Edge];
        for (uint32_t Segment = Each.FirstSegment; Segment < Each.FirstSegment + Each.SegmentCount; ++Segment)
        {
            const SolidArcSceneSegment& Piece = After.Segments[Segment];
            Insert.Segments.push_back({ Vector3{ Piece.Start[0], Piece.Start[1], Piece.Start[2] }, Vector3{ Piece.End[0], Piece.End[1], Piece.End[2] } });
        }
    }
    return {};
}

// A filled closed drawing on the plane z = Top swept down to z = Bottom (a first body: New). Key receives the body.
[[nodiscard]] std::string SweepFlatCutter(SolidArcDocument& Scratch, SolidArcDrawingRequest Request, double Top, double Bottom, std::string& Key)
{
    Request.PlaneCondition = true;
    Request.Plane          = OrthonormalFrame{};
    Request.Plane.Origin   = { 0.0, 0.0, Top };
    Request.FillCondition  = true;
    const SolidArcCommandOutcome Drawn = DrawSolidArcSketch(Scratch, Request);
    if (!Drawn.AcceptedCondition) return Drawn.Refusal;
    const std::string Sketch = Scratch.Tree.Steps.back().Label;
    const SolidArcCommandOutcome Swept = ExtrudeSolidArcSketch(Scratch, Sketch, Bottom - Top, FeatureCombination::New);
    if (!Swept.AcceptedCondition) return Swept.Refusal;
    Key = Scratch.Tree.Steps.back().Label;
    return {};
}

// 📝 Countersunk hole: the half profile on the upright plane (sketch x → x, sketch y → z) turned a full turn about z.
[[nodiscard]] std::string TurnCountersink(SolidArcDocument& Scratch, std::string& Key)
{
    SolidArcDrawingRequest Request;
    Request.Kind            = SolidArcDrawingKind::Line;
    Request.GridPlane       = 1;
    Request.ClosedCondition = true;
    Request.FillCondition   = true;
    Request.Points          = { { 0.0, 0.05 }, { 0.55, 0.05 }, { 0.25, -0.25 }, { 0.25, -0.8 }, { 0.0, -0.8 } };
    const SolidArcCommandOutcome Drawn = DrawSolidArcSketch(Scratch, Request);
    if (!Drawn.AcceptedCondition) return Drawn.Refusal;
    const std::string Sketch = Scratch.Tree.Steps.back().Label;
    SolidArcRevolveAxis Axis;
    Axis.Origin    = { 0.0, 0.0 };
    Axis.Direction = { 0.0, 1.0 };
    const SolidArcCommandOutcome Turned = RevolveSolidArcSketch(Scratch, Sketch, Axis, 2.0 * std::numbers::pi);
    if (!Turned.AcceptedCondition) return Turned.Refusal;
    Key = Scratch.Tree.Steps.back().Label;
    return {};
}

// One built-in: authored by Author in a scratch document, then normalized.
template <typename Author>
void AddBuiltIn(SolidArcInsertLibrary& Library, const std::string& Name, Author&& Make)
{
    SolidArcDocument Scratch = CreateSolidArcDocument("Insert " + Name);
    SolidArcInsert   Insert;
    Insert.Name = Name;
    std::string Key;
    std::string Refusal = Make(Scratch, Key);
    if (Refusal.empty()) Refusal = NormalizeSolidArcInsert(Scratch, Key, Insert);
    if (!Refusal.empty()) { Library.Notes.push_back(Name + ": " + Refusal); return; }
    Library.Inserts.push_back(std::move(Insert));
}

[[nodiscard]] SolidArcDrawingRequest Shape(SolidArcDrawingKind Kind, std::vector<Vector2> Points, bool CentreCondition = false, int Sides = 6, bool ClosedCondition = false)
{
    SolidArcDrawingRequest Request;
    Request.Kind            = Kind;
    Request.Points          = std::move(Points);
    Request.CentreCondition = CentreCondition;
    Request.Sides           = Sides;
    Request.ClosedCondition = ClosedCondition;
    return Request;
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

SolidArcInsertLibrary AssembleSolidArcInserts(const std::string& Folder)
{
    const auto Start = std::chrono::steady_clock::now();
    SolidArcInsertLibrary Library;
    Library.Folder = Folder;
    const auto Flat = [](SolidArcDrawingRequest Request, double Bottom)
    {
        return [Request, Bottom](SolidArcDocument& Scratch, std::string& Key) { return SweepFlatCutter(Scratch, Request, 0.05, Bottom, Key); };
    };
    AddBuiltIn(Library, "Square pocket", Flat(Shape(SolidArcDrawingKind::Rectangle, { { 0.0, 0.0 }, { 0.5, 0.5 } }, true), -0.3));
    AddBuiltIn(Library, "Round hole", Flat(Shape(SolidArcDrawingKind::Circle, { { 0.0, 0.0 }, { 0.5, 0.0 } }, true), -0.5));
    AddBuiltIn(Library, "Countersink", TurnCountersink);
    AddBuiltIn(Library, "Slot", Flat(Shape(SolidArcDrawingKind::Slot, { { -0.35, 0.0 }, { 0.35, 0.0 }, { 0.35, 0.15 } }), -0.3));
    AddBuiltIn(Library, "Hex pocket", Flat(Shape(SolidArcDrawingKind::Polygon, { { 0.0, 0.0 }, { 0.5, 0.0 } }, true, 6), -0.3));
    AddBuiltIn(Library, "Triangle", Flat(Shape(SolidArcDrawingKind::Polygon, { { 0.0, 0.0 }, { 0.0, 0.5 } }, true, 3), -0.3));
    AddBuiltIn(Library, "Cross", Flat(Shape(SolidArcDrawingKind::Line, { { -0.15, -0.5 }, { 0.15, -0.5 }, { 0.15, -0.15 }, { 0.5, -0.15 }, { 0.5, 0.15 }, { 0.15, 0.15 },
                                                                           { 0.15, 0.5 }, { -0.15, 0.5 }, { -0.15, 0.15 }, { -0.5, 0.15 }, { -0.5, -0.15 }, { -0.15, -0.15 } },
                                                  false, 6, true), -0.3));
    std::error_code Error;
    std::vector<std::filesystem::path> Files;
    if (!Folder.empty() && std::filesystem::is_directory(Folder, Error))
        for (const std::filesystem::directory_entry& Entry : std::filesystem::directory_iterator(Folder, Error))
        {
            std::string Extension = Entry.path().extension().string();
            std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](unsigned char Letter) { return char(std::tolower(Letter)); });
            if (Entry.is_regular_file(Error) && (Extension == ".step" || Extension == ".stp")) Files.push_back(Entry.path());
        }
    std::sort(Files.begin(), Files.end());
    for (const std::filesystem::path& File : Files)
    {
        const SolidArcStepReading Reading = ReadSolidArcStepFile(File.string());
        const std::string Name = File.stem().string();
        if (!Reading.Refusal.empty() || Reading.Texts.empty()) { Library.Notes.push_back(Name + ": " + (Reading.Refusal.empty() ? std::string("no solid") : Reading.Refusal)); continue; }
        SolidArcStepReading First = Reading;
        First.Texts.resize(1);
        SolidArcDocument Scratch = CreateSolidArcDocument("Insert " + Name);
        const SolidArcCommandOutcome Imported = ImportSolidArcStep(Scratch, First);
        SolidArcInsert Insert;
        Insert.Name = Name;
        Insert.Path = File.string();
        const std::string Refusal = Imported.AcceptedCondition ? NormalizeSolidArcInsert(Scratch, Scratch.Tree.Steps.back().Label, Insert) : Imported.Refusal;
        if (!Refusal.empty()) { Library.Notes.push_back(Name + ": " + Refusal); continue; }
        Library.Inserts.push_back(std::move(Insert));
    }
    Library.Milliseconds = ElapsedMilliseconds(Start);
    return Library;
}

AffineMap PlaceSolidArcInsert(const OrthonormalFrame& Spot, double Size, double Turn, bool RaisedCondition)
{
    const Vector3 Across = Spot.XAxis * std::cos(Turn) + Spot.YAxis * std::sin(Turn);
    const Vector3 Along  = Spot.YAxis * std::cos(Turn) - Spot.XAxis * std::sin(Turn);
    const double  Flip   = RaisedCondition ? -1.0 : 1.0;                                     // Half turn about x̂: det stays +1
    AffineMap Map;
    Map.ColumnX = Across * Size;
    Map.ColumnY = Along * (Size * Flip);
    Map.ColumnZ = Spot.ZAxis * (Size * Flip);
    Map.Shift   = Spot.Origin;
    return Map;
}

std::string SaveSolidArcInsert(const SolidArcDocument& Document, const std::string& Key, const std::string& Folder, const std::string& Name, std::string& Path)
{
    std::string Stem;
    for (const char Letter : Name) if (std::isalnum(static_cast<unsigned char>(Letter))) Stem.push_back(Letter);
    if (Stem.empty()) Stem = "Insert";
    if (Folder.empty()) return "no insert library folder (Files.InsertFolder in the options)";
    std::error_code Error;
    std::filesystem::create_directories(Folder, Error);
    std::filesystem::path Wanted = std::filesystem::path(Folder) / (Stem + ".step");
    for (int Ordinal = 2; std::filesystem::exists(Wanted, Error); ++Ordinal) Wanted = std::filesystem::path(Folder) / (Stem + std::to_string(Ordinal) + ".step");
    Path = Wanted.string();
    const SolidArcExportOutcome Outcome = ExportSolidArcBodies(Document, Path, { Key });
    return Outcome.Refusal;
}

} // namespace Frontier::SolidArc
