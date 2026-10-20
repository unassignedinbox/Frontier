/*====================================================================================================================================
                                                        WINDINGNUMBER.H
====================================================================================================================================*/
// 🧩 Generalized winding number of trimmed analytic faces: point-in-solid with precision-aware boundary detection

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Containment/FaceDomain.h"
#include "Precision/PrecisionSpecification.h"

#include <cstdint>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

struct WindingSettings
{
    double Precision         = SessionPrecision;                                             // [len] - On-boundary distance
    double OuterTolerance    = 1.0e-10;                                                      // [sr]  - Per Kronrod panel along loops
    double InnerTolerance    = 1.0e-11;                                                      // [-]   - Per Kronrod panel along 𝑢
    int    DepthLimit        = 40;                                                           // [-]   - Bisections per integral
    double NearDistance      = 1.0e-4;                                                       // [len] - Nearest-normal shortcut reach
};

struct WindingTally
{
    int64_t Evaluations = 0;                                                                 // [-] - Surface evaluations
};

enum class ContainmentMethod : uint8_t
{
    Winding       = 0,                                                                       // Σ Ω_F / 4π decided
    NearestNormal = 1,                                                                       // Nearest point is a face interior point
    Proximity     = 2,                                                                       // Within precision of the boundary
};

struct SolidPointClass
{
    PointMembership Membership      = PointMembership::Outside;                              // [-] - Result
    double          Winding         = 0.0;                                                   // [-] - Σ Ω_F / 4π (NaN when on the boundary)
    double          IntegerResidual = 0.0;                                                   // [-] - |w − round(w)|
    int             BoundaryFace    = -1;                                                    // [-] - Face index when on the boundary
    int64_t         Evaluations     = 0;                                                     // [-] - Surface evaluations
    ContainmentMethod Method        = ContainmentMethod::Winding;                            // [-] - What decided
};

// 📝 Prepared faces of a body (or of any face set — an open set gives the fractional winding of a leaky surface).
struct BodyContainment
{
    std::vector<FaceDomain> Faces = {};                                                      // [-] - One domain per face
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] BodyContainment PrepareBodyContainment(const ModelPartition& Partition, BodyToken Body);
[[nodiscard]] BodyContainment PrepareFaceContainment(const ModelPartition& Partition, const std::vector<FaceToken>& Faces);

// 💡 Ω_F(P) = ∬_F (S − P)·(S_𝑢 × S_𝑣) / |S − P|³ d𝑢 d𝑣 = ∮_∂F G d𝑣 with G(𝑢, 𝑣) = ∫_{𝑢ref}^{𝑢} (…)(s, 𝑣) ds (Green in
//    the chart; loop orientation carries the face sense, as in EnclosedVolume). Both integrals are adaptive Gauss–Kronrod
//    7/15, split at the foot of P so the near-surface Lorentzian peak (width ≈ distance) is never straddled.
[[nodiscard]] double MeasureFaceSolidAngle(const FaceDomain& Domain, Vector3 Position, const WindingSettings& Settings, WindingTally& Tally);

// Σ Ω_F / 4π without boundary detection (fractional for open face sets).
[[nodiscard]] double MeasureWindingNumber(const BodyContainment& Containment, Vector3 Position, const WindingSettings& Settings, WindingTally& Tally);

// 💡 ① Proximity: Boundary when within Precision of any face.
//    ② Nearest normal: when the nearest boundary point lies inside a face (closer than NearDistance and closer than every
//       edge), the open segment to it crosses no boundary, so the outward-normal sign decides exactly — this replaces the
//       near-singular quadrature (≈ 10⁸ evaluations at 1e-7) where it is not needed.
//    ③ Otherwise Inside iff the generalized winding number rounds to a non-zero integer.
[[nodiscard]] SolidPointClass ClassifySolidPoint(const BodyContainment& Containment, Vector3 Position, const WindingSettings& Settings = {});

[[nodiscard]] const char* PointMembershipLabel(PointMembership Membership) noexcept;
[[nodiscard]] const char* ContainmentMethodLabel(ContainmentMethod Method) noexcept;

} // namespace Frontier::SolidArcRemastered
