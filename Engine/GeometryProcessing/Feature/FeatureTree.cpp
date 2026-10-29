/*====================================================================================================================================
                                                        FEATURETREE.CPP
====================================================================================================================================*/
// 🧩 Parametric feature tree: parameters, sketches, sweeps, Booleans, blends, face operations; regeneration by replay

#include "Engine/GeometryProcessing/Feature/FeatureTree.h"

#include "Engine/GeometryProcessing/Diagnostics/OperationTrace.h"

#include "Engine/GeometryProcessing/Feature/BodyCut.h"
#include "Engine/GeometryProcessing/Feature/PathSweep.h"
#include "Engine/GeometryProcessing/Feature/ReplayDigest.h"
#include "Engine/GeometryProcessing/Exchange/StepExchange.h"
#include "Engine/GeometryProcessing/Geometry/NurbsCarrier.h"
#include "Engine/GeometryProcessing/Local/FaceOperations.h"
#include "Engine/GeometryProcessing/Local/TopologyRebuild.h"
#include "Engine/GeometryProcessing/Sketch/SketchConstraining.h"
#include "Engine/GeometryProcessing/Sketch/SketchProfile.h"
#include "Engine/GeometryProcessing/Sketch/SketchSection.h"
#include "Engine/GeometryProcessing/Sketch/SketchSpline.h"
#include "Engine/GeometryProcessing/Surfacing/CurveNetwork.h"
#include "Engine/GeometryProcessing/Surfacing/PanelAssembly.h"
#include "Engine/GeometryProcessing/Surfacing/SectionLoft.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <set>

namespace Frontier::GeometryProcessing
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    using Clock = std::chrono::steady_clock;

    struct SolvedSketch
    {
        SketchModel        Model    = {};                                                    // [-] - Solved positions
        bool               FillCondition = true;                                             // [-] - Closed loops are filled faces
        OrthonormalFrame   Plane    = {};                                                    // [-] - Plane with elevation applied
        bool               UsableCondition = false;                                          // [-] - Converged without conflicts
    };

    [[nodiscard]] std::string ConflictText(const SketchModel& Sketch, const SketchSolveOutcome& Solve)
    {
        std::string Text;
        for (const std::vector<int>& Group : Solve.ConflictGroups)
        {
            Text += Text.empty() ? "{" : " {";
            for (size_t Index = 0; Index < Group.size(); ++Index) Text += (Index ? ", " : "") + (Sketch.Constraints[Group[Index]].Label.empty() ? std::string(SketchConstraintLabel(Sketch.Constraints[Group[Index]].Kind)) : Sketch.Constraints[Group[Index]].Label);
            Text += "}";
        }
        return Text;
    }

    // 📝 A surface panel of the tree: its face name and the SpaceCurve labels that lie on its boundary (continuity lookup).
    struct TreePanel
    {
        SurfacePanel             Panel      = {};                                            // [-] - Carrier on [0, 1]²
        std::string              Name       = {};                                            // [-] - Face name
        std::vector<std::string> Boundaries = {};                                            // [-] - Section labels carried as boundaries
        bool                     ImageCondition = false;                                     // [-] - Made by a Mirror step
    };

    struct MirrorPlane
    {
        Vector3 Point  = {};                                                                 // [len] - On the plane
        Vector3 Normal = {};                                                                 // [-]   - Unit normal
    };

    constexpr double MirrorSnapFraction = 1.0;                                               // [-] - End points within this × fit tolerance of a mirror plane snap onto it

    // Names of a transformed copy: each image face takes its source face's name (Prefix empty) or "Prefix.Copy(name)".
    [[nodiscard]] BodyNaming CarryNaming(const ModelPartition& Partition, const TransformOutcome& Transform, const BodyNaming& Source, const std::string& Prefix)
    {
        BodyNaming Naming;
        for (size_t Index = 0; Index < Transform.SourceFaces.size() && Index < Transform.Faces.size(); ++Index)
        {
            const auto Found = Source.Faces.find(Transform.SourceFaces[Index].Slot);
            if (Found == Source.Faces.end()) continue;
            Naming.Faces[Transform.Faces[Index].Slot] = Prefix.empty() ? Found->second : Prefix + ".Copy(" + Found->second + ")";
        }
        CompleteNaming(Partition, Transform.Body, Naming);
        return Naming;
    }

    // Pipe faces: plane faces are caps (Start: the one nearer the path start, else End), the rest Side(Path).
    [[nodiscard]] BodyNaming NamePipeBody(const ModelPartition& Partition, BodyToken Body, const std::string& Label, Vector3 PathStart)
    {
        const BodyInventory Inventory = Partition.CollectInventory(Body);
        std::vector<FaceToken> Caps, Sides;
        for (FaceToken Face : Inventory.Faces)
        {
            const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Partition.Faces.Resolve(Face).Surface);
            (std::holds_alternative<PlaneSurface>(Surface) ? Caps : Sides).push_back(Face);
        }
        std::sort(Caps.begin(), Caps.end(), [&](FaceToken First, FaceToken Second)
        {
            return Distance(MeasureFaceBoundaryCentre(Partition, First), PathStart) < Distance(MeasureFaceBoundaryCentre(Partition, Second), PathStart);
        });
        std::vector<FaceToken>   Faces;
        std::vector<std::string> Names;
        for (FaceToken Face : Sides) { Faces.push_back(Face); Names.push_back(Label + ".Side(Path)"); }
        for (size_t Index = 0; Index < Caps.size(); ++Index) { Faces.push_back(Caps[Index]); Names.push_back(Label + (Index == 0 ? ".Cap(Start)" : ".Cap(End)")); }
        return NamePanelBody(Partition, Body, Faces, Names);
    }

    void ApplySettingsAmount(FaceOperationSettings& Settings, double Amount)
    {
        switch (Settings.Kind)
        {
        case FaceOperationKind::Rotate:
        case FaceOperationKind::Draft: Settings.Angle  = Amount; break;
        case FaceOperationKind::Scale: Settings.Factor = Amount; break;
        case FaceOperationKind::Tweak: Settings.Vector = { 0.0, 0.0, Amount }; break;
        default:                       Settings.Distance = Amount; break;
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

double EvaluateFeatureValue(const FeatureTree& Tree, const FeatureValue& Value)
{
    if (Value.Parameter.empty()) return Value.Literal;
    auto Find = [&](const std::string& Name) -> const FeatureParameter*
    {
        for (const FeatureParameter& Parameter : Tree.Parameters) if (Parameter.Name == Name) return &Parameter;
        return nullptr;
    };
    const FeatureParameter* First = Find(Value.Parameter);
    if (!First) return std::nan("");
    double Result = Value.Factor * First->Value + Value.Offset;
    if (!Value.SecondParameter.empty())
    {
        const FeatureParameter* Second = Find(Value.SecondParameter);
        if (!Second) return std::nan("");
        Result += Value.SecondFactor * Second->Value;
    }
    return Result;
}

bool AssignFeatureParameter(FeatureTree& Tree, const std::string& Name, double Value)
{
    for (FeatureParameter& Parameter : Tree.Parameters)
    {
        if (Parameter.Name != Name) continue;
        Parameter.Value = Value;
        return true;
    }
    return false;
}

FeatureValue LiteralValue(double Value) { return { Value, {}, 1.0, 0.0, {}, 0.0 }; }
FeatureValue ParameterValue(const std::string& Name, double Factor, double Offset) { return { 0.0, Name, Factor, Offset, {}, 0.0 }; }
FeatureValue SumValue(const std::string& First, double FirstFactor, const std::string& Second, double SecondFactor, double Offset)
{
    return { 0.0, First, FirstFactor, Offset, Second, SecondFactor };
}

//------------------------------------------------------------------------------------------------------------------------
//                                              SKETCH LINKS
//------------------------------------------------------------------------------------------------------------------------

OrthonormalFrame SketchStepFrame(const FeatureTree& Tree, const FeatureStep& Step)
{
    OrthonormalFrame Frame = Step.Plane;
    Frame.Origin = Step.Plane.Origin + Step.Plane.ZAxis * EvaluateFeatureValue(Tree, Step.Elevation);
    return Frame;
}

std::vector<std::string> ProjectSketchLinks(SketchModel& Model, const OrthonormalFrame& Plane, const std::function<LinkedSketchSource(const std::string&)>& Source)
{
    std::vector<std::string> Notes;
    for (size_t Point = 0; Point < Model.Points.size(); ++Point)
    {
        const SketchLink& Link = Model.Points[Point].Link;
        if (Link.Sketch.empty()) continue;
        const std::string Name  = NameSketchPick(Model, { SketchPickKind::Point, static_cast<int>(Point) });
        const LinkedSketchSource Found = Source(Link.Sketch);
        if (!Found.Model) { Notes.push_back(Name + ": link to " + Link.Sketch + " is dangling (held where it was)"); continue; }
        const SketchModel& From = *Found.Model;
        if (Link.Index < 0 || Link.Index >= static_cast<int>(From.Points.size())) { Notes.push_back(Name + ": " + Link.Sketch + " has no point " + std::to_string(Link.Index) + " (held where it was)"); continue; }
        const Vector2 Local = From.Points[Link.Index].Position;
        const Vector3 World = Found.Plane.Origin + Found.Plane.XAxis * Local.X + Found.Plane.YAxis * Local.Y;
        Model.Points[Point].Position = { Dot(World - Plane.Origin, Plane.XAxis), Dot(World - Plane.Origin, Plane.YAxis) };
    }
    for (SketchCurve& Curve : Model.Curves)
    {
        if (Curve.Kind != SketchCurveKind::Circle || Curve.Link.Sketch.empty()) continue;
        const LinkedSketchSource Found = Source(Curve.Link.Sketch);
        if (!Found.Model) { Notes.push_back(SketchCurveName(Curve) + ": link to " + Curve.Link.Sketch + " is dangling (held where it was)"); continue; }
        const SketchModel& From = *Found.Model;
        if (Curve.Link.Index < 0 || Curve.Link.Index >= static_cast<int>(From.Curves.size()) || From.Curves[Curve.Link.Index].Kind == SketchCurveKind::Line
            || From.Curves[Curve.Link.Index].Kind == SketchCurveKind::Spline)
        {
            Notes.push_back(SketchCurveName(Curve) + ": " + Curve.Link.Sketch + " has no circle " + std::to_string(Curve.Link.Index) + " (held where it was)");
            continue;
        }
        if (std::fabs(Dot(Found.Plane.ZAxis, Plane.ZAxis)) < 1.0 - 1.0e-12) { Notes.push_back(SketchCurveName(Curve) + ": " + Curve.Link.Sketch + " is not parallel (radius held)"); continue; }
        Curve.Radius = MeasureSketchRadius(From, From.Curves[Curve.Link.Index]);
    }
    return Notes;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    REPLAY CHECKPOINT
//------------------------------------------------------------------------------------------------------------------------

// 📝 Everything the step loop carries from one step to the next (the working copy Current / Naming / Target is reloaded
//    at the top of every step, so it is not saved).
struct ReplayCheckpoint
{
    ModelPartition                                                  Partition = {};          // [-] - Records after the step
    RegenerationOutcome                                             Outcome   = {};          // [-] - Steps / Sketches / Curves / Refused so far
    std::map<std::string, SolvedSketch>                             Sketches  = {};          // [-] - Solved sketches so far
    std::vector<RegeneratedBody>                                    Bodies    = {};          // [-] - Bodies so far
    int                                                             Active    = -1;          // [-] - Active body index
    int                                                             PanelBody = -1;          // [-] - Panel-built body index
    std::vector<TreePanel>                                          Panels    = {};          // [-] - Open panels
    std::string                                                     ClosedBy  = {};          // [-] - Closing step label
    std::map<std::pair<std::string, std::string>, RegeneratedBody>  Snapshots = {};          // [-] - Duplicate snapshots so far
    std::set<std::pair<std::string, std::string>>                   Served    = {};          // [-] - Snapshot requests met so far (taken or absent)
    std::map<std::string, std::string>                              Jointed   = {};          // [-] - Placed body key → the body it is jointed to
};

RegenerationOutcome RegenerateFeatureTree(ModelPartition& Partition, FeatureTree& Tree, bool CaptureCondition, RegenerationCache* Cache)
{
    FRONTIER_TRACE_SCOPE("Replay", "RegenerateFeatureTree · " + std::to_string(Tree.Steps.size()) + " step(s)" + (CaptureCondition ? " · capture" : ""));
    const auto Started = Clock::now();
    RegenerationOutcome Outcome;
    std::map<std::string, SolvedSketch> Sketches;
    std::vector<RegeneratedBody> Bodies;                                                     // Every body so far, in creation order
    int         Active    = -1;                                                              // Index of the active body (−1: none yet)
    int         PanelBody = -1;                                                              // Index of the panel-built body (−1: none)
    int         Target    = -1;                                                              // Index Current / Naming were loaded from
    BodyToken   Current;                                                                     // Working copy of the step's body
    BodyNaming  Naming;                                                                      // Its names
    std::string StepLabel;                                                                   // Label of the step being replayed
    std::vector<TreePanel> Panels;                                                           // Panels of the panel-built body
    std::string            ClosedBy;                                                         // Close / Combine step that ended them
    std::map<std::string, std::string> Jointed;                                              // Placed body key → the body it is jointed to

    auto FindBody = [&](const std::string& Key) -> int
    {
        for (size_t Index = 0; Index < Bodies.size(); ++Index) if (Bodies[Index].Key == Key) return static_cast<int>(Index);
        return -1;
    };
    // Loads the body a step acts on (empty key → the active body) into Current / Naming; false when there is none.
    auto Load = [&](const std::string& Key) -> bool
    {
        Target = Key.empty() ? Active : FindBody(Key);
        if (Target < 0) return false;
        Current = Bodies[Target].Body;
        Naming  = Bodies[Target].Naming;
        return true;
    };
    // Writes the working copy back; the changed body becomes the active one.
    auto Store = [&]()
    {
        Bodies[Target].Body   = Current;
        Bodies[Target].Naming = Naming;
        Active = Target;
    };
    // The working copy becomes a new body keyed by the current step label, and the active one.
    auto Create = [&]()
    {
        Bodies.push_back({ StepLabel, Current, Naming });
        Target = Active = static_cast<int>(Bodies.size()) - 1;
    };
    // Bodies jointed (directly or through a chain) to Key follow it when it moves by Map; their joint origins move too.
    auto CarryDependents = [&](const std::string& Key, const AffineMap& Map) -> std::string
    {
        std::set<std::string> Carried = { Key };
        for (bool Grown = true; Grown;)
        {
            Grown = false;
            for (const auto& [Placed, Base] : Jointed)
            {
                if (Carried.count(Base) == 0 || Carried.count(Placed) != 0) continue;
                Carried.insert(Placed);
                Grown = true;
            }
        }
        for (const std::string& Dependent : Carried)
        {
            const int Index = Dependent == Key ? -1 : FindBody(Dependent);
            if (Index < 0) continue;
            const TransformOutcome Follow = TransformBody(Partition, Bodies[Index].Body, Map);
            if (!Follow.Refusal.empty()) return Follow.Refusal;
            Bodies[Index].Naming = CarryNaming(Partition, Follow, Bodies[Index].Naming, {});
            Bodies[Index].Body   = Follow.Body;
        }
        for (RegeneratedJoint& Joint : Outcome.Joints)
        {
            if (Joint.Moving == Key || Carried.count(Joint.Moving) == 0) continue;
            Joint.MovingOrigin = MapFrame(Map, Joint.MovingOrigin);
            Joint.FixedOrigin  = MapFrame(Map, Joint.FixedOrigin);
        }
        return {};
    };
    // A removed or consumed body leaves every joint it took part in.
    auto ForgetJoints = [&](const std::string& Key)
    {
        for (auto Link = Jointed.begin(); Link != Jointed.end();) Link = Link->first == Key || Link->second == Key ? Jointed.erase(Link) : std::next(Link);
        std::erase_if(Outcome.Joints, [&](const RegeneratedJoint& Joint) { return Joint.Moving == Key || Joint.Fixed == Key; });
    };

    // Duplicate snapshots ahead of time: the bodies some later Duplicate copies as they stood after a given step.
    std::map<std::string, std::vector<std::string>> SnapshotRequests;                        // Step label → body keys
    std::map<std::pair<std::string, std::string>, RegeneratedBody> Snapshots;                // (step, key) → body then
    std::set<std::pair<std::string, std::string>>                  Served;                   // Requests met (a missing body takes none)
    for (const FeatureStep& Step : Tree.Steps)
    {
        if (Step.Kind == FeatureStepKind::Duplicate && !Step.Source.empty()) SnapshotRequests[Step.Source].push_back(Step.Target);
    }

    // Mirror planes ahead of time: SpaceCurves ending on one leave it perpendicularly (the halves then meet G1).
    std::vector<MirrorPlane> Planes;
    for (const FeatureStep& Step : Tree.Steps)
    {
        if (Step.Kind == FeatureStepKind::Mirror && Length(Step.Plane.ZAxis) > 0.0) Planes.push_back({ Step.Plane.Origin, Normalize(Step.Plane.ZAxis) });
    }

    // Sews Panels (with plane caps when Capped) into a new current body named per panel; refusal text or empty.
    auto Reassemble = [&](const std::string& Label, bool Capped) -> std::string
    {
        std::vector<SurfacePanel> Surfaces;
        for (const TreePanel& Panel : Panels) Surfaces.push_back(Panel.Panel);
        PanelAssemblySettings Settings;
        Settings.CapCondition = Capped;
        const PanelAssemblyOutcome Assembly = AssemblePanels(Partition, Surfaces, Settings, Capped ? "Close" : "Sew");
        if (!Assembly.Refusal.empty()) return Assembly.Refusal;
        if (Capped && !Assembly.SolidCondition) return "Close: the panels do not close (" + std::to_string(Assembly.LaminarEdges) + " rim edges remain open)";
        std::vector<FaceToken>   Faces = Assembly.Faces;
        std::vector<std::string> Names;
        for (const TreePanel& Panel : Panels) Names.push_back(Panel.Name);
        for (size_t Cap = 0; Cap < Assembly.Caps.size(); ++Cap)
        {
            Faces.push_back(Assembly.Caps[Cap]);
            Names.push_back(Label + ".Cap(" + std::to_string(Cap) + ")");
        }
        Naming  = NamePanelBody(Partition, Assembly.Body, Faces, Names);
        Current = Assembly.Body;
        if (PanelBody < 0)
        {
            Create();
            PanelBody = Target;
        }
        else
        {
            Target = PanelBody;
            Store();
        }
        return {};
    };

    // A new panel joins the panel-built body (its own body beside any sketch-built ones) while that is still open.
    auto PanelJoinRefusal = [&](const char* Verb) -> std::string
    {
        if (!ClosedBy.empty()) return std::string(Verb) + ": the panels are already closed by " + ClosedBy;
        return {};
    };

    // Incremental replay: restore the longest unchanged prefix (same digests, captured when capturing).
    size_t Resume = 0;
    if (Cache && Cache->EnabledCondition)
    {
        const size_t Limit = std::min({ Tree.Steps.size(), Cache->Digests.size(), Cache->Checkpoints.size(), Cache->Captured.size() });
        while (Resume < Limit && Cache->Checkpoints[Resume] && Cache->Digests[Resume] == DigestFeatureStep(Tree, Tree.Steps[Resume])) ++Resume;
        // A Duplicate snapshot due after a restored step (a newly added Duplicate) must be in the restored state; else
        // resume at that step. Snapshots no longer requested are harmless (nothing reads them).
        for (bool Lowered = true; Lowered && Resume > 0;)
        {
            Lowered = false;
            const ReplayCheckpoint& Saved = *Cache->Checkpoints[Resume - 1];
            for (size_t Index = 0; Index < Resume && !Lowered; ++Index)
            {
                const auto Requested = SnapshotRequests.find(Tree.Steps[Index].Label);
                if (Requested == SnapshotRequests.end()) continue;
                for (const std::string& Key : Requested->second)
                {
                    if (Saved.Served.count({ Requested->first, Key }) != 0) continue;
                    Resume  = Index;
                    Lowered = true;
                    break;
                }
            }
        }
    }
    // Capture on a restored step last replayed without it: a signature depends only on the partition before the step and
    // the resolved slot (the step's own result never reads it), so it is written from the previous checkpoint and the
    // step's resolutions — exactly what a capturing replay would write — without replaying the step.
    if (CaptureCondition)
    {
        const ModelPartition Empty;
        for (size_t Index = 0; Index < Resume; ++Index)
        {
            if (Cache->Captured[Index]) continue;
            FeatureStep& Step = Tree.Steps[Index];
            const ModelPartition& Before = Index == 0 ? Empty : Cache->Checkpoints[Index - 1]->Partition;
            const std::vector<ResolvedReference>& Resolutions = Cache->Checkpoints[Index]->Outcome.Steps[Index].Resolutions;
            const size_t Edges   = Step.References.size();
            const size_t Anchors = Step.Kind == FeatureStepKind::Blend && (Step.EndSize || !Step.AnchorSizes.empty()) ? Step.SizeAnchors.size() : 0;
            if (Resolutions.size() == Edges + Anchors)                                       // Fewer: refused before resolving, nothing captured
            {
                for (size_t Reference = 0; Reference < Edges + Anchors; ++Reference)
                {
                    const ResolvedReference& Resolved = Resolutions[Reference];
                    if (Resolved.Method == ResolutionMethod::Refused) continue;
                    TopologyReference& Target = Reference < Edges ? Step.References[Reference] : Step.SizeAnchors[Reference - Edges];
                    Target.Signature = CaptureSignature(Before, Target.Kind, Resolved.Slot);
                }
            }
            Cache->Digests[Index]  = DigestFeatureStep(Tree, Step);
            Cache->Captured[Index] = true;
        }
    }
    if (Resume > 0)
    {
        const ReplayCheckpoint& Saved = *Cache->Checkpoints[Resume - 1];
        Partition = Saved.Partition;
        Outcome   = Saved.Outcome;
        Sketches  = Saved.Sketches;
        Bodies    = Saved.Bodies;
        Active    = Saved.Active;
        PanelBody = Saved.PanelBody;
        Panels    = Saved.Panels;
        ClosedBy  = Saved.ClosedBy;
        Snapshots = Saved.Snapshots;
        Served    = Saved.Served;
        Jointed   = Saved.Jointed;
    }
    Outcome.Resumed = static_cast<int>(Resume);
    if (Cache)
    {
        Cache->Digests.resize(Tree.Steps.size());
        Cache->Checkpoints.resize(Tree.Steps.size());
        Cache->Captured.resize(Tree.Steps.size());
    }

    FRONTIER_TRACE("Replay", Resume > 0 ? "resumed at step " + std::to_string(Resume + 1) + " (earlier steps from the checkpoint)" : std::string("full replay"));
    for (size_t StepIndex = Resume; StepIndex < Tree.Steps.size(); ++StepIndex)
    {
        FeatureStep& Step = Tree.Steps[StepIndex];
        const auto StepStarted = Clock::now();
        FeatureStepReport Report;
        Report.Label = Step.Label;
        Report.Kind  = Step.Kind;
        auto Refuse = [&](const std::string& Reason) { Report.Status = FeatureStepStatus::Refused; Report.Refusal = Reason; };
        const char* Verb = FeatureStepKindLabel(Step.Kind);
        FRONTIER_TRACE_SCOPE("Replay", std::string(Verb) + " " + Step.Label);
        StepLabel = Step.Label;
        // Steps that touch no body report the active one.
        Target  = Active;
        Current = Active >= 0 ? Bodies[Active].Body : BodyToken{};
        Naming  = Active >= 0 ? Bodies[Active].Naming : BodyNaming{};

        switch (Step.Kind)
        {
        case FeatureStepKind::Sketch:
        {
            // ① Bind parameters, solve from the authored positions (deterministic), keep the solved copy.
            SolvedSketch Solved;
            Solved.Model = Step.Sketch;
            bool BoundCondition = true;
            for (SketchConstraint& Constraint : Solved.Model.Constraints)
            {
                if (Constraint.Parameter.empty()) continue;
                const double Value = EvaluateFeatureValue(Tree, ParameterValue(Constraint.Parameter));
                if (std::isnan(Value)) { BoundCondition = false; Refuse("Sketch: unknown parameter " + Constraint.Parameter); break; }
                Constraint.Value = Value;
            }
            // ② Links read the sketches solved so far (only earlier steps: a later or own label is dangling).
            Solved.Plane = SketchStepFrame(Tree, Step);
            for (std::string& Note : ProjectSketchLinks(Solved.Model, Solved.Plane, [&](const std::string& Label)
            {
                const auto Found = Sketches.find(Label);
                return Found == Sketches.end() || Label == Step.Label ? LinkedSketchSource{} : LinkedSketchSource{ &Found->second.Model, Found->second.Plane };
            })) Report.Notes.push_back(std::move(Note));
            if (BoundCondition)
            {
                Report.Solve = SolveSketch(Solved.Model);
                if (!Report.Solve.ConflictGroups.empty()) Refuse("Sketch: conflicting constraints " + ConflictText(Solved.Model, Report.Solve));
                else if (!Report.Solve.Converged) Refuse("Sketch: no solution reached (max residual " + std::to_string(Report.Solve.MaxResidual) + ")");
                else Solved.UsableCondition = true;
            }
            Solved.FillCondition = Step.FillCondition;
            Outcome.Sketches[Step.Label] = Solved.Model;
            Sketches[Step.Label] = Solved;
            break;
        }
        case FeatureStepKind::Extrude:
        case FeatureStepKind::Revolve:
        case FeatureStepKind::Evolution:
        {
            // ② Sweep the profile into a tool body named by role, then combine.
            const auto Found = Sketches.find(Step.Profile);
            if (Found == Sketches.end() || !Found->second.UsableCondition) { Refuse(std::string(Verb) + ": profile sketch " + Step.Profile + " is unavailable"); break; }
            const SolvedSketch& Solved  = Found->second;
            const SketchProfile Profile = ExtractSketchProfile(Solved.Model);
            SweepOutcome Sweep;
            // An unfilled profile (Plasticity's fresh curves) extrudes into walls: an open sheet, a body of its own.
            const bool WallsCondition = Step.Kind == FeatureStepKind::Extrude && !Solved.FillCondition;
            if (WallsCondition && (Step.Combination == FeatureCombination::Difference || Step.Combination == FeatureCombination::Intersection))
            {
                Refuse("Extrude: " + Step.Profile + " is unfilled, so it makes walls (a sheet) — fill it (F) to " + FeatureCombinationLabel(Step.Combination));
                break;
            }
            if (Step.Kind == FeatureStepKind::Extrude)
            {
                // A start offset slides the sketch plane along its normal; the far end stays at Extent.
                const double     Start  = EvaluateFeatureValue(Tree, Step.Start);
                const double     Extent = EvaluateFeatureValue(Tree, Step.Extent);
                OrthonormalFrame Plane  = Solved.Plane;
                Plane.Origin            = Plane.Origin + Plane.ZAxis * Start;
                if (Start != 0.0 && !((Extent - Start) * (Extent >= 0.0 ? 1.0 : -1.0) > 1.0e-9)) { Refuse("Extrude: the start " + std::to_string(Start) + " passes the end " + std::to_string(Extent)); break; }
                // A cut whose side stops a sliver short of a convex rim of the face it starts on opens through it.
                SketchProfile Outline = Profile;
                if (Step.Combination == FeatureCombination::Difference && !WallsCondition && !Bodies.empty())
                {
                    const int Host = Step.Target.empty() ? Active : FindBody(Step.Target);
                    if (Host >= 0 && Extent != 0.0) (void)OpenSliverBridges(Partition, Bodies[Host].Body, Solved.Plane, Extent, nullptr, Outline);
                }
                Sweep = ExtrudeProfile(Partition, Outline, Plane, Extent - Start, !WallsCondition);
            }
            else if (Step.Kind == FeatureStepKind::Evolution)
            {
                // ⑯ Path: a SpaceCurve, or another sketch holding exactly one open curve (line, arc or spline).
                std::optional<NurbsCurve> Path;
                std::string Problem;
                if (const auto Curve = Outcome.Curves.find(Step.Path); Curve != Outcome.Curves.end()) Path = Curve->second.Curve;
                else if (Step.Path == Step.Profile) Problem = "Evolution: the path must be another sketch than the profile";
                else if (const auto Source = Sketches.find(Step.Path); Source == Sketches.end() || !Source->second.UsableCondition) Problem = "Evolution: path " + (Step.Path.empty() ? std::string("(none)") : Step.Path) + " is unavailable";
                else
                {
                    const SolvedSketch& Carrier = Source->second;
                    std::vector<int> Drawn;
                    for (size_t Index = 0; Index < Carrier.Model.Curves.size(); ++Index) if (!Carrier.Model.Curves[Index].Construction) Drawn.push_back(static_cast<int>(Index));
                    if (Drawn.size() != 1) Problem = "Evolution: path sketch " + Step.Path + " has " + std::to_string(Drawn.size()) + " curves (🚧 one line, arc or spline; J joins a chain into a spline first)";
                    else
                    {
                        const ProfileSegment Segment = ResolveSketchCurveSegment(Carrier.Model, Drawn.front(), 1.0e-12, Problem);
                        if (!Problem.empty()) Problem = "Evolution: " + Problem;
                        else if (Segment.Kind == ProfileSegmentKind::Circle) Problem = "Evolution: the path " + Segment.Name + " is closed (🚧 closed sweeps)";
                        else
                        {
                            const OrthonormalFrame& Frame = Carrier.Plane;
                            const auto Lift = [&](Vector2 Point) { return Frame.Origin + Frame.XAxis * Point.X + Frame.YAxis * Point.Y; };
                            if (Segment.Kind == ProfileSegmentKind::Line) Path = ConvertLineToNurbs(StraightLine{ Lift(Segment.Start), Normalize(Lift(Segment.End) - Lift(Segment.Start)) }, 0.0, Distance(Segment.Start, Segment.End));
                            else if (Segment.Kind == ProfileSegmentKind::Arc)
                            {
                                const double Sign = Segment.Sweep > 0.0 ? 1.0 : -1.0;
                                const ConicForm Conic{ Lift(Segment.Centre), Frame.XAxis * Segment.Radius, Frame.YAxis * (Sign * Segment.Radius), Frame.ZAxis * Sign };
                                Path = ConvertConicToNurbs(Conic, Sign * Segment.StartAngle, Sign * Segment.StartAngle + std::fabs(Segment.Sweep));
                            }
                            else
                            {
                                std::vector<Vector3> Poles;
                                for (const Vector2& Pole : Segment.Spline.Poles) Poles.push_back(Lift(Pole));
                                Path = ConstructNurbsCurve(Segment.Spline.Degree, Segment.Spline.Knots, Poles, std::vector<double>(Poles.size(), 1.0));
                                if (!Path) Problem = "Evolution: the spline path has invalid knots";
                            }
                        }
                    }
                }
                if (!Problem.empty()) { Refuse(Problem); break; }
                // The sweep runs from the path end nearer the profile centroid: a sketch arc is stored counter-clockwise
                // and a curve's drawing order is arbitrary, so neither may decide where the profile starts.
                if (Path->Definition && !Path->Definition->Poles.empty())
                {
                    Vector3 Anchor{};
                    int     Count = 0;
                    for (const ProfileLoop& Loop : Profile.Loops)
                        for (const ProfileSegment& Segment : Loop.Segments)
                        {
                            const Vector2 Point = EvaluateProfileSegment(Segment, 0.0);
                            Anchor = Anchor + Solved.Plane.Origin + Solved.Plane.XAxis * Point.X + Solved.Plane.YAxis * Point.Y;
                            ++Count;
                        }
                    if (Count > 0) Anchor = Anchor * (1.0 / Count);
                    const std::vector<Vector3>& Poles = Path->Definition->Poles;
                    if (Count > 0 && Distance(Anchor, Poles.back()) < Distance(Anchor, Poles.front()) - 1.0e-12 * std::max(1.0, Profile.Scale)) Path = ReverseNurbsCurve(*Path->Definition);
                }
                const EvolutionOutcome Evolved = EvolveProfile(Partition, Profile, Solved.Plane, *Path);
                if (!Evolved.Refusal.empty()) { Refuse(Evolved.Refusal); break; }
                Sweep = Evolved.Sweep;
            }
            else
            {
                Vector2 Start = Step.AxisOrigin, Direction = Step.AxisDirection;
                if (Step.AxisIdentifier >= 0)
                {
                    const SketchCurve* Axis = nullptr;
                    for (const SketchCurve& Curve : Solved.Model.Curves) if (Curve.Kind == SketchCurveKind::Line && Curve.Identifier == Step.AxisIdentifier) Axis = &Curve;
                    if (!Axis) { Refuse("Revolve: axis line " + std::to_string(Step.AxisIdentifier) + " not in sketch " + Step.Profile); break; }
                    // The axis may close the profile loop itself (its segments sweep nothing) or be a construction line.
                    Start     = Solved.Model.Points[Axis->Start].Position;
                    Direction = Solved.Model.Points[Axis->End].Position - Start;
                }
                if (!(Length(Direction) > 1.0e-12 * Profile.Scale)) { Refuse("Revolve: the axis has no direction"); break; }
                Sweep = RevolveProfile(Partition, Profile, Solved.Plane, Start, Direction, EvaluateFeatureValue(Tree, Step.Angle));
            }
            if (!Sweep.Refusal.empty()) { Refuse(Sweep.Refusal); break; }
            const BodyNaming ToolNaming = NameSweptBody(Partition, Sweep, Step.Label);
            // A separate sweep (Plasticity "new body"), or the first body of the part, starts a body of its own.
            const bool SeparateCondition = Step.Combination == FeatureCombination::Separate || WallsCondition;
            if (SeparateCondition || Bodies.empty())
            {
                if (!SeparateCondition && Step.Combination != FeatureCombination::New && Step.Combination != FeatureCombination::Union) { Refuse(std::string(Verb) + ": nothing to " + FeatureCombinationLabel(Step.Combination) + " with"); break; }
                Current = Sweep.Body;
                Naming  = ToolNaming;
                Create();
                break;
            }
            if (!Load(Step.Target)) { Refuse(std::string(Verb) + ": no body named " + Step.Target); break; }
            const BooleanOperation Operation = Step.Combination == FeatureCombination::Difference ? BooleanOperation::Difference
                                             : Step.Combination == FeatureCombination::Intersection ? BooleanOperation::Intersection : BooleanOperation::Union;
            const BooleanOutcome Boolean = ComputeBoolean(Partition, Current, Sweep.Body, Operation);
            if (!Boolean.Refusal.empty()) { Refuse(Boolean.Refusal); break; }
            Naming  = NameBooleanBody(Partition, Boolean.Body, Boolean, Naming, ToolNaming);
            Current = Boolean.Body;
            Store();
            break;
        }
        case FeatureStepKind::Blend:
        case FeatureStepKind::FaceOperation:
        {
            // ③ Resolve references on the current body; any refusal refuses the step (never a partial selection).
            if (Bodies.empty()) { Refuse(std::string(Verb) + ": no body yet"); break; }
            if (!Load(Step.Target)) { Refuse(std::string(Verb) + ": no body named " + Step.Target); break; }
            if (Partition.Bodies.Resolve(Current).Category == BodyCategory::Sheet) { Refuse(std::string(Verb) + ": the body is a sheet (🚧 blends / face operations on sheets)"); break; }
            std::vector<uint32_t>      Slots;
            std::vector<ReferenceKind> Kinds;                                                // [-] - Per slot (Reshape takes vertices and edges)
            for (TopologyReference& Reference : Step.References)
            {
                const ResolvedReference Resolved = ResolveReference(Partition, Current, Naming, Reference);
                Report.Resolutions.push_back(Resolved);
                if (Resolved.Method == ResolutionMethod::Refused) continue;
                Slots.push_back(Resolved.Slot);
                Kinds.push_back(Reference.Kind);
                if (CaptureCondition) Reference.Signature = CaptureSignature(Partition, Reference.Kind, Resolved.Slot);
            }
            // ③′ A variable blend's anchors (the vertices taking Size) resolve like the edges, so a regeneration that
            //    reverses an edge keeps each size at its vertex.
            std::vector<VertexToken> Anchors;
            if (Step.Kind == FeatureStepKind::Blend && (Step.EndSize || !Step.AnchorSizes.empty()))
            {
                for (TopologyReference& Reference : Step.SizeAnchors)
                {
                    const ResolvedReference Resolved = ResolveReference(Partition, Current, Naming, Reference);
                    Report.Resolutions.push_back(Resolved);
                    if (Resolved.Method == ResolutionMethod::Refused) continue;
                    Anchors.push_back(Partition.Vertices.TokenAt(Resolved.Slot));
                    if (CaptureCondition) Reference.Signature = CaptureSignature(Partition, Reference.Kind, Resolved.Slot);
                }
            }
            for (const ResolvedReference& Resolved : Report.Resolutions)
            {
                if (Resolved.Method != ResolutionMethod::Refused) continue;
                std::string Reason = std::string(Verb) + ": " + Resolved.Refusal;
                if (!Resolved.Candidates.empty())
                {
                    Reason += " (";
                    for (size_t Index = 0; Index < Resolved.Candidates.size(); ++Index) Reason += (Index ? ", " : "") + Resolved.Candidates[Index];
                    Reason += ")";
                }
                Refuse(Reason);
                break;
            }
            if (Report.Status == FeatureStepStatus::Refused) break;
            const double Amount = EvaluateFeatureValue(Tree, Step.Kind == FeatureStepKind::Blend ? Step.Size : Step.Amount);
            if (Step.Kind == FeatureStepKind::Blend)
            {
                std::vector<EdgeToken> Edges;
                for (uint32_t Slot : Slots) Edges.push_back(Partition.Edges.TokenAt(Slot));
                BlendSettings Settings;
                Settings.Profile = Step.Shape;
                Settings.Size    = Amount;
                if (!Step.AnchorSizes.empty())
                {
                    if (Step.AnchorSizes.size() != Anchors.size()) { Refuse("Blend: per-vertex sizes do not match their vertices"); break; }
                    Settings.SizeAnchors = Anchors;
                    for (const FeatureValue& Size : Step.AnchorSizes) Settings.AnchorSizes.push_back(EvaluateFeatureValue(Tree, Size));
                }
                else if (Step.EndSize)
                {
                    Settings.EndSize     = EvaluateFeatureValue(Tree, *Step.EndSize);
                    Settings.SizeAnchors = Anchors;
                }
                const BlendOutcome Blend = BlendEdges(Partition, Current, Edges, Settings);
                if (!Blend.Refusal.empty()) { Refuse(Blend.Refusal); break; }
                Naming  = NameBlendedBody(Partition, Blend.Body, Blend, Naming, Step.Label);
                Current = Blend.Body;
                Store();
                break;
            }
            // ③″ Extrude manifold (Blender's push-in): a negative face extrusion goes into the body. A face whose
            //    neighbours can follow it is pushed; an imprinted region (every neighbour coplanar) — or a face the push
            //    refuses — becomes a prism of its own outline, started a hair outside the face, cut out of the body.
            if (Step.Operation == FaceOperationKind::Extrude && Amount < 0.0)
            {
                std::vector<FaceToken>        Faces;
                std::vector<SketchProfile>    Outlines;
                std::vector<OrthonormalFrame> Planes;
                std::string                   Problem;
                bool                          IslandCondition = true;
                for (uint32_t Slot : Slots)
                {
                    Faces.push_back(Partition.Faces.TokenAt(Slot));
                    SketchProfile    Outline;
                    OrthonormalFrame Plane;
                    bool             Island = false;
                    Problem = DescribePlanarFaceProfile(Partition, Current, Faces.back(), Outline, Plane, Island);
                    if (!Problem.empty()) break;
                    IslandCondition = IslandCondition && Island;
                    Outlines.push_back(std::move(Outline));
                    Planes.push_back(Plane);
                }
                if (!IslandCondition || !Problem.empty())
                {
                    FaceOperationSettings Push;
                    Push.Kind     = FaceOperationKind::Push;
                    Push.Faces    = Faces;
                    Push.Distance = Amount;
                    const FaceOperationOutcome Change = ApplyFaceOperation(Partition, Current, Push);
                    if (Change.Refusal.empty())
                    {
                        Naming  = NameChangedBody(Partition, Change.Body, Change, Naming, Step.Label);
                        Current = Change.Body;
                        Store();
                        break;
                    }
                }
                if (!Problem.empty()) { Refuse("Extrude: " + Problem); break; }
                std::string Refusal;
                for (size_t Index = 0; Index < Outlines.size() && Refusal.empty(); ++Index)
                {
                    const double     Lead  = 1.0e-3 * Outlines[Index].Scale;                 // [len] - Cutter start outside the face
                    // A side a sliver short of a convex rim of the face opens through it (no strip left over the cut).
                    (void)OpenSliverBridges(Partition, Current, Planes[Index], -1.0, &Faces[Index], Outlines[Index]);
                    OrthonormalFrame Plane = Planes[Index];
                    Plane.Origin           = Plane.Origin + Plane.ZAxis * Lead;
                    const SweepOutcome Sweep = ExtrudeProfile(Partition, Outlines[Index], Plane, Amount - Lead, true);
                    if (!Sweep.Refusal.empty()) { Refusal = Sweep.Refusal; break; }
                    const BodyNaming     ToolNaming = NameSweptBody(Partition, Sweep, Step.Label);
                    const BooleanOutcome Boolean    = ComputeBoolean(Partition, Current, Sweep.Body, BooleanOperation::Difference);
                    if (!Boolean.Refusal.empty()) { Refusal = Boolean.Refusal; break; }
                    Naming  = NameBooleanBody(Partition, Boolean.Body, Boolean, Naming, ToolNaming);
                    Current = Boolean.Body;
                }
                if (!Refusal.empty()) { Refuse("Extrude (push in): " + Refusal); break; }
                Store();
                break;
            }
            // ③‴ Reshape (direct vertex / edge editing): picked vertices, and both ends of picked edges, carried by Placement.
            if (Step.Operation == FaceOperationKind::Reshape)
            {
                FaceOperationSettings Reshape;
                Reshape.Kind      = FaceOperationKind::Reshape;
                Reshape.Placement = Step.Placement;
                const auto Carry  = [&](VertexToken Vertex)
                {
                    if (std::find(Reshape.Vertices.begin(), Reshape.Vertices.end(), Vertex) == Reshape.Vertices.end()) Reshape.Vertices.push_back(Vertex);
                };
                for (size_t Index = 0; Index < Slots.size(); ++Index)
                {
                    if (Kinds[Index] == ReferenceKind::Vertex) Carry(Partition.Vertices.TokenAt(Slots[Index]));
                    else if (Kinds[Index] == ReferenceKind::Edge)
                    {
                        const EdgeProfile& Edge = Partition.Edges.Resolve(Partition.Edges.TokenAt(Slots[Index]));
                        Carry(Edge.Start);
                        Carry(Edge.End);
                    }
                }
                const FaceOperationOutcome Change = ApplyFaceOperation(Partition, Current, Reshape);
                if (!Change.Refusal.empty()) { Refuse(Change.Refusal); break; }
                Naming  = NameChangedBody(Partition, Change.Body, Change, Naming, Step.Label);
                Current = Change.Body;
                Store();
                break;
            }
            FaceOperationSettings Settings;
            Settings.Kind = Step.Operation;
            for (size_t Index = 0; Index < Slots.size(); ++Index)
            {
                // ③⁗ Heal on picked edges / vertices (Delete on edges, vertices): only those dissolve.
                if (Kinds[Index] == ReferenceKind::Edge && Step.Operation == FaceOperationKind::Heal) Settings.Edges.push_back(Partition.Edges.TokenAt(Slots[Index]));
                else if (Kinds[Index] == ReferenceKind::Vertex && Step.Operation == FaceOperationKind::Heal) Settings.Vertices.push_back(Partition.Vertices.TokenAt(Slots[Index]));
                else Settings.Faces.push_back(Partition.Faces.TokenAt(Slots[Index]));
            }
            ApplySettingsAmount(Settings, Amount);
            // 📝 Gizmo tweaks carry their direction (Tweak: Vector = d̂·Amount) or axis (Rotate) in Points[0]; without it
            //    Tweak runs along +z and Rotate about +x as before.
            if (!Step.Points.empty() && Length(Step.Points.front()) > 1.0e-12)
            {
                if (Step.Operation == FaceOperationKind::Tweak)  Settings.Vector = Normalize(Step.Points.front()) * Amount;
                if (Step.Operation == FaceOperationKind::Rotate) Settings.Axis   = Normalize(Step.Points.front());
            }
            if (Step.Operation == FaceOperationKind::Inset) Settings.Depth = EvaluateFeatureValue(Tree, Step.Extent);
            const FaceOperationOutcome Change = ApplyFaceOperation(Partition, Current, Settings);
            if (!Change.Refusal.empty()) { Refuse(Change.Refusal); break; }
            Naming  = NameChangedBody(Partition, Change.Body, Change, Naming, Step.Label);
            Current = Change.Body;
            Store();
            break;
        }
        case FeatureStepKind::SpaceCurve:
        {
            // ④ Re-fit the imported points at the step tolerance, then re-apply the dragged poles (no body change).
            std::vector<Vector3> Points = Step.Points;
            if (Step.ClosedCondition && Points.size() > 2 && Length(Points.back() - Points.front()) > 0.0) Points.push_back(Points.front());
            CurveFitSettings Settings;
            Settings.Tolerance = EvaluateFeatureValue(Tree, Step.Tolerance);
            Settings.PlanarCondition = Step.PlanarCondition;
            Settings.SpanLimit = std::max(Settings.SpanLimit, static_cast<int>(Points.size()) / 4);   // Dense exact samples (Wrap): one span per 4 points may be needed
            // An end on a later Mirror plane (within the fit tolerance) snaps onto it and leaves it along its normal.
            if (!Step.ClosedCondition && Points.size() >= 3)
            {
                for (const MirrorPlane& Plane : Planes)
                {
                    double Excursion = 0.0;                                                  // [len] - A curve lying in the plane gets no tangent
                    for (const Vector3& Point : Points) Excursion = std::max(Excursion, std::fabs(Dot(Point - Plane.Point, Plane.Normal)));
                    if (Excursion <= MirrorSnapFraction * Settings.Tolerance) continue;
                    for (int End = 0; End < 2; ++End)
                    {
                        Vector3& Point = End == 0 ? Points.front() : Points.back();
                        const double Offset = Dot(Point - Plane.Point, Plane.Normal);
                        if (std::fabs(Offset) > MirrorSnapFraction * Settings.Tolerance) continue;
                        Point = Point - Plane.Normal * Offset;
                        (End == 0 ? Settings.StartTangent : Settings.EndTangent) = Plane.Normal;
                    }
                }
            }
            CurveFitOutcome Fit;
            if (!Step.CurveKnots.empty())
            {
                // ④′ An exact curve (a drawn 3D curve): Points are its poles on the clamped CurveKnots — nothing to fit.
                const int Degree = static_cast<int>(Step.CurveKnots.size()) - static_cast<int>(Step.Points.size()) - 1;
                if (Degree < 1 || Step.Points.size() < 2 || Step.CurveKnots.front() >= Step.CurveKnots.back()) { Refuse("SpaceCurve: knots do not fit the poles"); break; }
                NurbsCurveDefinition Exact;
                Exact.Degree  = Degree;
                Exact.Poles   = Step.Points;
                Exact.Weights.assign(Step.Points.size(), 1.0);
                const double Low = Step.CurveKnots.front(), Span = Step.CurveKnots.back() - Step.CurveKnots.front();
                for (double Knot : Step.CurveKnots) Exact.Knots.push_back((Knot - Low) / Span);  // Domain [0, 1] as a fit's
                Fit.Curve = NurbsCurve{ std::make_shared<NurbsCurveDefinition>(std::move(Exact)) };
                Fit.Spans = 0;                                                               // Distinct knot intervals
                for (size_t Index = 1; Index < Step.CurveKnots.size(); ++Index) Fit.Spans += Step.CurveKnots[Index] > Step.CurveKnots[Index - 1] ? 1 : 0;
            }
            else Fit = FitSpaceCurve(Points, Settings);
            if (!Fit.Refusal.empty()) { Refuse("SpaceCurve: " + Fit.Refusal); break; }
            if (!Step.PoleEdits.empty())
            {
                NurbsCurveDefinition Edited = *Fit.Curve.Definition;
                bool InsideCondition = true;
                for (const FeaturePoleEdit& Edit : Step.PoleEdits)
                {
                    if (Edit.Index < 0 || Edit.Index >= static_cast<int>(Edited.Poles.size()))
                    {
                        Refuse("SpaceCurve: pole edit " + std::to_string(Edit.Index) + " is beyond the " + std::to_string(Edited.Poles.size()) + " fitted poles (tolerance changed?)");
                        InsideCondition = false;
                        break;
                    }
                    Edited.Poles[Edit.Index] = Edit.Position;
                }
                if (!InsideCondition) break;
                Fit.Curve = NurbsCurve{ std::make_shared<NurbsCurveDefinition>(std::move(Edited)) };
            }
            Outcome.Curves[Step.Label] = std::move(Fit);
            break;
        }
        case FeatureStepKind::Loft:
        {
            // ⑤ Skin a sheet through the named sections, in order: SpaceCurve results, or sketches holding one chain
            //    (line, polyline, arc, slot, rectangle, circle, spline) composed into one exact NURBS each.
            std::vector<NurbsCurve> Sections;
            for (const std::string& Section : Step.Sections)
            {
                if (const auto Found = Outcome.Curves.find(Section); Found != Outcome.Curves.end()) { Sections.push_back(Found->second.Curve); continue; }
                const auto Drawn = Sketches.find(Section);
                if (Drawn == Sketches.end() || !Drawn->second.UsableCondition) { Refuse("Loft: section " + Section + " is unavailable"); break; }
                const SketchSectionOutcome Composed = ComposeSketchSection(Drawn->second.Model, Drawn->second.Plane);
                if (!Composed.Curve) { Refuse("Loft: section " + Section + ": " + Composed.Refusal); break; }
                Sections.push_back(*Composed.Curve);
            }
            if (Report.Status == FeatureStepStatus::Refused) break;
            if (Step.ClosedCondition)
            {
                // ⑤′ Solid loft (Plasticity: closed sections loft a solid): the skin and its two plane caps close into a
                //    body of its own, keyed by the step, beside the panel-built body.
                if (Step.Continuity != 0) { Refuse("Loft: a solid loft is G0 (G1 / G2 join a loft to neighbouring panels)"); break; }
                // Clamped sections: a closed one starts and ends on the same pole.
                size_t Open = Sections.size();
                for (size_t Index = 0; Index < Sections.size() && Open == Sections.size(); ++Index)
                {
                    if (!Sections[Index].Definition) { Open = Index; break; }
                    const std::vector<Vector3>& Poles = Sections[Index].Definition->Poles;
                    if (Poles.size() < 2 || Distance(Poles.front(), Poles.back()) > 1.0e-9 * (1.0 + Length(Poles.front()))) Open = Index;
                }
                if (Open < Sections.size()) { Refuse("Loft: section " + Step.Sections[Open] + " is open, so the loft cannot close into a solid"); break; }
                const LoftSurfaceOutcome Skin = LoftSectionSurface(Sections, LoftSettings{});
                if (!Skin.Refusal.empty()) { Refuse(Skin.Refusal); break; }
                PanelAssemblySettings Capping;
                Capping.CapCondition = true;
                const PanelAssemblyOutcome Assembly = AssemblePanels(Partition, { SurfacePanel{ Skin.Surface } }, Capping, "Close");
                if (!Assembly.Refusal.empty()) { Refuse("Loft: " + Assembly.Refusal); break; }
                if (!Assembly.SolidCondition)
                {
                    Refuse("Loft: open sections cannot close into a solid (" + std::to_string(Assembly.LaminarEdges) + " rim edges stay open)");
                    break;
                }
                std::vector<FaceToken>   Faces = Assembly.Faces;
                std::vector<std::string> Names = { Step.Label + ".Side(Surface)" };
                for (size_t Cap = 0; Cap < Assembly.Caps.size(); ++Cap)
                {
                    Faces.push_back(Assembly.Caps[Cap]);
                    Names.push_back(Step.Label + ".Cap(" + std::to_string(Cap) + ")");
                }
                Naming  = NamePanelBody(Partition, Assembly.Body, Faces, Names);
                Current = Assembly.Body;
                Create();
                break;
            }
            if (const std::string Blocked = PanelJoinRefusal("Loft"); !Blocked.empty()) { Refuse(Blocked); break; }
            // ⑥ G1 / G2 to the latest panel carrying an end section as its boundary (V6).
            LoftSettings Settings;
            if (Step.Continuity < 0 || Step.Continuity > 2) { Refuse("Loft: continuity must be 0 (G0), 1 (G1) or 2 (G2)"); break; }
            if (Step.Continuity > 0 && !Step.Sections.empty())
            {
                auto Neighbour = [&](const std::string& Section) -> const TreePanel*
                {
                    for (auto Panel = Panels.rbegin(); Panel != Panels.rend(); ++Panel)
                    {
                        if (!Panel->ImageCondition && std::find(Panel->Boundaries.begin(), Panel->Boundaries.end(), Section) != Panel->Boundaries.end()) return &*Panel;
                    }
                    return nullptr;
                };
                const TreePanel* Before = Neighbour(Step.Sections.front());
                const TreePanel* After  = Neighbour(Step.Sections.back());
                if (!Before && !After) { Refuse("Loft: G" + std::to_string(Step.Continuity) + " needs a panel ending on " + Step.Sections.front() + " or " + Step.Sections.back()); break; }
                if (Before) Settings.Start = { Step.Continuity, Before->Panel.Surface };
                if (After)  Settings.End   = { Step.Continuity, After->Panel.Surface };
            }
            const LoftSurfaceOutcome Loft = LoftSectionSurface(Sections, Settings);
            if (!Loft.Refusal.empty()) { Refuse(Loft.Refusal); break; }
            Panels.push_back({ SurfacePanel{ Loft.Surface }, Step.Label + ".Side(Surface)", { Step.Sections.front(), Step.Sections.back() }, false });
            if (const std::string Problem = Reassemble("Sew", false); !Problem.empty()) { Panels.pop_back(); Refuse(Problem); }
            break;
        }
        case FeatureStepKind::Network:
        {
            // ⑦ Gordon / Coons panel through the named curves (families found by meeting, V5).
            std::vector<NurbsCurve> Curves;
            for (const std::string& Section : Step.Sections)
            {
                const auto Found = Outcome.Curves.find(Section);
                if (Found == Outcome.Curves.end()) { Refuse("Network: curve " + Section + " is unavailable"); break; }
                Curves.push_back(Found->second.Curve);
            }
            if (Report.Status == FeatureStepStatus::Refused) break;
            if (Step.Continuity != 0) { Refuse("Network: G1 / G2 to a neighbour is not available for networks yet (🚧)"); break; }
            if (const std::string Blocked = PanelJoinRefusal("Network"); !Blocked.empty()) { Refuse(Blocked); break; }
            const NetworkSurfaceOutcome Network = ConstructNetworkSurface(Curves, NetworkSettings{}, Step.Sections);
            if (!Network.Refusal.empty()) { Refuse(Network.Refusal); break; }
            std::vector<std::string> Boundaries;
            for (const std::vector<int>* Order : { &Network.FirstOrder, &Network.SecondOrder })
            {
                if (Order->empty()) continue;
                Boundaries.push_back(Step.Sections[Order->front()]);
                Boundaries.push_back(Step.Sections[Order->back()]);
            }
            Panels.push_back({ SurfacePanel{ Network.Surface }, Step.Label + ".Side(Surface)", Boundaries, false });
            if (const std::string Problem = Reassemble("Sew", false); !Problem.empty()) { Panels.pop_back(); Refuse(Problem); }
            break;
        }
        case FeatureStepKind::Mirror:
        {
            // ⑧ Mirror image of every panel so far; halves sharing a boundary on the plane sew into one body (V7).
            if (Panels.empty()) { Refuse("Mirror: there are no surface panels to mirror"); break; }
            if (!ClosedBy.empty()) { Refuse("Mirror: the panels are already closed by " + ClosedBy); break; }
            if (!(Length(Step.Plane.ZAxis) > 0.0)) { Refuse("Mirror: the plane has no normal"); break; }
            const Vector3 Normal = Normalize(Step.Plane.ZAxis);
            const size_t  Count  = Panels.size();
            for (size_t Index = 0; Index < Count; ++Index)
            {
                Panels.push_back({ MirrorPanel(Panels[Index].Panel, Step.Plane.Origin, Normal), Step.Label + ".Image(" + Panels[Index].Name + ")", {}, true });
            }
            if (const std::string Problem = Reassemble("Sew", false); !Problem.empty()) { Panels.resize(Count); Refuse("Mirror: " + Problem); }
            break;
        }
        case FeatureStepKind::Close:
        {
            // ⑨ Plane caps on the planar rim loops; the closed panel body becomes a solid (V7).
            if (Panels.empty()) { Refuse("Close: there are no surface panels to close"); break; }
            if (!ClosedBy.empty()) { Refuse("Close: the panels are already closed by " + ClosedBy); break; }
            if (const std::string Problem = Reassemble(Step.Label, true); !Problem.empty()) { Refuse(Problem); break; }
            ClosedBy = Step.Label;
            break;
        }
        case FeatureStepKind::Combine:
        {
            // ⑩ Boolean between two bodies of the part (Plasticity Q): the target keeps its key, the tool is consumed.
            if (Step.Combination != FeatureCombination::Union && Step.Combination != FeatureCombination::Difference && Step.Combination != FeatureCombination::Intersection)
            {
                Refuse("Combine: choose Union, Difference or Intersection");
                break;
            }
            const int ToolIndex = FindBody(Step.Tool);
            if (!Load(Step.Target)) { Refuse("Combine: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
            if (ToolIndex < 0) { Refuse("Combine: no body named " + Step.Tool); break; }
            if (ToolIndex == Target) { Refuse("Combine: " + Step.Tool + " cannot combine with itself"); break; }
            if (Partition.Bodies.Resolve(Current).Category == BodyCategory::Sheet || Partition.Bodies.Resolve(Bodies[ToolIndex].Body).Category == BodyCategory::Sheet)
            {
                Refuse("Combine: sheet bodies cannot combine yet (🚧 close the panels first)");
                break;
            }
            const BooleanOperation Operation = Step.Combination == FeatureCombination::Difference ? BooleanOperation::Difference
                                             : Step.Combination == FeatureCombination::Intersection ? BooleanOperation::Intersection : BooleanOperation::Union;
            const BooleanOutcome Boolean = ComputeBoolean(Partition, Current, Bodies[ToolIndex].Body, Operation);
            if (!Boolean.Refusal.empty()) { Refuse(Boolean.Refusal); break; }
            Naming  = NameBooleanBody(Partition, Boolean.Body, Boolean, Naming, Bodies[ToolIndex].Naming);
            Current = Boolean.Body;
            Store();
            if (ToolIndex == PanelBody) ClosedBy = Step.Label;
            ForgetJoints(Bodies[ToolIndex].Key);
            Bodies.erase(Bodies.begin() + ToolIndex);
            for (int* Index : { &Target, &Active, &PanelBody })
            {
                if (*Index == ToolIndex) *Index = -1;
                else if (*Index > ToolIndex) --*Index;
            }
            break;
        }
        case FeatureStepKind::Cut:
        {
            // ⑪ Split a body (Plasticity C): the outside piece keeps the key, the inside piece becomes a body keyed by the step.
            if (!Load(Step.Target)) { Refuse("Cut: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
            const int        Source = Target;
            OrthonormalFrame Plane;
            SketchProfile    Profile;
            double           Reach = 0.0;
            if (!Step.Sections.empty())
            {
                // A SpaceCurve projected along the cut direction (character line seen from a view).
                const auto Found = Outcome.Curves.find(Step.Sections.front());
                if (Found == Outcome.Curves.end()) { Refuse("Cut: curve " + Step.Sections.front() + " is unavailable"); break; }
                if (!(Length(Step.Plane.ZAxis) > 0.0)) { Refuse("Cut: the cut direction is zero"); break; }
                Plane.Origin = Step.Plane.Origin;
                Plane.ZAxis  = Normalize(Step.Plane.ZAxis);
                Vector3 Reference = Step.Plane.XAxis - Plane.ZAxis * Dot(Step.Plane.XAxis, Plane.ZAxis);
                if (!(Length(Reference) > 1.0e-9)) Reference = std::fabs(Plane.ZAxis.X) < 0.9 ? Vector3{ 1.0, 0.0, 0.0 } - Plane.ZAxis * Plane.ZAxis.X : Vector3{ 0.0, 1.0, 0.0 } - Plane.ZAxis * Plane.ZAxis.Y;
                Plane.XAxis = Normalize(Reference);
                Plane.YAxis = Cross(Plane.ZAxis, Plane.XAxis);
                Reach = MeasureCutReach(Partition, Current, Plane);
                const std::optional<ProfileSegment> Segment = ProjectCutCurve(Found->second.Curve, Plane, Step.Sections.front());
                if (!Segment.has_value()) { Refuse("Cut: curve " + Step.Sections.front() + " is rational or periodic (🚧 polynomial curves only)"); break; }
                if (Distance(Segment->Start, Segment->End) <= 1.0e-9 * std::max(Reach, 1.0))
                {
                    ProfileLoop Loop;
                    Loop.Segments.push_back(*Segment);
                    Profile.Loops.push_back(std::move(Loop));
                    Profile.Scale = 2.0 * Reach;
                    ArrangeProfileLoops(Profile);
                }
                else Profile = CloseCutChain({ *Segment }, Reach);
            }
            else
            {
                const auto Found = Sketches.find(Step.Profile);
                if (Found == Sketches.end() || !Found->second.UsableCondition) { Refuse("Cut: cutting sketch " + Step.Profile + " is unavailable"); break; }
                Plane   = Found->second.Plane;
                Reach   = MeasureCutReach(Partition, Current, Plane);
                Profile = BuildSketchCutProfile(Found->second.Model, Reach);
            }
            const BodyCutOutcome Cut = CutBody(Partition, Current, Profile, Plane, Reach);
            if (!Cut.Refusal.empty()) { Refuse(Cut.Refusal); break; }
            const BodyNaming ToolNaming = NameSweptBody(Partition, Cut.Tool, Step.Label);
            const BodyNaming Original   = Naming;
            Naming  = NameBooleanBody(Partition, Cut.Outside.Body, Cut.Outside, Original, ToolNaming);
            Current = Cut.Outside.Body;
            Store();
            if (Source == PanelBody) ClosedBy = Step.Label;                                  // Its panels no longer re-sew
            Naming  = NameBooleanBody(Partition, Cut.Inside.Body, Cut.Inside, Original, ToolNaming);
            Current = Cut.Inside.Body;
            Create();
            break;
        }
        case FeatureStepKind::Imprint:
        {
            // ⑳ 2D cutter (Plasticity / Fusion split face): the sketch outline (closed loops, or an open chain closed past
            //    the body) is swept a hair along ±𝑧 — the side away from the material, Extent's sign — into a tool that only
            //    touches the body where the sketch lies; every body face keeps its fragments, split along that outline.
            if (!Load(Step.Target)) { Refuse("Imprint: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
            if (Partition.Bodies.Resolve(Current).Category == BodyCategory::Sheet) { Refuse("Imprint: the body is a sheet (🚧 imprints on sheets)"); break; }
            const auto Found = Sketches.find(Step.Profile);
            if (Found == Sketches.end() || !Found->second.UsableCondition) { Refuse("Imprint: sketch " + Step.Profile + " is unavailable"); break; }
            const OrthonormalFrame& Plane   = Found->second.Plane;
            const double            Reach   = MeasureCutReach(Partition, Current, Plane);
            const SketchProfile     Profile = BuildSketchCutProfile(Found->second.Model, Reach);
            if (!Profile.Refusal.empty()) { Refuse("Imprint: " + Profile.Refusal); break; }
            const double Lift  = std::max(1.0e-2 * Reach, 1.0e-4) * (EvaluateFeatureValue(Tree, Step.Extent) < 0.0 ? -1.0 : 1.0);   // [len] - Tool height
            const SweepOutcome Sweep = ExtrudeProfile(Partition, Profile, Plane, Lift, true);
            if (!Sweep.Refusal.empty()) { Refuse("Imprint: " + Sweep.Refusal); break; }
            int Before = 0;
            const BooleanOutcome Imprint = ImprintBody(Partition, Current, Sweep.Body, Plane.Origin, Plane.ZAxis, Before);
            if (!Imprint.Refusal.empty()) { Refuse("Imprint: " + Imprint.Refusal); break; }
            if (Imprint.Faces <= Before) { Refuse("Imprint: " + Step.Profile + " lies on no face of the body (draw it on a face)"); break; }
            Naming  = NameBooleanBody(Partition, Imprint.Body, Imprint, Naming, NameSweptBody(Partition, Sweep, Step.Label));
            Current = Imprint.Body;
            Store();
            break;
        }
        case FeatureStepKind::Remove:
        {
            // ⑫ Delete a body (Plasticity X); the last remaining body becomes active when the active one goes.
            if (!Load(Step.Target)) { Refuse("Remove: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
            const int Removed = Target;
            if (Removed == PanelBody) ClosedBy = Step.Label;
            ForgetJoints(Bodies[Removed].Key);
            Bodies.erase(Bodies.begin() + Removed);
            for (int* Index : { &Target, &Active, &PanelBody })
            {
                if (*Index == Removed) *Index = -1;
                else if (*Index > Removed) --*Index;
            }
            if (Active < 0) Active = static_cast<int>(Bodies.size()) - 1;
            Target  = Active;
            Current = Active >= 0 ? Bodies[Active].Body : BodyToken{};
            Naming  = Active >= 0 ? Bodies[Active].Naming : BodyNaming{};
            break;
        }
        case FeatureStepKind::Transform:
        {
            // ⑬ Move / rotate / scale a body in place (Plasticity G / R / S): exact image, every name kept.
            if (!Load(Step.Target)) { Refuse("Transform: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
            const auto KnownVolume = std::find_if(Outcome.Volumes.rbegin(), Outcome.Volumes.rend(), [&](const auto& Entry) { return Entry.first == Current; });
            const TransformOutcome Moved = TransformBody(Partition, Current, Step.Placement, KnownVolume != Outcome.Volumes.rend() ? KnownVolume->second : std::numeric_limits<double>::quiet_NaN());
            if (!Moved.Refusal.empty()) { Refuse(Moved.Refusal); break; }
            Naming  = CarryNaming(Partition, Moved, Naming, {});
            Current = Moved.Body;
            if (std::isfinite(Moved.Volume)) Outcome.Volumes.emplace_back(Moved.Body, Moved.Volume);   // The image was integrated by its assembly
            if (Target == PanelBody) ClosedBy = Step.Label;                                  // Re-sewing the panels would undo the move
            Store();
            // Bodies jointed to it follow a rigid move (an assembly moves as one); a scale leaves them where they are.
            const AffineMap& Map = Step.Placement;
            const double Determinant = Dot(Map.ColumnX, Cross(Map.ColumnY, Map.ColumnZ));
            const bool Rigid = std::fabs(Dot(Map.ColumnX, Map.ColumnX) - 1.0) < 1.0e-12 && std::fabs(Dot(Map.ColumnY, Map.ColumnY) - 1.0) < 1.0e-12
                            && std::fabs(Dot(Map.ColumnZ, Map.ColumnZ) - 1.0) < 1.0e-12 && std::fabs(Dot(Map.ColumnX, Map.ColumnY)) < 1.0e-12
                            && std::fabs(Dot(Map.ColumnY, Map.ColumnZ)) < 1.0e-12 && std::fabs(Dot(Map.ColumnZ, Map.ColumnX)) < 1.0e-12 && Determinant > 0.0;
            if (Rigid)
            {
                const std::string Key = Bodies[Target].Key;
                if (const std::string Problem = CarryDependents(Key, Map); !Problem.empty()) { Refuse("Transform: " + Problem); break; }
                for (RegeneratedJoint& Joint : Outcome.Joints) if (Joint.Moving == Key) Joint.MovingOrigin = MapFrame(Map, Joint.MovingOrigin);
            }
            break;
        }
        case FeatureStepKind::Flip:
        {
            // ⑲ A sheet turned over (flip normals): same carriers, every face sense and loop reversed, every name kept.
            if (!Load(Step.Target)) { Refuse("Flip: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
            // ⑲′ Face references: only the pieces (faces joined through shared edges) holding one turn over.
            std::vector<FaceToken> Seeds;
            bool                   Lost = false;
            for (TopologyReference& Reference : Step.References)
            {
                const ResolvedReference Resolved = ResolveReference(Partition, Current, Naming, Reference);
                Report.Resolutions.push_back(Resolved);
                if (Resolved.Method == ResolutionMethod::Refused || Reference.Kind != ReferenceKind::Face) { Lost = true; continue; }
                Seeds.push_back(Partition.Faces.TokenAt(Resolved.Slot));
                if (CaptureCondition) Reference.Signature = CaptureSignature(Partition, Reference.Kind, Resolved.Slot);
            }
            if (Lost) { Refuse("Flip: a picked face is gone (" + std::to_string(Step.References.size() - Seeds.size()) + " not found)"); break; }
            const TransformOutcome Turned = Step.References.empty() ? ReverseBody(Partition, Current) : ReverseBodyPieces(Partition, Current, Seeds);
            if (!Turned.Refusal.empty()) { Refuse(Turned.Refusal); break; }
            Naming  = CarryNaming(Partition, Turned, Naming, {});
            Current = Turned.Body;
            if (Target == PanelBody) ClosedBy = Step.Label;                                  // Re-sewing the panels would turn them back
            Store();
            break;
        }
        case FeatureStepKind::Duplicate:
        {
            // ⑭ Copy of a body (Plasticity Shift+D / Ctrl+V) as it is now, or as it stood after step Source.
            RegeneratedBody Original;
            if (Step.Source.empty())
            {
                if (!Load(Step.Target)) { Refuse("Duplicate: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
                Original = Bodies[Target];
            }
            else
            {
                const auto Found = Snapshots.find({ Step.Source, Step.Target });
                if (Found == Snapshots.end()) { Refuse("Duplicate: " + (Step.Target.empty() ? std::string("(active)") : Step.Target) + " did not exist after " + Step.Source); break; }
                Original = Found->second;
            }
            const TransformOutcome Copy = TransformBody(Partition, Original.Body, Step.Placement);
            if (!Copy.Refusal.empty()) { Refuse("Duplicate: " + Copy.Refusal); break; }
            Naming  = CarryNaming(Partition, Copy, Original.Naming, Step.Label);
            Current = Copy.Body;
            Create();
            break;
        }
        case FeatureStepKind::Import:
        {
            // ⑱ One body read from the step's embedded STEP text (priority 4 X4): a body of its own keyed by the step,
            //    faces named Label.Face(k) in file order. The text is the kernel's own canonical export of the body, so
            //    replay rebuilds the same topology with bit-identical vertices every time (ExchangeValidation EX9).
            StepImportRequest Request;
            Request.ModelUnitMillimetres = Step.ExchangeUnit;
            Request.Label                = "Import";
            const StepImport Reading = ReadStep(Partition, Step.Exchange, Request);
            if (!Reading.Refusal.empty()) { Refuse(Reading.Refusal); break; }
            if (Reading.Bodies.size() != 1)
            {
                Refuse("Import: the embedded STEP text holds " + std::to_string(Reading.Bodies.size()) + " bodies (one per step)");
                break;
            }
            const BodyInventory      Inventory = Partition.CollectInventory(Reading.Bodies.front());
            std::vector<std::string> Names;
            for (size_t Index = 0; Index < Inventory.Faces.size(); ++Index) Names.push_back(Step.Label + ".Face(" + std::to_string(Index + 1) + ")");
            Naming  = NamePanelBody(Partition, Reading.Bodies.front(), Inventory.Faces, Names);
            Current = Reading.Bodies.front();
            Create();
            for (const std::string& Note : Reading.Notes) Report.Notes.push_back(Note);
            break;
        }
        case FeatureStepKind::Joint:
        {
            // ⑰ Place Step.Target by a joint to Step.Tool (Fusion joints): both joint origins are read off the current
            //    geometry (References[0] on the target, [1] on the tool), the target takes the rigid placement and every
            //    body jointed to it earlier follows. The tool stays where it is (it is the ground of this joint).
            if (Step.References.size() != 2) { Refuse("Joint: pick one joint origin on each body"); break; }
            if (Step.Target.empty() || Step.Tool.empty()) { Refuse("Joint: name the placed body and the body it joins"); break; }
            if (Step.Target == Step.Tool) { Refuse("Joint: " + Step.Target + " cannot be jointed to itself"); break; }
            const int FixedIndex = FindBody(Step.Tool);
            if (FixedIndex < 0) { Refuse("Joint: no body named " + Step.Tool); break; }
            if (!Load(Step.Target)) { Refuse("Joint: no body named " + Step.Target); break; }
            if (const auto Existing = Jointed.find(Step.Target); Existing != Jointed.end())
            {
                Refuse("Joint: " + Step.Target + " is already jointed to " + Existing->second + " (🚧 one joint per placed body)");
                break;
            }
            bool Loop = false;
            for (std::string Key = Step.Tool; !Loop;)
            {
                const auto Up = Jointed.find(Key);
                if (Up == Jointed.end()) break;
                Key  = Up->second;
                Loop = Key == Step.Target;
            }
            if (Loop) { Refuse("Joint: " + Step.Tool + " is already placed relative to " + Step.Target + ", so this joint would close a loop (🚧 loop solving)"); break; }
            const ResolvedReference MovingResolved = ResolveReference(Partition, Current, Naming, Step.References[0]);
            const ResolvedReference FixedResolved  = ResolveReference(Partition, Bodies[FixedIndex].Body, Bodies[FixedIndex].Naming, Step.References[1]);
            Report.Resolutions = { MovingResolved, FixedResolved };
            if (MovingResolved.Method == ResolutionMethod::Refused) { Refuse("Joint: " + MovingResolved.Refusal); break; }
            if (FixedResolved.Method == ResolutionMethod::Refused) { Refuse("Joint: " + FixedResolved.Refusal); break; }
            if (CaptureCondition)
            {
                Step.References[0].Signature = CaptureSignature(Partition, Step.References[0].Kind, MovingResolved.Slot);
                Step.References[1].Signature = CaptureSignature(Partition, Step.References[1].Kind, FixedResolved.Slot);
            }
            const JointConnector MovingOrigin = ResolveJointConnector(Partition, Step.References[0].Kind, MovingResolved.Slot);
            if (!MovingOrigin.Refusal.empty()) { Refuse(MovingOrigin.Refusal); break; }
            const JointConnector FixedOrigin = ResolveJointConnector(Partition, Step.References[1].Kind, FixedResolved.Slot);
            if (!FixedOrigin.Refusal.empty()) { Refuse(FixedOrigin.Refusal); break; }
            const FeatureJoint& Joint = Step.Joint;
            JointMotion Values;
            Values.Angle       = EvaluateFeatureValue(Tree, Joint.Angle);
            Values.Slide       = EvaluateFeatureValue(Tree, Joint.Slide);
            Values.SecondSlide = EvaluateFeatureValue(Tree, Joint.SecondSlide);
            Values.Pitch       = EvaluateFeatureValue(Tree, Joint.Pitch);
            Values.Roll        = EvaluateFeatureValue(Tree, Joint.Roll);
            Values.Offset      = EvaluateFeatureValue(Tree, Joint.Offset);
            const JointPlacement Placement = SolveJointPlacement(MovingOrigin.Frame, FixedOrigin.Frame, Joint.Kind, Values, Joint.FlippedCondition, Joint.AngleRange, Joint.SlideRange);
            if (!Placement.Refusal.empty()) { Refuse(Placement.Refusal); break; }
            const TransformOutcome Moved = TransformBody(Partition, Current, Placement.Map);
            if (!Moved.Refusal.empty()) { Refuse("Joint: " + Moved.Refusal); break; }
            Naming  = CarryNaming(Partition, Moved, Naming, {});
            if (std::isfinite(Moved.Volume)) Outcome.Volumes.emplace_back(Moved.Body, Moved.Volume);   // The image was integrated by its assembly
            Current = Moved.Body;
            if (Target == PanelBody) ClosedBy = Step.Label;                                  // Re-sewing the panels would undo the move
            Store();
            if (const std::string Problem = CarryDependents(Step.Target, Placement.Map); !Problem.empty()) { Refuse("Joint: " + Problem); break; }
            Jointed[Step.Target] = Step.Tool;
            Outcome.Joints.push_back({ Step.Label, Step.Target, Step.Tool, Joint.Kind, Placement.Target, FixedOrigin.Frame });
            break;
        }
        case FeatureStepKind::Pipe:
        {
            // ⑮ Circle of radius Size along a path (Plasticity P): lines / arcs / circles exact, free-form paths a NURBS tube.
            const double Radius = EvaluateFeatureValue(Tree, Step.Size);
            BodyToken Tube;
            Vector3   PathStart;
            if (!Step.Sections.empty())
            {
                const auto Found = Outcome.Curves.find(Step.Sections.front());
                if (Found == Outcome.Curves.end()) { Refuse("Pipe: path curve " + Step.Sections.front() + " is unavailable"); break; }
                const PipeOutcome Swept = PipeAlongCurve(Partition, Found->second.Curve, Radius);
                if (!Swept.Refusal.empty()) { Refuse(Swept.Refusal); break; }
                Tube      = Swept.Body;
                PathStart = EvaluateNurbsCurve(*Found->second.Curve.Definition, MeasureNurbsCurveDomain(*Found->second.Curve.Definition).Low).Position;
            }
            else
            {
                const auto Found = Sketches.find(Step.Profile);
                if (Found == Sketches.end() || !Found->second.UsableCondition) { Refuse("Pipe: path sketch " + Step.Profile + " is unavailable"); break; }
                const SolvedSketch& Solved = Found->second;
                std::vector<int> Drawn;
                for (size_t Index = 0; Index < Solved.Model.Curves.size(); ++Index) if (!Solved.Model.Curves[Index].Construction) Drawn.push_back(static_cast<int>(Index));
                if (Drawn.size() != 1) { Refuse("Pipe: path sketch " + Step.Profile + " has " + std::to_string(Drawn.size()) + " curves (🚧 one line, arc, circle or spline; chains with corners need mitred joints)"); break; }
                std::string Problem;
                const ProfileSegment Segment = ResolveSketchCurveSegment(Solved.Model, Drawn.front(), 1.0e-12, Problem);
                if (!Problem.empty()) { Refuse("Pipe: " + Problem); break; }
                const OrthonormalFrame& Plane = Solved.Plane;
                const auto Lift = [&](Vector2 Point) { return Plane.Origin + Plane.XAxis * Point.X + Plane.YAxis * Point.Y; };
                PathStart = Lift(Segment.Start);
                if (Segment.Kind == ProfileSegmentKind::Spline)
                {
                    std::vector<Vector3> Poles;
                    for (const Vector2& Pole : Segment.Spline.Poles) Poles.push_back(Lift(Pole));
                    const std::optional<NurbsCurve> Path = ConstructNurbsCurve(Segment.Spline.Degree, Segment.Spline.Knots, Poles, std::vector<double>(Poles.size(), 1.0));
                    if (!Path) { Refuse("Pipe: the spline path has invalid knots"); break; }
                    const PipeOutcome Swept = PipeAlongCurve(Partition, *Path, Radius);
                    if (!Swept.Refusal.empty()) { Refuse(Swept.Refusal); break; }
                    Tube = Swept.Body;
                }
                else
                {
                    SweepOutcome Swept;
                    if (Segment.Kind == ProfileSegmentKind::Line)
                    {
                        const Vector3 Start = Lift(Segment.Start), End = Lift(Segment.End);
                        Swept = PipeAlongPath(Partition, StraightLine{ Start, Normalize(End - Start) }, 0.0, Distance(Start, End), Radius);
                    }
                    else
                    {
                        CircularArc Arc;
                        Arc.Frame  = { Lift(Segment.Centre), Plane.XAxis, Plane.YAxis, Plane.ZAxis };
                        Arc.Radius = Segment.Radius;
                        Swept = PipeAlongPath(Partition, Arc, Segment.StartAngle, Segment.StartAngle + Segment.Sweep, Radius);
                    }
                    if (!Swept.Refusal.empty()) { Refuse(Swept.Refusal); break; }
                    Tube = Swept.Body;
                }
            }
            const BodyNaming ToolNaming = NamePipeBody(Partition, Tube, Step.Label, PathStart);
            const bool SeparateCondition = Step.Combination == FeatureCombination::Separate || Step.Combination == FeatureCombination::New;
            if (SeparateCondition || Bodies.empty())
            {
                if (!SeparateCondition && Step.Combination != FeatureCombination::Union) { Refuse(std::string("Pipe: nothing to ") + FeatureCombinationLabel(Step.Combination) + " with"); break; }
                Current = Tube;
                Naming  = ToolNaming;
                Create();
                break;
            }
            if (!Load(Step.Target)) { Refuse("Pipe: no body named " + (Step.Target.empty() ? std::string("(active)") : Step.Target)); break; }
            if (Partition.Bodies.Resolve(Current).Category == BodyCategory::Sheet) { Refuse("Pipe: the target is a sheet (🚧 close the panels first, or pipe as a new body)"); break; }
            const BooleanOperation Operation = Step.Combination == FeatureCombination::Difference ? BooleanOperation::Difference
                                             : Step.Combination == FeatureCombination::Intersection ? BooleanOperation::Intersection : BooleanOperation::Union;
            const BooleanOutcome Boolean = ComputeBoolean(Partition, Current, Tube, Operation);
            if (!Boolean.Refusal.empty()) { Refuse(Boolean.Refusal); break; }
            Naming  = NameBooleanBody(Partition, Boolean.Body, Boolean, Naming, ToolNaming);
            Current = Boolean.Body;
            Store();
            break;
        }
        }
        if (const auto Requested = SnapshotRequests.find(Step.Label); Requested != SnapshotRequests.end())
        {
            for (const std::string& Key : Requested->second)
            {
                const int Index = Key.empty() ? Active : FindBody(Key);
                if (Index >= 0) Snapshots[{ Step.Label, Key }] = Bodies[Index];
                Served.insert({ Step.Label, Key });
            }
        }
        Report.Body         = Current;
        Report.Naming       = Naming;
        Report.Target       = Target >= 0 ? Bodies[Target].Key : std::string();
        Report.Milliseconds = std::chrono::duration<double, std::milli>(Clock::now() - StepStarted).count();
        Outcome.Refused    += Report.Status == FeatureStepStatus::Refused ? 1 : 0;
        if (Report.Status == FeatureStepStatus::Refused) FRONTIER_TRACE("Replay", Step.Label + " refused: " + Report.Refusal);
        Outcome.Steps.push_back(std::move(Report));
        if (Cache)
        {
            // Digest after the replay: capture may have rewritten the step's reference signatures.
            auto Saved = std::make_shared<ReplayCheckpoint>();
            Saved->Partition = Partition;
            Saved->Outcome   = Outcome;
            Saved->Sketches  = Sketches;
            Saved->Bodies    = Bodies;
            Saved->Active    = Active;
            Saved->PanelBody = PanelBody;
            Saved->Panels    = Panels;
            Saved->ClosedBy  = ClosedBy;
            Saved->Snapshots = Snapshots;
            Saved->Served    = Served;
            Saved->Jointed   = Jointed;
            Cache->Checkpoints[StepIndex] = std::move(Saved);
            Cache->Digests[StepIndex]     = DigestFeatureStep(Tree, Step);
            Cache->Captured[StepIndex]    = CaptureCondition || (Step.References.empty() && Step.SizeAnchors.empty());   // Capture writes reference signatures only
        }
    }
    if (Active >= 0)
    {
        Outcome.Body   = Bodies[Active].Body;
        Outcome.Naming = Bodies[Active].Naming;
        Outcome.Active = Bodies[Active].Key;
    }
    Outcome.Bodies       = Bodies;
    for (const TreePanel& Panel : Panels) Outcome.Panels[Panel.Name] = Panel.Panel.Surface;
    Outcome.Milliseconds = std::chrono::duration<double, std::milli>(Clock::now() - Started).count();
    return Outcome;
}

const char* FeatureStepKindLabel(FeatureStepKind Kind) noexcept
{
    switch (Kind)
    {
    case FeatureStepKind::Sketch:        return "Sketch";
    case FeatureStepKind::Extrude:       return "Extrude";
    case FeatureStepKind::Revolve:       return "Revolve";
    case FeatureStepKind::Blend:         return "Blend";
    case FeatureStepKind::FaceOperation: return "FaceOperation";
    case FeatureStepKind::SpaceCurve:    return "SpaceCurve";
    case FeatureStepKind::Loft:          return "Loft";
    case FeatureStepKind::Network:       return "Network";
    case FeatureStepKind::Mirror:        return "Mirror";
    case FeatureStepKind::Close:         return "Close";
    case FeatureStepKind::Combine:       return "Combine";
    case FeatureStepKind::Cut:           return "Cut";
    case FeatureStepKind::Remove:        return "Remove";
    case FeatureStepKind::Transform:     return "Transform";
    case FeatureStepKind::Duplicate:     return "Duplicate";
    case FeatureStepKind::Pipe:          return "Pipe";
    case FeatureStepKind::Evolution:     return "Evolution";
    case FeatureStepKind::Joint:         return "Joint";
    case FeatureStepKind::Import:        return "Import";
    case FeatureStepKind::Flip:          return "Flip";
    case FeatureStepKind::Imprint:       return "Imprint";
    }
    return "?";
}

const char* FeatureCombinationLabel(FeatureCombination Combination) noexcept
{
    switch (Combination)
    {
    case FeatureCombination::New:          return "New";
    case FeatureCombination::Union:        return "Union";
    case FeatureCombination::Difference:   return "Difference";
    case FeatureCombination::Intersection: return "Intersection";
    case FeatureCombination::Separate:     return "New body";
    }
    return "?";
}

} // namespace Frontier::GeometryProcessing
