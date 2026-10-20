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

#include <map>
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
    Revolve       = 2,                                                                       // Sketch profile revolved 360°
    Blend         = 3,                                                                       // Fillet / chamfer on referenced edges
    FaceOperation = 4,                                                                       // Push, offset, draft, … on referenced faces
};

enum class FeatureCombination : uint8_t
{
    New          = 0,                                                                        // First body (later: union)
    Union        = 1,                                                                        // Current ∪ tool
    Difference   = 2,                                                                        // Current − tool
    Intersection = 3,                                                                        // Current ∩ tool
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
    BlendProfile                   Shape          = BlendProfile::Fillet;                    // [-]   - Blend cross-section
    FeatureValue                   Size           = {};                                      // [len] - Blend radius / setback
    FaceOperationKind              Operation      = FaceOperationKind::Push;                 // [-]   - Face-operation verb
    FeatureValue                   Amount         = {};                                      // [len|rad|-] - Distance / angle / factor (by verb)
    std::vector<TopologyReference> References     = {};                                      // [-]   - Blend: edges · face operation: faces
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
    double                         Milliseconds = 0.0;                                       // [ms] - Step time
};

struct RegenerationOutcome
{
    BodyToken                          Body         = {};                                    // [-]  - Final body
    BodyNaming                         Naming       = {};                                    // [-]  - Its persistent names
    std::vector<FeatureStepReport>     Steps        = {};                                    // [-]  - One per step
    std::map<std::string, SketchModel> Sketches     = {};                                    // [-]  - Solved sketch per label
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
