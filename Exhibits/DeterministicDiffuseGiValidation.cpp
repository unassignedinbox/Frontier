#include "Engine/GeometricRaster/GlobalIllumination/DeterministicDiffuseGi.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace Frontier;
int main()
{
    unsigned Passed = 0;
    auto Verify = [&](bool Good, const char* Name) {
        std::printf("[%s] %s\n", Good ? "PASS" : "FAIL", Name);
        if (!Good) std::exit(1);
        ++Passed;
    };
    auto Close = [](float A, float B) { return std::abs(A - B) < 0.0001f; };
    Verify(Close(GeometricRaster::ReverseBinaryFraction(0), 0), "binary origin");
    Verify(Close(GeometricRaster::ReverseBinaryFraction(1), 0.5f), "binary first sample");
    unsigned Calls = 0;
    auto Constant = [&](Mathematics::Axis3D D) {
        ++Calls;
        Verify(D.Z >= 0 && Close(D.X*D.X + D.Y*D.Y + D.Z*D.Z, 1), "unit upper hemisphere");
        return Mathematics::LinearRGB { 2, 3, 4 };
    };
    auto Empty = GeometricRaster::IntegrateDeterministicDiffuseGi({1,1,1}, 0, 0, Constant);
    Verify(Calls == 0 && Close(Empty.R, 0), "zero samples finite black");
    auto Lit = GeometricRaster::IntegrateDeterministicDiffuseGi({0.5f,0.25f,0.125f}, 32, 123, Constant);
    Verify(Calls == 32, "exact ray budget");
    Verify(Close(Lit.R,1) && Close(Lit.G,0.75f) && Close(Lit.B,0.5f), "constant radiance integral and coloured transport");
    auto Dark = GeometricRaster::IntegrateDeterministicDiffuseGi({1,1,1}, 32, 7,
        [](Mathematics::Axis3D) { return Mathematics::LinearRGB {}; });
    Verify(Close(Dark.R,0) && Close(Dark.G,0) && Close(Dark.B,0), "fully occluded hemisphere");
    auto Directional = [](Mathematics::Axis3D D) { return Mathematics::LinearRGB { D.Z,D.Z,D.Z }; };
    auto A = GeometricRaster::IntegrateDeterministicDiffuseGi({1,1,1}, 1024, 42, Directional);
    auto B = GeometricRaster::IntegrateDeterministicDiffuseGi({1,1,1}, 1024, 42, Directional);
    Verify(A.R == B.R && A.G == B.G && A.B == B.B, "bitwise repeatability");
    Verify(std::abs(A.R - 2.0f/3.0f) < 0.001f, "cosine radiance analytic integral");
    std::printf("%u checks passed\n", Passed);
}
