/*====================================================================================================================================
                                                      BLENDCATALOGUE.CPP
====================================================================================================================================*/
// 🧩 Blend specimens: bodies, edge selections, closed-form volume changes and membership oracles (validation + viewer)

#include "Blend/BlendCatalogue.h"

#include "Boolean/GeneralFuse.h"
#include "Construction/PrimitiveConstruction.h"

#include <algorithm>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

namespace
{
    constexpr double π = std::numbers::pi;                                                   // [rad] - π

    // 📝 Centroid factor of the fillet spandrel (square minus quarter disc): centroid k·r from the corner, k = (10 − 3π)/(12 − 3π).
    constexpr double SpandrelFactor = (10.0 - 3.0 * π) / (12.0 - 3.0 * π);                   // [-]

    [[nodiscard]] OrthonormalFrame UprightFrame(Vector3 Origin) { return { Origin, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }; }

    // Removed volume of a convex 90° fillet of radius r along a straight edge of length L: (1 − π/4) r² L.
    [[nodiscard]] double StraightFilletLoss(double Radius, double Span) { return (1.0 - π / 4.0) * Radius * Radius * Span; }

    // 📝 Top perimeter P of a box filleted with mitres: four prisms minus four corner overlaps ∫(r − √(r² − t²))² dt = r³(5/3 − π/2).
    [[nodiscard]] double MitredFilletLoss(double Radius, double Perimeter) { return StraightFilletLoss(Radius, Perimeter) - 4.0 * Radius * Radius * Radius * (5.0 / 3.0 - π / 2.0); }

    // Top perimeter chamfered with mitres: prisms d²/2 · P minus four corner overlaps d³/3.
    [[nodiscard]] double MitredChamferLoss(double Setback, double Perimeter) { return 0.5 * Setback * Setback * Perimeter - 4.0 * Setback * Setback * Setback / 3.0; }

    // 📝 Box a × b × c with all twelve edges filleted (sphere corners): V loses (4 − π) r² Σ(edge − 2r) + (8 − 4π/3) r³.
    [[nodiscard]] double RoundedBoxLoss(Vector3 Extent, double Radius)
    {
        return (4.0 - π) * Radius * Radius * ((Extent.X - 2.0 * Radius) + (Extent.Y - 2.0 * Radius) + (Extent.Z - 2.0 * Radius)) + (8.0 - 4.0 / 3.0 * π) * Radius * Radius * Radius;
    }

    // All twelve edges chamfered (triple mitres): prisms d²/2 · 4(a + b + c), minus 24 pair overlaps d³/3, plus 8 triple overlaps d³/4.
    [[nodiscard]] double ChamferedBoxLoss(Vector3 Extent, double Setback)
    {
        return 0.5 * Setback * Setback * 4.0 * (Extent.X + Extent.Y + Extent.Z) - 8.0 * Setback * Setback * Setback + 2.0 * Setback * Setback * Setback;
    }

    // Signed clearance of a box with all edges filleted: it is the inner box ⊕ ball(r).
    [[nodiscard]] std::function<double(Vector3)> RoundedBoxClearance(Vector3 Corner, Vector3 Extent, double Radius)
    {
        return [=](Vector3 Point)
        {
            const auto Axis = [&](double Value, double Low, double High) { return std::max({ Low + Radius - Value, 0.0, Value - (High - Radius) }); };
            const double Gap = std::sqrt(std::pow(Axis(Point.X, Corner.X, Corner.X + Extent.X), 2) + std::pow(Axis(Point.Y, Corner.Y, Corner.Y + Extent.Y), 2)
                                         + std::pow(Axis(Point.Z, Corner.Z, Corner.Z + Extent.Z), 2));
            if (Gap > 0.0) return Radius - Gap;
            return std::min({ Point.X - Corner.X, Corner.X + Extent.X - Point.X, Point.Y - Corner.Y, Corner.Y + Extent.Y - Point.Y, Point.Z - Corner.Z, Corner.Z + Extent.Z - Point.Z });
        };
    }

    // 📝 Box with all edges chamfered by d: the box cut by twelve half-spaces s₁ + s₂ ≥ d (s = inward face distances).
    [[nodiscard]] std::function<double(Vector3)> ChamferedBoxClearance(Vector3 Corner, Vector3 Extent, double Setback)
    {
        return [=](Vector3 Point)
        {
            const double Gaps[6] = { Point.X - Corner.X, Corner.X + Extent.X - Point.X, Point.Y - Corner.Y, Corner.Y + Extent.Y - Point.Y, Point.Z - Corner.Z, Corner.Z + Extent.Z - Point.Z };
            double Clearance = *std::min_element(Gaps, Gaps + 6);
            for (int First = 0; First < 6; ++First)
            {
                for (int Second = First + 1; Second < 6; ++Second)
                {
                    if (First / 2 == Second / 2) continue;                                    // Opposite faces share no edge
                    Clearance = std::min(Clearance, (Gaps[First] + Gaps[Second] - Setback) / std::sqrt(2.0));
                }
            }
            return Clearance;
        };
    }

    // Cylinder (axis Z through the origin, radius R, height h) with its top rim filleted by r.
    [[nodiscard]] std::function<double(Vector3)> RoundedRimClearance(double CylinderRadius, double Height, double Radius)
    {
        return [=](Vector3 Point)
        {
            const double Reach = std::hypot(Point.X, Point.Y);                                // [len] - ρ
            if (Reach > CylinderRadius - Radius && Point.Z > Height - Radius)
            {
                return Radius - std::hypot(Reach - (CylinderRadius - Radius), Point.Z - (Height - Radius));
            }
            return std::min({ CylinderRadius - Reach, Point.Z, Height - Point.Z });
        };
    }

    // ---- Cliff overflow closed forms ---------------------------------------------------------------------------------------
    // 📝 A convex blend wider than the face it rests on (depth t < blend reach) runs off the far edge and is trimmed by the
    //    cliff face. In the cross-section (p = depth from the consumed face, q = depth from the kept face) the removed
    //    region is the corner {p < r, q < t, outside the ball} for a fillet, {p + q < c, q < t} for a chamfer. With
    //    w = r − q: b(w) = r − √(r² − w²) is its width, A = ∫ b = r·t − ΔF, F(w) = ½(w√(r² − w²) + r² asin(w/r)).

    [[nodiscard]] double CliffCircleIntegral(double Radius, double Depth)
    {
        const auto Primitive = [&](double Level) { return 0.5 * (Level * std::sqrt(std::max(Radius * Radius - Level * Level, 0.0)) + Radius * Radius * std::asin(std::clamp(Level / Radius, -1.0, 1.0))); };
        return Primitive(Radius) - Primitive(Radius - Depth);                               // ∫_{r−t}^{r} √(r² − w²) dw
    }

    // Removed cross-section area: fillet r·t − ΔF, chamfer ½(c² − (c − t)²).
    [[nodiscard]] double CliffArea(double Size, double Depth, BlendProfile Profile)
    {
        if (Profile == BlendProfile::Chamfer) return 0.5 * (Size * Size - (Size - Depth) * (Size - Depth));
        return Size * Depth - CliffCircleIntegral(Size, Depth);
    }

    // ∫ b² over the removed cross-section (second moment along p): fillet 2r²t − 2rΔF − (r³ − (r − t)³)/3, chamfer (c³ − (c − t)³)/3.
    [[nodiscard]] double CliffSquareIntegral(double Size, double Depth, BlendProfile Profile)
    {
        const double Lower = Size - Depth;                                                   // [len] - r − t
        if (Profile == BlendProfile::Chamfer) return (Size * Size * Size - Lower * Lower * Lower) / 3.0;
        return 2.0 * Size * Size * Depth - 2.0 * Size * CliffCircleIntegral(Size, Depth) - (Size * Size * Size - Lower * Lower * Lower) / 3.0;
    }

    // Rim of revolution (outer radius R): Pappus over the removed region, ΔV = −2π ∫∫ (R − p) dA = −2π (R·A − ½∫b²).
    [[nodiscard]] double CliffRimLoss(double CylinderRadius, double Size, double Depth, BlendProfile Profile)
    {
        return 2.0 * π * (CylinderRadius * CliffArea(Size, Depth, Profile) - 0.5 * CliffSquareIntegral(Size, Depth, Profile));
    }

    // Corner term of the clearance in cross-section depths (p, q): negative inside the removed region.
    [[nodiscard]] double CliffCornerClearance(double Across, double Down, double Size, BlendProfile Profile)
    {
        if (Profile == BlendProfile::Chamfer) return (Across + Down - Size) / std::sqrt(2.0);
        if (Across < Size && Down < Size) return Size - std::hypot(Across - Size, Down - Size);
        return 1.0e300;
    }

    // Box [0, Extent] with its top front edge (y = 0, z = Extent.Z) blended past the front face.
    [[nodiscard]] std::function<double(Vector3)> CliffBoxClearance(Vector3 Extent, double Size, BlendProfile Profile)
    {
        return [=](Vector3 Point)
        {
            const double Box = std::min({ Point.X, Extent.X - Point.X, Point.Y, Extent.Y - Point.Y, Point.Z, Extent.Z - Point.Z });
            return std::min(Box, CliffCornerClearance(Point.Y, Extent.Z - Point.Z, Size, Profile));
        };
    }

    // Disc (axis Z, radius R, height t) with its top rim blended past the side face.
    [[nodiscard]] std::function<double(Vector3)> CliffDiscClearance(double CylinderRadius, double Height, double Size, BlendProfile Profile)
    {
        return [=](Vector3 Point)
        {
            const double Reach = std::hypot(Point.X, Point.Y);                                // [len] - ρ
            const double Disc  = std::min({ CylinderRadius - Reach, Point.Z, Height - Point.Z });
            return std::min(Disc, CliffCornerClearance(CylinderRadius - Reach, Height - Point.Z, Size, Profile));
        };
    }

    using EdgeTest = std::function<bool(Vector3, Vector3, const CurveGeometry&)>;

    [[nodiscard]] std::vector<EdgeToken> SelectEdges(const ModelPartition& Partition, BodyToken Body, const EdgeTest& Test)
    {
        std::vector<EdgeToken> Selection;
        for (EdgeToken Edge : Partition.CollectInventory(Body).Edges)
        {
            const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
            if (Test(Partition.Vertices.Resolve(Profile.Start).Position, Partition.Vertices.Resolve(Profile.End).Position, Partition.Curves.Resolve(Profile.Curve))) Selection.push_back(Edge);
        }
        return Selection;
    }

    [[nodiscard]] EdgeTest RimAt(double Height)
    {
        return [=](Vector3 Start, Vector3, const CurveGeometry& Curve) { return std::fabs(Start.Z - Height) < 1.0e-9 && !std::holds_alternative<StraightLine>(Curve); };
    }

    [[nodiscard]] EdgeTest TopEdges(double Height)
    {
        return [=](Vector3 Start, Vector3 End, const CurveGeometry& Curve) { return std::holds_alternative<StraightLine>(Curve) && std::fabs(Start.Z - Height) < 1.0e-9 && std::fabs(End.Z - Height) < 1.0e-9; };
    }

    [[nodiscard]] EdgeTest AnyEdge() { return [](Vector3, Vector3, const CurveGeometry&) { return true; }; }

    [[nodiscard]] BodyToken Fuse(ModelPartition& Partition, BodyToken First, BodyToken Second, BooleanOperation Operation)
    {
        return ComputeBoolean(Partition, First, Second, Operation).Body;
    }

    using SpecimenBuilder = std::function<BlendSpecimen(ModelPartition&)>;

    [[nodiscard]] BlendSpecimen Make(const char* Name, const char* Family, const char* Description, BodyToken Body, std::vector<EdgeToken> Edges, BlendProfile Profile, double Size,
                                     double Expected, int Faces = -1)
    {
        BlendSpecimen Specimen{ Name, Family, Description, Body, std::move(Edges), { Profile, Size }, Expected, Faces };
        Specimen.RefusalExpected = std::isnan(Expected) && Faces == 0;
        return Specimen;
    }

    [[nodiscard]] BlendSpecimen MakeRefusal(const char* Name, const char* Family, const char* Description, BodyToken Body, std::vector<EdgeToken> Edges, BlendProfile Profile, double Size)
    {
        BlendSpecimen Specimen = Make(Name, Family, Description, Body, std::move(Edges), Profile, Size, std::nan(""), 0);
        Specimen.RefusalExpected = true;
        return Specimen;
    }

    [[nodiscard]] const std::vector<std::pair<std::string, SpecimenBuilder>>& Specimens()
    {
        static const Vector3 BoxExtent = { 4.0, 3.0, 2.0 };
        static const std::vector<std::pair<std::string, SpecimenBuilder>> Table = {
            // ---- Harness: the old RollingBallBlend harness, same bodies, selections and sizes (r = 0.3 on the box).
            { "BoxEdgeFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                return Make("BoxEdgeFillet", "Harness", "One top edge, r 0.3: cylinder strip, two circular caps", Body,
                            SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 1.99 && B.Z > 1.99; }),
                            BlendProfile::Fillet, 0.3, -StraightFilletLoss(0.3, 4.0), 7);
            } },
            { "BoxEdgeChamfer", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                return Make("BoxEdgeChamfer", "Harness", "One top edge, d 0.3: planar strip, straight caps", Body,
                            SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 1.99 && B.Z > 1.99; }),
                            BlendProfile::Chamfer, 0.3, -0.5 * 0.3 * 0.3 * 4.0, 7);
            } },
            { "BoxTopFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                return Make("BoxTopFillet", "Harness", "Four top edges, r 0.3: four elliptical mitres", Body, SelectEdges(Partition, Body, TopEdges(2.0)),
                            BlendProfile::Fillet, 0.3, -MitredFilletLoss(0.3, 14.0), 10);
            } },
            { "BoxTopChamfer", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                return Make("BoxTopChamfer", "Harness", "Four top edges, d 0.3: straight mitres", Body, SelectEdges(Partition, Body, TopEdges(2.0)),
                            BlendProfile::Chamfer, 0.3, -MitredChamferLoss(0.3, 14.0), 10);
            } },
            { "BoxCornerFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                const Vector3   Corner = BoxExtent;
                const double    Radius = 0.3;
                return Make("BoxCornerFillet", "Harness", "Three edges at a corner, r 0.3: sphere corner + three caps", Body,
                            SelectEdges(Partition, Body, [=](Vector3 A, Vector3 B, const CurveGeometry&) { return Distance(A, Corner) < 1e-9 || Distance(B, Corner) < 1e-9; }),
                            BlendProfile::Fillet, Radius,
                            -(StraightFilletLoss(Radius, (4.0 - Radius) + (3.0 - Radius) + (2.0 - Radius)) + Radius * Radius * Radius * (1.0 - π / 6.0)), 10);
            } },
            { "BoxAllFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                BlendSpecimen Specimen = Make("BoxAllFillet", "Harness", "All twelve edges, r 0.3: eight sphere corners", Body, SelectEdges(Partition, Body, AnyEdge()),
                                              BlendProfile::Fillet, 0.3, -RoundedBoxLoss(BoxExtent, 0.3), 26);
                Specimen.Clearance = RoundedBoxClearance({}, BoxExtent, 0.3);
                return Specimen;
            } },
            { "BoxAllChamfer", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                BlendSpecimen Specimen = Make("BoxAllChamfer", "Harness", "All twelve edges, d 0.3: eight triple mitres to an apex", Body, SelectEdges(Partition, Body, AnyEdge()),
                                              BlendProfile::Chamfer, 0.3, -ChamferedBoxLoss(BoxExtent, 0.3), 18);
                Specimen.Clearance = ChamferedBoxClearance({}, BoxExtent, 0.3);
                return Specimen;
            } },
            { "CylinderRimFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructCylinder(Partition, UprightFrame({}), 1.0, 2.0).Value;
                BlendSpecimen Specimen = Make("CylinderRimFillet", "Harness", "Top rim r 0.3: torus, seam continuation", Body, SelectEdges(Partition, Body, RimAt(2.0)),
                                              BlendProfile::Fillet, 0.3, -(2.0 * π * (1.0 - 0.3 * SpandrelFactor) * (1.0 - π / 4.0) * 0.09), 4);
                Specimen.Clearance = RoundedRimClearance(1.0, 2.0, 0.3);
                return Specimen;
            } },
            { "CylinderRimsChamfer", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructCylinder(Partition, UprightFrame({}), 1.0, 2.0).Value;
                const EdgeTest  Rims = [](Vector3, Vector3, const CurveGeometry& Curve) { return !std::holds_alternative<StraightLine>(Curve); };
                return Make("CylinderRimsChamfer", "Harness", "Both rims d 0.3: two cones, seam trimmed at both ends", Body, SelectEdges(Partition, Body, Rims),
                            BlendProfile::Chamfer, 0.3, -2.0 * (2.0 * π * (1.0 - 0.3 / 3.0) * 0.5 * 0.09), 5);
            } },
            { "BossRootFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = Fuse(Partition, ConstructBlock(Partition, {}, BoxExtent).Value, ConstructCylinder(Partition, UprightFrame({ 2.0, 1.5, 1.0 }), 0.6, 2.0).Value, BooleanOperation::Union);
                return Make("BossRootFillet", "Harness", "Concave boss root r 0.2: material added (reversed torus)", Body, SelectEdges(Partition, Body, RimAt(2.0)),
                            BlendProfile::Fillet, 0.2, 2.0 * π * (0.6 + 0.2 * SpandrelFactor) * (1.0 - π / 4.0) * 0.04, 9);
            } },
            { "BossRimFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = Fuse(Partition, ConstructBlock(Partition, {}, BoxExtent).Value, ConstructCylinder(Partition, UprightFrame({ 2.0, 1.5, 1.0 }), 0.6, 2.0).Value, BooleanOperation::Union);
                return Make("BossRimFillet", "Harness", "Boss top rim r 0.2 on a Boolean result", Body, SelectEdges(Partition, Body, RimAt(3.0)),
                            BlendProfile::Fillet, 0.2, -(2.0 * π * (0.6 - 0.2 * SpandrelFactor) * (1.0 - π / 4.0) * 0.04), 9);
            } },
            { "HoleRimFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = Fuse(Partition, ConstructBlock(Partition, {}, BoxExtent).Value, ConstructCylinder(Partition, UprightFrame({ 2.0, 1.5, -1.0 }), 0.6, 4.0).Value, BooleanOperation::Difference);
                return Make("HoleRimFillet", "Harness", "Convex rim of a through hole r 0.2 (genus 1)", Body, SelectEdges(Partition, Body, RimAt(2.0)),
                            BlendProfile::Fillet, 0.2, -(2.0 * π * (0.6 + 0.2 * SpandrelFactor) * (1.0 - π / 4.0) * 0.04), 8);
            } },
            { "FrustumRimFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructFrustum(Partition, UprightFrame({}), 1.2, 0.6, 1.5).Value;
                const Vector2   Down = Normalize(Vector2{ 1.2 - 0.6, -1.5 });
                return Make("FrustumRimFillet", "Harness", "Cone top rim r 0.15 (old harness: no closed form; now Pappus)", Body, SelectEdges(Partition, Body, RimAt(1.5)),
                            BlendProfile::Fillet, 0.15, MeasureRimBlendVolume({ 0.6, 1.5 }, { -1.0, 0.0 }, Down, 0.15, BlendProfile::Fillet, true), 4);
            } },
            { "CylinderChamferOversize", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructCylinder(Partition, UprightFrame({}), 0.5, 2.0).Value;
                return MakeRefusal("CylinderChamferOversize", "Harness", "Chamfer 0.6 on a rim of radius 0.5 (must refuse)", Body, SelectEdges(Partition, Body, RimAt(2.0)), BlendProfile::Chamfer, 0.6);
            } },
            { "CylinderFilletOversize", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructCylinder(Partition, UprightFrame({}), 0.5, 2.0).Value;
                return MakeRefusal("CylinderFilletOversize", "Harness", "Fillet 0.6 on a rim of radius 0.5 (must refuse)", Body, SelectEdges(Partition, Body, RimAt(2.0)), BlendProfile::Fillet, 0.6);
            } },

            // ---- BevelLab: the old editor proof scene (same coordinates).
            { "RoundedBlock", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, { 2.0, 1.5, 1.0 }).Value;
                BlendSpecimen Specimen = Make("RoundedBlock", "BevelLab", "All 12 edges fillet 0.2: 8 sphere corners", Body, SelectEdges(Partition, Body, AnyEdge()),
                                              BlendProfile::Fillet, 0.2, -RoundedBoxLoss({ 2.0, 1.5, 1.0 }, 0.2), 26);
                Specimen.Clearance = RoundedBoxClearance({}, { 2.0, 1.5, 1.0 }, 0.2);
                return Specimen;
            } },
            { "ChamferBlock", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, { 3.0, 0.0, 0.0 }, { 2.0, 1.5, 1.0 }).Value;
                return Make("ChamferBlock", "BevelLab", "All 12 edges chamfer 0.15: 8 triple mitres", Body, SelectEdges(Partition, Body, AnyEdge()),
                            BlendProfile::Chamfer, 0.15, -ChamferedBoxLoss({ 2.0, 1.5, 1.0 }, 0.15), 18);
            } },
            { "BossPlate", [](ModelPartition& Partition)
            {
                const BodyToken Body = Fuse(Partition, ConstructBlock(Partition, { 6.0, 0.0, 0.0 }, { 3.0, 2.0, 0.6 }).Value, ConstructCylinder(Partition, UprightFrame({ 7.5, 1.0, 0.3 }), 0.5, 1.0).Value, BooleanOperation::Union);
                std::vector<EdgeToken> Edges = SelectEdges(Partition, Body, TopEdges(0.6));
                for (double Height : { 0.6, 1.3 }) for (EdgeToken Edge : SelectEdges(Partition, Body, RimAt(Height))) Edges.push_back(Edge);
                const double Radius = 0.15;
                const double Delta  = -MitredFilletLoss(Radius, 10.0)
                                    + MeasureRimBlendVolume({ 0.5, 0.6 }, { 1.0, 0.0 }, { 0.0, 1.0 }, Radius, BlendProfile::Fillet, false)
                                    + MeasureRimBlendVolume({ 0.5, 1.3 }, { -1.0, 0.0 }, { 0.0, -1.0 }, Radius, BlendProfile::Fillet, true);
                return Make("BossPlate", "BevelLab", "Plate top edges (mitres) + concave boss root + boss rim, fillet 0.15", Body, Edges, BlendProfile::Fillet, Radius, Delta);
            } },
            { "Puck", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructCylinder(Partition, UprightFrame({ 11.0, 1.0, 0.0 }), 0.7, 1.2).Value;
                const EdgeTest  Rims = [](Vector3, Vector3, const CurveGeometry& Curve) { return !std::holds_alternative<StraightLine>(Curve); };
                const double    Each = MeasureRimBlendVolume({ 0.7, 1.2 }, { -1.0, 0.0 }, { 0.0, -1.0 }, 0.25, BlendProfile::Fillet, true);
                return Make("Puck", "BevelLab", "Both rims fillet 0.25 (seam trimmed at both ends)", Body, SelectEdges(Partition, Body, Rims), BlendProfile::Fillet, 0.25, 2.0 * Each, 5);
            } },
            { "HoleSlab", [](ModelPartition& Partition)
            {
                const BodyToken Body = Fuse(Partition, ConstructBlock(Partition, { 13.0, 0.0, 0.0 }, { 3.0, 2.0, 0.6 }).Value, ConstructCylinder(Partition, UprightFrame({ 14.5, 1.0, -0.5 }), 0.45, 2.0).Value, BooleanOperation::Difference);
                std::vector<EdgeToken> Edges = SelectEdges(Partition, Body, TopEdges(0.6));
                for (EdgeToken Edge : SelectEdges(Partition, Body, RimAt(0.6))) Edges.push_back(Edge);
                const double Radius = 0.12;
                const double Delta  = -MitredFilletLoss(Radius, 10.0) + MeasureRimBlendVolume({ 0.45, 0.6 }, { 1.0, 0.0 }, { 0.0, -1.0 }, Radius, BlendProfile::Fillet, true);
                return Make("HoleSlab", "BevelLab", "Slab top edges (mitres) + hole rim, fillet 0.12", Body, Edges, BlendProfile::Fillet, Radius, Delta);
            } },
            { "Frustum", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructFrustum(Partition, UprightFrame({ 18.0, 1.0, 0.0 }), 0.9, 0.45, 1.1).Value;
                const EdgeTest  Rims = [](Vector3, Vector3, const CurveGeometry& Curve) { return !std::holds_alternative<StraightLine>(Curve); };
                const Vector2   Up   = Normalize(Vector2{ 0.45 - 0.9, 1.1 });
                const double    Delta = MeasureRimBlendVolume({ 0.45, 1.1 }, { -1.0, 0.0 }, -Up, 0.12, BlendProfile::Chamfer, true)
                                      + MeasureRimBlendVolume({ 0.9, 0.0 }, { -1.0, 0.0 }, Up, 0.12, BlendProfile::Chamfer, true);
                return Make("Frustum", "BevelLab", "Both cone rims chamfer 0.12 (angles 112° / 68°)", Body, SelectEdges(Partition, Body, Rims), BlendProfile::Chamfer, 0.12, Delta, 5);
            } },

            // ---- Extra: wedge angles ≠ 90° and caps that are ellipses.
            { "PrismAcuteFillet", [](ModelPartition& Partition)
            {
                const double Side = 3.0;
                const BodyToken Body = ConstructPrism(Partition, { { 0.0, 0.0, 0.0 }, { Side, 0.0, 0.0 }, { 0.5 * Side, 0.5 * Side * std::sqrt(3.0), 0.0 } }, { 0.0, 0.0, 2.0 }).Value.Body;
                const double Radius = 0.2;
                return Make("PrismAcuteFillet", "Extra", "60° edge of an equilateral prism, r 0.2: area r²(√3 − π/3)", Body,
                            SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return std::hypot(A.X, A.Y) < 1e-9 && std::hypot(B.X, B.Y) < 1e-9; }),
                            BlendProfile::Fillet, Radius, -Radius * Radius * (std::sqrt(3.0) - π / 3.0) * 2.0, 6);
            } },
            { "HexagonPrismChamfer", [](ModelPartition& Partition)
            {
                std::vector<Vector3> Profile;
                for (int Corner = 0; Corner < 6; ++Corner) Profile.push_back({ 1.5 * std::cos(Corner * π / 3.0), 1.5 * std::sin(Corner * π / 3.0), 0.0 });
                const BodyToken Body = ConstructPrism(Partition, Profile, { 0.0, 0.0, 1.5 }).Value.Body;
                const double Setback = 0.2;
                return Make("HexagonPrismChamfer", "Extra", "120° edge of a hexagonal prism, d 0.2: area ½d² sin 120°", Body,
                            SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return std::fabs(A.X - 1.5) < 1e-9 && std::fabs(B.X - 1.5) < 1e-9 && std::fabs(A.Y) < 1e-9 && std::fabs(B.Y) < 1e-9; }),
                            BlendProfile::Chamfer, Setback, -0.5 * Setback * Setback * std::sin(2.0 * π / 3.0) * 1.5, 9);
            } },
            { "SkewPrismFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructPrism(Partition, { { 0.0, 0.0, 0.0 }, { 4.0, 0.0, 0.0 }, { 5.0, 3.0, 0.0 }, { 1.0, 3.0, 0.0 } }, { 0.0, 0.0, 2.0 }).Value.Body;
                return Make("SkewPrismFillet", "Extra", "Edge ending on slanted (parallel) faces, r 0.3: two elliptical caps", Body,
                            SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 1.99 && B.Z > 1.99; }),
                            BlendProfile::Fillet, 0.3, -StraightFilletLoss(0.3, 4.0), 7);
            } },

            // ---- Overflow: blends wider than a support run off its far edge and are trimmed by the cliff face (Parasolid
            //      cliff overflow); partial, notch and overlapping overflows must still refuse.
            { "CliffBoxFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                BlendSpecimen Specimen = Make("CliffBoxFillet", "Overflow", "Top edge r 2.5 on a box 2 high: front face consumed, the bottom face trims the blend", Body,
                                              SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 1.99 && B.Z > 1.99; }),
                                              BlendProfile::Fillet, 2.5, -BoxExtent.X * CliffArea(2.5, BoxExtent.Z, BlendProfile::Fillet), 6);
                Specimen.Clearance = CliffBoxClearance(BoxExtent, 2.5, BlendProfile::Fillet);
                return Specimen;
            } },
            { "CliffPlateFillet", [](ModelPartition& Partition)
            {
                const Vector3   Plate = { 4.0, 2.0, 0.2 };
                const BodyToken Body  = ConstructBlock(Partition, {}, Plate).Value;
                BlendSpecimen Specimen = Make("CliffPlateFillet", "Overflow", "Plate 0.2 thick, front top edge r 0.3: the blend runs off the front face onto the bottom", Body,
                                              SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 0.19 && B.Z > 0.19; }),
                                              BlendProfile::Fillet, 0.3, -Plate.X * CliffArea(0.3, Plate.Z, BlendProfile::Fillet), 6);
                Specimen.Clearance = CliffBoxClearance(Plate, 0.3, BlendProfile::Fillet);
                return Specimen;
            } },
            { "CliffPlateChamfer", [](ModelPartition& Partition)
            {
                const Vector3   Plate = { 4.0, 2.0, 0.2 };
                const BodyToken Body  = ConstructBlock(Partition, {}, Plate).Value;
                BlendSpecimen Specimen = Make("CliffPlateChamfer", "Overflow", "Plate 0.2 thick, front top edge chamfer 0.3: the chamfer plane is trimmed by the bottom face", Body,
                                              SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 0.19 && B.Z > 0.19; }),
                                              BlendProfile::Chamfer, 0.3, -Plate.X * CliffArea(0.3, Plate.Z, BlendProfile::Chamfer), 6);
                Specimen.Clearance = CliffBoxClearance(Plate, 0.3, BlendProfile::Chamfer);
                return Specimen;
            } },
            { "CliffPlateFlush", [](ModelPartition& Partition)
            {
                const Vector3   Plate = { 4.0, 2.0, 0.2 };
                const BodyToken Body  = ConstructBlock(Partition, {}, Plate).Value;
                BlendSpecimen Specimen = Make("CliffPlateFlush", "Overflow", "Plate 0.2 thick, front top edge r 0.2 exactly: the rail lands on the far edge (boundary case)", Body,
                                              SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 0.19 && B.Z > 0.19; }),
                                              BlendProfile::Fillet, 0.2, -StraightFilletLoss(0.2, Plate.X), 6);
                Specimen.Clearance = CliffBoxClearance(Plate, 0.2, BlendProfile::Fillet);
                return Specimen;
            } },
            { "CliffDiscFillet", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructCylinder(Partition, UprightFrame({}), 1.0, 0.2).Value;
                BlendSpecimen Specimen = Make("CliffDiscFillet", "Overflow", "Disc 0.2 thick, top rim r 0.3: the side band is consumed, the torus meets the bottom in a circle", Body,
                                              SelectEdges(Partition, Body, RimAt(0.2)), BlendProfile::Fillet, 0.3, -CliffRimLoss(1.0, 0.3, 0.2, BlendProfile::Fillet), 3);
                Specimen.Clearance = CliffDiscClearance(1.0, 0.2, 0.3, BlendProfile::Fillet);
                return Specimen;
            } },
            { "CliffDiscChamfer", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructCylinder(Partition, UprightFrame({}), 1.0, 0.2).Value;
                BlendSpecimen Specimen = Make("CliffDiscChamfer", "Overflow", "Disc 0.2 thick, top rim chamfer 0.3: the cone meets the bottom in a circle", Body,
                                              SelectEdges(Partition, Body, RimAt(0.2)), BlendProfile::Chamfer, 0.3, -CliffRimLoss(1.0, 0.3, 0.2, BlendProfile::Chamfer), 3);
                Specimen.Clearance = CliffDiscClearance(1.0, 0.2, 0.3, BlendProfile::Chamfer);
                return Specimen;
            } },
            { "CliffPartial", [](ModelPartition& Partition)
            {
                // Front face 0.45 high at x = 0 and 0.1 high at x = 4: r 0.3 runs off it at one end only.
                const BodyToken Body = ConstructPrism(Partition, { { 0.0, 0.0, 0.5 }, { 4.0, 0.0, 0.5 }, { 4.0, 0.0, 0.4 }, { 0.0, 0.0, 0.05 } }, { 0.0, 2.0, 0.0 }).Value.Body;
                return MakeRefusal("CliffPartial", "Refusal", "Tapering front face, top edge r 0.3: wider than the face at one end only (partial overflow, 🚧)", Body,
                                   SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && A.Z > 0.49 && B.Z > 0.49; }), BlendProfile::Fillet, 0.3);
            } },
            { "CliffOverlap", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, { 4.0, 2.0, 0.2 }).Value;
                return MakeRefusal("CliffOverlap", "Refusal", "Plate 0.2 thick, top AND bottom front edges r 0.3: both blends would consume the front face", Body,
                                   SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return A.Y < 1e-9 && B.Y < 1e-9 && std::fabs(A.Z - B.Z) < 1e-9; }), BlendProfile::Fillet, 0.3);
            } },
            { "NotchOverflow", [](ModelPartition& Partition)
            {
                // Step: riser 0.2 high at x = 1 above a ledge; the riser's lower edge is concave.
                const BodyToken Body = ConstructPrism(Partition, { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 0.0, 1.2 }, { 2.0, 0.0, 1.2 }, { 2.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }, { 0.0, 1.5, 0.0 }).Value.Body;
                return MakeRefusal("NotchOverflow", "Refusal", "Riser 0.2 high, its top edge r 0.3: the blend would run off into a concave edge (notch overflow, 🚧)", Body,
                                   SelectEdges(Partition, Body, [](Vector3 A, Vector3 B, const CurveGeometry&) { return std::fabs(A.X - 1.0) < 1e-9 && std::fabs(B.X - 1.0) < 1e-9 && A.Z > 1.19 && B.Z > 1.19; }), BlendProfile::Fillet, 0.3);
            } },

            // ---- Refusal: configurations the operation must refuse atomically.
            { "FilletRailsCross", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructBlock(Partition, {}, BoxExtent).Value;
                return MakeRefusal("FilletRailsCross", "Refusal", "All edges r 1.2 on a box 2 high: vertical contact rails run backwards", Body, SelectEdges(Partition, Body, AnyEdge()), BlendProfile::Fillet, 1.2);
            } },
            { "MixedConvexity", [](ModelPartition& Partition)
            {
                const BodyToken Body = ConstructPrism(Partition, { { 0.0, 0.0, 0.0 }, { 3.0, 0.0, 0.0 }, { 3.0, 1.0, 0.0 }, { 1.0, 1.0, 0.0 }, { 1.0, 3.0, 0.0 }, { 0.0, 3.0, 0.0 } }, { 0.0, 0.0, 1.0 }).Value.Body;
                const EdgeTest Pick = [](Vector3 A, Vector3 B, const CurveGeometry&)
                {
                    const bool Inner = std::fabs(A.X - 1.0) < 1e-9 && std::fabs(A.Y - 1.0) < 1e-9 && std::fabs(B.X - 1.0) < 1e-9 && std::fabs(B.Y - 1.0) < 1e-9;
                    const bool Rim   = A.Z > 0.99 && B.Z > 0.99 && std::fabs(A.X - 1.0) < 1e-9 && std::fabs(B.X - 1.0) < 1e-9;
                    return Inner || Rim;
                };
                return MakeRefusal("MixedConvexity", "Refusal", "Concave inner edge + convex top edge of an L block meet beside a sharp edge (mixed convexity with a sharp edge, 🚧)", Body, SelectEdges(Partition, Body, Pick), BlendProfile::Fillet, 0.2);
            } },
        };
        return Table;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

double MeasureRimBlendVolume(Vector2 Corner, Vector2 AlongFirst, Vector2 AlongSecond, double Size, BlendProfile Profile, bool ConvexCondition)
{
    const Vector2 First  = Normalize(AlongFirst);                                            // [-] - Along face 1
    const Vector2 Second = Normalize(AlongSecond);                                           // [-] - Along face 2
    const double  Wedge  = std::acos(std::clamp(Dot(First, Second), -1.0, 1.0));             // [rad] - θ
    const double  Sign   = ConvexCondition ? -1.0 : 1.0;                                     // [-]   - Removed / added
    // 📝 Pappus: V = 2π ρ̄ A over the cross-section in the meridian half-plane.
    const auto Triangle = [](Vector2 A, Vector2 B, Vector2 C, double& Moment)
    {
        const double Area = 0.5 * std::fabs(Cross(B - A, C - A));                            // [len²] - Area
        Moment += Area * (A.X + B.X + C.X) / 3.0;                                             // [len³] - ∫ρ dA
        return Area;
    };
    double Moment = 0.0;                                                                     // [len³] - ∫ρ dA of the region
    if (Profile == BlendProfile::Chamfer)
    {
        Triangle(Corner, Corner + First * Size, Corner + Second * Size, Moment);
        return Sign * 2.0 * π * Moment;
    }
    const double  Reach    = Size / std::tan(0.5 * Wedge);                                   // [len] - Contact distance from E
    const Vector2 Bisector = Normalize(First + Second);                                      // [-]   - Into the wedge
    const Vector2 Centre   = Corner + Bisector * (Size / std::sin(0.5 * Wedge));             // [len] - Ball centre
    Triangle(Corner, Corner + First * Reach, Centre, Moment);
    Triangle(Corner, Centre, Corner + Second * Reach, Moment);
    const double Sweep    = π - Wedge;                                                       // [rad] - Sector angle
    const double Sector   = 0.5 * Size * Size * Sweep;                                       // [len²] - Sector area
    const double Distance = 4.0 * Size * std::sin(0.5 * Sweep) / (3.0 * Sweep);              // [len]  - Centroid from the centre
    Moment -= Sector * (Centre - Bisector * Distance).X;
    return Sign * 2.0 * π * Moment;
}

std::vector<std::string> EnumerateBlendSpecimens()
{
    std::vector<std::string> Names;
    for (const auto& Entry : Specimens()) Names.push_back(Entry.first);
    return Names;
}

BlendSpecimen ConstructBlendSpecimen(ModelPartition& Partition, const std::string& Name)
{
    for (const auto& Entry : Specimens())
    {
        if (Entry.first == Name) return Entry.second(Partition);
    }
    return {};
}

} // namespace Frontier::SolidArcRemastered
