#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Build products stay outside tracked content.
Executable="${TMPDIR:-/tmp}/frontier_control_vehicle_gi"
Mode="${1:---methods}"
case "$Mode" in
    --methods) Output=Exhibits/ControlVehicleGiMethods ;;
    --shadow-methods) Output=Exhibits/ControlVehicleShadowMethods ;;
    --gi) Output=Exhibits/ControlVehicleGiRenders ;;
    *) echo "Use --methods, --shadow-methods, or --gi" >&2; exit 2 ;;
esac
g++ -std=c++20 -O3 -pthread -I. \
    Exhibits/ControlVehicleRenderExhibit.cpp \
    Engine/GeometricRaster/GlobalIllumination/StokesContourGiSequence.cpp \
    Engine/GeometricRaster/GlobalIllumination/SurfelCascadeGiSequence.cpp \
    Engine/GeometricRaster/DirectLighting/SurfelOccluderField.cpp \
    Engine/SpatialAcceleration/WideBoundingTree.cpp \
    Engine/SpatialAcceleration/ClusterPyramid.cpp \
    Engine/GeometrySpace/Analysis/CurvatureAnalysis.cpp \
    Engine/GeometricRaster/Visibility/*.cpp \
    Engine/MaterialSpace/*.cpp \
    Engine/MaterialSpace/EnergyCompensation/*.cpp \
    -o "$Executable"
"$Executable" "$Mode"
for Image in "$Output"/*.ppm; do
    convert "$Image" "${Image%.ppm}.png"
    rm "$Image"
done
case "$Mode" in
    --methods) python3 Automation/BuildVehicleGiComparison.py gi ;;
    --shadow-methods) python3 Automation/BuildVehicleGiComparison.py shadows ;;
esac
