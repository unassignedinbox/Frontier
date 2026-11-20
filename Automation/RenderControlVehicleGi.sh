#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Build products stay outside tracked content.
Executable="${TMPDIR:-/tmp}/frontier_control_vehicle_gi"
g++ -std=c++20 -O3 -pthread -I. \
    Exhibits/ControlVehicleRenderExhibit.cpp \
    Engine/SpatialAcceleration/WideBoundingTree.cpp \
    Engine/SpatialAcceleration/ClusterPyramid.cpp \
    Engine/GeometrySpace/Analysis/CurvatureAnalysis.cpp \
    Engine/GeometricRaster/Visibility/*.cpp \
    Engine/MaterialSpace/*.cpp \
    Engine/MaterialSpace/EnergyCompensation/*.cpp \
    -o "$Executable"
"$Executable" --gi
for Image in Exhibits/ControlVehicleGiRenders/*.ppm; do
    convert "$Image" "${Image%.ppm}.png"
    rm "$Image"
done
