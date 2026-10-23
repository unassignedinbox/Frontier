/*====================================================================================================================================
                                                   DRAFTINGDRAWINGSHEET.CPP
====================================================================================================================================*/
// 🧩 Drawing sheet (Alt+D): shown solid bodies → kernel ComposeDrawingSheet (metres → millimetres) → SVG / DXF R12

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDrawingSheet.h"

#include "Drawing/DrawingExport.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{

[[nodiscard]] bool ShownSolidCondition(const DraftingDocument& Document, const Modeller::RegeneratedBody& Body, int& Sheets)
{
    if (Document.Appearance.HiddenBodies.contains(Body.Key) || !Body.Body.PresentCondition()) return false;
    for (const DraftingBodyFigures& Figures : Document.Summary.Bodies)
    {
        if (Figures.Key == Body.Key && Figures.SheetCondition) { ++Sheets; return false; }
    }
    return true;
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

const char* DraftingProjectionAngleLabel(Modeller::DrawingProjectionAngle Angle) noexcept
{
    return Angle == Modeller::DrawingProjectionAngle::First ? "First" : "Third";
}

bool ParseDraftingPaperSize(const std::string& Label, Modeller::DrawingPaperSize& Paper) noexcept
{
    for (int Index = 0; Index < 5; ++Index)
    {
        if (Label == Modeller::DrawingPaperLabel(static_cast<Modeller::DrawingPaperSize>(Index))) { Paper = static_cast<Modeller::DrawingPaperSize>(Index); return true; }
    }
    return false;
}

bool ParseDraftingProjectionAngle(const std::string& Label, Modeller::DrawingProjectionAngle& Angle) noexcept
{
    if (Label == "First") { Angle = Modeller::DrawingProjectionAngle::First; return true; }
    if (Label == "Third") { Angle = Modeller::DrawingProjectionAngle::Third; return true; }
    return false;
}

std::string DraftingDrawingSheetKey(const DraftingDrawingSheetSettings& Settings)
{
    const DraftingTitleFields& Title = Settings.Title;
    char Text[160];
    std::snprintf(Text, sizeof(Text), "%s|%s|%.17g|%d%d%d%d%d|", Modeller::DrawingPaperLabel(Settings.Paper), DraftingProjectionAngleLabel(Settings.Angle), Settings.Scale,
                  Settings.IsometricEnabled, Settings.SectionEnabled, Settings.HiddenLinesEnabled, Settings.TangentEdgesEnabled, Settings.DimensionsEnabled);
    std::string Key = Text;
    if (Settings.SectionEnabled) Key += DraftingSectionKey(Settings.Section);
    for (const std::string* Field : { &Title.Title, &Title.DrawingNumber, &Title.Author, &Title.Date, &Title.Material, &Title.Revision, &Title.Company }) Key += "|" + *Field;
    return Key;
}

std::string DraftingDrawingSheetCacheKey(const DraftingDocument& Document, const DraftingDrawingSheetSettings& Settings)
{
    std::string Key = DraftingDrawingSheetKey(Settings) + "#";
    for (const std::string& Hidden : Document.Appearance.HiddenBodies) Key += Hidden + ";";
    return Key;
}

bool DefaultDraftingDrawingSheetCondition(const DraftingDrawingSheetSettings& Settings)
{
    return DraftingDrawingSheetKey(Settings) == DraftingDrawingSheetKey(DraftingDrawingSheetSettings{}) && DraftingSectionKey(Settings.Section) == DraftingSectionKey(DraftingDrawingSheetSettings{}.Section);
}

Modeller::OrthonormalFrame DraftingSheetSectionFrame(const Modeller::OrthonormalFrame& Plane)
{
    return Modeller::ConstructFrame(Plane.Origin, Plane.ZAxis * -1.0, Plane.XAxis);
}

DraftingDrawingSheet ComposeDraftingDrawingSheet(const DraftingDocument& Document, const DraftingDrawingSheetSettings& Settings)
{
    const auto Start = std::chrono::steady_clock::now();
    DraftingDrawingSheet Out;
    Out.Revision    = Document.Revision;
    Out.SettingsKey = DraftingDrawingSheetCacheKey(Document, Settings);
    const auto Finish = [&]()
    {
        Out.Milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
        return Out;
    };
    std::vector<Modeller::BodyToken> Bodies;
    if (Document.Partition)
    {
        for (const Modeller::RegeneratedBody& Body : Document.Regeneration.Bodies) if (ShownSolidCondition(Document, Body, Out.Sheets)) Bodies.push_back(Body.Body);
    }
    if (Bodies.empty())
    {
        Out.Refusal = Out.Sheets ? "Drawing sheet: no shown solid body (sheet bodies are not drawn)" : "Drawing sheet: no shown solid body";
        return Finish();
    }
    Out.Bodies = static_cast<int>(Bodies.size());

    Modeller::DrawingSheetRequest Request;
    Request.Paper                = Settings.Paper;
    Request.Angle                = Settings.Angle;
    Request.Scale                = Settings.Scale;
    Request.ModelUnitMillimetres = DraftingModelUnitMillimetres;
    Request.IsometricEnabled     = Settings.IsometricEnabled;
    Request.HiddenLinesEnabled   = Settings.HiddenLinesEnabled;
    Request.TangentEdgesEnabled  = Settings.TangentEdgesEnabled;
    Request.DimensionsEnabled    = Settings.DimensionsEnabled;
    const DraftingTitleFields& Title = Settings.Title;
    Request.Title = { Title.Title.empty() ? Document.Title : Title.Title, Title.DrawingNumber, Title.Author, Title.Date, Title.Material, Title.Revision, Title.Company };
    if (Settings.SectionEnabled)
    {
        const DraftingSection Plane = ResolveDraftingSectionPlane(Document, Settings.Section);
        if (!Plane.PlaneCondition)
        {
            Out.Refusal = "Drawing sheet: section A-A — " + (Plane.Refusal.empty() ? std::string("the plane does not resolve") : Plane.Refusal);
            return Finish();
        }
        Request.Section = DraftingSheetSectionFrame(Plane.Plane);
    }
    Out.Sheet   = Modeller::ComposeDrawingSheet(*Document.Partition, Bodies, Request);
    Out.Refusal = Out.Sheet.Refusal;
    return Finish();
}

std::string DraftingDrawingSheetExportPath(const std::string& DocumentPath, const std::string& Extension)
{
    if (DocumentPath.empty()) return "Untitled" + Extension;
    std::string Stem = DocumentPath;
    for (const char* Suffix : { ".drafting.toml", ".toml" })
    {
        const std::string Tail = Suffix;
        if (Stem.size() > Tail.size() && Stem.compare(Stem.size() - Tail.size(), Tail.size(), Tail) == 0) { Stem.resize(Stem.size() - Tail.size()); break; }
    }
    return Stem + Extension;
}

std::string ExportDraftingDrawingSheet(const DraftingDrawingSheet& Sheet, const std::string& Path)
{
    if (!Sheet.Refusal.empty()) return "Export: the drawing sheet was refused (" + Sheet.Refusal + ")";
    return Modeller::ExportDrawingFile(Sheet.Sheet, Path);
}

} // namespace Frontier::DraftingWorkspace
