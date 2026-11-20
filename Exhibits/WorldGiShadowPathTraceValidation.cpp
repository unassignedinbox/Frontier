/*============================================================================================================================================
                                                   WORLDGISHADOWPATHTRACEVALIDATION.CPP
============================================================================================================================================*/
#include "Engine/GeometricRaster/GlobalIllumination/StokesContourGiSequence.h"
#include "Engine/GeometricRaster/GlobalIllumination/SurfelCascadeGiSequence.h"
#include "Engine/GeometricRaster/DirectLighting/SurfelOccluderField.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace Frontier;
using namespace Frontier::GeometricRaster;
using Mathematics::Axis3D;

int main()
{
    unsigned Passed = 0;
    const auto Check = [&](bool Good, const char* Label)
    {
        std::printf("[%s] %s\n", Good ? "PASS" : "FAIL", Label);
        if (!Good) std::exit(1);
        ++Passed;
    };
    const Axis3D P {};
    const Axis3D N { 0, 0, 1 };
    const Axis3D A { -1, -1, 2 }, B { 1, -1, 2 }, C { 0, 1, 2 };
    const float Form = StokesContourGiSequence::EvaluateTriangleFormFactor(P, N, A, B, C);
    Check(Form > 0 && Form < 1, "bounded positive triangle form factor");
    Check(std::abs(Form - StokesContourGiSequence::EvaluateTriangleFormFactor(P, N, C, B, A)) < 1.0e-6f,
        "winding-independent receiver integral");
    Check(StokesContourGiSequence::EvaluateTriangleFormFactor(P, {0,0,-1}, A, B, C) == 0,
        "triangle below receiver horizon rejected");
    const Axis3D Shift { 8, -4, 7 };
    Check(std::abs(Form - StokesContourGiSequence::EvaluateTriangleFormFactor(Shift,N,A+Shift,B+Shift,C+Shift)) < 1e-6f,
        "translation invariance");
    Check(std::abs(Form - StokesContourGiSequence::EvaluateTriangleFormFactor(P,N,A*3,B*3,C*3)) < 1e-6f,
        "solid angle scale invariance");
    // Independent uniform-area quadrature of cos(receiver)*cos(source)/(pi*r^2).
    double Integral = 0;
    constexpr unsigned Count = 400;
    for (unsigned I = 0; I < Count; ++I)
    {
        for (unsigned J = 0; J < Count; ++J)
        {
            const float U = std::sqrt((I+0.5f)/Count);
            const float V = (J+0.5f)/Count;
            const auto Q = A*(1-U) + B*(U*(1-V)) + C*(U*V);
            const float R2 = Mathematics::EvaluateLengthSquared(Q);
            Integral += 4.0 / (3.141592653589793 * R2 * R2) * 2.0 / (Count*Count);
        }
    }
    Check(std::abs(Form - Integral) < 0.0001, "Stokes versus independent area quadrature");
    const float Clipped = StokesContourGiSequence::EvaluateTriangleFormFactor(P,N,{-1,-1,-1},{1,-1,1},{0,1,1});
    Check(std::isfinite(Clipped) && Clipped > 0 && Clipped <= 1, "horizon crossing including entering-edge denominator");
    Check(StokesContourGiSequence::EvaluateTriangleFormFactor(P,N,A,A,A) == 0, "degenerate triangle finite black");
    bool Roundtrip = true;
    for (unsigned I = 0; I < 32; ++I)
    {
        for (unsigned J = 0; J < 32; ++J)
        {
            const auto Direction = SurfelCascadeGiSequence::DecodeOctahedral((I+0.5f)/32,(J+0.5f)/32);
            const auto UV = SurfelCascadeGiSequence::EncodeOctahedral(Direction);
            const auto Decoded = SurfelCascadeGiSequence::DecodeOctahedral(UV.X,UV.Y);
            Roundtrip &= Mathematics::EvaluateDot(Direction,Decoded) > 0.99999f;
        }
    }
    Check(Roundtrip, "1024 octahedral direction round trips");
    SpatialAcceleration::WideBoundingTree Tree;
    SpatialAcceleration::RayTrianglePrimitive Triangle;
    Triangle.Corner0 = A;
    Triangle.Edge01 = C-A;
    Triangle.Edge02 = B-A;
    Triangle.MaterialOrdinal = 0;
    Check(Tree.BuildFromTrianglePrimitives({&Triangle,1}), "real triangle BVH build");
    const Mathematics::LinearRGB Emission {2,1,0.5f}, Reflectance {0.5f,0.5f,0.5f};
    StokesContourGiSequence Stokes;
    Stokes.Prepare(Tree,{&Emission,1},{&Reflectance,1},N,{});
    const auto Irradiance = Stokes.EvaluateIrradianceOverPi(P,N);
    Check(Stokes.Sources.size() == 1 && std::abs(Irradiance.R - Form*2) < 1e-5f,
        "emissive real triangle extraction and visible Stokes radiance");
    Check(std::abs(Irradiance.G - Form) < 1e-5f, "coloured source radiance");
    SurfelCascadeGiSequence Surfel;
    Surfel.Prepare(Stokes,Tree);
    Check(Surfel.Cascades[0].Probes.size() == 252 && Surfel.Cascades[1].Probes.size() == 140,
        "two populated octahedral irradiance cascades");
    const auto Cached = Surfel.EvaluateIrradianceOverPi(P,N);
    Check(std::isfinite(Cached.R) && Cached.R > 0, "surfel probe interpolation yields finite irradiance");
    Check(Surfel.EvaluateIrradianceOverPi({1000,0,0},N).R == 0, "outside cascade finite black");
    SurfelOccluderField Field;
    Field.BuildFromTriangles(Tree);
    Check(!Field.Disks.empty(), "occluder disks sampled from triangles");
    Check(Field.EvaluateSolarVisibility(Tree,{4,0,0},N,N,0.016f) > 0.99f, "open solar visibility");
    const auto Blocked = Field.EvaluateSolarVisibility(Tree,P,N,N,0.016f);
    std::printf("Surfel blocker visibility: %f\n", Blocked);
    Check(Blocked < 0.1f, "triangle-derived distant solar blocker");
    Check(Field.EvaluateSolarVisibility(Tree,{0,0,1.8f},N,N,0.016f) == 0,
        "exact BVH triangle contact shadow");
    SpatialAcceleration::WideBoundingTree Empty;
    Stokes.Prepare(Empty,{&Emission,1},{&Reflectance,1},N,{});
    Check(Stokes.Sources.empty() && Stokes.EvaluateIrradianceOverPi(P,N).R == 0, "rebuild removes stale triangle sources");
    std::printf("%u checks passed\n", Passed);
}
