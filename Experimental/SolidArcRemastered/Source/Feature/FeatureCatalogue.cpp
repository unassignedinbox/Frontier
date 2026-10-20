/*====================================================================================================================================
                                                      FEATURECATALOGUE.CPP
====================================================================================================================================*/
// 🧩 Feature-tree specimens with closed-form volumes, parameter sweeps and expected refusals

#include "Feature/FeatureCatalogue.h"

#include "Sketch/SketchCatalogue.h"

#include <cmath>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    using Kind = SketchConstraintKind;
    constexpr double Pi = std::numbers::pi;

    struct TreeAuthoring
    {
        FeatureTree& Tree;

        double Value(const std::string& Name) const { return FeatureParameterValue(Tree, Name); }

        void Parameter(const std::string& Name, double Value, double Low, double High, const std::string& Description)
        {
            Tree.Parameters.push_back({ Name, Value, Low, High, Description });
        }

        // A dimension bound to a parameter (its literal value is the parameter's current value).
        void Bound(SketchModel& Sketch, Kind Constraint, int First, int Second, const std::string& Name)
        {
            const int Index = AppendSketchConstraint(Sketch, Constraint, First, Second, -1, Value(Name), Name);
            Sketch.Constraints[Index].Parameter = Name;
        }

        FeatureStep& Step(FeatureStepKind StepKind, const std::string& Label)
        {
            FeatureStep Step;
            Step.Kind  = StepKind;
            Step.Label = Label;
            Tree.Steps.push_back(std::move(Step));
            return Tree.Steps.back();
        }

        // Width × Depth rectangle at the origin, lines 1…4 = bottom (y = 0), right (x = W), top, left.
        FeatureStep& PlateSketch(const std::string& Label, const std::string& Width, const std::string& Depth)
        {
            FeatureStep& Sketch = Step(FeatureStepKind::Sketch, Label);
            const SketchRectangle Rectangle = AppendSketchRectangle(Sketch.Sketch, { 0.0, 0.0 }, { Value(Width), Value(Depth) }, 1);
            AppendSketchConstraint(Sketch.Sketch, Kind::Fixed, Rectangle.FirstPoint, -1, -1, 0.0, "Origin", 0.0);
            Bound(Sketch.Sketch, Kind::HorizontalDistance, Rectangle.FirstPoint, Rectangle.FirstPoint + 1, Width);
            Bound(Sketch.Sketch, Kind::VerticalDistance, Rectangle.FirstPoint, Rectangle.FirstPoint + 3, Depth);
            return Sketch;
        }

        // Circle 1 placed from a fixed origin point by (X, Y) and its radius.
        FeatureStep& CircleSketch(const std::string& Label, const std::string& X, const std::string& Y, const std::string& Radius)
        {
            FeatureStep& Sketch = Step(FeatureStepKind::Sketch, Label);
            const int Origin = AppendSketchPoint(Sketch.Sketch, { 0.0, 0.0 });
            const int Centre = AppendSketchPoint(Sketch.Sketch, { Value(X), Value(Y) });
            const int Circle = AppendSketchCircle(Sketch.Sketch, Centre, Value(Radius), 1);
            AppendSketchConstraint(Sketch.Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
            Bound(Sketch.Sketch, Kind::HorizontalDistance, Origin, Centre, X);
            Bound(Sketch.Sketch, Kind::VerticalDistance, Origin, Centre, Y);
            Bound(Sketch.Sketch, Kind::Radius, Circle, -1, Radius);
            return Sketch;
        }

        FeatureStep& Sweep(FeatureStepKind StepKind, const std::string& Label, const std::string& Profile, FeatureValue Extent, FeatureCombination Combination)
        {
            FeatureStep& Swept = Step(StepKind, Label);
            Swept.Profile     = Profile;
            Swept.Extent      = Extent;
            Swept.Combination = Combination;
            return Swept;
        }

        FeatureStep& Blend(const std::string& Label, BlendProfile Shape, const std::string& Size, const std::vector<std::string>& Edges)
        {
            FeatureStep& Blended = Step(FeatureStepKind::Blend, Label);
            Blended.Shape = Shape;
            Blended.Size  = ParameterValue(Size);
            for (const std::string& Name : Edges) Blended.References.push_back({ ReferenceKind::Edge, Name, {} });
            return Blended;
        }

        FeatureStep& Change(const std::string& Label, FaceOperationKind Operation, const std::string& Amount, const std::vector<std::string>& Faces)
        {
            FeatureStep& Changed = Step(FeatureStepKind::FaceOperation, Label);
            Changed.Operation = Operation;
            Changed.Amount    = ParameterValue(Amount);
            for (const std::string& Name : Faces) Changed.References.push_back({ ReferenceKind::Face, Name, {} });
            return Changed;
        }
    };

    [[nodiscard]] std::vector<std::string> NoRefusals(const FeatureTree&) { return {}; }

    // Pappus: ring of a right-isosceles chamfer triangle (legs c) at radius R, convex (−) or concave (+) side.
    [[nodiscard]] double ChamferRing(double Radius, double Leg, double Side) { return Pi * Leg * Leg * (Radius + Side * Leg / 3.0); }

    // Pappus: concave fillet spandrel (area r²(1 − π/4), centroid x̄ from the corner) revolved at radius R.
    [[nodiscard]] double FilletSpandrelRing(double Radius, double Size)
    {
        const double Centroid = Size * (10.0 - 3.0 * Pi) / (3.0 * (4.0 - Pi));
        return 2.0 * Pi * (Radius + Centroid) * Size * Size * (1.0 - Pi / 4.0);
    }

    // ① Block with a filleted top-front edge: the bevel follows the sketch (plan gap G1).
    FeatureSpecimen BuildBlockFillet()
    {
        FeatureSpecimen Specimen;
        FeatureTree& Tree = Specimen.Tree;
        Tree.Name        = "BlockFillet";
        Tree.Description = "sketch → extrude → fillet the top-front edge; the bevel follows Width / Depth / Height";
        TreeAuthoring Author{ Tree };
        Author.Parameter("Width", 3.0, 1.5, 6.0, "block length along x");
        Author.Parameter("Depth", 2.0, 1.0, 3.0, "block depth along y");
        Author.Parameter("Height", 1.0, 0.5, 2.0, "extrude distance");
        Author.Parameter("Radius", 0.25, 0.05, 0.45, "fillet radius");
        Author.PlateSketch("Sketch#1", "Width", "Depth");
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#2", "Sketch#1", ParameterValue("Height"), FeatureCombination::New);
        Author.Blend("Blend#3", BlendProfile::Fillet, "Radius", { "Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))" });
        Specimen.Sweep = { {}, { { "Width", 4.5 } }, { { "Width", 6.0 }, { "Radius", 0.4 } }, { { "Depth", 1.2 }, { "Height", 0.6 }, { "Radius", 0.1 } },
                           { { "Width", 1.5 }, { "Depth", 3.0 }, { "Height", 2.0 }, { "Radius", 0.45 } } };
        Specimen.ExpectedVolume = [](const FeatureTree& Tree)
        {
            const double W = FeatureParameterValue(Tree, "Width"), D = FeatureParameterValue(Tree, "Depth");
            const double H = FeatureParameterValue(Tree, "Height"), R = FeatureParameterValue(Tree, "Radius");
            return W * D * H - (1.0 - Pi / 4.0) * R * R * W;
        };
        Specimen.ExpectedRefusals = NoRefusals;
        Specimen.Oracle           = "W·D·H − (1 − π/4)·r²·W";
        return Specimen;
    }

    // ② Plate with a boss united on top and a concave fillet at the boss root.
    FeatureSpecimen BuildBossFillet()
    {
        FeatureSpecimen Specimen;
        FeatureTree& Tree = Specimen.Tree;
        Tree.Name        = "BossFillet";
        Tree.Description = "plate ∪ cylindrical boss, concave fillet at the root circle (Boolean history → blend reference)";
        TreeAuthoring Author{ Tree };
        Author.Parameter("Width", 3.0, 2.0, 5.0, "plate length");
        Author.Parameter("Depth", 2.0, 1.5, 3.0, "plate depth");
        Author.Parameter("Thickness", 0.5, 0.3, 1.0, "plate thickness");
        Author.Parameter("BossX", 1.5, 0.8, 2.2, "boss centre x");
        Author.Parameter("BossY", 1.0, 0.8, 1.2, "boss centre y");
        Author.Parameter("BossRadius", 0.5, 0.3, 0.7, "boss radius");
        Author.Parameter("BossHeight", 0.8, 0.4, 1.5, "boss height above the plate");
        Author.Parameter("Radius", 0.15, 0.05, 0.25, "root fillet radius");
        Author.PlateSketch("Sketch#1", "Width", "Depth");
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#2", "Sketch#1", ParameterValue("Thickness"), FeatureCombination::New);
        FeatureStep& Boss = Author.CircleSketch("Sketch#3", "BossX", "BossY", "BossRadius");
        Boss.Elevation = ParameterValue("Thickness", 0.5);                                   // starts inside the plate: no coplanar caps
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#4", "Sketch#3", SumValue("BossHeight", 1.0, "Thickness", 0.5), FeatureCombination::Union);
        Author.Blend("Blend#5", BlendProfile::Fillet, "Radius", { "Edge(Extrude#2.Cap(End)|Extrude#4.Side(Circle1))" });
        Specimen.Sweep = { {}, { { "BossRadius", 0.7 } }, { { "BossX", 1.0 }, { "BossHeight", 1.5 } }, { { "Thickness", 1.0 }, { "Radius", 0.25 } },
                           { { "Width", 5.0 }, { "BossX", 2.2 }, { "Radius", 0.05 } } };
        Specimen.ExpectedVolume = [](const FeatureTree& Tree)
        {
            const double W = FeatureParameterValue(Tree, "Width"), D = FeatureParameterValue(Tree, "Depth"), T = FeatureParameterValue(Tree, "Thickness");
            const double R = FeatureParameterValue(Tree, "BossRadius"), H = FeatureParameterValue(Tree, "BossHeight"), r = FeatureParameterValue(Tree, "Radius");
            return W * D * T + Pi * R * R * H + FilletSpandrelRing(R, r);
        };
        Specimen.ExpectedRefusals = NoRefusals;
        Specimen.Oracle           = "W·D·T + π·R²·H + 2π(R + x̄)·r²(1 − π/4), x̄ = r(10 − 3π)/(3(4 − π))";
        return Specimen;
    }

    // ③ Revolved stepped shaft with a chamfered end.
    FeatureSpecimen BuildSteppedShaft()
    {
        FeatureSpecimen Specimen;
        FeatureTree& Tree = Specimen.Tree;
        Tree.Name        = "SteppedShaft";
        Tree.Description = "half-section sketch revolved about its own closing line, chamfer on the small end";
        TreeAuthoring Author{ Tree };
        Author.Parameter("MainRadius", 1.0, 0.7, 1.4, "large step radius");
        Author.Parameter("MainLength", 1.5, 0.8, 2.5, "large step length");
        Author.Parameter("PinRadius", 0.6, 0.3, 0.65, "small step radius");
        Author.Parameter("PinLength", 1.2, 0.5, 2.0, "small step length");
        Author.Parameter("Chamfer", 0.1, 0.03, 0.2, "end chamfer setback");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, "Sketch#1");
        SketchModel& Sketch  = Section.Sketch;
        const double R1 = Author.Value("MainRadius"), L1 = Author.Value("MainLength"), R2 = Author.Value("PinRadius"), L2 = Author.Value("PinLength");
        const int P0 = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int P1 = AppendSketchPoint(Sketch, { R1, 0.0 });
        const int P2 = AppendSketchPoint(Sketch, { R1, L1 });
        const int P3 = AppendSketchPoint(Sketch, { R2, L1 });
        const int P4 = AppendSketchPoint(Sketch, { R2, L1 + L2 });
        const int P5 = AppendSketchPoint(Sketch, { 0.0, L1 + L2 });
        const int Line1 = AppendSketchLine(Sketch, P0, P1, 1);
        const int Line2 = AppendSketchLine(Sketch, P1, P2, 2);
        const int Line3 = AppendSketchLine(Sketch, P2, P3, 3);
        const int Line4 = AppendSketchLine(Sketch, P3, P4, 4);
        const int Line5 = AppendSketchLine(Sketch, P4, P5, 5);
        const int Line6 = AppendSketchLine(Sketch, P5, P0, 6);
        AppendSketchConstraint(Sketch, Kind::Fixed, P0, -1, -1, 0.0, "Origin", 0.0);
        for (int Line : { Line1, Line3, Line5 }) AppendSketchConstraint(Sketch, Kind::Horizontal, Line);
        for (int Line : { Line2, Line4, Line6 }) AppendSketchConstraint(Sketch, Kind::Vertical, Line);
        Author.Bound(Sketch, Kind::HorizontalDistance, P0, P1, "MainRadius");
        Author.Bound(Sketch, Kind::VerticalDistance, P1, P2, "MainLength");
        Author.Bound(Sketch, Kind::HorizontalDistance, P0, P3, "PinRadius");
        Author.Bound(Sketch, Kind::VerticalDistance, P3, P4, "PinLength");
        FeatureStep& Revolve = Author.Sweep(FeatureStepKind::Revolve, "Revolve#2", "Sketch#1", LiteralValue(0.0), FeatureCombination::New);
        Revolve.AxisIdentifier = 6;
        Author.Blend("Blend#3", BlendProfile::Chamfer, "Chamfer", { "Edge(Revolve#2.Side(Line4)|Revolve#2.Side(Line5))" });
        Specimen.Sweep = { {}, { { "PinRadius", 0.4 } }, { { "MainLength", 2.5 }, { "PinLength", 0.5 } }, { { "MainRadius", 1.4 }, { "Chamfer", 0.2 } },
                           { { "MainRadius", 0.7 }, { "PinRadius", 0.65 }, { "Chamfer", 0.03 } } };
        Specimen.ExpectedVolume = [](const FeatureTree& Tree)
        {
            const double R1 = FeatureParameterValue(Tree, "MainRadius"), L1 = FeatureParameterValue(Tree, "MainLength");
            const double R2 = FeatureParameterValue(Tree, "PinRadius"), L2 = FeatureParameterValue(Tree, "PinLength"), C = FeatureParameterValue(Tree, "Chamfer");
            return Pi * R1 * R1 * L1 + Pi * R2 * R2 * L2 - ChamferRing(R2, C, -1.0);
        };
        Specimen.ExpectedRefusals = NoRefusals;
        Specimen.Oracle           = "π·R₁²·L₁ + π·R₂²·L₂ − π·c²(R₂ − c/3)";
        return Specimen;
    }

    // ④ Slot pocket cut into a plate, then its floor pushed up.
    FeatureSpecimen BuildSlotPocket()
    {
        FeatureSpecimen Specimen;
        FeatureTree& Tree = Specimen.Tree;
        Tree.Name        = "SlotPocket";
        Tree.Description = "plate − stadium pocket, pocket floor pushed (face reference through a Difference)";
        TreeAuthoring Author{ Tree };
        Author.Parameter("Width", 4.0, 3.0, 6.0, "plate length");
        Author.Parameter("Depth", 2.0, 1.5, 3.0, "plate depth");
        Author.Parameter("Thickness", 0.8, 0.5, 1.2, "plate thickness");
        Author.Parameter("SlotX", 1.0, 0.6, 1.5, "left slot centre x");
        Author.Parameter("SlotY", 1.0, 0.8, 1.2, "slot centre y");
        Author.Parameter("SlotLength", 2.0, 0.5, 2.5, "centre separation");
        Author.Parameter("SlotRadius", 0.4, 0.2, 0.5, "slot half width");
        Author.Parameter("PocketDepth", 0.5, 0.2, 0.6, "pocket depth");
        Author.Parameter("Push", 0.1, -0.1, 0.15, "floor push (positive raises it)");
        Author.PlateSketch("Sketch#1", "Width", "Depth");
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#2", "Sketch#1", ParameterValue("Thickness"), FeatureCombination::New);
        FeatureStep& Slot = Author.Step(FeatureStepKind::Sketch, "Sketch#3");
        Slot.Elevation = ParameterValue("Thickness", 1.0, 0.5);                              // tool starts above the plate
        SketchModel& Sketch = Slot.Sketch;
        const double X = Author.Value("SlotX"), Y = Author.Value("SlotY"), L = Author.Value("SlotLength"), R = Author.Value("SlotRadius");
        const int Origin      = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int LeftCentre  = AppendSketchPoint(Sketch, { X, Y });
        const int RightCentre = AppendSketchPoint(Sketch, { X + L, Y });
        const int BottomLeft  = AppendSketchPoint(Sketch, { X, Y - R });
        const int BottomRight = AppendSketchPoint(Sketch, { X + L, Y - R });
        const int TopRight    = AppendSketchPoint(Sketch, { X + L, Y + R });
        const int TopLeft     = AppendSketchPoint(Sketch, { X, Y + R });
        const int Bottom      = AppendSketchLine(Sketch, BottomLeft, BottomRight, 1);
        const int RightArc    = AppendSketchArc(Sketch, RightCentre, BottomRight, TopRight, 2);
        const int Top         = AppendSketchLine(Sketch, TopRight, TopLeft, 3);
        const int LeftArc     = AppendSketchArc(Sketch, LeftCentre, TopLeft, BottomLeft, 4);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, LeftCentre, "SlotX");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, LeftCentre, "SlotY");
        Author.Bound(Sketch, Kind::HorizontalDistance, LeftCentre, RightCentre, "SlotLength");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, LeftCentre, RightCentre, -1, 0.0, "Level");
        Author.Bound(Sketch, Kind::Radius, LeftArc, -1, "SlotRadius");
        AppendSketchConstraint(Sketch, Kind::EqualRadius, LeftArc, RightArc, -1, 0.0, "EqualRadius");
        AppendSketchConstraint(Sketch, Kind::Tangent, Bottom, LeftArc, -1, 0.0, "T1");
        AppendSketchConstraint(Sketch, Kind::Tangent, Bottom, RightArc, -1, 0.0, "T2");
        AppendSketchConstraint(Sketch, Kind::Tangent, Top, RightArc, -1, 0.0, "T3");
        AppendSketchConstraint(Sketch, Kind::Tangent, Top, LeftArc, -1, 0.0, "T4");
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#4", "Sketch#3", ParameterValue("PocketDepth", -1.0, -0.5), FeatureCombination::Difference);
        Author.Change("FaceOperation#5", FaceOperationKind::Push, "Push", { "Extrude#4.Cap(End)" });
        Specimen.Sweep = { {}, { { "SlotLength", 0.5 } }, { { "SlotRadius", 0.2 }, { "Push", -0.1 } }, { { "PocketDepth", 0.2 }, { "Push", 0.15 } },
                           { { "Width", 6.0 }, { "SlotX", 1.5 }, { "SlotLength", 2.5 } } };
        Specimen.ExpectedVolume = [](const FeatureTree& Tree)
        {
            const double W = FeatureParameterValue(Tree, "Width"), D = FeatureParameterValue(Tree, "Depth"), T = FeatureParameterValue(Tree, "Thickness");
            const double L = FeatureParameterValue(Tree, "SlotLength"), R = FeatureParameterValue(Tree, "SlotRadius");
            const double P = FeatureParameterValue(Tree, "PocketDepth"), Push = FeatureParameterValue(Tree, "Push");
            return W * D * T - (2.0 * R * L + Pi * R * R) * (P - Push);
        };
        Specimen.ExpectedRefusals = NoRefusals;
        Specimen.Oracle           = "W·D·T − (2rL + πr²)(depth − push)";
        return Specimen;
    }

    // ⑤ Torus from a revolved circle about a construction axis.
    FeatureSpecimen BuildTorusRing()
    {
        FeatureSpecimen Specimen;
        FeatureTree& Tree = Specimen.Tree;
        Tree.Name        = "TorusRing";
        Tree.Description = "circle revolved about a construction line: one toroidal face at a full turn, end caps below it";
        TreeAuthoring Author{ Tree };
        Author.Parameter("Major", 2.0, 0.8, 3.0, "axis to circle centre");
        Author.Parameter("Minor", 0.5, 0.2, 0.7, "tube radius");
        Author.Parameter("Turn", 2.0 * Pi, 0.3, 2.0 * Pi, "revolve angle [rad] (< 2π: partial torus with end caps)");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, "Sketch#1");
        SketchModel& Sketch  = Section.Sketch;
        const int AxisStart = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int AxisEnd   = AppendSketchPoint(Sketch, { 0.0, 1.0 });
        const int Centre    = AppendSketchPoint(Sketch, { Author.Value("Major"), 0.0 });
        AppendSketchLine(Sketch, AxisStart, AxisEnd, 1, true);
        const int Circle = AppendSketchCircle(Sketch, Centre, Author.Value("Minor"), 2);
        AppendSketchConstraint(Sketch, Kind::Fixed, AxisStart, -1, -1, 0.0, "AxisStart", 0.0);
        AppendSketchConstraint(Sketch, Kind::Fixed, AxisEnd, -1, -1, 0.0, "AxisEnd", 1.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, AxisStart, Centre, "Major");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, AxisStart, Centre, -1, 0.0, "Level");
        Author.Bound(Sketch, Kind::Radius, Circle, -1, "Minor");
        FeatureStep& Revolve = Author.Sweep(FeatureStepKind::Revolve, "Revolve#2", "Sketch#1", LiteralValue(0.0), FeatureCombination::New);
        Revolve.AxisIdentifier = 1;
        Revolve.Angle          = ParameterValue("Turn");
        Specimen.Sweep = { {}, { { "Major", 0.8 }, { "Minor", 0.7 } }, { { "Major", 3.0 }, { "Minor", 0.2 } }, { { "Turn", 0.5 * Pi } }, { { "Turn", 0.3 }, { "Major", 0.8 } } };
        Specimen.ExpectedVolume = [](const FeatureTree& Tree)
        {
            const double R = FeatureParameterValue(Tree, "Major"), r = FeatureParameterValue(Tree, "Minor");
            return FeatureParameterValue(Tree, "Turn") * Pi * R * r * r;
        };
        Specimen.ExpectedRefusals = NoRefusals;
        Specimen.Oracle           = "Φ·π·R·r² (Pappus; 2π²·R·r² at a full turn)";
        return Specimen;
    }

    // ⑥ Drilled + chamfered hole that a parameter moves off the plate: the chamfer loses its edge and is refused,
    //    the previous body is kept, and the later push still builds.
    FeatureSpecimen BuildHoleLeavesPlate()
    {
        FeatureSpecimen Specimen;
        FeatureTree& Tree = Specimen.Tree;
        Tree.Name        = "HoleLeavesPlate";
        Tree.Description = "plate − hole, chamfer on the hole rim, push the right side; HoleX = 5 drops the hole off the plate";
        TreeAuthoring Author{ Tree };
        Author.Parameter("Width", 3.0, 2.0, 4.0, "plate length");
        Author.Parameter("Depth", 2.0, 1.5, 3.0, "plate depth");
        Author.Parameter("Thickness", 0.6, 0.4, 1.0, "plate thickness");
        Author.Parameter("HoleX", 1.5, 0.6, 5.0, "hole centre x (≥ Width + r: off the plate)");
        Author.Parameter("HoleY", 1.0, 0.6, 1.4, "hole centre y");
        Author.Parameter("HoleRadius", 0.45, 0.2, 0.55, "hole radius");
        Author.Parameter("Chamfer", 0.1, 0.03, 0.15, "rim chamfer setback");
        Author.Parameter("Grow", 0.2, -0.2, 0.4, "push of the right side");
        Author.PlateSketch("Sketch#1", "Width", "Depth");
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#2", "Sketch#1", ParameterValue("Thickness"), FeatureCombination::New);
        FeatureStep& Hole = Author.CircleSketch("Sketch#3", "HoleX", "HoleY", "HoleRadius");
        Hole.Elevation = LiteralValue(-0.5);
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#4", "Sketch#3", ParameterValue("Thickness", 1.0, 1.0), FeatureCombination::Difference);
        Author.Blend("Blend#5", BlendProfile::Chamfer, "Chamfer", { "Edge(Extrude#2.Cap(End)|Extrude#4.Side(Circle1))" });
        Author.Change("FaceOperation#6", FaceOperationKind::Push, "Grow", { "Extrude#2.Side(Line2)" });
        Specimen.Sweep = { {}, { { "HoleX", 0.8 }, { "HoleRadius", 0.55 } }, { { "Chamfer", 0.15 }, { "Grow", -0.2 } }, { { "HoleX", 5.0 } },
                           { { "HoleX", 5.0 }, { "Grow", 0.4 } } };
        Specimen.ExpectedVolume = [](const FeatureTree& Tree)
        {
            const double W = FeatureParameterValue(Tree, "Width"), D = FeatureParameterValue(Tree, "Depth"), T = FeatureParameterValue(Tree, "Thickness");
            const double X = FeatureParameterValue(Tree, "HoleX"), R = FeatureParameterValue(Tree, "HoleRadius");
            const double C = FeatureParameterValue(Tree, "Chamfer"), G = FeatureParameterValue(Tree, "Grow");
            if (X - R >= W) return (W + G) * D * T;
            return ((W + G) * D - Pi * R * R) * T - ChamferRing(R, C, 1.0);
        };
        Specimen.ExpectedRefusals = [](const FeatureTree& Tree) -> std::vector<std::string>
        {
            if (FeatureParameterValue(Tree, "HoleX") - FeatureParameterValue(Tree, "HoleRadius") >= FeatureParameterValue(Tree, "Width")) return { "Blend#5" };
            return {};
        };
        Specimen.Oracle = "((W + g)·D − π·r²)·T − π·c²(r + c/3); hole off the plate: (W + g)·D·T with Blend#5 refused";
        return Specimen;
    }

    // ⑦ A through-slot that splits the referenced top face: missing (exact), off-centre (geometric rebind to the
    //    larger piece; the small piece fails the size test), centred (two equal pieces → ambiguous → refused).
    FeatureSpecimen BuildSplitFacePush()
    {
        FeatureSpecimen Specimen;
        FeatureTree& Tree = Specimen.Tree;
        Tree.Name        = "SplitFacePush";
        Tree.Description = "block, groove across y at GrooveX, push the top face; the groove splits the referenced face";
        TreeAuthoring Author{ Tree };
        Author.Parameter("Width", 3.0, 2.0, 4.0, "block length");
        Author.Parameter("Depth", 2.0, 1.0, 3.0, "block depth");
        Author.Parameter("Height", 1.0, 0.6, 1.5, "block height");
        Author.Parameter("GrooveX", -1.0, -1.0, 3.5, "groove centre x (−1: misses the block)");
        Author.Parameter("GrooveHalfWidth", 0.2, 0.1, 0.3, "half groove width");
        Author.Parameter("GrooveDepth", 0.4, 0.2, 0.5, "groove depth");
        Author.Parameter("Push", 0.2, -0.2, 0.4, "top face push");
        Author.PlateSketch("Sketch#1", "Width", "Depth");
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#2", "Sketch#1", ParameterValue("Height"), FeatureCombination::New);
        FeatureStep& Groove = Author.Step(FeatureStepKind::Sketch, "Sketch#3");
        Groove.Elevation = ParameterValue("Height", 1.0, 0.5);
        SketchModel& Sketch = Groove.Sketch;
        const double X = Author.Value("GrooveX"), Half = Author.Value("GrooveHalfWidth");
        const int Origin = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Centre = AppendSketchPoint(Sketch, { X, 0.0 });
        const SketchRectangle Rectangle = AppendSketchRectangle(Sketch, { X - Half, -0.5 }, { X + Half, 4.5 }, 1);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Centre, "GrooveX");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, Origin, Centre, -1, 0.0, "Level");
        Author.Bound(Sketch, Kind::HorizontalDistance, Rectangle.FirstPoint, Centre, "GrooveHalfWidth");
        Author.Bound(Sketch, Kind::HorizontalDistance, Centre, Rectangle.FirstPoint + 1, "GrooveHalfWidth");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, Origin, Rectangle.FirstPoint, -1, -0.5, "Below");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, Rectangle.FirstPoint, Rectangle.FirstPoint + 3, -1, 5.0, "Span");
        Author.Sweep(FeatureStepKind::Extrude, "Extrude#4", "Sketch#3", ParameterValue("GrooveDepth", -1.0, -0.5), FeatureCombination::Difference);
        Author.Change("FaceOperation#5", FaceOperationKind::Push, "Push", { "Extrude#2.Cap(End)" });
        Specimen.Sweep = { {}, { { "Push", -0.2 } }, { { "GrooveX", 0.5 } }, { { "GrooveX", 1.5 } }, { { "GrooveX", 2.5 }, { "Push", 0.4 } } };
        Specimen.ExpectedVolume = [](const FeatureTree& Tree)
        {
            const double W = FeatureParameterValue(Tree, "Width"), D = FeatureParameterValue(Tree, "Depth"), H = FeatureParameterValue(Tree, "Height");
            const double X = FeatureParameterValue(Tree, "GrooveX"), Half = FeatureParameterValue(Tree, "GrooveHalfWidth");
            const double S = FeatureParameterValue(Tree, "GrooveDepth"), P = FeatureParameterValue(Tree, "Push");
            if (X + Half <= 0.0 || X - Half >= W) return W * D * (H + P);
            const double Left = X - Half, Right = W - X - Half, Groove = 2.0 * Half * D * S;
            if (std::abs(Left - Right) < 1e-12 * W) return W * D * H - Groove;                // ambiguous: push refused
            return W * D * H - Groove + P * std::max(Left, Right) * D;
        };
        Specimen.ExpectedRefusals = [](const FeatureTree& Tree) -> std::vector<std::string>
        {
            const double W = FeatureParameterValue(Tree, "Width"), X = FeatureParameterValue(Tree, "GrooveX");
            if (std::abs(2.0 * X - W) < 1e-12 * W) return { "FaceOperation#5" };
            return {};
        };
        Specimen.Oracle = "groove misses: W·D·(H + p); off-centre: W·D·H − 2h·D·s + p·max(piece)·D; centred: push refused";
        return Specimen;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

std::vector<std::string> EnumerateFeatureSpecimens()
{
    return { "BlockFillet", "BossFillet", "SteppedShaft", "SlotPocket", "TorusRing", "HoleLeavesPlate", "SplitFacePush" };
}

FeatureSpecimen BuildFeatureSpecimen(const std::string& Name)
{
    if (Name == "BlockFillet") return BuildBlockFillet();
    if (Name == "BossFillet") return BuildBossFillet();
    if (Name == "SteppedShaft") return BuildSteppedShaft();
    if (Name == "SlotPocket") return BuildSlotPocket();
    if (Name == "TorusRing") return BuildTorusRing();
    if (Name == "HoleLeavesPlate") return BuildHoleLeavesPlate();
    if (Name == "SplitFacePush") return BuildSplitFacePush();
    return {};
}

bool AssignFeatureParameters(FeatureTree& Tree, const FeatureAssignment& Assignment)
{
    bool AllCondition = true;
    for (const auto& [Name, Value] : Assignment) AllCondition = AssignFeatureParameter(Tree, Name, Value) && AllCondition;
    return AllCondition;
}

double FeatureParameterValue(const FeatureTree& Tree, const std::string& Name)
{
    for (const FeatureParameter& Parameter : Tree.Parameters) if (Parameter.Name == Name) return Parameter.Value;
    return std::nan("");
}

} // namespace Frontier::SolidArcRemastered
