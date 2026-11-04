/*==========================================================================================================================================
                                                          PORTALPROJECTION.CPP
==========================================================================================================================================*/
// 🧩 Validated orientation, aperture crossing and scaled motion mapping.

#include "Frontier/Extensions/Portal/PortalProjection.h"

#include <algorithm>
#include <cmath>

namespace Frontier
{
namespace
{
using V3 = std::array<double, 3>;
using Q4 = std::array<double, 4>;

template <size_t N> bool Finite(const std::array<double, N>& Values)
{
    return std::all_of(Values.begin(), Values.end(), [](double Value) { return std::isfinite(Value); });
}
bool Normalize(Q4& Q)
{
    if (!Finite(Q)) return false;
    const double Length = std::hypot(std::hypot(Q[0], Q[1]), std::hypot(Q[2], Q[3]));
    if (!std::isfinite(Length) || Length < 1e-12) return false;
    for (double& Value : Q) Value /= Length;
    return true;
}
Q4 Product(const Q4& A, const Q4& B)
{
    return {A[3]*B[0]+A[0]*B[3]+A[1]*B[2]-A[2]*B[1],
            A[3]*B[1]-A[0]*B[2]+A[1]*B[3]+A[2]*B[0],
            A[3]*B[2]+A[0]*B[1]-A[1]*B[0]+A[2]*B[3],
            A[3]*B[3]-A[0]*B[0]-A[1]*B[1]-A[2]*B[2]};
}
Q4 Inverse(const Q4& Q) { return {-Q[0], -Q[1], -Q[2], Q[3]}; }
V3 Rotate(const Q4& Q, const V3& V)
{
    const Q4 P = Product(Product(Q, {V[0], V[1], V[2], 0.0}), Inverse(Q));
    return {P[0], P[1], P[2]};
}
V3 Subtract(const V3& A, const V3& B) { return {A[0]-B[0], A[1]-B[1], A[2]-B[2]}; }
bool Valid(const PortalAperture& A, Q4& Orientation)
{
    Orientation = A.Orientation;
    return Finite(A.Position) && Finite(A.HalfExtent) && A.HalfExtent[0] > 0.0 && A.HalfExtent[1] > 0.0 && Normalize(Orientation);
}
} // namespace

bool FindPortalCrossing(const PortalAperture& Aperture, const V3& Previous, const V3& Current, double& Fraction) noexcept
{
    Q4 Orientation;
    if (!Valid(Aperture, Orientation) || !Finite(Previous) || !Finite(Current)) return false;
    const V3 A = Rotate(Inverse(Orientation), Subtract(Previous, Aperture.Position));
    const V3 B = Rotate(Inverse(Orientation), Subtract(Current, Aperture.Position));
    if (!(A[2] > 0.0 && B[2] <= 0.0)) return false;
    const double Denominator = A[2] - B[2];
    if (!std::isfinite(Denominator) || Denominator <= 0.0) return false;
    const double T = A[2] / Denominator;
    const double X = A[0] + T*(B[0]-A[0]), Y = A[1] + T*(B[1]-A[1]);
    if (!std::isfinite(X) || !std::isfinite(Y) || std::abs(X) > Aperture.HalfExtent[0] || std::abs(Y) > Aperture.HalfExtent[1]) return false;
    Fraction = T;
    return true;
}

bool ProjectPortalMotion(const PortalAperture& Source, const PortalAperture& Destination,
                         const PortalMotion& Input, PortalMotion& Output) noexcept
{
    Q4 A, B, Object = Input.Orientation;
    if (!Valid(Source, A) || !Valid(Destination, B) || !Normalize(Object) || !Finite(Input.Position) ||
        !Finite(Input.LinearVelocity) || !Finite(Input.AngularVelocity) || !std::isfinite(Input.Scale) || Input.Scale <= 0.0) return false;
    const double Ratio = Destination.HalfExtent[0] / Source.HalfExtent[0];
    const double Vertical = Destination.HalfExtent[1] / Source.HalfExtent[1];
    if (!std::isfinite(Ratio) || !std::isfinite(Vertical) || Ratio <= 0.0 ||
        std::abs(Ratio - Vertical) > 1e-9 * std::max(Ratio, Vertical)) return false;
    const Q4 Turn{0.0, 1.0, 0.0, 0.0};
    Q4 Map = Product(Product(B, Turn), Inverse(A));
    if (!Normalize(Map)) return false;
    PortalMotion Result;
    const V3 Local = Rotate(Map, Subtract(Input.Position, Source.Position));
    Result.LinearVelocity = Rotate(Map, Input.LinearVelocity);
    Result.AngularVelocity = Rotate(Map, Input.AngularVelocity);
    for (size_t Axis = 0; Axis < 3; ++Axis)
    {
        Result.Position[Axis] = Destination.Position[Axis] + Ratio*Local[Axis];
        Result.LinearVelocity[Axis] *= Ratio;
    }
    Result.Scale = Ratio*Input.Scale;
    Result.Orientation = Product(Map, Object);
    if (!Finite(Result.Position) || !Finite(Result.LinearVelocity) || !Finite(Result.AngularVelocity) ||
        !std::isfinite(Result.Scale) || !Normalize(Result.Orientation)) return false;
    Output = Result;
    return true;
}

} // namespace Frontier
