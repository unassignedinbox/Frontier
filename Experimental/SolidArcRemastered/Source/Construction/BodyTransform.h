/*====================================================================================================================================
                                                        BODYTRANSFORM.H
====================================================================================================================================*/
// 🧩 Transformed copies of a body — move, rotate, mirror, scale and arrays (G2), exact on every carrier kind it accepts

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Topology/ModelPartition.h"

#include <string>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 x ↦ M·x + Shift with M given by its columns (the images of the unit axes).
struct AffineMap
{
    Vector3 ColumnX = { 1.0, 0.0, 0.0 };                                                     // [-]   - M·x̂
    Vector3 ColumnY = { 0.0, 1.0, 0.0 };                                                     // [-]   - M·ŷ
    Vector3 ColumnZ = { 0.0, 0.0, 1.0 };                                                     // [-]   - M·ẑ
    Vector3 Shift   = {};                                                                    // [len] - Translation
};

struct TransformOutcome
{
    BodyToken   Body    = {};                                                                // [-] - New body (the source is untouched)
    std::string Refusal = {};                                                                // [-] - "Transform: …" when refused
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] AffineMap TranslationMap(Vector3 Displacement) noexcept;
[[nodiscard]] AffineMap RotationMap(Vector3 AxisPoint, Vector3 AxisDirection, double Angle) noexcept;
[[nodiscard]] AffineMap ReflectionMap(Vector3 PlanePoint, Vector3 PlaneNormal) noexcept;
[[nodiscard]] AffineMap ScalingMap(Vector3 Centre, Vector3 Factors) noexcept;

// 💡 Copies Source through Map into a new body (blueprint assembly). Similarities (M = s·Q, Q orthogonal) are exact on
//    every analytic carrier: frames become X′ = Q·X, Y′ = ±Q·Y (sign of det M), so Z′ = Q·Z, face senses are kept and
//    circle / ellipse parameters are unchanged (line parameters scale by s). A mirror (det M < 0) reverses every loop.
//    Other affine maps are exact on planes and lines only (curved carriers are refused: an ellipse-section cylinder is
//    not a carrier kind). Intersection-curve edges map exactly under similarities (guide, surfaces and knots mapped).
[[nodiscard]] TransformOutcome TransformBody(ModelPartition& Partition, BodyToken Source, const AffineMap& Map);

// As TransformBody, reading Source from Partition and assembling the copy into Target (Map = TranslationMap({}) is a
// plain transfer): composite constructions can be built in a scratch partition and only the result kept.
[[nodiscard]] TransformOutcome TransformBodyAcross(const ModelPartition& Partition, BodyToken Source, ModelPartition& Target, const AffineMap& Map);

// Mirror copy across the plane through PlanePoint with normal PlaneNormal.
[[nodiscard]] TransformOutcome MirrorBody(ModelPartition& Partition, BodyToken Source, Vector3 PlanePoint, Vector3 PlaneNormal);

// Linear array: Count − 1 copies at Step, 2·Step, … (the source is the first instance and is not copied).
[[nodiscard]] std::vector<TransformOutcome> ArrayBody(ModelPartition& Partition, BodyToken Source, Vector3 Step, int Count);

} // namespace Frontier::SolidArcRemastered
