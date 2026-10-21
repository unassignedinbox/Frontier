/*====================================================================================================================================
                                                 DRAFTINGVIEWPORTVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Step 9: DraftingWorkspace viewport gates on a real Vulkan device — GPU identity vs CPU pick, tints, views, determinism

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingMatcap.h"
#include "Document/DraftingScene.h"
#include "Document/DraftingSpecimen.h"
#include "Viewport/DraftingCamera.h"
#include "Viewport/DraftingDevice.h"
#include "Viewport/DraftingPortableImage.h"
#include "Viewport/DraftingRasterization.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Frontier::DraftingWorkspace;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr int SkipCode = 77;                                                             // [-] - ctest SKIP_RETURN_CODE (no Vulkan)

    struct GateTally
    {
        const char* Code     = "";                                                           // [-] - V1 …
        const char* Subject  = "";                                                           // [-] - What is measured
        const char* Limit    = "";                                                           // [-] - Pass condition (text)
        double      Worst    = 0.0;                                                          // [-] - Worst value
        long        Checks   = 0;                                                            // [-] - Measurements
        long        Failures = 0;                                                            // [-] - Measurements over the limit
    };

    GateTally DeviceGate      = { "V1", "Vulkan 1.3 device, SPIR-V modules from the Slang archive, three pipelines", "created" };
    GateTally FaceGate        = { "V2", "Face pixels away from edges: GPU identity = CPU ray pick (7 views × perspective / orthographic)", "mismatch ≤ 0.1 %" };
    GateTally EdgeGate        = { "V3", "Edge-stroke pixels: the GPU edge passes within the stroke reach of the pixel ray and is not hidden behind the CPU face hit (orthographic)", "explained ≥ 99.5 %" };
    GateTally ProbeGate       = { "V4", "Single-texel identity probe (hover path) = full readback = CPU pick", "all equal" };
    GateTally TintGate        = { "V5", "Selected face blue (#4099ff), active edge green (#33ff73), hovered face lighter; others unchanged", "all hold" };
    GateTally ViewGate        = { "V6", "View presets: centre pixel names the expected face (Top → top cap, Front → front side, …)", "all equal" };
    GateTally DeterminismGate = { "V7", "Same frame twice → identical colour and identity digests", "equal" };
    GateTally MatcapGate      = { "V8", "Matcap layers: planar faces = the bilinear texel at their view-space normal (body + 2 face overrides); Studio shader = CPU mirror; clearing restores the Studio frame bit-exact", "≤ 2 / 255" };

    std::vector<std::string> Journal;                                                        // [-] - Narrative rows

    void Tally(GateTally& Gate, double Value, bool PassedCondition, const std::string& Note)
    {
        Gate.Worst = std::max(Gate.Worst, Value);
        ++Gate.Checks;
        if (!PassedCondition) ++Gate.Failures;
        char Row[600];
        std::snprintf(Row, sizeof(Row), "| %s | %s | %.4g | %s |", Gate.Code, Note.c_str(), Value, PassedCondition ? "🟢" : "🔴");
        Journal.push_back(Row);
    }

    [[nodiscard]] std::string Compact(double Value)
    {
        char Text[32];
        std::snprintf(Text, sizeof(Text), "%.3g", Value);
        return Text;
    }

    struct Rendered
    {
        std::vector<uint32_t> Colour   = {};
        std::vector<uint32_t> Identity = {};
        uint32_t              Probe    = 0;
        double                Milliseconds = 0.0;
    };

    Rendered RenderFrame(const DraftingDevice& Device, DraftingRasterization& Raster, const DraftingCameraFrame& Frame, const DraftingViewportStyle& Style,
                         int ProbeX = -1, int ProbeY = -1)
    {
        const auto Start = std::chrono::steady_clock::now();
        Device.SubmitImmediately([&](VkCommandBuffer Commands) { Raster.Record(Commands, Frame, Style, true, ProbeX, ProbeY); });
        Rendered Result;
        Result.Milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
        const size_t Count  = size_t(Raster.Width()) * Raster.Height();
        Result.Colour.assign(Raster.ReadbackColour(), Raster.ReadbackColour() + Count);
        Result.Identity.assign(Raster.ReadbackIdentity(), Raster.ReadbackIdentity() + Count);
        Result.Probe = Raster.ProbedIdentity();
        return Result;
    }

    // ① every 3rd pixel whose 5 × 5 neighbourhood is one single face (or all background) is compared with the CPU pick
    void CompareFaces(const DraftingScene& Scene, const DraftingCameraFrame& Frame, const Rendered& Image, uint32_t Width, uint32_t Height,
                      const std::string& Label)
    {
        long Compared = 0, Mismatched = 0;
        for (uint32_t Row = 3; Row + 3 < Height; Row += 3)
            for (uint32_t Column = 3; Column + 3 < Width; Column += 3)
            {
                const uint32_t Centre = Image.Identity[size_t(Row) * Width + Column];
                bool UniformCondition = (Centre & IdentityEdgeBit) == 0u;
                for (int DeltaRow = -2; DeltaRow <= 2 && UniformCondition; ++DeltaRow)
                    for (int DeltaColumn = -2; DeltaColumn <= 2 && UniformCondition; ++DeltaColumn)
                        UniformCondition = Image.Identity[size_t(Row + DeltaRow) * Width + Column + DeltaColumn] == Centre;
                if (!UniformCondition) continue;
                const std::optional<DraftingPick> Hit = PickDraftingScene(Scene, CastDraftingRay(Frame, Column + 0.5, Row + 0.5), 0.0);
                const uint32_t Expected = Hit ? Hit->Identity : 0u;
                ++Compared;
                if (Expected != Centre) ++Mismatched;
            }
        const double Rate = Compared ? 100.0 * double(Mismatched) / double(Compared) : 100.0;
        Tally(FaceGate, Rate, Compared > 1000 && Rate <= 0.1, Label + ": " + std::to_string(Mismatched) + " / " + std::to_string(Compared) + " sampled pixels differ (%)");
    }

    [[nodiscard]] std::array<double, 3> MeanColour(const Rendered& Image, uint32_t Identity)
    {
        std::array<double, 3> Sum{};
        long Count = 0;
        for (size_t Index = 0; Index < Image.Identity.size(); ++Index)
        {
            if (Image.Identity[Index] != Identity) continue;
            const uint32_t Pixel = Image.Colour[Index];
            Sum[0] += (Pixel & 0xff) / 255.0; Sum[1] += ((Pixel >> 8) & 0xff) / 255.0; Sum[2] += ((Pixel >> 16) & 0xff) / 255.0;
            ++Count;
        }
        for (double& Channel : Sum) Channel /= std::max<long>(Count, 1);
        return Sum;
    }

    [[nodiscard]] uint32_t IdentityNamed(const DraftingScene& Scene, const std::string& Fragment, bool EdgeCondition)
    {
        if (EdgeCondition)
        {
            for (size_t Index = 0; Index < Scene.Edges.size(); ++Index) if (Scene.Edges[Index].Name.find(Fragment) != std::string::npos) return IdentityEdgeBit | uint32_t(Index + 1);
            return 0u;
        }
        for (size_t Index = 0; Index < Scene.Faces.size(); ++Index) if (Scene.Faces[Index].Name == Fragment) return uint32_t(Index + 1);
        return 0u;
    }

    void WriteExhibit(const std::string& Directory, const std::string& Stem, const Rendered& Image, uint32_t Width, uint32_t Height)
    {
        if (Directory.empty()) return;
        WriteDraftingPng(Directory + "/" + Stem + ".png", Width, Height, Image.Colour.data());
        const std::vector<uint32_t> Painted = PaintDraftingIdentity(Image.Identity.data(), Width, Height);
        WriteDraftingPng(Directory + "/" + Stem + "Identity.png", Width, Height, Painted.data());
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    MAIN
//------------------------------------------------------------------------------------------------------------------------

int main(int ArgumentCount, char** Arguments)
{
    const auto  Start = std::chrono::steady_clock::now();
    std::string Driver = std::getenv("DRAFTING_VULKAN_DRIVER") ? std::getenv("DRAFTING_VULKAN_DRIVER") : "";
    std::string Exhibits;
    for (int Index = 1; Index < ArgumentCount; ++Index)
    {
        const std::string Argument = Arguments[Index];
        if (Argument == "--vulkan-driver" && Index + 1 < ArgumentCount) Driver = Arguments[++Index];
        if (Argument == "--exhibits" && Index + 1 < ArgumentCount) Exhibits = Arguments[++Index];
    }

    // ---- V1
    DraftingDevice         Device;
    DraftingDeviceSettings Settings;
    Settings.DriverPath = Driver;
    std::string Refusal;
    if (!Device.Open(Settings, Refusal))
    {
        std::printf("# DraftingViewportValidation\n\n⚠️ skipped: %s\n", Refusal.c_str());
        return SkipCode;
    }
    DraftingRasterization Raster;
    const bool CreatedCondition = Raster.Create(Device, Refusal);
    Tally(DeviceGate, 0.0, CreatedCondition, Device.Describe() + (CreatedCondition ? "" : " — " + Refusal));
    if (!CreatedCondition) return 1;

    std::vector<std::string> Notes;
    DraftingDocument Document = BuildDraftingSpecimen(&Notes);
    for (const std::string& Line : Notes) std::printf("⚠️ specimen: %s\n", Line.c_str());
    const DraftingScene Scene = BuildDraftingScene(Document);
    constexpr uint32_t Width = 960, Height = 640;
    Raster.Resize(Width, Height);
    Raster.UploadScene(Scene);
    DraftingCamera Camera;
    FitDraftingCamera(Camera, Scene);
    DraftingViewportStyle Style;

    // ---- V2 · V6 over every preset, perspective and orthographic
    struct Expectation { DraftingViewPreset Preset; const char* Face; };
    const Expectation Views[] = { { DraftingViewPreset::Isometric, "" }, { DraftingViewPreset::Top, "Extrude#2.Cap(End)" },
                                  { DraftingViewPreset::Bottom, "Extrude#2.Cap(Start)" }, { DraftingViewPreset::Front, "Extrude#2.Side(Line1)" },
                                  { DraftingViewPreset::Back, "Extrude#2.Side(Line3)" }, { DraftingViewPreset::Right, "" }, { DraftingViewPreset::Left, "" } };
    double SlowestMs = 0.0;
    for (const bool OrthographicCondition : { false, true })
        for (const Expectation& View : Views)
        {
            Camera.OrthographicEnabled = OrthographicCondition;
            PresentDraftingView(Camera, View.Preset);
            const DraftingCameraFrame Frame = FrameDraftingCamera(Camera, Width, Height);
            const Rendered            Image = RenderFrame(Device, Raster, Frame, Style);
            SlowestMs = std::max(SlowestMs, Image.Milliseconds);
            const std::string Label = std::string(DraftingViewLabel(View.Preset)) + (OrthographicCondition ? " ortho" : " persp");
            CompareFaces(Scene, Frame, Image, Width, Height, Label);
            if (View.Face[0] != '\0')
            {
                // 📝 Sample just off-centre: the Top / Bottom centre lies between the boss and the hole.
                const uint32_t Probe = Image.Identity[size_t(Height / 2) * Width + Width / 2 + 40];
                const std::string Seen = NameDraftingIdentity(Scene, Probe);
                Tally(ViewGate, 0.0, Seen == View.Face, Label + " centre → " + (Seen.empty() ? "background" : Seen));
            }
            if (!OrthographicCondition && (View.Preset == DraftingViewPreset::Isometric || View.Preset == DraftingViewPreset::Top || View.Preset == DraftingViewPreset::Front))
                WriteExhibit(Exhibits, std::string("Viewport") + DraftingViewLabel(View.Preset), Image, Width, Height);
        }

    // ---- V3 edge strokes against the CPU nearest edge (orthographic: one world reach for every pixel)
    {
        Camera.OrthographicEnabled = true;
        PresentDraftingView(Camera, DraftingViewPreset::Isometric);
        const DraftingCameraFrame Frame = FrameDraftingCamera(Camera, Width, Height);
        const Rendered            Image = RenderFrame(Device, Raster, Frame, Style);
        long Compared = 0, Agreed = 0;
        for (uint32_t Row = 0; Row < Height; ++Row)
            for (uint32_t Column = 0; Column < Width; ++Column)
            {
                const uint32_t Identity = Image.Identity[size_t(Row) * Width + Column];
                if (!(Identity & IdentityEdgeBit)) continue;
                // 📝 At a vertex two strokes overlap and either identity is right, so the test is "explained", not "nearest".
                const DraftingRay                 Ray   = CastDraftingRay(Frame, Column + 0.5, Row + 0.5);
                const DraftingEdgeApproach        Edge  = ApproachDraftingEdge(Scene, Ray, (Identity & ~IdentityEdgeBit) - 1u);
                const std::optional<DraftingPick> Face  = PickDraftingScene(Scene, Ray, 0.0);
                const double                      Reach = (0.5 * 2.0 * Style.EdgeStroke + 1.0) * Frame.WorldPerPixel * 1.05;
                ++Compared;
                if (Edge.Separation <= Reach && (!Face || Edge.Reach <= Face->Distance + 4.0 * Reach)) ++Agreed;
            }
        const double Rate = Compared ? 100.0 * double(Agreed) / double(Compared) : 0.0;
        Tally(EdgeGate, 100.0 - Rate, Compared > 500 && Rate >= 99.5, "isometric ortho: " + std::to_string(Agreed) + " / " + std::to_string(Compared) + " edge pixels explained (unexplained %)");
    }

    // ---- V4 probe path
    {
        Camera.OrthographicEnabled = false;
        PresentDraftingView(Camera, DraftingViewPreset::Isometric);
        const DraftingCameraFrame Frame = FrameDraftingCamera(Camera, Width, Height);
        const int Spots[][2] = { { 480, 320 }, { 300, 400 }, { 700, 260 }, { 10, 10 }, { 520, 360 } };
        for (const auto& Spot : Spots)
        {
            const Rendered Image = RenderFrame(Device, Raster, Frame, Style, Spot[0], Spot[1]);
            const uint32_t Whole = Image.Identity[size_t(Spot[1]) * Width + Spot[0]];
            // 📝 Faces and background are compared with the face-only CPU pick; edge texels with the edge approach.
            const DraftingRay                 Ray  = CastDraftingRay(Frame, Spot[0] + 0.5, Spot[1] + 0.5);
            const std::optional<DraftingPick> Face = PickDraftingScene(Scene, Ray, 0.0);
            const std::string Name = NameDraftingIdentity(Scene, Image.Probe);
            bool CpuCondition = Whole == 0u ? !Face : (Face && Face->Identity == Whole);
            if (Whole & IdentityEdgeBit) CpuCondition = ApproachDraftingEdge(Scene, Ray, (Whole & ~IdentityEdgeBit) - 1u).Separation <= 3.0 * Frame.WorldPerPixel * 2.0;
            const bool AgreeCondition = Image.Probe == Whole && CpuCondition;
            Tally(ProbeGate, 0.0, AgreeCondition, "(" + std::to_string(Spot[0]) + ", " + std::to_string(Spot[1]) + ") → " + (Name.empty() ? "background" : Name));
        }
    }

    // ---- V5 tints: select the top cap and the fillet's x = 4 boundary edge (active), hover the front face
    {
        Camera.OrthographicEnabled = false;
        PresentDraftingView(Camera, DraftingViewPreset::Isometric);
        const DraftingCameraFrame Frame = FrameDraftingCamera(Camera, Width, Height);
        const Rendered Plain = RenderFrame(Device, Raster, Frame, Style);
        const uint32_t Top   = IdentityNamed(Scene, "Extrude#2.Cap(End)", false);
        const uint32_t Front = IdentityNamed(Scene, "Extrude#2.Side(Line1)", false);
        const uint32_t Side  = IdentityNamed(Scene, "Extrude#2.Side(Line2)", false);
        uint32_t FilletEdge = 0u;
        for (size_t Index = 0; Index < Scene.Edges.size() && !FilletEdge; ++Index)
            if (Scene.Edges[Index].Name.find("Blend#") != std::string::npos && Scene.Edges[Index].Name.find("Cap(End)") != std::string::npos) FilletEdge = IdentityEdgeBit | uint32_t(Index + 1);
        DraftingSelection Selection;
        Selection.Faces    = { NameDraftingIdentity(Scene, Top) };
        Selection.Edges    = { NameDraftingIdentity(Scene, FilletEdge) };
        Selection.LastKind = DraftingSelectionKind::Edge;
        const DraftingSceneStates States = ComposeDraftingStates(Scene, Selection, Front);
        Raster.AssignStates(States.Faces, States.Edges);
        const Rendered Tinted = RenderFrame(Device, Raster, Frame, Style);
        Raster.AssignStates(std::vector<uint32_t>(Scene.Faces.size(), 0u), std::vector<uint32_t>(Scene.Edges.size(), 0u));
        const auto TopBefore = MeanColour(Plain, Top), TopAfter = MeanColour(Tinted, Top);
        const auto FrontBefore = MeanColour(Plain, Front), FrontAfter = MeanColour(Tinted, Front);
        const auto SideBefore = MeanColour(Plain, Side), SideAfter = MeanColour(Tinted, Side);
        const auto EdgeAfter = MeanColour(Tinted, FilletEdge);
        const double Blueness = TopAfter[2] - TopAfter[0];
        Tally(TintGate, Blueness, Blueness > 0.25 && std::abs(TopBefore[2] - TopBefore[0]) < 0.1, "selected top cap: B − R " + std::to_string(TopBefore[2] - TopBefore[0]) + " → " + std::to_string(Blueness));
        const double Greenness = EdgeAfter[1] - std::max(EdgeAfter[0], EdgeAfter[2]) * 0.5;
        Tally(TintGate, Greenness, FilletEdge != 0u && EdgeAfter[1] > 0.6 && EdgeAfter[1] > EdgeAfter[0] + 0.3, "active fillet edge: mean RGB (" + std::to_string(EdgeAfter[0]) + ", " + std::to_string(EdgeAfter[1]) + ", " + std::to_string(EdgeAfter[2]) + ")");
        const double Lift = (FrontAfter[0] + FrontAfter[1] + FrontAfter[2]) - (FrontBefore[0] + FrontBefore[1] + FrontBefore[2]);
        Tally(TintGate, Lift, Lift > 0.05, "hovered front face brightened by " + std::to_string(Lift));
        const double Drift = std::abs(SideAfter[0] - SideBefore[0]) + std::abs(SideAfter[1] - SideBefore[1]) + std::abs(SideAfter[2] - SideBefore[2]);
        Tally(TintGate, Drift, Drift < 1.0e-6, "unselected right side unchanged (Σ|ΔRGB|)");
        WriteExhibit(Exhibits, "ViewportSelection", Tinted, Width, Height);
    }

    // ---- V8 matcaps: planar faces shade with exactly the texel their view-space normal addresses (bilinear, clamp)
    {
        Camera.OrthographicEnabled = false;
        PresentDraftingView(Camera, DraftingViewPreset::Isometric);
        const DraftingCameraFrame Frame = FrameDraftingCamera(Camera, Width, Height);
        const Rendered Plain = RenderFrame(Device, Raster, Frame, Style);
        DraftingAppearance Appearance;
        Appearance.BodyMatcap                             = "BrushedSteel";
        Appearance.FaceMatcaps["Extrude#2.Cap(End)"]      = "Brass";
        Appearance.FaceMatcaps["Extrude#2.Side(Line2)"]   = "RedPlastic";
        const DraftingMatcapLibrary Library = AssembleDraftingMatcaps(Appearance);
        Raster.UploadMatcaps(Library.Pictures);
        Raster.AssignStates(ComposeDraftingMatcapLayers(Scene, Appearance, Library), std::vector<uint32_t>(Scene.Edges.size(), 0u));
        const Rendered Shaded = RenderFrame(Device, Raster, Frame, Style);

        // Bilinear clamp-to-edge sample of a picture at uv, as the sampler does it (texel centres at (i + ½) / E).
        auto Sample = [](const DraftingPicture& Picture, double U, double V)
        {
            const double X = std::clamp(U * Picture.Width - 0.5, 0.0, double(Picture.Width - 1));
            const double Y = std::clamp(V * Picture.Height - 0.5, 0.0, double(Picture.Height - 1));
            const uint32_t Left = uint32_t(X), Top = uint32_t(Y);
            const uint32_t Right = std::min(Left + 1, Picture.Width - 1), Bottom = std::min(Top + 1, Picture.Height - 1);
            const double Across = X - Left, Down = Y - Top;
            std::array<double, 3> Value{};
            for (int Channel = 0; Channel < 3; ++Channel)
            {
                auto At = [&](uint32_t Column, uint32_t Row) { return Picture.Pixels[(size_t(Row) * Picture.Width + Column) * 4 + Channel] / 255.0; };
                Value[Channel] = (1 - Down) * ((1 - Across) * At(Left, Top) + Across * At(Right, Top)) + Down * ((1 - Across) * At(Left, Bottom) + Across * At(Right, Bottom));
            }
            return Value;
        };
        // Worst per-channel deviation over every pixel of the face from the expected colour, in 1 / 255.
        auto Deviation = [&](const Rendered& Image, uint32_t Identity, const std::array<double, 3>& Expected, long& Count)
        {
            double Worst = 0.0;
            Count = 0;
            for (size_t Index = 0; Index < Image.Identity.size(); ++Index)
            {
                if (Image.Identity[Index] != Identity) continue;
                const uint32_t Pixel = Image.Colour[Index];
                for (int Channel = 0; Channel < 3; ++Channel) Worst = std::max(Worst, std::abs(((Pixel >> (8 * Channel)) & 0xff) - Expected[Channel] * 255.0));
                ++Count;
            }
            return Worst;
        };
        const DraftingPicture Studio = ComposeDraftingStudioMatcap(DraftingMatcapExtent);
        struct Probe { const char* Face; const char* Matcap; };
        for (const Probe& Expect : { Probe{ "Extrude#2.Cap(End)", "Brass" }, Probe{ "Extrude#2.Side(Line2)", "RedPlastic" }, Probe{ "Extrude#2.Side(Line1)", "BrushedSteel" } })
        {
            const uint32_t Identity = IdentityNamed(Scene, Expect.Face, false);
            const DraftingSceneCorner& Corner = Scene.Corners[Scene.Faces[Identity - 1].FirstCorner];
            const double View[2] = { Corner.Normal[0] * Frame.Right[0] + Corner.Normal[1] * Frame.Right[1] + Corner.Normal[2] * Frame.Right[2],
                                     Corner.Normal[0] * Frame.Up[0] + Corner.Normal[1] * Frame.Up[1] + Corner.Normal[2] * Frame.Up[2] };
            const uint32_t Layer = DraftingMatcapLayer(Library, Expect.Matcap);
            const std::array<double, 3> Expected = Sample(Library.Pictures[Layer - 1], 0.5 + 0.5 * View[0], 0.5 - 0.5 * View[1]);
            long Count = 0;
            const double Worst = Deviation(Shaded, Identity, Expected, Count);
            char Note[300];
            std::snprintf(Note, sizeof(Note), "%s → %s (layer %u): view normal (%.3f, %.3f), %ld pixels within %.2f / 255 of the texel (RGB %.0f %.0f %.0f)", Expect.Face, Expect.Matcap,
                          Layer, View[0], View[1], Count, Worst, Expected[0] * 255, Expected[1] * 255, Expected[2] * 255);
            Tally(MatcapGate, Worst, Count > 500 && Worst <= 2.0, Note);
            // Studio (layer 0) is analytic in the shader; its CPU mirror sampled at the same normal must agree.
            const std::array<double, 3> Analytic = Sample(Studio, 0.5 + 0.5 * View[0], 0.5 - 0.5 * View[1]);
            const double StudioWorst = Deviation(Plain, Identity, Analytic, Count);
            Tally(MatcapGate, StudioWorst, Count > 500 && StudioWorst <= 2.0, std::string(Expect.Face) + " without matcap: Studio shader = CPU Studio mirror within " + Compact(StudioWorst) + " / 255");
        }
        // Back to Studio: the frame is bit-identical to the one before any matcap.
        Raster.AssignStates(std::vector<uint32_t>(Scene.Faces.size(), 0u), std::vector<uint32_t>(Scene.Edges.size(), 0u));
        const Rendered Restored = RenderFrame(Device, Raster, Frame, Style);
        Tally(MatcapGate, 0.0, Restored.Colour == Plain.Colour && Shaded.Identity == Plain.Identity,
              "layers cleared → colour bit-identical to the Studio frame; identity unchanged by matcaps");
        WriteExhibit(Exhibits, "ViewportMatcaps", Shaded, Width, Height);
    }

    // ---- V7 determinism
    {
        PresentDraftingView(Camera, DraftingViewPreset::Isometric);
        const DraftingCameraFrame Frame = FrameDraftingCamera(Camera, Width, Height);
        const Rendered First  = RenderFrame(Device, Raster, Frame, Style);
        const Rendered Second = RenderFrame(Device, Raster, Frame, Style);
        const std::string ColourDigest   = DigestDraftingPixels(First.Colour.data(), First.Colour.size());
        const std::string IdentityDigest = DigestDraftingPixels(First.Identity.data(), First.Identity.size());
        const bool EqualCondition = ColourDigest == DigestDraftingPixels(Second.Colour.data(), Second.Colour.size()) &&
                                    IdentityDigest == DigestDraftingPixels(Second.Identity.data(), Second.Identity.size());
        Tally(DeterminismGate, 0.0, EqualCondition, "colour " + ColourDigest + " · identity " + IdentityDigest);
    }

    // ---- report
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    std::printf("# DraftingViewportValidation\n\n");
    std::printf("Viewport of DraftingWorkspace on %s — specimen: %zu faces, %zu edges, %zu triangles; %u × %u; slowest frame %.1f ms (%.1f s total).\n\n",
                Device.Describe().c_str(), Scene.Faces.size(), Scene.Edges.size(), Scene.Corners.size() / 3, Width, Height, SlowestMs, Seconds);
    std::printf("| Gate | Subject | Limit | Worst | Checks | Verdict |\n|------|---------|-------|-------|--------|---------|\n");
    long Failures = 0;
    for (const GateTally* Gate : { &DeviceGate, &FaceGate, &EdgeGate, &ProbeGate, &TintGate, &ViewGate, &DeterminismGate, &MatcapGate })
    {
        std::printf("| %s | %s | %s | %.4g | %ld | %s |\n", Gate->Code, Gate->Subject, Gate->Limit, Gate->Worst, Gate->Checks, Gate->Failures ? "🔴" : "🟢");
        Failures += Gate->Failures;
    }
    std::printf("\n## Walkthrough\n\n| Gate | Check | Value | Verdict |\n|------|-------|-------|---------|\n");
    for (const std::string& Row : Journal) std::printf("%s\n", Row.c_str());
    Raster.Destroy();
    return Failures == 0 ? 0 : 1;
}
