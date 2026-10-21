/*====================================================================================================================================
                                                        FEATURETREE.CPP
====================================================================================================================================*/
// 🧩 Parametric feature tree: parameters, sketches, sweeps, Booleans, blends, face operations; regeneration by replay

#include "Feature/FeatureTree.h"

#include "Sketch/SketchProfile.h"

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
    BodyToken  Current;
    BodyNaming Naming;

    for (FeatureStep& Step : Tree.Steps)
    {
        const auto StepStarted = Clock::now();
        FeatureStepReport Report;
        Report.Label = Step.Label;
        Report.Kind  = Step.Kind;
        auto Refuse = [&](const std::string& Reason) { Report.Status = FeatureStepStatus::Refused; Report.Refusal = Reason; };
        const char* Verb = FeatureStepKindLabel(Step.Kind);

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
            if (!Current.PresentCondition())
            {
                if (Step.Combination != FeatureCombination::New && Step.Combination != FeatureCombination::Union) { Refuse(std::string(Verb) + ": nothing to " + FeatureCombinationLabel(Step.Combination) + " with"); break; }
                Current = Sweep.Body;
                Naming  = ToolNaming;
                break;
            }
            const BooleanOperation Operation = Step.Combination == FeatureCombination::Difference ? BooleanOperation::Difference
                                             : Step.Combination == FeatureCombination::Intersection ? BooleanOperation::Intersection : BooleanOperation::Union;
            const BooleanOutcome Boolean = ComputeBoolean(Partition, Current, Sweep.Body, Operation);
            if (!Boolean.Refusal.empty()) { Refuse(Boolean.Refusal); break; }
            Naming  = NameBooleanBody(Partition, Boolean.Body, Boolean, Naming, ToolNaming);
            Current = Boolean.Body;
            break;
        }
        case FeatureStepKind::Blend:
        case FeatureStepKind::FaceOperation:
        {
            // ③ Resolve references on the current body; any refusal refuses the step (never a partial selection).
            if (!Current.PresentCondition()) { Refuse(std::string(Verb) + ": no body yet"); break; }
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
            break;
        }
        }
        Report.Body         = Current;
        Report.Naming       = Naming;
        Report.Milliseconds = std::chrono::duration<double, std::milli>(Clock::now() - StepStarted).count();
        Outcome.Refused    += Report.Status == FeatureStepStatus::Refused ? 1 : 0;
        Outcome.Steps.push_back(std::move(Report));
    }
    Outcome.Body         = Current;
    Outcome.Naming       = Naming;
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
    }
    return "?";
}

} // namespace Frontier::SolidArcRemastered
