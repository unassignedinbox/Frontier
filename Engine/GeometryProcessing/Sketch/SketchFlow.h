/*====================================================================================================================================
                                                            SKETCHFLOW.H
====================================================================================================================================*/
// 🧩 Flow along curve (Rhino Flow, Blender curve deform): a 2D shape bent so its long axis runs along a rail curve

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/GeometryProcessing/Sketch/SketchProfile.h"
#include "Engine/GeometryProcessing/Sketch/SketchSolver.h"

#include <string>
#include <vector>

namespace Frontier::GeometryProcessing
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

struct SketchFlowSettings
{
    int  AlongAxis        = -1;                                                              // [-] - Shape axis laid on the rail: −1 the longer side · 0 x · 1 y
    bool StretchEnabled   = true;                                                            // [-] - The shape's length fitted to the rail's (else kept, from the rail start)
    bool ReverseCondition = false;                                                           // [-] - Run from the rail's other end
};

struct SketchFlowOutcome
{
    SketchModel Sketch      = {};                                                            // [-]   - The bent shape, in the rail's coordinates
    int         AlongAxis   = 0;                                                             // [-]   - Axis used (0 x · 1 y)
    double      ShapeLength = 0.0;                                                           // [len] - Shape extent along that axis
    double      RailLength  = 0.0;                                                           // [len] - Rail arc length
    int         Lines       = 0;                                                             // [-]   - Curves kept straight (across the axis)
    int         Splines     = 0;                                                             // [-]   - Curves bent into cubic splines
    std::string Refusal     = {};                                                            // [-]   - Why nothing flowed (empty: flowed)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Flow: the shape's box gives the axis (a, b) — a along AlongAxis from the box start, b across from the box middle.
//    The rail (the non-construction curves of Rail chained end to end, one open or closed chain) is measured by arc length
//    s (Gauss–Legendre); a shape point goes to C(s) + b·N(s), s = a·(rail / shape length when stretched), N the rail's left
//    normal, past an open rail's ends along its end tangents. A shape line across the axis stays an exact line; every other
//    curve becomes a cubic spline through dense mapped samples (chord-length parameters); curves sharing an end still share
//    it. Construction curves of the shape are left out. Refused: no shape curves, a shape with no extent along the axis,
//    a rail that is not one chain or has no length.
[[nodiscard]] SketchFlowOutcome FlowSketchModel(const SketchModel& Shape, const SketchModel& Rail, const SketchFlowSettings& Settings);

} // namespace Frontier::GeometryProcessing
