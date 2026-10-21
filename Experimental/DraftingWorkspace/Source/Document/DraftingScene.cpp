/*====================================================================================================================================
                                                       DRAFTINGSCENE.CPP
====================================================================================================================================*/
// 🧩 Display arrays of the current body (per-face corners, edge segments, name tables) and the CPU ray pick

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingScene.h"

#include "Geometry/CurveGeometry.h"
#include "Geometry/NurbsCarrier.h"
#include "Geometry/SurfaceGeometry.h"
#include "Tessellation/BodyTessellation.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <numbers>
#include <set>

namespace Frontier::DraftingWorkspace
{

using namespace Frontier::SolidArcRemastered;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

using Triple = std::array<double, 3>;

[[nodiscard]] Triple Difference(const Triple& Left, const Triple& Right) { return { Left[0] - Right[0], Left[1] - Right[1], Left[2] - Right[2] }; }
[[nodiscard]] double Dot(const Triple& Left, const Triple& Right) { return Left[0] * Right[0] + Left[1] * Right[1] + Left[2] * Right[2]; }

[[nodiscard]] Triple Cross(const Triple& Left, const Triple& Right)
{
    return { Left[1] * Right[2] - Left[2] * Right[1], Left[2] * Right[0] - Left[0] * Right[2], Left[0] * Right[1] - Left[1] * Right[0] };
}

[[nodiscard]] Triple Widen(const float Source[3]) { return { Source[0], Source[1], Source[2] }; }
[[nodiscard]] Triple Widen(const Vector3& Source) { return { Source.X, Source.Y, Source.Z }; }

void Narrow(const Triple& Source, float Target[3])
{
    for (int Axis = 0; Axis < 3; ++Axis) Target[Axis] = static_cast<float>(Source[Axis]);
}

[[nodiscard]] Triple Normalised(const Triple& Source)
{
    const double Length = std::sqrt(Dot(Source, Source));
    if (Length <= 0.0) return { 0.0, 0.0, 1.0 };
    return { Source[0] / Length, Source[1] / Length, Source[2] / Length };
}

// Möller–Trumbore: ray parameter of the hit, or nothing.
[[nodiscard]] std::optional<double> IntersectTriangle(const DraftingRay& Ray, const Triple& First, const Triple& Second, const Triple& Third)
{
    const Triple SideA = Difference(Second, First);                                          // [len] - First → second
    const Triple SideB = Difference(Third, First);                                           // [len] - First → third
    const Triple Pivot = Cross(Ray.Direction, SideB);
    const double Determinant = Dot(SideA, Pivot);
    if (std::abs(Determinant) < 1.0e-14) return std::nullopt;
    const double Inverse = 1.0 / Determinant;
    const Triple Offset  = Difference(Ray.Origin, First);
    const double Along   = Dot(Offset, Pivot) * Inverse;                                     // [-] - Barycentric weight of Second
    if (Along < 0.0 || Along > 1.0) return std::nullopt;
    const Triple Lever   = Cross(Offset, SideA);
    const double Across  = Dot(Ray.Direction, Lever) * Inverse;                              // [-] - Barycentric weight of Third
    if (Across < 0.0 || Along + Across > 1.0) return std::nullopt;
    const double Reach   = Dot(SideB, Lever) * Inverse;                                      // [len] - Ray parameter
    if (Reach <= 0.0) return std::nullopt;
    return Reach;
}

// 📝 Closest approach of the ray and a segment: (ray parameter, separation). Standard two-line solve, clamped.
[[nodiscard]] std::pair<double, double> ApproachSegment(const DraftingRay& Ray, const Triple& Start, const Triple& End)
{
    const Triple Span   = Difference(End, Start);
    const Triple Offset = Difference(Ray.Origin, Start);
    const double SpanSquared = Dot(Span, Span);
    const double Coupling    = Dot(Ray.Direction, Span);
    const double RayOffset   = Dot(Ray.Direction, Offset);
    const double SpanOffset  = Dot(Span, Offset);
    const double Denominator = SpanSquared - Coupling * Coupling;                            // [len²] - |d|² = 1
    double Fraction = Denominator > 1.0e-18 ? (SpanOffset - Coupling * RayOffset) / Denominator : 0.0;
    Fraction        = std::clamp(Fraction, 0.0, 1.0);
    const Triple Spot = { Start[0] + Span[0] * Fraction, Start[1] + Span[1] * Fraction, Start[2] + Span[2] * Fraction };
    const double Reach = std::max(0.0, Dot(Difference(Spot, Ray.Origin), Ray.Direction));
    const Triple Foot  = { Ray.Origin[0] + Ray.Direction[0] * Reach, Ray.Origin[1] + Ray.Direction[1] * Reach, Ray.Origin[2] + Ray.Direction[2] * Reach };
    const Triple Gap   = Difference(Spot, Foot);
    return { Reach, std::sqrt(Dot(Gap, Gap)) };
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

DraftingDeviationMap MeasureDraftingDeviation(const DraftingDocument& Document)
{
    DraftingDeviationMap Map;
    Map.Revision = Document.Revision;
    if (Document.Regeneration.Panels.empty()) return Map;
    constexpr int Grid = 8;                                                                  // 9 × 9 seeds per panel
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::SpaceCurve) continue;
        for (const Vector3& Point : Step.Points)
        {
            DraftingDeviationNeedle Needle;
            Needle.Point    = Point;
            Needle.Distance = std::numeric_limits<double>::infinity();
            for (const auto& [Name, Surface] : Document.Regeneration.Panels)
            {
                const NurbsDomain AlongU = MeasureNurbsSurfaceDomain(*Surface.Definition, true);
                const NurbsDomain AlongV = MeasureNurbsSurfaceDomain(*Surface.Definition, false);
                Vector2 Seed  = { 0.5, 0.5 };
                double  Least = std::numeric_limits<double>::infinity();
                for (int First = 0; First <= Grid; ++First)
                {
                    for (int Second = 0; Second <= Grid; ++Second)
                    {
                        const Vector2 Parameter = { AlongU.Low + (AlongU.High - AlongU.Low) * First / Grid, AlongV.Low + (AlongV.High - AlongV.Low) * Second / Grid };
                        const double  Gap       = Length(EvaluateNurbsSurface(Surface, Parameter).Position - Point);
                        if (Gap < Least) { Least = Gap; Seed = Parameter; }
                    }
                }
                const Vector3 Foot = EvaluateNurbsSurface(Surface, ProjectOntoNurbsSurface(Surface, Point, Seed)).Position;
                const double  Gap  = Length(Foot - Point);
                if (Gap < Needle.Distance) { Needle.Distance = Gap; Needle.Foot = Foot; }
            }
            Map.Worst = std::max(Map.Worst, Needle.Distance);
            Map.Mean += Needle.Distance;
            Map.Needles.push_back(Needle);
        }
    }
    if (!Map.Needles.empty()) Map.Mean /= double(Map.Needles.size());
    return Map;
}

double DraftingZebraPhase(Vector3 Normal, Vector3 Forward, Vector3 Up, double Stripes) noexcept
{
    const Vector3 Reflected = Forward - Normal * (2.0 * Dot(Forward, Normal));
    return Stripes * std::acos(std::clamp(Dot(Reflected, Up), -1.0, 1.0)) / std::numbers::pi;
}

DraftingScene BuildDraftingScene(const DraftingDocument& Document, double SagRatio, bool FineCondition)
{
    const auto    Start = std::chrono::steady_clock::now();
    DraftingScene Scene;
    static std::atomic<uint64_t> BuildCounter{0};
    Scene.Revision   = Document.Revision;
    Scene.Generation = ++BuildCounter;
    if (!Document.Summary.PresentCondition || !Document.Partition) return Scene;
    const ModelPartition& Partition = *Document.Partition;

    TessellationSettings Settings;
    Settings.SagRatio = SagRatio;
    if (FineCondition) Settings.EdgeLengthRatio = FineEdgeRatio;
    bool BoundsCondition = false;
    for (const RegeneratedBody& Regenerated : Document.Regeneration.Bodies)
    {
        if (!Regenerated.Body.PresentCondition() || !Partition.Bodies.Authenticate(Regenerated.Body)) continue;
        const TessellationOutcome Outcome = TessellateBody(Partition, Regenerated.Body, Settings);
        Scene.Sag = std::max(Scene.Sag, Outcome.Sag);
        if (!Outcome.Refusal.empty()) Scene.Refusal += (Scene.Refusal.empty() ? "" : "; ") + Outcome.Refusal;
        const PolygonComplex&  Complex   = Outcome.Complex;
        const BodyNaming&      Naming    = Regenerated.Naming;
        const uint32_t         BodyIndex = static_cast<uint32_t>(Scene.Bodies.size());
        DraftingSceneBody      Listed;
        Listed.Key       = Regenerated.Key;
        Listed.Caption   = DraftingBodyCaption(Document, Regenerated.Key);
        Listed.FirstFace = static_cast<uint32_t>(Scene.Faces.size());
        Listed.FirstEdge = static_cast<uint32_t>(Scene.Edges.size());

        // ① faces: three corners per triangle, exact surface normals, facet normal where the chart normal disagrees
        for (const PolygonComplexFace& Patch : Complex.Faces)
        {
            DraftingSceneFace Face;
            const auto Named  = Naming.Faces.find(Patch.Face.Slot);
            Face.Name         = Named != Naming.Faces.end() ? Named->second : "Face" + std::to_string(Patch.Face.Slot);
            Face.Body         = BodyIndex;
            Face.Slot         = Patch.Face.Slot;
            Face.FirstCorner  = static_cast<uint32_t>(Scene.Corners.size());
            const FaceProfile& Profile = Partition.Faces.Resolve(Patch.Face);
            const bool CarrierCondition = Profile.Surface.PresentCondition();
            const double Sense = Profile.Sense == Orientation::Forward ? 1.0 : -1.0;
            if (CarrierCondition) Face.Surface = SurfaceCategoryLabel(Partition.Surfaces.Resolve(Profile.Surface));
            const uint32_t FaceIndex = static_cast<uint32_t>(Scene.Faces.size());
            for (int Triangle = Patch.FirstTriangle; Triangle < Patch.FirstTriangle + Patch.TriangleCount; ++Triangle)
            {
                const std::array<int, 3>& Corner = Complex.Triangles[Triangle];
                const Triple First  = Widen(Complex.Positions[Corner[0]]);
                const Triple Second = Widen(Complex.Positions[Corner[1]]);
                const Triple Third  = Widen(Complex.Positions[Corner[2]]);
                const Triple Facet  = Cross(Difference(Second, First), Difference(Third, First));
                Face.Area          += 0.5 * std::sqrt(Dot(Facet, Facet));
                const Triple FacetNormal = Normalised(Facet);
                for (int Index = 0; Index < 3; ++Index)
                {
                    Triple Normal = FacetNormal;
                    if (CarrierCondition)
                    {
                        const Vector2 Chart = Complex.TriangleCharts[Triangle][Index];
                        Triple Exact = Widen(EvaluateSurfaceNormal(Partition.Surfaces.Resolve(Profile.Surface), Chart));
                        Exact        = { Exact[0] * Sense, Exact[1] * Sense, Exact[2] * Sense };
                        if (std::isfinite(Exact[0]) && Dot(Exact, FacetNormal) > 0.25) Normal = Normalised(Exact);
                    }
                    DraftingSceneCorner Packed;
                    Narrow(Widen(Complex.Positions[Corner[Index]]), Packed.Position);
                    Narrow(Normal, Packed.Normal);
                    Packed.Face = FaceIndex;
                    Scene.Corners.push_back(Packed);
                }
            }
            Face.CornerCount = static_cast<uint32_t>(Scene.Corners.size()) - Face.FirstCorner;
            Scene.Faces.push_back(std::move(Face));
        }

        // ② edges: the shared samplings, so edges sit exactly on the face boundaries
        for (const PolygonComplexEdge& Sampled : Complex.Edges)
        {
            DraftingSceneEdge Edge;
            const auto Named   = Naming.Edges.find(Sampled.Edge.Slot);
            Edge.Name          = Named != Naming.Edges.end() ? Named->second : "Edge" + std::to_string(Sampled.Edge.Slot);
            Edge.Body          = BodyIndex;
            Edge.Slot          = Sampled.Edge.Slot;
            Edge.FirstSegment  = static_cast<uint32_t>(Scene.Segments.size());
            const EdgeProfile& Profile = Partition.Edges.Resolve(Sampled.Edge);
            if (Profile.Curve.PresentCondition()) Edge.Curve = CurveCategoryLabel(Partition.Curves.Resolve(Profile.Curve));
            const uint32_t EdgeIndex = static_cast<uint32_t>(Scene.Edges.size());
            for (size_t Index = 1; Index < Sampled.Vertices.size(); ++Index)
            {
                const Triple From = Widen(Complex.Positions[Sampled.Vertices[Index - 1]]);
                const Triple To   = Widen(Complex.Positions[Sampled.Vertices[Index]]);
                const Triple Span = Difference(To, From);
                Edge.Length      += std::sqrt(Dot(Span, Span));
                DraftingSceneSegment Packed;
                Narrow(From, Packed.Start);
                Narrow(To, Packed.End);
                Packed.Edge = EdgeIndex;
                Scene.Segments.push_back(Packed);
            }
            Edge.SegmentCount = static_cast<uint32_t>(Scene.Segments.size()) - Edge.FirstSegment;
            Scene.Edges.push_back(std::move(Edge));
        }

        // ③ bounds: the union over every body
        if (!Complex.Positions.empty())
        {
            if (!BoundsCondition)
            {
                Scene.BoundsLow  = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };
                Scene.BoundsHigh = { std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest() };
                BoundsCondition  = true;
            }
            for (const Vector3& Position : Complex.Positions)
            {
                const Triple Point = Widen(Position);
                for (int Axis = 0; Axis < 3; ++Axis)
                {
                    Scene.BoundsLow[Axis]  = std::min(Scene.BoundsLow[Axis], Point[Axis]);
                    Scene.BoundsHigh[Axis] = std::max(Scene.BoundsHigh[Axis], Point[Axis]);
                }
            }
        }
        Listed.FaceCount = static_cast<uint32_t>(Scene.Faces.size()) - Listed.FirstFace;
        Listed.EdgeCount = static_cast<uint32_t>(Scene.Edges.size()) - Listed.FirstEdge;
        Scene.Bodies.push_back(std::move(Listed));
    }
    Scene.Milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
    return Scene;
}

std::optional<DraftingPick> PickDraftingScene(const DraftingScene& Scene, const DraftingRay& Ray, double EdgeReach)
{
    std::optional<DraftingPick> Face;
    for (size_t Corner = 0; Corner + 2 < Scene.Corners.size(); Corner += 3)
    {
        const std::optional<double> Hit = IntersectTriangle(Ray, Widen(Scene.Corners[Corner].Position),
                                                            Widen(Scene.Corners[Corner + 1].Position), Widen(Scene.Corners[Corner + 2].Position));
        if (Hit && (!Face || *Hit < Face->Distance)) Face = DraftingPick{ Scene.Corners[Corner].Face + 1u, *Hit };
    }
    std::optional<DraftingPick> Edge;
    double                      Closest = EdgeReach;                                         // [len] - Best separation so far
    for (const DraftingSceneSegment& Segment : Scene.Segments)
    {
        const auto [Reach, Separation] = ApproachSegment(Ray, Widen(Segment.Start), Widen(Segment.End));
        if (Separation > Closest || Reach <= 0.0) continue;
        if (Face && Reach > Face->Distance + EdgeReach) continue;
        Closest = Separation;
        Edge    = DraftingPick{ IdentityEdgeBit | (Segment.Edge + 1u), Reach };
    }
    return Edge ? Edge : Face;
}

DraftingSceneStates ComposeDraftingStates(const DraftingScene& Scene, const DraftingSelection& Selection, uint32_t HoverIdentity)
{
    DraftingSceneStates States;
    States.Faces.assign(Scene.Faces.size(), 0u);
    States.Edges.assign(Scene.Edges.size(), 0u);
    const auto Mark = [](const auto& Table, std::vector<uint32_t>& Words, const std::vector<std::string>& Names, bool ActiveCondition)
    {
        for (size_t Index = 0; Index < Table.size(); ++Index)
        {
            const auto Found = std::find(Names.begin(), Names.end(), Table[Index].Name);
            if (Found == Names.end()) continue;
            Words[Index] |= 1u;
            if (ActiveCondition && Found == Names.end() - 1) Words[Index] |= 2u;
        }
    };
    Mark(Scene.Faces, States.Faces, Selection.Faces, Selection.LastKind == DraftingSelectionKind::Face);
    Mark(Scene.Edges, States.Edges, Selection.Edges, Selection.LastKind == DraftingSelectionKind::Edge);
    for (const DraftingSceneBody& Body : Scene.Bodies)
    {
        const auto Found = std::find(Selection.Bodies.begin(), Selection.Bodies.end(), Body.Key);
        if (Found == Selection.Bodies.end()) continue;
        const uint32_t Word = 1u | (Selection.LastKind == DraftingSelectionKind::Body && Found == Selection.Bodies.end() - 1 ? 2u : 0u);
        for (uint32_t Index = Body.FirstFace; Index < Body.FirstFace + Body.FaceCount; ++Index) States.Faces[Index] |= Word;
        for (uint32_t Index = Body.FirstEdge; Index < Body.FirstEdge + Body.EdgeCount; ++Index) States.Edges[Index] |= Word;
    }
    if (HoverIdentity != 0u)
    {
        const uint32_t Index = (HoverIdentity & ~IdentityEdgeBit) - 1u;
        std::vector<uint32_t>& Words = (HoverIdentity & IdentityEdgeBit) ? States.Edges : States.Faces;
        if (Index < Words.size()) Words[Index] |= 4u;
    }
    return States;
}

DraftingEdgeApproach ApproachDraftingEdge(const DraftingScene& Scene, const DraftingRay& Ray, uint32_t EdgeIndex)
{
    DraftingEdgeApproach Best;
    if (EdgeIndex >= Scene.Edges.size()) return Best;
    const DraftingSceneEdge& Edge = Scene.Edges[EdgeIndex];
    for (uint32_t Index = Edge.FirstSegment; Index < Edge.FirstSegment + Edge.SegmentCount; ++Index)
    {
        const auto [Reach, Separation] = ApproachSegment(Ray, Widen(Scene.Segments[Index].Start), Widen(Scene.Segments[Index].End));
        if (Separation < Best.Separation) Best = { Reach, Separation };
    }
    return Best;
}

std::string NameDraftingIdentity(const DraftingScene& Scene, uint32_t Identity)
{
    if (Identity == 0) return {};
    const uint32_t Index = (Identity & ~IdentityEdgeBit) - 1u;
    if (Identity & IdentityEdgeBit) return Index < Scene.Edges.size() ? Scene.Edges[Index].Name : std::string{};
    return Index < Scene.Faces.size() ? Scene.Faces[Index].Name : std::string{};
}

DraftingSelectionKind DraftingIdentityKind(uint32_t Identity) noexcept
{
    return (Identity & IdentityEdgeBit) ? DraftingSelectionKind::Edge : DraftingSelectionKind::Face;
}

std::vector<uint32_t> ComposeDraftingMatcapLayers(const DraftingScene& Scene, const DraftingAppearance& Appearance, const DraftingMatcapLibrary& Library)
{
    std::vector<uint32_t> Layers(Scene.Faces.size(), 0u);
    for (size_t Index = 0; Index < Scene.Faces.size(); ++Index)
    {
        const DraftingSceneFace& Face = Scene.Faces[Index];
        const std::string&       Key  = Face.Body < Scene.Bodies.size() ? Scene.Bodies[Face.Body].Key : std::string{};
        Layers[Index] = DraftingMatcapLayer(Library, DraftingFaceMatcap(Appearance, Key, Face.Name)) << DraftingMatcapLayerShift;
    }
    return Layers;
}

size_t CountDanglingMatcaps(const DraftingScene& Scene, const DraftingAppearance& Appearance)
{
    std::set<std::string> Present;
    for (const DraftingSceneFace& Face : Scene.Faces) Present.insert(Face.Name);
    size_t Dangling = 0;
    for (const auto& Entry : Appearance.FaceMatcaps) Dangling += Present.contains(Entry.first) ? 0 : 1;
    std::set<std::string> Keys;
    for (const DraftingSceneBody& Body : Scene.Bodies) Keys.insert(Body.Key);
    for (const auto& Entry : Appearance.BodyMatcaps) Dangling += Keys.contains(Entry.first) ? 0 : 1;
    return Dangling;
}

} // namespace Frontier::DraftingWorkspace
