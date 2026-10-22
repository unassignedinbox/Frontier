/*====================================================================================================================================
                                                      DRAFTINGSCRIPT.CPP
====================================================================================================================================*/
// 🧩 Scripted pointer / keyboard input for headless workspace captures, with expectations on the resulting state

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Interface/DraftingScript.h"
#include "Sketch/SketchSpline.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <sstream>

namespace Frontier::DraftingWorkspace
{

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PARSING
//------------------------------------------------------------------------------------------------------------------------

std::vector<std::string> SplitWords(const std::string& Line)
{
    std::vector<std::string> Words;
    std::istringstream       Stream(Line);
    std::string              Word;
    while (Stream >> Word) Words.push_back(Word);
    return Words;
}

ImGuiKey KeyByName(const std::string& Name)
{
    static const std::map<std::string, ImGuiKey> Keys = {
        { "A", ImGuiKey_A }, { "B", ImGuiKey_B }, { "C", ImGuiKey_C }, { "D", ImGuiKey_D }, { "E", ImGuiKey_E }, { "F", ImGuiKey_F },
        { "G", ImGuiKey_G }, { "H", ImGuiKey_H }, { "I", ImGuiKey_I }, { "J", ImGuiKey_J }, { "K", ImGuiKey_K }, { "L", ImGuiKey_L }, { "M", ImGuiKey_M }, { "N", ImGuiKey_N }, { "O", ImGuiKey_O }, { "P", ImGuiKey_P }, { "Q", ImGuiKey_Q }, { "R", ImGuiKey_R },
        { "S", ImGuiKey_S }, { "T", ImGuiKey_T }, { "U", ImGuiKey_U }, { "V", ImGuiKey_V }, { "W", ImGuiKey_W }, { "X", ImGuiKey_X }, { "Y", ImGuiKey_Y }, { "Z", ImGuiKey_Z },
        { "1", ImGuiKey_1 }, { "2", ImGuiKey_2 }, { "3", ImGuiKey_3 }, { "4", ImGuiKey_4 }, { "5", ImGuiKey_5 },
        { "Keypad1", ImGuiKey_Keypad1 }, { "Keypad3", ImGuiKey_Keypad3 }, { "Keypad5", ImGuiKey_Keypad5 }, { "Keypad7", ImGuiKey_Keypad7 },
        { "Space", ImGuiKey_Space }, { "Enter", ImGuiKey_Enter }, { "Escape", ImGuiKey_Escape },
        { "Delete", ImGuiKey_Delete }, { "Backspace", ImGuiKey_Backspace }, { "Tab", ImGuiKey_Tab },
        { "LeftBracket", ImGuiKey_LeftBracket }, { "RightBracket", ImGuiKey_RightBracket },
    };
    const auto Found = Keys.find(Name);
    return Found == Keys.end() ? ImGuiKey_None : Found->second;
}

int ButtonByName(const std::string& Name)
{
    if (Name == "Right") return ImGuiMouseButton_Right;
    if (Name == "Middle") return ImGuiMouseButton_Middle;
    return ImGuiMouseButton_Left;
}

// Consumes a target from Words at Index: "@Widget", "world X Y Z", "viewport U V" or "X Y", plus optional "+DX +DY".
bool ReadTarget(const std::vector<std::string>& Words, size_t& Index, DraftingScriptTarget& Target)
{
    if (Index >= Words.size()) return false;
    try
    {
        if (Words[Index][0] == '@')
        {
            Target.Widget = Words[Index].substr(1);
            Index += 1;
        }
        else if (Words[Index] == "world" && Index + 3 < Words.size())
        {
            Target.World = true;
            for (int Axis = 0; Axis < 3; ++Axis) Target.Values[Axis] = std::stod(Words[Index + 1 + Axis]);
            Index += 4;
        }
        else if (Words[Index] == "viewport" && Index + 2 < Words.size())
        {
            Target.Fraction  = true;
            Target.Values[0] = std::stod(Words[Index + 1]);
            Target.Values[1] = std::stod(Words[Index + 2]);
            Index += 3;
        }
        else if (Index + 1 < Words.size())
        {
            Target.Values[0] = std::stod(Words[Index]);
            Target.Values[1] = std::stod(Words[Index + 1]);
            Index += 2;
        }
        else
        {
            return false;
        }
        if (Index + 1 < Words.size() && Words[Index] == "offset")
        {
            Target.OffsetX = std::stof(Words[Index + 1]);
            Target.OffsetY = Index + 2 < Words.size() ? std::stof(Words[Index + 2]) : 0.0f;
            Index += 3;
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}

void ReadModifiers(const std::vector<std::string>& Words, size_t Index, bool& Control, bool& Shift, bool& Alternate)
{
    for (; Index < Words.size(); ++Index)
    {
        if (Words[Index] == "Ctrl") Control = true;
        if (Words[Index] == "Shift") Shift = true;
        if (Words[Index] == "Alt") Alternate = true;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    EXECUTION
//------------------------------------------------------------------------------------------------------------------------

bool ResolveTarget(const DraftingScriptTarget& Target, const DraftingWorkspaceState& State, ImVec2& Point)
{
    const auto& Rect = State.ViewportRect;
    if (!Target.Widget.empty())
    {
        const auto Found = State.Widgets.find(Target.Widget);
        if (Found == State.Widgets.end()) return false;
        Point = ImVec2(0.5f * (Found->second.x + Found->second.z), 0.5f * (Found->second.y + Found->second.w));
    }
    else if (Target.World)
    {
        const DraftingCameraFrame Frame = FrameDraftingCamera(State.Camera, Rect[2], Rect[3]);
        double Clip[4] = {};
        for (int Row = 0; Row < 4; ++Row)
            Clip[Row] = Frame.ClipFromWorld[0 * 4 + Row] * Target.Values[0] + Frame.ClipFromWorld[1 * 4 + Row] * Target.Values[1] +
                        Frame.ClipFromWorld[2 * 4 + Row] * Target.Values[2] + Frame.ClipFromWorld[3 * 4 + Row];
        if (Clip[3] <= 0.0) return false;
        Point = ImVec2(Rect[0] + float((Clip[0] / Clip[3] * 0.5 + 0.5) * Rect[2]), Rect[1] + float((Clip[1] / Clip[3] * 0.5 + 0.5) * Rect[3]));
    }
    else if (Target.Fraction)
    {
        Point = ImVec2(Rect[0] + float(Target.Values[0]) * Rect[2], Rect[1] + float(Target.Values[1]) * Rect[3]);
    }
    else
    {
        Point = ImVec2(float(Target.Values[0]), float(Target.Values[1]));
    }
    Point.x += Target.OffsetX;
    Point.y += Target.OffsetY;
    return true;
}

// 📝 Observed value of a named state field, as text and (when numeric) as a number.
bool ObserveField(const DraftingWorkspaceState& State, const std::string& Field, std::string& Text, double& Number)
{
    const DraftingDocument& Document = State.Document;
    auto Numeric = [&](double Value)
    {
        Number = Value;
        char Buffer[64];
        std::snprintf(Buffer, sizeof(Buffer), "%.10g", Value);
        Text = Buffer;
        return true;
    };
    auto Join = [&](const std::vector<std::string>& Names)
    {
        Text.clear();
        for (const std::string& Name : Names) Text += (Text.empty() ? "" : ",") + Name;
        Number = double(Names.size());
        return true;
    };
    if (Field == "faces") return Numeric(Document.Summary.Faces);
    if (Field == "edges") return Numeric(Document.Summary.Edges);
    if (Field == "genus") return Numeric(Document.Summary.Genus);
    if (Field == "volume") return Numeric(Document.Summary.Volume);
    if (Field == "sound") return Numeric(Document.Summary.SoundCondition ? 1 : 0);
    if (Field == "steps") return Numeric(double(Document.Tree.Steps.size()));
    if (Field == "refused") return Numeric(Document.Regeneration.Refused);
    if (Field == "replay-resumed") return Numeric(Document.Regeneration.Resumed);
    if (Field == "replay-ms") return Numeric(Document.Summary.RegenerationMs);
    if (Field == "preview-resumed")
    {
        // The staged preview of the open session (bevel, move / rotate / scale, revolve / sweep); −1 without one.
        const DraftingDocument* Preview = State.Bevel.Preview ? State.Bevel.Preview.get() : State.Transform.Preview ? State.Transform.Preview.get() : State.Sweep.Preview.get();
        return Numeric(Preview ? Preview->Regeneration.Resumed : -1);
    }
    if (Field == "undo") return Numeric(double(Document.UndoEntries.size()));
    if (Field == "redo") return Numeric(double(Document.RedoEntries.size()));
    if (Field == "selected-faces") return Join(Document.Selection.Faces);
    if (Field == "selected-edges") return Join(Document.Selection.Edges);
    if (Field == "selected-bodies") return Join(Document.Selection.Bodies);
    if (Field == "bodies") return Numeric(double(Document.Summary.Bodies.size()));
    if (Field == "active-body") { Text = Document.Regeneration.Active; Number = double(Text.size()); return true; }
    if (Field == "body-captions")
    {
        std::vector<std::string> Captions;
        for (const DraftingBodyFigures& Figures : Document.Summary.Bodies) Captions.push_back(Figures.Caption);
        return Join(Captions);
    }
    if (Field == "body-volumes")
    {
        // "Block1 1.000000 · Cylinder1 0.392699" (creation order), Number = the count.
        Text.clear();
        for (const DraftingBodyFigures& Figures : Document.Summary.Bodies)
        {
            char Line[96];
            std::snprintf(Line, sizeof(Line), "%s%s %.6f", Text.empty() ? "" : " · ", Figures.Caption.c_str(), Figures.Volume);
            Text += Line;
        }
        Number = double(Document.Summary.Bodies.size());
        return true;
    }
    if (Field == "body-matcaps") return Numeric(double(Document.Appearance.BodyMatcaps.size()));
    if (Field == "boolean") return Numeric(State.Boolean.ActiveCondition ? 1 : 0);
    if (Field == "boolean-commits") return Numeric(double(State.Boolean.Commits));
    if (Field == "transform") return Numeric(State.Transform.ActiveCondition ? 1 : 0);
    if (Field == "transform-commits") return Numeric(double(State.Transform.Commits));
    if (Field == "transform-readout") { Text = State.Transform.Readout; Number = double(Text.size()); return true; }
    if (Field == "transform-preview-volume") return Numeric(State.Transform.Preview ? State.Transform.Preview->Summary.Volume : 0.0);
    if (Field == "sweep") return Numeric(State.Sweep.ActiveCondition ? 1 : 0);
    if (Field == "sweep-commits") return Numeric(double(State.Sweep.Commits));
    if (Field == "sweep-amount") return Numeric(State.Sweep.Amount);
    if (Field == "sweep-refusal") { Text = State.Sweep.Refusal; Number = double(Text.size()); return true; }
    if (Field == "sweep-outline") return Numeric(double(State.Sweep.Outline.size()));
    if (Field == "sweep-verb") { Text = State.Sweep.Verb == DraftingSweepVerb::Offset ? "Offset" : State.Sweep.Verb == DraftingSweepVerb::Pipe ? "Pipe" : State.Sweep.Verb == DraftingSweepVerb::Revolve ? "Revolve" : "Evolution"; Number = double(int(State.Sweep.Verb)); return true; }
    if (Field == "sweep-axis") { Text = DescribeDraftingRevolveAxis(State.Document, State.Sweep.Path, State.Sweep.Axis); Number = double(State.Sweep.Axis.Line); return true; }
    if (Field == "sweep-path") { Text = State.Sweep.Path; Number = double(Text.size()); return true; }
    if (Field == "sweep-profile") { Text = State.Sweep.Profile; Number = double(Text.size()); return true; }
    if (Field == "sweep-hover") { Text = State.Sweep.HoverSketch; Number = double(State.Sweep.HoverCurve); return true; }
    if (Field == "sweep-preview") return Numeric(State.Sweep.Preview ? 1 : 0);
    if (Field == "hidden") return Numeric(double(Document.Appearance.HiddenBodies.size()));
    if (Field == "scene-bodies") return Numeric(double(State.Scene.Bodies.size()));
    if (Field == "clipboard") return Numeric(double(State.Clipboard.Keys.size()));
    if (Field == "constrain") return Numeric(State.Constrain.ActiveCondition ? 1 : 0);
    if (Field == "constrain-commits") return Numeric(double(State.Constrain.Commits));
    if (Field == "constrain-refusal") { Text = State.Constrain.Refusal; Number = double(Text.size()); return true; }
    if (Field.starts_with("constrain") || Field.starts_with("constraint") || Field.starts_with("sketch-") || Field == "picked-constraint")
    {
        // 📝 The K session's sketch (else the focused sketch step) as regenerated.
        const std::string Label  = DraftingConstraintSketch(State);
        const auto        Solved = Document.Regeneration.Sketches.find(Label);
        const Modeller::SketchModel*        Sketch = Solved != Document.Regeneration.Sketches.end() ? &Solved->second : nullptr;
        const Modeller::SketchSolveOutcome* Solve  = nullptr;
        for (const Modeller::FeatureStepReport& Report : Document.Regeneration.Steps) if (Report.Label == Label) Solve = &Report.Solve;
        if (!Sketch) { Text.clear(); Number = -1.0; return true; }
        if (Field == "constrain-picks")
        {
            std::vector<std::string> Names;
            for (const DraftingSketchPick& Pick : State.Constrain.Picks)
            {
                if (Pick.Sketch.empty()) { Names.push_back(Modeller::NameSketchPick(*Sketch, Pick.Pick)); continue; }
                const auto Other   = Document.Regeneration.Sketches.find(Pick.Sketch);
                const auto Caption = Document.Captions.find(Pick.Sketch);
                Names.push_back((Other != Document.Regeneration.Sketches.end() ? Modeller::NameSketchPick(Other->second, Pick.Pick) : std::string("?")) + " of "
                                + (Caption != Document.Captions.end() ? Caption->second : Pick.Sketch));
            }
            return Join(Names);
        }
        if (Field == "sketch-links")
        {
            // Linked points + curves of the sketch: "Point2,Line2.Start,…,Line2", Number = the count.
            std::vector<std::string> Names;
            for (int Point = 0; Point < int(Sketch->Points.size()); ++Point)
                if (!Sketch->Points[Point].Link.Sketch.empty()) Names.push_back(Modeller::NameSketchPick(*Sketch, { Modeller::SketchPickKind::Point, Point }));
            for (int Curve = 0; Curve < int(Sketch->Curves.size()); ++Curve)
                if (!Sketch->Curves[Curve].Link.Sketch.empty()) Names.push_back(Modeller::SketchCurveName(Sketch->Curves[Curve]));
            return Join(Names);
        }
        if (Field == "constraint-count") return Numeric(double(Sketch->Constraints.size()));
        if (Field == "constraints")
        {
            std::vector<std::string> Labels;
            for (const Modeller::SketchConstraint& Constraint : Sketch->Constraints) Labels.push_back(Constraint.Label.empty() ? std::string("-") : Constraint.Label);
            return Join(Labels);
        }
        if (Field == "sketch-dof") return Numeric(Solve ? double(Solve->DegreesOfFreedom) : -1.0);
        if (Field == "sketch-status")
        {
            Text   = !Solve ? std::string("unsolved") : (!Solve->ConflictGroups.empty() || !Solve->Converged) ? std::string("conflicting") : Solve->DegreesOfFreedom == 0 ? std::string("fully constrained") : std::string("under-constrained");
            Number = Solve ? double(Solve->DegreesOfFreedom) : -1.0;
            return true;
        }
        if (Field == "picked-constraint")
        {
            const int Picked = State.Constrain.ActiveCondition ? State.Constrain.PickedConstraint : -1;
            Text   = Picked >= 0 && Picked < int(Sketch->Constraints.size()) ? Sketch->Constraints[Picked].Label + " " + Modeller::DescribeSketchConstraint(*Sketch, Sketch->Constraints[Picked]) : std::string{};
            Number = double(Picked);
            return true;
        }
        if (Field == "sketch-points")
        {
            // "(0, 0) (3, 0) …" at %.6g, Number = the count.
            Text.clear();
            for (const Modeller::SketchPoint& Point : Sketch->Points)
            {
                char Pair[80];
                std::snprintf(Pair, sizeof(Pair), "%s(%.6g, %.6g)", Text.empty() ? "" : " ", std::abs(Point.Position.X) < 5.0e-13 ? 0.0 : Point.Position.X, std::abs(Point.Position.Y) < 5.0e-13 ? 0.0 : Point.Position.Y);
                Text += Pair;
            }
            Number = double(Sketch->Points.size());
            return true;
        }
    }
    if (Field == "trim") return Numeric(State.Trim.ActiveCondition ? 1 : 0);
    if (Field == "trim-commits") return Numeric(double(State.Trim.Commits));
    if (Field == "trim-refusal")
    {
        Text   = State.Trim.Refusal;
        Number = double(Text.size());
        return true;
    }
    if (Field == "trim-hover")
    {
        Text.clear();
        if (State.Trim.HoverCondition)
        {
            const auto Caption = Document.Captions.find(State.Trim.Sketch);
            const auto Solved  = Document.Regeneration.Sketches.find(State.Trim.Sketch);
            Text = (Caption != Document.Captions.end() ? Caption->second : State.Trim.Sketch);
            if (Solved != Document.Regeneration.Sketches.end() && State.Trim.Curve >= 0 && State.Trim.Curve < int(Solved->second.Curves.size()))
                Text += " · " + Modeller::SketchCurveName(Solved->second.Curves[State.Trim.Curve]);
        }
        Number = double(Text.size());
        return true;
    }
    if (Field == "sketch-curve-total")
    {
        double Total = 0.0;
        for (const auto& [Label, Sketch] : Document.Regeneration.Sketches) Total += double(Sketch.Curves.size());
        return Numeric(Total);
    }
    if (Field == "pick-mode") return Numeric(int(State.PickMode));
    if (Field == "surfaces") return Numeric(State.Style.SurfacesEnabled ? 1 : 0);
    if (Field == "xray") return Numeric(State.Style.XRayEnabled ? 1 : 0);
    if (Field == "measure")
    {
        // Text = subject (or the refusal), Number = 1 while Measure (I) is shown.
        const DraftingMeasurement& Measurement = State.Measure.Measurement;
        Text   = !State.Measure.ActiveCondition ? std::string("off") : Measurement.Refusal.empty() ? Measurement.Subject : Measurement.Refusal;
        Number = State.Measure.ActiveCondition ? 1.0 : 0.0;
        return true;
    }
    if (Field == "measure-distance") return Numeric(State.Measure.Measurement.Distance);
    if (Field == "measure-angle") return Numeric(State.Measure.Measurement.Angle);
    if (Field == "measure-area") return Numeric(State.Measure.Measurement.Area);
    if (Field == "measure-length") return Numeric(State.Measure.Measurement.Length);
    if (Field == "measure-volume") return Numeric(State.Measure.Measurement.Volume);
    if (Field == "measure-refreshes") return Numeric(State.Measure.Refreshes);
    if (Field == "construct-open") return Numeric(State.ConstructShown ? 1 : 0);
    if (Field == "construct-combination") return Numeric(int(State.ConstructForm.Combination));
    if (Field == "bevel-active") return Numeric(State.Bevel.ActiveCondition ? 1 : 0);
    if (Field == "bevel-size") return Numeric(State.Bevel.Size);
    if (Field == "bevel-pinned") return Numeric(State.Bevel.PinnedCondition ? 1 : 0);
    if (Field == "bevel-chamfer") return Numeric(State.Bevel.Shape == Modeller::BlendProfile::Chamfer ? 1 : 0);
    if (Field == "bevel-variable") return Numeric(State.Bevel.VariableEnabled ? 1 : 0);
    if (Field == "bevel-end-size") return Numeric(State.Bevel.EndSize);
    if (Field == "bevel-flipped") return Numeric(State.Bevel.ReversedCondition ? 1 : 0);
    if (Field == "bevel-preview-volume") return Numeric(State.Bevel.Preview ? State.Bevel.Preview->Summary.Volume : 0.0);
    if (Field == "bevel-refusal") { Text = State.Bevel.Refusal; Number = double(Text.size()); return true; }
    if (Field == "preview-faces") return Numeric(State.Bevel.Preview ? State.Bevel.Preview->Summary.Faces : 0);
    if (Field == "camera-yaw") return Numeric(State.Camera.Yaw);
    if (Field == "camera-pitch") return Numeric(State.Camera.Pitch);
    if (Field == "camera-distance") return Numeric(State.Camera.Distance);
    if (Field == "camera-target-x") return Numeric(State.Camera.Target[0]);
    if (Field == "camera-target-z") return Numeric(State.Camera.Target[2]);
    if (Field == "orthographic") return Numeric(State.Camera.OrthographicEnabled ? 1 : 0);
    if (Field == "grid") return Numeric(State.Style.GridEnabled ? 1 : 0);
    if (Field == "grid-plane") return Numeric(State.Style.GridPlane);
    if (Field == "dirty") return Numeric(State.DirtyCondition ? 1 : 0);
    if (Field == "welcome") return Numeric(State.WelcomeOpen ? 1 : 0);
    if (Field == "focused-step") { Text = State.FocusedStep; Number = double(Text.size()); return true; }
    if (Field == "status") { Text = State.Status; Number = double(Text.size()); return true; }
    if (Field == "title") { Text = Document.Title; Number = double(Text.size()); return true; }
    if (Field == "document-path") { Text = State.DocumentPath; Number = double(Text.size()); return true; }
    if (Field == "hover")
    {
        Text   = State.HoverIdentity ? NameDraftingIdentity(DisplayedDraftingScene(State), State.HoverIdentity) : std::string();
        Number = double(State.HoverIdentity);
        return true;
    }
    if (Field == "drawing")
    {
        Text   = State.Drawing.ActiveCondition ? DraftingDrawingLabel(State.Drawing.Kind) : "";
        Number = State.Drawing.ActiveCondition ? 1.0 : 0.0;
        return true;
    }
    if (Field == "drawing-points") return Numeric(double(State.Drawing.Points.size()));
    if (Field == "drawing-form") { Text = Modeller::SketchSplineFormLabel(State.Drawing.Form); Number = double(int(State.Drawing.Form)); return true; }
    if (Field == "drawing-centre") return Numeric(State.Drawing.CentreCondition ? 1 : 0);
    if (Field == "drawing-plane") return Numeric(State.Drawing.GridPlane);
    if (Field == "drawing-refusal") { Text = State.Drawing.Refusal; Number = double(Text.size()); return true; }
    if (Field == "curves" || Field == "selected-curves")
    {
        std::vector<std::string> Captions;
        auto CaptionOf = [&](const std::string& Label)
        {
            const auto Caption = Document.Captions.find(Label);
            return Caption != Document.Captions.end() ? Caption->second : Label;
        };
        if (Field == "selected-curves") for (const std::string& Label : Document.Selection.Curves) Captions.push_back(CaptionOf(Label));
        else for (const auto& [Label, Fit] : Document.Regeneration.Curves) Captions.push_back(CaptionOf(Label));
        return Join(Captions);
    }
    if (Field == "sheet") return Numeric(Document.Summary.SheetCondition ? 1 : 0);
    if (Field == "panels") return Numeric(double(Document.Regeneration.Panels.size()));
    if (Field == "zebra") return Numeric(State.ZebraEnabled ? 1 : 0);
    if (Field == "image-planes") return Numeric(double(State.Document.ImagePlanes.size()));
    if (Field == "image-planes-drawn") return Numeric(double(State.Pictures.Drawn));
    if (Field == "body-matcap") { Text = Document.Appearance.BodyMatcap; Number = double(Text.size()); return true; }
    if (Field == "face-matcaps") return Numeric(double(Document.Appearance.FaceMatcaps.size()));
    if (Field == "selection-matcap")
    {
        // The matcap every selected face (else body) shows ("mixed" when they differ), the default when nothing is selected.
        const auto FaceShows = [&](const std::string& Face)
        { return DraftingFaceMatcap(Document.Appearance, DraftingBodyOfName(Document, DraftingSelectionKind::Face, Face), Face); };
        if (!Document.Selection.Faces.empty())
        {
            Text = FaceShows(Document.Selection.Faces.front());
            for (const std::string& Face : Document.Selection.Faces) if (FaceShows(Face) != Text) Text = "mixed";
        }
        else if (!Document.Selection.Bodies.empty())
        {
            Text = DraftingBodyMatcap(Document.Appearance, Document.Selection.Bodies.front());
            for (const std::string& Key : Document.Selection.Bodies) if (DraftingBodyMatcap(Document.Appearance, Key) != Text) Text = "mixed";
        }
        else Text = Document.Appearance.BodyMatcap;
        Number = double(Text.size());
        return true;
    }
    if (Field == "matcap-layers") return Numeric(double(State.Matcaps.Library.Names.size()));
    if (Field == "matcap-thumbnails") return Numeric(double(std::count_if(State.Matcaps.Thumbnails.begin(), State.Matcaps.Thumbnails.end(), [](ImTextureID Id) { return Id != ImTextureID{}; })));
    if (Field == "matcap-picker") return Numeric(State.Matcaps.PickerOpen ? 1 : 0);
    if (Field == "matcap-dangling") return Numeric(double(CountDanglingMatcaps(State.Scene, Document.Appearance)));
    if (Field == "triangles") return Numeric(double(State.Scene.Corners.size() / 3));
    if (Field == "deviation-worst" || Field == "deviation-points")
    {
        const DraftingDeviationMap Map = MeasureDraftingDeviation(Document);
        return Numeric(Field == "deviation-worst" ? Map.Worst : double(Map.Needles.size()));
    }
    if (Field == "loft-continuity") return Numeric(State.LoftContinuity);
    if (Field == "panel-names")
    {
        std::vector<std::string> Names;
        for (const auto& [Name, Surface] : Document.Regeneration.Panels) Names.push_back(Name);
        return Join(Names);
    }
    if (Field == "pole-drags") return Numeric(State.PoleDrag.Commits);
    if (Field == "box-selections") return Numeric(State.Box.Commits);
    if (Field == "box-edges") return Numeric(State.Box.Edges);
    if (Field == "pole-drag-active") return Numeric(State.PoleDrag.ActiveCondition ? 1 : 0);
    if (Field == "pole-edits")
    {
        size_t Edits = 0;
        for (const Modeller::FeatureStep& Step : Document.Tree.Steps) Edits += Step.PoleEdits.size();
        return Numeric(double(Edits));
    }
    if (Field == "curve-fit-worst")
    {
        double Worst = 0.0;
        for (const auto& [Label, Fit] : Document.Regeneration.Curves) Worst = std::max(Worst, Fit.WorstDeviation);
        return Numeric(Worst);
    }
    if (Field == "sketches")
    {
        std::vector<std::string> Captions;
        for (const Modeller::FeatureStep& Step : Document.Tree.Steps)
        {
            if (Step.Kind != Modeller::FeatureStepKind::Sketch) continue;
            const auto Caption = Document.Captions.find(Step.Label);
            Captions.push_back(Caption != Document.Captions.end() ? Caption->second : Step.Label);
        }
        return Join(Captions);
    }
    if (Field == "sketch-curves")
    {
        const auto Solved = Document.Regeneration.Sketches.find(State.FocusedStep);
        return Numeric(Solved != Document.Regeneration.Sketches.end() ? double(Solved->second.Curves.size()) : 0.0);
    }
    if (Field == "focused-caption")
    {
        const auto Caption = Document.Captions.find(State.FocusedStep);
        Text   = Caption != Document.Captions.end() ? Caption->second : std::string();
        Number = double(Text.size());
        return true;
    }
    if (Field == "bevel-verb") return Numeric(int(State.Bevel.Verb));
    if (Field == "bevel-combination")
    {
        Text   = State.Bevel.Combination == Modeller::FeatureCombination::New ? "New" : Modeller::FeatureCombinationLabel(State.Bevel.Combination);
        Number = double(int(State.Bevel.Combination));
        return true;
    }
    if (Field.rfind("file:", 0) == 0) return Numeric(std::filesystem::exists(Field.substr(5)) ? 1 : 0);
    return false;
}

DraftingExpectation Evaluate(const DraftingScriptStep& Step, const DraftingWorkspaceState& State)
{
    DraftingExpectation Result{ Step.Line, Step.Text, {}, false };
    const std::vector<std::string> Words = SplitWords(Step.Text);
    if (Words.size() < 3)
    {
        Result.Observed = "malformed expectation";
        return Result;
    }
    std::string Expected;
    for (size_t Index = 2; Index < Words.size(); ++Index) Expected += (Index > 2 ? " " : "") + Words[Index];
    double Number = 0.0;
    if (!ObserveField(State, Words[0], Result.Observed, Number))
    {
        Result.Observed = "unknown field " + Words[0];
        return Result;
    }
    const std::string& Operator = Words[1];
    if (Operator == "contains")
    {
        Result.Passed = Result.Observed.find(Expected) != std::string::npos;
        return Result;
    }
    if (Operator == "lacks")
    {
        Result.Passed = Result.Observed.find(Expected) == std::string::npos;
        return Result;
    }
    double Target = 0.0;
    try
    {
        Target = std::stod(Expected);
    }
    catch (...)
    {
        Result.Passed = Operator == "==" ? Result.Observed == Expected : Operator == "!=" ? Result.Observed != Expected : false;
        return Result;
    }
    if (Operator == "==") Result.Passed = Number == Target;
    else if (Operator == "!=") Result.Passed = Number != Target;
    else if (Operator == "<") Result.Passed = Number < Target;
    else if (Operator == ">") Result.Passed = Number > Target;
    else if (Operator == "~") Result.Passed = std::abs(Number - Target) <= 1.0e-6 * std::max(1.0, std::abs(Target));
    return Result;
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

DraftingScript ParseDraftingScript(const std::string& Text)
{
    DraftingScript     Script;
    std::istringstream Stream(Text);
    std::string        Line;
    int                LineNumber = 0;
    auto Push = [&](DraftingScriptStep Step)
    {
        Step.Line = LineNumber;
        Script.Steps.push_back(std::move(Step));
    };
    auto Refuse = [&](const std::string& Reason)
    {
        if (Script.Refusal.empty()) Script.Refusal = "line " + std::to_string(LineNumber) + ": " + Reason;
    };
    while (std::getline(Stream, Line))
    {
        LineNumber += 1;
        // Comments are whole lines starting with '#' (names such as Extrude#2 contain the character).
        const size_t First = Line.find_first_not_of(" \t");
        if (First == std::string::npos || Line[First] == '#') continue;
        const std::vector<std::string> Words = SplitWords(Line);
        if (Words.empty()) continue;
        const std::string& Verb = Words[0];
        size_t             Index = 1;
        if (Verb == "frames")
        {
            const int Count = Words.size() > 1 ? std::max(1, std::stoi(Words[1])) : 1;
            for (int Frame = 0; Frame < Count; ++Frame) Push({});
        }
        else if (Verb == "move")
        {
            // move TARGET (remembered as the anchor) · move offset DX DY (pixels from the last anchor, e.g. mid-drag)
            DraftingScriptStep Step{ DraftingStepKind::Move };
            if (Words.size() == 4 && Words[1] == "offset")
            {
                Step.Anchored       = true;
                Step.Target.OffsetX = std::stof(Words[2]);
                Step.Target.OffsetY = std::stof(Words[3]);
                Push(Step);
                Push({});
                continue;
            }
            Step.Anchoring = true;
            if (!ReadTarget(Words, Index, Step.Target)) { Refuse("move target"); continue; }
            Push(Step);
            Push({});
        }
        else if (Verb == "click" || Verb == "press" || Verb == "release")
        {
            DraftingScriptStep Move{ DraftingStepKind::Move };
            if (Verb == "click" && !ReadTarget(Words, Index, Move.Target)) { Refuse("click target"); continue; }
            DraftingScriptStep Down{ DraftingStepKind::Button };
            Down.Button = Index < Words.size() ? ButtonByName(Words[Index]) : ImGuiMouseButton_Left;
            ReadModifiers(Words, Index, Down.Control, Down.Shift, Down.Alternate);
            Down.Down = true;
            DraftingScriptStep Up = Down;
            Up.Down = false;
            if (Verb == "click") { Push(Move); Push({}); }
            if (Verb != "release") Push(Down);
            if (Verb != "press") Push(Up);
            Push({});
        }
        else if (Verb == "drag")
        {
            // drag BUTTON FROM by DX DY STEPS [Ctrl] [Shift]
            if (Words.size() < 2) { Refuse("drag button"); continue; }
            const int Button = ButtonByName(Words[1]);
            Index = 2;
            DraftingScriptTarget From;
            if (!ReadTarget(Words, Index, From) || Index + 3 >= Words.size() || Words[Index] != "by") { Refuse("drag FROM by DX DY STEPS"); continue; }
            const float TravelX = std::stof(Words[Index + 1]);
            const float TravelY = std::stof(Words[Index + 2]);
            const int   Count   = std::max(1, std::stoi(Words[Index + 3]));
            bool Control = false, Shift = false, Alternate = false;
            ReadModifiers(Words, Index + 4, Control, Shift, Alternate);
            DraftingScriptStep Start{ DraftingStepKind::Move };
            Start.Target    = From;
            Start.Anchoring = true;
            Push(Start);
            Push({});
            DraftingScriptStep Down{ DraftingStepKind::Button };
            Down.Button  = Button;
            Down.Down    = true;
            Down.Control   = Control;
            Down.Shift     = Shift;
            Down.Alternate = Alternate;
            Push(Down);
            for (int Frame = 1; Frame <= Count; ++Frame)
            {
                DraftingScriptStep Along{ DraftingStepKind::Move };
                Along.Anchored       = true;
                Along.Target.OffsetX = float(Frame) / float(Count) * TravelX;
                Along.Target.OffsetY = float(Frame) / float(Count) * TravelY;
                Along.Control         = Control;
                Along.Shift           = Shift;
                Push(Along);
            }
            DraftingScriptStep Up = Down;
            Up.Down = false;
            Push(Up);
            Push({});
        }
        else if (Verb == "wheel")
        {
            DraftingScriptStep Move{ DraftingStepKind::Move };
            if (!ReadTarget(Words, Index, Move.Target) || Index >= Words.size()) { Refuse("wheel target"); continue; }
            Push(Move);
            Push({});
            DraftingScriptStep Wheel{ DraftingStepKind::Wheel };
            Wheel.Wheel = std::stof(Words[Index]);
            Push(Wheel);
            Push({});
        }
        else if (Verb == "key")
        {
            if (Words.size() < 2 || KeyByName(Words[1]) == ImGuiKey_None) { Refuse("unknown key"); continue; }
            DraftingScriptStep Down{ DraftingStepKind::Key };
            Down.Key  = KeyByName(Words[1]);
            Down.Down = true;
            ReadModifiers(Words, 2, Down.Control, Down.Shift, Down.Alternate);
            DraftingScriptStep Up = Down;
            Up.Down = false;
            Push(Down);
            Push(Up);
            Push({});
        }
        else if (Verb == "type")
        {
            DraftingScriptStep Typed{ DraftingStepKind::Text };
            Typed.Text = Line.substr(Line.find("type") + 4);
            Typed.Text.erase(0, Typed.Text.find_first_not_of(' '));
            Typed.Text.erase(Typed.Text.find_last_not_of(" \r") + 1);
            Push(Typed);
            Push({});
        }
        else if (Verb == "capture")
        {
            if (Words.size() < 2) { Refuse("capture name"); continue; }
            Push({});
            DraftingScriptStep Capture{ DraftingStepKind::Capture };
            Capture.Text = Words[1];
            Push(Capture);
        }
        else if (Verb == "expect")
        {
            DraftingScriptStep Expect{ DraftingStepKind::Expect };
            Expect.Text = Line.substr(Line.find("expect") + 6);
            Expect.Text.erase(0, Expect.Text.find_first_not_of(' '));
            Expect.Text.erase(Expect.Text.find_last_not_of(" \r") + 1);
            Push(Expect);
        }
        else
        {
            Refuse("unknown command " + Verb);
        }
    }
    return Script;
}

std::string AdvanceDraftingScript(DraftingScript& Script, DraftingWorkspaceState& State)
{
    ImGuiIO& Io = ImGui::GetIO();
    // Expectations run back to back without consuming frames.
    while (Script.Cursor < Script.Steps.size() && Script.Steps[Script.Cursor].Kind == DraftingStepKind::Expect)
        Script.Expectations.push_back(Evaluate(Script.Steps[Script.Cursor++], State));
    if (Script.Cursor >= Script.Steps.size()) return {};
    const DraftingScriptStep& Step = Script.Steps[Script.Cursor++];
    switch (Step.Kind)
    {
    case DraftingStepKind::Idle:
        break;
    case DraftingStepKind::Move:
    {
        ImVec2 Point;
        if (Step.Anchored)
        {
            Io.AddMousePosEvent(Script.Anchor.x + Step.Target.OffsetX, Script.Anchor.y + Step.Target.OffsetY);
        }
        else if (ResolveTarget(Step.Target, State, Point))
        {
            if (Step.Anchoring) Script.Anchor = Point;
            Io.AddMousePosEvent(Point.x, Point.y);
        }
        else Script.Expectations.push_back({ Step.Line, "target resolves", "unresolved target " + Step.Target.Widget, false });
        break;
    }
    case DraftingStepKind::Button:
        if (Step.Down)
        {
            Io.AddKeyEvent(ImGuiMod_Ctrl, Step.Control);
            Io.AddKeyEvent(ImGuiMod_Shift, Step.Shift);
            Io.AddKeyEvent(ImGuiMod_Alt, Step.Alternate);
        }
        Io.AddMouseButtonEvent(Step.Button, Step.Down);
        if (!Step.Down)
        {
            Io.AddKeyEvent(ImGuiMod_Ctrl, false);
            Io.AddKeyEvent(ImGuiMod_Shift, false);
            Io.AddKeyEvent(ImGuiMod_Alt, false);
        }
        break;
    case DraftingStepKind::Key:
        if (Step.Down)
        {
            Io.AddKeyEvent(ImGuiMod_Ctrl, Step.Control);
            Io.AddKeyEvent(ImGuiMod_Shift, Step.Shift);
            Io.AddKeyEvent(ImGuiMod_Alt, Step.Alternate);
            Io.AddKeyEvent(Step.Key, true);
        }
        else
        {
            Io.AddKeyEvent(Step.Key, false);
            Io.AddKeyEvent(ImGuiMod_Ctrl, false);
            Io.AddKeyEvent(ImGuiMod_Shift, false);
            Io.AddKeyEvent(ImGuiMod_Alt, false);
        }
        break;
    case DraftingStepKind::Text:
        Io.AddInputCharactersUTF8(Step.Text.c_str());
        break;
    case DraftingStepKind::Wheel:
        Io.AddMouseWheelEvent(0.0f, Step.Wheel);
        break;
    case DraftingStepKind::Capture:
        return Step.Text;
    case DraftingStepKind::Expect:
        break;
    }
    return {};
}

bool FinishedDraftingScript(const DraftingScript& Script)
{
    return Script.Cursor >= Script.Steps.size();
}

} // namespace Frontier::DraftingWorkspace
