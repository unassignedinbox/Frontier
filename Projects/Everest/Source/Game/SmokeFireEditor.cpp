/*==========================================================================================================================================
                                                         SMOKEFIREDITOR.CPP
==========================================================================================================================================*/

#include "Projects/Everest/Source/Game/SmokeFireEditor.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace Everest
{
namespace
{
using namespace Frontier;

constexpr size_t Idx(GasParameter Id) { return static_cast<size_t>(Id); }
constexpr ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
    | ImGuiWindowFlags_NoBringToFrontOnFocus;

void BeginPanel(const char* Name, ImVec2 Position, ImVec2 Size)
{
    ImGui::SetNextWindowPos(Position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(Size, ImGuiCond_Always);
    ImGui::Begin(Name, nullptr, PanelFlags);
}

bool Slider(SmokeFluidSettings& Settings, GasParameter Id)
{
    const GasParameterSpec& Spec = QueryGasParameterCatalogue()[Idx(Id)];
    float& Value = Settings.Gas[Idx(Id)];
    const float Before = Value;
    ImGui::PushID(static_cast<int>(Id));
    ImGui::TextUnformatted(Spec.Label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - 90.0f));
    const int Digits = Spec.Step >= 1.0f ? 0 : Spec.Step >= 0.1f ? 1 : Spec.Step >= 0.01f ? 2 : 3;
    char Format[32];
    std::snprintf(Format, sizeof(Format), "%%.%df %%s", Digits);
    ImGui::Text(Format, Value, Spec.Unit);
    const bool Implemented = Id != GasParameter::Albedo && Id != GasParameter::Phase &&
        Id != GasParameter::Detail && Id != GasParameter::DetailScale && Id != GasParameter::Erosion &&
        Id != GasParameter::Warp && Id != GasParameter::Powder && Id != GasParameter::FlameCore &&
        Id != GasParameter::FlameTip && Id != GasParameter::FlameTemperature;
    if (!Implemented) ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(-1.0f);
    char Widget[40]; std::snprintf(Widget, sizeof(Widget), "##%s", Spec.Key);
    ImGui::SliderFloat(Widget, &Value, Spec.Minimum, Spec.Maximum, "", ImGuiSliderFlags_AlwaysClamp);
    if (!Implemented) ImGui::EndDisabled();
    if (Value != Before)
    {
        Value = std::clamp(std::round(Value / Spec.Step) * Spec.Step, Spec.Minimum, Spec.Maximum);
        ImGui::Spacing();
        ImGui::PopID();
        return true;
    }
    ImGui::Spacing();
    ImGui::PopID();
    return false;
}

void DrawDecayInstrument(const SmokeFluidSettings& S, float Width)
{
    const ImVec2 P = ImGui::GetCursorScreenPos();
    const float H = 98.0f;
    ImDrawList* Draw = ImGui::GetWindowDrawList();
    const ImU32 Grid = IM_COL32(44, 44, 44, 255);
    for (int I = 0; I <= 4; ++I)
    {
        const float Y = P.y + 8.0f + (H - 24.0f) * I / 4.0f;
        Draw->AddLine({ P.x + 12.0f, Y }, { P.x + Width - 10.0f, Y }, Grid);
    }
    const auto Curve = [&](GasParameter Id, ImU32 Colour)
    {
        const float Loss = S.Get(Id);
        ImVec2 Previous{};
        for (int I = 0; I <= 64; ++I)
        {
            const float T = 12.0f * I / 64.0f;
            const float Survival = std::pow(std::max(0.0f, 1.0f - Loss), T);
            const ImVec2 Point{ P.x + 12.0f + (Width - 22.0f) * I / 64.0f,
                                P.y + H - 15.0f - (H - 26.0f) * Survival };
            if (I) Draw->AddLine(Previous, Point, Colour, 2.0f);
            Previous = Point;
        }
    };
    Curve(GasParameter::Dissipation, IM_COL32(216, 216, 216, 255));
    Curve(GasParameter::Cooling, IM_COL32(232, 131, 58, 255));
    Draw->AddText({ P.x + 12.0f, P.y + H - 14.0f }, IM_COL32(160, 160, 160, 255), "density");
    Draw->AddText({ P.x + 74.0f, P.y + H - 14.0f }, IM_COL32(232, 131, 58, 255), "heat");
    ImGui::Dummy({ Width, H });
}

void DrawFireInstrument(const SmokeFluidSettings& S, float Width)
{
    const ImVec2 P = ImGui::GetCursorScreenPos();
    const float H = 58.0f;
    ImDrawList* Draw = ImGui::GetWindowDrawList();
    const float Temperature = S.Get(GasParameter::FlameCore) * S.Get(GasParameter::FlameTemperature);
    const float Hot = std::clamp((Temperature - 900.0f) / 1500.0f, 0.0f, 1.0f);
    for (int I = 0; I < 64; ++I)
    {
        const float T = I / 63.0f;
        const float Orange = std::clamp(1.4f * (1.0f - std::fabs(T - Hot)), 0.0f, 1.0f);
        Draw->AddRectFilled({ P.x + Width * I / 64.0f, P.y + 7.0f },
                            { P.x + Width * (I + 1) / 64.0f, P.y + H - 6.0f },
                            IM_COL32(static_cast<int>(255 * Orange), static_cast<int>(180 * Orange * T), static_cast<int>(55 * T), 255));
    }
    Draw->AddText({ P.x + 5.0f, P.y + H - 21.0f }, IM_COL32(255, 255, 255, 230), "900 K");
    Draw->AddText({ P.x + Width - 45.0f, P.y + H - 21.0f }, IM_COL32(255, 255, 255, 230), "2400 K");
    ImGui::Dummy({ Width, H });
}

void SectionTitle(const char* Name, const char* Subtitle = nullptr)
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.91f, 0.52f, 0.24f, 1.0f), "%s", Name);
    if (Subtitle)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("· %s", Subtitle);
    }
    ImGui::Spacing();
}

} // namespace

void SmokeFireEditor::Draw(SmokeFluidSettings& Settings, const SmokeFluidMetrics& Metrics,
                           const std::array<int, 3>& Extent, bool Resident)
{
    const ImGuiIO& IO = ImGui::GetIO();
    const float DisplayWidth = IO.DisplaySize.x;
    const float DisplayHeight = IO.DisplaySize.y;
    const float LeftWidth = std::clamp(DisplayWidth * 0.23f, 220.0f, 288.0f);
    const float RightWidth = std::clamp(DisplayWidth * 0.30f, 330.0f, 390.0f);
    const float StatusHeight = 26.0f;
    const float PanelHeight = std::max(200.0f, DisplayHeight - StatusHeight);

    BeginPanel("##frontier-outliner", { 0, 0 }, { LeftWidth, PanelHeight });
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 222, 222, 255));
    ImGui::TextColored({ 0.78f, 0.78f, 0.78f, 1.0f }, "◈  frontier");
    ImGui::SameLine(); ImGui::TextDisabled("ENGINE / 01");
    ImGui::Spacing();
    ImGui::TextDisabled("WORKSPACE");
    ImGui::Text("Proving Grounds.scene");
    ImGui::TextDisabled("EFFECTS");
    const auto Item = [&](const char* Label, Selection Value, const char* Meta)
    {
        const bool Active = Selected == Value;
        if (Active) ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(65, 48, 36, 255));
        if (ImGui::Selectable(Label, Active, ImGuiSelectableFlags_AllowDoubleClick, { LeftWidth - 76.0f, 34.0f }))
        {
            Selected = Value;
            FocusSelection = true;
        }
        if (Meta)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", Meta);
        }
        if (Active) ImGui::PopStyleColor();
    };
    Item("Smoke", Selection::Smoke, Resident ? "live" : "idle");
    Item("Fluid", Selection::Fluid, "solver");
    Item("Plume", Selection::Plume, "volume");
    Item("Fire", Selection::Fire, "gas");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("IlluminationHost");
    ImGui::TextDisabled("Distance-field SDF world");
    ImGui::PopStyleColor();
    ImGui::End();

    BeginPanel("##smoke-inspector", { DisplayWidth - RightWidth, 0 }, { RightWidth, PanelHeight });
    ImGui::TextUnformatted("Inspector");
    ImGui::SameLine();
    const char* SelectionLabel = Selected == Selection::Smoke ? "Smoke & fire" : Selected == Selection::Fluid ? "Fluid domain" : Selected == Selection::Plume ? "Plume" : "Fire";
    ImGui::TextDisabled("/ %s", SelectionLabel);
    ImGui::Separator();
    ImGui::BeginChild("##smoke-scroll", { 0, 0 }, false);
    const float ContentWidth = ImGui::GetContentRegionAvail().x;

    SectionTitle("Domain", "moving CPU lattice");
    char ExtentText[64];
    std::snprintf(ExtentText, sizeof(ExtentText), "%d × %d × %d", Extent[0], Extent[1], Extent[2]);
    ImGui::Text("%s", ExtentText); ImGui::SameLine(); ImGui::TextDisabled("cells"); ImGui::SameLine();
    ImGui::TextColored(Resident ? ImVec4(0.24f, 0.77f, 0.43f, 1) : ImVec4(0.49f, 0.49f, 0.49f, 1), "%s", Resident ? "resident" : "idle");
    ImGui::TextDisabled("CPU preview · single lattice / depth-clipped march");
    ImGui::Text("Sources  %u     Ticks  %llu     Relocations  %u", Metrics.Sources,
                static_cast<unsigned long long>(Metrics.Ticks), Metrics.Relocations);
    ImGui::Checkbox("Enable smoke and fire", &Settings.Enabled);
    const char* GradeNames[] = { "Low", "Medium", "High", "Ultra" };
    int Grade = Settings.QualityGrade;
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("Quality grade", &Grade, GradeNames, 4)) Settings.QualityGrade = static_cast<uint8_t>(Grade);
    const char* TestNames[] = { "Off", "Smoke plume", "Fire" };
    int Test = Settings.TestPlume;
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("Test emitter · 3 m ahead of the car", &Test, TestNames, 3)) Settings.TestPlume = static_cast<uint8_t>(Test);
    if (ImGui::Button("Clear smoke", { ContentWidth * 0.48f, 0 })) ClearRequested = true;
    ImGui::SameLine();
    if (ImGui::Button("Defaults", { ContentWidth * 0.48f, 0 })) Settings.ResetGasDefaults();

    if (FocusSelection && Selected == Selection::Smoke) ImGui::SetScrollHereY(0.0f);
    SectionTitle("Source", "multipliers on the tyre model");
    Slider(Settings, GasParameter::Emission); Slider(Settings, GasParameter::Heat); Slider(Settings, GasParameter::Soot);
    if (FocusSelection && Selected == Selection::Fluid) ImGui::SetScrollHereY(0.0f);
    SectionTitle("Simulation");
    DrawDecayInstrument(Settings, ContentWidth);
    Slider(Settings, GasParameter::Buoyancy); Slider(Settings, GasParameter::Dissipation); Slider(Settings, GasParameter::Cooling);
    Slider(Settings, GasParameter::Vorticity); Slider(Settings, GasParameter::Turbulence); Slider(Settings, GasParameter::TurbulenceScale);
    Slider(Settings, GasParameter::WindSpeed); Slider(Settings, GasParameter::WindHeading); Slider(Settings, GasParameter::TimeScale);
    Slider(Settings, GasParameter::PressureSweeps);
    if (FocusSelection && Selected == Selection::Plume) ImGui::SetScrollHereY(0.0f);
    SectionTitle("Appearance");
    ImGui::TextWrapped("CPU preview: some exhibit appearance controls are not yet implemented.");
    Slider(Settings, GasParameter::Density); Slider(Settings, GasParameter::Albedo); Slider(Settings, GasParameter::Phase);
    Slider(Settings, GasParameter::MultiScatter); Slider(Settings, GasParameter::Ambient); Slider(Settings, GasParameter::Detail);
    Slider(Settings, GasParameter::DetailScale); Slider(Settings, GasParameter::Erosion); Slider(Settings, GasParameter::Warp); Slider(Settings, GasParameter::Powder);
    if (FocusSelection && Selected == Selection::Fire) ImGui::SetScrollHereY(0.0f);
    SectionTitle("Fire", "combustion of the tyre");
    DrawFireInstrument(Settings, ContentWidth);
    ImGui::TextWrapped("Tyre ignition is not yet connected in the CPU preview. Use the test emitter above.");
    const char* IgnitionOptions[] = { "Authored · 350 °C held 4 s", "Diagnostic · 160 °C held 1 s", "Off" };
    int IgnitionIndex = Settings.IgnitionMode;
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::BeginDisabled();
    ImGui::Combo("Ignition", &IgnitionIndex, IgnitionOptions, 3);
    ImGui::EndDisabled();
    Slider(Settings, GasParameter::FlameIntensity); Slider(Settings, GasParameter::FlameCore); Slider(Settings, GasParameter::FlameTip);
    Slider(Settings, GasParameter::FlameTemperature); Slider(Settings, GasParameter::FlameLoss); Slider(Settings, GasParameter::FireLight);
    Slider(Settings, GasParameter::FlameFlicker);

    SectionTitle("Presets");
    const char* Presets[] = { "Tyre smoke", "Thick burnout", "Light haze", "Inferno", "Crosswind", "Legacy billows" };
    for (int I = 0; I < 6; ++I)
    {
        if (I % 2) ImGui::SameLine();
        if (ImGui::Button(Presets[I], { ContentWidth * 0.48f, 0 })) Settings.ApplyPreset(Presets[I]);
    }
    ImGui::TextDisabled("CPU preview · %s · %s", GradeNames[Settings.QualityGrade], Resident ? "resident" : "idle");
    FocusSelection = false;
    ImGui::EndChild();
    ImGui::End();

    BeginPanel("##frontier-status", { 0, DisplayHeight - StatusHeight }, { DisplayWidth, StatusHeight });
    ImGui::TextDisabled("REALTIME   ·   FLUID  %u sources / %llu ticks   ·   QUALITY  %s   ·   SDF  INTERACTIVE",
                        Metrics.Sources, static_cast<unsigned long long>(Metrics.Ticks), GradeNames[Settings.QualityGrade]);
    ImGui::End();
}

bool SmokeFireEditor::ConsumeClearRequest() noexcept
{
    const bool Requested = ClearRequested;
    ClearRequested = false;
    return Requested;
}

} // namespace Everest
