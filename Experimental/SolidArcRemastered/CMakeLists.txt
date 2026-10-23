#=====================================================================================================================================
#                                                        CMAKELISTS.TXT
#=====================================================================================================================================
# 🧩 SolidArcRemastered — Parasolid-class B-rep rewrite (steps 1–2: precision, geometry, topology, Euler edits, audit, intersection)

cmake_minimum_required(VERSION 3.20)
project(SolidArcRemastered LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# 📝 Identifiers use Greek / mathematical Unicode (SKILL-formatting § 4): MSVC needs /utf-8.
if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

#-------------------------------------------------------------------------------------------------------------------
#                                                    LIBRARY
#-------------------------------------------------------------------------------------------------------------------

add_library(SolidArcRemastered STATIC
    Source/Precision/ExactPredicates.cpp
    Source/Geometry/CurveGeometry.cpp
    Source/Geometry/SurfaceGeometry.cpp
    Source/Geometry/RollingBallCarrier.cpp
    Source/Geometry/NurbsCarrier.cpp
    Source/Geometry/ParallelCarrier.cpp
    Source/Geometry/ProjectedCurve.cpp
    Source/Containment/FaceDomain.cpp
    Source/Containment/WindingNumber.cpp
    Source/Tessellation/ChartTriangulation.cpp
    Source/Tessellation/BodyTessellation.cpp
    Source/Boolean/GeneralFuse.cpp
    Source/Boolean/BooleanCatalogue.cpp
    Source/Intersection/BernsteinPolynomial.cpp
    Source/Intersection/ImplicitSurface.cpp
    Source/Intersection/CurveSurfaceIntersection.cpp
    Source/Intersection/IsoParameterCurve.cpp
    Source/Intersection/SurfaceIntersection.cpp
    Source/Intersection/IntersectionCurve.cpp
    Source/Topology/ModelPartition.cpp
    Source/Topology/EulerEdits.cpp
    Source/Construction/PrimitiveConstruction.cpp
    Source/Construction/BodyAssembly.cpp
    Source/Construction/BodyTransform.cpp
    Source/Blend/RollingBallGeometry.cpp
    Source/Blend/FairPatch.cpp
    Source/Blend/EdgeBlend.cpp
    Source/Blend/BlendCatalogue.cpp
    Source/Local/FaceChange.cpp
    Source/Local/FaceOperations.cpp
    Source/Local/TopologyRebuild.cpp
    Source/Local/RedundantTopology.cpp
    Source/Local/FaceOperationCatalogue.cpp
    Source/Sketch/SketchSolver.cpp
    Source/Sketch/SketchProfile.cpp
    Source/Sketch/SketchSpline.cpp
    Source/Sketch/SketchCatalogue.cpp
    Source/Sketch/SketchTrim.cpp
    Source/Sketch/SketchEditing.cpp
    Source/Sketch/SketchConstraining.cpp
    Source/Feature/ProfileSweep.cpp
    Source/Feature/BodyCut.cpp
    Source/Feature/PathSweep.cpp
    Source/Feature/ReplayDigest.cpp
    Source/Feature/TopologicalNaming.cpp
    Source/Feature/FeatureTree.cpp
    Source/Feature/FeatureCatalogue.cpp
    Source/Surfacing/LinearSystem.cpp
    Source/Surfacing/CurveFitting.cpp
    Source/Surfacing/SectionLoft.cpp
    Source/Surfacing/SurfaceAlgebra.cpp
    Source/Surfacing/CurveNetwork.cpp
    Source/Surfacing/PanelAssembly.cpp
    Source/Surfacing/CarrierApproximation.cpp
    Source/Inspection/BlendEncoding.cpp
    Source/Audit/EnclosedVolume.cpp
    Source/Assembly/AssemblyAnalysis.cpp
    Source/Assembly/JointSolver.cpp
    Source/Measurement/MassProperties.cpp
    Source/Measurement/MinimumDistance.cpp
    Source/Measurement/PlaneSection.cpp
    Source/Drawing/DrawingProjection.cpp
    Source/Drawing/DrawingSheet.cpp
    Source/Drawing/DrawingExport.cpp
    Source/Exchange/StlExchange.cpp
    Source/Audit/ModelAudit.cpp
    Source/Audit/IntersectionAudit.cpp
    Source/Audit/TessellationAudit.cpp
    Source/Inspection/SpecimenCatalogue.cpp
    Source/Inspection/InspectionEncoding.cpp
    Source/Inspection/BooleanEncoding.cpp
    Source/Inspection/FaceOperationEncoding.cpp
    Source/Inspection/FeatureEncoding.cpp
    Source/Inspection/IntersectionCatalogue.cpp
    Source/Inspection/IntersectionEncoding.cpp
    Source/Inspection/TessellationEncoding.cpp
)
target_include_directories(SolidArcRemastered PUBLIC Source)

# 📝 Embedded by another project (Experimental/DraftingWorkspace adds this folder): the library only, no tools or tests.
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    return()
endif()

#-------------------------------------------------------------------------------------------------------------------
#                                                    EXECUTABLES
#-------------------------------------------------------------------------------------------------------------------

add_executable(RemasteredInspection Source/Applications/RemasteredInspection.cpp)
target_link_libraries(RemasteredInspection PRIVATE SolidArcRemastered)

add_executable(TopologyValidation Validation/TopologyValidation.cpp)
target_link_libraries(TopologyValidation PRIVATE SolidArcRemastered)

add_executable(PredicateValidation Validation/PredicateValidation.cpp)
target_link_libraries(PredicateValidation PRIVATE SolidArcRemastered)

add_executable(IntersectionValidation Validation/IntersectionValidation.cpp)
target_link_libraries(IntersectionValidation PRIVATE SolidArcRemastered)

add_executable(ContainmentValidation Validation/ContainmentValidation.cpp)
target_link_libraries(ContainmentValidation PRIVATE SolidArcRemastered)

add_executable(BooleanValidation Validation/BooleanValidation.cpp)
target_link_libraries(BooleanValidation PRIVATE SolidArcRemastered)

add_executable(TessellationValidation Validation/TessellationValidation.cpp)
target_link_libraries(TessellationValidation PRIVATE SolidArcRemastered)

add_executable(BlendValidation Validation/BlendValidation.cpp)
target_link_libraries(BlendValidation PRIVATE SolidArcRemastered)

add_executable(FaceOperationValidation Validation/FaceOperationValidation.cpp)
target_link_libraries(FaceOperationValidation PRIVATE SolidArcRemastered)
add_executable(FeatureValidation Validation/FeatureValidation.cpp)
target_link_libraries(FeatureValidation PRIVATE SolidArcRemastered)

add_executable(SketchValidation Validation/SketchValidation.cpp)
target_link_libraries(SketchValidation PRIVATE SolidArcRemastered)
add_executable(SketchTrimValidation Validation/SketchTrimValidation.cpp)
target_link_libraries(SketchTrimValidation PRIVATE SolidArcRemastered)
add_executable(SketchEditingValidation Validation/SketchEditingValidation.cpp)
target_link_libraries(SketchEditingValidation PRIVATE SolidArcRemastered)
add_executable(SketchConstrainingValidation Validation/SketchConstrainingValidation.cpp)
target_link_libraries(SketchConstrainingValidation PRIVATE SolidArcRemastered)
add_executable(BodyEditingValidation Validation/BodyEditingValidation.cpp)
target_link_libraries(BodyEditingValidation PRIVATE SolidArcRemastered)
add_executable(SplineValidation Validation/SplineValidation.cpp)
target_link_libraries(SplineValidation PRIVATE SolidArcRemastered)

add_executable(CorpusValidation Validation/CorpusValidation.cpp)
target_link_libraries(CorpusValidation PRIVATE SolidArcRemastered)

add_executable(PatternStressValidation Validation/PatternStressValidation.cpp)
target_link_libraries(PatternStressValidation PRIVATE SolidArcRemastered)

add_executable(RollingBallValidation Validation/RollingBallValidation.cpp)
target_link_libraries(RollingBallValidation PRIVATE SolidArcRemastered)
add_executable(SurfacingValidation Validation/SurfacingValidation.cpp)
target_link_libraries(SurfacingValidation PRIVATE SolidArcRemastered)
add_executable(PanelSurfacingValidation Validation/PanelSurfacingValidation.cpp)
target_link_libraries(PanelSurfacingValidation PRIVATE SolidArcRemastered)
add_executable(NurbsValidation Validation/NurbsValidation.cpp)
target_link_libraries(NurbsValidation PRIVATE SolidArcRemastered)
add_executable(SetbackValidation Validation/SetbackValidation.cpp)
target_link_libraries(SetbackValidation PRIVATE SolidArcRemastered)
add_executable(BodyCutValidation Validation/BodyCutValidation.cpp)
target_link_libraries(BodyCutValidation PRIVATE SolidArcRemastered)
add_executable(SweepValidation Validation/SweepValidation.cpp)
target_link_libraries(SweepValidation PRIVATE SolidArcRemastered)
add_executable(ReplayValidation Validation/ReplayValidation.cpp)
target_link_libraries(ReplayValidation PRIVATE SolidArcRemastered)
add_executable(MeasureValidation Validation/MeasureValidation.cpp)
target_link_libraries(MeasureValidation PRIVATE SolidArcRemastered)
add_executable(AssemblyValidation Validation/AssemblyValidation.cpp)
target_link_libraries(AssemblyValidation PRIVATE SolidArcRemastered)
add_executable(DrawingValidation Validation/DrawingValidation.cpp)
target_link_libraries(DrawingValidation PRIVATE SolidArcRemastered)
add_executable(ExchangeValidation Validation/ExchangeValidation.cpp)
target_link_libraries(ExchangeValidation PRIVATE SolidArcRemastered)
add_executable(FreeformBlendValidation Validation/FreeformBlendValidation.cpp)
target_link_libraries(FreeformBlendValidation PRIVATE SolidArcRemastered)

enable_testing()
add_test(NAME TopologyValidation  COMMAND TopologyValidation)
add_test(NAME PredicateValidation COMMAND PredicateValidation)
add_test(NAME IntersectionValidation COMMAND IntersectionValidation)
add_test(NAME ContainmentValidation COMMAND ContainmentValidation)
add_test(NAME BooleanValidation COMMAND BooleanValidation)
add_test(NAME TessellationValidation COMMAND TessellationValidation)
add_test(NAME BlendValidation COMMAND BlendValidation)
add_test(NAME FaceOperationValidation COMMAND FaceOperationValidation)
add_test(NAME SketchValidation COMMAND SketchValidation)
add_test(NAME SketchTrimValidation COMMAND SketchTrimValidation)
add_test(NAME SketchEditingValidation COMMAND SketchEditingValidation)
add_test(NAME SketchConstrainingValidation COMMAND SketchConstrainingValidation)
add_test(NAME BodyEditingValidation COMMAND BodyEditingValidation)
add_test(NAME SplineValidation COMMAND SplineValidation)
add_test(NAME FeatureValidation COMMAND FeatureValidation)
add_test(NAME CorpusValidation COMMAND CorpusValidation)
add_test(NAME RollingBallValidation COMMAND RollingBallValidation)
add_test(NAME SurfacingValidation COMMAND SurfacingValidation)
add_test(NAME PanelSurfacingValidation COMMAND PanelSurfacingValidation)
add_test(NAME NurbsValidation COMMAND NurbsValidation)
add_test(NAME SetbackValidation COMMAND SetbackValidation)
add_test(NAME BodyCutValidation COMMAND BodyCutValidation)
add_test(NAME SweepValidation COMMAND SweepValidation)
add_test(NAME ReplayValidation COMMAND ReplayValidation)
add_test(NAME MeasureValidation COMMAND MeasureValidation)
add_test(NAME AssemblyValidation COMMAND AssemblyValidation)
add_test(NAME DrawingValidation COMMAND DrawingValidation)
add_test(NAME ExchangeValidation COMMAND ExchangeValidation)
add_test(NAME FreeformBlendValidation COMMAND FreeformBlendValidation)
add_test(NAME PatternStressValidation COMMAND PatternStressValidation)
