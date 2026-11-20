/*============================================================================================================================================
                                                  CONTROLVEHICLERENDEREXHIBIT.CPP
============================================================================================================================================*/
// 🧩 High-Resolution (1280x720) 10-Render ControlVehicle Showcase using Real .face Meshes, WideBoundingTree BVH4, OpenPBR & Deterministic PT

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Constants/MathConstants.h"
#include "Engine/GeometricRaster/GlobalIllumination/DeterministicDiffuseGi.h"
#include "Engine/GeometricRaster/GlobalIllumination/StokesContourGiSequence.h"
#include "Engine/GeometricRaster/GlobalIllumination/SurfelCascadeGiSequence.h"
#include <chrono>
#include "Engine/GeometricRaster/DirectLighting/SurfelOccluderField.h"
#include "Engine/GeometricRaster/DirectLighting/LinearCosineLightIntegration.h"
#include "Engine/GeometricRaster/DirectLighting/SolarShadowSequence.h"
#include "Engine/GeometrySpace/Analysis/CurvatureAnalysis.h"
#include "Engine/MaterialSpace/CompactMaterialRegister.h"
#include "Engine/MaterialSpace/Scattering/OpenPbrSurfaceScattering.h"
#include "Engine/SpatialAcceleration/WideBoundingTree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace Frontier;
using namespace Frontier::GeometricRaster;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                              FCT1 BINARY .FACE STREAM READER
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] uint32_t ReadUint32LE(const std::byte* Bytes) noexcept
{
    uint32_t Out = 0u;
    std::memcpy(&Out, Bytes, sizeof(Out));
    return Out;
}

[[nodiscard]] float ReadFloat32LE(const std::byte* Bytes) noexcept
{
    float Out = 0.0f;
    std::memcpy(&Out, Bytes, sizeof(Out));
    return Out;
}

struct LoadedFct1Mesh
{
    std::vector<Mathematics::Axis3D>                    Positions {};
    std::vector<std::array<uint32_t, 3u>>               Triangles {};
    std::vector<uint32_t>                               SlotIds   {};
    std::vector<SpatialAcceleration::TriangleSmoothNormals> Normals {};
};

[[nodiscard]] bool LoadFct1FaceFile(const std::filesystem::path& FilePath, LoadedFct1Mesh& Out)
{
    std::ifstream Stream(FilePath, std::ios::binary | std::ios::ate);
    if (!Stream)
    {
        std::fprintf(stderr, "Failed to open FCT1 file: %s\n", FilePath.c_str());
        return false;
    }
    const std::streamsize FileSize = Stream.tellg();
    if (FileSize < 48)
    {
        return false;
    }
    std::vector<std::byte> Raw(static_cast<std::size_t>(FileSize));
    Stream.seekg(0, std::ios::beg);
    if (!Stream.read(reinterpret_cast<char*>(Raw.data()), FileSize))
    {
        return false;
    }
    if (std::memcmp(Raw.data(), "FCT1", 4) != 0)
    {
        return false;
    }

    const uint32_t VC     = ReadUint32LE(Raw.data() + 4u);
    const uint32_t FC     = ReadUint32LE(Raw.data() + 8u);
    const uint32_t Mask   = ReadUint32LE(Raw.data() + 12u);
    const bool     HasNrm = (Mask & 1u) != 0u;

    const std::size_t PosBytes = static_cast<std::size_t>(VC) * 12u;
    const std::size_t IdxBytes = static_cast<std::size_t>(FC) * 12u;
    const std::size_t SltBytes = static_cast<std::size_t>(FC) * 4u;
    const std::size_t NrmBytes = HasNrm ? static_cast<std::size_t>(FC) * 36u : 0u;
    if (Raw.size() != 48u + PosBytes + IdxBytes + SltBytes + NrmBytes)
    {
        return false;
    }

    const std::byte* PosPtr = Raw.data() + 48u;
    const std::byte* IdxPtr = PosPtr + PosBytes;
    const std::byte* SltPtr = IdxPtr + IdxBytes;
    const std::byte* NrmPtr = SltPtr + SltBytes;

    Out.Positions.resize(VC);
    for (uint32_t V = 0u; V < VC; ++V)
    {
        Out.Positions[V] = {
            ReadFloat32LE(PosPtr + V * 12u + 0u),
            ReadFloat32LE(PosPtr + V * 12u + 4u),
            ReadFloat32LE(PosPtr + V * 12u + 8u),
        };
    }

    Out.Triangles.reserve(FC);
    Out.SlotIds.reserve(FC);
    Out.Normals.reserve(FC);

    std::vector<Mathematics::Axis3D> FaceNormals(FC, { 0.0f, 0.0f, 1.0f });
    std::vector<float>               FaceAreas(FC, 0.0f);
    std::vector<std::vector<uint32_t>> VertexTris(VC);

    for (uint32_t T = 0u; T < FC; ++T)
    {
        const uint32_t A = ReadUint32LE(IdxPtr + T * 12u + 0u);
        const uint32_t B = ReadUint32LE(IdxPtr + T * 12u + 4u);
        const uint32_t C = ReadUint32LE(IdxPtr + T * 12u + 8u);
        if (A >= VC || B >= VC || C >= VC || A == B || B == C || A == C)
        {
            continue;
        }
        const uint32_t TriIdx = static_cast<uint32_t>(Out.Triangles.size());
        Out.Triangles.push_back({ A, B, C });
        Out.SlotIds.push_back(ReadUint32LE(SltPtr + T * 4u));

        const Mathematics::Axis3D E1 = Out.Positions[B] - Out.Positions[A];
        const Mathematics::Axis3D E2 = Out.Positions[C] - Out.Positions[A];
        const Mathematics::Axis3D Cr = Mathematics::EvaluateCross(E1, E2);
        const float Len = Mathematics::EvaluateLength(Cr);
        const Mathematics::Axis3D FN = (Len > 1.0e-8f) ? (Cr * (1.0f / Len)) : Mathematics::Axis3D{ 0.0f, 0.0f, 1.0f };
        FaceNormals[TriIdx] = FN;
        FaceAreas[TriIdx]   = 0.5f * Len;
        VertexTris[A].push_back(TriIdx);
        VertexTris[B].push_back(TriIdx);
        VertexTris[C].push_back(TriIdx);

        if (HasNrm)
        {
            const std::byte* Base = NrmPtr + static_cast<std::size_t>(T) * 36u;
            SpatialAcceleration::TriangleSmoothNormals SN {};
            SN.Normal0 = Mathematics::NormalizeAxis({ ReadFloat32LE(Base + 0u),  ReadFloat32LE(Base + 4u),  ReadFloat32LE(Base + 8u) }, FN);
            SN.Normal1 = Mathematics::NormalizeAxis({ ReadFloat32LE(Base + 12u), ReadFloat32LE(Base + 16u), ReadFloat32LE(Base + 20u) }, FN);
            SN.Normal2 = Mathematics::NormalizeAxis({ ReadFloat32LE(Base + 24u), ReadFloat32LE(Base + 28u), ReadFloat32LE(Base + 32u) }, FN);
            Out.Normals.push_back(SN);
        }
        else
        {
            Out.Normals.push_back({ FN, FN, FN });
        }
    }

    // Smooth dihedral normal synthesis when corner normals aren't baked (e.g. Body)
    if (!HasNrm)
    {
        const float CosThresh = std::cos(42.0f * Constants::DegToRadFloat);
        for (std::size_t T = 0u; T < Out.Triangles.size(); ++T)
        {
            const auto& Tri = Out.Triangles[T];
            const auto& FN  = FaceNormals[T];
            auto SmoothCorner = [&](uint32_t VertIdx) noexcept -> Mathematics::Axis3D
            {
                Mathematics::Axis3D Sum {};
                for (uint32_t Adj : VertexTris[VertIdx])
                {
                    if (Mathematics::EvaluateDot(FN, FaceNormals[Adj]) >= CosThresh)
                    {
                        Sum = Sum + FaceNormals[Adj] * std::max(1.0e-6f, FaceAreas[Adj]);
                    }
                }
                return Mathematics::NormalizeAxis(Sum, FN);
            };
            Out.Normals[T].Normal0 = SmoothCorner(Tri[0]);
            Out.Normals[T].Normal1 = SmoothCorner(Tri[1]);
            Out.Normals[T].Normal2 = SmoothCorner(Tri[2]);
        }
    }
    return true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                       ASSEMBLE REAL CONTROL VEHICLE + STUDIO STAGE
//------------------------------------------------------------------------------------------------------------------------

void AppendTransformedFct1Mesh(
    const LoadedFct1Mesh&                                    Src,
    Mathematics::Axis3D                                      TranslationWorld,
    bool                                                     Rotate180YawZ,
    const std::array<uint16_t, 16u>&                         SlotToMatOrdinal,
    std::vector<SpatialAcceleration::RayTrianglePrimitive>&  OutPrims,
    std::vector<SpatialAcceleration::TriangleSmoothNormals>& OutNormals)
{
    auto XformPt = [&](Mathematics::Axis3D P) noexcept -> Mathematics::Axis3D
    {
        if (Rotate180YawZ)
        {
            P.X = -P.X;
            P.Y = -P.Y;
        }
        return P + TranslationWorld;
    };
    auto XformDir = [&](Mathematics::Axis3D D) noexcept -> Mathematics::Axis3D
    {
        if (Rotate180YawZ)
        {
            D.X = -D.X;
            D.Y = -D.Y;
        }
        return D;
    };

    for (std::size_t T = 0u; T < Src.Triangles.size(); ++T)
    {
        const auto& Tri = Src.Triangles[T];
        const auto  P0  = XformPt(Src.Positions[Tri[0]]);
        const auto  P1  = XformPt(Src.Positions[Tri[1]]);
        const auto  P2  = XformPt(Src.Positions[Tri[2]]);

        const uint32_t SlotId = Src.SlotIds[T];
        const uint16_t MatOrd = (SlotId < 16u) ? SlotToMatOrdinal[SlotId] : SlotToMatOrdinal[15u];

        const uint32_t Idx = static_cast<uint32_t>(OutPrims.size());
        SpatialAcceleration::RayTrianglePrimitive Prim {};
        Prim.Corner0          = P0;
        Prim.Edge01           = P1 - P0;
        Prim.Edge02           = P2 - P0;
        Prim.SourceFaceIndex  = Idx;
        Prim.PrimitiveOrdinal = Idx;
        Prim.MaterialOrdinal  = MatOrd;
        Prim.OpaqueMask       = 1u;
        OutPrims.push_back(Prim);

        SpatialAcceleration::TriangleSmoothNormals SN {};
        SN.Normal0 = XformDir(Src.Normals[T].Normal0);
        SN.Normal1 = XformDir(Src.Normals[T].Normal1);
        SN.Normal2 = XformDir(Src.Normals[T].Normal2);
        OutNormals.push_back(SN);
    }
}

void AppendStudioBoxZUp(
    std::vector<SpatialAcceleration::RayTrianglePrimitive>&  OutPrims,
    std::vector<SpatialAcceleration::TriangleSmoothNormals>& OutNormals,
    Mathematics::Axis3D                                      Center,
    Mathematics::Axis3D                                      Half,
    float                                                    YawZRadians,
    uint16_t                                                 MatOrd)
{
    const float Cs = std::cos(YawZRadians);
    const float Sn = std::sin(YawZRadians);
    auto Xform = [&](Mathematics::Axis3D Local) noexcept -> Mathematics::Axis3D
    {
        return {
            Center.X + Local.X * Cs - Local.Y * Sn,
            Center.Y + Local.X * Sn + Local.Y * Cs,
            Center.Z + Local.Z,
        };
    };
    auto XformN = [&](Mathematics::Axis3D LocalN) noexcept -> Mathematics::Axis3D
    {
        return {
            LocalN.X * Cs - LocalN.Y * Sn,
            LocalN.X * Sn + LocalN.Y * Cs,
            LocalN.Z,
        };
    };

    const Mathematics::Axis3D V[8] = {
        Xform({ -Half.X, -Half.Y, -Half.Z }),
        Xform({  Half.X, -Half.Y, -Half.Z }),
        Xform({  Half.X,  Half.Y, -Half.Z }),
        Xform({ -Half.X,  Half.Y, -Half.Z }),
        Xform({ -Half.X, -Half.Y,  Half.Z }),
        Xform({  Half.X, -Half.Y,  Half.Z }),
        Xform({  Half.X,  Half.Y,  Half.Z }),
        Xform({ -Half.X,  Half.Y,  Half.Z }),
    };
    const uint32_t Quads[6][4] = {
        { 0u, 3u, 2u, 1u },
        { 4u, 5u, 6u, 7u },
        { 0u, 1u, 5u, 4u },
        { 2u, 3u, 7u, 6u },
        { 0u, 4u, 7u, 3u },
        { 1u, 2u, 6u, 5u },
    };
    const Mathematics::Axis3D FaceN[6] = {
        XformN({ 0.0f, 0.0f, -1.0f }), XformN({ 0.0f, 0.0f, 1.0f }),
        XformN({ 0.0f, -1.0f, 0.0f }), XformN({ 0.0f, 1.0f, 0.0f }),
        XformN({ -1.0f, 0.0f, 0.0f }), XformN({ 1.0f, 0.0f, 0.0f }),
    };

    for (uint32_t F = 0u; F < 6u; ++F)
    {
        const auto& Q = Quads[F];
        const auto& N = FaceN[F];
        for (uint32_t Tri = 0u; Tri < 2u; ++Tri)
        {
            const uint32_t I0 = Q[0];
            const uint32_t I1 = (Tri == 0u) ? Q[1] : Q[2];
            const uint32_t I2 = (Tri == 0u) ? Q[2] : Q[3];
            const uint32_t Idx = static_cast<uint32_t>(OutPrims.size());
            SpatialAcceleration::RayTrianglePrimitive Prim {};
            Prim.Corner0          = V[I0];
            Prim.Edge01           = V[I1] - V[I0];
            Prim.Edge02           = V[I2] - V[I0];
            Prim.SourceFaceIndex  = Idx;
            Prim.PrimitiveOrdinal = Idx;
            Prim.MaterialOrdinal  = MatOrd;
            Prim.OpaqueMask       = 1u;
            OutPrims.push_back(Prim);
            OutNormals.push_back({ N, N, N });
        }
    }
}

void AppendProceduralRearWingZUp(
    std::vector<SpatialAcceleration::RayTrianglePrimitive>&  OutPrims,
    std::vector<SpatialAcceleration::TriangleSmoothNormals>& OutNormals,
    const std::array<uint16_t, 16u>&                         SlotToMatOrdinal)
{
    // Exact ControlVehicleClassBuilder::ConstructProceduralRearWing translated from Y-up to Z-up: (x, y_yup, z_yup) -> (x, -z_yup, y_yup)
    constexpr float GroundOffset   = 0.50855f - 0.0914f;
    const Mathematics::Axis3D WingOrigin { -2.9822f, 0.0f, 0.8367f + GroundOffset };
    constexpr float AoaDeg         = 8.0f;
    constexpr float BodyHalfWidth  = 1.2498f;
    constexpr float ChordHalf      = 0.17f;
    constexpr float PlaneHalfThick = 0.016f;
    constexpr float PlateHalfThick = 0.008f;
    constexpr float PlaneY         = 0.25f;
    const float Tilt = -AoaDeg * Constants::DegToRadFloat;
    const float PlateZ = BodyHalfWidth - PlateHalfThick;
    const float Span   = PlateZ - PlateHalfThick;

    auto AppendWingBox = [&](Mathematics::Axis3D CenterYup, Mathematics::Axis3D HalfYup, uint32_t SlotId, float TiltRad)
    {
        const float Cs = std::cos(TiltRad);
        const float Sn = std::sin(TiltRad);
        std::array<Mathematics::Axis3D, 8u> V {};
        for (uint32_t C = 0u; C < 8u; ++C)
        {
            const float X = ((C & 1u) != 0u ? 1.0f : -1.0f) * HalfYup.X;
            const float Y = ((C & 2u) != 0u ? 1.0f : -1.0f) * HalfYup.Y;
            const float Z = ((C & 4u) != 0u ? 1.0f : -1.0f) * HalfYup.Z;
            const float Xy = CenterYup.X + X * Cs - Y * Sn;
            const float Yy = CenterYup.Y + X * Sn + Y * Cs;
            const float Zy = CenterYup.Z + Z;
            // Convert Y-up to Z-up and translate to WingOrigin
            V[C] = WingOrigin + Mathematics::Axis3D{ Xy, -Zy, Yy };
        }
        constexpr std::array<std::array<uint32_t, 4u>, 6u> Quads {{
            { 0u, 2u, 3u, 1u }, { 4u, 5u, 7u, 6u }, { 0u, 1u, 5u, 4u },
            { 2u, 6u, 7u, 3u }, { 0u, 4u, 6u, 2u }, { 1u, 3u, 7u, 5u },
        }};
        const uint16_t MatOrd = SlotToMatOrdinal[SlotId];
        for (const auto& Q : Quads)
        {
            for (uint32_t Tri = 0u; Tri < 2u; ++Tri)
            {
                const auto P0 = V[Q[0]];
                const auto P1 = V[(Tri == 0u) ? Q[1] : Q[2]];
                const auto P2 = V[(Tri == 0u) ? Q[2] : Q[3]];
                const auto N  = Mathematics::NormalizeAxis(Mathematics::EvaluateCross(P1 - P0, P2 - P0), { 0.0f, 0.0f, 1.0f });
                const uint32_t Idx = static_cast<uint32_t>(OutPrims.size());
                SpatialAcceleration::RayTrianglePrimitive Prim {};
                Prim.Corner0 = P0;
                Prim.Edge01 = P1 - P0;
                Prim.Edge02 = P2 - P0;
                Prim.SourceFaceIndex = Idx;
                Prim.PrimitiveOrdinal = Idx;
                Prim.MaterialOrdinal = MatOrd;
                Prim.OpaqueMask = 1u;
                OutPrims.push_back(Prim);
                OutNormals.push_back({ N, N, N });
            }
        }
    };

    AppendWingBox({ 0.0f, PlaneY, 0.0f }, { ChordHalf, PlaneHalfThick, Span }, 8u, Tilt);
    const float TeX = -ChordHalf * std::cos(Tilt) - PlaneHalfThick * std::sin(Tilt);
    const float TeY = PlaneY - ChordHalf * std::sin(Tilt) + PlaneHalfThick * std::cos(Tilt);
    AppendWingBox(
        { TeX + 0.008f * std::cos(Tilt) - 0.022f * std::sin(Tilt), TeY + 0.008f * std::sin(Tilt) + 0.022f * std::cos(Tilt), 0.0f },
        { 0.008f, 0.022f, Span }, 6u, Tilt);
    AppendWingBox({ -0.01f, 0.23f, -PlateZ }, { 0.20f, 0.10f, PlateHalfThick }, 6u, 0.0f);
    AppendWingBox({ -0.01f, 0.23f,  PlateZ }, { 0.20f, 0.10f, PlateHalfThick }, 6u, 0.0f);
    AppendWingBox({  0.02f, 0.11f, -0.56f  }, { 0.05f, 0.11f, 0.01f }, 10u, 0.0f);
    AppendWingBox({  0.02f, 0.11f,  0.56f  }, { 0.05f, 0.11f, 0.01f }, 10u, 0.0f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                    OWEN-SCRAMBLED SOBOL (0,2) + DUPUY-BENYOUB GGX VNDF
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] inline uint32_t ReverseBits32(uint32_t X) noexcept
{
    X = ((X & 0x55555555u) << 1u) | ((X >> 1u) & 0x55555555u);
    X = ((X & 0x33333333u) << 2u) | ((X >> 2u) & 0x33333333u);
    X = ((X & 0x0F0F0F0Fu) << 4u) | ((X >> 4u) & 0x0F0F0F0Fu);
    X = ((X & 0x00FF00FFu) << 8u) | ((X >> 8u) & 0x00FF00FFu);
    return (X << 16u) | (X >> 16u);
}

[[nodiscard]] inline uint32_t AvalancheKey32(uint32_t Input) noexcept
{
    uint32_t Mixed = Input;
    Mixed ^= Mixed >> 16u;
    Mixed *= 0x7FEB352Du;
    Mixed ^= Mixed >> 15u;
    Mixed *= 0x846CA68Bu;
    Mixed ^= Mixed >> 16u;
    return Mixed;
}

[[nodiscard]] inline uint32_t LaineKarrasPerm(uint32_t Input, uint32_t Key) noexcept
{
    uint32_t P = Input + Key;
    P ^= P * 0x6C50B47Cu;
    P ^= P * 0xB82F1E52u;
    P ^= P * 0xC7AFE638u;
    P ^= P * 0x8D22F6E6u;
    return P;
}

[[nodiscard]] inline uint32_t SobolSecondDim(uint32_t Index) noexcept
{
    uint32_t Bits = Index;
    Bits ^= (Bits & 0xAAAAAAAAu) >> 1u;
    Bits ^= (Bits & 0xCCCCCCCCu) >> 2u;
    Bits ^= (Bits & 0xF0F0F0F0u) >> 4u;
    Bits ^= (Bits & 0xFF00FF00u) >> 8u;
    Bits ^= (Bits & 0xFFFF0000u) >> 16u;
    return ReverseBits32(Bits);
}

[[nodiscard]] inline Mathematics::Axis2D Sobol02(uint32_t SampleIdx, uint32_t PixKey, uint32_t PairIdx) noexcept
{
    const uint32_t PK  = AvalancheKey32(PairIdx * 0x9E3779B9u + 0x85EBCA6Bu) ^ PixKey;
    const uint32_t Idx = ReverseBits32(LaineKarrasPerm(ReverseBits32(SampleIdx), PK));
    const uint32_t D0  = ReverseBits32(LaineKarrasPerm(Idx, AvalancheKey32(PK ^ 0xA511E9B3u)));
    const uint32_t D1  = ReverseBits32(LaineKarrasPerm(ReverseBits32(SobolSecondDim(Idx)), AvalancheKey32(PK ^ 0x63D83595u)));
    constexpr float Inv24 = 1.0f / 16777216.0f;
    return { static_cast<float>(D0 >> 8u) * Inv24, static_cast<float>(D1 >> 8u) * Inv24 };
}

[[nodiscard]] inline Mathematics::Axis3D SampleCosineHemiLocal(Mathematics::Axis2D U) noexcept
{
    const float R = std::sqrt(std::max(0.0f, U.X));
    const float Phi = Constants::TauFloat * U.Y;
    return { R * std::cos(Phi), R * std::sin(Phi), std::sqrt(std::max(0.0f, 1.0f - U.X)) };
}

[[nodiscard]] inline Mathematics::Axis3D SampleSphericalCapGgxVndfLocal(
    Mathematics::Axis3D LocalWo,
    float               Alpha,
    Mathematics::Axis2D U) noexcept
{
    const float A = std::clamp(Alpha, 0.001f, 1.0f);
    const Mathematics::Axis3D Vh = Mathematics::NormalizeAxis({ A * LocalWo.X, A * LocalWo.Y, LocalWo.Z });
    const float Phi = Constants::TauFloat * U.X;
    const float Z   = std::fma(1.0f - U.Y, 1.0f + Vh.Z, -Vh.Z);
    const float SinT = std::sqrt(std::clamp(1.0f - Z * Z, 0.0f, 1.0f));
    const Mathematics::Axis3D C { SinT * std::cos(Phi), SinT * std::sin(Phi), Z };
    const Mathematics::Axis3D H = C + Vh;
    return Mathematics::NormalizeAxis({ A * H.X, A * H.Y, std::max(0.0f, H.Z) });
}

//------------------------------------------------------------------------------------------------------------------------
//                                           OPENPBR + ACES + PPM WRITER
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] constexpr Mathematics::LinearRGB MulRgb(Mathematics::LinearRGB A, Mathematics::LinearRGB B) noexcept
{
    return { A.R * B.R, A.G * B.G, A.B * B.B };
}

[[nodiscard]] inline Mathematics::LinearRGB ApplyAcesFilmic(Mathematics::LinearRGB Hdr, float Exposure = 1.05f) noexcept
{
    const auto Curve = [&](float X) noexcept -> float
    {
        const float V = std::max(0.0f, X * Exposure * 0.85f);
        return std::clamp((V * (2.51f * V + 0.03f)) / (V * (2.43f * V + 0.59f) + 0.14f), 0.0f, 1.0f);
    };
    return { Curve(Hdr.R), Curve(Hdr.G), Curve(Hdr.B) };
}

[[nodiscard]] inline uint8_t ToSrgb8(float LinearVal) noexcept
{
    const float C = std::clamp(LinearVal, 0.0f, 1.0f);
    const float S = (C <= 0.0031308f) ? (12.92f * C) : (1.055f * std::pow(C, 1.0f / 2.4f) - 0.055f);
    return static_cast<uint8_t>(std::lround(std::clamp(S, 0.0f, 1.0f) * 255.0f));
}

struct RenderRigConfig
{
    std::string          OutputName {};
    std::string          Label      {};
    Mathematics::Axis3D  CamPos     {};
    Mathematics::Axis3D  CamTarget  {};
    float                FovDeg     = 40.0f;
    Mathematics::Axis3D  SunDir     {};
    Mathematics::LinearRGB SunIrrad {};
    Mathematics::LinearRGB SkyColor {};
    uint32_t             Mode       = 0u; // 0=OpenPbrDirect+BVH4Refl+8Shadow, 1=MatCapClay, 2=DeterministicPT16spp
    float                Exposure   = 1.08f;
    float                EmissiveBoost = 1.0f;
};

struct SceneData
{
    SpatialAcceleration::WideBoundingTree  Tree        {};
    MaterialSpace::CompactMaterialRegister MatReg      {};
    std::vector<uint32_t>                  MatLobeMask {};
    std::vector<Mathematics::LinearRGB>    MatEmission {};
    PolygonalAreaQuadLight                 Softbox     {};
    uint16_t                               FloorMat    = 0u;
    uint16_t                               SoftboxMat  = 0u;
};

[[nodiscard]] Mathematics::LinearRGB EvaluateStudioFloorPattern(
    Mathematics::Axis3D P,
    Mathematics::LinearRGB Base) noexcept
{
    // Subtle calibrated automotive turntable plinth + metric grid markings on floor Z ~ 0
    if (std::fabs(P.Z) > 0.02f)
    {
        return Base;
    }
    const float R = std::sqrt(P.X * P.X + P.Y * P.Y);
    if (std::fabs(R - 3.65f) < 0.035f)
    {
        return { 0.80f, 0.72f, 0.16f }; // Subtle racing-yellow turntable ring
    }
    const float GX = std::fabs(P.X - std::round(P.X));
    const float GY = std::fabs(P.Y - std::round(P.Y));
    if (GX < 0.012f || GY < 0.012f)
    {
        return { Base.R * 0.86f, Base.G * 0.86f, Base.B * 0.88f };
    }
    return Base;
}

[[nodiscard]] Mathematics::LinearRGB EvaluateSkyBackdrop(
    Mathematics::Axis3D    Dir,
    Mathematics::LinearRGB SkyBase,
    Mathematics::Axis3D    SunDir,
    Mathematics::LinearRGB SunIrrad) noexcept
{
    const float Up = std::clamp(Dir.Z * 0.5f + 0.5f, 0.0f, 1.0f);
    Mathematics::LinearRGB Sky = SkyBase * (0.55f + 0.85f * Up);
    const float SunCos = std::max(0.0f, Mathematics::EvaluateDot(Dir, SunDir));
    const float Halo = std::pow(SunCos, 16.0f) * 0.14f + std::pow(SunCos, 180.0f) * 0.65f;
    return Sky + SunIrrad * Halo;
}

[[nodiscard]] Mathematics::LinearRGB ShadeSurfaceDirect(
    const SceneData&       Sc,
    Mathematics::Axis3D    Pos,
    Mathematics::Axis3D    SmoothN,
    Mathematics::Axis3D    GeomN,
    Mathematics::Axis3D    Wo,
    uint16_t               MatSlot,
    uint32_t               PX,
    uint32_t               PY,
    Mathematics::Axis3D    SunDir,
    Mathematics::LinearRGB SunIrrad,
    Mathematics::LinearRGB SkyColor,
    float                  EmissiveBoost,
    bool                   TraceSunSoft,
    bool                   AllowReflection,
    uint32_t               BounceDepth = 0u,
    bool                   IncludeAmbient = true,
    bool                   IncludeSoftbox = true,
    float                  SolarVisibilityOverride = -1.0f) noexcept
{
    const auto& RawMat = Sc.MatReg.InspectAuthoredMaterial(MatSlot);
    MaterialSpace::SurfaceMaterial Mat = RawMat;
    if (MatSlot == Sc.FloorMat)
    {
        Mat.Diffuse.BaseColor = EvaluateStudioFloorPattern(Pos, RawMat.Diffuse.BaseColor);
    }

    Mathematics::Axis3D T {}, B {};
    GeometrySpace::ConstructDuffOrthonormalBasis(SmoothN, T, B);
    const MaterialSpace::Scattering::SurfaceShadingBasis Basis { T, B, SmoothN };
    const Mathematics::Axis3D LocalWo = Basis.WorldToLocal(Wo);

    Mathematics::LinearRGB Out = Sc.MatEmission[MatSlot] * EmissiveBoost;

    // 1. Solar Direct + Soft 8-Tap Vogel BVH4 Shadow (or 1-tap on secondary reflection)
    const float NdotL = Mathematics::EvaluateDot(SmoothN, SunDir);
    if (NdotL > 1.0e-4f)
    {
        float SunVis = SolarVisibilityOverride >= 0.0f ? SolarVisibilityOverride : 1.0f;
        if (TraceSunSoft && SolarVisibilityOverride < 0.0f)
        {
            Mathematics::Axis3D ST {}, SB {};
            GeometrySpace::ConstructDuffOrthonormalBasis(SunDir, ST, SB);
            const float TanSun = 0.016f;
            const float RotPhi = std::fmod(52.9829189f * std::fmod(PX * 0.06711056f + PY * 0.00583715f, 1.0f), 1.0f) * Constants::TauFloat;
            const uint32_t Taps = (BounceDepth == 0u) ? 16u : 1u;
            float VisSum = 0.0f;
            for (uint32_t R = 0u; R < Taps; ++R)
            {
                const float Rad = (Taps == 1u) ? 0.0f : (std::sqrt((static_cast<float>(R) + 0.5f) / static_cast<float>(Taps)) * TanSun);
                const float Ang = static_cast<float>(R) * 2.39996323f + RotPhi;
                const Mathematics::Axis3D JDir = Mathematics::NormalizeAxis(SunDir + ST * (Rad * std::cos(Ang)) + SB * (Rad * std::sin(Ang)));
                const SpatialAcceleration::RaySegmentQuery SQ { Pos + GeomN * 0.003f, 0.002f, JDir, 80.0f };
                if (!Sc.Tree.QueryRayOccluded(SQ))
                {
                    VisSum += 1.0f;
                }
            }
            SunVis = VisSum / static_cast<float>(Taps);
        }
        if (SunVis > 1.0e-4f)
        {
            const Mathematics::Axis3D LocalWi = Basis.WorldToLocal(SunDir);
            const auto F = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(Mat, Sc.MatLobeMask[MatSlot], LocalWo, LocalWi);
            Out = Out + MulRgb(F, SunIrrad) * (NdotL * SunVis);
        }
    }

    // 2. Overhead Studio Softbox Area Light (4-point stratified quad visibility + OpenPBR BSDF)
    if (IncludeSoftbox)
    {
        const auto& Q = Sc.Softbox;
        const uint32_t QTaps = (BounceDepth == 0u) ? 4u : 1u;
        const float Area = 4.0f * Q.HalfWidthMeters * Q.HalfHeightMeters;
        Mathematics::LinearRGB QuadSum {};
        for (uint32_t K = 0u; K < QTaps; ++K)
        {
            const float U = (QTaps == 1u) ? 0.0f : (((K & 1u) ? 0.5f : -0.5f) * 1.15f);
            const float V = (QTaps == 1u) ? 0.0f : (((K & 2u) ? 0.5f : -0.5f) * 1.15f);
            const Mathematics::Axis3D LP = Q.CenterWorld + Q.RightTangent * (U * Q.HalfWidthMeters) + Q.UpBitangent * (V * Q.HalfHeightMeters);
            const Mathematics::Axis3D Delta = LP - Pos;
            const float DistSq = Mathematics::EvaluateLengthSquared(Delta);
            const float Dist   = std::sqrt(std::max(1.0e-5f, DistSq));
            const Mathematics::Axis3D LDir = Delta * (1.0f / Dist);
            const float CosR = Mathematics::EvaluateDot(SmoothN, LDir);
            const float CosL = std::max(0.0f, -LDir.Z); // Softbox faces -Z
            if (CosR > 1.0e-4f && CosL > 1.0e-4f)
            {
                const SpatialAcceleration::RaySegmentQuery SQ { Pos + GeomN * 0.003f, 0.002f, LDir, Dist - 0.02f };
                if (!Sc.Tree.QueryRayOccluded(SQ))
                {
                    const auto F = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(
                        Mat, Sc.MatLobeMask[MatSlot], LocalWo, Basis.WorldToLocal(LDir));
                    const float Geom = (CosR * CosL * Area) / (DistSq * static_cast<float>(QTaps));
                    QuadSum = QuadSum + MulRgb(F, Q.EmittedRadiance) * Geom;
                }
            }
        }
        Out = Out + QuadSum;
    }

    float Ao = 1.0f;
    if (IncludeAmbient)
    {
    // 3. Hemisphere Sky Ambient + Short-Range BVH4 Cavity AO
    if (BounceDepth == 0u)
    {
        float Occ = 0.0f;
        for (uint32_t K = 0u; K < 4u; ++K)
        {
            const auto U = Sobol02(K, AvalancheKey32(PX * 1973u + PY * 9277u + 17u), 1u);
            const auto LocalD = SampleCosineHemiLocal(U);
            const auto WorldD = Basis.LocalToWorld(LocalD);
            const SpatialAcceleration::RaySegmentQuery AQ { Pos + GeomN * 0.003f, 0.002f, WorldD, 1.35f };
            if (Sc.Tree.QueryRayOccluded(AQ))
            {
                Occ += 0.25f;
            }
        }
        Ao = 1.0f - 0.72f * Occ;
    }
    const float DiffWeight = (1.0f - Mat.Specular.Metalness) * Mat.Diffuse.BaseWeight;
    const auto SkyFill = EvaluateSkyBackdrop(SmoothN, SkyColor, SunDir, SunIrrad);
    Out.R += SkyFill.R * 0.28f * Mat.Diffuse.BaseColor.R * DiffWeight * Ao;
    Out.G += SkyFill.G * 0.28f * Mat.Diffuse.BaseColor.G * DiffWeight * Ao;
    Out.B += SkyFill.B * 0.28f * Mat.Diffuse.BaseColor.B * DiffWeight * Ao;

    }

    // 4. Specular / Clearcoat Reflection Ray (BVH4 exact reflection on vehicle paint, glass, rims, brakes!)
    if (AllowReflection && BounceDepth == 0u)
    {
        const float Rough = std::min(Mat.Specular.Roughness, (Mat.Clearcoat.CoatWeight > 0.1f) ? Mat.Clearcoat.CoatRoughness : 1.0f);
        const float CoatW = Mat.Clearcoat.CoatWeight;
        const float Metal = Mat.Specular.Metalness;
        if (Rough < 0.48f || CoatW > 0.1f || Metal > 0.4f)
        {
            const float NdotV = std::max(1.0e-3f, Mathematics::EvaluateDot(SmoothN, Wo));
            const float F0Die = std::pow((Mat.Specular.DielectricIOR - 1.0f) / (Mat.Specular.DielectricIOR + 1.0f), 2.0f);
            const float OneMinusC = 1.0f - NdotV;
            const float Schlick5 = OneMinusC * OneMinusC * OneMinusC * OneMinusC * OneMinusC;
            const Mathematics::LinearRGB F0 = {
                F0Die * (1.0f - Metal) + Mat.Diffuse.BaseColor.R * Metal,
                F0Die * (1.0f - Metal) + Mat.Diffuse.BaseColor.G * Metal,
                F0Die * (1.0f - Metal) + Mat.Diffuse.BaseColor.B * Metal,
            };
            Mathematics::LinearRGB Fresnel = {
                F0.R + (1.0f - F0.R) * Schlick5,
                F0.G + (1.0f - F0.G) * Schlick5,
                F0.B + (1.0f - F0.B) * Schlick5,
            };
            if (CoatW > 0.05f)
            {
                const float FCoat = 0.043f + 0.957f * Schlick5;
                Fresnel = {
                    std::max(Fresnel.R, CoatW * FCoat * 1.15f),
                    std::max(Fresnel.G, CoatW * FCoat * 1.15f),
                    std::max(Fresnel.B, CoatW * FCoat * 1.15f),
                };
            }

            const auto ReflDir = Mathematics::NormalizeAxis(Wo * -1.0f + SmoothN * (2.0f * Mathematics::EvaluateDot(SmoothN, Wo)));
            const SpatialAcceleration::RaySegmentQuery RQ { Pos + GeomN * 0.003f, 0.002f, ReflDir, 90.0f };
            const auto RHit = Sc.Tree.TraceClosestRay(RQ, false);
            Mathematics::LinearRGB Env {};
            if (RHit.HitConfirmed)
            {
                const auto& RTri = Sc.Tree.PrimitiveNormals[RHit.PrimitiveIndex];
                const float W0 = 1.0f - RHit.BarycentricU - RHit.BarycentricV;
                Mathematics::Axis3D RN = Mathematics::NormalizeAxis(
                    RTri.Normal0 * W0 + RTri.Normal1 * RHit.BarycentricU + RTri.Normal2 * RHit.BarycentricV,
                    RHit.GeometricNormal);
                Mathematics::Axis3D RGN = RHit.GeometricNormal;
                if (Mathematics::EvaluateDot(RGN, ReflDir) > 0.0f) RGN = RGN * -1.0f;
                if (Mathematics::EvaluateDot(RN, ReflDir) > 0.0f) RN = RN * -1.0f;
                Env = ShadeSurfaceDirect(
                    Sc,
                    RQ.Origin + ReflDir * RHit.HitDistanceMeters,
                    RN,
                    RGN,
                    ReflDir * -1.0f,
                    RHit.MaterialOrdinal,
                    PX,
                    PY,
                    SunDir,
                    SunIrrad,
                    SkyColor,
                    EmissiveBoost,
                    false,
                    false,
                    1u);
            }
            else
            {
                Env = EvaluateSkyBackdrop(ReflDir, SkyColor, SunDir, SunIrrad);
            }
            const float RoughFade = std::clamp(1.0f - Rough * 1.4f, 0.15f, 1.0f) * Ao;
            Out.R += Env.R * Fresnel.R * RoughFade;
            Out.G += Env.G * Fresnel.G * RoughFade;
            Out.B += Env.B * Fresnel.B * RoughFade;
        }
    }
    return Out;
}

// Exact BVH4 secondary visibility with direct lighting at the secondary surface.
// Includes sky escape and one diffuse surface bounce; not a surfel approximation.
[[nodiscard]] Mathematics::LinearRGB ShadeVehicleDiffuseGi(
    const SceneData& Sc, Mathematics::Axis3D P, Mathematics::Axis3D N,
    Mathematics::Axis3D GN, uint16_t Slot, uint32_t PX, uint32_t PY,
    Mathematics::Axis3D SunDir, Mathematics::LinearRGB SunI,
    Mathematics::LinearRGB Sky)
{
    auto Mat = Sc.MatReg.InspectAuthoredMaterial(Slot);
    if (Slot == Sc.FloorMat)
        Mat.Diffuse.BaseColor = EvaluateStudioFloorPattern(P, Mat.Diffuse.BaseColor);
    Mathematics::Axis3D T {}, B {};
    GeometrySpace::ConstructDuffOrthonormalBasis(N, T, B);
    const MaterialSpace::Scattering::SurfaceShadingBasis Basis { T, B, N };
    return IntegrateDeterministicDiffuseGi(
        Mat.Diffuse.BaseColor * ((1.0f - Mat.Specular.Metalness) * Mat.Diffuse.BaseWeight),
        32u, AvalancheKey32(PX * 1973u + PY * 9277u + 17u),
        [&](Mathematics::Axis3D LocalD) {
            const auto D = Basis.LocalToWorld(LocalD);
            const SpatialAcceleration::RaySegmentQuery Q { P + GN * 0.003f, 0.002f, D, 120.0f };
            const auto H = Sc.Tree.TraceClosestRay(Q, false);
            if (!H.HitConfirmed) return EvaluateSkyBackdrop(D, Sky, SunDir, SunI);
            const auto& TN = Sc.Tree.PrimitiveNormals[H.PrimitiveIndex];
            auto SN = Mathematics::NormalizeAxis(TN.Normal0 * (1.0f - H.BarycentricU - H.BarycentricV)
                + TN.Normal1 * H.BarycentricU + TN.Normal2 * H.BarycentricV, H.GeometricNormal);
            auto NG = H.GeometricNormal;
            if (Mathematics::EvaluateDot(NG, D) > 0) NG = NG * -1.0f;
            if (Mathematics::EvaluateDot(SN, D) > 0) SN = SN * -1.0f;
            return ShadeSurfaceDirect(Sc, Q.Origin + D * H.HitDistanceMeters, SN, NG,
                D * -1.0f, H.MaterialOrdinal, PX, PY, SunDir, SunI, Sky, 1.0f,
                true, false, 1u, false);
        });
}

[[nodiscard]] Mathematics::LinearRGB IntegrateDeterministicPathPixel(
    const SceneData&       Sc,
    Mathematics::Axis3D    CamOrigin,
    Mathematics::Axis3D    RayDir,
    uint32_t               PX,
    uint32_t               PY,
    uint32_t               Spp,
    Mathematics::Axis3D    SunDir,
    Mathematics::LinearRGB SunIrrad,
    Mathematics::LinearRGB SkyColor) noexcept
{
    const uint32_t PixKey = AvalancheKey32(PX * 747796405u + PY * 2891336453u + 1u);
    Mathematics::LinearRGB Sum {};

    for (uint32_t S = 0u; S < Spp; ++S)
    {
        Mathematics::Axis3D Origin = CamOrigin;
        Mathematics::Axis3D Dir    = RayDir;
        Mathematics::LinearRGB Throughput { 1.0f, 1.0f, 1.0f };
        Mathematics::LinearRGB SampleRad {};

        for (uint32_t Bounce = 0u; Bounce < 3u; ++Bounce)
        {
            const SpatialAcceleration::RaySegmentQuery RQ { Origin, 0.002f, Dir, 120.0f };
            const auto Hit = Sc.Tree.TraceClosestRay(RQ, false);
            if (!Hit.HitConfirmed)
            {
                SampleRad = SampleRad + MulRgb(Throughput, EvaluateSkyBackdrop(Dir, SkyColor, SunDir, SunIrrad));
                break;
            }

            const auto& TriN = Sc.Tree.PrimitiveNormals[Hit.PrimitiveIndex];
            const float W0 = 1.0f - Hit.BarycentricU - Hit.BarycentricV;
            Mathematics::Axis3D N = Mathematics::NormalizeAxis(
                TriN.Normal0 * W0 + TriN.Normal1 * Hit.BarycentricU + TriN.Normal2 * Hit.BarycentricV,
                Hit.GeometricNormal);
            Mathematics::Axis3D GN = Hit.GeometricNormal;
            if (Mathematics::EvaluateDot(GN, Dir) > 0.0f) GN = GN * -1.0f;
            if (Mathematics::EvaluateDot(N, Dir) > 0.0f)  N  = N * -1.0f;

            const Mathematics::Axis3D P  = Origin + Dir * Hit.HitDistanceMeters;
            const Mathematics::Axis3D Wo = Dir * -1.0f;
            const uint16_t MatSlot       = Hit.MaterialOrdinal;
            const auto& RawMat           = Sc.MatReg.InspectAuthoredMaterial(MatSlot);
            MaterialSpace::SurfaceMaterial Mat = RawMat;
            if (MatSlot == Sc.FloorMat)
            {
                Mat.Diffuse.BaseColor = EvaluateStudioFloorPattern(P, RawMat.Diffuse.BaseColor);
            }

            SampleRad = SampleRad + MulRgb(Throughput, Sc.MatEmission[MatSlot]);

            Mathematics::Axis3D T {}, B {};
            GeometrySpace::ConstructDuffOrthonormalBasis(N, T, B);
            const MaterialSpace::Scattering::SurfaceShadingBasis Basis { T, B, N };
            const Mathematics::Axis3D LocalWo = Basis.WorldToLocal(Wo);

            // Direct Solar + Softbox NEE with deterministic (0,2) Sobol
            const auto USun = Sobol02(S, PixKey, Bounce * 4u + 1u);
            {
                Mathematics::Axis3D ST {}, SB {};
                GeometrySpace::ConstructDuffOrthonormalBasis(SunDir, ST, SB);
                const float R = std::sqrt(USun.X) * 0.022f;
                const float A = Constants::TauFloat * USun.Y;
                const Mathematics::Axis3D LDir = Mathematics::NormalizeAxis(SunDir + ST * (R * std::cos(A)) + SB * (R * std::sin(A)));
                const float NdotL = Mathematics::EvaluateDot(N, LDir);
                if (NdotL > 1.0e-4f && !Sc.Tree.QueryRayOccluded({ P + GN * 0.003f, 0.002f, LDir, 80.0f }))
                {
                    const auto F = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(
                        Mat, Sc.MatLobeMask[MatSlot], LocalWo, Basis.WorldToLocal(LDir));
                    SampleRad = SampleRad + MulRgb(MulRgb(Throughput, F), SunIrrad) * NdotL;
                }
            }
            const auto UQuad = Sobol02(S, PixKey, Bounce * 4u + 2u);
            {
                const auto& Q = Sc.Softbox;
                const float Area = 4.0f * Q.HalfWidthMeters * Q.HalfHeightMeters;
                const Mathematics::Axis3D LP = Q.CenterWorld + Q.RightTangent * ((UQuad.X * 2.0f - 1.0f) * Q.HalfWidthMeters) +
                                               Q.UpBitangent * ((UQuad.Y * 2.0f - 1.0f) * Q.HalfHeightMeters);
                const Mathematics::Axis3D Delta = LP - P;
                const float DistSq = Mathematics::EvaluateLengthSquared(Delta);
                const float Dist   = std::sqrt(std::max(1.0e-5f, DistSq));
                const Mathematics::Axis3D LDir = Delta * (1.0f / Dist);
                const float CosR = Mathematics::EvaluateDot(N, LDir);
                const float CosL = std::max(0.0f, -LDir.Z);
                if (CosR > 1.0e-4f && CosL > 1.0e-4f &&
                    !Sc.Tree.QueryRayOccluded({ P + GN * 0.003f, 0.002f, LDir, Dist - 0.02f }))
                {
                    const auto F = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(
                        Mat, Sc.MatLobeMask[MatSlot], LocalWo, Basis.WorldToLocal(LDir));
                    SampleRad = SampleRad + MulRgb(MulRgb(Throughput, F), Q.EmittedRadiance) * ((CosR * CosL * Area) / DistSq);
                }
            }

            if (Bounce == 2u)
            {
                break;
            }

            // Deterministic Lobe Sample: Clearcoat/Specular VNDF vs Cosine Diffuse
            const auto UBsdf  = Sobol02(S, PixKey, Bounce * 4u + 3u);
            const auto ULobe  = Sobol02(S, PixKey, Bounce * 4u + 4u);
            const float Rough = std::min(Mat.Specular.Roughness, (Mat.Clearcoat.CoatWeight > 0.1f) ? Mat.Clearcoat.CoatRoughness : 1.0f);
            const float Metal = Mat.Specular.Metalness;
            const float SpecProb = std::clamp(0.18f + 0.65f * std::max(Metal, Mat.Clearcoat.CoatWeight), 0.15f, 0.85f);

            Mathematics::Axis3D NextDir {};
            Mathematics::LinearRGB NextWeight {};
            if (ULobe.X < SpecProb)
            {
                const auto LocalH = SampleSphericalCapGgxVndfLocal(LocalWo, Rough * Rough, UBsdf);
                const auto LocalWi = Mathematics::NormalizeAxis(LocalWo * -1.0f + LocalH * (2.0f * Mathematics::EvaluateDot(LocalWo, LocalH)));
                if (LocalWi.Z <= 1.0e-4f) break;
                NextDir = Basis.LocalToWorld(LocalWi);
                const float NdotV = std::max(1.0e-3f, LocalWo.Z);
                const float F0 = std::pow((Mat.Specular.DielectricIOR - 1.0f) / (Mat.Specular.DielectricIOR + 1.0f), 2.0f);
                const float S5 = std::pow(1.0f - NdotV, 5.0f);
                const float Coat = Mat.Clearcoat.CoatWeight;
                const Mathematics::LinearRGB F = {
                    std::max(Coat * (0.043f + 0.957f * S5), (F0 * (1.0f - Metal) + Mat.Diffuse.BaseColor.R * Metal) + (1.0f - F0) * S5),
                    std::max(Coat * (0.043f + 0.957f * S5), (F0 * (1.0f - Metal) + Mat.Diffuse.BaseColor.G * Metal) + (1.0f - F0) * S5),
                    std::max(Coat * (0.043f + 0.957f * S5), (F0 * (1.0f - Metal) + Mat.Diffuse.BaseColor.B * Metal) + (1.0f - F0) * S5),
                };
                NextWeight = F * (1.0f / SpecProb);
            }
            else
            {
                const auto LocalWi = SampleCosineHemiLocal(UBsdf);
                NextDir = Basis.LocalToWorld(LocalWi);
                NextWeight = Mat.Diffuse.BaseColor * ((1.0f - Metal) * Mat.Diffuse.BaseWeight / (1.0f - SpecProb));
            }
            Throughput = MulRgb(Throughput, NextWeight);
            Origin     = P + GN * 0.003f;
            Dir        = NextDir;
        }
        Sum = Sum + SampleRad;
    }
    return Sum * (1.0f / static_cast<float>(Spp));
}

[[nodiscard]] bool WritePpmImage(const std::filesystem::path& Path, uint32_t W, uint32_t H, const std::vector<uint8_t>& Rgb)
{
    std::ofstream Out(Path, std::ios::binary);
    if (!Out) return false;
    Out << "P6\n" << W << " " << H << "\n255\n";
    Out.write(reinterpret_cast<const char*>(Rgb.data()), static_cast<std::streamsize>(Rgb.size()));
    return true;
}

} // namespace

int main(int ArgCount, char** Args)
{
    const bool ShadowComparison = ArgCount > 1 && std::string(Args[1]) == "--shadow-methods";
    const bool MethodComparison = ShadowComparison || (ArgCount > 1 && std::string(Args[1]) == "--methods");
    const bool GiComparison = MethodComparison || (ArgCount > 1 && std::string(Args[1]) == "--gi");
    constexpr uint32_t W = 1280u;
    constexpr uint32_t H = 720u;

    const std::filesystem::path VehicleDir = "EngineContent/EnvironmentArchives/ControlVehicle";
    LoadedFct1Mesh BodyMesh {}, TyreMesh {}, RimMesh {};
    if (!LoadFct1FaceFile(VehicleDir / "ControlVehicleBody.face", BodyMesh) ||
        !LoadFct1FaceFile(VehicleDir / "ControlVehicleTyre.face", TyreMesh) ||
        !LoadFct1FaceFile(VehicleDir / "ControlVehicleRim.face", RimMesh))
    {
        std::fprintf(stderr, "Failed to load ControlVehicle .face streams.\n");
        return 1;
    }
    std::printf(
        "Loaded ControlVehicle: Body=%zu tris, Tyre=%zu tris, Rim=%zu tris\n",
        BodyMesh.Triangles.size(), TyreMesh.Triangles.size(), RimMesh.Triangles.size());

    SceneData Sc {};
    std::array<uint16_t, 16u> SlotToMat {};
    auto AddCourseMat = [&](uint32_t Slot, Mathematics::LinearRGB Col, float Rough, float Metal, float Coat = 0.0f, Mathematics::LinearRGB Emis = {}) -> uint16_t
    {
        MaterialSpace::SurfaceMaterial M {};
        M.Diffuse.BaseColor        = Col;
        M.Diffuse.BaseWeight       = 1.0f;
        M.Specular.Roughness       = Rough;
        M.Specular.Metalness       = Metal;
        M.Specular.DielectricIOR   = 1.52f;
        M.Clearcoat.CoatWeight     = Coat;
        M.Clearcoat.CoatRoughness  = 0.04f;
        M.Clearcoat.CoatIOR        = 1.54f;
        const uint16_t Ord = Sc.MatReg.RegisterMaterial(M);
        SlotToMat[Slot]    = Ord;
        while (Sc.MatEmission.size() <= Ord) Sc.MatEmission.emplace_back();
        Sc.MatEmission[Ord] = Emis;
        return Ord;
    };

    // Authentic ControlVehiclePalette (IndexedTriangleStreamInterchange.cpp)
    (void)AddCourseMat( 6u, { 0.680f, 0.800f, 0.000f }, 0.18f, 0.00f, 1.00f); // BodyCoat (clearcoated racing lime-yellow)
    (void)AddCourseMat( 7u, { 0.024f, 0.027f, 0.030f }, 0.04f, 0.00f, 0.90f); // VehicleGlass (gloss tinted automotive glass)
    (void)AddCourseMat( 8u, { 0.016f, 0.018f, 0.022f }, 0.42f, 0.00f, 0.00f); // VehiclePlastic
    (void)AddCourseMat( 9u, { 0.032f, 0.033f, 0.036f }, 0.56f, 0.00f, 0.00f); // Tyre & Aero Rim Dish
    (void)AddCourseMat(10u, { 0.060f, 0.240f, 0.720f }, 0.20f, 1.00f, 0.00f, { 0.000f, 0.080f, 0.280f }); // Anodized Blue Hub
    (void)AddCourseMat(11u, { 0.300f, 0.075f, 0.025f }, 0.29f, 0.82f, 0.00f); // Copper-Bronze Brake Rotor
    (void)AddCourseMat(12u, { 0.780f, 0.790f, 0.820f }, 0.36f, 0.35f, 0.00f); // VehicleTrim
    (void)AddCourseMat(13u, { 0.880f, 0.900f, 0.940f }, 0.06f, 0.00f, 0.90f, { 1.480f, 1.460f, 1.380f }); // Headlight (clearcoated LED bezel)
    (void)AddCourseMat(14u, { 0.800f, 0.040f, 0.030f }, 0.08f, 0.00f, 0.80f, { 2.250f, 0.075f, 0.030f }); // Taillight (rich ruby LED bar)
    (void)AddCourseMat(15u, { 0.750f, 0.750f, 0.750f }, 0.50f, 0.00f, 0.00f); // VehicleDefault

    auto AddStudioMat = [&](Mathematics::LinearRGB Col, float Rough, float Metal = 0.0f, Mathematics::LinearRGB Emis = {}) -> uint16_t
    {
        MaterialSpace::SurfaceMaterial M {};
        M.Diffuse.BaseColor      = Col;
        M.Diffuse.BaseWeight     = 1.0f;
        M.Specular.Roughness     = Rough;
        M.Specular.Metalness     = Metal;
        M.Specular.DielectricIOR = 1.50f;
        const uint16_t Ord = Sc.MatReg.RegisterMaterial(M);
        while (Sc.MatEmission.size() <= Ord) Sc.MatEmission.emplace_back();
        Sc.MatEmission[Ord] = Emis;
        return Ord;
    };

    Sc.FloorMat               = AddStudioMat({ 0.42f, 0.44f, 0.48f }, 0.34f, 0.0f);
    const uint16_t WallMat    = AddStudioMat({ 0.24f, 0.26f, 0.30f }, 0.82f, 0.0f);
    const uint16_t WallMid    = AddStudioMat({ 0.28f, 0.31f, 0.36f }, 0.82f, 0.0f);
    const uint16_t RibLight   = AddStudioMat({ 0.90f, 0.92f, 0.96f }, 0.90f, 0.0f, { 2.2f, 2.3f, 2.5f });
    Sc.SoftboxMat             = AddStudioMat({ 0.95f, 0.95f, 0.95f }, 0.90f, 0.0f, { 5.2f, 5.0f, 4.7f });

    // Build Lobe Masks
    Sc.MatLobeMask.resize(Sc.MatReg.QuerySlotCount());
    for (uint16_t S = 0u; S < Sc.MatReg.QuerySlotCount(); ++S)
    {
        Sc.MatLobeMask[S] = MaterialSpace::DetermineActiveLobeMask(Sc.MatReg.InspectAuthoredMaterial(S));
    }

    // Assemble Real Vehicle Triangles (Z-up: GroundOffset = 0.50855 - 0.0914 = 0.41715)
    std::vector<SpatialAcceleration::RayTrianglePrimitive>  Prims {};
    std::vector<SpatialAcceleration::TriangleSmoothNormals> Normals {};
    constexpr float GroundOffset = 0.50855f - 0.0914f;

    AppendTransformedFct1Mesh(BodyMesh, { 0.0f, 0.0f, GroundOffset }, false, SlotToMat, Prims, Normals);
    AppendProceduralRearWingZUp(Prims, Normals, SlotToMat);

    struct AxlePos { float X; float Y; bool RightSide; };
    constexpr std::array<AxlePos, 4u> Axles {{
        {  1.7274f,  1.0475f, false },
        {  1.7274f, -1.0475f, true  },
        { -1.6686f,  1.0475f, false },
        { -1.6686f, -1.0475f, true  },
    }};
    for (const auto& A : Axles)
    {
        const Mathematics::Axis3D HubPos { A.X, A.Y, 0.50855f };
        AppendTransformedFct1Mesh(TyreMesh, HubPos, A.RightSide, SlotToMat, Prims, Normals);
        AppendTransformedFct1Mesh(RimMesh,  HubPos, A.RightSide, SlotToMat, Prims, Normals);
    }

    // Studio Floor, Neutral Architectural Studio Bays + Vertical Diffuser Strips (crisp automotive reflections!) & Overhead Softbox
    AppendStudioBoxZUp(Prims, Normals, {   0.0f,   0.0f, -0.05f }, { 22.0f, 22.0f, 0.05f }, 0.0f, Sc.FloorMat);
    AppendStudioBoxZUp(Prims, Normals, { -12.5f,   0.0f,  5.00f }, {  0.15f, 20.0f, 5.10f }, 0.0f, WallMat);
    AppendStudioBoxZUp(Prims, Normals, {  12.5f,   0.0f,  5.00f }, {  0.15f, 20.0f, 5.10f }, 0.0f, WallMat);
    AppendStudioBoxZUp(Prims, Normals, {   0.0f,  13.5f,  5.00f }, { 20.0f,  0.15f, 5.10f }, 0.0f, WallMid);
    AppendStudioBoxZUp(Prims, Normals, {   0.0f, -13.5f,  5.00f }, { 20.0f,  0.15f, 5.10f }, 0.0f, WallMid);
    // Subtle studio wall vertical light panels for clearcoat specular streaks
    AppendStudioBoxZUp(Prims, Normals, {  -4.0f,  13.2f,  2.40f }, {  0.35f,  0.04f, 1.80f }, 0.0f, RibLight);
    AppendStudioBoxZUp(Prims, Normals, {   4.0f,  13.2f,  2.40f }, {  0.35f,  0.04f, 1.80f }, 0.0f, RibLight);
    AppendStudioBoxZUp(Prims, Normals, {  -4.0f, -13.2f,  2.40f }, {  0.35f,  0.04f, 1.80f }, 0.0f, RibLight);
    AppendStudioBoxZUp(Prims, Normals, {   4.0f, -13.2f,  2.40f }, {  0.35f,  0.04f, 1.80f }, 0.0f, RibLight);
    AppendStudioBoxZUp(Prims, Normals, {   0.0f,   0.0f,  4.45f }, {  2.80f,  1.40f, 0.04f }, 0.0f, Sc.SoftboxMat);

    Sc.Softbox.CenterWorld      = { 0.0f, 0.0f, 4.40f };
    Sc.Softbox.RightTangent     = { 1.0f, 0.0f, 0.0f };
    Sc.Softbox.UpBitangent      = { 0.0f, 1.0f, 0.0f };
    Sc.Softbox.HalfWidthMeters  = 2.80f;
    Sc.Softbox.HalfHeightMeters = 1.40f;
    Sc.Softbox.EmittedRadiance  = { 4.8f, 4.6f, 4.3f };

    std::printf("Building WideBoundingTree (BVH4) for %zu total triangles...\n", Prims.size());
    if (!Sc.Tree.BuildFromTrianglePrimitivesWithNormals(Prims, Normals))
    {
        std::fprintf(stderr, "Failed to build WideBoundingTree.\n");
        return 1;
    }
    std::printf("BVH4 built: %zu wide entries, depth=%u.\n", Sc.Tree.WideEntries.size(), Sc.Tree.MaxTreeDepth);

    const std::filesystem::path OutDir = ShadowComparison ? "Exhibits/ControlVehicleShadowMethods" : MethodComparison ? "Exhibits/ControlVehicleGiMethods" : GiComparison ? "Exhibits/ControlVehicleGiRenders" : "Exhibits/ControlVehicleRenders";
    std::filesystem::create_directories(OutDir);

    const Mathematics::Axis3D StdSun = Mathematics::NormalizeAxis({ 0.58f, 0.48f, 0.658f });
    const Mathematics::LinearRGB StdSunI { 3.5f, 3.35f, 3.10f };
    const Mathematics::LinearRGB StdSky  { 0.34f, 0.42f, 0.56f };

    StokesContourGiSequence StokesGi;
    SurfelCascadeGiSequence SurfelGi;
    SurfelOccluderField ShadowField;
    if (MethodComparison)
    {
        std::vector<Mathematics::LinearRGB> Reflectance(Sc.MatReg.QuerySlotCount());
        for (uint16_t Slot = 0; Slot < Reflectance.size(); ++Slot)
        {
            const auto& Material = Sc.MatReg.InspectAuthoredMaterial(Slot);
            Reflectance[Slot] = Material.Diffuse.BaseColor * (Material.Diffuse.BaseWeight *
                (1.0f - Material.Specular.Metalness));
        }
        const auto Start = std::chrono::steady_clock::now();
        StokesGi.Prepare(Sc.Tree, Sc.MatEmission, Reflectance, StdSun, StdSunI);
        std::printf("Stokes prepared: %zu emissive triangles, %zu diffuse patches, %.3f seconds\n",
            StokesGi.Sources.size(), StokesGi.Patches.size(),
            std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count());
        SurfelGi.Prepare(StokesGi, Sc.Tree);
        if (ShadowComparison) ShadowField.BuildFromTriangles(Sc.Tree);
        std::printf("Surfel cascades prepared: %zu radiant surfels, %zu + %zu probes, %.3f seconds total\n",
            SurfelGi.RadiantSurfels.size(), SurfelGi.Cascades[0].Probes.size(), SurfelGi.Cascades[1].Probes.size(),
            std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count());
    }

    std::vector<RenderRigConfig> Rigs = {
        { "01_hero_front_3q_openpbr",        "Hero Front 3/4 (OpenPBR + BVH4 Reflections)", {  5.40f,  3.65f, 1.82f }, {  0.05f,  0.00f, 0.78f }, 38.0f, StdSun, StdSunI, StdSky, 0u, 1.05f, 1.0f },
        { "02_hero_rear_3q_openpbr",         "Hero Rear 3/4 (RearWing + Taillights)",       { -5.50f, -3.65f, 1.88f }, { -0.20f,  0.00f, 0.80f }, 38.0f, StdSun, StdSunI, StdSky, 0u, 1.05f, 1.0f },
        { "03_side_profile_elevation",       "Long-Lens Side Elevation (Ride Height & Aero)", {  0.00f, 10.60f, 0.96f }, {  0.00f,  0.00f, 0.84f }, 28.0f, StdSun, StdSunI, StdSky, 0u, 1.05f, 1.0f },
        { "04_front_wheel_rim_brake_closeup","Macro Close-Up: Tyre, Wheel Dish & Blue Hub", {  2.85f,  2.35f, 0.68f }, {  1.72f,  1.00f, 0.51f }, 28.0f, Mathematics::NormalizeAxis({ 0.65f, 0.35f, 0.674f }), StdSunI, StdSky, 0u, 1.15f, 1.2f },
        { "05_rear_wing_taillight_closeup",  "Close-Up: Procedural RearWing & Taillights",  { -4.25f,  2.15f, 1.60f }, { -2.45f,  0.00f, 1.00f }, 32.0f, StdSun, StdSunI, StdSky, 0u, 1.08f, 1.1f },
        { "06_overhead_3q_studio_footprint", "High 3/4 Overhead (Roof, Glass & Shadow)",    {  4.65f,  4.40f, 4.35f }, {  0.00f,  0.00f, 0.72f }, 40.0f, StdSun, StdSunI, StdSky, 0u, 1.05f, 1.0f },
        { "07_low_front_headlight_dramatic", "Low Ground-Level Front Quarter",              {  5.35f, -3.15f, 0.64f }, {  0.10f,  0.00f, 0.74f }, 38.0f, Mathematics::NormalizeAxis({ 0.45f, -0.58f, 0.678f }), StdSunI, StdSky, 0u, 1.08f, 1.0f },
        { "08_matcap_studio_clay_surface",   "Studio Ceramic Clay MatCap Form Inspection",  {  5.40f,  3.65f, 1.82f }, {  0.05f,  0.00f, 0.78f }, 38.0f, StdSun, StdSunI, StdSky, 1u, 1.05f, 1.0f },
        { "09_deterministic_pt_32spp",       "32-SPP Deterministic (0,2) Sobol + GGX VNDF", {  5.10f,  3.45f, 1.72f }, {  0.05f,  0.00f, 0.76f }, 38.0f, StdSun, StdSunI, StdSky, 2u, 1.10f, 1.0f },
        { "10_dusk_emissive_showcase",       "Twilight Dusk Showcase (Emissive Lamps/Hubs)",{  5.15f,  3.35f, 1.55f }, {  0.05f,  0.00f, 0.76f }, 38.0f, Mathematics::NormalizeAxis({ -0.82f, -0.35f, 0.452f }), { 1.45f, 0.82f, 0.48f }, { 0.09f, 0.14f, 0.28f }, 0u, 1.25f, 2.0f },
    };

    if (GiComparison)
    {
        const auto Front = Rigs[0];
        const auto Rear = Rigs[1];
        Rigs.clear();
        for (uint32_t View = 0u; View < 2u; ++View)
        {
            const auto Base = View == 0u ? Front : Rear;
            const std::string Prefix = View == 0u ? "front_" : "rear_";
            for (uint32_t Mode : (ShadowComparison ? std::vector<uint32_t>{8u, 9u, 10u, 11u} : MethodComparison ? std::vector<uint32_t>{3u, 6u, 7u, 2u} : std::vector<uint32_t>{3u, 4u, 5u, 2u}))
            {
                auto Rig = Base;
                Rig.Mode = Mode;
                Rig.OutputName = Prefix + (Mode == 8u ? "bvh4_shadow_surfel_gi" : Mode == 9u ? "surfel_shadow_surfel_gi" : Mode == 10u ? "bvh4_visibility" : Mode == 11u ? "surfel_visibility" : Mode == 6u ? "stokes_contour_gi" : Mode == 7u ? "surfel_cascade_gi" : Mode == 3u ? "direct_only" : Mode == 4u ? "diffuse_gi_32spp" : Mode == 5u ? "indirect_only_32spp" : "path_trace_64spp");
                Rig.Label = Rig.OutputName;
                if (MethodComparison) Rig.SkyColor = {};
                Rigs.push_back(Rig);
            }
        }
    }

    const unsigned NumThreads = std::max(1u, std::thread::hardware_concurrency());

    for (std::size_t RIdx = 0u; RIdx < Rigs.size(); ++RIdx)
    {
        const auto& Rig = Rigs[RIdx];
        PerspectiveCameraBasis Cam {};
        Cam.CameraOrigin       = Rig.CamPos;
        Cam.TargetPoint        = Rig.CamTarget;
        Cam.WorldUpAxis        = { 0.0f, 0.0f, 1.0f };
        Cam.VerticalFovRadians = Rig.FovDeg * Constants::DegToRadFloat;
        Cam.ViewportWidthPx    = W;
        Cam.ViewportHeightPx   = H;
        const OrthonormalCameraBasis Basis = ComputeCameraBasis(Cam);

        std::vector<uint8_t> Rgb(static_cast<std::size_t>(W) * H * 3u);
        std::vector<std::thread> Workers;
        Workers.reserve(NumThreads);

        for (unsigned T = 0u; T < NumThreads; ++T)
        {
            Workers.emplace_back([&, T]()
            {
                for (uint32_t PY = T; PY < H; PY += NumThreads)
                {
                    const float NdcY = 1.0f - ((static_cast<float>(PY) + 0.5f) / static_cast<float>(H)) * 2.0f;
                    for (uint32_t PX = 0u; PX < W; ++PX)
                    {
                        const float NdcX = ((static_cast<float>(PX) + 0.5f) / static_cast<float>(W)) * 2.0f - 1.0f;
                        const Mathematics::Axis3D RayDir = Mathematics::NormalizeAxis(
                            Basis.ForwardAxis +
                            Basis.RightAxis * (NdcX * Basis.TanHalfFovX) +
                            Basis.UpAxis    * (NdcY * Basis.TanHalfFovY));

                        Mathematics::LinearRGB Hdr {};
                        if (Rig.Mode == 2u)
                        {
                            Hdr = IntegrateDeterministicPathPixel(
                                Sc, Basis.CameraOrigin, RayDir, PX, PY, GiComparison ? 64u : 32u,
                                Rig.SunDir, Rig.SunIrrad, Rig.SkyColor);
                        }
                        else
                        {
                            const SpatialAcceleration::RaySegmentQuery PQ { Basis.CameraOrigin, 0.02f, RayDir, 150.0f };
                            const auto Hit = Sc.Tree.TraceClosestRay(PQ, false);
                            if (!Hit.HitConfirmed)
                            {
                                Hdr = (Rig.Mode == 1u || Rig.Mode == 5u)
                                    ? Mathematics::LinearRGB{ 0.14f, 0.16f, 0.19f }
                                    : EvaluateSkyBackdrop(RayDir, Rig.SkyColor, Rig.SunDir, Rig.SunIrrad);
                            }
                            else
                            {
                                const auto& TriN = Sc.Tree.PrimitiveNormals[Hit.PrimitiveIndex];
                                const float W0 = 1.0f - Hit.BarycentricU - Hit.BarycentricV;
                                Mathematics::Axis3D N = Mathematics::NormalizeAxis(
                                    TriN.Normal0 * W0 + TriN.Normal1 * Hit.BarycentricU + TriN.Normal2 * Hit.BarycentricV,
                                    Hit.GeometricNormal);
                                Mathematics::Axis3D GN = Hit.GeometricNormal;
                                if (Mathematics::EvaluateDot(GN, RayDir) > 0.0f) GN = GN * -1.0f;
                                if (Mathematics::EvaluateDot(N, RayDir) > 0.0f)  N  = N * -1.0f;
                                const Mathematics::Axis3D HitPos = Basis.CameraOrigin + RayDir * Hit.HitDistanceMeters;

                                if (Rig.Mode == 1u)
                                {
                                    // Studio Clay MatCap shading (view-space normal sphere + subtle rim & contact shadow)
                                    const float VX = Mathematics::EvaluateDot(N, Basis.RightAxis);
                                    const float VY = Mathematics::EvaluateDot(N, Basis.UpAxis);
                                    const float VZ = std::max(0.0f, Mathematics::EvaluateDot(N, Basis.ForwardAxis * -1.0f));
                                    const float Key = std::clamp(0.32f + 0.58f * (0.45f * VX + 0.70f * VY + 0.55f * VZ), 0.08f, 1.15f);
                                    const float Rim = std::pow(1.0f - VZ, 2.4f) * 0.28f;
                                    const bool IsFloor = (Hit.MaterialOrdinal == Sc.FloorMat);
                                    const Mathematics::LinearRGB Clay = IsFloor
                                        ? Mathematics::LinearRGB{ 0.34f, 0.36f, 0.39f }
                                        : Mathematics::LinearRGB{ 0.72f, 0.68f, 0.62f };
                                    float SoftVis = 1.0f;
                                    if (Sc.Tree.QueryRayOccluded({ HitPos + GN * 0.003f, 0.002f, Rig.SunDir, 60.0f }))
                                    {
                                        SoftVis = 0.42f;
                                    }
                                    Hdr = Clay * (Key * SoftVis) + Mathematics::LinearRGB{ Rim, Rim, Rim };
                                }
                                else
                                {
                                    float ShadowOverride = -1.0f;
                                    if (ShadowComparison && (Rig.Mode == 9u || Rig.Mode == 11u))
                                        ShadowOverride = ShadowField.EvaluateSolarVisibility(Sc.Tree, HitPos, GN, Rig.SunDir, 0.016f);
                                    Hdr = ShadeSurfaceDirect(
                                        Sc, HitPos, N, GN, RayDir * -1.0f, Hit.MaterialOrdinal, PX, PY,
                                        Rig.SunDir, Rig.SunIrrad, Rig.SkyColor, Rig.EmissiveBoost,
                                        true, !GiComparison, 0u, !GiComparison, !MethodComparison, ShadowOverride);
                                    if (MethodComparison && (Rig.Mode == 6u || Rig.Mode == 7u || Rig.Mode == 8u || Rig.Mode == 9u))
                                    {
                                        const auto Irradiance = Rig.Mode == 6u
                                            ? StokesGi.EvaluateIrradianceOverPi(HitPos, N)
                                            : SurfelGi.EvaluateIrradianceOverPi(HitPos, N);
                                        const auto& Material = Sc.MatReg.InspectAuthoredMaterial(Hit.MaterialOrdinal);
                                        auto Albedo = Material.Diffuse.BaseColor;
                                        if (Hit.MaterialOrdinal == Sc.FloorMat)
                                            Albedo = EvaluateStudioFloorPattern(HitPos, Albedo);
                                        Hdr = Hdr + MulRgb(Albedo, Irradiance) *
                                            ((1.0f - Material.Specular.Metalness) * Material.Diffuse.BaseWeight);
                                    }
                                    if (ShadowComparison && Rig.Mode >= 10u)
                                    {
                                        float Visibility = ShadowOverride;
                                        if (Rig.Mode == 10u)
                                        {
                                            Visibility = 0.0f;
                                            Mathematics::Axis3D ST {}, SB {};
                                            GeometrySpace::ConstructDuffOrthonormalBasis(Rig.SunDir, ST, SB);
                                            for (uint32_t K = 0; K < 16; ++K)
                                            {
                                                const float Radius = std::sqrt((K+0.5f)/16) * 0.016f;
                                                const float Angle = K * 2.39996323f;
                                                const auto D = Mathematics::NormalizeAxis(Rig.SunDir +
                                                    ST * (Radius*std::cos(Angle)) + SB * (Radius*std::sin(Angle)));
                                                if (!Sc.Tree.QueryRayOccluded({ HitPos + GN*0.006f, 0.002f, D, 80.0f }))
                                                    Visibility += 1.0f/16;
                                            }
                                            if (Mathematics::EvaluateDot(GN,Rig.SunDir) <= 0) Visibility = 0;
                                        }
                                        // Encode scalar visibility directly; bypass ACES below.
                                        Hdr = { Visibility, Visibility, Visibility };
                                    }
                                    if (GiComparison && (Rig.Mode == 4u || Rig.Mode == 5u))
                                    {
                                        const auto Gi = ShadeVehicleDiffuseGi(Sc, HitPos, N, GN,
                                            Hit.MaterialOrdinal, PX, PY, Rig.SunDir, Rig.SunIrrad, Rig.SkyColor);
                                        Hdr = Rig.Mode == 5u ? Gi : Hdr + Gi;
                                    }
                                }
                            }
                        }

                        const auto Tone = ShadowComparison && Rig.Mode >= 10u ? Hdr : ApplyAcesFilmic(Hdr, Rig.Exposure);
                        const std::size_t Off = (static_cast<std::size_t>(PY) * W + PX) * 3u;
                        Rgb[Off + 0u] = ToSrgb8(Tone.R);
                        Rgb[Off + 1u] = ToSrgb8(Tone.G);
                        Rgb[Off + 2u] = ToSrgb8(Tone.B);
                    }
                }
            });
        }
        for (auto& Wk : Workers) Wk.join();

        const auto PpmPath = OutDir / (Rig.OutputName + ".ppm");
        if (!WritePpmImage(PpmPath, W, H, Rgb)) return 2;
        std::printf("[%2zu/%zu] Rendered %s (%ux%u) -> %s\n", RIdx + 1u, Rigs.size(), Rig.Label.c_str(), W, H, PpmPath.c_str());
    }

    return 0;
}
