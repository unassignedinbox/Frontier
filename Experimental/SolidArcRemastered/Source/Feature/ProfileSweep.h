/*====================================================================================================================================
                                                         PROFILESWEEP.H
====================================================================================================================================*/
// 🧩 Solids from sketch profiles: linear extrude (planes, cylinders, holes) and full revolve (plane … torus, poles)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Construction/BodyAssembly.h"
#include "Sketch/SketchProfile.h"

#include <string>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

enum class SweepFaceRole : uint8_t
{
    Side     = 0,                                                                            // Swept by one profile curve
    StartCap = 1,                                                                            // Extrude cap on the sketch plane
    EndCap   = 2,                                                                            // Extrude cap at the far end
};

// 📝 Feature-semantic role of a result face (research § L 🥉): which profile curve swept it, or which cap it is.
struct SweepFace
{
    SweepFaceRole Role  = SweepFaceRole::Side;                                               // [-] - Side / cap
    std::string   Curve = {};                                                                // [-] - Sketch curve name (sides)
    int           Loop  = 0;                                                                 // [-] - Profile loop index (caps: outer loop)
};

struct SweepOutcome
{
    BodyToken              Body    = {};                                                     // [-] - New solid
    std::string            Refusal = {};                                                     // [-] - "Extrude: …" / "Revolve: …" when refused
    std::vector<FaceToken> Faces   = {};                                                     // [-] - Result faces
    std::vector<SweepFace> Roles   = {};                                                     // [-] - Role per entry of Faces
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Extrude: the profile on Plane (sketch x → XAxis, y → YAxis) swept along ZAxis·Extent (negative: below the
//    plane). Lines → planar sides, arcs / circles → cylinders (full circles get a seam), one cap pair per outer loop with
//    its holes; multiple outer loops make multiple solid regions.
[[nodiscard]] SweepOutcome ExtrudeProfile(ModelPartition& Partition, const SketchProfile& Profile, const OrthonormalFrame& Plane, double Extent);

// 💡 Revolve 360° about the sketch line through AxisPoint along AxisDirection: segments ⟂ axis → planes (disc /
//    annulus), ∥ axis → cylinders, oblique → cones (apex vertex on the axis), arcs centred on the axis → spheres (poles),
//    other arcs → tori, a full circle off the axis → a closed torus. Segments on the axis vanish; the profile must not
//    cross the axis.
[[nodiscard]] SweepOutcome RevolveProfile(ModelPartition& Partition, const SketchProfile& Profile, const OrthonormalFrame& Plane, Vector2 AxisPoint, Vector2 AxisDirection);

// "Side(Line7)", "Cap(Start)", "Cap(End)".
[[nodiscard]] std::string SweepRoleName(const SweepFace& Role);

} // namespace Frontier::SolidArcRemastered
