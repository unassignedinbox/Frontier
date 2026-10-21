/*====================================================================================================================================
                                                            SECTIONLOFT.H
====================================================================================================================================*/
// 🧩 Loft (skinning) through section curves: compatible sections → interpolation across them → one exact NURBS sheet face

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Feature/ProfileSweep.h"
#include "Geometry/NurbsCarrier.h"

#include <string>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    TYPES
//------------------------------------------------------------------------------------------------------------------------

struct LoftSettings
{
    int  DegreeAcross    = 3;                                                                // [-] - Degree in 𝑣 (lowered to sections − 1)
    bool AlignCondition  = true;                                                             // [-] - Reverse open sections running against the first
};

struct LoftSurfaceOutcome
{
    NurbsSurface            Surface            = {};                                         // [-] - 𝑢 along the sections, 𝑣 across, both on [0, 1]
    std::vector<NurbsCurve> Sections           = {};                                         // [-] - Compatible sections (same degree and knots)
    std::vector<double>     SectionParameters  = {};                                         // [-] - 𝑣ₖ where section k is interpolated
    int                     Reversed           = 0;                                          // [-] - Sections turned to agree with the first
    bool                    ClosedCondition    = false;                                      // [-] - Every section closed (a tube: one seam)
    std::string             Refusal            = {};                                         // [-] - "Loft: …" when refused
};

struct LoftSheetOutcome
{
    LoftSurfaceOutcome Loft  = {};                                                           // [-] - The carrier and its sections
    SweepOutcome       Sheet = {};                                                           // [-] - Sheet body (one face) and its role
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Skinning (Piegl & Tiller § 10.3): sections mapped onto 𝑢 ∈ [0, 1], made compatible by merging their knot vectors
//    (Boehm insertion — exact), 𝑣ₖ from the averaged chord length of each pole column, averaged 𝑣-knots, then each
//    pole column interpolated in homogeneous coordinates so S(𝑢, 𝑣ₖ) ≡ section k (rational sections stay exact: circles
//    loft into true cylinders and cones). Refuses fewer than 2 sections, periodic carriers, mixed degrees (degree
//    elevation 🚧), mixed open / closed sections, coincident sections, or a non-positive interpolated weight.
[[nodiscard]] LoftSurfaceOutcome LoftSectionSurface(const std::vector<NurbsCurve>& Sections, const LoftSettings& Settings);

// 💡 The loft as a sheet body: one NURBS face bounded by the first and last sections and the two 𝑢-boundary iso-curves
//    (closed sections share one seam edge used twice), every boundary carried by an exact iso-line chart curve.
[[nodiscard]] LoftSheetOutcome LoftSectionSheet(ModelPartition& Partition, const std::vector<NurbsCurve>& Sections, const LoftSettings& Settings);

}
