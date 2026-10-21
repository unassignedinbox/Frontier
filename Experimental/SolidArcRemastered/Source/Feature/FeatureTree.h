/*====================================================================================================================================
                                                         FEATURETREE.H
====================================================================================================================================*/
// 🧩 Parametric feature tree: parameters, sketches, sweeps, Booleans, blends, face operations; regeneration by replay

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Feature/TopologicalNaming.h"
#include "Sketch/SketchSolver.h"
#include "Surfacing/CurveFitting.h"
#include "Geometry/SurfaceGeometry.h"

#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENUMS
//------------------------------------------------------------------------------------------------------------------------

enum class FeatureStepKind : uint8_t
{
    Sketch        = 0,                                                                       // Constrained 2D sketch on a plane
    Extrude       = 1,                                                                       // Sketch profile swept linearly
    Revolve       = 2,                                                                       // Sketch profile revolved (360° default)
    Blend         = 3,                                                                       // Fillet / chamfer on referenced edges
    FaceOperation = 4,                                                                       // Push, offset, draft, … on referenced faces
    SpaceCurve    = 5,                                                                       // 3D curve fitted to imported points (vehicle plan V3)
    Loft          = 6,                                                                       // Panel skinned through SpaceCurve sections (V4, G1/G2 V6)
    Network       = 7,                                                                       // Panel through a SpaceCurve network, Gordon / Coons (V5)
    Mirror        = 8,                                                                       // Mirror image of every panel so far through a plane (V7)
    Close         = 9,                                                                       // Plane caps on the planar rim loops → solid (V7)
    Combine       = 10,                                                                      // Boolean between two bodies; the tool body is consumed
};

enum class FeatureCombination : uint8_t
{
    New          = 0,                                                                        // First body (later: union with the target)
    Union        = 1,                                                                        // Target ∪ tool
    Difference   = 2,                                                                        // Target − tool
    Intersection = 3,                                                                        // Target ∩ tool
    Separate     = 4,                                                                        // A new body of its own (Plasticity "new body")
};

enum class FeatureStepStatus : uint8_t
{
    Built   = 0,                                                                             // Step applied
    Refused = 1,                                                                             // Step marked; previous body kept
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 A number in the tree: Literal, or Factor·Parameter + SecondFactor·SecondParameter + Offset when Parameter is named.
struct FeatureValue
{
    double      Literal   = 0.0;                                                             // [len|rad|-] - Value without a parameter
    std::string Parameter = {};                                                              // [-]         - Named parameter (empty: literal)
    double      Factor    = 1.0;                                                             // [-]         - Scale on the parameter
    double      Offset    = 0.0;                                                             // [len|rad|-] - Added after scaling
    std::string SecondParameter = {};                                                        // [-]         - Optional second term
    double      SecondFactor    = 0.0;                                                       // [-]         - Scale on the second term
};

struct FeatureParameter
{
    std::string Name        = {};                                                            // [-]         - Key ("Width")
    double      Value       = 0.0;                                                           // [len|rad|-] - Current value
    double      Low         = 0.0;                                                           // [len|rad|-] - Slider range
    double      High        = 1.0;                                                           // [len|rad|-] - Slider range
    std::string Description = {};                                                            // [-]         - One line
};

// 📝 One step; only the fields of its kind are read. Label is the stable naming tag ("Extrude#2").
// 📝 A dragged pole of a SpaceCurve: replaces pole Index of the fitted curve after every re-fit (mode 1 edit).
struct FeaturePoleEdit
{
    int     Index    = 0;                                                                    // [-]   - Pole index in the fitted curve
    Vector3 Position = {};                                                                   // [len] - New pole position
};

struct FeatureStep
{
    FeatureStepKind                Kind           = FeatureStepKind::Sketch;                 // [-]   - Step type
    std::string                    Label          = {};                                      // [-]   - Unique tag, prefix of the names it creates
    SketchModel                    Sketch         = {};                                      // [-]   - Sketch: unsolved model (constraints may bind parameters)
    OrthonormalFrame               Plane          = {};                                      // [-]   - Sketch: plane frame
    FeatureValue                   Elevation      = {};                                      // [len] - Sketch: plane shift along its normal
    std::string                    Profile        = {};                                      // [-]   - Extrude / revolve: sketch step label
    FeatureValue                   Extent         = {};                                      // [len] - Extrude distance (negative: below the plane)
    FeatureCombination             Combination    = FeatureCombination::New;                 // [-]   - Extrude / revolve: Boolean with the current body
    int                            AxisIdentifier = -1;                                      // [-]   - Revolve: identifier of the axis line in the sketch
    FeatureValue                   Angle          = { 2.0 * std::numbers::pi };              // [rad] - Revolve: turn angle in (0, 2π]
    BlendProfile                   Shape          = BlendProfile::Fillet;                    // [-]   - Blend cross-section
    FeatureValue                   Size           = {};                                      // [len] - Blend radius / setback (at the anchors)
    std::optional<FeatureValue>    EndSize        = {};                                      // [len] - Blend: size at the far ends (empty = constant)
    std::vector<TopologyReference> SizeAnchors    = {};                                      // [-]   - Blend (variable): vertices that take Size, one per edge
    FaceOperationKind              Operation      = FaceOperationKind::Push;                 // [-]   - Face-operation verb
    FeatureValue                   Amount         = {};                                      // [len|rad|-] - Distance / angle / factor (by verb)
    std::vector<TopologyReference> References     = {};                                      // [-]   - Blend: edges · face operation: faces
    std::vector<Vector3>           Points         = {};                                      // [len] - SpaceCurve: imported points in order
    bool                           ClosedCondition = false;                                  // [-]   - SpaceCurve: the loop closes (last joins first)
    bool                           PlanarCondition = false;                                  // [-]   - SpaceCurve: fit in the points' best plane (cap sections)
    FeatureValue                   Tolerance      = { 2.0e-4 };                              // [len] - SpaceCurve: fit tolerance (0.2 mm)
    std::vector<FeaturePoleEdit>   PoleEdits      = {};                                      // [-]   - SpaceCurve: dragged poles, applied after the fit
    std::vector<std::string>       Sections       = {};                                      // [-]   - Loft: SpaceCurve labels in order · Network: its curves
    int                            Continuity     = 0;                                       // [-]   - Loft: 0 G0 · 1 G1 · 2 G2 to panels sharing an end section
    std::string                    Target         = {};                                      // [-]   - Extrude / revolve Boolean, blend, face operation, combine: body key (empty = active body)
    std::string                    Tool           = {};                                      // [-]   - Combine: body key consumed by the Boolean
};

struct FeatureTree
{
    std::string                   Name        = {};                                          // [-] - Catalogue key
    std::string                   Description = {};                                          // [-] - One line
    std::vector<FeatureParameter> Parameters  = {};                                          // [-] - Named driving values
    std::vector<FeatureStep>      Steps       = {};                                          // [-] - Replay order
};

struct FeatureStepReport
{
    std::string                    Label        = {};                                        // [-]  - Step tag
    FeatureStepKind                Kind         = FeatureStepKind::Sketch;                   // [-]  - Step type
    FeatureStepStatus              Status       = FeatureStepStatus::Built;                  // [-]  - Built / refused
    std::string                    Refusal      = {};                                        // [-]  - Reason when refused
    std::vector<ResolvedReference> Resolutions  = {};                                        // [-]  - One per reference
    SketchSolveOutcome             Solve        = {};                                        // [-]  - Sketch steps
    BodyToken                      Body         = {};                                        // [-]  - Body after the step (kept one when refused)
    BodyNaming                     Naming       = {};                                        // [-]  - Names of that body
    std::string                    Target       = {};                                        // [-]  - Key of that body (empty before any body)
    double                         Milliseconds = 0.0;                                       // [ms] - Step time
};

// 📝 One body of the regenerated part. Key = label of the step that created it ("Extrude#2", "Loft#12"): stable across
//    regenerations, so appearance and selections can name bodies the way they name faces.
struct RegeneratedBody
{
    std::string Key    = {};                                                                 // [-] - Creating step label
    BodyToken   Body   = {};                                                                 // [-] - Body token
    BodyNaming  Naming = {};                                                                 // [-] - Its persistent names
};

struct RegenerationOutcome
{
    BodyToken                          Body         = {};                                    // [-]  - Active body (the one the last body step built or changed)
    BodyNaming                         Naming       = {};                                    // [-]  - Its persistent names
    std::vector<RegeneratedBody>       Bodies       = {};                                    // [-]  - Every body, in creation order
    std::string                        Active       = {};                                    // [-]  - Key of the active body
    std::vector<FeatureStepReport>     Steps        = {};                                    // [-]  - One per step
    std::map<std::string, SketchModel> Sketches     = {};                                    // [-]  - Solved sketch per label
    std::map<std::string, CurveFitOutcome> Curves   = {};                                    // [-]  - Fitted (and edited) curve per SpaceCurve label
    std::map<std::string, NurbsSurface> Panels      = {};                                    // [-]  - Carrier per panel face name (zebra / deviation)
    int                                Refused      = 0;                                     // [-]  - Steps refused
    double                             Milliseconds = 0.0;                                   // [ms] - Whole replay
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Replays every step from scratch (plan § 6, step 5): sketches are solved with their parameters bound, sweeps are
//    named by role, Booleans / blends / face operations are named from their histories, and references are resolved
//    exact → source → geometric. A refused step (no solution, unresolvable reference, modeller refusal) is marked and the
//    previous body is kept; later steps still run. CaptureCondition (authoring): store each resolved reference's
//    signature in the tree so later regenerations can rebind geometrically.
//    Bodies: the first sweep (or a Separate one) starts a body keyed by its step label; Booleans, blends and face
//    operations act on Step.Target (empty: the active body, the last one built or changed); panels form one body of their
//    own; Combine merges Step.Tool into Step.Target and consumes the tool.
[[nodiscard]] RegenerationOutcome RegenerateFeatureTree(ModelPartition& Partition, FeatureTree& Tree, bool CaptureCondition = false);

[[nodiscard]] double EvaluateFeatureValue(const FeatureTree& Tree, const FeatureValue& Value);

// Sets a parameter; false when no parameter has that name.
bool AssignFeatureParameter(FeatureTree& Tree, const std::string& Name, double Value);

// Shorthands for catalogue authors.
[[nodiscard]] FeatureValue LiteralValue(double Value);
[[nodiscard]] FeatureValue ParameterValue(const std::string& Name, double Factor = 1.0, double Offset = 0.0);
[[nodiscard]] FeatureValue SumValue(const std::string& First, double FirstFactor, const std::string& Second, double SecondFactor, double Offset = 0.0);

[[nodiscard]] const char* FeatureStepKindLabel(FeatureStepKind Kind) noexcept;
[[nodiscard]] const char* FeatureCombinationLabel(FeatureCombination Combination) noexcept;

} // namespace Frontier::SolidArcRemastered
