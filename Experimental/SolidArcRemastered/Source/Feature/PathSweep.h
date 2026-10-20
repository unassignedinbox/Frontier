/*====================================================================================================================================
                                                          PATHSWEEP.H
====================================================================================================================================*/
// 🧩 Path verbs reduced to exact constructions — pipe along a line / arc, sweep along a line, loft of coaxial circles (G2)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Feature/ProfileSweep.h"

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Circle of Radius swept along Path over τ ∈ [Low, High], the section ⟂ the path: a line gives a cylinder, an arc a
//    partial torus (= the section circle revolved about the arc axis by High − Low; plan G2). Other paths 🚧.
[[nodiscard]] SweepOutcome PipeAlongPath(ModelPartition& Partition, const CurveGeometry& Path, double Low, double High, double Radius);

// Profile on Plane swept along Displacement: a displacement ∥ the plane normal is an extrude; oblique ones 🚧.
[[nodiscard]] SweepOutcome SweepProfileAlongLine(ModelPartition& Partition, const SketchProfile& Profile, const OrthonormalFrame& Plane, Vector3 Displacement);

// 💡 Ruled loft between two circles: coaxial ones (both normals ∥ the centre line) give a cylinder or a frustum.
//    Others are skinned surfaces outside the carrier kinds (🚧).
[[nodiscard]] SweepOutcome LoftCircles(ModelPartition& Partition, Vector3 FirstCentre, Vector3 FirstNormal, double FirstRadius,
                                       Vector3 SecondCentre, Vector3 SecondNormal, double SecondRadius);

} // namespace Frontier::SolidArcRemastered
