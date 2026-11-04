/*==========================================================================================================================================
                                                        PORTALCLIPPINGVALIDATION.CPP
==========================================================================================================================================*/
// 🧩 Triangle splits preserve area and interpolated attributes, including rotated portal planes.

#include "Frontier/Extensions/Portal/PortalClipping.h"
#include <cmath>
#include <cstdio>
#include <limits>

using namespace Frontier;
namespace
{
int Checks = 0, Failures = 0;
void Expect(bool Value, const char* Name) { ++Checks; if (!Value) ++Failures; std::printf("%s %s\n", Value ? "PASS" : "FAIL", Name); }
bool Near(double A, double B) { return std::abs(A - B) < 1e-8; }
double Area(const PortalClipTriangle& T)
{
    const auto& A = T[0].Position; const auto& B = T[1].Position; const auto& C = T[2].Position;
    const double X=(B[1]-A[1])*(C[2]-A[2])-(B[2]-A[2])*(C[1]-A[1]);
    const double Y=(B[2]-A[2])*(C[0]-A[0])-(B[0]-A[0])*(C[2]-A[2]);
    const double Z=(B[0]-A[0])*(C[1]-A[1])-(B[1]-A[1])*(C[0]-A[0]);
    return 0.5*std::hypot(X,Y,Z);
}
double Area(const std::vector<PortalClipTriangle>& Triangles)
{
    double Sum=0; for (const auto& T : Triangles) Sum += Area(T); return Sum;
}
std::array<double, 3> FaceNormal(const PortalClipTriangle& T)
{
    const auto& A=T[0].Position; const auto& B=T[1].Position; const auto& C=T[2].Position;
    return {(B[1]-A[1])*(C[2]-A[2])-(B[2]-A[2])*(C[1]-A[1]),
            (B[2]-A[2])*(C[0]-A[0])-(B[0]-A[0])*(C[2]-A[2]),
            (B[0]-A[0])*(C[1]-A[1])-(B[1]-A[1])*(C[0]-A[0])};
}
}
int main()
{
    PortalAperture Aperture;
    PortalClipTriangle Triangle{};
    Triangle[0].Position = {0,0,1}; Triangle[1].Position = {2,0,-1}; Triangle[2].Position = {0,2,-1};
    Triangle[0].Texture = {0,0}; Triangle[1].Texture = {1,0}; Triangle[2].Texture = {0,1};
    for (auto& V : Triangle) V.Normal = {0,0,1};
    PortalClipResult Result;
    Expect(ClipPortalTriangle(Aperture, Triangle, Result), "crossing triangle clipped");
    Expect(Result.Front.size() == 1 && Result.Back.size() == 2, "one front and two back triangles");
    Expect(Near(Area(Result.Front)+Area(Result.Back), Area(Triangle)), "split preserves total surface area");
    bool Interpolated = false, Consistent = true;
    for (const auto& T : Result.Front) for (const auto& V : T)
    {
        Consistent &= V.Position[2] >= -1e-9;
        Interpolated |= Near(V.Position[0],1) && Near(V.Position[1],0) && Near(V.Position[2],0) &&
                        Near(V.Texture[0],0.5) && Near(V.Texture[1],0) && Near(V.Normal[2],1);
    }
    for (const auto& T : Result.Back) for (const auto& V : T) Consistent &= V.Position[2] <= 1e-9;
    Expect(Consistent, "each output stays on its respective side");
    const auto OriginalNormal = FaceNormal(Triangle);
    bool Winding = true;
    for (const auto* Side : {&Result.Front, &Result.Back}) for (const auto& T : *Side)
    {
        const auto N = FaceNormal(T);
        Winding &= N[0]*OriginalNormal[0] + N[1]*OriginalNormal[1] + N[2]*OriginalNormal[2] > 0;
    }
    Expect(Winding, "split triangles retain their original winding");
    Expect(Interpolated, "intersection interpolates texture and normal");
    for (auto& V : Triangle) V.Position[2] = 1;
    Expect(ClipPortalTriangle(Aperture, Triangle, Result) && Result.Front.size()==1 && Result.Back.empty(), "front-only triangle remains intact");
    for (auto& V : Triangle) V.Position[2] = -1;
    Expect(ClipPortalTriangle(Aperture, Triangle, Result) && Result.Front.empty() && Result.Back.size()==1, "back-only triangle remains intact");
    for (auto& V : Triangle) V.Position[2] = 0;
    Expect(ClipPortalTriangle(Aperture, Triangle, Result) && Result.Front.size()==1 && Result.Back.empty(), "coplanar triangle appears once");
    Triangle[0].Position = {0,0,0}; Triangle[1].Position = {1,0,1}; Triangle[2].Position = {0,1,-1};
    Expect(ClipPortalTriangle(Aperture, Triangle, Result) && Result.Front.size()==1 && Result.Back.size()==1 &&
           Near(Area(Result.Front)+Area(Result.Back), Area(Triangle)), "vertex on plane produces no duplicate or degenerate triangle");
    Aperture.Orientation = {0, std::sin(0.25*3.14159265358979323846), 0, std::cos(0.25*3.14159265358979323846)};
    Triangle[0].Position = {1,0,0}; Triangle[1].Position = {-1,2,0}; Triangle[2].Position = {-1,0,2};
    Expect(ClipPortalTriangle(Aperture, Triangle, Result) && Result.Front.size()==1 && Result.Back.size()==2 &&
           Near(Area(Result.Front)+Area(Result.Back), Area(Triangle)), "rotated aperture clips against its rotated normal");
    const auto Saved = Result.Front.size();
    Aperture.Orientation = {0,0,0,0};
    Expect(!ClipPortalTriangle(Aperture, Triangle, Result) && Result.Front.size() == Saved, "invalid aperture leaves output untouched");
    Aperture.Orientation = {0,0,0,1}; Triangle[0].Position[0] = std::numeric_limits<double>::infinity();
    Expect(!ClipPortalTriangle(Aperture, Triangle, Result) && Result.Front.size() == Saved, "non-finite vertex rejected without output changes");
    std::printf("%d / %d checks passed\n", Checks-Failures, Checks);
    return Failures ? 1 : 0;
}
