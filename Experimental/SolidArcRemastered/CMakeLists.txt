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
    Source/Topology/ModelPartition.cpp
    Source/Topology/EulerEdits.cpp
    Source/Construction/PrimitiveConstruction.cpp
    Source/Construction/BodyAssembly.cpp
    Source/Blend/RollingBallGeometry.cpp
    Source/Blend/EdgeBlend.cpp
    Source/Blend/BlendCatalogue.cpp
    Source/Local/FaceChange.cpp
    Source/Local/FaceOperations.cpp
    Source/Local/TopologyRebuild.cpp
    Source/Local/FaceOperationCatalogue.cpp
    Source/Inspection/BlendEncoding.cpp
    Source/Audit/EnclosedVolume.cpp
    Source/Audit/ModelAudit.cpp
    Source/Audit/IntersectionAudit.cpp
    Source/Audit/TessellationAudit.cpp
    Source/Inspection/SpecimenCatalogue.cpp
    Source/Inspection/InspectionEncoding.cpp
    Source/Inspection/BooleanEncoding.cpp
    Source/Inspection/FaceOperationEncoding.cpp
    Source/Inspection/IntersectionCatalogue.cpp
    Source/Inspection/IntersectionEncoding.cpp
    Source/Inspection/TessellationEncoding.cpp
)
target_include_directories(SolidArcRemastered PUBLIC Source)

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

enable_testing()
add_test(NAME TopologyValidation  COMMAND TopologyValidation)
add_test(NAME PredicateValidation COMMAND PredicateValidation)
add_test(NAME IntersectionValidation COMMAND IntersectionValidation)
add_test(NAME ContainmentValidation COMMAND ContainmentValidation)
add_test(NAME BooleanValidation COMMAND BooleanValidation)
add_test(NAME TessellationValidation COMMAND TessellationValidation)
add_test(NAME BlendValidation COMMAND BlendValidation)
add_test(NAME FaceOperationValidation COMMAND FaceOperationValidation)
