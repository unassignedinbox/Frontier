#include "RollingBallBlend.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <cstdio>
#include <cstdlib>

namespace Frontier
{
namespace
{

thread_local std::string Diagnostic;                                                    // Refusal::Detail must outlive the call

const char* Explain(std::string Text) noexcept
{
    Diagnostic = std::move(Text);
    return Diagnostic.c_str();
}

constexpr double SolveTolerance  = 1e-12;                                               // [m] Newton target
constexpr double AcceptTolerance = 1e-8;                                                // [m] residual accepted as a solution
constexpr double JoinTolerance   = ScalarCriteria::MergeTolerance;                      // [m] topological coincidence
constexpr double FitTolerance    = 1e-6;                                                // [m] strip vs exact section between stations
constexpr int    TraceSamples    = 96;                                                  // [-] (u,v) polyline segments per section / end / mitre curve
constexpr double TangentTolerance = 1e-8;                                                 // [-] 1 − cos θ for G1 (θ ≈ 1.4e-4 rad)
inline std::string Scientific(double X) { char T[32]; std::snprintf(T, sizeof(T), "%.2e", X); return T; }

//==============================================================================================================
// Small dense algebra
//==============================================================================================================

bool SolveLinear(int N, double* A, double* B) noexcept                                  // A row-major N×N, B overwritten by X
{
    for (int Column = 0; Column < N; ++Column)
    {
        int Pivot = Column;
        for (int Row = Column + 1; Row < N; ++Row) if (std::fabs(A[Row * N + Column]) > std::fabs(A[Pivot * N + Column])) Pivot = Row;
        if (std::fabs(A[Pivot * N + Column]) < 1e-300) return false;
        if (Pivot != Column) { for (int K = 0; K < N; ++K) std::swap(A[Column * N + K], A[Pivot * N + K]); std::swap(B[Column], B[Pivot]); }
        for (int Row = Column + 1; Row < N; ++Row)
        {
            double Factor = A[Row * N + Column] / A[Column * N + Column];
            for (int K = Column; K < N; ++K) A[Row * N + K] -= Factor * A[Column * N + K];
            B[Row] -= Factor * B[Column];
        }
    }
    for (int Row = N - 1; Row >= 0; --Row)
    {
        double Sum = B[Row];
        for (int K = Row + 1; K < N; ++K) Sum -= A[Row * N + K] * B[K];
        B[Row] = Sum / A[Row * N + Row];
    }
    return true;
}

using Residual = std::function<void(const double* X, double* F)>;

// Damped Newton (square) / Gauss–Newton (overdetermined) with a central-difference Jacobian. Returns |F|.
double SolveNonlinear(int M, int N, const Residual& Fn, double* X, int Iterations = 60) noexcept
{
    double F[8], Fp[8], Fm[8], Ft[8], J[64], Xt[8], A[64], G[8];
    auto Norm = [M](const double* V) { double S = 0; for (int I = 0; I < M; ++I) S += V[I] * V[I]; return std::sqrt(S); };
    Fn(X, F);
    double R = Norm(F);
    for (int Iteration = 0; Iteration < Iterations && R > SolveTolerance; ++Iteration)
    {
        for (int Column = 0; Column < N; ++Column)
        {
            double H = 1e-7 * std::max(1.0, std::fabs(X[Column]));
            for (int K = 0; K < N; ++K) Xt[K] = X[K];
            Xt[Column] = X[Column] + H; Fn(Xt, Fp);
            Xt[Column] = X[Column] - H; Fn(Xt, Fm);
            for (int Row = 0; Row < M; ++Row) J[Row * N + Column] = (Fp[Row] - Fm[Row]) / (2.0 * H);
        }
        if (M == N) { for (int K = 0; K < N * N; ++K) A[K] = J[K]; for (int K = 0; K < N; ++K) G[K] = -F[K]; }
        else
        {
            for (int I = 0; I < N; ++I)
            {
                for (int K = 0; K < N; ++K) { double S = 0; for (int Row = 0; Row < M; ++Row) S += J[Row * N + I] * J[Row * N + K]; A[I * N + K] = S; }
                double S = 0; for (int Row = 0; Row < M; ++Row) S += J[Row * N + I] * F[Row]; G[I] = -S;
            }
        }
        if (!SolveLinear(N, A, G)) break;
        double Step = 1.0; bool Accepted = false;
        for (int Halving = 0; Halving < 16; ++Halving)
        {
            for (int K = 0; K < N; ++K) Xt[K] = X[K] + Step * G[K];
            Fn(Xt, Ft);
            double Rt = Norm(Ft);
            if (std::isfinite(Rt) && Rt < R)
            {
                for (int K = 0; K < N; ++K) X[K] = Xt[K];
                for (int K = 0; K < M; ++K) F[K] = Ft[K];
                R = Rt; Accepted = true; break;
            }
            Step *= 0.5;
        }
        if (!Accepted) break;
    }
    return R;
}

//==============================================================================================================
// Surface evaluation with periodic wrap and first-order extension beyond the domain
//==============================================================================================================

struct SurfaceProbe
{
    const NurbsSurface* Surface = nullptr;
    double Sign = 1.0;                                                                  // [-] −1 when the face is Reversed
    bool   WrapU = false, WrapV = false;
    double U0 = 0, U1 = 1, V0 = 0, V1 = 1;

    SurfaceProbe() = default;
    SurfaceProbe(const NurbsSurface& S, bool Reversed) noexcept : Surface(&S), Sign(Reversed ? -1.0 : 1.0)
    {
        U0 = S.DomainStartU(); U1 = S.DomainEndU(); V0 = S.DomainStartV(); V1 = S.DomainEndV();
        WrapU = S.ClosedU(ScalarCriteria::KernelTolerance); WrapV = S.ClosedV(ScalarCriteria::KernelTolerance);
    }

    static double Wrap(double X, double A, double B) noexcept
    {
        double P = B - A, R = std::fmod(X - A, P);
        if (R < 0) R += P;
        return A + R;
    }

    void Evaluate(double U, double V, Vec3& P, Vec3& DU, Vec3& DV) const noexcept
    {
        if (WrapU) U = Wrap(U, U0, U1);
        if (WrapV) V = Wrap(V, V0, V1);
        double Uc = ScalarCriteria::Clamp(U, U0, U1), Vc = ScalarCriteria::Clamp(V, V0, V1);
        Surface->Derivatives(Uc, Vc, P, DU, DV);
        P += DU * (U - Uc) + DV * (V - Vc);
    }
    [[nodiscard]] Vec3 Point(double U, double V) const noexcept { Vec3 P, DU, DV; Evaluate(U, V, P, DU, DV); return P; }
    [[nodiscard]] Vec3 Normal(double U, double V) const noexcept
    {
        Vec3 P, DU, DV; Evaluate(U, V, P, DU, DV);
        Vec3 N = DU.Cross(DV);
        if (N.Length() < 1e-14)
            N = Surface->Normal(ScalarCriteria::Clamp(WrapU ? Wrap(U, U0, U1) : U, U0, U1), ScalarCriteria::Clamp(WrapV ? Wrap(V, V0, V1) : V, V0, V1));
        return N.Normalised() * Sign;
    }
    // Gauss–Newton foot point from a seed, continuous across seams (no wrapping of the result).
    double Project(Vec3 Target, double& U, double& V) const noexcept
    {
        double X[2] = { U, V };
        SolveNonlinear(3, 2, [&](const double* Y, double* F) { Vec3 D = Point(Y[0], Y[1]) - Target; F[0] = D.X; F[1] = D.Y; F[2] = D.Z; }, X, 40);
        U = X[0]; V = X[1];
        return Point(U, V).Distance(Target);
    }
    // Seed for Project from a polyline of candidate parameters.
    void Nearest(Vec3 Target, const std::vector<Vec2>& Candidates, double& U, double& V) const noexcept
    {
        double Best = ScalarCriteria::Infinity;
        for (const Vec2& C : Candidates) { double D = Point(C.X, C.Y).Distance(Target); if (D < Best) { Best = D; U = C.X; V = C.Y; } }
        if (Candidates.empty()) Surface->ClosestParameter(Target, U, V);
    }
};

// Shift a continuous trace by whole periods into the domain, then clamp tiny overshoots.
void FitIntoDomain(const SurfaceProbe& Probe, std::vector<Vec2>& Trace) noexcept
{
    if (Trace.empty()) return;
    auto Shift = [](double Mid, double A, double B) { double P = B - A; return std::floor((Mid - A) / P) * P; };
    double MeanU = 0, MeanV = 0;
    for (const Vec2& T : Trace) { MeanU += T.X; MeanV += T.Y; }
    MeanU /= static_cast<double>(Trace.size()); MeanV /= static_cast<double>(Trace.size());
    double DU = Probe.WrapU ? Shift(MeanU, Probe.U0, Probe.U1) : 0.0;
    double DV = Probe.WrapV ? Shift(MeanV, Probe.V0, Probe.V1) : 0.0;
    for (Vec2& T : Trace)
    {
        T.X = ScalarCriteria::Clamp(T.X - DU, Probe.U0, Probe.U1);
        T.Y = ScalarCriteria::Clamp(T.Y - DV, Probe.V0, Probe.V1);
    }
}

double SignedArea(const std::vector<Vec2>& Ring) noexcept
{
    double A = 0;
    for (size_t I = 0; I < Ring.size(); ++I) { const Vec2& P = Ring[I]; const Vec2& Q = Ring[(I + 1) % Ring.size()]; A += P.X * Q.Y - Q.X * P.Y; }
    return 0.5 * A;
}

//==============================================================================================================
// Data
//==============================================================================================================

struct Station
{
    double T = 0;                                                                       // [-] spine parameter
    double X[4] = { 0, 0, 0, 0 };                                                       // [-] (u1,v1,u2,v2)
    Vec3   P[2];                                                                        // [m] contacts
    Vec3   Centre;                                                                      // [m] ball centre (fillet) / spine point (chamfer)
};

struct Piece
{
    NurbsCurve Curve;
    std::map<int, std::vector<Vec2>> FaceTraces;                                        // new face → (u,v) in curve order
    std::map<int, std::vector<Vec2>> CoedgeTraces;                                      // original coedge → (u,v) in curve order
};

enum class EndRange : uint8_t { Unresolved, Fixed, Extended };

struct BlendEnd
{
    EndRange Range = EndRange::Unresolved;
    double   FixedT = 0;                                                                // [-] spine parameter of a fixed end section
    bool     HasFixedX = false;
    double   FixedX[4] = { 0, 0, 0, 0 };
    bool     Section = false;                                                           // boundary is the iso-τ section
    std::vector<int> Pieces;                                                            // boundary from s=0 to s=1
    std::vector<bool> PieceReversed;
    double   TauRail[2] = { 0, 0 };                                                     // [-] where the boundary meets rail s=0 / s=1
};

struct BlendEdge
{
    int Edge = -1;
    int Coedge[2] = { -1, -1 };
    int Face[2] = { -1, -1 };
    SurfaceProbe Probe[2];
    double Sigma = 1.0;                                                                 // [-] +1 convex, −1 concave
    bool Closed = false;
    const NurbsCurve* Curve = nullptr;
    double T0 = 0, T1 = 1;                                                              // [-] edge domain
    double TA = 0, TB = 1;                                                              // [-] strip spine range
    BlendEnd Ends[2];
    std::vector<Station> Stations;
    NurbsSurface Strip;
    SurfaceProbe StripProbe;
    bool StripReversed = false;
    int  FaceKey = -1;
    int  RailPiece[2] = { -1, -1 };
    double Deviation = 0.0;                                                             // [m] final strip vs exact sections

    // The strip's along-direction parameter τ is normalised arc length of the spine: rational edge curves have speed jumps
    //    at their double knots, arc length does not.
    double Total = 1.0;                                                                 // [m] edge length

    // Arc-length table, built once per edge: 8-point Gauss-Legendre on 16 sub-intervals of every knot span (rational
    //    speed is smooth inside a span, so each panel is exact to round-off). Lookups add one partial panel; the inverse
    //    is bracketed Newton on that panel. Replaces per-call curve splitting and bisection (≈40 integrations per call).
    std::vector<double> PanelT, PanelL;                                                 // [-] / [m] panel starts, cumulative length
    [[nodiscard]] double Speed(double T) const noexcept { Vec3 D[2]; Curve->Derivatives(T, 1, D); return D[1].Length(); }
    [[nodiscard]] double PanelIntegral(double A, double B) const noexcept
    {
        static constexpr double X[4] = { 0.1834346424956498, 0.5255324099163290, 0.7966664774136267, 0.9602898564975363 };
        static constexpr double W[4] = { 0.3626837833783620, 0.3137066458778873, 0.2223810344533745, 0.1012285362903763 };
        const double M = 0.5 * (A + B), H = 0.5 * (B - A);
        double Sum = 0.0;
        for (int K = 0; K < 4; ++K) Sum += W[K] * (Speed(M - H * X[K]) + Speed(M + H * X[K]));
        return Sum * H;
    }
    void BuildLengthTable() noexcept
    {
        std::vector<double> Breaks;
        for (double K : Curve->Knots) if (K > T0 && K < T1 && (Breaks.empty() || K - Breaks.back() > 1e-12)) Breaks.push_back(K);
        Breaks.insert(Breaks.begin(), T0); Breaks.push_back(T1);
        PanelT.clear(); PanelL.assign(1, 0.0);
        for (size_t I = 0; I + 1 < Breaks.size(); ++I)
            for (int J = 0; J < 16; ++J)
            {
                double A = Breaks[I] + (Breaks[I + 1] - Breaks[I]) * J / 16.0, B = Breaks[I] + (Breaks[I + 1] - Breaks[I]) * (J + 1) / 16.0;
                PanelT.push_back(A); PanelL.push_back(PanelL.back() + PanelIntegral(A, B));
            }
        PanelT.push_back(T1);
        Total = PanelL.back();
    }
    [[nodiscard]] double TableLength(double T) const noexcept
    {
        size_t I = size_t(std::upper_bound(PanelT.begin(), PanelT.end(), T) - PanelT.begin());
        I = std::min(std::max<size_t>(I, 1), PanelT.size() - 1) - 1;
        return PanelL[I] + PanelIntegral(PanelT[I], T);
    }
    [[nodiscard]] double TableParameter(double L) const noexcept
    {
        size_t I = size_t(std::upper_bound(PanelL.begin(), PanelL.end(), L) - PanelL.begin());
        I = std::min(std::max<size_t>(I, 1), PanelL.size() - 1) - 1;
        double A = PanelT[I], B = PanelT[I + 1];
        double T = A + (B - A) * (L - PanelL[I]) / std::max(PanelL[I + 1] - PanelL[I], 1e-300);
        for (int K = 0; K < 8; ++K)
        {
            double F = PanelL[I] + PanelIntegral(PanelT[I], T) - L, S = Speed(T);
            if (F > 0) B = T; else A = T;
            double Next = T - F / std::max(S, 1e-300);
            if (!(Next > A && Next < B)) Next = 0.5 * (A + B);
            if (std::abs(Next - T) < 1e-15 * std::max(1.0, std::abs(T))) { T = Next; break; }
            T = Next;
        }
        return T;
    }
    [[nodiscard]] double EdgeSpeed(double T) const noexcept
    {
        double H = 1e-6 * (T1 - T0), A = ScalarCriteria::Clamp(T - H, T0, T1), C = ScalarCriteria::Clamp(T + H, T0, T1);
        return Curve->Sample(C).Distance(Curve->Sample(A)) / std::max(C - A, 1e-300);
    }
    [[nodiscard]] double LengthAt(double T) const noexcept
    {
        if (Closed && T < T0) return LengthAt(T + (T1 - T0)) - Total;
        if (Closed && T > T1) return Total + LengthAt(T - (T1 - T0));
        if (T <= T0) return (T - T0) * EdgeSpeed(T0);
        if (T >= T1) return Total + (T - T1) * EdgeSpeed(T1);
        return TableLength(T);
    }
    [[nodiscard]] double SpineAtLength(double L) const noexcept
    {
        if (Closed && L < 0.0) return SpineAtLength(L + Total) - (T1 - T0);
        if (Closed && L > Total) return SpineAtLength(L - Total) + (T1 - T0);
        if (L <= 0.0) return T0 + L / EdgeSpeed(T0);
        if (L >= Total) return T1 + (L - Total) / EdgeSpeed(T1);
        return TableParameter(L);
    }
    double LA = 0.0, LB = 1.0;                                                          // [m] arc length at TA / TB
    void Range(double A, double B) noexcept { TA = A; TB = B; LA = LengthAt(A); LB = LengthAt(B); }
    [[nodiscard]] double Tau(double T) const noexcept { return (LengthAt(T) - LA) / (LB - LA); }
    [[nodiscard]] double SpineT(double Tau) const noexcept { return SpineAtLength(LA + Tau * (LB - LA)); }
    [[nodiscard]] int Side(int FaceIndex) const noexcept { return Face[0] == FaceIndex ? 0 : (Face[1] == FaceIndex ? 1 : -1); }
};

//==============================================================================================================
// The construction
//==============================================================================================================

class Construction
{
public:
    Construction(const BrepBody& Body, double Size, BlendProfile Profile, RollingBlendReport& Report) noexcept
        : Body(Body), Size(Size), Profile(Profile), Report(Report) {}

    Refusal Run(const std::vector<int>& Edges, BrepBody& Result) noexcept;

private:
    const BrepBody& Body;
    double Size;
    BlendProfile Profile;
    RollingBlendReport& Report;

    std::vector<BlendEdge> Blends;
    std::map<int, int> BlendOfEdge;                                                     // original edge → Blends index
    std::vector<Piece> Pieces;
    std::map<std::pair<int, int>, Vec3> Trims;                                          // (edge, 0=start/1=end) → new end point
    std::map<std::pair<int, int>, std::vector<int>> Connectors;                         // (face, vertex) → pieces
    std::map<int, int> PlainPieces;                                                     // original edge → piece
    struct Corner { NurbsSurface Surface; SurfaceProbe Probe; bool Reversed = false; std::vector<int> Pieces; int FaceKey = -1; Vec3 Centre; };
    std::vector<Corner> Corners;
    std::set<int> Touched;                                                              // original faces that are rebuilt

    // geometry
    void SpineAt(const BlendEdge& B, double T, Vec3& P, Vec3& Tangent) const noexcept;
    double Speed(const BlendEdge& B, double T) const noexcept;
    void StationResidual(const BlendEdge& B, double T, const double* X, double* F) const noexcept;
    bool SolveStation(const BlendEdge& B, double T, double* X, double* ResidualOut = nullptr) const noexcept;
    Station MakeStation(const BlendEdge& B, double T, const double* X) const noexcept;
    bool SeedStation(BlendEdge& B, double T, double* X) const noexcept;
    Vec3 InFaceDirection(int Coedge, Vec3 At, double T, const NurbsCurve& Curve) const noexcept;
    bool March(const BlendEdge& B, double TFrom, const double* XFrom, double TTo, int Steps, std::vector<Station>& Out, bool AllowTruncate) const noexcept;
    Refusal Walk(BlendEdge& B) noexcept;
    Refusal FitStrip(BlendEdge& B) noexcept;
    Vec3 ExactMidSection(const BlendEdge& B, const Station& S) const noexcept;

    // topology
    std::vector<int> IncidentEdges(int Vertex) const noexcept;
    int OtherFace(int Edge, int Face) const noexcept;
    int EdgeBetween(const std::vector<int>& Edges, int FaceA, int FaceB) const noexcept;
    int EndAt(int Edge, int Vertex) const noexcept { return Body.Edges[Edge].VertexStart == Vertex ? 0 : 1; }

    // events
    Refusal ClassifyVertices(std::vector<std::function<Refusal()>>& Later) noexcept;
    Refusal ResolveRim(int Vertex, const std::vector<int>& Incident) noexcept;
    Refusal ResolveEndOnFace(int Vertex, int Blend, int EndFace, const std::vector<int>& Incident) noexcept;
    Refusal ResolveContinuation(int Vertex, int A, int B, int Third) noexcept;
    Refusal ResolveMitre(int Vertex, int A, int B, int Third) noexcept;
    Refusal PrepareSphere(int Vertex, const int Blend[3], int Faces[3]) noexcept;
    Refusal ResolveSphere(int Vertex, const int Blend[3]) noexcept;
    Refusal ResolveTripleMitre(int Vertex, const int Blend[3]) noexcept;
    bool RailMeetRail(const BlendEdge& A, int SideA, const BlendEdge& B, int SideB, double& TauA, double& TauB) const noexcept;
    int  SectionPiece(BlendEdge& B, int End) noexcept;
    int  MitrePiece(BlendEdge& A, int SideStart, double TauStart, double SEnd, double TauEnd, BlendEdge& B, double SBStart, double TauBStart, double SBEnd, double TauBEnd, bool& Ok) noexcept;
    void AttachEnd(BlendEdge& B, int End, std::vector<int> PieceList, std::vector<bool> Reversed, double Tau0, double Tau1) noexcept;

    // assembly
    int  RailPieceOf(BlendEdge& B, int Side) noexcept;
    [[nodiscard]] bool EdgeSmoothAt(int Edge, int Vertex) const noexcept;
    [[nodiscard]] Vec3 EdgeTangentAt(int Edge, int Vertex) const noexcept;
    void PropagateTangentChains(std::set<int>& Selected) const noexcept;
    Refusal PlainPieceOf(int Edge, int& Out) noexcept;
    std::vector<Vec2> ProjectTrace(const SurfaceProbe& Probe, const NurbsCurve& Curve, std::vector<Vec2> Seeds, int Samples) const noexcept;
    Refusal Assemble(BrepBody& Result) noexcept;
};

//------------------------------------------------------------------------------------------------ spine & stations

double Construction::Speed(const BlendEdge& B, double T) const noexcept
{
    double H = 1e-6 * (B.T1 - B.T0);
    double A = ScalarCriteria::Clamp(T - H, B.T0, B.T1), C = ScalarCriteria::Clamp(T + H, B.T0, B.T1);
    return B.Curve->Sample(C).Distance(B.Curve->Sample(A)) / std::max(C - A, 1e-300);
}

void Construction::SpineAt(const BlendEdge& B, double T, Vec3& P, Vec3& Tangent) const noexcept
{
    double Tc = ScalarCriteria::Clamp(T, B.T0, B.T1);
    if (B.Closed) Tc = SurfaceProbe::Wrap(T, B.T0, B.T1), T = Tc;
    P = B.Curve->Sample(Tc);
    Tangent = B.Curve->Tangent(Tc);
    if (T != Tc) P += Tangent * ((T - Tc) * Speed(B, Tc));                              // straight extension past the vertex
}

void Construction::StationResidual(const BlendEdge& B, double T, const double* X, double* F) const noexcept
{
    Vec3 E, Tn; SpineAt(B, T, E, Tn);
    Vec3 P1 = B.Probe[0].Point(X[0], X[1]), P2 = B.Probe[1].Point(X[2], X[3]);
    if (Profile == BlendProfile::Fillet)
    {
        Vec3 C1 = P1 - B.Probe[0].Normal(X[0], X[1]) * (B.Sigma * Size);
        Vec3 C2 = P2 - B.Probe[1].Normal(X[2], X[3]) * (B.Sigma * Size);
        Vec3 D = C1 - C2;
        F[0] = D.X; F[1] = D.Y; F[2] = D.Z;
        F[3] = ((C1 + C2) * 0.5 - E).Dot(Tn);
    }
    else
    {
        Vec3 D1 = P1 - E, D2 = P2 - E;
        F[0] = (D1.LengthSquared() - Size * Size) / (2.0 * Size); F[1] = D1.Dot(Tn);
        F[2] = (D2.LengthSquared() - Size * Size) / (2.0 * Size); F[3] = D2.Dot(Tn);
    }
}

bool Construction::SolveStation(const BlendEdge& B, double T, double* X, double* ResidualOut) const noexcept
{
    double R = SolveNonlinear(4, 4, [&](const double* Y, double* F) { StationResidual(B, T, Y, F); }, X);
    if (ResidualOut) *ResidualOut = R;
    return R <= AcceptTolerance;
}

Station Construction::MakeStation(const BlendEdge& B, double T, const double* X) const noexcept
{
    Station S; S.T = T;
    for (int K = 0; K < 4; ++K) S.X[K] = X[K];
    S.P[0] = B.Probe[0].Point(X[0], X[1]);
    S.P[1] = B.Probe[1].Point(X[2], X[3]);
    if (Profile == BlendProfile::Fillet)
        S.Centre = ((S.P[0] - B.Probe[0].Normal(X[0], X[1]) * (B.Sigma * Size)) + (S.P[1] - B.Probe[1].Normal(X[2], X[3]) * (B.Sigma * Size))) * 0.5;
    else { Vec3 Tn; SpineAt(B, T, S.Centre, Tn); }
    return S;
}

// Direction into the face, perpendicular to the edge: interior lies left of the coedge seen from the outward normal.
Vec3 Construction::InFaceDirection(int Coedge, Vec3 At, double T, const NurbsCurve& Curve) const noexcept
{
    const BrepCoedge& C = Body.Coedges[Coedge];
    SurfaceProbe Probe(Body.Faces[C.Face].Surface, Body.Faces[C.Face].Reversed);
    double U = 0, V = 0;
    Probe.Nearest(At, Body.CoedgeTrace(Coedge), U, V);
    Probe.Project(At, U, V);
    Vec3 Tangent = Curve.Tangent(T) * (C.Reversed ? -1.0 : 1.0);
    return Probe.Normal(U, V).Cross(Tangent).Normalised();
}

bool Construction::SeedStation(BlendEdge& B, double T, double* X) const noexcept
{
    Vec3 E, Tn; SpineAt(B, T, E, Tn);
    Vec3 Bd[2];
    for (int K = 0; K < 2; ++K) Bd[K] = InFaceDirection(B.Coedge[K], E, ScalarCriteria::Clamp(T, B.T0, B.T1), *B.Curve);
    double Angle = std::acos(ScalarCriteria::Clamp(Bd[0].Dot(Bd[1]), -1.0, 1.0));
    double Reach = Profile == BlendProfile::Fillet ? Size / std::tan(0.5 * std::max(Angle, 1e-3)) : Size;
    for (int K = 0; K < 2; ++K)
    {
        double U = 0, V = 0;
        B.Probe[K].Nearest(E, Body.CoedgeTrace(B.Coedge[K]), U, V);
        B.Probe[K].Project(E, U, V);
        B.Probe[K].Project(E + Bd[K] * Reach, U, V);
        X[2 * K] = U; X[2 * K + 1] = V;
    }
    return SolveStation(B, T, X);
}

// Continuation from (TFrom, XFrom) to TTo in Steps; failed steps are bisected. Appends the stations after TFrom.
bool Construction::March(const BlendEdge& B, double TFrom, const double* XFrom, double TTo, int Steps, std::vector<Station>& Out, bool AllowTruncate) const noexcept
{
    double X[4], Previous[4]; bool HavePrevious = false; double TPrevious = TFrom;
    for (int K = 0; K < 4; ++K) X[K] = XFrom[K];
    double T = TFrom;
    std::function<bool(double, int)> Advance = [&](double Target, int Depth) -> bool
    {
        double Y[4];
        for (int K = 0; K < 4; ++K)                                                     // secant predictor
            Y[K] = HavePrevious && std::fabs(T - TPrevious) > 1e-300 ? X[K] + (X[K] - Previous[K]) * ((Target - T) / (T - TPrevious)) : X[K];
        if (!SolveStation(B, Target, Y))
        {
            for (int K = 0; K < 4; ++K) Y[K] = X[K];
            if (!SolveStation(B, Target, Y))
            {
                if (Depth > 7) return false;
                double Mid = 0.5 * (T + Target);
                return Advance(Mid, Depth + 1) && Advance(Target, Depth + 1);
            }
        }
        // a jump to another solution branch shows up as a large contact move
        Vec3 Was0 = B.Probe[0].Point(X[0], X[1]), Now0 = B.Probe[0].Point(Y[0], Y[1]);
        Vec3 E0, E1, Tn; SpineAt(B, T, E0, Tn); SpineAt(B, Target, E1, Tn);
        if (Now0.Distance(Was0) > 4.0 * E1.Distance(E0) + 0.5 * Size)
        {
            if (Depth > 7) return false;
            double Mid = 0.5 * (T + Target);
            return Advance(Mid, Depth + 1) && Advance(Target, Depth + 1);
        }
        for (int K = 0; K < 4; ++K) { Previous[K] = X[K]; X[K] = Y[K]; }
        HavePrevious = true; TPrevious = T; T = Target;
        Out.push_back(MakeStation(B, T, X));
        return true;
    };
    for (int Step = 1; Step <= Steps; ++Step)
    {
        double Target = TFrom + (TTo - TFrom) * Step / Steps;
        if (!Advance(Target, 0)) return AllowTruncate && !Out.empty();
    }
    return true;
}

Vec3 Construction::ExactMidSection(const BlendEdge& B, const Station& S) const noexcept
{
    if (Profile == BlendProfile::Chamfer) return (S.P[0] + S.P[1]) * 0.5;
    Vec3 Mid = ((S.P[0] - S.Centre) + (S.P[1] - S.Centre)).Normalised();
    return S.Centre + Mid * Size;
}

Refusal Construction::Walk(BlendEdge& B) noexcept
{
    double Length = B.Curve->Length();
    int Steps = std::clamp(static_cast<int>(std::ceil(Length / (0.25 * Size))), 8, 64);
    if (B.Closed) Steps = std::max(Steps, 32);
    for (int Attempt = 0; Attempt < 4; ++Attempt, Steps *= 2)
    {
        std::vector<Station> Back, Front;
        double Start = B.Ends[0].Range == EndRange::Fixed ? B.Ends[0].FixedT : (B.Closed ? B.T0 : 0.5 * (B.T0 + B.T1));
        double X[4];
        if (B.Ends[0].Range == EndRange::Fixed && B.Ends[0].HasFixedX) { for (int K = 0; K < 4; ++K) X[K] = B.Ends[0].FixedX[K]; if (!SolveStation(B, Start, X)) return Refusal::Reject(RefusalReason::NoConvergence, Explain("fixed start section of edge " + std::to_string(B.Edge) + " does not solve")); }
        else if (!SeedStation(B, Start, X)) return Refusal::Reject(RefusalReason::NoConvergence, Explain("no rolling-ball station found on edge " + std::to_string(B.Edge) + " (radius too large for the adjacent faces?)"));

        double Low = B.Ends[0].Range == EndRange::Fixed ? B.Ends[0].FixedT : B.TA;
        double High = B.Ends[1].Range == EndRange::Fixed ? B.Ends[1].FixedT : B.TB;
        if (B.Closed) { Low = B.T0; High = B.T1; }
        double Span = High - Low;
        auto StepsFor = [&](double From, double To) { return std::max(2, static_cast<int>(std::ceil(Steps * std::fabs(To - From) / std::max(Span, 1e-300)))); };
        if (Start > Low + 1e-12 && !March(B, Start, X, Low, StepsFor(Start, Low), Back, B.Ends[0].Range == EndRange::Extended))
            return Refusal::Reject(RefusalReason::NoConvergence, Explain("station walk failed towards the start of edge " + std::to_string(B.Edge)));
        if (High > Start + 1e-12 && !March(B, Start, X, High, StepsFor(Start, High), Front, B.Ends[1].Range == EndRange::Extended))
            return Refusal::Reject(RefusalReason::NoConvergence, Explain("station walk failed towards the end of edge " + std::to_string(B.Edge)));
        size_t SeamIndex = Front.size();                                                // closed: index of the station at T1
        if (B.Closed)
        {
            // walk a few stations past the seam both ways so the interpolation has no end effect there
            double Overlap = 4.0 * Span / Steps;
            if (!March(B, B.T1, Front.back().X, B.T1 + Overlap, 4, Front, false) || !March(B, B.T0, X, B.T0 - Overlap, 4, Back, false))
                return Refusal::Reject(RefusalReason::NoConvergence, Explain("closed edge " + std::to_string(B.Edge) + ": walk past the seam failed"));
            SeamIndex = Back.size() + Front.size() - 4;
        }
        B.Stations.clear();
        for (auto It = Back.rbegin(); It != Back.rend(); ++It) B.Stations.push_back(*It);
        B.Stations.push_back(MakeStation(B, Start, X));
        for (const Station& S : Front) B.Stations.push_back(S);
        // pin fixed end sections exactly
        for (int End = 0; End < 2; ++End)
            if (B.Ends[End].Range == EndRange::Fixed && B.Ends[End].HasFixedX)
            {
                Station& S = End == 0 ? B.Stations.front() : B.Stations.back();
                double Y[4]; for (int K = 0; K < 4; ++K) Y[K] = B.Ends[End].FixedX[K];
                if (std::fabs(S.T - B.Ends[End].FixedT) > 1e-9 || !SolveStation(B, S.T, Y)) return Refusal::Reject(RefusalReason::NoConvergence, Explain("end section of edge " + std::to_string(B.Edge) + " was not reached"));
                S = MakeStation(B, S.T, B.Ends[End].FixedX);
            }
        size_t StartIndex = Back.size();
        if (B.Closed)
        {
            Station& First = B.Stations[StartIndex]; Station& Last = B.Stations[SeamIndex];
            if (Last.P[0].Distance(First.P[0]) > 1e-7 || Last.P[1].Distance(First.P[1]) > 1e-7)
                return Refusal::Reject(RefusalReason::NoConvergence, Explain("closed edge " + std::to_string(B.Edge) + ": the walk did not return to its start"));
            double T = Last.T; double Xs[4]; for (int K = 0; K < 4; ++K) Xs[K] = Last.X[K];
            Last = First; Last.T = T; for (int K = 0; K < 4; ++K) Last.X[K] = Xs[K];
        }
        // a contact rail that runs against its edge has passed the support face's centre of curvature (size larger than
        //    the local radius): the section sweep folds through itself although the topology still closes
        for (size_t I = 0; I + 1 < B.Stations.size(); ++I)
        {
            const Station& S0 = B.Stations[I]; const Station& S1 = B.Stations[I + 1];
            Vec3 Along, Point; SpineAt(B, 0.5 * (S0.T + S1.T), Point, Along);              // extends past the vertices, wraps closed edges
            for (int Side = 0; Side < 2; ++Side)
                if ((S1.P[Side] - S0.P[Side]).Dot(Along) < 0.0)
                    return Refusal::Reject(RefusalReason::Unsupported, Explain("size exceeds the curvature radius of face " + std::to_string(B.Face[Side])
                                                                               + " along edge " + std::to_string(B.Edge) + " (the blend would fold through itself)"));
        }
        B.Range(B.Stations.front().T, B.Stations.back().T);
        if (B.Stations.size() < 5) continue;
        Refusal Fit = FitStrip(B);
        if (!Fit && B.Closed)
        {
            // keep exactly one period: τ(T0) … τ(T1) remapped to [0, 1]
            double Ta = B.Tau(B.T0), Tb = B.Tau(B.T1);
            NurbsSurface Middle = B.Strip.SplitV(Tb).first.SplitV(Ta).second;
            for (double& K : Middle.KnotsV) K = ScalarCriteria::Clamp((K - Ta) / (Tb - Ta), 0.0, 1.0);
            B.Strip = std::move(Middle);
            B.StripProbe = SurfaceProbe(B.Strip, B.StripReversed);
            std::vector<Station> Kept(B.Stations.begin() + static_cast<long>(StartIndex), B.Stations.begin() + static_cast<long>(SeamIndex) + 1);
            B.Stations = std::move(Kept);
            B.Range(B.T0, B.T1);
        }
        if (!Fit) return Fit;
        if (Fit.Reason != RefusalReason::NoConvergence || Attempt == 3) return Fit;
    }
    return Refusal::Reject(RefusalReason::NoConvergence, Explain("strip of edge " + std::to_string(B.Edge) + " could not be fitted"));
}

// Rational quadratic arc (fillet) or line (chamfer) at each station, rows interpolated along the spine.
Refusal Construction::FitStrip(BlendEdge& B) noexcept
{
    const size_t N = B.Stations.size();
    const bool Fillet = Profile == BlendProfile::Fillet;
    const int Rows = Fillet ? 3 : 2;
    std::vector<std::vector<Vec4>> Through(Rows, std::vector<Vec4>(N));
    std::vector<double> Tau(N);
    for (size_t K = 0; K < N; ++K)
    {
        const Station& S = B.Stations[K];
        Tau[K] = B.Tau(S.T);
        Through[0][K] = Vec4(S.P[0], 1.0);
        Through[Rows - 1][K] = Vec4(S.P[1], 1.0);
        if (Fillet)
        {
            Vec3 A = S.P[0] - S.Centre, C = S.P[1] - S.Centre;
            double CosPhi = ScalarCriteria::Clamp(A.Dot(C) / (A.Length() * C.Length()), -1.0, 1.0);
            if (CosPhi < std::cos(ScalarCriteria::Radians(175.0))) return Refusal::Reject(RefusalReason::Unsupported, Explain("fillet arc on edge " + std::to_string(B.Edge) + " spans more than 175°"));
            Vec3 M = S.Centre + (A + C) / (1.0 + CosPhi);
            double W = std::sqrt(0.5 * (1.0 + CosPhi));                                 // cos(φ/2)
            Through[1][K] = Vec4(M * W, W);
        }
    }
    Tau.front() = 0.0; Tau.back() = 1.0;
    std::vector<NurbsCurve> Columns;
    for (int R = 0; R < Rows; ++R)
    {
        Deliver<NurbsCurve> C = NurbsCurve::InterpolateHomogeneous(Through[R], 3, &Tau);
        if (!C) return Refusal::Reject(C.Denial.Reason, C.Denial.Detail);
        Columns.push_back(std::move(C.Payload));
    }
    const int CountV = Columns[0].PoleCount();
    std::vector<Vec4> Poles(static_cast<size_t>(Rows) * CountV);
    for (int R = 0; R < Rows; ++R) for (int J = 0; J < CountV; ++J) Poles[static_cast<size_t>(R) * CountV + J] = Columns[R].Poles[J];
    std::vector<double> KnotsU = Fillet ? std::vector<double>{ 0, 0, 0, 1, 1, 1 } : std::vector<double>{ 0, 0, 1, 1 };
    Deliver<NurbsSurface> S = NurbsSurface::Build(Rows - 1, 3, Rows, CountV, std::move(Poles), KnotsU, Columns[0].Knots);
    if (!S) return Refusal::Reject(S.Denial.Reason, S.Denial.Detail);
    B.Strip = std::move(S.Payload);
    B.Strip.Classification = SurfaceClassification::Freeform;
    B.StripProbe = SurfaceProbe(B.Strip, false);

    // deviation between stations against freshly solved exact sections
    double Worst = 0.0;
    for (size_t K = 0; K + 1 < N; ++K)
    {
        double T = 0.5 * (B.Stations[K].T + B.Stations[K + 1].T), X[4];
        for (int J = 0; J < 4; ++J) X[J] = 0.5 * (B.Stations[K].X[J] + B.Stations[K + 1].X[J]);
        double R = 0;
        if (!SolveStation(B, T, X, &R)) continue;
        Station Exact = MakeStation(B, T, X);
        double V = B.Tau(T);
        // geometric deviation: exact section points against the strip surface (foot points, not equal parameters —
        //    a noisy spine tangent re-parameterises the strip without moving it)
        Vec3 Exacts[3] = { Exact.P[0], ExactMidSection(B, Exact), Exact.P[1] };
        const double Across[3] = { 0.0, 0.5, 1.0 };
        for (int J = 0; J < 3; ++J)
        {
            double U = Across[J], W = V;
            Worst = std::max(Worst, B.StripProbe.Project(Exacts[J], U, W));
        }
    }
    B.Deviation = Worst;
    if (Worst > FitTolerance * std::max(1.0, Size)) return Refusal::Reject(RefusalReason::NoConvergence, Explain("strip of edge " + std::to_string(B.Edge) + " deviates " + std::to_string(Worst) + " m from the exact sections between stations"));

    // orientation: outward away from the ball centre (convex) / towards it (concave)
    const Station& Mid = B.Stations[N / 2];
    Vec3 P, DU, DV; B.Strip.Derivatives(0.5, B.Tau(Mid.T), P, DU, DV);
    Vec3 Outward = Fillet ? (P - Mid.Centre) * B.Sigma : (Mid.Centre - P) * B.Sigma;
    B.StripReversed = DU.Cross(DV).Dot(Outward) < 0.0;
    B.StripProbe = SurfaceProbe(B.Strip, B.StripReversed);
    double R = 0;
    for (const Station& St : B.Stations) { double F[4]; StationResidual(B, St.T, St.X, F); for (double V : F) R = std::max(R, std::fabs(V)); }
    Report.MaxResidual = std::max(Report.MaxResidual, R);
    Report.Stations += static_cast<int>(N);
    return Refusal::Accept();
}

//------------------------------------------------------------------------------------------------ topology helpers

std::vector<int> Construction::IncidentEdges(int Vertex) const noexcept
{
    std::vector<int> Out;
    for (size_t E = 0; E < Body.Edges.size(); ++E)
        if (Body.Edges[E].VertexStart == Vertex || Body.Edges[E].VertexEnd == Vertex) Out.push_back(static_cast<int>(E));
    return Out;
}

int Construction::OtherFace(int Edge, int Face) const noexcept
{
    for (int C : Body.Edges[Edge].Coedges) if (Body.Coedges[C].Face != Face) return Body.Coedges[C].Face;
    return -1;
}

int Construction::EdgeBetween(const std::vector<int>& Edges, int FaceA, int FaceB) const noexcept
{
    for (int E : Edges)
    {
        const auto& Cs = Body.Edges[E].Coedges;
        if (Cs.size() != 2) continue;
        int A = Body.Coedges[Cs[0]].Face, B = Body.Coedges[Cs[1]].Face;
        if ((A == FaceA && B == FaceB) || (A == FaceB && B == FaceA)) return E;
    }
    return -1;
}

//------------------------------------------------------------------------------------------------ pieces

void Construction::AttachEnd(BlendEdge& B, int End, std::vector<int> PieceList, std::vector<bool> Reversed, double Tau0, double Tau1) noexcept
{
    B.Ends[End].Pieces = std::move(PieceList);
    B.Ends[End].PieceReversed = std::move(Reversed);
    B.Ends[End].TauRail[0] = Tau0;
    B.Ends[End].TauRail[1] = Tau1;
}

// Iso-τ section of a strip at one of its ends (exact arc / line because the end is a station).
int Construction::SectionPiece(BlendEdge& B, int End) noexcept
{
    double Tau = End == 0 ? 0.0 : 1.0;
    Piece P;
    P.Curve = B.Strip.IsoCurveV(Tau);
    std::vector<Vec2> Trace;
    for (int K = 0; K <= TraceSamples; ++K) Trace.emplace_back(static_cast<double>(K) / TraceSamples, Tau);
    P.FaceTraces[B.FaceKey] = Trace;
    Pieces.push_back(std::move(P));
    return static_cast<int>(Pieces.size() - 1);
}

bool Construction::RailMeetRail(const BlendEdge& A, int SideA, const BlendEdge& B, int SideB, double& TauA, double& TauB) const noexcept
{
    double X[2] = { TauA, TauB };
    double R = SolveNonlinear(3, 2, [&](const double* Y, double* F) {
        Vec3 D = A.StripProbe.Point(SideA, Y[0]) - B.StripProbe.Point(SideB, Y[1]);
        F[0] = D.X; F[1] = D.Y; F[2] = D.Z; }, X);
    TauA = X[0]; TauB = X[1];
    return R <= AcceptTolerance;
}

std::vector<Vec2> Construction::ProjectTrace(const SurfaceProbe& Probe, const NurbsCurve& Curve, std::vector<Vec2> Seeds, int Samples) const noexcept
{
    std::vector<Vec2> Out;
    double U = 0, V = 0;
    const double A = Curve.DomainStart(), B = Curve.DomainEnd();
    for (int K = 0; K <= Samples; ++K)
    {
        Vec3 P = Curve.Sample(A + (B - A) * K / Samples);
        if (K == 0) Probe.Nearest(P, Seeds, U, V);
        else if (static_cast<int>(Seeds.size()) == Samples + 1) { U = Seeds[K].X; V = Seeds[K].Y; }
        Probe.Project(P, U, V);
        Out.emplace_back(U, V);
    }
    // unwrap: a projection may have landed one period away
    for (size_t K = 1; K < Out.size(); ++K)
    {
        if (Probe.WrapU) { double P = Probe.U1 - Probe.U0; while (Out[K].X - Out[K - 1].X > 0.5 * P) Out[K].X -= P; while (Out[K - 1].X - Out[K].X > 0.5 * P) Out[K].X += P; }
        if (Probe.WrapV) { double P = Probe.V1 - Probe.V0; while (Out[K].Y - Out[K - 1].Y > 0.5 * P) Out[K].Y -= P; while (Out[K - 1].Y - Out[K].Y > 0.5 * P) Out[K].Y += P; }
    }
    FitIntoDomain(Probe, Out);
    return Out;
}

// Strip-strip intersection from (A at SideStart, TauStart) to (A at SEnd, TauEnd), parameterised by s on A.
int Construction::MitrePiece(BlendEdge& A, int SideStart, double TauStart, double SEnd, double TauEnd, BlendEdge& B, double SBStart, double TauBStart, double SBEnd, double TauBEnd, bool& Ok) noexcept
{
    const int Samples = TraceSamples;
    std::vector<Vec3> Points(Samples + 1);
    std::vector<Vec2> OnA(Samples + 1), OnB(Samples + 1);
    double S0 = SideStart;
    OnA[0] = { S0, TauStart }; OnB[0] = { SBStart, TauBStart };
    OnA[Samples] = { SEnd, TauEnd }; OnB[Samples] = { SBEnd, TauBEnd };
    Ok = true;
    // march from the transversal end inwards: a fillet mitre starts at a tangency on the shared face
    double X[3] = { TauEnd, SBEnd, TauBEnd };
    for (int K = Samples - 1; K >= 1; --K)
    {
        double S = S0 + (SEnd - S0) * K / Samples;
        double R = SolveNonlinear(3, 3, [&](const double* Y, double* F) {
            Vec3 D = A.StripProbe.Point(S, Y[0]) - B.StripProbe.Point(Y[1], Y[2]);
            F[0] = D.X; F[1] = D.Y; F[2] = D.Z; }, X);
        if (R > 1e-7) { Ok = false; return -1; }
        OnA[K] = { S, X[0] }; OnB[K] = { X[1], X[2] };
    }
    for (int K = 0; K <= Samples; ++K) Points[K] = A.StripProbe.Point(OnA[K].X, OnA[K].Y);
    Points[0] = 0.5 * (Points[0] + B.StripProbe.Point(OnB[0].X, OnB[0].Y));
    Points[Samples] = 0.5 * (Points[Samples] + B.StripProbe.Point(OnB[Samples].X, OnB[Samples].Y));
    Deliver<NurbsCurve> C = NurbsCurve::Interpolate(Points, 3, false);
    if (!C) { Ok = false; return -1; }
    Piece P; P.Curve = std::move(C.Payload);
    P.FaceTraces[A.FaceKey] = OnA;
    P.FaceTraces[B.FaceKey] = OnB;
    Pieces.push_back(std::move(P));
    ++Report.Mitres;
    return static_cast<int>(Pieces.size() - 1);
}

//------------------------------------------------------------------------------------------------ vertex events

Refusal Construction::ResolveRim(int Vertex, const std::vector<int>& Incident) noexcept
{
    int Closed = -1;
    for (int E : Incident) if (BlendOfEdge.count(E) && Body.Edges[E].Closed()) { if (Closed >= 0) return Refusal::Reject(RefusalReason::Unsupported, Explain("two blended closed edges share vertex " + std::to_string(Vertex))); Closed = E; }
    BlendEdge& B = Blends[BlendOfEdge[Closed]];
    for (int End = 0; End < 2; ++End) { B.Ends[End].Section = true; AttachEnd(B, End, { SectionPiece(B, End) }, { false }, End == 0 ? 0.0 : 1.0, End == 0 ? 0.0 : 1.0); }
    for (int E : Incident)
    {
        if (E == Closed) continue;
        const auto& Cs = Body.Edges[E].Coedges;
        bool Seam = Cs.size() == 2 && Body.Coedges[Cs[0]].Face == Body.Coedges[Cs[1]].Face;
        if (!Seam) return Refusal::Reject(RefusalReason::Unsupported, Explain("closed edge " + std::to_string(Closed) + " meets non-seam edge " + std::to_string(E) + " at vertex " + std::to_string(Vertex)));
        if (BlendOfEdge.count(E)) return Refusal::Reject(RefusalReason::Unsupported, Explain("seam edge " + std::to_string(E) + " cannot be blended"));
        int Side = B.Side(Body.Coedges[Cs[0]].Face);
        if (Side < 0) return Refusal::Reject(RefusalReason::Unsupported, Explain("seam edge " + std::to_string(E) + " is not on a face of blended edge " + std::to_string(Closed)));
        Trims[{ E, EndAt(E, Vertex) }] = B.Strip.Sample(static_cast<double>(Side), 0.0);   // the rail's own end (fitted strip), not the exact station
        if (Body.Edges[E].Closed()) return Refusal::Reject(RefusalReason::Unsupported, Explain("closed seam edge"));
    }
    return Refusal::Accept();
}

Refusal Construction::ResolveEndOnFace(int Vertex, int Index, int EndFace, const std::vector<int>& Incident) noexcept
{
    BlendEdge& B = Blends[Index];
    const int End = EndAt(B.Edge, Vertex);
    int Side0Edge = EdgeBetween(Incident, B.Face[0], EndFace), Side1Edge = EdgeBetween(Incident, B.Face[1], EndFace);
    if (Side0Edge < 0 || Side1Edge < 0) return Refusal::Reject(RefusalReason::Unsupported, Explain("vertex " + std::to_string(Vertex) + ": end face does not meet both blend faces"));
    SurfaceProbe Face3(Body.Faces[EndFace].Surface, Body.Faces[EndFace].Reversed);
    const int Samples = TraceSamples;
    std::vector<Vec3> Points(Samples + 1);
    std::vector<Vec2> OnStrip(Samples + 1), OnFace(Samples + 1);
    // seed: rail s=0 against the end face, starting from the section through the vertex
    double TauV = B.Tau(End == 0 ? B.T0 : B.T1);
    Vec3 Seed = B.StripProbe.Point(0.0, TauV);
    double U = 0, V = 0;
    std::vector<Vec2> Candidates;
    for (int C : Body.Edges[Side0Edge].Coedges) if (Body.Coedges[C].Face == EndFace) Candidates = Body.CoedgeTrace(C);
    Face3.Nearest(Seed, Candidates, U, V);
    Face3.Project(Seed, U, V);
    double X[3] = { TauV, U, V };
    for (int K = 0; K <= Samples; ++K)
    {
        double S = static_cast<double>(K) / Samples;
        double R = SolveNonlinear(3, 3, [&](const double* Y, double* F) {
            Vec3 D = B.StripProbe.Point(S, Y[0]) - Face3.Point(Y[1], Y[2]);
            F[0] = D.X; F[1] = D.Y; F[2] = D.Z; }, X);
        if (R > 1e-7) return Refusal::Reject(RefusalReason::NoConvergence, Explain("blend of edge " + std::to_string(B.Edge) + " does not reach end face " + std::to_string(EndFace) + " at vertex " + std::to_string(Vertex)));
        if (X[0] < -1e-9 || X[0] > 1.0 + 1e-9) return Refusal::Reject(RefusalReason::Unsupported, Explain("end face at vertex " + std::to_string(Vertex) + " lies beyond the strip extension"));
        OnStrip[K] = { S, X[0] }; OnFace[K] = { X[1], X[2] };
        Points[K] = B.StripProbe.Point(S, X[0]);
    }
    // the rails must land on the adjacent edges (no support hop)
    for (int K : { 0, Samples })
    {
        int Edge = K == 0 ? Side0Edge : Side1Edge;
        double D = 0; (void)Body.Edges[Edge].Curve.ClosestParameter(Points[K], &D);
        if (D > 1e-6) return Refusal::Reject(RefusalReason::Unsupported, Explain("rail of edge " + std::to_string(B.Edge) + " leaves its face before edge " + std::to_string(Edge) + " (support hop not implemented)"));
        Trims[{ Edge, EndAt(Edge, Vertex) }] = Points[K];
    }
    FitIntoDomain(Face3, OnFace);
    Deliver<NurbsCurve> C = NurbsCurve::Interpolate(Points, 3, false);
    if (!C) return Refusal::Reject(C.Denial.Reason, C.Denial.Detail);
    Piece P; P.Curve = std::move(C.Payload);
    P.FaceTraces[B.FaceKey] = OnStrip;
    P.FaceTraces[EndFace] = OnFace;
    Pieces.push_back(std::move(P));
    int Id = static_cast<int>(Pieces.size() - 1);
    Connectors[{ EndFace, Vertex }].push_back(Id);
    Touched.insert(EndFace);
    AttachEnd(B, End, { Id }, { false }, OnStrip.front().Y, OnStrip.back().Y);
    ++Report.EndCurves;
    return Refusal::Accept();
}

Refusal Construction::ResolveContinuation(int Vertex, int IA, int IB, int Third) noexcept
{
    BlendEdge& A = Blends[IA]; BlendEdge& B = Blends[IB];
    int EndA = EndAt(A.Edge, Vertex), EndB = EndAt(B.Edge, Vertex);
    int Id = SectionPiece(A, EndA);
    const Station& SA = EndA == 0 ? A.Stations.front() : A.Stations.back();
    const Station& SB = EndB == 0 ? B.Stations.front() : B.Stations.back();
    // shared face side and the matching orientation of the section on B
    int Shared = -1;
    for (int K = 0; K < 2; ++K) if (B.Side(A.Face[K]) >= 0) Shared = A.Face[K];
    int SideA = A.Side(Shared), SideB = B.Side(Shared);
    const double GapShared = SA.P[SideA].Distance(SB.P[SideB]), GapFar = SA.P[1 - SideA].Distance(SB.P[1 - SideB]);
    if (GapShared > JoinTolerance || GapFar > JoinTolerance)                            // fitted inputs (earlier blends) carry ~1e-7
        return Refusal::Reject(RefusalReason::Unsupported, Explain("tangent blends at vertex " + std::to_string(Vertex) + " do not share their end section (contact gaps "
                                                                   + Scientific(GapShared) + " / " + Scientific(GapFar) + " m)"));
    double TauB = EndB == 0 ? 0.0 : 1.0;
    std::vector<Vec2> OnB;
    for (int K = 0; K <= TraceSamples; ++K) { double S = static_cast<double>(K) / TraceSamples; OnB.emplace_back(SideA == SideB ? S : 1.0 - S, TauB); }
    Pieces[Id].FaceTraces[B.FaceKey] = OnB;
    A.Ends[EndA].Section = B.Ends[EndB].Section = true;
    double TA = EndA == 0 ? 0.0 : 1.0;
    AttachEnd(A, EndA, { Id }, { false }, TA, TA);
    AttachEnd(B, EndB, { Id }, { SideA != SideB }, TauB, TauB);
    // the smooth third edge is trimmed where the two rails on its faces meet
    Vec3 Y = A.Strip.Sample(static_cast<double>(1 - SideA), TA);                       // the rail's own end (fitted strip), not the exact station
    double D = 0; (void)Body.Edges[Third].Curve.ClosestParameter(Y, &D);
    if (D > JoinTolerance) return Refusal::Reject(RefusalReason::Unsupported, Explain("continuation at vertex " + std::to_string(Vertex) + ": end section misses the smooth edge " + std::to_string(Third)));
    Trims[{ Third, EndAt(Third, Vertex) }] = Y;
    return Refusal::Accept();
}

Refusal Construction::ResolveMitre(int Vertex, int IA, int IB, int Third) noexcept
{
    BlendEdge& A = Blends[IA]; BlendEdge& B = Blends[IB];
    int EndA = EndAt(A.Edge, Vertex), EndB = EndAt(B.Edge, Vertex);
    int Shared = -1;
    for (int K = 0; K < 2; ++K) if (B.Side(A.Face[K]) >= 0) Shared = A.Face[K];
    int SideA = A.Side(Shared), SideB = B.Side(Shared);
    double TauA = A.Tau(EndA == 0 ? A.T0 : A.T1), TauB = B.Tau(EndB == 0 ? B.T0 : B.T1);
    if (!RailMeetRail(A, SideA, B, SideB, TauA, TauB)) return Refusal::Reject(RefusalReason::NoConvergence, Explain("rails of edges " + std::to_string(A.Edge) + " and " + std::to_string(B.Edge) + " do not cross on their shared face"));
    // far rails against the sharp third edge
    const NurbsCurve& Sharp = Body.Edges[Third].Curve;
    auto RailOnEdge = [&](const BlendEdge& S, int Side, double& Tau, Vec3& Q) -> bool {
        double T = Sharp.ClosestParameter(S.StripProbe.Point(Side, Tau));
        double X[2] = { Tau, T };
        double R = SolveNonlinear(3, 2, [&](const double* Y, double* F) {
            Vec3 D = S.StripProbe.Point(Side, Y[0]) - Sharp.Sample(ScalarCriteria::Clamp(Y[1], Sharp.DomainStart(), Sharp.DomainEnd()));
            F[0] = D.X; F[1] = D.Y; F[2] = D.Z; }, X);
        Tau = X[0]; Q = S.StripProbe.Point(Side, Tau);
        return R <= 1e-8;
    };
    double TauFarA = A.Tau(EndA == 0 ? A.T0 : A.T1), TauFarB = B.Tau(EndB == 0 ? B.T0 : B.T1);
    Vec3 QA, QB;
    if (!RailOnEdge(A, 1 - SideA, TauFarA, QA) || !RailOnEdge(B, 1 - SideB, TauFarB, QB))
        return Refusal::Reject(RefusalReason::NoConvergence, Explain("mitre at vertex " + std::to_string(Vertex) + ": a rail does not reach the sharp edge " + std::to_string(Third)));
    if (QA.Distance(QB) > 1e-6)
        return Refusal::Reject(RefusalReason::Unsupported, Explain("mitre at vertex " + std::to_string(Vertex) + " needs a corner patch (rails meet edge " + std::to_string(Third) + " " + std::to_string(QA.Distance(QB)) + " m apart)"));
    bool Ok = false;
    int Id = MitrePiece(A, SideA, TauA, 1.0 - SideA, TauFarA, B, SideB, TauB, 1.0 - SideB, TauFarB, Ok);
    if (!Ok) return Refusal::Reject(RefusalReason::NoConvergence, Explain("mitre curve between edges " + std::to_string(A.Edge) + " and " + std::to_string(B.Edge) + " could not be traced"));
    // the piece runs from the shared face (s = SideA) to the sharp edge; boundaries run s = 0 → 1
    AttachEnd(A, EndA, { Id }, { SideA == 1 }, SideA == 0 ? TauA : TauFarA, SideA == 0 ? TauFarA : TauA);
    AttachEnd(B, EndB, { Id }, { SideB == 1 }, SideB == 0 ? TauB : TauFarB, SideB == 0 ? TauFarB : TauB);
    Trims[{ Third, EndAt(Third, Vertex) }] = 0.5 * (QA + QB);
    return Refusal::Accept();
}

// Ball touching three faces: fixes the end sections of the three strips before they are walked.
Refusal Construction::PrepareSphere(int Vertex, const int Index[3], int Faces[3]) noexcept
{
    // faces: each strip contributes two; the three faces are the union
    std::vector<int> F;
    for (int K = 0; K < 3; ++K) for (int J = 0; J < 2; ++J) if (std::find(F.begin(), F.end(), Blends[Index[K]].Face[J]) == F.end()) F.push_back(Blends[Index[K]].Face[J]);
    if (F.size() != 3) return Refusal::Reject(RefusalReason::Unsupported, Explain("corner at vertex " + std::to_string(Vertex) + " is not a three-face corner"));
    SurfaceProbe Probe[3];
    Vec3 VP = Body.Vertices[Vertex].Point;
    double X[6];
    for (int K = 0; K < 3; ++K)
    {
        Faces[K] = F[K];
        Probe[K] = SurfaceProbe(Body.Faces[F[K]].Surface, Body.Faces[F[K]].Reversed);
        Vec3 Into{};
        for (int J = 0; J < 3; ++J)
        {
            const BlendEdge& B = Blends[Index[J]];
            int Side = B.Side(F[K]);
            if (Side < 0) continue;
            double T = EndAt(B.Edge, Vertex) == 0 ? B.T0 : B.T1;
            Into += InFaceDirection(B.Coedge[Side], VP, T, *B.Curve);
        }
        double U = 0, V = 0;
        std::vector<Vec2> Seeds;
        for (int C = 0; C < static_cast<int>(Body.Coedges.size()); ++C) if (Body.Coedges[C].Face == F[K]) { auto Tr = Body.CoedgeTrace(C); Seeds.insert(Seeds.end(), Tr.begin(), Tr.end()); }
        Probe[K].Nearest(VP, Seeds, U, V);
        Probe[K].Project(VP + Into * Size, U, V);
        X[2 * K] = U; X[2 * K + 1] = V;
    }
    const double Sigma = Blends[Index[0]].Sigma;
    auto Centre = [&](int K, const double* Y) { return Probe[K].Point(Y[2 * K], Y[2 * K + 1]) - Probe[K].Normal(Y[2 * K], Y[2 * K + 1]) * (Sigma * Size); };
    double R = SolveNonlinear(6, 6, [&](const double* Y, double* Out) {
        Vec3 A = Centre(0, Y) - Centre(1, Y), B = Centre(0, Y) - Centre(2, Y);
        Out[0] = A.X; Out[1] = A.Y; Out[2] = A.Z; Out[3] = B.X; Out[4] = B.Y; Out[5] = B.Z; }, X);
    if (R > AcceptTolerance) return Refusal::Reject(RefusalReason::NoConvergence, Explain("no ball touches all three faces at vertex " + std::to_string(Vertex)));
    Vec3 C0 = (Centre(0, X) + Centre(1, X) + Centre(2, X)) / 3.0;
    Corner Patch; Patch.Centre = C0;
    // each strip ends at the spine parameter whose section plane contains the corner ball centre
    for (int J = 0; J < 3; ++J)
    {
        BlendEdge& B = Blends[Index[J]];
        int End = EndAt(B.Edge, Vertex);
        double T = End == 0 ? B.T0 : B.T1;
        double Y[1] = { T };
        double G = SolveNonlinear(1, 1, [&](const double* Z, double* Out) { Vec3 E, Tn; SpineAt(B, Z[0], E, Tn); Out[0] = (C0 - E).Dot(Tn); }, Y);
        if (G > AcceptTolerance) return Refusal::Reject(RefusalReason::NoConvergence, Explain("corner section on edge " + std::to_string(B.Edge) + " not found"));
        B.Ends[End].Range = EndRange::Fixed;
        B.Ends[End].FixedT = Y[0];
        B.Ends[End].HasFixedX = true;
        for (int Side = 0; Side < 2; ++Side)
        {
            int K = static_cast<int>(std::find(F.begin(), F.end(), B.Face[Side]) - F.begin());
            B.Ends[End].FixedX[2 * Side] = X[2 * K]; B.Ends[End].FixedX[2 * Side + 1] = X[2 * K + 1];
        }
        if ((End == 0 && Y[0] > B.T1) || (End == 1 && Y[0] < B.T0)) return Refusal::Reject(RefusalReason::Unsupported, Explain("corner ball at vertex " + std::to_string(Vertex) + " overruns edge " + std::to_string(B.Edge)));
    }
    // sphere with its seam and poles away from the patch
    std::vector<Vec3> Directions;
    for (int K = 0; K < 3; ++K) Directions.push_back((Probe[K].Point(X[2 * K], X[2 * K + 1]) - C0).Normalised());
    Vec3 Mid = (Directions[0] + Directions[1] + Directions[2]).Normalised();
    Vec3 E1 = Mid.AnyPerpendicular(), E2 = Mid.Cross(E1).Normalised();
    Vec3 Axis = E1; double Best = ScalarCriteria::Infinity;
    for (int I = 0; I < 36; ++I)
    {
        double Angle = ScalarCriteria::Pi * I / 36.0;
        Vec3 A = E1 * std::cos(Angle) + E2 * std::sin(Angle);
        double Worst = 0; for (const Vec3& D : Directions) Worst = std::max(Worst, std::fabs(D.Dot(A)));
        if (Worst < Best) { Best = Worst; Axis = A; }
    }
    Vec3 Seam = Mid * -1.0;
    const double H = std::sqrt(0.5);
    std::vector<Vec4> ProfilePoles = {
        Vec4(C0 - Axis * Size, 1.0), Vec4((C0 + (Seam - Axis) * Size) * H, H), Vec4(C0 + Seam * Size, 1.0),
        Vec4((C0 + (Seam + Axis) * Size) * H, H), Vec4(C0 + Axis * Size, 1.0) };
    Deliver<NurbsCurve> Profile = NurbsCurve::Build(2, ProfilePoles, { 0, 0, 0, 0.5, 0.5, 1, 1, 1 });
    if (!Profile) return Refusal::Reject(Profile.Denial.Reason, Profile.Denial.Detail);
    Deliver<NurbsSurface> Sphere = NurbsSurface::Revolution(Profile.Payload, C0, Axis, ScalarCriteria::TwoPi);
    if (!Sphere) return Refusal::Reject(Sphere.Denial.Reason, Sphere.Denial.Detail);
    Patch.Surface = std::move(Sphere.Payload);
    Patch.Surface.Classification = SurfaceClassification::Sphere;
    Patch.Surface.Origin = C0; Patch.Surface.Axis = Axis; Patch.Surface.RadiusMajor = Patch.Surface.RadiusMinor = Size;
    double U = 0, V = 0; Patch.Surface.ClosestParameter(C0 + Mid * Size, U, V);
    Vec3 P, DU, DV; Patch.Surface.Derivatives(U, V, P, DU, DV);
    Patch.Reversed = DU.Cross(DV).Dot((P - C0) * Sigma) < 0.0;
    Patch.Probe = SurfaceProbe(Patch.Surface, Patch.Reversed);
    Corners.push_back(std::move(Patch));
    return Refusal::Accept();
}

Refusal Construction::ResolveSphere(int Vertex, const int Index[3]) noexcept
{
    Corner* Patch = nullptr;
    Vec3 VP = Body.Vertices[Vertex].Point;
    for (Corner& C : Corners) if (C.Pieces.empty() && (Patch == nullptr || C.Centre.Distance(VP) < Patch->Centre.Distance(VP))) Patch = &C;
    if (!Patch) return Refusal::Reject(RefusalReason::Unsupported, Explain("corner patch missing"));
    for (int J = 0; J < 3; ++J)
    {
        BlendEdge& B = Blends[Index[J]];
        int End = EndAt(B.Edge, Vertex);
        int Id = SectionPiece(B, End);
        Pieces[Id].FaceTraces[Patch->FaceKey] = ProjectTrace(Patch->Probe, Pieces[Id].Curve, {}, TraceSamples);
        B.Ends[End].Section = true;
        double Tau = End == 0 ? 0.0 : 1.0;
        AttachEnd(B, End, { Id }, { false }, Tau, Tau);
        Patch->Pieces.push_back(Id);
    }
    ++Report.Corners;
    return Refusal::Accept();
}

Refusal Construction::ResolveTripleMitre(int Vertex, const int Index[3]) noexcept
{
    BlendEdge* S[3] = { &Blends[Index[0]], &Blends[Index[1]], &Blends[Index[2]] };
    double Seed[3];
    for (int K = 0; K < 3; ++K) Seed[K] = S[K]->Tau(EndAt(S[K]->Edge, Vertex) == 0 ? S[K]->T0 : S[K]->T1);
    // rail crossings on each shared face
    struct Crossing { int A, B, SideA, SideB; double TauA, TauB; Vec3 Point; };
    std::vector<Crossing> Cross;
    for (int I = 0; I < 3; ++I) for (int J = I + 1; J < 3; ++J)
    {
        int Shared = -1;
        for (int K = 0; K < 2; ++K) if (S[J]->Side(S[I]->Face[K]) >= 0) Shared = S[I]->Face[K];
        if (Shared < 0) return Refusal::Reject(RefusalReason::Unsupported, Explain("triple corner without shared faces"));
        Crossing C{ I, J, S[I]->Side(Shared), S[J]->Side(Shared), Seed[I], Seed[J], {} };
        if (!RailMeetRail(*S[I], C.SideA, *S[J], C.SideB, C.TauA, C.TauB)) return Refusal::Reject(RefusalReason::NoConvergence, Explain("rails do not cross at vertex " + std::to_string(Vertex)));
        C.Point = S[I]->StripProbe.Point(C.SideA, C.TauA);
        Cross.push_back(C);
    }
    // triple point on all three strips
    Vec3 Guess = (Cross[0].Point + Cross[1].Point + Cross[2].Point) / 3.0;
    double X[6];
    for (int K = 0; K < 3; ++K) { double U = 0.5, V = Seed[K]; S[K]->StripProbe.Project(Guess, U, V); X[2 * K] = U; X[2 * K + 1] = V; }
    double R = SolveNonlinear(6, 6, [&](const double* Y, double* F) {
        Vec3 A = S[0]->StripProbe.Point(Y[0], Y[1]) - S[1]->StripProbe.Point(Y[2], Y[3]);
        Vec3 B = S[0]->StripProbe.Point(Y[0], Y[1]) - S[2]->StripProbe.Point(Y[4], Y[5]);
        F[0] = A.X; F[1] = A.Y; F[2] = A.Z; F[3] = B.X; F[4] = B.Y; F[5] = B.Z; }, X);
    if (R > 1e-8) return Refusal::Reject(RefusalReason::NoConvergence, Explain("three blends at vertex " + std::to_string(Vertex) + " have no common point"));
    for (int K = 0; K < 3; ++K) if (X[2 * K] < -1e-9 || X[2 * K] > 1 + 1e-9) return Refusal::Reject(RefusalReason::Unsupported, Explain("triple point at vertex " + std::to_string(Vertex) + " lies outside a strip"));
    // mitre from each crossing to the triple point
    std::vector<int> Ids;
    for (const Crossing& C : Cross)
    {
        bool Ok = false;
        int Id = MitrePiece(*S[C.A], C.SideA, C.TauA, X[2 * C.A], X[2 * C.A + 1], *S[C.B], C.SideB, C.TauB, X[2 * C.B], X[2 * C.B + 1], Ok);
        if (!Ok) return Refusal::Reject(RefusalReason::NoConvergence, Explain("mitre to the triple point at vertex " + std::to_string(Vertex) + " failed"));
        Ids.push_back(Id);
    }
    // each strip's end boundary: crossing on its s=0 face → triple point → crossing on its s=1 face
    for (int K = 0; K < 3; ++K)
    {
        std::vector<int> List(2); std::vector<bool> Rev(2); double Tau[2] = { 0, 0 };
        for (size_t C = 0; C < Cross.size(); ++C)
        {
            int Side = Cross[C].A == K ? Cross[C].SideA : (Cross[C].B == K ? Cross[C].SideB : -1);
            if (Side < 0) continue;
            double TauHere = Cross[C].A == K ? Cross[C].TauA : Cross[C].TauB;
            // piece runs crossing → triple point
            List[Side] = Ids[C]; Rev[Side] = Side == 1; Tau[Side] = TauHere;
        }
        AttachEnd(*S[K], EndAt(S[K]->Edge, Vertex), List, Rev, Tau[0], Tau[1]);
    }
    return Refusal::Accept();
}

Refusal Construction::ClassifyVertices(std::vector<std::function<Refusal()>>& Later) noexcept
{
    std::set<int> Vertices;
    for (const BlendEdge& B : Blends) { Vertices.insert(Body.Edges[B.Edge].VertexStart); Vertices.insert(Body.Edges[B.Edge].VertexEnd); }
    for (int Vertex : Vertices)
    {
        std::vector<int> Incident = IncidentEdges(Vertex);
        std::vector<int> Blended;
        bool Rim = false;
        for (int E : Incident) if (BlendOfEdge.count(E)) { Blended.push_back(E); Rim = Rim || Body.Edges[E].Closed(); }
        if (Rim)
        {
            for (int E : Blended) { BlendEdge& B = Blends[BlendOfEdge[E]]; B.Ends[0].Range = B.Ends[1].Range = EndRange::Fixed; B.Ends[0].FixedT = B.T0; B.Ends[1].FixedT = B.T1; }
            Later.push_back([this, Vertex, Incident] { return ResolveRim(Vertex, Incident); });
            continue;
        }
        if (Incident.size() != 3)
            return Refusal::Reject(RefusalReason::Unsupported, Explain("vertex " + std::to_string(Vertex) + " has " + std::to_string(Incident.size()) + " edges; only three-edge corners are supported"));
        std::set<int> Faces;
        for (int E : Incident) for (int C : Body.Edges[E].Coedges) Faces.insert(Body.Coedges[C].Face);
        if (Faces.size() != 3) return Refusal::Reject(RefusalReason::Unsupported, Explain("vertex " + std::to_string(Vertex) + " is not a simple three-face corner"));
        if (Blended.size() == 1)
        {
            int Index = BlendOfEdge[Blended[0]];
            BlendEdge& B = Blends[Index];
            int End = EndAt(B.Edge, Vertex);
            B.Ends[End].Range = EndRange::Extended;
            int EndFace = -1;
            for (int F : Faces) if (B.Side(F) < 0) EndFace = F;
            Later.push_back([this, Vertex, Index, EndFace, Incident] { return ResolveEndOnFace(Vertex, Index, EndFace, Incident); });
        }
        else if (Blended.size() == 2)
        {
            int IA = BlendOfEdge[Blended[0]], IB = BlendOfEdge[Blended[1]];
            int Third = -1;
            for (int E : Incident) if (!BlendOfEdge.count(E)) Third = E;
            BlendEdge& A = Blends[IA]; BlendEdge& B = Blends[IB];
            int EndA = EndAt(A.Edge, Vertex), EndB = EndAt(B.Edge, Vertex);
            double TA = EndA == 0 ? A.T0 : A.T1, TB = EndB == 0 ? B.T0 : B.T1;
            bool Tangent = std::fabs(A.Curve->Tangent(TA).Dot(B.Curve->Tangent(TB))) > 1.0 - TangentTolerance;
            // G1 across the third edge: equal outward normals of its two faces at the vertex
            const auto& Cs = Body.Edges[Third].Coedges;
            Vec3 N[2];
            for (int K = 0; K < 2; ++K)
            {
                const BrepCoedge& C = Body.Coedges[Cs[K]];
                SurfaceProbe P(Body.Faces[C.Face].Surface, Body.Faces[C.Face].Reversed);
                double U = 0, V = 0; P.Nearest(Body.Vertices[Vertex].Point, Body.CoedgeTrace(Cs[K]), U, V); P.Project(Body.Vertices[Vertex].Point, U, V);
                N[K] = P.Normal(U, V);
            }
            bool Smooth = N[0].Dot(N[1]) > 1.0 - TangentTolerance;
            if (Tangent && Smooth)
            {
                A.Ends[EndA].Range = B.Ends[EndB].Range = EndRange::Fixed;
                A.Ends[EndA].FixedT = TA; B.Ends[EndB].FixedT = TB;
                Later.push_back([this, Vertex, IA, IB, Third] { return ResolveContinuation(Vertex, IA, IB, Third); });
            }
            else
            {
                A.Ends[EndA].Range = B.Ends[EndB].Range = EndRange::Extended;
                Later.push_back([this, Vertex, IA, IB, Third] { return ResolveMitre(Vertex, IA, IB, Third); });
            }
        }
        else
        {
            int Index[3] = { BlendOfEdge[Blended[0]], BlendOfEdge[Blended[1]], BlendOfEdge[Blended[2]] };
            bool SameSense = Blends[Index[0]].Sigma == Blends[Index[1]].Sigma && Blends[Index[1]].Sigma == Blends[Index[2]].Sigma;
            if (Profile == BlendProfile::Fillet)
            {
                if (!SameSense) return Refusal::Reject(RefusalReason::Unsupported, Explain("vertex " + std::to_string(Vertex) + " mixes convex and concave fillets (needs a setback corner)"));
                int Faces3[3];
                Refusal R = PrepareSphere(Vertex, Index, Faces3);
                if (R) return R;
                Later.push_back([this, Vertex, Index0 = Index[0], Index1 = Index[1], Index2 = Index[2]] { int I[3] = { Index0, Index1, Index2 }; return ResolveSphere(Vertex, I); });
            }
            else
            {
                for (int K = 0; K < 3; ++K) { BlendEdge& B = Blends[Index[K]]; B.Ends[EndAt(B.Edge, Vertex)].Range = EndRange::Extended; }
                Later.push_back([this, Vertex, Index0 = Index[0], Index1 = Index[1], Index2 = Index[2]] { int I[3] = { Index0, Index1, Index2 }; return ResolveTripleMitre(Vertex, I); });
            }
        }
    }
    return Refusal::Accept();
}

//------------------------------------------------------------------------------------------------ tangent propagation

// G1 across an edge at one of its vertices: the outward normals of its two faces agree there.
bool Construction::EdgeSmoothAt(int Edge, int Vertex) const noexcept
{
    const auto& Cs = Body.Edges[Edge].Coedges;
    if (Cs.size() != 2 || Body.Coedges[Cs[0]].Face == Body.Coedges[Cs[1]].Face) return false;   // seams are not chain links
    Vec3 N[2];
    for (int K = 0; K < 2; ++K)
    {
        const BrepCoedge& C = Body.Coedges[Cs[K]];
        SurfaceProbe P(Body.Faces[C.Face].Surface, Body.Faces[C.Face].Reversed);
        double U = 0, V = 0; P.Nearest(Body.Vertices[Vertex].Point, Body.CoedgeTrace(Cs[K]), U, V); P.Project(Body.Vertices[Vertex].Point, U, V);
        N[K] = P.Normal(U, V);
    }
    return N[0].Dot(N[1]) > 1.0 - TangentTolerance;
}

Vec3 Construction::EdgeTangentAt(int Edge, int Vertex) const noexcept
{
    const BrepEdge& E = Body.Edges[Edge];
    return E.Curve.Tangent(E.VertexStart == Vertex ? E.Curve.DomainStart() : E.Curve.DomainEnd());
}

// A blend follows its edge through tangent-continuous corners (the CAD default "tangent propagation"). At a three-edge
//    vertex where one unselected edge is smooth and the other is sharp and tangent to the selected edge, the sharp edge is
//    the same feature line continued across a smooth face junction (typically onto an earlier blend): it joins the
//    selection, and the vertex then resolves as an ordinary continuation with a shared section. Without this the end
//    face is tangent to a support face and the end intersection is singular.
void Construction::PropagateTangentChains(std::set<int>& Selected) const noexcept
{
    std::vector<int> Open(Selected.begin(), Selected.end());
    while (!Open.empty())
    {
        const int Edge = Open.back(); Open.pop_back();
        const BrepEdge& E = Body.Edges[Edge];
        if (E.Closed()) continue;
        for (int Vertex : { E.VertexStart, E.VertexEnd })
        {
            std::vector<int> Incident = IncidentEdges(Vertex);
            if (Incident.size() != 3) continue;
            int Smooth = -1, Candidate = -1, Others = 0;
            for (int Other : Incident)
            {
                if (Other == Edge) continue;
                ++Others;
                if (EdgeSmoothAt(Other, Vertex)) Smooth = Other; else Candidate = Other;
            }
            if (Others != 2 || Smooth < 0 || Candidate < 0 || Selected.count(Candidate)) continue;
            if (Body.Edges[Candidate].Coedges.size() != 2) continue;
            if (std::fabs(EdgeTangentAt(Edge, Vertex).Dot(EdgeTangentAt(Candidate, Vertex))) < 1.0 - TangentTolerance) continue;
            Selected.insert(Candidate);
            Open.push_back(Candidate);
        }
    }
}

//------------------------------------------------------------------------------------------------ assembly

int Construction::RailPieceOf(BlendEdge& B, int Side) noexcept
{
    if (B.RailPiece[Side] >= 0) return B.RailPiece[Side];
    double Tau0 = B.Ends[0].TauRail[Side], Tau1 = B.Ends[1].TauRail[Side];
    Piece P;
    NurbsCurve Rail = B.Strip.IsoCurveU(static_cast<double>(Side));
    P.Curve = Rail.Trimmed(Tau0, Tau1);
    const int Samples = std::clamp(static_cast<int>(B.Stations.size()) * 2, 24, 256);
    std::vector<Vec2> OnStrip, Seeds;
    double A = P.Curve.DomainStart(), C = P.Curve.DomainEnd();
    for (int K = 0; K <= Samples; ++K)
    {
        double Tau = A + (C - A) * K / Samples;
        OnStrip.emplace_back(static_cast<double>(Side), Tau);
        // seed from the stations bracketing τ
        double T = B.SpineT(Tau);
        size_t I = 0; while (I + 2 < B.Stations.size() && B.Stations[I + 1].T < T) ++I;
        const Station& S0 = B.Stations[I]; const Station& S1 = B.Stations[std::min(I + 1, B.Stations.size() - 1)];
        double W = S1.T > S0.T ? ScalarCriteria::Clamp((T - S0.T) / (S1.T - S0.T), 0.0, 1.0) : 0.0;
        Seeds.emplace_back(S0.X[2 * Side] + W * (S1.X[2 * Side] - S0.X[2 * Side]), S0.X[2 * Side + 1] + W * (S1.X[2 * Side + 1] - S0.X[2 * Side + 1]));
    }
    P.FaceTraces[B.FaceKey] = OnStrip;
    P.FaceTraces[B.Face[Side]] = ProjectTrace(B.Probe[Side], P.Curve, Seeds, Samples);
    Pieces.push_back(std::move(P));
    return B.RailPiece[Side] = static_cast<int>(Pieces.size() - 1);
}

Refusal Construction::PlainPieceOf(int Edge, int& Out) noexcept
{
    auto Found = PlainPieces.find(Edge);
    if (Found != PlainPieces.end()) { Out = Found->second; return Refusal::Accept(); }
    const BrepEdge& E = Body.Edges[Edge];
    Piece P;
    auto TrimStart = Trims.find({ Edge, 0 }), TrimEnd = Trims.find({ Edge, 1 });
    bool Trimmed = TrimStart != Trims.end() || TrimEnd != Trims.end();
    if (!Trimmed) P.Curve = E.Curve;
    else
    {
        double A = E.Curve.DomainStart(), B = E.Curve.DomainEnd();
        if (TrimStart != Trims.end()) A = E.Curve.ClosestParameter(TrimStart->second);
        if (TrimEnd != Trims.end()) B = E.Curve.ClosestParameter(TrimEnd->second);
        if (E.Closed() && TrimStart != Trims.end() && TrimEnd == Trims.end()) B = E.Curve.DomainEnd();
        if (B - A <= 1e-9 * (E.Curve.DomainEnd() - E.Curve.DomainStart()))
            return Refusal::Reject(RefusalReason::Unsupported, Explain("edge " + std::to_string(Edge) + " is consumed by the blend (size too large)"));
        P.Curve = E.Curve.Trimmed(A, B);
        // Coincidence snap: the clamped end pole takes the rail end exactly so the trimmed edge and the rail share one vertex.
        //    Gated at JoinTolerance: a larger gap is an upstream geometry defect (e.g. an inexact edge curve) that the
        //    GeometricAudit must see, not something to drag into place.
        auto Pin = [](Vec4& Pole, Vec3 Point) { Pole = Vec4(Point.X * Pole.W, Point.Y * Pole.W, Point.Z * Pole.W, Pole.W); };
        if (TrimStart != Trims.end() && P.Curve.StartPoint().Distance(TrimStart->second) <= JoinTolerance) Pin(P.Curve.Poles.front(), TrimStart->second);
        if (TrimEnd != Trims.end() && P.Curve.EndPoint().Distance(TrimEnd->second) <= JoinTolerance) Pin(P.Curve.Poles.back(), TrimEnd->second);
    }
    for (int C : E.Coedges)
    {
        const BrepCoedge& Co = Body.Coedges[C];
        std::vector<Vec2> Original = Body.CoedgeTrace(C);
        if (Co.Reversed) std::reverse(Original.begin(), Original.end());
        if (!Trimmed) { P.CoedgeTraces[C] = Original; continue; }
        SurfaceProbe Probe(Body.Faces[Co.Face].Surface, Body.Faces[Co.Face].Reversed);
        // seeds stay on the coedge's own side of a seam
        std::vector<Vec2> Trace;
        double U = 0, V = 0;
        const int Samples = std::max<int>(24, static_cast<int>(Original.size()));
        double A = P.Curve.DomainStart(), B = P.Curve.DomainEnd();
        for (int K = 0; K <= Samples; ++K)
        {
            Vec3 Q = P.Curve.Sample(A + (B - A) * K / Samples);
            if (K == 0) Probe.Nearest(Q, Original, U, V);
            Probe.Project(Q, U, V);
            Trace.emplace_back(U, V);
        }
        // keep the seam side of the original trace
        if (Probe.WrapU && !Original.empty())
        {
            double P0 = Probe.U1 - Probe.U0, Ref = Original[Original.size() / 2].X;
            for (Vec2& T : Trace) { while (T.X - Ref > 0.5 * P0) T.X -= P0; while (Ref - T.X > 0.5 * P0) T.X += P0; T.X = ScalarCriteria::Clamp(T.X, Probe.U0, Probe.U1); }
        }
        if (Probe.WrapV && !Original.empty())
        {
            double P0 = Probe.V1 - Probe.V0, Ref = Original[Original.size() / 2].Y;
            for (Vec2& T : Trace) { while (T.Y - Ref > 0.5 * P0) T.Y -= P0; while (Ref - T.Y > 0.5 * P0) T.Y += P0; T.Y = ScalarCriteria::Clamp(T.Y, Probe.V0, Probe.V1); }
        }
        P.CoedgeTraces[C] = Trace;
    }
    Pieces.push_back(std::move(P));
    Out = PlainPieces[Edge] = static_cast<int>(Pieces.size() - 1);
    return Refusal::Accept();
}

Refusal Construction::Assemble(BrepBody& R) noexcept
{
    struct Use { int Piece; bool Reversed; int Coedge; };                               // Coedge ≥ 0 → trace keyed by the original coedge
    auto StartOf = [&](const Use& U) { const NurbsCurve& C = Pieces[U.Piece].Curve; return U.Reversed ? C.EndPoint() : C.StartPoint(); };
    auto EndOf = [&](const Use& U) { const NurbsCurve& C = Pieces[U.Piece].Curve; return U.Reversed ? C.StartPoint() : C.EndPoint(); };
    auto TraceOf = [&](const Use& U, int FaceKey) {
        const Piece& P = Pieces[U.Piece];
        std::vector<Vec2> T;
        if (U.Coedge >= 0) { auto It = P.CoedgeTraces.find(U.Coedge); if (It != P.CoedgeTraces.end()) T = It->second; }
        else { auto It = P.FaceTraces.find(FaceKey); if (It != P.FaceTraces.end()) T = It->second; }
        if (U.Reversed) std::reverse(T.begin(), T.end());
        return T;
    };
    auto Emit = [&](int Face, bool Outer, const std::vector<Use>& Loop) {
        int L = R.AddLoop(Face, Outer);
        for (const Use& U : Loop)
        {
            const Piece& P = Pieces[U.Piece];
            int E = R.AddEdge(P.Curve, JoinTolerance);
            const NurbsCurve& Stored = R.Edges[E].Curve;
            double Mid = 0.5 * (P.Curve.DomainStart() + P.Curve.DomainEnd());
            double Ts = Stored.ClosestParameter(P.Curve.Sample(Mid));
            bool Same = Stored.Tangent(Ts).Dot(P.Curve.Tangent(Mid)) > 0.0;
            int C = R.AddCoedge(E, U.Reversed ? Same : !Same, Face, L);
            R.Coedges[C].Trace = TraceOf(U, Face);
        }
    };
    auto Oriented = [&](std::vector<Use>& Loop, int FaceKey, bool Reversed) {
        std::vector<Vec2> Ring;
        for (const Use& U : Loop) { auto T = TraceOf(U, FaceKey); if (!T.empty()) Ring.insert(Ring.end(), T.begin(), T.end() - 1); }
        bool Ccw = SignedArea(Ring) > 0.0;
        if (Ccw == Reversed)
        {
            std::reverse(Loop.begin(), Loop.end());
            for (Use& U : Loop) U.Reversed = !U.Reversed;
        }
    };

    const int OriginalFaces = static_cast<int>(Body.Faces.size());
    for (int F = 0; F < OriginalFaces; ++F)
    {
        const BrepFace& Src = Body.Faces[F];
        int Face = R.AddFace(Src.Surface);
        R.Faces[Face].Reversed = Src.Reversed;
        bool Rebuilt = Touched.count(F) > 0;
        R.Faces[Face].Natural = Src.Natural && !Rebuilt;
        for (int LoopIndex : Src.Loops)
        {
            const BrepLoop& L = Body.Loops[LoopIndex];
            std::vector<Use> Loop;
            for (int C : L.Coedges)
            {
                const BrepCoedge& Co = Body.Coedges[C];
                const BrepEdge& E = Body.Edges[Co.Edge];
                int Vs = Co.Reversed ? E.VertexEnd : E.VertexStart, Ve = Co.Reversed ? E.VertexStart : E.VertexEnd;
                (void)Vs;
                auto Blend = BlendOfEdge.find(Co.Edge);
                if (Blend != BlendOfEdge.end())
                {
                    BlendEdge& B = Blends[Blend->second];
                    Loop.push_back({ RailPieceOf(B, B.Side(F)), Co.Reversed, -1 });
                }
                else
                {
                    int Id = -1;
                    Refusal Rf = PlainPieceOf(Co.Edge, Id);
                    if (Rf) return Rf;
                    if (!Rebuilt) { Loop.push_back({ Id, Co.Reversed, C }); continue; }
                    Loop.push_back({ Id, Co.Reversed, C });
                }
                auto Conn = Connectors.find({ F, Ve });
                if (Conn != Connectors.end() && !E.Closed())
                    for (int Id : Conn->second)
                    {
                        Use U{ Id, false, -1 };
                        if (EndOf(Loop.back()).Distance(StartOf(U)) > EndOf(Loop.back()).Distance(EndOf(U))) U.Reversed = true;
                        Loop.push_back(U);
                    }
            }
            for (size_t K = 0; K < Loop.size(); ++K)
                if (EndOf(Loop[K]).Distance(StartOf(Loop[(K + 1) % Loop.size()])) > 10.0 * JoinTolerance)
                    return Refusal::Reject(RefusalReason::OpenWire, Explain("rebuilt loop of face " + std::to_string(F) + " does not close (gap " + std::to_string(EndOf(Loop[K]).Distance(StartOf(Loop[(K + 1) % Loop.size()]))) + " m)"));
            Emit(Face, L.Outer, Loop);
        }
    }
    // Blend carriers are built larger than their faces: strips are fitted past the edge ends so mitres / end curves can be
    //    intersected, sphere corners are full revolutions. Cut each carrier back to the (u,v) box of its boundary traces
    //    (no knot remap, so every trace stays valid). An oversized carrier inflates pole-hull bounds, view fitting, picking
    //    and boolean broad phases although the trimmed face itself is exact. A patch straddling a periodic seam has a
    //    full-period trace box and is left whole.
    auto TrimCarrier = [&](NurbsSurface S, const std::vector<int>& Used, int FaceKey)
    {
        double Low[2] = { ScalarCriteria::Infinity, ScalarCriteria::Infinity }, High[2] = { -ScalarCriteria::Infinity, -ScalarCriteria::Infinity };
        for (int Id : Used)
        {
            auto Trace = Pieces[Id].FaceTraces.find(FaceKey);
            if (Trace == Pieces[Id].FaceTraces.end()) return S;
            for (const Vec2& Q : Trace->second)
            {
                Low[0] = std::min(Low[0], Q.X); High[0] = std::max(High[0], Q.X);
                Low[1] = std::min(Low[1], Q.Y); High[1] = std::max(High[1], Q.Y);
            }
        }
        for (int Axis = 0; Axis < 2; ++Axis)
        {
            const double Start = Axis ? S.DomainStartV() : S.DomainStartU(), End = Axis ? S.DomainEndV() : S.DomainEndU();
            const double Margin = 1e-7 * (End - Start);
            const double A = std::max(Start, Low[Axis] - Margin), B = std::min(End, High[Axis] + Margin);
            if (!(B - A > 1e3 * Margin)) continue;
            if (B < End - Margin) S = Axis ? S.SplitV(B).first : S.SplitU(B).first;
            if (A > Start + Margin) S = Axis ? S.SplitV(A).second : S.SplitU(A).second;
        }
        return S;
    };
    for (BlendEdge& B : Blends)
    {
        std::vector<Use> Loop;
        for (size_t K = 0; K < B.Ends[0].Pieces.size(); ++K) Loop.push_back({ B.Ends[0].Pieces[K], static_cast<bool>(B.Ends[0].PieceReversed[K]), -1 });
        Loop.push_back({ RailPieceOf(B, 1), false, -1 });
        for (size_t K = B.Ends[1].Pieces.size(); K-- > 0;) Loop.push_back({ B.Ends[1].Pieces[K], !B.Ends[1].PieceReversed[K], -1 });
        Loop.push_back({ RailPieceOf(B, 0), true, -1 });
        std::vector<int> Used; for (const Use& U : Loop) Used.push_back(U.Piece);
        NurbsSurface Carrier = TrimCarrier(B.Strip, Used, B.FaceKey);
        int Face = R.AddFace(Carrier);
        if (Face != B.FaceKey) return Refusal::Reject(RefusalReason::Unsupported, Explain("face numbering"));
        R.Faces[Face].Reversed = B.StripReversed;
        R.Faces[Face].Natural = B.Ends[0].Section && B.Ends[1].Section;
        for (size_t K = 0; K < Loop.size(); ++K)
            if (EndOf(Loop[K]).Distance(StartOf(Loop[(K + 1) % Loop.size()])) > 10.0 * JoinTolerance)
                return Refusal::Reject(RefusalReason::OpenWire, Explain("strip loop of edge " + std::to_string(B.Edge) + " does not close"));
        Oriented(Loop, Face, B.StripReversed);
        Emit(Face, true, Loop);
        ++Report.Strips;
    }
    for (Corner& C : Corners)
    {
        int Face = R.AddFace(TrimCarrier(C.Surface, C.Pieces, C.FaceKey));
        R.Faces[Face].Reversed = C.Reversed;
        R.Faces[Face].Natural = false;
        std::vector<Use> Loop;
        std::vector<int> Remaining = C.Pieces;
        Loop.push_back({ Remaining.front(), false, -1 }); Remaining.erase(Remaining.begin());
        while (!Remaining.empty())
        {
            Vec3 At = EndOf(Loop.back());
            auto Best = Remaining.begin(); bool Rev = false; double D = ScalarCriteria::Infinity;
            for (auto It = Remaining.begin(); It != Remaining.end(); ++It)
            {
                double A = Pieces[*It].Curve.StartPoint().Distance(At), B = Pieces[*It].Curve.EndPoint().Distance(At);
                if (A < D) { D = A; Best = It; Rev = false; }
                if (B < D) { D = B; Best = It; Rev = true; }
            }
            Loop.push_back({ *Best, Rev, -1 }); Remaining.erase(Best);
        }
        Oriented(Loop, Face, C.Reversed);
        Emit(Face, true, Loop);
    }
    return Refusal::Accept();
}

//------------------------------------------------------------------------------------------------ driver

Refusal Construction::Run(const std::vector<int>& Edges, BrepBody& Result) noexcept
{
    if (!(Size > ScalarCriteria::KernelTolerance)) return Refusal::Reject(RefusalReason::DegenerateInput, Explain("blend size must be positive"));
    std::set<int> Unique(Edges.begin(), Edges.end());
    if (Unique.empty()) return Refusal::Reject(RefusalReason::DegenerateInput, Explain("no edges selected"));
    PropagateTangentChains(Unique);
    const int OriginalFaces = static_cast<int>(Body.Faces.size());
    for (int E : Unique)
    {
        if (E < 0 || E >= static_cast<int>(Body.Edges.size())) return Refusal::Reject(RefusalReason::OutOfDomain, Explain("edge " + std::to_string(E) + " does not exist"));
        const BrepEdge& Ed = Body.Edges[E];
        if (Ed.Coedges.size() != 2) return Refusal::Reject(RefusalReason::NonManifold, Explain("edge " + std::to_string(E) + " is not a manifold interior edge"));
        BlendEdge B;
        B.Edge = E; B.Curve = &Ed.Curve; B.T0 = Ed.Curve.DomainStart(); B.T1 = Ed.Curve.DomainEnd(); B.Closed = Ed.Closed();
        B.BuildLengthTable(); B.Range(B.T0, B.T1);
        for (int K = 0; K < 2; ++K)
        {
            B.Coedge[K] = Ed.Coedges[K];
            B.Face[K] = Body.Coedges[Ed.Coedges[K]].Face;
            B.Probe[K] = SurfaceProbe(Body.Faces[B.Face[K]].Surface, Body.Faces[B.Face[K]].Reversed);
        }
        if (B.Face[0] == B.Face[1]) return Refusal::Reject(RefusalReason::Unsupported, Explain("edge " + std::to_string(E) + " is a surface seam; there is nothing to blend"));
        // convexity from the in-face direction of face 0 against the outward normal of face 1
        double Tm = 0.5 * (B.T0 + B.T1);
        Vec3 At = Ed.Curve.Sample(Tm);
        Vec3 Into0 = InFaceDirection(B.Coedge[0], At, Tm, Ed.Curve);
        double U = 0, V = 0; B.Probe[1].Nearest(At, Body.CoedgeTrace(B.Coedge[1]), U, V); B.Probe[1].Project(At, U, V);
        double Bend = Into0.Dot(B.Probe[1].Normal(U, V));
        if (std::fabs(Bend) < 1e-4) return Refusal::Reject(RefusalReason::Unsupported, Explain("edge " + std::to_string(E) + " is tangent-continuous; there is nothing to blend"));
        B.Sigma = Bend < 0.0 ? 1.0 : -1.0;
        B.FaceKey = OriginalFaces + static_cast<int>(Blends.size());
        BlendOfEdge[E] = static_cast<int>(Blends.size());
        Touched.insert(B.Face[0]); Touched.insert(B.Face[1]);
        Blends.push_back(std::move(B));
    }

    std::vector<std::function<Refusal()>> Later;
    if (Refusal R = ClassifyVertices(Later)) return R;
    // extended ends reach past the vertex by a few blend sizes
    for (BlendEdge& B : Blends)
    {
        B.TA = B.T0; B.TB = B.T1;
        if (B.Ends[0].Range == EndRange::Extended) B.TA = B.T0 - 3.0 * Size / std::max(Speed(B, B.T0), 1e-300);
        if (B.Ends[1].Range == EndRange::Extended) B.TB = B.T1 + 3.0 * Size / std::max(Speed(B, B.T1), 1e-300);
        if (Refusal R = Walk(B)) return R;
    }
    int Key = OriginalFaces + static_cast<int>(Blends.size());
    for (Corner& C : Corners) { C.FaceKey = Key++; C.Probe = SurfaceProbe(C.Surface, C.Reversed); }   // vector is final: bind probes now
    for (auto& Resolve : Later) if (Refusal R = Resolve()) return R;
    // edges trimmed at a vertex touch both of their faces
    for (const auto& [Key2, Point] : Trims) for (int C : Body.Edges[Key2.first].Coedges) Touched.insert(Body.Coedges[C].Face);
    for (const BlendEdge& B : Blends)
        for (int End = 0; End < 2; ++End)
            if (B.Ends[End].Pieces.empty()) return Refusal::Reject(RefusalReason::Unsupported, Explain("end " + std::to_string(End) + " of edge " + std::to_string(B.Edge) + " was not resolved"));
    for (const BlendEdge& B : Blends) Report.MaxDeviation = std::max(Report.MaxDeviation, B.Deviation);
    if (Refusal R = Assemble(Result)) return R;
    return Refusal::Accept();
}

}

Deliver<BrepBody> RollingBallBlend::Construct(const BrepBody& Body, const std::vector<int>& Edges, double Size, BlendProfile Profile, RollingBlendReport* ReportOut) noexcept
{
    RollingBlendReport Local;
    RollingBlendReport& Report = ReportOut ? *ReportOut : Local;
    Report = RollingBlendReport{};
    BodyReport Before = Body.Validate();
    Report.VolumeBefore = Before.Volume;
    if (!Before.Solid()) { Report.Diagnostic = "input is not a closed solid"; return Deliver<BrepBody>::Reject(RefusalReason::NonManifold, Explain(Report.Diagnostic)); }
    BrepBody Result;
    Construction Work(Body, Size, Profile, Report);
    Refusal R = Work.Run(Edges, Result);
    if (R) { Report.Diagnostic = R.Detail ? R.Detail : "refused"; return Deliver<BrepBody>::Reject(R.Reason, Explain(Report.Diagnostic)); }
    BodyReport After = Result.Validate();
    Report.VolumeAfter = After.Volume;
    if (!After.Solid())
    {
        Report.Diagnostic = "rebuilt body is not a valid solid (open " + std::to_string(After.OpenEdges) + ", non-manifold " + std::to_string(After.NonManifoldEdges) + ", misoriented " + std::to_string(After.MisorientedEdges) + ", volume " + std::to_string(After.Volume) + ")";
        return Deliver<BrepBody>::Reject(RefusalReason::NonManifold, Explain(Report.Diagnostic));
    }
    if (After.Genus != Before.Genus || After.Hulls != Before.Hulls)
    {
        Report.Diagnostic = "blend changed the topology (genus " + std::to_string(Before.Genus) + " → " + std::to_string(After.Genus) + ")";
        return Deliver<BrepBody>::Reject(RefusalReason::NonManifold, Explain(Report.Diagnostic));
    }
    Report.Diagnostic = "ok";
    return Deliver<BrepBody>::Accept(std::move(Result));
}

}
