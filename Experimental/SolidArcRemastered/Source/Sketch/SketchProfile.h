/*====================================================================================================================================
                                                        SKETCHPROFILE.H
====================================================================================================================================*/
// 🧩 Closed profile loops from a solved sketch: chaining, nesting, orientation (outer CCW, holes CW), exact area

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Sketch/SketchSolver.h"

#include <string>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

enum class ProfileSegmentKind : uint8_t
{
    Line   = 0,                                                                              // Start → End
    Arc    = 1,                                                                              // Centre, StartAngle + Sweep
    Circle = 2,                                                                              // Full turn from angle 0
};

// 📝 One oriented piece of a loop, in traversal order. Arcs: P(φ) = Centre + r·(cos φ, sin φ), φ from StartAngle by
//    Sweep (Sweep > 0: counter-clockwise traversal).
struct ProfileSegment
{
    ProfileSegmentKind Kind       = ProfileSegmentKind::Line;                                // [-]   - Piece type
    Vector2            Start      = {};                                                      // [len] - Traversal start
    Vector2            End        = {};                                                      // [len] - Traversal end
    Vector2            Centre     = {};                                                      // [len] - Arc / circle centre
    double             Radius     = 0.0;                                                     // [len] - Arc / circle radius
    double             StartAngle = 0.0;                                                     // [rad] - φ at Start
    double             Sweep      = 0.0;                                                     // [rad] - Signed angular extent
    int                Curve      = -1;                                                      // [-]   - Sketch curve index
    std::string        Name       = {};                                                      // [-]   - Sketch curve name (Line7)
};

struct ProfileLoop
{
    std::vector<ProfileSegment> Segments   = {};                                             // [-]     - Traversal order
    double                      SignedArea = 0.0;                                            // [len²]  - > 0 outer, < 0 hole (after orientation)
    int                         Depth      = 0;                                              // [-]     - Loops enclosing it (even: outer)
    int                         Enclosure  = -1;                                             // [-]     - Directly enclosing loop (holes)
};

struct SketchProfile
{
    std::vector<ProfileLoop> Loops    = {};                                                  // [-]    - Outer loops and holes
    double                   Area     = 0.0;                                                 // [len²] - Net enclosed area
    Vector2                  Centroid = {};                                                  // [len]  - Area centroid
    double                   Scale    = 1.0;                                                 // [len]  - Characteristic length
    std::string              Refusal  = {};                                                  // [-]    - Why no profile (empty: valid)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Chains the non-construction curves of a solved sketch into closed loops: endpoints within 1e-9·L join, every
//    junction must join exactly two curves (else refused: open or branching), loops must not cross (polyline test on
//    arcs) and are oriented by nesting depth. Area and centroid are exact (Green's theorem; Gauss–Legendre on arcs).
[[nodiscard]] SketchProfile ExtractSketchProfile(const SketchModel& Sketch);

[[nodiscard]] Vector2 EvaluateProfileSegment(const ProfileSegment& Segment, double Fraction) noexcept;

// Segment reversed in place (start ↔ end, sweep negated).
void ReverseProfileSegment(ProfileSegment& Segment) noexcept;

} // namespace Frontier::SolidArcRemastered
