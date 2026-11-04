/*==========================================================================================================================================
                                                           PORTALCLIPPING.CPP
==========================================================================================================================================*/
// 🧩 Triangle/plane clipping with linear attributes; this does not construct a closed solid.

#include "Frontier/Extensions/Portal/PortalClipping.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Frontier
{
namespace
{
template <size_t N> bool Finite(const std::array<double, N>& Values)
{
    return std::all_of(Values.begin(), Values.end(), [](double V) { return std::isfinite(V); });
}

bool ValidVertex(const PortalClipVertex& Vertex)
{
    return Finite(Vertex.Position) && Finite(Vertex.Normal) && Finite(Vertex.Texture);
}

PortalClipVertex Interpolate(const PortalClipVertex& A, const PortalClipVertex& B, double T)
{
    PortalClipVertex Vertex;
    for (size_t Axis = 0; Axis < 3; ++Axis)
    {
        Vertex.Position[Axis] = A.Position[Axis] + T * (B.Position[Axis] - A.Position[Axis]);
        Vertex.Normal[Axis] = A.Normal[Axis] + T * (B.Normal[Axis] - A.Normal[Axis]);
    }
    for (size_t Axis = 0; Axis < 2; ++Axis) Vertex.Texture[Axis] = A.Texture[Axis] + T * (B.Texture[Axis] - A.Texture[Axis]);
    return Vertex;
}

bool SamePosition(const PortalClipVertex& A, const PortalClipVertex& B)
{
    return A.Position == B.Position;
}

// 📝 Half-space clip of a three-vertex polygon. The output can contain four points.
bool ClipHalf(const PortalClipTriangle& Triangle, const std::array<double, 3>& Distances, bool Front,
              std::vector<PortalClipTriangle>& Result)
{
    std::array<PortalClipVertex, 6> Polygon{};
    size_t Count = 0;
    auto Add = [&](const PortalClipVertex& Vertex) {
        if (Count == 0 || !SamePosition(Polygon[Count - 1], Vertex)) Polygon[Count++] = Vertex;
    };
    for (size_t End = 0; End < 3; ++End)
    {
        const size_t Start = (End + 2) % 3;
        const double A = Distances[Start], B = Distances[End];
        const bool InsideA = Front ? A >= 0.0 : A <= 0.0;
        const bool InsideB = Front ? B >= 0.0 : B <= 0.0;
        if (InsideA != InsideB)
        {
            const double Denominator = A - B;
            if (!std::isfinite(Denominator) || Denominator == 0.0) return false;
            const PortalClipVertex Vertex = A == 0.0 ? Triangle[Start] : B == 0.0 ? Triangle[End] :
                                            Interpolate(Triangle[Start], Triangle[End], A / Denominator);
            if (!ValidVertex(Vertex)) return false;
            Add(Vertex);
        }
        if (InsideB) Add(Triangle[End]);
    }
    if (Count > 1 && SamePosition(Polygon[0], Polygon[Count - 1])) --Count;
    for (size_t Index = 1; Index + 1 < Count; ++Index)
    {
        const auto& A = Polygon[0].Position;
        const auto& B = Polygon[Index].Position;
        const auto& C = Polygon[Index + 1].Position;
        const double X = (B[1]-A[1])*(C[2]-A[2])-(B[2]-A[2])*(C[1]-A[1]);
        const double Y = (B[2]-A[2])*(C[0]-A[0])-(B[0]-A[0])*(C[2]-A[2]);
        const double Z = (B[0]-A[0])*(C[1]-A[1])-(B[1]-A[1])*(C[0]-A[0]);
        if (!std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z)) return false;
        if (X == 0.0 && Y == 0.0 && Z == 0.0) continue;
        Result.push_back({Polygon[0], Polygon[Index], Polygon[Index + 1]});
    }
    return true;
}
} // namespace

bool ClipPortalTriangle(const PortalAperture& Aperture, const PortalClipTriangle& Triangle, PortalClipResult& Output) noexcept
{
    if (!Finite(Aperture.Position) || !Finite(Aperture.Orientation) || !Finite(Aperture.HalfExtent) ||
        Aperture.HalfExtent[0] <= 0.0 || Aperture.HalfExtent[1] <= 0.0) return false;
    auto Q = Aperture.Orientation;
    const double Length = std::hypot(std::hypot(Q[0], Q[1]), std::hypot(Q[2], Q[3]));
    if (!std::isfinite(Length) || Length < 1e-12) return false;
    for (double& Value : Q) Value /= Length;
    const std::array<double, 3> Normal{2*(Q[0]*Q[2]+Q[3]*Q[1]), 2*(Q[1]*Q[2]-Q[3]*Q[0]),
                                       1-2*(Q[0]*Q[0]+Q[1]*Q[1])};
    std::array<double, 3> Distances{};
    for (size_t Index = 0; Index < 3; ++Index)
    {
        if (!ValidVertex(Triangle[Index])) return false;
        for (size_t Axis = 0; Axis < 3; ++Axis)
            Distances[Index] += (Triangle[Index].Position[Axis] - Aperture.Position[Axis]) * Normal[Axis];
        if (!std::isfinite(Distances[Index])) return false;
        if (std::abs(Distances[Index]) < 1e-10) Distances[Index] = 0.0;
    }
    try
    {
        PortalClipResult Result;
        if (!ClipHalf(Triangle, Distances, true, Result.Front)) return false;
        if ((Distances[0] != 0.0 || Distances[1] != 0.0 || Distances[2] != 0.0) &&
            !ClipHalf(Triangle, Distances, false, Result.Back)) return false;
        Output = std::move(Result);
        return true;
    }
    catch (...) { return false; }
}
} // namespace Frontier
