/*====================================================================================================================================
                                                        ENCLOSEDVOLUME.H
====================================================================================================================================*/
// 🧩 Exact enclosed volume and parametric face area by Green's theorem over SP-curves (no tessellation)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Topology/ModelPartition.h"

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 V = ⅓ ∮∮ P·n dA. Per face, ∬_D f d𝑢 d𝑣 = ∮_∂D G d𝑣 with G(𝑢, 𝑣) = ∫ f(s, 𝑣) ds, integrated along each fin's SP-curve
//    in traversal order. Loop orientation in (𝑢, 𝑣) already carries the face sense, so the fin sum IS the outward flux.
//    Works for any trimmed face with SP-curves — replaces the tessellation volume that SolidArc used as truth.
[[nodiscard]] double IntegrateShellVolume(const ModelPartition& Partition, SolidShellToken Shell);
[[nodiscard]] double IntegrateBodyVolume(const ModelPartition& Partition, BodyToken Body);

// Signed parametric area Σ ∮ 𝑢 d𝑣 over a face's loops: positive when the loops run counter-clockwise in (𝑢, 𝑣).
[[nodiscard]] double IntegrateParametricArea(const ModelPartition& Partition, FaceToken Face);

} // namespace Frontier::SolidArcRemastered
