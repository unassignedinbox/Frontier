/*====================================================================================================================================
                                                       BODYTRANSFORM.CPP
====================================================================================================================================*/
// 🧩 Transformed copies of a body — carriers mapped per kind, SP-curves re-fitted, loops reversed by mirrors

#include "Construction/BodyTransform.h"

#include "Construction/BodyAssembly.h"
#include "Geometry/ProjectedCurve.h"
#include "Intersection/IntersectionCurve.h"
#include "Geometry/RollingBallCarrier.h"

#include <cmath>
#include <map>
#include <memory>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double SimilaritySlack = 1.0e-12;                                              // [-] - Relative Gram-matrix agreement

    // 📝 Linear part, determinant, and the similarity split M = s·Q when it exists.
    struct MapAnalysis
    {
        AffineMap Map         = {};                                                          // [-] - The map
        double    Determinant = 1.0;                                                         // [-] - det M
        bool      Similar     = true;                                                        // [-] - M = s·Q
        double    Scale       = 1.0;                                                         // [-] - s
    };

    [[nodiscard]] Vector3 ApplyLinear(const AffineMap& Map, Vector3 Vector) noexcept
    {
        return Map.ColumnX * Vector.X + Map.ColumnY * Vector.Y + Map.ColumnZ * Vector.Z;
    }

    [[nodiscard]] Vector3 ApplyPoint(const AffineMap& Map, Vector3 Point) noexcept { return ApplyLinear(Map, Point) + Map.Shift; }

    [[nodiscard]] MapAnalysis AnalyseMap(const AffineMap& Map)
    {
        MapAnalysis Analysis;
        Analysis.Map         = Map;
        Analysis.Determinant = Dot(Map.ColumnX, Cross(Map.ColumnY, Map.ColumnZ));
        const double Square  = std::cbrt(Analysis.Determinant * Analysis.Determinant);       // [-] - s² if similar
        const auto   Near    = [&](double Value, double Target) { return std::fabs(Value - Target) <= SimilaritySlack * std::max(1.0, Square); };
        Analysis.Similar = Near(Dot(Map.ColumnX, Map.ColumnX), Square) && Near(Dot(Map.ColumnY, Map.ColumnY), Square) && Near(Dot(Map.ColumnZ, Map.ColumnZ), Square)
                        && Near(Dot(Map.ColumnX, Map.ColumnY), 0.0) && Near(Dot(Map.ColumnY, Map.ColumnZ), 0.0) && Near(Dot(Map.ColumnZ, Map.ColumnX), 0.0);
        Analysis.Scale = std::sqrt(Square);
        return Analysis;
    }

    // Q·v for a similarity (unit result for unit input).
    [[nodiscard]] Vector3 Rotate(const MapAnalysis& Analysis, Vector3 Vector) noexcept { return ApplyLinear(Analysis.Map, Vector) * (1.0 / Analysis.Scale); }

    // Frame of a mapped surface: X′ = Q·X, Y′ = ±Q·Y (sign of det), hence Z′ = Q·Z and the natural normal maps with Q.
    [[nodiscard]] OrthonormalFrame MapSurfaceFrame(const MapAnalysis& Analysis, const OrthonormalFrame& Frame)
    {
        const Vector3 X = Rotate(Analysis, Frame.XAxis);
        const Vector3 Y = Rotate(Analysis, Frame.YAxis) * (Analysis.Determinant > 0.0 ? 1.0 : -1.0);
        return { ApplyPoint(Analysis.Map, Frame.Origin), X, Y, Cross(X, Y) };
    }

    // Frame of a mapped circle / ellipse: X′ = Q·X, Y′ = Q·Y keeps τ (Z′ = X′ × Y′ is only the plane normal).
    [[nodiscard]] OrthonormalFrame MapCurveFrame(const MapAnalysis& Analysis, const OrthonormalFrame& Frame)
    {
        const Vector3 X = Rotate(Analysis, Frame.XAxis), Y = Rotate(Analysis, Frame.YAxis);
        return { ApplyPoint(Analysis.Map, Frame.Origin), X, Y, Cross(X, Y) };
    }

    struct MappedSurface
    {
        SurfaceGeometry Surface = PlaneSurface{};                                            // [-] - Image carrier
        bool            Flip    = false;                                                     // [-] - Face sense inverts
        std::string     Refusal = {};                                                        // [-] - Why not mappable
    };

    [[nodiscard]] CurveGeometry MapCurveImage(const MapAnalysis& Analysis, const CurveGeometry& Curve);   // MapCurve(...).Curve

    [[nodiscard]] MappedSurface MapSurface(const MapAnalysis& Analysis, const SurfaceGeometry& Surface)
    {
        MappedSurface Mapped;
        if (const auto* Plane = std::get_if<PlaneSurface>(&Surface))
        {
            if (Analysis.Similar) { Mapped.Surface = PlaneSurface{ MapSurfaceFrame(Analysis, Plane->Frame) }; return Mapped; }
            // General affine: X′ ∥ M·X, Y′ from M·Y (Gram–Schmidt); Z′ ∝ M·X × M·Y = det·M⁻ᵀ·Z, so the sense flips when det < 0.
            const Vector3 X = Normalize(ApplyLinear(Analysis.Map, Plane->Frame.XAxis));
            const Vector3 Image = ApplyLinear(Analysis.Map, Plane->Frame.YAxis);
            const Vector3 Y = Normalize(Image - X * Dot(Image, X));
            Mapped.Surface = PlaneSurface{ { ApplyPoint(Analysis.Map, Plane->Frame.Origin), X, Y, Cross(X, Y) } };
            Mapped.Flip    = Analysis.Determinant < 0.0;
            return Mapped;
        }
        if (!Analysis.Similar) { Mapped.Refusal = "Transform: a non-similar map turns curved faces into surfaces outside the carrier kinds"; return Mapped; }
        const double Scale = Analysis.Scale;                                                 // [-] - s
        if (const auto* Ball = std::get_if<RollingBallSurface>(&Surface))
        {
            // 📝 Geometric definition: the image spine and guide rebuild the carrier with radius s·r.
            //    A reflection flips the handedness so the image chart is M·P(𝑢, 𝑣) and every SP-curve keeps its (𝑢, 𝑣);
            //    ∂𝑢 × ∂𝑣 (the natural normal) then flips relative to M·n, so the face sense flips with it.
            const MappedSurface Guide = MapSurface(Analysis, Ball->Definition->Guide);
            if (!Guide.Refusal.empty()) { Mapped.Refusal = Guide.Refusal; return Mapped; }
            Mapped.Surface = ConstructRollingBallSurface(MapCurveImage(Analysis, Ball->Definition->Spine), Ball->Definition->Radius * Scale, Guide.Surface,
                                                         Ball->Definition->Handedness * (Analysis.Determinant < 0.0 ? -1.0 : 1.0));
            Mapped.Flip    = Analysis.Determinant < 0.0;
            return Mapped;
        }
        std::visit([&](const auto& Carrier)
        {
            using Kind = std::decay_t<decltype(Carrier)>;
            if constexpr (std::is_same_v<Kind, RollingBallSurface>) return;                  // Mapped above
            else
            {
            const OrthonormalFrame Frame = MapSurfaceFrame(Analysis, Carrier.Frame);
            if constexpr (std::is_same_v<Kind, CylinderSurface>) Mapped.Surface = CylinderSurface{ Frame, Carrier.Radius * Scale };
            else if constexpr (std::is_same_v<Kind, ConeSurface>) Mapped.Surface = ConeSurface{ Frame, Carrier.OriginRadius * Scale, Carrier.RadialSlope };
            else if constexpr (std::is_same_v<Kind, SphereSurface>) Mapped.Surface = SphereSurface{ Frame, Carrier.Radius * Scale };
            else if constexpr (std::is_same_v<Kind, TorusSurface>) Mapped.Surface = TorusSurface{ Frame, Carrier.MajorRadius * Scale, Carrier.MinorRadius * Scale };
            }
        }, Surface);
        return Mapped;
    }

    struct MappedCurve
    {
        CurveGeometry Curve      = StraightLine{};                                           // [-] - Image carrier
        double        Stretch    = 1.0;                                                      // [-] - τ′ = Stretch · τ
        std::string   Refusal    = {};                                                       // [-] - Why not mappable
    };

    [[nodiscard]] MappedCurve MapCurve(const MapAnalysis& Analysis, const CurveGeometry& Curve)
    {
        MappedCurve Mapped;
        if (const auto* Line = std::get_if<StraightLine>(&Curve))
        {
            const Vector3 Image = ApplyLinear(Analysis.Map, Line->Direction);                // [-] - M·d
            Mapped.Stretch = Length(Image);
            Mapped.Curve   = StraightLine{ ApplyPoint(Analysis.Map, Line->Origin), Image * (1.0 / Mapped.Stretch) };
            return Mapped;
        }
        if (!Analysis.Similar) { Mapped.Refusal = "Transform: a non-similar map turns circles into curves outside the carrier kinds"; return Mapped; }
        if (const auto* Traced = std::get_if<IntersectionCurve>(&Curve))
        {
            // 📝 The carrier is geometric (Newton onto both surfaces in the guide's normal plane, 𝐭 oriented with H′), so a
            //    similarity maps it exactly: surfaces and guide map, closed knots (2π·chord / perimeter) are invariant,
            //    open knots (chord length) scale by s together with the edge interval; dH/dτ follows.
            const TracedIntersection& Source = *Traced->Trace;
            auto Image = std::make_shared<TracedIntersection>(Source);
            Image->First  = MapSurface(Analysis, Source.First).Surface;
            Image->Second = MapSurface(Analysis, Source.Second).Surface;
            Mapped.Stretch = Source.Closed ? 1.0 : Analysis.Scale;
            for (Vector3& Point : Image->Points) Point = ApplyPoint(Analysis.Map, Point);
            for (Vector3& Tangent : Image->Tangents) Tangent = ApplyLinear(Analysis.Map, Tangent) * (1.0 / Mapped.Stretch);
            for (double& Knot : Image->Knots) Knot *= Mapped.Stretch;
            Image->Perimeter *= Analysis.Scale;
            Mapped.Curve = IntersectionCurve{ std::move(Image) };
            return Mapped;
        }
        if (const auto* Contact = std::get_if<ContactCurve>(&Curve))
        {
            // 📝 A rail is the foot of its spine on its support: map both and rebuild; τ stretches like the spine's.
            const ContactDefinition& Source = *Contact->Definition;
            const MappedCurve        Spine  = MapCurve(Analysis, Source.Spine);
            const MappedSurface      Support = MapSurface(Analysis, Source.Support);
            if (!Spine.Refusal.empty() || !Support.Refusal.empty()) { Mapped.Refusal = Spine.Refusal.empty() ? Support.Refusal : Spine.Refusal; return Mapped; }
            const ContactCurve Forward = ConstructContactCurve(Spine.Curve, Support.Surface);
            Mapped.Curve   = Source.Direction > 0.0 ? CurveGeometry{ Forward } : ReverseContactCurve(*Forward.Definition);
            Mapped.Stretch = Spine.Stretch;
            return Mapped;
        }
        if (const auto* Arc = std::get_if<CircularArc>(&Curve)) Mapped.Curve = CircularArc{ MapCurveFrame(Analysis, Arc->Frame), Arc->Radius * Analysis.Scale };
        else
        {
            const auto& Ellipse = std::get<EllipticalArc>(Curve);
            Mapped.Curve = EllipticalArc{ MapCurveFrame(Analysis, Ellipse.Frame), Ellipse.MajorRadius * Analysis.Scale, Ellipse.MinorRadius * Analysis.Scale };
        }
        return Mapped;
    }

    CurveGeometry MapCurveImage(const MapAnalysis& Analysis, const CurveGeometry& Curve) { return MapCurve(Analysis, Curve).Curve; }

    // Chart image of a point for the periodic-image choice: 𝑢 → ±𝑢 (mirror), 𝑣 scales with s on cylinders / cones only.
    [[nodiscard]] Vector2 MapAnchor(const MapAnalysis& Analysis, const SurfaceGeometry& Surface, Vector2 Parameter)
    {
        if (std::holds_alternative<PlaneSurface>(Surface)) return Parameter;                // No periodic image to choose
        if (std::holds_alternative<RollingBallSurface>(Surface)) return Parameter;          // Chart carried exactly (handedness)
        const double U = Analysis.Determinant > 0.0 ? Parameter.X : -Parameter.X;
        const bool   Linear = std::holds_alternative<CylinderSurface>(Surface) || std::holds_alternative<ConeSurface>(Surface);
        return { U, Linear ? Parameter.Y * Analysis.Scale : Parameter.Y };
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

AffineMap TranslationMap(Vector3 Displacement) noexcept
{
    AffineMap Map;
    Map.Shift = Displacement;
    return Map;
}

AffineMap RotationMap(Vector3 AxisPoint, Vector3 AxisDirection, double Angle) noexcept
{
    // Rodrigues: R·v = v cos θ + (k × v) sin θ + k (k·v)(1 − cos θ).
    const Vector3 Axis = Normalize(AxisDirection);
    const double  Cosine = std::cos(Angle), Sine = std::sin(Angle);
    const auto    Turn = [&](Vector3 Vector) { return Vector * Cosine + Cross(Axis, Vector) * Sine + Axis * (Dot(Axis, Vector) * (1.0 - Cosine)); };
    AffineMap Map{ Turn({ 1.0, 0.0, 0.0 }), Turn({ 0.0, 1.0, 0.0 }), Turn({ 0.0, 0.0, 1.0 }), {} };
    Map.Shift = AxisPoint - ApplyLinear(Map, AxisPoint);
    return Map;
}

AffineMap ReflectionMap(Vector3 PlanePoint, Vector3 PlaneNormal) noexcept
{
    // M = I − 2 n nᵀ, Shift = 2 (p·n) n.
    const Vector3 Normal = Normalize(PlaneNormal);
    const auto    Mirror = [&](Vector3 Vector) { return Vector - Normal * (2.0 * Dot(Normal, Vector)); };
    return { Mirror({ 1.0, 0.0, 0.0 }), Mirror({ 0.0, 1.0, 0.0 }), Mirror({ 0.0, 0.0, 1.0 }), Normal * (2.0 * Dot(PlanePoint, Normal)) };
}

AffineMap ScalingMap(Vector3 Centre, Vector3 Factors) noexcept
{
    AffineMap Map{ { Factors.X, 0.0, 0.0 }, { 0.0, Factors.Y, 0.0 }, { 0.0, 0.0, Factors.Z }, {} };
    Map.Shift = Centre - ApplyLinear(Map, Centre);
    return Map;
}

TransformOutcome TransformBodyAcross(const ModelPartition& Partition, BodyToken Source, ModelPartition& Target, const AffineMap& Map)
{
    TransformOutcome Outcome;
    const MapAnalysis Analysis = AnalyseMap(Map);
    if (!(std::fabs(Analysis.Determinant) > 1.0e-12)) { Outcome.Refusal = "Transform: the map is singular"; return Outcome; }
    const BodyInventory Inventory = Partition.CollectInventory(Source);
    BodyBlueprint Blueprint;
    Blueprint.Category = Partition.Bodies.Resolve(Source).Category;

    // ① Vertices and edges (line intervals stretch by |M·d|; circle / ellipse parameters are kept).
    std::map<uint32_t, int> VertexIndex, EdgeIndex;                                          // [-] - Source slot → blueprint index
    for (VertexToken Vertex : Inventory.Vertices)
    {
        VertexIndex[Vertex.Slot] = static_cast<int>(Blueprint.Vertices.size());
        Blueprint.Vertices.push_back({ ApplyPoint(Map, Partition.Vertices.Resolve(Vertex).Position) });
    }
    for (EdgeToken Edge : Inventory.Edges)
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
        if (!Profile.Curve.PresentCondition()) { Outcome.Refusal = "Transform: edge without a carrier"; return Outcome; }
        const MappedCurve Mapped = MapCurve(Analysis, Partition.Curves.Resolve(Profile.Curve));
        if (!Mapped.Refusal.empty()) { Outcome.Refusal = Mapped.Refusal; return Outcome; }
        EdgeIndex[Edge.Slot] = static_cast<int>(Blueprint.Edges.size());
        Blueprint.Edges.push_back({ Mapped.Curve, Profile.IntervalLow * Mapped.Stretch, Profile.IntervalHigh * Mapped.Stretch, VertexIndex.at(Profile.Start.Slot), VertexIndex.at(Profile.End.Slot) });
    }

    // ② Faces: mapped carrier, sense kept (flipped only for mirrored general-affine planes), SP-curves re-fitted with the
    //    mapped chart midpoint as the periodic anchor; a mirror reverses every loop (orientation about the normal flips).
    for (FaceToken Face : Inventory.Faces)
    {
        const FaceProfile& Profile = Partition.Faces.Resolve(Face);
        const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
        const MappedSurface Mapped = MapSurface(Analysis, Surface);
        if (!Mapped.Refusal.empty()) { Outcome.Refusal = Mapped.Refusal; return Outcome; }
        BlueprintFace Image;
        Image.Surface = Mapped.Surface;
        Image.Sense   = Mapped.Flip ? (Profile.Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward) : Profile.Sense;
        for (LoopToken Loop : Profile.Loops)
        {
            std::vector<BlueprintUse> Uses;
            bool Valid = true;
            Partition.EnumerateLoopFins(Loop, [&](FinToken Fin)
            {
                const FinProfile&  Use  = Partition.Fins.Resolve(Fin);
                const int          Edge = EdgeIndex.at(Use.Edge.Slot);
                const BlueprintEdge& EdgeImage = Blueprint.Edges[Edge];
                const EdgeProfile& UsedEdge = Partition.Edges.Resolve(Use.Edge);
                if (!Use.SurfaceCurve) { Valid = false; return; }
                const Vector2 Middle = EvaluateParameterCurve(*Use.SurfaceCurve, 0.5 * (UsedEdge.IntervalLow + UsedEdge.IntervalHigh)).Position;
                const Vector2 Anchor = MapAnchor(Analysis, Surface, Middle);
                BlueprintUse Placement;
                Placement.Edge         = Edge;
                Placement.Sense        = Use.Sense;
                Placement.SurfaceCurve = FitSurfaceCurve(Image.Surface, EdgeImage.Curve, EdgeImage.Low, EdgeImage.High, { Anchor - Vector2{ 1.0, 1.0 }, Anchor + Vector2{ 1.0, 1.0 } }, Anchor);
                Uses.push_back(std::move(Placement));
            });
            if (!Valid) { Outcome.Refusal = "Transform: fin without an SP-curve"; return Outcome; }
            if (Analysis.Determinant < 0.0)
            {
                std::reverse(Uses.begin(), Uses.end());
                for (BlueprintUse& Placement : Uses) Placement.Sense = Placement.Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward;
            }
            Image.Loops.push_back(std::move(Uses));
        }
        Blueprint.Faces.push_back(std::move(Image));
    }
    // ③ The blueprint is complete: assembling into Target (possibly the same partition) cannot disturb the reads above.
    const AssemblyOutcome Assembly = AssembleBody(Target, Blueprint, "Transform");
    if (!Assembly.Refusal.empty()) { Outcome.Refusal = Assembly.Refusal; return Outcome; }
    Outcome.Body = Assembly.Body;
    return Outcome;
}

TransformOutcome TransformBody(ModelPartition& Partition, BodyToken Source, const AffineMap& Map)
{
    return TransformBodyAcross(Partition, Source, Partition, Map);
}

TransformOutcome MirrorBody(ModelPartition& Partition, BodyToken Source, Vector3 PlanePoint, Vector3 PlaneNormal)
{
    return TransformBody(Partition, Source, ReflectionMap(PlanePoint, PlaneNormal));
}

std::vector<TransformOutcome> ArrayBody(ModelPartition& Partition, BodyToken Source, Vector3 Step, int Count)
{
    std::vector<TransformOutcome> Copies;
    for (int Instance = 1; Instance < Count; ++Instance) Copies.push_back(TransformBody(Partition, Source, TranslationMap(Step * static_cast<double>(Instance))));
    return Copies;
}

std::vector<TransformOutcome> CircularArrayBody(ModelPartition& Partition, BodyToken Source, Vector3 AxisPoint, Vector3 AxisDirection, int Count, double Sweep)
{
    std::vector<TransformOutcome> Copies;
    if (Count < 1) return Copies;
    const double Pitch = Sweep / static_cast<double>(Count);                                 // [rad] - Angle between instances
    for (int Instance = 1; Instance < Count; ++Instance)
        Copies.push_back(TransformBody(Partition, Source, RotationMap(AxisPoint, AxisDirection, Pitch * static_cast<double>(Instance))));
    return Copies;
}

} // namespace Frontier::SolidArcRemastered
