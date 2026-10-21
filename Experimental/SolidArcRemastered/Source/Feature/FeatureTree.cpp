/*====================================================================================================================================
                                                        FEATURETREE.CPP
====================================================================================================================================*/
// 🧩 Parametric feature tree: parameters, sketches, sweeps, Booleans, blends, face operations; regeneration by replay

#include "Feature/FeatureTree.h"

#include "Sketch/SketchProfile.h"
#include "Surfacing/CurveNetwork.h"
#include "Surfacing/PanelAssembly.h"
#include "Surfacing/SectionLoft.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Frontier::SolidArcRemastered
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

RegenerationOutcome RegenerateFeatureTree(ModelPartition& Partition, FeatureTree& Tree, bool CaptureCondition)
{
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

    for (FeatureStep& Step : Tree.Steps)
    {
        const auto StepStarted = Clock::now();
        FeatureStepReport Report;
        Report.Label = Step.Label;
        Report.Kind  = Step.Kind;
        auto Refuse = [&](const std::string& Reason) { Report.Status = FeatureStepStatus::Refused; Report.Refusal = Reason; };
        const char* Verb = FeatureStepKindLabel(Step.Kind);
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
            Solved.Plane = Step.Plane;
            Solved.Plane.Origin = Step.Plane.Origin + Step.Plane.ZAxis * EvaluateFeatureValue(Tree, Step.Elevation);
            if (BoundCondition)
            {
                Report.Solve = SolveSketch(Solved.Model);
                if (!Report.Solve.ConflictGroups.empty()) Refuse("Sketch: conflicting constraints " + ConflictText(Solved.Model, Report.Solve));
                else if (!Report.Solve.Converged) Refuse("Sketch: no solution reached (max residual " + std::to_string(Report.Solve.MaxResidual) + ")");
                else Solved.UsableCondition = true;
            }
            Outcome.Sketches[Step.Label] = Solved.Model;
            Sketches[Step.Label] = Solved;
            break;
        }
        case FeatureStepKind::Extrude:
        case FeatureStepKind::Revolve:
        {
            // ② Sweep the profile into a tool body named by role, then combine.
            const auto Found = Sketches.find(Step.Profile);
            if (Found == Sketches.end() || !Found->second.UsableCondition) { Refuse(std::string(Verb) + ": profile sketch " + Step.Profile + " is unavailable"); break; }
            const SolvedSketch& Solved  = Found->second;
            const SketchProfile Profile = ExtractSketchProfile(Solved.Model);
            SweepOutcome Sweep;
            if (Step.Kind == FeatureStepKind::Extrude) Sweep = ExtrudeProfile(Partition, Profile, Solved.Plane, EvaluateFeatureValue(Tree, Step.Extent));
            else
            {
                const SketchCurve* Axis = nullptr;
                for (const SketchCurve& Curve : Solved.Model.Curves) if (Curve.Kind == SketchCurveKind::Line && Curve.Identifier == Step.AxisIdentifier) Axis = &Curve;
                if (!Axis) { Refuse("Revolve: axis line " + std::to_string(Step.AxisIdentifier) + " not in sketch " + Step.Profile); break; }
                // The axis may close the profile loop itself (its segments sweep nothing) or be a construction line.
                const Vector2 Start = Solved.Model.Points[Axis->Start].Position, End = Solved.Model.Points[Axis->End].Position;
                Sweep = RevolveProfile(Partition, Profile, Solved.Plane, Start, End - Start, EvaluateFeatureValue(Tree, Step.Angle));
            }
            if (!Sweep.Refusal.empty()) { Refuse(Sweep.Refusal); break; }
            const BodyNaming ToolNaming = NameSweptBody(Partition, Sweep, Step.Label);
            // A separate sweep (Plasticity "new body"), or the first body of the part, starts a body of its own.
            const bool SeparateCondition = Step.Combination == FeatureCombination::Separate;
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
            std::vector<uint32_t> Slots;
            for (TopologyReference& Reference : Step.References)
            {
                const ResolvedReference Resolved = ResolveReference(Partition, Current, Naming, Reference);
                Report.Resolutions.push_back(Resolved);
                if (Resolved.Method == ResolutionMethod::Refused) continue;
                Slots.push_back(Resolved.Slot);
                if (CaptureCondition) Reference.Signature = CaptureSignature(Partition, Reference.Kind, Resolved.Slot);
            }
            // ③′ A variable blend's anchors (the vertices taking Size) resolve like the edges, so a regeneration that
            //    reverses an edge keeps each size at its vertex.
            std::vector<VertexToken> Anchors;
            if (Step.Kind == FeatureStepKind::Blend && Step.EndSize)
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
                if (Step.EndSize)
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
            FaceOperationSettings Settings;
            Settings.Kind = Step.Operation;
            for (uint32_t Slot : Slots) Settings.Faces.push_back(Partition.Faces.TokenAt(Slot));
            ApplySettingsAmount(Settings, Amount);
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
            CurveFitOutcome Fit = FitSpaceCurve(Points, Settings);
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
            // ⑤ Skin a sheet through the named SpaceCurve results, in order.
            std::vector<NurbsCurve> Sections;
            for (const std::string& Section : Step.Sections)
            {
                const auto Found = Outcome.Curves.find(Section);
                if (Found == Outcome.Curves.end()) { Refuse("Loft: section " + Section + " is unavailable"); break; }
                Sections.push_back(Found->second.Curve);
            }
            if (Report.Status == FeatureStepStatus::Refused) break;
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
            Bodies.erase(Bodies.begin() + ToolIndex);
            for (int* Index : { &Target, &Active, &PanelBody })
            {
                if (*Index == ToolIndex) *Index = -1;
                else if (*Index > ToolIndex) --*Index;
            }
            break;
        }
        }
        Report.Body         = Current;
        Report.Naming       = Naming;
        Report.Target       = Target >= 0 ? Bodies[Target].Key : std::string();
        Report.Milliseconds = std::chrono::duration<double, std::milli>(Clock::now() - StepStarted).count();
        Outcome.Refused    += Report.Status == FeatureStepStatus::Refused ? 1 : 0;
        Outcome.Steps.push_back(std::move(Report));
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

} // namespace Frontier::SolidArcRemastered
