/*============================================================================================================================================
                                                    ROLLINGBALLSTATIONVALIDATION.CPP
============================================================================================================================================*/
// 🧩 One surface-agnostic rolling-ball / chamfer cross-section solver, marched along nine support pairs with zero type branches

//------------------------------------------------------------------------------------------------------------------------
//                                                      INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// 💡 The solver below never asks what a surface IS. It only calls Evaluate(U, V) → point + first derivatives.
//    Planes, cylinders, cones, spheres and a freeform bicubic patch all go through the identical code path.
//    Analytic closed forms appear ONLY in the independent verification column, never in the construction.

namespace Frontier
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    VECTOR ALGEBRA
//------------------------------------------------------------------------------------------------------------------------

struct Vec3
{
    double X = 0.0;                                                                     // [m]
    double Y = 0.0;                                                                     // [m]
    double Z = 0.0;                                                                     // [m]
};

static Vec3   operator+(Vec3 A, Vec3 B) { return { A.X + B.X, A.Y + B.Y, A.Z + B.Z }; }
static Vec3   operator-(Vec3 A, Vec3 B) { return { A.X - B.X, A.Y - B.Y, A.Z - B.Z }; }
static Vec3   operator*(Vec3 A, double S) { return { A.X * S, A.Y * S, A.Z * S }; }
static Vec3   operator*(double S, Vec3 A) { return A * S; }
static double Dot(Vec3 A, Vec3 B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
static Vec3   Cross(Vec3 A, Vec3 B) { return { A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X }; }
static double Length(Vec3 A) { return std::sqrt(Dot(A, A)); }
static Vec3   Normalised(Vec3 A) { double L = Length(A); return L > 0.0 ? A * (1.0 / L) : A; }

constexpr double π = 3.14159265358979323846;                                            // [-]

//------------------------------------------------------------------------------------------------------------------------
//                                                  SUPPORT SURFACES
//------------------------------------------------------------------------------------------------------------------------

struct SurfaceSample
{
    Vec3 Point;                                                                         // [m]   S(u, v)
    Vec3 DerivativeU;                                                                   // [m]   ∂S/∂u
    Vec3 DerivativeV;                                                                   // [m]   ∂S/∂v
};

struct ParameterDomain
{
    double U0 = 0.0, U1 = 1.0, V0 = 0.0, V1 = 1.0;                                      // [-]
};

// 📝 The ONLY contract a support must satisfy. A NURBS surface, an offset surface, a procedural surface — all qualify.
class SupportSurface
{
public:
    virtual ~SupportSurface() = default;
    [[nodiscard]] virtual SurfaceSample Evaluate(double U, double V) const noexcept = 0;

    double          Orientation = 1.0;                                                  // [-]   +1 ⇒ Su×Sv points out of material (BrepFace.Reversed)
    ParameterDomain Domain;                                                             // [-]   seed lattice extent
    ParameterDomain DisplayDomain;                                                      // [-]   iso-line extent for the exhibit image only

    [[nodiscard]] Vec3 OutwardNormal(double U, double V) const noexcept
    {
        SurfaceSample S = Evaluate(U, V);
        return Normalised(Cross(S.DerivativeU, S.DerivativeV)) * Orientation;
    }

    // 📝 Coarse lattice seed + Gauss–Newton on |S − P|². Generic: no surface classification consulted.
    void ClosestParameter(Vec3 P, double& U, double& V) const noexcept
    {
        double Best = 1e300;
        for (int I = 0; I <= 40; ++I)
        {
            for (int J = 0; J <= 40; ++J)
            {
                double Uc = Domain.U0 + (Domain.U1 - Domain.U0) * I / 40.0;
                double Vc = Domain.V0 + (Domain.V1 - Domain.V0) * J / 40.0;
                double D  = Length(Evaluate(Uc, Vc).Point - P);
                if (D < Best) { Best = D; U = Uc; V = Vc; }
            }
        }
        for (int Iteration = 0; Iteration < 50; ++Iteration)
        {
            SurfaceSample S = Evaluate(U, V);
            Vec3   R   = P - S.Point;
            double A11 = Dot(S.DerivativeU, S.DerivativeU), A12 = Dot(S.DerivativeU, S.DerivativeV), A22 = Dot(S.DerivativeV, S.DerivativeV);
            double B1  = Dot(S.DerivativeU, R), B2 = Dot(S.DerivativeV, R);
            double Det = A11 * A22 - A12 * A12;
            if (std::fabs(Det) < 1e-300) break;
            double ΔU = (A22 * B1 - A12 * B2) / Det, ΔV = (A11 * B2 - A12 * B1) / Det;
            U += ΔU; V += ΔV;
            if (std::fabs(ΔU) + std::fabs(ΔV) < 1e-15) break;
        }
    }
};

// 📝 Each support below is just a parameterisation. None of them is referenced by name inside the solver.
class PlaneSupport final : public SupportSurface
{
public:
    Vec3 Origin, AxisU, AxisV;
    PlaneSupport(Vec3 O, Vec3 A, Vec3 B) : Origin(O), AxisU(A), AxisV(B) {}
    SurfaceSample Evaluate(double U, double V) const noexcept override { return { Origin + U * AxisU + V * AxisV, AxisU, AxisV }; }
};

class CylinderSupport final : public SupportSurface
{
public:
    Vec3 Foot, FrameX, FrameY, Axis; double Radius;
    CylinderSupport(Vec3 F, Vec3 X, Vec3 Y, double R) : Foot(F), FrameX(X), FrameY(Y), Axis(Cross(X, Y)), Radius(R) {}
    SurfaceSample Evaluate(double U, double V) const noexcept override
    {
        Vec3 Radial = std::cos(U) * FrameX + std::sin(U) * FrameY;
        Vec3 Swing  = -std::sin(U) * FrameX + std::cos(U) * FrameY;
        return { Foot + Radius * Radial + V * Axis, Radius * Swing, Axis };
    }
};

class ConeSupport final : public SupportSurface
{
public:
    Vec3 Foot; double RadiusFoot, RadiusTop, Height;
    ConeSupport(Vec3 F, double Rf, double Rt, double H) : Foot(F), RadiusFoot(Rf), RadiusTop(Rt), Height(H) {}
    SurfaceSample Evaluate(double U, double V) const noexcept override
    {
        double ρ = RadiusFoot + (RadiusTop - RadiusFoot) * V / Height, Slope = (RadiusTop - RadiusFoot) / Height;
        Vec3 Radial{ std::cos(U), std::sin(U), 0.0 }, Swing{ -std::sin(U), std::cos(U), 0.0 };
        return { Foot + ρ * Radial + Vec3{ 0, 0, V }, ρ * Swing, Slope * Radial + Vec3{ 0, 0, 1 } };
    }
};

class SphereSupport final : public SupportSurface
{
public:
    Vec3 Centre; double Radius;
    SphereSupport(Vec3 C, double R) : Centre(C), Radius(R) {}
    SurfaceSample Evaluate(double U, double V) const noexcept override
    {
        double Cu = std::cos(U), Su = std::sin(U), Cv = std::cos(V), Sv = std::sin(V);
        return { Centre + Radius * Vec3{ Cv * Cu, Cv * Su, Sv }, Radius * Vec3{ -Cv * Su, Cv * Cu, 0 }, Radius * Vec3{ -Sv * Cu, -Sv * Su, Cv } };
    }
};

class BicubicPatchSupport final : public SupportSurface
{
public:
    std::array<Vec3, 16> Poles;                                                         // [m]   Poles[I * 4 + J], I along U
    static void Bernstein(double T, double B[4], double D[4])
    {
        double S = 1.0 - T;
        B[0] = S * S * S; B[1] = 3 * T * S * S; B[2] = 3 * T * T * S; B[3] = T * T * T;
        D[0] = -3 * S * S; D[1] = 3 * S * S - 6 * T * S; D[2] = 6 * T * S - 3 * T * T; D[3] = 3 * T * T;
    }
    SurfaceSample Evaluate(double U, double V) const noexcept override
    {
        double Bu[4], Du[4], Bv[4], Dv[4];
        Bernstein(U, Bu, Du); Bernstein(V, Bv, Dv);
        SurfaceSample S{};
        for (int I = 0; I < 4; ++I)
        {
            for (int J = 0; J < 4; ++J)
            {
                Vec3 P = Poles[I * 4 + J];
                S.Point       = S.Point + (Bu[I] * Bv[J]) * P;
                S.DerivativeU = S.DerivativeU + (Du[I] * Bv[J]) * P;
                S.DerivativeV = S.DerivativeV + (Bu[I] * Dv[J]) * P;
            }
        }
        return S;
    }
};

//------------------------------------------------------------------------------------------------------------------------
//                                                CROSS-SECTION SOLVER
//------------------------------------------------------------------------------------------------------------------------

enum class BlendProfile : uint8_t { Fillet, Chamfer };

struct BlendSpecimen
{
    std::string                     Title;
    std::shared_ptr<SupportSurface> Support1, Support2;
    std::function<Vec3(double)>     Spine;                                              // [m]   the sharp edge e(t) (BrepEdge curve)
    double                          T0 = 0.0, T1 = 1.0;                                 // [-]   spine extent
    Vec3                            Interior1, Interior2;                               // [-]   into-face direction at T0 (from coedge sense)
    double                          Size = 0.1;                                         // [m]   radius (fillet) or setback (chamfer)
    BlendProfile                    Profile = BlendProfile::Fillet;
    std::function<double(Vec3, Vec3, Vec3, double)> AnalyticDeviation;                  // [m]   independent closed form; empty ⇒ none
    std::function<double(Vec3)>     IndependentDistance1, IndependentDistance2;         // [m]   exact unsigned distance to infinite support
};

struct StationSolution
{
    double U1 = 0, V1 = 0, U2 = 0, V2 = 0;                                              // [-]
    Vec3   Contact1, Contact2, Centre, Spine;                                           // [m]
    double Residual   = 0.0;                                                            // [m]
    int    Iterations = 0;                                                              // [-]
    bool   Converged  = false;
};

static Vec3 SpineTangent(const BlendSpecimen& B, double T)
{
    double H = 1e-6 * std::max(1.0, std::fabs(B.T1 - B.T0));
    return Normalised(B.Spine(T + H) - B.Spine(T - H));
}

// 📝 Four equations, four unknowns (u₁, v₁, u₂, v₂). Identical for every support pair:
//    Fillet  ① T·(c − e) = 0          ②–④ c₁ − c₂ = 0,   cᵢ = pᵢ − σ·r·nᵢ   (σ = +1 convex, −1 concave)
//    Chamfer ① T·(p₁ − e) = 0  ② T·(p₂ − e) = 0  ③ |p₁ − e|² − d² = 0  ④ |p₂ − e|² − d² = 0
static std::array<double, 4> EvaluateStationResidual(const BlendSpecimen& B, double T, double σ, const std::array<double, 4>& X,
                                                     Vec3* ContactOut1 = nullptr, Vec3* ContactOut2 = nullptr, Vec3* CentreOut = nullptr)
{
    Vec3 E  = B.Spine(T), Tg = SpineTangent(B, T);
    Vec3 P1 = B.Support1->Evaluate(X[0], X[1]).Point, P2 = B.Support2->Evaluate(X[2], X[3]).Point;
    if (ContactOut1) *ContactOut1 = P1;
    if (ContactOut2) *ContactOut2 = P2;
    if (B.Profile == BlendProfile::Fillet)
    {
        Vec3 C1 = P1 - (σ * B.Size) * B.Support1->OutwardNormal(X[0], X[1]);
        Vec3 C2 = P2 - (σ * B.Size) * B.Support2->OutwardNormal(X[2], X[3]);
        if (CentreOut) *CentreOut = (C1 + C2) * 0.5;
        Vec3 Gap = C1 - C2;
        return { Dot(Tg, (C1 + C2) * 0.5 - E), Gap.X, Gap.Y, Gap.Z };
    }
    if (CentreOut) *CentreOut = E;
    return { Dot(Tg, P1 - E), Dot(Tg, P2 - E), Dot(P1 - E, P1 - E) - B.Size * B.Size, Dot(P2 - E, P2 - E) - B.Size * B.Size };
}

static double Magnitude(const std::array<double, 4>& F) { return std::sqrt(F[0] * F[0] + F[1] * F[1] + F[2] * F[2] + F[3] * F[3]); }

static bool SolveLinear4(double A[4][4], double Rhs[4], double Out[4])
{
    for (int Column = 0; Column < 4; ++Column)
    {
        int Pivot = Column;
        for (int Row = Column + 1; Row < 4; ++Row) if (std::fabs(A[Row][Column]) > std::fabs(A[Pivot][Column])) Pivot = Row;
        if (std::fabs(A[Pivot][Column]) < 1e-300) return false;
        for (int K = 0; K < 4; ++K) std::swap(A[Column][K], A[Pivot][K]);
        std::swap(Rhs[Column], Rhs[Pivot]);
        for (int Row = Column + 1; Row < 4; ++Row)
        {
            double Factor = A[Row][Column] / A[Column][Column];
            for (int K = Column; K < 4; ++K) A[Row][K] -= Factor * A[Column][K];
            Rhs[Row] -= Factor * Rhs[Column];
        }
    }
    for (int Row = 3; Row >= 0; --Row)
    {
        double Sum = Rhs[Row];
        for (int K = Row + 1; K < 4; ++K) Sum -= A[Row][K] * Out[K];
        Out[Row] = Sum / A[Row][Row];
    }
    return true;
}

// 📝 Damped Newton with a central-difference Jacobian. Production would use analytic derivatives (Su, Sv, Suu, …).
static StationSolution SolveStation(const BlendSpecimen& B, double T, double σ, std::array<double, 4> X)
{
    StationSolution S;
    for (S.Iterations = 0; S.Iterations < 60; ++S.Iterations)
    {
        std::array<double, 4> F = EvaluateStationResidual(B, T, σ, X);
        double Norm = Magnitude(F);
        if (Norm < 1e-13) { S.Converged = true; break; }
        double J[4][4];
        for (int K = 0; K < 4; ++K)
        {
            std::array<double, 4> Xp = X, Xm = X;
            double H = 1e-7 * std::max(1.0, std::fabs(X[K]));
            Xp[K] += H; Xm[K] -= H;
            std::array<double, 4> Fp = EvaluateStationResidual(B, T, σ, Xp), Fm = EvaluateStationResidual(B, T, σ, Xm);
            for (int Row = 0; Row < 4; ++Row) J[Row][K] = (Fp[Row] - Fm[Row]) / (2.0 * H);
        }
        double Rhs[4] = { -F[0], -F[1], -F[2], -F[3] }, Δ[4] = {};
        if (!SolveLinear4(J, Rhs, Δ)) break;
        double Step = 1.0;
        for (int Damping = 0; Damping < 30; ++Damping, Step *= 0.5)
        {
            std::array<double, 4> Trial = { X[0] + Step * Δ[0], X[1] + Step * Δ[1], X[2] + Step * Δ[2], X[3] + Step * Δ[3] };
            if (Magnitude(EvaluateStationResidual(B, T, σ, Trial)) < Norm) { X = Trial; break; }
        }
    }
    S.U1 = X[0]; S.V1 = X[1]; S.U2 = X[2]; S.V2 = X[3];
    std::array<double, 4> F = EvaluateStationResidual(B, T, σ, X, &S.Contact1, &S.Contact2, &S.Centre);
    S.Residual = Magnitude(F);
    S.Spine    = B.Spine(T);
    if (S.Residual < 1e-11) S.Converged = true;
    return S;
}

// 📝 Walking: seed station 0 from the into-face directions, then continue each station from its predecessor.
static std::vector<StationSolution> MarchSpine(const BlendSpecimen& B, int Stations, double& σOut)
{
    Vec3 E0 = B.Spine(B.T0);
    double U1 = 0, V1 = 0, U2 = 0, V2 = 0;
    B.Support1->ClosestParameter(E0, U1, V1);
    B.Support2->ClosestParameter(E0, U2, V2);
    // 📝 Convexity from topology + normals, not from surface type: into-face-1 direction against face-2 normal.
    Vec3 N2 = B.Support2->OutwardNormal(U2, V2);
    double σ = Dot(Normalised(B.Interior1), N2) < 0.0 ? 1.0 : -1.0;
    σOut = σ;
    B.Support1->ClosestParameter(E0 + B.Size * Normalised(B.Interior1), U1, V1);
    B.Support2->ClosestParameter(E0 + B.Size * Normalised(B.Interior2), U2, V2);
    std::array<double, 4> X = { U1, V1, U2, V2 };
    std::vector<StationSolution> Out;
    for (int I = 0; I <= Stations; ++I)
    {
        double T = B.T0 + (B.T1 - B.T0) * I / Stations;
        StationSolution S = SolveStation(B, T, σ, X);
        X = { S.U1, S.V1, S.U2, S.V2 };
        Out.push_back(S);
    }
    return Out;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 SPECIMEN CATALOGUE
//------------------------------------------------------------------------------------------------------------------------

static Vec3 UnitX() { return { 1, 0, 0 }; }
static Vec3 UnitY() { return { 0, 1, 0 }; }
static Vec3 UnitZ() { return { 0, 0, 1 }; }

static std::vector<BlendSpecimen> AssembleSpecimens(BlendProfile Profile)
{
    std::vector<BlendSpecimen> Out;
    const bool Fillet = Profile == BlendProfile::Fillet;

    // ① Box edge, two planes at 90°.
    {
        BlendSpecimen B; B.Title = "Plane-plane 90 deg (box edge)"; B.Profile = Profile; B.Size = 0.2;
        auto Floor = std::make_shared<PlaneSupport>(Vec3{}, UnitX(), UnitY()); Floor->Orientation = -1;
        auto Wall  = std::make_shared<PlaneSupport>(Vec3{}, UnitX(), UnitZ()); Wall->Orientation = 1;
        Floor->Domain = { -1, 3, -1, 2 }; Wall->Domain = { -1, 3, -1, 2 };
        Floor->DisplayDomain = { 0, 2, 0, 1 }; Wall->DisplayDomain = { 0, 2, 0, 1 };
        B.Support1 = Floor; B.Support2 = Wall;
        B.Spine = [](double T) { return Vec3{ T, 0, 0 }; }; B.T0 = 0; B.T1 = 2;
        B.Interior1 = UnitY(); B.Interior2 = UnitZ();
        B.IndependentDistance1 = [](Vec3 P) { return std::fabs(P.Z); };
        B.IndependentDistance2 = [](Vec3 P) { return std::fabs(P.Y); };
        B.AnalyticDeviation = [Fillet](Vec3 E, Vec3 C, Vec3 P1, double R)
        {
            return Fillet ? Length(C - Vec3{ E.X, R, R }) : std::max(Length(P1 - Vec3{ E.X, R, 0 }), 0.0);
        };
        Out.push_back(B);
    }

    // ②③ Oblique plane pairs, interior dihedral 60° and 135°.
    for (double Dihedral : { 60.0, 135.0 })
    {
        double θ = Dihedral * π / 180.0;
        BlendSpecimen B; B.Title = "Plane-plane " + std::to_string((int)Dihedral) + " deg"; B.Profile = Profile; B.Size = 0.15;
        Vec3 Lean{ 0, std::cos(θ), std::sin(θ) };
        auto Floor = std::make_shared<PlaneSupport>(Vec3{}, UnitX(), UnitY()); Floor->Orientation = -1;
        auto Slope = std::make_shared<PlaneSupport>(Vec3{}, UnitX(), Lean);    Slope->Orientation = 1;
        Floor->Domain = { -1, 3, -1, 2 }; Slope->Domain = { -1, 3, -1, 2 };
        Floor->DisplayDomain = { 0, 2, 0, 1 }; Slope->DisplayDomain = { 0, 2, 0, 1 };
        B.Support1 = Floor; B.Support2 = Slope;
        B.Spine = [](double T) { return Vec3{ T, 0, 0 }; }; B.T0 = 0; B.T1 = 2;
        B.Interior1 = UnitY(); B.Interior2 = Lean;
        B.AnalyticDeviation = [θ, Fillet](Vec3 E, Vec3 C, Vec3 P1, double R)
        {
            // 📝 Tangent setback t = r / tan(θ/2) for interior dihedral θ (independent closed form).
            return Fillet ? Length(C - Vec3{ E.X, R / std::tan(θ / 2), R }) : Length(P1 - Vec3{ E.X, R, 0 });
        };
        Out.push_back(B);
    }

    // ④ Right-cylinder cap rim (convex), full closed loop.
    {
        BlendSpecimen B; B.Title = "Plane-cylinder cap rim (convex)"; B.Profile = Profile; B.Size = 0.15;
        auto Cap  = std::make_shared<PlaneSupport>(Vec3{ 0, 0, 1 }, UnitX(), UnitY()); Cap->Orientation = 1;
        auto Side = std::make_shared<CylinderSupport>(Vec3{}, UnitX(), UnitY(), 1.0); Side->Orientation = 1;
        Cap->Domain = { -1.5, 1.5, -1.5, 1.5 }; Side->Domain = { -π, π, -0.5, 1.5 };
        Cap->DisplayDomain = { -1, 1, -1, 1 }; Side->DisplayDomain = { -π, π, 0.4, 1 };
        B.Support1 = Cap; B.Support2 = Side;
        B.Spine = [](double T) { return Vec3{ std::cos(T), std::sin(T), 1 }; }; B.T0 = 0; B.T1 = 2 * π;
        B.Interior1 = Vec3{ -1, 0, 0 }; B.Interior2 = Vec3{ 0, 0, -1 };
        B.IndependentDistance1 = [](Vec3 P) { return std::fabs(P.Z - 1); };
        B.IndependentDistance2 = [](Vec3 P) { return std::fabs(std::hypot(P.X, P.Y) - 1); };
        B.AnalyticDeviation = [Fillet](Vec3 E, Vec3 C, Vec3 P1, double R)
        {
            Vec3 Radial = Normalised(Vec3{ E.X, E.Y, 0 });
            return Fillet ? Length(C - (Vec3{ 0, 0, 1 - R } + (1 - R) * Radial)) : Length(P1 - (Vec3{ 0, 0, 1 } + (1 - R) * Radial));
        };
        Out.push_back(B);
    }

    // ⑤ Cylindrical boss root on a plate (concave), full closed loop.
    {
        BlendSpecimen B; B.Title = "Plane-cylinder boss root (concave)"; B.Profile = Profile; B.Size = 0.12;
        auto Plate = std::make_shared<PlaneSupport>(Vec3{}, UnitX(), UnitY()); Plate->Orientation = 1;
        auto Boss  = std::make_shared<CylinderSupport>(Vec3{}, UnitX(), UnitY(), 0.5); Boss->Orientation = 1;
        Plate->Domain = { -1.5, 1.5, -1.5, 1.5 }; Boss->Domain = { -π, π, -0.5, 1.5 };
        Plate->DisplayDomain = { -1, 1, -1, 1 }; Boss->DisplayDomain = { -π, π, 0, 0.6 };
        B.Support1 = Plate; B.Support2 = Boss;
        B.Spine = [](double T) { return Vec3{ 0.5 * std::cos(T), 0.5 * std::sin(T), 0 }; }; B.T0 = 0; B.T1 = 2 * π;
        B.Interior1 = UnitX(); B.Interior2 = UnitZ();
        B.IndependentDistance1 = [](Vec3 P) { return std::fabs(P.Z); };
        B.IndependentDistance2 = [](Vec3 P) { return std::fabs(std::hypot(P.X, P.Y) - 0.5); };
        B.AnalyticDeviation = [Fillet](Vec3 E, Vec3 C, Vec3 P1, double R)
        {
            Vec3 Radial = Normalised(Vec3{ E.X, E.Y, 0 });
            return Fillet ? Length(C - (Vec3{ 0, 0, R } + (0.5 + R) * Radial)) : Length(P1 - (0.5 + R) * Radial);
        };
        Out.push_back(B);
    }

    // ⑥ Sphere truncated by a plane: z ≤ 0.4 rim (convex).
    {
        BlendSpecimen B; B.Title = "Plane-sphere truncation rim"; B.Profile = Profile; B.Size = 0.1;
        double ρ0 = std::sqrt(1 - 0.16);
        auto Top    = std::make_shared<PlaneSupport>(Vec3{ 0, 0, 0.4 }, UnitX(), UnitY()); Top->Orientation = 1;
        auto Sphere = std::make_shared<SphereSupport>(Vec3{}, 1.0); Sphere->Orientation = 1;
        Top->Domain = { -1.2, 1.2, -1.2, 1.2 }; Sphere->Domain = { -π, π, -1.4, 1.4 };
        Top->DisplayDomain = { -0.9, 0.9, -0.9, 0.9 }; Sphere->DisplayDomain = { -π, π, -0.3, 0.41 };
        B.Support1 = Top; B.Support2 = Sphere;
        B.Spine = [ρ0](double T) { return Vec3{ ρ0 * std::cos(T), ρ0 * std::sin(T), 0.4 }; }; B.T0 = 0; B.T1 = 2 * π;
        B.Interior1 = Vec3{ -1, 0, 0 }; B.Interior2 = Vec3{ 0, 0, -1 };
        B.IndependentDistance1 = [](Vec3 P) { return std::fabs(P.Z - 0.4); };
        B.IndependentDistance2 = [](Vec3 P) { return std::fabs(Length(P) - 1); };
        B.AnalyticDeviation = [Fillet](Vec3 E, Vec3 C, Vec3 P1, double R)
        {
            Vec3 Radial = Normalised(Vec3{ E.X, E.Y, 0 });
            if (Fillet) return Length(C - (Vec3{ 0, 0, 0.4 - R } + std::sqrt((1 - R) * (1 - R) - (0.4 - R) * (0.4 - R)) * Radial));
            return std::fabs(Length(P1 - E) - R) + std::fabs(P1.Z - 0.4);
        };
        Out.push_back(B);
    }

    // ⑦ Frustum foot rim on its bottom cap (convex, non-right dihedral).
    {
        BlendSpecimen B; B.Title = "Plane-cone frustum foot rim"; B.Profile = Profile; B.Size = 0.1;
        auto Base = std::make_shared<PlaneSupport>(Vec3{}, UnitX(), UnitY()); Base->Orientation = -1;
        auto Cone = std::make_shared<ConeSupport>(Vec3{}, 1.0, 0.5, 1.0); Cone->Orientation = 1;
        Base->Domain = { -1.5, 1.5, -1.5, 1.5 }; Cone->Domain = { -π, π, -0.5, 1.5 };
        Base->DisplayDomain = { -1, 1, -1, 1 }; Cone->DisplayDomain = { -π, π, 0, 0.5 };
        B.Support1 = Base; B.Support2 = Cone;
        B.Spine = [](double T) { return Vec3{ std::cos(T), std::sin(T), 0 }; }; B.T0 = 0; B.T1 = 2 * π;
        B.Interior1 = Vec3{ -1, 0, 0 }; B.Interior2 = Normalised(Vec3{ -0.5, 0, 1 });
        B.IndependentDistance1 = [](Vec3 P) { return std::fabs(P.Z); };
        B.AnalyticDeviation = [Fillet](Vec3 E, Vec3 C, Vec3 P1, double R)
        {
            Vec3 Radial = Normalised(Vec3{ E.X, E.Y, 0 });
            if (Fillet) return Length(C - (Vec3{ 0, 0, R } + (1 - 0.5 * R - R * std::sqrt(1.25)) * Radial));
            return Length(P1 - (1 - R) * Radial);
        };
        Out.push_back(B);
    }

    // ⑧ Crossed cylinders, T-junction root (concave, saddle-shaped spine — no closed form).
    {
        BlendSpecimen B; B.Title = "Cylinder-cylinder T junction"; B.Profile = Profile; B.Size = 0.08;
        auto Main   = std::make_shared<CylinderSupport>(Vec3{ -2, 0, 0 }, UnitY(), UnitZ(), 1.0); Main->Orientation = 1;
        auto Branch = std::make_shared<CylinderSupport>(Vec3{}, UnitX(), UnitY(), 0.5); Branch->Orientation = 1;
        Main->Domain = { -π, π, 0, 4 }; Branch->Domain = { -π, π, 0, 2 };
        Main->DisplayDomain = { 0.3, π - 0.3, 1.1, 2.9 }; Branch->DisplayDomain = { -π, π, 0.85, 1.6 };
        B.Support1 = Main; B.Support2 = Branch;
        B.Spine = [](double T) { double Y = 0.5 * std::sin(T); return Vec3{ 0.5 * std::cos(T), Y, std::sqrt(1 - Y * Y) }; };
        B.T0 = 0; B.T1 = 2 * π;
        B.Interior1 = UnitX(); B.Interior2 = UnitZ();
        B.IndependentDistance1 = [](Vec3 P) { return std::fabs(std::hypot(P.Y, P.Z) - 1.0); };
        B.IndependentDistance2 = [](Vec3 P) { return std::fabs(std::hypot(P.X, P.Y) - 0.5); };
        Out.push_back(B);
    }

    // ⑨ Freeform bicubic top face meeting a vertical plane (convex, no closed form).
    {
        BlendSpecimen B; B.Title = "Freeform bicubic - plane"; B.Profile = Profile; B.Size = 0.08;
        auto Patch = std::make_shared<BicubicPatchSupport>(); Patch->Orientation = 1;
        const double Heights[4][4] = { { 1.0, 1.1, 1.0, 0.9 }, { 1.25, 1.15, 1.05, 1.0 }, { 0.85, 0.95, 1.1, 1.2 }, { 1.1, 1.0, 0.9, 1.0 } };
        for (int I = 0; I < 4; ++I) for (int J = 0; J < 4; ++J) Patch->Poles[I * 4 + J] = { 2.0 * I / 3.0, J / 3.0, Heights[I][J] };
        Patch->Domain = { 0, 1, 0, 1 }; Patch->DisplayDomain = { 0, 1, 0, 1 };
        auto Wall = std::make_shared<PlaneSupport>(Vec3{}, UnitX(), UnitZ()); Wall->Orientation = 1;
        Wall->Domain = { -0.5, 2.5, -0.5, 2.0 }; Wall->DisplayDomain = { 0, 2, 0.5, 1.25 };
        B.Support1 = Patch; B.Support2 = Wall;
        std::shared_ptr<BicubicPatchSupport> Edge = Patch;
        B.Spine = [Edge](double T) { return Edge->Evaluate(T, 0.0).Point; }; B.T0 = 0.02; B.T1 = 0.98;
        B.Interior1 = UnitY(); B.Interior2 = Vec3{ 0, 0, -1 };
        B.IndependentDistance2 = [](Vec3 P) { return std::fabs(P.Y); };
        B.IndependentDistance1 = [Edge](Vec3 P)
        {
            // 📝 Independent closest point: dense brute-force lattice + golden refinement, no shared code with the solver seed.
            double Best = 1e300, Bu = 0, Bv = 0;
            for (int I = 0; I <= 400; ++I) for (int J = 0; J <= 400; ++J)
            {
                double D = Length(Edge->Evaluate(I / 400.0, J / 400.0).Point - P);
                if (D < Best) { Best = D; Bu = I / 400.0; Bv = J / 400.0; }
            }
            double Step = 1.0 / 400.0;
            for (int K = 0; K < 60; ++K, Step *= 0.6)
            {
                for (double Du : { -Step, 0.0, Step }) for (double Dv : { -Step, 0.0, Step })
                {
                    double D = Length(Edge->Evaluate(Bu + Du, Bv + Dv).Point - P);
                    if (D < Best) { Best = D; Bu += Du; Bv += Dv; }
                }
            }
            return Best;
        };
        Out.push_back(B);
    }
    (void)Fillet;
    return Out;
}

//------------------------------------------------------------------------------------------------------------------------
//                                               VERIFICATION + EVIDENCE
//------------------------------------------------------------------------------------------------------------------------

struct SpecimenReport
{
    std::string Title;
    const char* Convexity      = "";
    int         Stations       = 0;
    int         Converged      = 0;
    double      Residual       = 0.0;                                                   // [m]   max |F|
    double      BallDeviation  = 0.0;                                                   // [m]   max ||c − pᵢ| − r|  or ||pᵢ − e| − d|
    double      NormalDeviation= 0.0;                                                   // [rad] max angle(c − pᵢ, ∓nᵢ)  (fillet only)
    double      IndependentDev = -1.0;                                                  // [m]   max |dist(c, Sᵢ) − r| via exact distance
    double      AnalyticDev    = -1.0;                                                  // [m]   max deviation from closed form
    double      MeanIterations = 0.0;                                                   // [-]
};

static SpecimenReport VerifySpecimen(const BlendSpecimen& B, const std::vector<StationSolution>& Path, double σ)
{
    SpecimenReport R; R.Title = B.Title; R.Convexity = σ > 0 ? "convex" : "concave"; R.Stations = (int)Path.size();
    for (const StationSolution& S : Path)
    {
        R.Converged     += S.Converged ? 1 : 0;
        R.Residual       = std::max(R.Residual, S.Residual);
        R.MeanIterations += S.Iterations;
        if (B.Profile == BlendProfile::Fillet)
        {
            R.BallDeviation = std::max({ R.BallDeviation, std::fabs(Length(S.Centre - S.Contact1) - B.Size), std::fabs(Length(S.Centre - S.Contact2) - B.Size) });
            Vec3 N1 = B.Support1->OutwardNormal(S.U1, S.V1) * (-σ), N2 = B.Support2->OutwardNormal(S.U2, S.V2) * (-σ);
            double A1 = std::acos(std::min(1.0, Dot(Normalised(S.Centre - S.Contact1), N1)));
            double A2 = std::acos(std::min(1.0, Dot(Normalised(S.Centre - S.Contact2), N2)));
            R.NormalDeviation = std::max({ R.NormalDeviation, A1, A2 });
            if (B.IndependentDistance1) R.IndependentDev = std::max(R.IndependentDev, std::fabs(B.IndependentDistance1(S.Centre) - B.Size));
            if (B.IndependentDistance2) R.IndependentDev = std::max(R.IndependentDev, std::fabs(B.IndependentDistance2(S.Centre) - B.Size));
        }
        else
        {
            R.BallDeviation = std::max({ R.BallDeviation, std::fabs(Length(S.Contact1 - S.Spine) - B.Size), std::fabs(Length(S.Contact2 - S.Spine) - B.Size) });
            if (B.IndependentDistance1) R.IndependentDev = std::max(R.IndependentDev, B.IndependentDistance1(S.Contact1));
            if (B.IndependentDistance2) R.IndependentDev = std::max(R.IndependentDev, B.IndependentDistance2(S.Contact2));
        }
        if (B.AnalyticDeviation) R.AnalyticDev = std::max(R.AnalyticDev, B.AnalyticDeviation(S.Spine, S.Centre, S.Contact1, B.Size));
    }
    R.MeanIterations /= std::max(1, R.Stations);
    return R;
}

// 📝 Cross-section curve between the two contacts: circular arc about the centre (fillet) or straight chord (chamfer).
static std::vector<Vec3> SectionPolyline(const BlendSpecimen& B, const StationSolution& S, int Segments)
{
    std::vector<Vec3> Out;
    Vec3 A = S.Contact1 - S.Centre, C = S.Contact2 - S.Centre;
    double Φ = std::acos(std::max(-1.0, std::min(1.0, Dot(Normalised(A), Normalised(C)))));
    for (int K = 0; K <= Segments; ++K)
    {
        double W = (double)K / Segments;
        if (B.Profile == BlendProfile::Chamfer || Φ < 1e-9) { Out.push_back(S.Contact1 + W * (S.Contact2 - S.Contact1)); continue; }
        double Wa = std::sin((1 - W) * Φ) / std::sin(Φ), Wc = std::sin(W * Φ) / std::sin(Φ);
        Out.push_back(S.Centre + Wa * A + Wc * C);
    }
    return Out;
}

struct ViewProjection
{
    Vec3   Right, Up;
    double Scale = 1, OffsetX = 0, OffsetY = 0;
    void Project(Vec3 P, double& X, double& Y) const { X = OffsetX + Scale * Dot(P, Right); Y = OffsetY - Scale * Dot(P, Up); }
};

static void WritePolyline(FILE* F, const ViewProjection& View, const std::vector<Vec3>& Points, const char* Colour, double Width, double Opacity)
{
    std::fprintf(F, "<polyline fill='none' stroke='%s' stroke-width='%.2f' stroke-opacity='%.2f' points='", Colour, Width, Opacity);
    for (Vec3 P : Points) { double X, Y; View.Project(P, X, Y); std::fprintf(F, "%.2f,%.2f ", X, Y); }
    std::fprintf(F, "'/>\n");
}

static void WriteIsoLines(FILE* F, const ViewProjection& View, const SupportSurface& S, const char* Colour)
{
    const ParameterDomain& D = S.DisplayDomain;
    for (int I = 0; I <= 10; ++I)
    {
        std::vector<Vec3> AlongV, AlongU;
        for (int K = 0; K <= 48; ++K)
        {
            AlongV.push_back(S.Evaluate(D.U0 + (D.U1 - D.U0) * I / 10.0, D.V0 + (D.V1 - D.V0) * K / 48.0).Point);
            AlongU.push_back(S.Evaluate(D.U0 + (D.U1 - D.U0) * K / 48.0, D.V0 + (D.V1 - D.V0) * I / 10.0).Point);
        }
        WritePolyline(F, View, AlongV, Colour, 0.6, 0.35);
        WritePolyline(F, View, AlongU, Colour, 0.6, 0.35);
    }
}

static void WritePanel(FILE* F, const BlendSpecimen& B, const std::vector<StationSolution>& Path, const SpecimenReport& R,
                       double PanelX, double PanelY, double PanelW, double PanelH)
{
    ViewProjection View;
    Vec3 Eye = Normalised(Vec3{ 1.0, -1.35, 0.9 });
    View.Right = Normalised(Cross(UnitZ(), Eye));
    View.Up    = Normalised(Cross(Eye, View.Right));
    std::vector<Vec3> Every;
    for (const SupportSurface* S : { B.Support1.get(), B.Support2.get() })
        for (int I = 0; I <= 8; ++I) for (int J = 0; J <= 8; ++J)
        {
            const ParameterDomain& D = S->DisplayDomain;
            Every.push_back(S->Evaluate(D.U0 + (D.U1 - D.U0) * I / 8.0, D.V0 + (D.V1 - D.V0) * J / 8.0).Point);
        }
    double X0 = 1e300, X1 = -1e300, Y0 = 1e300, Y1 = -1e300;
    for (Vec3 P : Every) { double X = Dot(P, View.Right), Y = Dot(P, View.Up); X0 = std::min(X0, X); X1 = std::max(X1, X); Y0 = std::min(Y0, Y); Y1 = std::max(Y1, Y); }
    View.Scale   = std::min((PanelW - 30) / (X1 - X0), (PanelH - 70) / (Y1 - Y0));
    View.OffsetX = PanelX + PanelW / 2 - View.Scale * (X0 + X1) / 2;
    View.OffsetY = PanelY + 40 + (PanelH - 70) / 2 + View.Scale * (Y0 + Y1) / 2;
    std::fprintf(F, "<rect x='%.1f' y='%.1f' width='%.1f' height='%.1f' fill='#1b1d22' stroke='#3a3f48'/>\n", PanelX, PanelY, PanelW, PanelH);
    WriteIsoLines(F, View, *B.Support1, "#8a93a6");
    WriteIsoLines(F, View, *B.Support2, "#8a93a6");
    std::vector<Vec3> Spine, Rail1, Rail2;
    for (size_t I = 0; I < Path.size(); ++I)
    {
        Spine.push_back(Path[I].Spine); Rail1.push_back(Path[I].Contact1); Rail2.push_back(Path[I].Contact2);
        if (I % 2 == 0) WritePolyline(F, View, SectionPolyline(B, Path[I], 16), "#e8c35a", 1.1, 0.95);
    }
    WritePolyline(F, View, Spine, "#ff5d5d", 1.0, 0.8);
    WritePolyline(F, View, Rail1, "#4fb4ff", 1.8, 1.0);
    WritePolyline(F, View, Rail2, "#58e08a", 1.8, 1.0);
    std::fprintf(F, "<text x='%.1f' y='%.1f' fill='#e6e8ee' font-family='DejaVu Sans' font-size='14' font-weight='bold'>%s</text>\n", PanelX + 10, PanelY + 20, R.Title.c_str());
    std::fprintf(F, "<text x='%.1f' y='%.1f' fill='#aab1bf' font-family='DejaVu Sans' font-size='11'>%s · %d/%d stations · max residual %.1e m</text>\n",
                 PanelX + 10, PanelY + 36, R.Convexity, R.Converged, R.Stations, R.Residual);
}

static void WriteContactSheet(const char* Path, const char* Heading, const std::vector<BlendSpecimen>& Specimens,
                              const std::vector<std::vector<StationSolution>>& Paths, const std::vector<SpecimenReport>& Reports)
{
    FILE* F = std::fopen(Path, "w");
    if (!F) return;
    const double PanelW = 420, PanelH = 330;
    std::fprintf(F, "<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d'>\n<rect width='100%%' height='100%%' fill='#111317'/>\n",
                 (int)(3 * PanelW + 40), (int)(3 * PanelH + 110));
    std::fprintf(F, "<text x='20' y='34' fill='#ffffff' font-family='DejaVu Sans' font-size='20' font-weight='bold'>%s</text>\n", Heading);
    std::fprintf(F, "<text x='20' y='58' fill='#aab1bf' font-family='DejaVu Sans' font-size='13'>One solver, zero surface-type branches. "
                    "red = sharp edge (spine) · blue / green = contact rails · gold = solved cross-sections · grey = support iso-lines</text>\n");
    for (size_t I = 0; I < Specimens.size(); ++I)
        WritePanel(F, Specimens[I], Paths[I], Reports[I], 10 + (I % 3) * (PanelW + 10), 80 + (I / 3) * (PanelH + 10), PanelW, PanelH);
    std::fprintf(F, "</svg>\n");
    std::fclose(F);
}

} // namespace Frontier

//------------------------------------------------------------------------------------------------------------------------
//                                                        MAIN
//------------------------------------------------------------------------------------------------------------------------

int main(int ArgumentCount, char** Arguments)
{
    using namespace Frontier;
    std::string Folder = ArgumentCount > 1 ? Arguments[1] : ".";
    std::string Table  = Folder + "/RollingBallStationResults.md";
    FILE* Out = std::fopen(Table.c_str(), "w");
    if (!Out) { std::fprintf(stderr, "cannot write %s\n", Table.c_str()); return 2; }
    int Failures = 0;
    std::fprintf(Out, "# Rolling-ball station validation results\n\n");
    for (BlendProfile Profile : { BlendProfile::Fillet, BlendProfile::Chamfer })
    {
        const bool Fillet = Profile == BlendProfile::Fillet;
        std::vector<BlendSpecimen> Specimens = AssembleSpecimens(Profile);
        std::vector<std::vector<StationSolution>> Paths;
        std::vector<SpecimenReport> Reports;
        std::fprintf(Out, "## %s\n\n", Fillet ? "Fillet (constant-radius rolling ball)" : "Chamfer (equal chord setback)");
        std::fprintf(Out, "| Specimen | Edge | Stations | Converged | Max residual [m] | %s [m] | Normal dev [rad] | Independent dev [m] | Closed-form dev [m] | Mean Newton its |\n",
                     Fillet ? "Ball dev" : "Setback dev");
        std::fprintf(Out, "|---|---|---|---|---|---|---|---|---|---|\n");
        for (const BlendSpecimen& B : Specimens)
        {
            double σ = 1;
            std::vector<StationSolution> Path = MarchSpine(B, 160, σ);
            SpecimenReport R = VerifySpecimen(B, Path, σ);
            bool Pass = R.Converged == R.Stations && R.BallDeviation < 1e-9 && (R.IndependentDev < 0 || R.IndependentDev < 1e-7) &&
                        (R.AnalyticDev < 0 || R.AnalyticDev < 1e-9) && (!Fillet || R.NormalDeviation < 1e-6);
            Failures += Pass ? 0 : 1;
            auto Cell = [](double V) { char T[32]; if (V < 0) std::snprintf(T, 32, "n/a"); else std::snprintf(T, 32, "%.1e", V); return std::string(T); };
            std::fprintf(Out, "| %s | %s | %d | %d %s | %.1e | %.1e | %s | %s | %s | %.1f |\n", R.Title.c_str(), R.Convexity, R.Stations, R.Converged,
                         Pass ? "PASS" : "FAIL", R.Residual, R.BallDeviation, Fillet ? Cell(R.NormalDeviation).c_str() : "n/a",
                         Cell(R.IndependentDev).c_str(), Cell(R.AnalyticDev).c_str(), R.MeanIterations);
            std::printf("%-8s %-36s %-8s %3d/%3d  res %.1e  ball %.1e  indep %s  analytic %s  %s\n", Fillet ? "fillet" : "chamfer", R.Title.c_str(),
                        R.Convexity, R.Converged, R.Stations, R.Residual, R.BallDeviation, Cell(R.IndependentDev).c_str(), Cell(R.AnalyticDev).c_str(),
                        Pass ? "PASS" : "FAIL");
            Paths.push_back(Path); Reports.push_back(R);
        }
        std::fprintf(Out, "\n");
        std::string Sheet = Folder + (Fillet ? "/RollingBallFilletSheet.svg" : "/RollingBallChamferSheet.svg");
        WriteContactSheet(Sheet.c_str(), Fillet ? "Generic rolling-ball FILLET — 9 support pairs, 1 solver"
                                                : "Generic equal-setback CHAMFER — 9 support pairs, 1 solver", Specimens, Paths, Reports);
    }
    std::fclose(Out);
    std::printf("%s\n", Failures ? "VALIDATION FAILED" : "ALL SPECIMENS PASS");
    return Failures ? 1 : 0;
}
