/*====================================================================================================================================
                                                        STEPEXCHANGE.H
====================================================================================================================================*/
// 🧩 STEP exchange (ISO 10303-21 file, AP214 automotive_design schema): solid bodies → MANIFOLD_SOLID_BREP / BREP_WITH_VOIDS
//    with exact analytic carriers (PLANE, CYLINDRICAL / CONICAL / SPHERICAL / TOROIDAL_SURFACE, LINE, CIRCLE, ELLIPSE), exact
//    B-splines for NURBS carriers, and certified cubic B-spline stand-ins for procedural carriers (rolling-ball and parallel
//    surfaces, traced intersection and contact curves), in millimetres

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

// 📝 STEP carries its unit: the file declares millimetres and model coordinates are multiplied by ModelUnitMillimetres.
//    Timestamp goes into FILE_NAME verbatim (empty: a fixed epoch, so the same model always writes the same bytes).
struct StepRequest
{
    double      ModelUnitMillimetres   = 1.0;                                                // [mm]  - Length of one model unit
    double      ApproximationTolerance = 1.0e-6;                                             // [len] - Stand-in deviation bound (model units)
    std::string Name                   = "Frontier";                                         // [-]   - PRODUCT name and file name
    std::string Timestamp              = {};                                                 // [-]   - ISO 8601, FILE_NAME time_stamp
};

struct StepExport
{
    std::string Bytes                = {};                                                   // [-]   - File content
    int         Instances            = 0;                                                    // [-]   - #n written
    int         Solids               = 0;                                                    // [-]   - MANIFOLD_SOLID_BREP / BREP_WITH_VOIDS
    int         Voids                = 0;                                                    // [-]   - Cavity shells
    int         Faces                = 0;                                                    // [-]   - ADVANCED_FACE
    int         Edges                = 0;                                                    // [-]   - EDGE_CURVE
    int         Vertices             = 0;                                                    // [-]   - VERTEX_POINT
    int         ExactSurfaces        = 0;                                                    // [-]   - Analytic or NURBS, written exactly
    int         ApproximatedSurfaces = 0;                                                    // [-]   - Cubic stand-ins
    int         ExactCurves          = 0;                                                    // [-]   - Line / circle / ellipse / NURBS
    int         ApproximatedCurves   = 0;                                                    // [-]   - Cubic stand-ins
    double      Deviation            = 0.0;                                                  // [len] - Largest certified stand-in deviation (model units)
    double      Milliseconds         = 0.0;                                                  // [ms]
    std::string Refusal              = {};                                                   // [-]   - Empty on success
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// Every body must be a solid; each solid region becomes one MANIFOLD_SOLID_BREP (BREP_WITH_VOIDS when it has cavities).
[[nodiscard]] StepExport WriteStep(const ModelPartition& Partition, const std::vector<BodyToken>& Bodies, const StepRequest& Request);

// Writes Export.Bytes to Path; returns a refusal (empty on success).
[[nodiscard]] std::string ExportStepFile(const StepExport& Export, const std::string& Path);

} // namespace Frontier::SolidArcRemastered
