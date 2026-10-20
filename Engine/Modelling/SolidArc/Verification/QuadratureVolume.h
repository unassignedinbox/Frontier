// 📦 Engine/Modelling/SolidArc/Verification/QuadratureVolume.h — exact-surface volume for verification harnesses
//
// Shared by RollingBallBlendVerification, FeatureSuitabilityVerification and BrepDefectVerification so every harness
//    judges volume by the same independent integral (not the chord-tolerance tessellation volume of Validate()).
#pragma once
#include "Kernel/TopologySpecification.h"
#include <cmath>
#include <vector>

namespace Frontier
{
// Volume by the divergence theorem with a degree-5 Dunavant rule on every (u,v) triangle of every face, evaluated on
//    the exact surfaces: V = ⅓ Σ_faces ±∬ S·(S_u × S_v) du dv. Independent of the chord-tolerance tessellation volume.
inline double QuadratureVolume(const BrepBody& B)
{
    static const double W[7] = { 0.225, 0.132394152788506, 0.132394152788506, 0.132394152788506, 0.125939180544827, 0.125939180544827, 0.125939180544827 };
    static const double A1 = 0.059715871789770, B1 = 0.470142064105115, A2 = 0.797426985353087, B2 = 0.101286507323456;
    static const double L[7][3] = { { 1.0 / 3, 1.0 / 3, 1.0 / 3 }, { A1, B1, B1 }, { B1, A1, B1 }, { B1, B1, A1 }, { A2, B2, B2 }, { B2, A2, B2 }, { B2, B2, A2 } };
    double Volume = 0.0;
    static const double GX[8] = { -0.9602898564975363, -0.7966664774136267, -0.5255324099163290, -0.1834346424956498, 0.1834346424956498, 0.5255324099163290, 0.7966664774136267, 0.9602898564975363 };
    static const double GW[8] = { 0.1012285362903763, 0.2223810344533745, 0.3137066699503325, 0.3626837833783620, 0.3626837833783620, 0.3137066699503325, 0.2223810344533745, 0.1012285362903763 };
    for (size_t F = 0; F < B.Faces.size(); ++F)
    {
        if (B.Faces[F].Surface.Classification == SurfaceClassification::Plane)
        {
            // planar face: ∬ x·n dA = (p0·n)·A with A·n = ½ ∮ x × dx over the exact edge curves (Green / Stokes)
            const NurbsSurface& S = B.Faces[F].Surface;
            Vec3 N = B.FaceNormal(static_cast<int>(F), 0.5 * (S.DomainStartU() + S.DomainEndU()), 0.5 * (S.DomainStartV() + S.DomainEndV()));
            Vec3 P0 = S.Sample(S.DomainStartU(), S.DomainStartV());
            double Area = 0.0;
            for (int L : B.Faces[F].Loops)
                for (int C : B.Loops[L].Coedges)
                {
                    NurbsCurve K = B.CoedgeCurve(C);
                    std::vector<double> Spans;
                    for (double Knot : K.Knots) if (Spans.empty() || Knot > Spans.back() + 1e-14) Spans.push_back(Knot);
                    for (size_t I = 0; I + 1 < Spans.size(); ++I)
                    {
                        double A = Spans[I], Z = Spans[I + 1];
                        if (A < K.DomainStart() - 1e-14 || Z > K.DomainEnd() + 1e-14) continue;
                        for (int G = 0; G < 8; ++G)
                        {
                            double T = 0.5 * (A + Z) + 0.5 * (Z - A) * GX[G], H = 1e-6 * (Z - A);
                            Vec3 X = K.Sample(T), D = (K.Sample(T + H) - K.Sample(T - H)) / (2.0 * H);
                            Area += 0.5 * GW[G] * 0.5 * (Z - A) * N.Dot(X.Cross(D));
                        }
                    }
                }
            Volume += P0.Dot(N) * Area / 3.0;
            continue;
        }
        BrepBody::FaceTriangles T = B.TessellateFace(static_cast<int>(F), 1e-5);
        const NurbsSurface& S = B.Faces[F].Surface;
        double Sign = B.Faces[F].Reversed ? -1.0 : 1.0, Face = 0.0;
        for (size_t K = 0; K + 2 < T.Triangles.size(); K += 3)
        {
            Vec2 P0 = T.Parameters[T.Triangles[K]], P1 = T.Parameters[T.Triangles[K + 1]], P2 = T.Parameters[T.Triangles[K + 2]];
            double Area = 0.5 * std::fabs((P1 - P0).Cross(P2 - P0));
            double Sum = 0.0;
            for (int Q = 0; Q < 7; ++Q)
            {
                Vec2 UV = P0 * L[Q][0] + P1 * L[Q][1] + P2 * L[Q][2];
                Vec3 P, DU, DV; S.Derivatives(UV.X, UV.Y, P, DU, DV);
                Sum += W[Q] * P.Dot(DU.Cross(DV));
            }
            Face += Area * Sum;
        }
        Volume += Sign * Face / 3.0;
    }
    return Volume;
}
} // namespace Frontier
