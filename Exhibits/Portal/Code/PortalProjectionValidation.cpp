/*==========================================================================================================================================
                                                       PORTALPROJECTIONVALIDATION.CPP
==========================================================================================================================================*/
// 🧩 Crossing and round-trip checks for portal projection, without a renderer or physics engine.

#include "Frontier/Extensions/Portal/PortalProjection.h"

#include <cmath>
#include <cstdio>
#include <limits>

using namespace Frontier;
namespace
{
int Checks = 0, Failures = 0;
void Expect(bool Value, const char* Text) { ++Checks; if (!Value) ++Failures; std::printf("%s %s\n", Value ? "PASS" : "FAIL", Text); }
bool Near(double A, double B) { return std::abs(A-B) < 1e-8; }
}
int main()
{
    PortalAperture A, B;
    A.HalfExtent = {2, 3};
    B.HalfExtent = {4, 6};
    B.Position = {10, 0, 0};
    double Fraction = -1;
    Expect(FindPortalCrossing(A, {0,0,1}, {0,0,-1}, Fraction) && Near(Fraction, 0.5), "front-to-back crossing");
    Expect(!FindPortalCrossing(A, {0,0,-1}, {0,0,1}, Fraction), "reverse crossing rejected");
    Expect(!FindPortalCrossing(A, {3,0,1}, {3,0,-1}, Fraction), "outside aperture rejected");
    Expect(FindPortalCrossing(A, {2,3,1}, {2,3,-1}, Fraction), "aperture boundary accepted");
    Expect(!FindPortalCrossing(A, {0,0,0}, {0,0,-1}, Fraction), "start on plane does not recross");
    PortalMotion Input, Result, Returned;
    Input.Position = {1, 0, -1};
    Input.LinearVelocity = {1, 2, -3};
    Input.AngularVelocity = {1, 2, 3};
    Input.Scale = 1.5;
    Expect(ProjectPortalMotion(A, B, Input, Result), "proportional apertures map motion");
    Expect(Near(Result.Position[0], 8) && Near(Result.Position[2], 2), "position turns and scales");
    Expect(Near(Result.LinearVelocity[0], -2) && Near(Result.LinearVelocity[1], 4) && Near(Result.LinearVelocity[2], 6), "linear velocity turns and scales");
    Expect(Near(Result.AngularVelocity[0], -1) && Near(Result.AngularVelocity[1], 2) && Near(Result.AngularVelocity[2], -3), "angular velocity turns without scaling");
    Expect(Near(Result.Scale, 3), "object scale follows ratio");
    Expect(ProjectPortalMotion(B, A, Result, Returned) && Near(Returned.Position[0], Input.Position[0]) &&
           Near(Returned.Position[2], Input.Position[2]) && Near(Returned.Scale, Input.Scale) &&
           Near(Returned.LinearVelocity[2], Input.LinearVelocity[2]), "reverse projection round-trips");
    A.Orientation = {std::sin(0.35), 0, 0, std::cos(0.35)};
    B.Orientation = {0, std::sin(0.6), 0, std::cos(0.6)};
    Input.Orientation = {0, 0, std::sin(0.2), std::cos(0.2)};
    Expect(ProjectPortalMotion(A, B, Input, Result) && ProjectPortalMotion(B, A, Result, Returned) &&
           Near(Returned.Position[0], Input.Position[0]) && Near(Returned.Position[1], Input.Position[1]) &&
           Near(Returned.Position[2], Input.Position[2]) && Near(Returned.AngularVelocity[0], Input.AngularVelocity[0]) &&
           Near(Returned.AngularVelocity[1], Input.AngularVelocity[1]) && Near(Returned.AngularVelocity[2], Input.AngularVelocity[2]) &&
           Near(std::abs(Returned.Orientation[0]*Input.Orientation[0] + Returned.Orientation[1]*Input.Orientation[1] +
                         Returned.Orientation[2]*Input.Orientation[2] + Returned.Orientation[3]*Input.Orientation[3]), 1.0),
           "rotated apertures round-trip position, orientation and angular velocity");
    A.Orientation = {0,0,0,1}; B.Orientation = {0,0,0,1};
    const PortalMotion Saved = Result;
    B.HalfExtent[1] = 5;
    Expect(!ProjectPortalMotion(A, B, Input, Result) && Result.Position == Saved.Position, "mismatched aspect ratios rejected without output changes");
    B.HalfExtent[1] = 6;
    A.Orientation = {0,0,0,0};
    Expect(!ProjectPortalMotion(A, B, Input, Result), "zero quaternion rejected");
    A.Orientation = {0,0,0,1};
    Input.Position[0] = std::numeric_limits<double>::infinity();
    Expect(!ProjectPortalMotion(A, B, Input, Result), "non-finite motion rejected");
    std::printf("%d / %d checks passed\n", Checks-Failures, Checks);
    return Failures ? 1 : 0;
}
