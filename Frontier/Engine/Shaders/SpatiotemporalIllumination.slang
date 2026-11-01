//==========================================================================================================================================
//                                            SPATIOTEMPORAL ILLUMINATION SHADER
//==========================================================================================================================================
// 🧩 Unified Dual-Option Vulkan 1.3 Illumination Pipeline (Hardware Raster + Hybrid SDF-GI / Raytraced Reflections + Full Raytracing Option):
//    0. SolveSurfaceRadiosityMain      : Multi-bounce GPU Surface Card Jacobi radiosity solver
//    1. SurfaceRaster + ForwardRaster  : Hardware triangle Reversed-Z visibility + ddx/ddy mipmaps & flake AA + Snell glazing (SurfaceRaster.slang)
//    2. ShadowRaster                   : Hardware 3-cascade D32_SFLOAT solar depth rasterizer (ShadowRaster.slang)
//    3. EvaluateDirectRadianceMain     : Deterministic ShadowRaster PCF + Karis area softboxes (zero DI noise!) | Option 2: Full-RT RIS + BVH Shadow
//    4. TraceIndirectDiffuseMain       : Half-Res (TraceScale=2, 1/4 px) 1-ray Hardware Sampler3D SDF Clipmaps + SurfaceCards | Option 2: Full-RT BVH GI
//    5. TraceSpecularReflectionsMain   : Raytraced Heitz VNDF CWBVH Reflections + SurfaceCards (both modes; full ray-shadowed in Option 2)
//    6. IntegrateTemporalVarianceMain  : SVGF VarianceTemporal (motion-reprojected 2x2 moments mu1/mu2 + 3x3 disocclusion bootstrap)
//    7. FilterSpatiotemporalAtrousMain : SVGF VarianceWavelet 5x5 B3-spline + VarianceUpsample joint bilateral half-res GI upsample + ACES

#include "BoundingVolumeSearch.slang"
#include "OctetVolumeSearch.slang"
#include "VolumeDistanceSampling.slang"
#include "RangeDistanceSampling.slang"
#include "SurfaceScatteringEvaluation.slang"
#include "SurfaceRadiositySampling.slang"
#include "LuminousFacetSampling.slang"
#include "DistanceTraversal.slang"
#include "DirectRadiance.slang"
#include "ReflectionTraversal.slang"

[[vk::binding(0, 0)]]  StructuredBuffer<float4>   FrameHeader;
[[vk::binding(1, 0)]]  StructuredBuffer<float4>   BvhOrTopCells;
[[vk::binding(2, 0)]]  StructuredBuffer<float4>   BvhTriangles;
[[vk::binding(3, 0)]]  StructuredBuffer<float4>   MaterialTable;
[[vk::binding(4, 0)]]  StructuredBuffer<float4>   DistanceOrRangeTable;
[[vk::binding(5, 0)]]  StructuredBuffer<float4>   OctetInstances;
[[vk::binding(6, 0)]]  StructuredBuffer<float4>   OctetBlocks;
[[vk::binding(7, 0)]]  StructuredBuffer<float4>   LuminousFacets;
[[vk::binding(8, 0)]]  RWStructuredBuffer<float4> SurfaceCardPack;
[[vk::binding(9, 0)]]  RWStructuredBuffer<float4> DirectReservoirCurr;
[[vk::binding(10, 0)]] RWStructuredBuffer<float4> DirectReservoirPrev;
[[vk::binding(11, 0)]] Sampler2DArray             MaterialMaps;
[[vk::binding(12, 0)]] Sampler3D                  SdfClipmapVolume;
[[vk::binding(13, 0)]] Sampler2DArray             ShadowDepthArray;

// TemporalHistoryPrev / TemporalHistoryCurr (6 layers x rgba16f, double-buffered across frames -> 100% race-free!):
//   0: PriorDiffSignal  {IntegratedDiff.rgb, DiffSpread}  (at GI TraceExtent)
//   1: PriorDiffMoments {MeanLuma, MeanLumaSq, TallyN, SceneAo}
//   2: PriorDiffGuide   {OctNormal.xy, HitT, ClassIdPlusOne}
//   3: PriorSpecSignal  {IntegratedSpec.rgb, SpecSpread}  (at Full ScreenExtent)
//   4: PriorSpecMoments {MeanLuma, MeanLumaSq, TallyN, Roughness}
//   5: PriorSpecGuide   {OctNormal.xy, HitT, ClassIdPlusOne}
[[vk::binding(14, 0), vk::image_format("rgba16f")]] RWTexture2DArray<float4> TemporalHistoryPrev;
[[vk::binding(15, 0), vk::image_format("rgba16f")]] RWTexture2DArray<float4> TemporalHistoryCurr;

// SurfaceAndWorkAtlas layers (14 x rgba16f):
//   0..5 : G-Buffer (written by Hardware SurfaceRaster + ForwardRaster, or TracePrimarySurfaceMain compute fallback)
//   6    : DirectRadiance (.rgb = linear direct diffuse+specular radiance, .a = primary solar shadow visibility [0..1])
//   7    : Raw Diffuse GI (.rgb = demodulated diffuse irradiance at TraceExtent, .a = SDF AO)
//   8    : Raw Specular Reflection (.rgb = specular reflection radiance at ScreenExtent, .a = HitFlag: >0 traced, <0 rough fallback)
//   9, 10: Diffuse GI Wavelet Ping-Pong (.rgb = filtered GI, .a = variance) at TraceExtent
//  11, 12: Specular Reflection Wavelet Ping-Pong (.rgb = filtered Spec, .a = variance) at ScreenExtent
//  13    : Current Half-Res GI Guide {OctNormal.xy, HitT, ClassIdPlusOne}
[[vk::binding(16, 0), vk::image_format("rgba16f")]] RWTexture2DArray<float4> SurfaceAndWorkAtlas;
[[vk::binding(17, 0), vk::image_format("rgba16f")]] RWTexture2DArray<float4> OutputImage;
[[vk::binding(18, 0), vk::image_format("rgba8")]]   RWTexture2D<float4>      ViewportDisplayTarget;
// ReSTIR GI reservoirs (ReservoirRecords.wgsl parity, 32 B/record as 8 x uint at TraceExtent):
//   [0..2] asuint(SamplePosition.xyz)  [3] packed octahedral normal (2x16 SNORM)
//   [4] packed half radiance.rg        [5] packed half (radiance.b, identity: 0 = sky, 1 = surface hit)
//   [6] asuint(W)                      [7] asuint(M tally)
[[vk::binding(19, 0)]] RWStructuredBuffer<uint> GiReservoirCurr;
[[vk::binding(20, 0)]] RWStructuredBuffer<uint> GiReservoirPrev;
// Display TAA history (DisplayAntialias.wgsl parity): prior / current resolved linear HDR frame
[[vk::binding(21, 0), vk::image_format("rgba16f")]] RWTexture2DArray<float4> DisplayHistoryPrev;
[[vk::binding(22, 0), vk::image_format("rgba16f")]] RWTexture2DArray<float4> DisplayHistoryCurr;

struct SpatiotemporalPush
{
    uint4  Counts; // {Width, Height, FrameOrParity, FinalIterFlag}
    float4 Params; // {ExposureScale, AtrousStepSize, ColourSigma, UseTemporalHistory}
};

[[vk::push_constant]] ConstantBuffer<SpatiotemporalPush> Push;

uint ResolveLuminaireBaseSlot()
{
    return max(20u, uint(FrameHeader[19].w + 0.5f));
}

bool IsFullRaytracingEnabled()
{
    return (FrameHeader[18].z > 0.5f);
}

uint ResolveGiTraceScale()
{
    return max(1u, uint(FrameHeader[18].w + 0.5f));
}

//------------------------------------------------------------------------------------------------------------------------
// DETERMINISTIC PRNG, OCTAHEDRAL ENCODING, & ACES TONEMAPPING
//------------------------------------------------------------------------------------------------------------------------

uint AdvancePcg32(inout uint Rng)
{
    uint Prev = Rng * 747796405u + 2891336453u;
    uint Word = ((Prev >> ((Prev >> 28u) + 4u)) ^ Prev) * 277803737u;
    Rng       = Prev;
    return (Word >> 22u) ^ Word;
}

float SampleUniform01(inout uint Rng)
{
    return float(AdvancePcg32(Rng) & 0x00FFFFFFu) * (1.0f / 16777216.0f);
}

uint SeedPixelStream(uint2 Pixel, uint FrameIdx, uint PassSalt)
{
    uint Seed = (Pixel.x * 1973u) ^ (Pixel.y * 9277u) ^ (FrameIdx * 26699u) ^ (PassSalt * 31847u) ^ 0x9E3779B9u;
    AdvancePcg32(Seed);
    return Seed;
}

float2 EncodeOctahedral(float3 Dir)
{
    float  InvL1 = 1.0f / max(1.0e-6f, abs(Dir.x) + abs(Dir.y) + abs(Dir.z));
    float2 P     = Dir.xy * InvL1;
    if (Dir.z < 0.0f)
    {
        float2 SignNotZero = float2((P.x >= 0.0f) ? 1.0f : -1.0f, (P.y >= 0.0f) ? 1.0f : -1.0f);
        P                  = (1.0f - abs(P.yx)) * SignNotZero;
    }
    return P;
}

float3 DecodeOctahedral(float2 E)
{
    float3 V = float3(E.x, E.y, 1.0f - abs(E.x) - abs(E.y));
    if (V.z < 0.0f)
    {
        float2 SignNotZero = float2((V.x >= 0.0f) ? 1.0f : -1.0f, (V.y >= 0.0f) ? 1.0f : -1.0f);
        V.xy               = (1.0f - abs(V.yx)) * SignNotZero;
    }
    return normalize(V);
}

float Luminance709(float3 Rgb)
{
    return dot(max(Rgb, float3(0.0f)), float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ApplyAcesAndSrgbOetf(float3 LinearHdr, float ExposureScale)
{
    float3 X      = max(float3(0.0f), LinearHdr * ExposureScale);
    float3 Mapped = saturate((X * (2.51f * X + 0.03f)) / max(X * (2.43f * X + 0.59f) + 0.14f, float3(1.0e-5f)));
    float3 Lo     = Mapped * 12.92f;
    float3 Hi     = 1.055f * pow(max(Mapped, float3(1.0e-6f)), float3(1.0f / 2.4f)) - 0.055f;
    return select(Mapped <= float3(0.0031308f), Lo, Hi);
}

float3 ReconstructPrimaryRayDirection(uint2 Px, uint Width, uint Height)
{
    float2 Uv       = (float2(Px) + 0.5f) / float2(float(Width), float(Height));
    float2 Ndc      = float2(2.0f * Uv.x - 1.0f, 1.0f - 2.0f * Uv.y);
    float  TanHalfV = FrameHeader[0].w;
    float3 CamRight = FrameHeader[1].xyz;
    float  TanHalfH = FrameHeader[1].w;
    float3 CamUp    = FrameHeader[2].xyz;
    float3 CamFwd   = FrameHeader[3].xyz;
    return normalize(CamFwd + Ndc.x * TanHalfH * CamRight + Ndc.y * TanHalfV * CamUp);
}

//------------------------------------------------------------------------------------------------------------------------
// COUPLED PLANETARY SKY, VOLUMETRIC CLOUDS, DUAL MOONS, & EXPONENTIAL HEIGHT FOG
//------------------------------------------------------------------------------------------------------------------------

float HashNoise3D(float3 P)
{
    float3 I = floor(P);
    float3 F = frac(P);
    F        = F * F * (3.0f - 2.0f * F);
    float  N = dot(I, float3(17.0f, 59.0f, 113.0f));
    float4 A = frac(sin(N + float4(0.0f, 17.0f, 59.0f, 76.0f)) * 43758.5453f);
    float4 B = frac(sin(N + 113.0f + float4(0.0f, 17.0f, 59.0f, 76.0f)) * 43758.5453f);
    float4 C = lerp(A, B, F.z);
    float2 D = lerp(C.xy, C.zw, F.y);
    return lerp(D.x, D.y, F.x);
}

// Physical Rayleigh + Mie + ozone sky (AtmosphereScattering.wgsl / CelestialSky.wgsl parity, analytic form of the
// Sky-View LUT): Kasten-Young air mass, single scattering + multi-scatter ambient boost, limb-darkened solar disc,
// aureole and twilight band. Config zenith luminance acts as an overall sky gain so day/dusk presets keep working.
float ResolveAirMass(float CosZenith)
{
    float Deg = clamp(acos(clamp(CosZenith, -1.0f, 1.0f)) * 57.29577951f, 0.0f, 93.0f);
    return clamp(1.0f / (CosZenith + 0.50572f * pow(max(96.07995f - Deg, 1.0f), -1.6364f)), 1.0f, 38.0f);
}

float3 EvaluatePhysicalSky(float3 RayDir, float3 SunDir, float3 SunRad)
{
    // Earth-like sea-level scattering coefficients [/km] (Bruneton/Hillaire values used by AtmosphereScattering.wgsl)
    const float3 BetaRayleigh = float3(5.802e-3f, 13.558e-3f, 33.10e-3f);
    const float  BetaMie      = 3.996e-3f;
    const float3 BetaOzone    = float3(0.65e-3f, 1.881e-3f, 0.085e-3f);
    const float3 TauVertical  = BetaRayleigh * 8.0f + float3(BetaMie * 1.2f) + BetaOzone * 12.0f;

    float  MuView   = RayDir.y;
    float  MuSun    = SunDir.y;
    float  CosAngle = dot(RayDir, SunDir);
    float  AmView   = ResolveAirMass(max(MuView, -0.05f));
    float  AmSun    = ResolveAirMass(MuSun);
    float3 TransView = exp(-TauVertical * AmView);
    float3 TransSun  = exp(-TauVertical * AmSun);

    float PhaseRayleigh = 0.0596831f * (1.0f + CosAngle * CosAngle); // 3/(16 pi) (1 + cos^2)
    float G             = 0.76f;
    float PhaseMie      = (1.0f - G * G) / (12.5663706f * pow(max(1.0f + G * G - 2.0f * G * CosAngle, 1.0e-4f), 1.5f));

    float3 BetaScatter = BetaRayleigh + float3(BetaMie);
    float3 ScatterMix  = (BetaRayleigh * PhaseRayleigh + float3(BetaMie * PhaseMie)) / max(BetaScatter, float3(1.0e-6f));
    float  MultiBoost  = 1.0f + 0.45f * (1.0f - saturate(MuSun + 0.15f)) + 0.25f * (1.0f - saturate(MuView));
    float3 Sky         = SunRad * 13.0f * ScatterMix * (1.0f - TransView) * TransSun * MultiBoost;

    // Twilight band (sun below horizon): warm/violet glow hugging the horizon toward the sun azimuth
    float Twilight = saturate(1.0f - abs(MuSun + 0.045f) * 7.0f);
    Sky += SunRad * float3(0.85f, 0.32f, 0.42f) * Twilight * exp(-abs(MuView) * 9.0f) * pow(saturate(CosAngle), 3.0f) * 0.22f;

    // Aureole + limb-darkened solar disc (atmosphere-attenuated)
    Sky += SunRad * TransSun * pow(saturate(CosAngle), 350.0f) * 0.55f;
    if (CosAngle > 0.99930f)
    {
        float DiscT  = saturate((CosAngle - 0.99930f) / (1.0f - 0.99930f));
        float Limb   = 0.62f + 0.38f * pow(DiscT, 0.6f); // limb darkening toward the solar edge
        Sky         += SunRad * TransSun * 3.5f * Limb * smoothstep(0.99930f, 0.99955f, CosAngle);
    }
    return max(Sky, float3(0.0f));
}

float3 EvaluateCoupledEnvironmentSky(float3 RayDir)
{
    float3 Zenith     = FrameHeader[4].xyz;
    float3 Horizon    = FrameHeader[5].xyz;
    float3 GroundTint = FrameHeader[6].xyz;
    uint   LumCount   = uint(FrameHeader[3].w + 0.5f);
    uint   LumBase    = ResolveLuminaireBaseSlot();

    float3 PrimarySunDir = float3(0.35f, 0.85f, 0.38f);
    float3 PrimarySunRad = float3(0.0f);
    for (uint L = 0u; L < LumCount; ++L)
    {
        uint   Slot = LumBase + L * 4u;
        float4 L0   = FrameHeader[Slot + 0u];
        if (uint(L0.w + 0.5f) == 0u)
        {
            PrimarySunDir = normalize(L0.xyz);
            PrimarySunRad = FrameHeader[Slot + 3u].xyz;
            break;
        }
    }

    float  SkyGain = clamp(Luminance709(Zenith) / 0.27f, 0.22f, 4.0f);
    float3 Sky     = EvaluatePhysicalSky(RayDir, PrimarySunDir, PrimarySunRad) * SkyGain;
    if (RayDir.y < 0.0f)
    {
        Sky = lerp(Sky, GroundTint * (0.35f + 0.65f * saturate(PrimarySunDir.y + 0.25f)), saturate(-RayDir.y * 3.0f));
    }

    float4 CloudMoonCfg = FrameHeader[14];
    if (CloudMoonCfg.w >= 0.0f && RayDir.y > -0.02f)
    {
        float Veil = saturate(1.0f - CloudMoonCfg.w * saturate(PrimarySunDir.y * 1.4f));
        [unroll]
        for (uint M = 0u; M < 2u; ++M)
        {
            float4 MoonSpec = FrameHeader[15u + M];
            float3 MoonDir  = normalize(MoonSpec.xyz);
            float  CosRad   = cos(max(0.004f, MoonSpec.w));
            float  CosRay   = dot(RayDir, MoonDir);
            if (CosRay >= CosRad)
            {
                float3 DiskNorm = normalize(RayDir - MoonDir * CosRay * 0.985f + MoonDir * 0.15f);
                float  Phase    = 0.12f + 0.88f * saturate(dot(DiskNorm, PrimarySunDir));
                float3 AlbedoM  = (M == 0u) ? float3(0.76f, 0.78f, 0.84f) : float3(0.84f, 0.73f, 0.65f);
                float  Limb     = smoothstep(CosRad, min(0.99999f, CosRad + 0.0005f), CosRay);
                Sky             = lerp(Sky, Sky * (1.0f - 0.65f * Veil) + AlbedoM * Phase * (1.6f * Veil + 0.25f), Limb);
            }
        }
    }

    if (CloudMoonCfg.x > 0.01f && RayDir.y > 0.035f)
    {
        float  Coverage  = saturate(CloudMoonCfg.x);
        float  DensityS  = max(0.0f, CloudMoonCfg.y);
        float  BottomM   = max(200.0f, CloudMoonCfg.z);
        float  SlabDist  = BottomM / max(0.06f, RayDir.y);
        float3 SamplePos = (FrameHeader[0].xyz + RayDir * SlabDist) * 0.00095f;
        float  Fbm       = 0.55f * HashNoise3D(SamplePos)
                         + 0.30f * HashNoise3D(SamplePos * 2.13f + 4.7f)
                         + 0.15f * HashNoise3D(SamplePos * 4.41f + 11.3f);
        float  Billow    = saturate((Fbm - (1.0f - Coverage * 0.88f)) / max(0.12f, Coverage * 0.55f));
        float  OptDepth  = Billow * Billow * DensityS * 2.8f;
        float  Trans     = exp(-OptDepth);
        float  Powder    = 1.0f - exp(-OptDepth * 2.0f);
        float  SunPhase  = 0.35f + 0.65f * pow(saturate(dot(RayDir, PrimarySunDir)), 6.0f);
        float3 CloudLit  = Zenith * 0.55f + (PrimarySunRad * 0.22f + Horizon * 0.65f) * SunPhase * (0.45f + 0.55f * Powder);
        Sky              = Sky * Trans + CloudLit * (1.0f - Trans);
    }

    return max(Sky, float3(0.0f));
}

float3 ApplyCoupledHeightFog(float3 Radiance, float3 RayOrigin, float3 RayDir, float TravelMeters)
{
    float4 Fog0 = FrameHeader[17];
    if (Fog0.w <= 1.0e-6f || TravelMeters <= 0.05f)
    {
        return Radiance;
    }
    float Density = Fog0.w;
    float Falloff = max(1.0e-4f, FrameHeader[18].x);
    float Dist    = min(TravelMeters, 450.0f);
    float BaseExp = exp(-clamp(RayOrigin.y * Falloff, -8.0f, 12.0f));
    float DeltaH  = RayDir.y * Dist * Falloff;
    float LineInt = (abs(DeltaH) > 1.0e-3f) ? ((1.0f - exp(-clamp(DeltaH, -10.0f, 10.0f))) / DeltaH) : 1.0f;
    float Tau     = clamp(Density * Dist * BaseExp * LineInt, 0.0f, 12.0f);
    float Trans   = exp(-Tau);
    return Radiance * Trans + Fog0.xyz * (1.0f - Trans);
}

//------------------------------------------------------------------------------------------------------------------------
// UNIFIED SCENE GEOMETRY & DISTANCE QUERIES
//------------------------------------------------------------------------------------------------------------------------

struct UnifiedOpaqueHit
{
    bool   Hit;
    float  DistanceM;
    uint   ClassId;
    uint   VisitedBlocks;
    float3 WorldPos;
    float3 GeoNormal;
    float3 WorldTangent;
    float2 TexCoord;
    float3 InstanceTint;
    float  InstanceSelfRadiance;
};

UnifiedOpaqueHit TraceSceneOpaque(float3 Origin, float3 RayDir, float MinT, float MaxT)
{
    uint CellCount = uint(FrameHeader[2].w + 0.5f);
    if (FrameHeader[12].x > 0.5f)
    {
        GpuOctetSurfaceHit H = SearchOctetInstancesOpaque(
            BvhOrTopCells, OctetInstances, OctetBlocks, BvhTriangles, CellCount, Origin, RayDir, MinT, MaxT);
        UnifiedOpaqueHit Out;
        Out.Hit                  = H.Hit;
        Out.DistanceM            = H.DistanceM;
        Out.ClassId              = H.ClassId;
        Out.VisitedBlocks        = H.VisitedBlocks;
        Out.WorldPos             = H.WorldPos;
        Out.GeoNormal            = (dot(H.GeoNormal, RayDir) > 0.0f) ? -H.GeoNormal : H.GeoNormal;
        Out.WorldTangent         = H.WorldTangent;
        Out.TexCoord             = H.TexCoord;
        Out.InstanceTint         = H.InstanceTint;
        Out.InstanceSelfRadiance = H.InstanceSelfRadiance;
        return Out;
    }

    GpuSurfaceHit B = SearchBoundingVolumeOpaque(BvhOrTopCells, BvhTriangles, CellCount, Origin, RayDir, MinT, MaxT);
    UnifiedOpaqueHit Out;
    Out.Hit                  = B.Hit;
    Out.DistanceM            = B.DistanceM;
    Out.ClassId              = B.ClassId;
    Out.VisitedBlocks        = 1u;
    Out.WorldPos             = B.WorldPos;
    Out.GeoNormal            = B.GeoNormal;
    Out.WorldTangent         = normalize(cross(abs(B.GeoNormal.y) < 0.99f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f), B.GeoNormal));
    Out.TexCoord             = B.TexCoord;
    Out.InstanceTint         = float3(1.0f);
    Out.InstanceSelfRadiance = 0.0f;
    return Out;
}

float3 TraceSceneShadowTransmittance(float3 Origin, float3 RayDir, float MaxT)
{
    uint CellCount = uint(FrameHeader[2].w + 0.5f);
    if (FrameHeader[12].x > 0.5f)
    {
        return SearchOctetInstancesShadow(BvhOrTopCells, OctetInstances, OctetBlocks, BvhTriangles, CellCount, Origin, RayDir, MaxT);
    }
    return SearchBoundingVolumeShadowTransmittance(BvhOrTopCells, BvhTriangles, CellCount, Origin, RayDir, 1.5e-3f, MaxT);
}

float EvaluateSceneDistanceAO(float3 WorldPos, float3 Normal)
{
    if (FrameHeader[7].z < 0.5f)
    {
        return 1.0f;
    }
    return EvaluateHardwareSdfAmbientOcclusion(
        SdfClipmapVolume, DistanceOrRangeTable, FrameHeader[12].y > 0.5f, FrameHeader[19].z, WorldPos, Normal);
}

struct LuminaireProposal
{
    bool   Valid;
    float3 LightDir;
    float  DistanceM;
    float3 UnshadowedLi;
    float  ProposalPdf;
};

LuminaireProposal SampleAnalyticLuminaire(uint LumIdx, float3 SurfacePos, float2 XiArea)
{
    LuminaireProposal Out;
    Out.Valid        = false;
    Out.LightDir     = float3(0.0f, 1.0f, 0.0f);
    Out.DistanceM    = 1.0e4f;
    Out.UnshadowedLi = float3(0.0f);
    Out.ProposalPdf  = 1.0f;

    uint   Slot = ResolveLuminaireBaseSlot() + LumIdx * 4u;
    float4 L0   = FrameHeader[Slot + 0u];
    float4 L1   = FrameHeader[Slot + 1u];
    float4 L2   = FrameHeader[Slot + 2u];
    float4 L3   = FrameHeader[Slot + 3u];
    uint   Type = uint(L0.w + 0.5f);

    if (Type == 0u)
    {
        Out.Valid        = true;
        Out.LightDir     = normalize(L0.xyz);
        Out.DistanceM    = 1.0e4f;
        Out.UnshadowedLi = L3.xyz;
        Out.ProposalPdf  = 1.0f;
    }
    else if (Type == 2u)
    {
        float3 ToLight   = L0.xyz - SurfacePos;
        float  DistSq    = max(1.0e-3f, dot(ToLight, ToLight));
        float  Dist      = sqrt(DistSq);
        Out.Valid        = true;
        Out.LightDir     = ToLight / Dist;
        Out.DistanceM    = Dist - 1.0e-2f;
        Out.UnshadowedLi = L3.xyz / (1.0f + DistSq);
        Out.ProposalPdf  = 1.0f;
    }
    else
    {
        float2 Uv      = XiArea * 2.0f - 1.0f;
        float3 SampleP = L0.xyz + Uv.x * L1.xyz + Uv.y * L2.xyz;
        float3 CrossUV = cross(L1.xyz, L2.xyz);
        float  Area    = max(1.0e-4f, 4.0f * length(CrossUV));
        float3 LightN  = normalize(CrossUV);
        float3 ToLight = SampleP - SurfacePos;
        float  DistSq  = max(1.0e-3f, dot(ToLight, ToLight));
        float  Dist    = sqrt(DistSq);
        float3 Dir     = ToLight / Dist;
        float  CosL    = abs(dot(LightN, -Dir));
        if (CosL > 1.0e-4f)
        {
            Out.Valid        = true;
            Out.LightDir     = Dir;
            Out.DistanceM    = Dist - 1.5e-2f;
            Out.UnshadowedLi = L3.xyz * (Area * CosL / DistSq);
            Out.ProposalPdf  = 1.0f;
        }
    }
    return Out;
}

LuminaireProposal ProposeCombinedDirectSample(float3 SurfacePos, inout uint Rng)
{
    uint  LumCount   = uint(FrameHeader[3].w + 0.5f);
    uint  FacetCount = uint(FrameHeader[12].z + 0.5f);
    float ProbFacet  = (FacetCount > 0u) ? ((LumCount > 0u) ? 0.25f : 1.0f) : 0.0f;
    float Roll       = SampleUniform01(Rng);

    if (ProbFacet > 0.0f && Roll < ProbFacet)
    {
        GpuLuminousFacetSample S = SampleGpuLuminousFacet(
            LuminousFacets, FacetCount, SurfacePos, SampleUniform01(Rng), SampleUniform01(Rng), SampleUniform01(Rng));
        LuminaireProposal Out;
        Out.Valid        = S.Valid && (S.SolidAnglePdf > 1.0e-6f);
        float3 ToL       = S.WorldPoint - SurfacePos;
        float  Dist      = max(1.0e-3f, length(ToL));
        Out.LightDir     = ToL / Dist;
        Out.DistanceM    = max(1.0e-3f, Dist - 1.5e-2f);
        Out.UnshadowedLi = S.SelfRadiance;
        Out.ProposalPdf  = ProbFacet * S.SolidAnglePdf;
        return Out;
    }

    if (LumCount == 0u)
    {
        LuminaireProposal Empty;
        Empty.Valid        = false;
        Empty.LightDir     = float3(0.0f, 1.0f, 0.0f);
        Empty.DistanceM    = 1.0f;
        Empty.UnshadowedLi = float3(0.0f);
        Empty.ProposalPdf  = 1.0f;
        return Empty;
    }

    uint   Chosen = min(uint(SampleUniform01(Rng) * float(LumCount)), LumCount - 1u);
    float2 XiA    = float2(SampleUniform01(Rng), SampleUniform01(Rng));
    LuminaireProposal Out = SampleAnalyticLuminaire(Chosen, SurfacePos, XiA);
    Out.ProposalPdf *= (1.0f - ProbFacet) / float(LumCount);
    return Out;
}

UnpackedSurfaceMaterial ReconstructGbufferMaterial(uint2 Px, out float3 OutN, out float3 OutT, out float3 OutB)
{
    float4 G1 = SurfaceAndWorkAtlas[uint3(Px, 1u)];
    float4 G2 = SurfaceAndWorkAtlas[uint3(Px, 2u)];
    float4 G3 = SurfaceAndWorkAtlas[uint3(Px, 3u)];
    float4 G4 = SurfaceAndWorkAtlas[uint3(Px, 4u)];
    float4 G5 = SurfaceAndWorkAtlas[uint3(Px, 5u)];

    uint ClassId                = uint(max(0.0f, G2.w) + 0.5f);
    UnpackedSurfaceMaterial Mat = FetchSurfaceMaterial(MaterialTable, ClassId);
    Mat.BaseAlbedo              = G5.xyz;
    Mat.AmbientOcclusion        = G5.w;
    Mat.BaseRoughness           = clamp(G1.w, 0.045f, 1.0f);
    Mat.Metallic                = saturate(G3.x);
    Mat.CoatCoverage            = saturate(G3.y);
    Mat.CoatRoughness           = clamp(G3.z, 0.045f, 1.0f);
    Mat.Anisotropy              = clamp(G4.w, -0.9f, 0.9f);
    Mat.SelfRadiance            = G2.xyz;

    OutN = normalize(G1.xyz);
    OutT = G4.xyz - OutN * dot(OutN, G4.xyz);
    OutT = (length(OutT) > 1.0e-5f)
               ? normalize(OutT)
               : normalize(cross(abs(OutN.y) < 0.99f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f), OutN));
    OutB = cross(OutN, OutT);
    return Mat;
}

bool ProjectToPreviousScreenPosition(float3 WorldPos, uint Width, uint Height, out float2 OutPrevScreenPos, out float2 OutMotionUv, int2 CurrPx)
{
    float4 R0    = FrameHeader[8];
    float4 R1    = FrameHeader[9];
    float4 R2    = FrameHeader[10];
    float  ViewZ = dot(R2.xyz, WorldPos) + R2.w;
    if (ViewZ <= 0.05f)
    {
        OutPrevScreenPos = float2(CurrPx) + 0.5f;
        OutMotionUv      = float2(0.0f);
        return false;
    }
    float  NdcX   = (dot(R0.xyz, WorldPos) + R0.w) / ViewZ;
    float  NdcY   = (dot(R1.xyz, WorldPos) + R1.w) / ViewZ;
    float  PrevU  = 0.5f * (NdcX + 1.0f);
    float  PrevV  = 0.5f * (1.0f - NdcY);
    float2 CurrUv = (float2(CurrPx) + 0.5f) / float2(float(Width), float(Height));
    OutMotionUv   = float2(PrevU, PrevV) - CurrUv;
    OutPrevScreenPos = float2(PrevU * float(Width), PrevV * float(Height));
    return (PrevU >= 0.0f && PrevU < 1.0f && PrevV >= 0.0f && PrevV < 1.0f);
}

float3 ShadeSecondarySurface(float3 HitPos,
                             float2 HitUv,
                             float  HitDistM,
                             float3 GeoN,
                             uint   ClassId,
                             float3 ViewDirToOrigin,
                             float3 InstanceTint,
                             float  InstanceSelfRad,
                             bool   UseRaytracedShadow)
{
    UnpackedSurfaceMaterial Mat = FetchSurfaceMaterial(MaterialTable, ClassId);
    if (Mat.AlbedoLayer >= 0)
    {
        float Mip = clamp(log2(max(1.0f, HitDistM * 0.20f * Mat.UvTilingScale)), 0.0f, 8.0f);
        Mat.BaseAlbedo *= MaterialMaps.SampleLevel(float3(HitUv * Mat.UvTilingScale, float(Mat.AlbedoLayer)), Mip).rgb;
    }
    Mat.BaseAlbedo *= InstanceTint;
    float3 OutRad   = Mat.SelfRadiance + Mat.BaseAlbedo * InstanceSelfRad;

    uint LumCount = uint(FrameHeader[3].w + 0.5f);
    for (uint L = 0u; L < min(LumCount, 3u); ++L)
    {
        LuminaireProposal Prop = SampleAnalyticLuminaire(L, HitPos, float2(0.5f, 0.5f));
        float             NdotL = max(0.0f, dot(GeoN, Prop.LightDir));
        if (Prop.Valid && NdotL > 0.0f)
        {
            float3 Vis = float3(1.0f);
            if (L == 0u)
            {
                if (UseRaytracedShadow || FrameHeader[32].w < 0.5f)
                {
                    Vis = TraceSceneShadowTransmittance(HitPos + GeoN * 0.015f, Prop.LightDir, Prop.DistanceM);
                }
                else
                {
                    float CamDist = length(HitPos - FrameHeader[0].xyz);
                    Vis           = float3(ResolveDeterministicSolarVisibility(ShadowDepthArray, FrameHeader, HitPos, GeoN, CamDist));
                }
            }
            OutRad += EvaluateSurfaceScatteringBrdf(Mat, HitPos, GeoN, ViewDirToOrigin, Prop.LightDir) * Prop.UnshadowedLi * Vis * NdotL;
        }
    }

    uint PatchCount = uint(FrameHeader[12].w + 0.5f);
    if (PatchCount > 0u && FrameHeader[13].z > 0.0f)
    {
        float3 CardRad = SampleGpuCombinedSurfaceRadiosity(SurfaceCardPack, PatchCount, HitPos, GeoN);
        OutRad += (1.0f - Mat.Metallic) * Mat.BaseAlbedo * CardRad * FrameHeader[13].z;
    }
    else
    {
        float3 SkyHemi = lerp(FrameHeader[6].xyz, FrameHeader[4].xyz, 0.5f * GeoN.y + 0.5f);
        OutRad += (1.0f - Mat.Metallic) * Mat.BaseAlbedo * SkyHemi * 0.32f;
    }
    return OutRad;
}

//------------------------------------------------------------------------------------------------------------------------
// KERNEL 0: MULTI-BOUNCE GPU SURFACE CARD RADIOSITY SOLVER
//------------------------------------------------------------------------------------------------------------------------

groupshared float3 SharedCardR0[256];
groupshared float3 SharedCardB[256];

[shader("compute")]
[numthreads(256, 1, 1)]
void SolveSurfaceRadiosityMain(uint3 Tid : SV_DispatchThreadID)
{
    uint Idx        = Tid.x;
    uint PatchCount = min(uint(FrameHeader[12].w + 0.5f), 256u);
    uint Bounces    = min(uint(FrameHeader[18].y + 0.5f), 8u);

    float3 Center = float3(0.0f);
    float3 Normal = float3(0.0f, 1.0f, 0.0f);
    float3 Refl   = float3(0.0f);
    float3 Direct = float3(0.0f);

    if (Idx < PatchCount)
    {
        uint   Base = Idx * 4u;
        float4 G0   = SurfaceCardPack[Base + 0u];
        float4 G1   = SurfaceCardPack[Base + 1u];
        float4 G2   = SurfaceCardPack[Base + 2u];
        float4 G3   = SurfaceCardPack[Base + 3u];

        Center = G0.xyz;
        Normal = normalize(G1.xyz);
        Refl   = clamp(G2.xyz, float3(0.0f), float3(0.95f));

        float3 Irrad    = float3(0.0f);
        uint   LumCount = uint(FrameHeader[3].w + 0.5f);
        for (uint L = 0u; L < LumCount; ++L)
        {
            LuminaireProposal Prop = SampleAnalyticLuminaire(L, Center, float2(0.5f, 0.5f));
            float             CosT = max(0.0f, dot(Normal, Prop.LightDir));
            if (Prop.Valid && CosT > 0.0f)
            {
                float3 Vis = TraceSceneShadowTransmittance(Center + Normal * 0.04f, Prop.LightDir, Prop.DistanceM);
                Irrad += Prop.UnshadowedLi * Vis * CosT;
            }
        }

        float  SkyWeight = FrameHeader[13].w;
        float3 SkyIrrad  = lerp(FrameHeader[6].xyz, FrameHeader[4].xyz, 0.5f * Normal.y + 0.5f) * 3.14159265f * SkyWeight * 0.32f;
        Irrad += SkyIrrad * EvaluateSceneDistanceAO(Center, Normal);
        Direct = G3.xyz + (Refl * (1.0f / 3.14159265f)) * Irrad;
    }

    SharedCardR0[Idx] = Direct;
    SharedCardB[Idx]  = Direct;
    GroupMemoryBarrierWithGroupSync();

    for (uint Bounce = 0u; Bounce < Bounces; ++Bounce)
    {
        float3 NextB = Direct;
        if (Idx < PatchCount)
        {
            float3 IndirectIrrad = float3(0.0f);
            for (uint J = 0u; J < PatchCount; ++J)
            {
                if (J == Idx) continue;
                float4 GJ  = SurfaceCardPack[J * 4u + 0u];
                float4 NJ  = SurfaceCardPack[J * 4u + 1u];
                float3 Vec = GJ.xyz - Center;
                float  D2  = dot(Vec, Vec);
                if (D2 <= 1.0e-4f) continue;
                float  InvD = rsqrt(D2);
                float3 Dir  = Vec * InvD;
                float  CosI = dot(Normal, Dir);
                float  CosJ = dot(NJ.xyz, -Dir);
                if (CosI > 0.0f && CosJ > 0.0f)
                {
                    float FormFactor = min(0.22f, (CosI * CosJ * GJ.w) / (3.14159265f * D2 + GJ.w));
                    IndirectIrrad += SharedCardB[J] * (3.14159265f * FormFactor);
                }
            }
            NextB = SharedCardR0[Idx] + (Refl * (1.0f / 3.14159265f)) * IndirectIrrad;
        }
        GroupMemoryBarrierWithGroupSync();
        SharedCardB[Idx] = NextB;
        GroupMemoryBarrierWithGroupSync();
    }

    if (Idx < PatchCount)
    {
        SurfaceCardPack[PatchCount * 4u + Idx] = float4(SharedCardB[Idx], 1.0f);
    }
}

//------------------------------------------------------------------------------------------------------------------------
// EMERGENCY COMPUTE PRIMARY FALLBACK (ONLY USED ON HEADLESS COMPUTE-ONLY QUEUES WITHOUT GRAPHICS BIT)
//------------------------------------------------------------------------------------------------------------------------

[shader("compute")]
[numthreads(8, 8, 1)]
void TracePrimarySurfaceMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint Width  = Push.Counts.x;
    uint Height = Push.Counts.y;
    if (DispatchId.x >= Width || DispatchId.y >= Height)
    {
        return;
    }

    uint2  Px      = DispatchId.xy;
    float3 CamOrig = FrameHeader[0].xyz;
    float3 RayDir  = ReconstructPrimaryRayDirection(Px, Width, Height);

    UnifiedOpaqueHit Opaque    = TraceSceneOpaque(CamOrig, RayDir, 1.0e-3f, 2000.0f);
    float            MaxGlassT = Opaque.Hit ? Opaque.DistanceM : 2000.0f;
    uint             CellCount = uint(FrameHeader[2].w + 0.5f);

    bool   HitGlass       = false;
    float  GlassFrontT    = MaxGlassT;
    float  GlassThickness = 0.014f;
    float  GlassIor       = 1.52f;
    float3 GlassFrontN    = -RayDir;
    float3 GlassBackN     = RayDir;
    float3 GlassAbsorb    = float3(0.08f, 0.03f, 0.05f);

    if (FrameHeader[12].x > 0.5f)
    {
        GpuOctetGlassPeel P = SearchOctetInstancesGlassPeel(
            BvhOrTopCells, OctetInstances, OctetBlocks, BvhTriangles, CellCount, CamOrig, RayDir, MaxGlassT);
        HitGlass       = P.HitGlass;
        GlassFrontT    = P.FrontT;
        GlassThickness = P.ThicknessM;
        GlassIor       = P.Ior;
        GlassFrontN    = P.FrontNormal;
        GlassBackN     = P.BackNormal;
        GlassAbsorb    = P.AbsorptionPerM;
    }
    else
    {
        GpuGlassPeelHit P = SearchBoundingVolumeGlassPeel(
            BvhOrTopCells, BvhTriangles, CellCount, CamOrig, RayDir, 1.0e-3f, MaxGlassT);
        HitGlass       = P.HitGlass;
        GlassFrontT    = P.FrontT;
        GlassThickness = P.ThicknessM;
        GlassIor       = P.Ior;
        GlassFrontN    = P.FrontNormal;
        GlassBackN     = -P.FrontNormal;
        GlassAbsorb    = P.AbsorptionPerM;
    }

    if (HitGlass)
    {
        float3 FrontPos = CamOrig + RayDir * GlassFrontT;
        float  R1       = 0.04f;
        float3 DirIn    = RayDir;
        bool   Tir1     = false;
        EvaluateExactDielectricBoundary(RayDir, GlassFrontN, 1.0f, GlassIor, R1, DirIn, Tir1);
        float3 BackPos  = FrontPos + DirIn * GlassThickness;
        float3 ExitNorm = (dot(-DirIn, GlassBackN) > 0.05f) ? GlassBackN : GlassFrontN;
        float  R2       = 0.04f;
        float3 DirExit  = DirIn;
        bool   Tir2     = false;
        EvaluateExactDielectricBoundary(DirIn, ExitNorm, GlassIor, 1.0f, R2, DirExit, Tir2);
        if (Tir2)
        {
            DirExit         = normalize(lerp(RayDir, reflect(DirIn, ExitNorm), 0.45f));
            GlassThickness *= 2.2f;
        }
        float3           BeerLambert = exp(-GlassAbsorb * GlassThickness * 4.0f);
        UnifiedOpaqueHit Behind      = TraceSceneOpaque(BackPos + DirExit * 0.008f, DirExit, 1.0e-3f, 2000.0f);
        float3           TransRad    = Behind.Hit
                                           ? ApplyCoupledHeightFog(
                                                 ShadeSecondarySurface(Behind.WorldPos, Behind.TexCoord, Behind.DistanceM, Behind.GeoNormal, Behind.ClassId, -DirExit, Behind.InstanceTint, Behind.InstanceSelfRadiance, true),
                                                 BackPos, DirExit, Behind.DistanceM)
                                           : EvaluateCoupledEnvironmentSky(DirExit);
        float3           ReflDir = reflect(RayDir, GlassFrontN);
        UnifiedOpaqueHit ReflHit = TraceSceneOpaque(FrontPos + GlassFrontN * 0.01f, ReflDir, 1.0e-3f, 1500.0f);
        float3           ReflRad = ReflHit.Hit
                                       ? ShadeSecondarySurface(ReflHit.WorldPos, ReflHit.TexCoord, ReflHit.DistanceM, ReflHit.GeoNormal, ReflHit.ClassId, -ReflDir, ReflHit.InstanceTint, ReflHit.InstanceSelfRadiance, true)
                                       : EvaluateCoupledEnvironmentSky(ReflDir);
        float  EffectiveR = saturate(R1 + (1.0f - R1) * R2 * 0.65f);
        float3 GlassRad   = ReflRad * EffectiveR + TransRad * BeerLambert * (1.0f - EffectiveR);

        SurfaceAndWorkAtlas[uint3(Px, 0u)] = float4(FrontPos, -max(1.0e-3f, GlassFrontT));
        SurfaceAndWorkAtlas[uint3(Px, 1u)] = float4(GlassFrontN, 0.045f);
        SurfaceAndWorkAtlas[uint3(Px, 2u)] = float4(GlassRad, 7.0f);
        SurfaceAndWorkAtlas[uint3(Px, 3u)] = float4(0.0f, 0.0f, 0.045f, EffectiveR);
        SurfaceAndWorkAtlas[uint3(Px, 4u)] = float4(1.0f, 0.0f, 0.0f, 0.0f);
        SurfaceAndWorkAtlas[uint3(Px, 5u)] = float4(BeerLambert, 1.0f);
        return;
    }

    if (!Opaque.Hit)
    {
        SurfaceAndWorkAtlas[uint3(Px, 0u)] = float4(0.0f);
        SurfaceAndWorkAtlas[uint3(Px, 1u)] = float4(0.0f, 1.0f, 0.0f, 1.0f);
        SurfaceAndWorkAtlas[uint3(Px, 2u)] = float4(0.0f);
        SurfaceAndWorkAtlas[uint3(Px, 3u)] = float4(0.0f);
        SurfaceAndWorkAtlas[uint3(Px, 4u)] = float4(1.0f, 0.0f, 0.0f, 0.0f);
        SurfaceAndWorkAtlas[uint3(Px, 5u)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float3                  GeoN = (dot(Opaque.GeoNormal, RayDir) > 0.0f) ? -Opaque.GeoNormal : Opaque.GeoNormal;
    UnpackedSurfaceMaterial Mat  = FetchSurfaceMaterial(MaterialTable, Opaque.ClassId);
    float3                  ShadingN, RotT, RotB;
    ResolveAuthoredMaterialAtHit(
        Mat, MaterialMaps, Opaque.WorldPos, Opaque.TexCoord, Opaque.DistanceM, GeoN, Opaque.WorldTangent, ShadingN, RotT, RotB);
    Mat.BaseAlbedo   *= Opaque.InstanceTint;
    Mat.SelfRadiance += Mat.BaseAlbedo * Opaque.InstanceSelfRadiance;

    float EncodedTierBlocks            = float(Mat.MaterialLodTier) + 0.01f * float(min(Opaque.VisitedBlocks, 99u));
    SurfaceAndWorkAtlas[uint3(Px, 0u)] = float4(Opaque.WorldPos, Opaque.DistanceM);
    SurfaceAndWorkAtlas[uint3(Px, 1u)] = float4(ShadingN, Mat.BaseRoughness);
    SurfaceAndWorkAtlas[uint3(Px, 2u)] = float4(Mat.SelfRadiance, float(Opaque.ClassId));
    SurfaceAndWorkAtlas[uint3(Px, 3u)] = float4(Mat.Metallic, Mat.CoatCoverage, Mat.CoatRoughness, EncodedTierBlocks);
    SurfaceAndWorkAtlas[uint3(Px, 4u)] = float4(RotT, Mat.Anisotropy);
    SurfaceAndWorkAtlas[uint3(Px, 5u)] = float4(Mat.BaseAlbedo, Mat.AmbientOcclusion);
}

//------------------------------------------------------------------------------------------------------------------------
// KERNEL 3: DIRECT RADIANCE (HYBRID: ZERO-NOISE SHADOWRASTER + KARIS AREA LIGHTS | OPTION 2: FULL RAYTRACED DI)
//------------------------------------------------------------------------------------------------------------------------

float EvaluateDirectTargetPdf(UnpackedSurfaceMaterial Mat, float3 N, float3 T, float3 B, float3 V, float3 L, float3 Li)
{
    float NdotL = max(0.0f, dot(N, L));
    if (NdotL <= 0.0f) return 0.0f;
    float3 DiffBrdf, SpecBrdf;
    EvaluateSurfaceSplitBrdf(Mat, N, T, B, V, L, DiffBrdf, SpecBrdf);
    return Luminance709((DiffBrdf + SpecBrdf) * Li * NdotL);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void EvaluateDirectRadianceMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint Width  = Push.Counts.x;
    uint Height = Push.Counts.y;
    if (DispatchId.x >= Width || DispatchId.y >= Height)
    {
        return;
    }

    uint2  Px = DispatchId.xy;
    float4 G0 = SurfaceAndWorkAtlas[uint3(Px, 0u)];
    if (G0.w <= 0.0f)
    {
        SurfaceAndWorkAtlas[uint3(Px, 6u)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float3 WorldPos = G0.xyz;
    float  HitT     = G0.w;
    float3 ViewDir  = normalize(FrameHeader[0].xyz - WorldPos);
    float3 N, T, B;
    UnpackedSurfaceMaterial Mat = ReconstructGbufferMaterial(Px, N, T, B);

    // Default Hybrid Mode (or whenever ShadowRaster cascades are populated and FullRaytracingMode == false):
    // 100% deterministic, zero Monte Carlo noise!
    if (!IsFullRaytracingEnabled() && FrameHeader[32].w > 0.5f)
    {
        float  PrimaryShadow = 1.0f;
        float3 DirectRgb     = EvaluateDeterministicDirectRadiance(
            FrameHeader, ShadowDepthArray, SdfClipmapVolume, DistanceOrRangeTable, Mat, WorldPos, HitT, N, T, B, ViewDir, PrimaryShadow);
        SurfaceAndWorkAtlas[uint3(Px, 6u)] = float4(DirectRgb, PrimaryShadow);
        return;
    }

    // Option 2 (Full Raytracing Mode, or compute-only fallback without ShadowRaster):
    // Raytraced Beer-Lambert shadow transmittance across solar + area softboxes + luminous facets
    uint   Rng          = SeedPixelStream(Px, Push.Counts.z, 1u);
    uint   LumCount     = uint(FrameHeader[3].w + 0.5f);
    float3 RayDirectSum = float3(0.0f);
    float  PrimaryVis   = 1.0f;

    for (uint L = 0u; L < LumCount; ++L)
    {
        float2            Xi   = SampleR2Sequence(Px, Push.Counts.z, L + 1u);
        LuminaireProposal Prop = SampleAnalyticLuminaire(L, WorldPos, Xi);
        float             NdL  = max(0.0f, dot(N, Prop.LightDir));
        if (Prop.Valid && NdL > 0.0f)
        {
            float3 Vis = TraceSceneShadowTransmittance(WorldPos + N * 0.012f, Prop.LightDir, Prop.DistanceM);
            if (L == 0u) PrimaryVis = Luminance709(Vis);
            float3 DiffB, SpecB;
            EvaluateSurfaceSplitBrdf(Mat, N, T, B, ViewDir, Prop.LightDir, DiffB, SpecB);
            RayDirectSum += (DiffB + SpecB) * Prop.UnshadowedLi * Vis * NdL;
        }
    }

    uint FacetCount = uint(FrameHeader[12].z + 0.5f);
    if (FacetCount > 0u)
    {
        GpuLuminousFacetSample S = SampleGpuLuminousFacet(
            LuminousFacets, FacetCount, WorldPos, SampleUniform01(Rng), SampleUniform01(Rng), SampleUniform01(Rng));
        if (S.Valid && S.SolidAnglePdf > 1.0e-5f)
        {
            float3 ToF  = S.WorldPoint - WorldPos;
            float  Dist = max(1.0e-3f, length(ToF));
            float3 LDir = ToF / Dist;
            float  NdL  = max(0.0f, dot(N, LDir));
            if (NdL > 0.0f)
            {
                float3 Vis = TraceSceneShadowTransmittance(WorldPos + N * 0.012f, LDir, max(1.0e-3f, Dist - 0.02f));
                float3 DiffB, SpecB;
                EvaluateSurfaceSplitBrdf(Mat, N, T, B, ViewDir, LDir, DiffB, SpecB);
                RayDirectSum += min((DiffB + SpecB) * S.SelfRadiance * Vis * (NdL / S.SolidAnglePdf), float3(6.0f));
            }
        }
    }

    // Temporal accumulation of Full-RT Direct Radiance in DirectReservoirPrev/Curr
    uint   ResIdx = Px.y * Width + Px.x;
    float2 PrevPos, MotionUv;
    if (Push.Params.w > 0.5f && ProjectToPreviousScreenPosition(WorldPos, Width, Height, PrevPos, MotionUv, int2(Px)))
    {
        uint2 PrevPx = uint2(clamp(int2(PrevPos), int2(0), int2(int(Width) - 1, int(Height) - 1)));
        float4 PrevD = DirectReservoirPrev[PrevPx.y * Width + PrevPx.x];
        if (PrevD.w > 0.5f)
        {
            float HistN  = min(PrevD.w + 1.0f, 12.0f);
            RayDirectSum = lerp(PrevD.xyz, RayDirectSum, 1.0f / HistN);
            DirectReservoirCurr[ResIdx] = float4(RayDirectSum, HistN);
        }
        else
        {
            DirectReservoirCurr[ResIdx] = float4(RayDirectSum, 1.0f);
        }
    }
    else
    {
        DirectReservoirCurr[ResIdx] = float4(RayDirectSum, 1.0f);
    }
    SurfaceAndWorkAtlas[uint3(Px, 6u)] = float4(RayDirectSum, PrimaryVis);
}

//------------------------------------------------------------------------------------------------------------------------
// KERNEL 4: RESTIR DIFFUSE GLOBAL ILLUMINATION (ReservoirTemporal.wgsl + ReservoirSpatial.wgsl 1:1 PORT)
//   - TraceIndirectDiffuseMain : 1 cosine initial candidate (GTX: Hardware Sampler3D SDF + SurfaceCards | RTX: CWBVH
//     triangle ray) + motion-reprojected temporal reservoir reuse with rotating-quarter visibility re-trace.
//   - ResampleGiSpatialMain    : disk-proposal spatial reuse with reconnection-shift Jacobian, 1/Z bias correction,
//     adopted-sample visibility test, and demodulated irradiance resolve into layer 7 (feeds SVGF).
//   Both modes trace at TraceScale (half-res default) with exactly 1 ray/px, matching the WebGPU demo workload.
//------------------------------------------------------------------------------------------------------------------------

static const float kGiSkyReachM = 10000.0f; // [m] escaped direction pushed to a position (SkyReach in ReservoirRecords.wgsl)

struct GiReservoirSample
{
    float3 Position;
    float3 Normal;
    float3 Radiance;
    uint   Identity; // 0 = sky, 1 = surface hit
};

struct GiReservoir
{
    GiReservoirSample Sample;
    float             WeightSum;
    float             Tally;
    float             Weight;
    bool              Adopted;
};

GiReservoir ConstructEmptyGiReservoir()
{
    GiReservoir Empty;
    Empty.Sample.Position = float3(0.0f);
    Empty.Sample.Normal   = float3(0.0f, 0.0f, 1.0f);
    Empty.Sample.Radiance = float3(0.0f);
    Empty.Sample.Identity = 0u;
    Empty.WeightSum       = 0.0f;
    Empty.Tally           = 0.0f;
    Empty.Weight          = 0.0f;
    Empty.Adopted         = false;
    return Empty;
}


// IEEE-754 binary16 <-> binary32 via pure integer bit ops (no Float16 SPIR-V capability required,
// so the kernels also run on the SwiftShader CI device).
uint PackFloatToHalfBits(float Value)
{
    uint Bits  = asuint(Value);
    uint Sign  = (Bits >> 16u) & 0x8000u;
    int  Exp   = int((Bits >> 23u) & 0xFFu) - 127 + 15;
    uint Mant  = Bits & 0x7FFFFFu;
    if (Exp >= 31)
    {
        return Sign | 0x7C00u; // clamp to max finite (reservoir radiance is pre-clamped to 60000 anyway)
    }
    if (Exp <= 0)
    {
        if (Exp < -10)
        {
            return Sign;
        }
        Mant        |= 0x800000u;
        uint Shifted = Mant >> uint(14 - Exp);
        if (((Mant >> uint(13 - Exp)) & 1u) != 0u)
        {
            Shifted += 1u;
        }
        return Sign | Shifted;
    }
    if ((Mant & 0x1000u) != 0u)
    {
        Mant += 0x2000u;
        if ((Mant & 0x800000u) != 0u)
        {
            Mant  = 0u;
            Exp  += 1;
            if (Exp >= 31)
            {
                return Sign | 0x7C00u;
            }
        }
    }
    return Sign | (uint(Exp) << 10u) | (Mant >> 13u);
}

float UnpackHalfBitsToFloat(uint Half)
{
    uint Sign = (Half & 0x8000u) << 16u;
    int  Exp  = int((Half >> 10u) & 0x1Fu);
    uint Mant = Half & 0x3FFu;
    if (Exp == 0)
    {
        if (Mant == 0u)
        {
            return asfloat(Sign);
        }
        Exp = 1;
        while ((Mant & 0x400u) == 0u)
        {
            Mant <<= 1u;
            Exp   -= 1;
        }
        Mant &= 0x3FFu;
    }
    else if (Exp == 31)
    {
        return asfloat(Sign | 0x7F800000u | (Mant << 13u));
    }
    return asfloat(Sign | (uint(Exp - 15 + 127) << 23u) | (Mant << 13u));
}

float2 UnpackGiOctahedral(uint Packed)
{
    int X = (int(Packed & 0xFFFFu) << 16) >> 16;
    int Y = int(Packed & 0xFFFF0000u) >> 16;
    return float2(float(X), float(Y)) / 32767.0f;
}

void StoreGiReservoir(uint Index, GiReservoir Source)
{
    uint Base         = Index * 8u;
    float2 Oct        = clamp(EncodeOctahedral(normalize(Source.Sample.Normal)), -1.0f, 1.0f);
    GiReservoirCurr[Base + 0u] = asuint(Source.Sample.Position.x);
    GiReservoirCurr[Base + 1u] = asuint(Source.Sample.Position.y);
    GiReservoirCurr[Base + 2u] = asuint(Source.Sample.Position.z);
    GiReservoirCurr[Base + 3u] = (uint(int(round(Oct.x * 32767.0f))) & 0xFFFFu) |
                                 ((uint(int(round(Oct.y * 32767.0f))) & 0xFFFFu) << 16u);
    GiReservoirCurr[Base + 4u] = PackFloatToHalfBits(min(Source.Sample.Radiance.x, 60000.0f)) |
                                 (PackFloatToHalfBits(min(Source.Sample.Radiance.y, 60000.0f)) << 16u);
    GiReservoirCurr[Base + 5u] = PackFloatToHalfBits(min(Source.Sample.Radiance.z, 60000.0f)) |
                                 (PackFloatToHalfBits(float(Source.Sample.Identity)) << 16u);
    GiReservoirCurr[Base + 6u] = asuint(Source.Weight);
    GiReservoirCurr[Base + 7u] = asuint(Source.Tally);
}

GiReservoir LoadGiReservoirPrev(uint Index)
{
    uint        Base   = Index * 8u;
    GiReservoir Result = ConstructEmptyGiReservoir();
    Result.Sample.Position = float3(asfloat(GiReservoirPrev[Base + 0u]),
                                    asfloat(GiReservoirPrev[Base + 1u]),
                                    asfloat(GiReservoirPrev[Base + 2u]));
    Result.Sample.Normal   = DecodeOctahedral(UnpackGiOctahedral(GiReservoirPrev[Base + 3u]));
    uint PackRg            = GiReservoirPrev[Base + 4u];
    uint PackBi            = GiReservoirPrev[Base + 5u];
    Result.Sample.Radiance = float3(UnpackHalfBitsToFloat(PackRg & 0xFFFFu), UnpackHalfBitsToFloat(PackRg >> 16u), UnpackHalfBitsToFloat(PackBi & 0xFFFFu));
    Result.Sample.Identity = uint(UnpackHalfBitsToFloat(PackBi >> 16u) + 0.5f);
    Result.Weight          = asfloat(GiReservoirPrev[Base + 6u]);
    Result.Tally           = asfloat(GiReservoirPrev[Base + 7u]);
    return Result;
}

GiReservoir LoadGiReservoirCurr(uint Index)
{
    uint        Base   = Index * 8u;
    GiReservoir Result = ConstructEmptyGiReservoir();
    Result.Sample.Position = float3(asfloat(GiReservoirCurr[Base + 0u]),
                                    asfloat(GiReservoirCurr[Base + 1u]),
                                    asfloat(GiReservoirCurr[Base + 2u]));
    Result.Sample.Normal   = DecodeOctahedral(UnpackGiOctahedral(GiReservoirCurr[Base + 3u]));
    uint PackRg            = GiReservoirCurr[Base + 4u];
    uint PackBi            = GiReservoirCurr[Base + 5u];
    Result.Sample.Radiance = float3(UnpackHalfBitsToFloat(PackRg & 0xFFFFu), UnpackHalfBitsToFloat(PackRg >> 16u), UnpackHalfBitsToFloat(PackBi & 0xFFFFu));
    Result.Sample.Identity = uint(UnpackHalfBitsToFloat(PackBi >> 16u) + 0.5f);
    Result.Weight          = asfloat(GiReservoirCurr[Base + 6u]);
    Result.Tally           = asfloat(GiReservoirCurr[Base + 7u]);
    return Result;
}

// Weighted reservoir sampling (MergeReservoir in ReservoirRecords.wgsl): keep candidate with probability w / sum(w).
bool MergeGiReservoir(inout GiReservoir Target, GiReservoirSample Candidate, float Weight, float Tally, float Random)
{
    Target.WeightSum += Weight;
    Target.Tally     += Tally;
    if (Weight > 0.0f && Random * Target.WeightSum < Weight)
    {
        Target.Sample = Candidate;
        return true;
    }
    return false;
}

// Target function p(y) = luminance(L(y)) * max(0, n_v . w) (ResolveReservoirTarget in ReservoirRecords.wgsl).
float ResolveGiReservoirTarget(GiReservoirSample Sample, float3 Position, float3 Normal)
{
    float Cosine = dot(Normal, normalize(Sample.Position - Position));
    return (Cosine > 0.0f) ? Luminance709(Sample.Radiance) * Cosine : 0.0f;
}

// Binary visibility between a shading point and a reconnection vertex (TraverseSceneVisibility parity).
// GTX mode marches the hardware 3D SDF; RTX mode traces the CWBVH. Returns 1 visible / 0 occluded.
float TraceGiVisibility(float3 Position, float3 Normal, float3 TargetPos, bool TargetIsSky, bool FullRt)
{
    float3 Toward = TargetPos - Position;
    float  Length = length(Toward);
    if (Length < 1.0e-4f)
    {
        return 1.0f;
    }
    float3 Dir   = Toward / Length;
    float  Reach = TargetIsSky ? 300.0f : max(Length - 0.03f, 0.0f);
    if (Reach <= 0.0f)
    {
        return 1.0f;
    }
    if (FullRt)
    {
        UnifiedOpaqueHit Blocker = TraceSceneOpaque(Position + Normal * 0.015f, Dir, 1.0e-3f, Reach);
        return Blocker.Hit ? 0.0f : 1.0f;
    }
    bool ClipmapActive = (FrameHeader[12].y > 0.5f);
    HardwareSdfHit Blocker = TraverseHardwareSdfClipmap(
        SdfClipmapVolume, DistanceOrRangeTable, ClipmapActive, FrameHeader[19].z, Position, Normal, Dir, Reach);
    return Blocker.Hit ? 0.0f : 1.0f;
}

// Traces the frame's single cosine candidate and shades its reconnection vertex (sample exitance / sky on escape).
GiReservoirSample TraceGiInitialCandidate(float3 Position, float3 Normal, float3 RayDir, bool FullRt)
{
    GiReservoirSample Candidate;
    float             SkyWeight  = FrameHeader[13].w;
    uint              PatchCount = uint(FrameHeader[12].w + 0.5f);
    float             CardGain   = FrameHeader[13].z;

    if (FullRt)
    {
        UnifiedOpaqueHit Hit = TraceSceneOpaque(Position + Normal * 0.015f, RayDir, 1.0e-3f, 220.0f);
        if (Hit.Hit)
        {
            Candidate.Position = Hit.WorldPos;
            Candidate.Normal   = Hit.GeoNormal;
            Candidate.Identity = 1u;
            Candidate.Radiance = min(ShadeSecondarySurface(Hit.WorldPos, Hit.TexCoord, Hit.DistanceM, Hit.GeoNormal,
                                                           Hit.ClassId, -RayDir, Hit.InstanceTint,
                                                           Hit.InstanceSelfRadiance, true),
                                     float3(32.0f));
            return Candidate;
        }
    }
    else if (FrameHeader[7].z > 0.5f)
    {
        bool           ClipmapActive = (FrameHeader[12].y > 0.5f);
        HardwareSdfHit SdfHit        = TraverseHardwareSdfClipmap(
            SdfClipmapVolume, DistanceOrRangeTable, ClipmapActive, FrameHeader[19].z, Position, Normal, RayDir, 180.0f);
        if (SdfHit.Hit)
        {
            float3 IncomingLi = float3(0.0f);
            if (PatchCount > 0u && CardGain > 0.0f)
            {
                IncomingLi = SampleGpuCombinedSurfaceRadiosity(SurfaceCardPack, PatchCount, SdfHit.WorldPos, SdfHit.Normal) * CardGain;
            }
            uint LumCount = uint(FrameHeader[3].w + 0.5f);
            if (LumCount > 0u)
            {
                uint   LumBase = ResolveLuminaireBaseSlot();
                float4 Sun0    = FrameHeader[LumBase + 0u];
                if (uint(Sun0.w + 0.5f) == 0u)
                {
                    float3 SunDir = normalize(Sun0.xyz);
                    float  SunNdL = max(0.0f, dot(SdfHit.Normal, SunDir));
                    if (SunNdL > 0.0f)
                    {
                        float CamDist = length(SdfHit.WorldPos - FrameHeader[0].xyz);
                        float SunVis  = ResolveDeterministicSolarVisibility(
                            ShadowDepthArray, FrameHeader, SdfHit.WorldPos, SdfHit.Normal, CamDist);
                        IncomingLi += FrameHeader[LumBase + 3u].xyz * (0.24f * SunVis * SunNdL);
                    }
                }
            }
            Candidate.Position = SdfHit.WorldPos;
            Candidate.Normal   = SdfHit.Normal;
            Candidate.Identity = 1u;
            Candidate.Radiance = min(IncomingLi, float3(32.0f));
            return Candidate;
        }
    }

    Candidate.Position = Position + RayDir * kGiSkyReachM;
    Candidate.Normal   = -RayDir;
    Candidate.Identity = 0u;
    Candidate.Radiance = EvaluateCoupledEnvironmentSky(RayDir) * SkyWeight;
    return Candidate;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void TraceIndirectDiffuseMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint FullW      = Push.Counts.x;
    uint FullH      = Push.Counts.y;
    uint TraceScale = ResolveGiTraceScale();
    uint TraceW     = (FullW + TraceScale - 1u) / TraceScale;
    uint TraceH     = (FullH + TraceScale - 1u) / TraceScale;

    if (DispatchId.x >= TraceW || DispatchId.y >= TraceH)
    {
        return;
    }

    uint2 TracePx  = DispatchId.xy;
    uint  ResIndex = TracePx.y * TraceW + TracePx.x;
    uint2 ScreenPx = min(TracePx * TraceScale, uint2(FullW - 1u, FullH - 1u));
    float4 G0      = SurfaceAndWorkAtlas[uint3(ScreenPx, 0u)];

    // On half-res silhouette quads, pick the first valid opaque sub-pixel if top-left corner hit sky/glass
    if (G0.w <= 0.0f && TraceScale > 1u)
    {
        for (uint Sub = 1u; Sub < 4u; ++Sub)
        {
            uint2 CandPx = min(TracePx * TraceScale + uint2(Sub & 1u, Sub >> 1u), uint2(FullW - 1u, FullH - 1u));
            float4 CG0   = SurfaceAndWorkAtlas[uint3(CandPx, 0u)];
            if (CG0.w > 0.0f)
            {
                ScreenPx = CandPx;
                G0       = CG0;
                break;
            }
        }
    }

    if (G0.w <= 0.0f)
    {
        SurfaceAndWorkAtlas[uint3(TracePx, 7u)]  = float4(0.0f, 0.0f, 0.0f, 1.0f);
        SurfaceAndWorkAtlas[uint3(TracePx, 13u)] = float4(0.0f);
        StoreGiReservoir(ResIndex, ConstructEmptyGiReservoir());
        return;
    }

    float3 WorldPos = G0.xyz;
    float  HitT     = G0.w;
    float3 N, T, B;
    UnpackedSurfaceMaterial Mat = ReconstructGbufferMaterial(ScreenPx, N, T, B);
    uint ClassId                = uint(max(0.0f, SurfaceAndWorkAtlas[uint3(ScreenPx, 2u)].w) + 0.5f);

    SurfaceAndWorkAtlas[uint3(TracePx, 13u)] = float4(EncodeOctahedral(N), HitT, float(ClassId + 1u));

    float SceneAo = EvaluateSceneDistanceAO(WorldPos, N) * Mat.AmbientOcclusion;
    if (FrameHeader[7].w < 0.5f)
    {
        SurfaceAndWorkAtlas[uint3(TracePx, 7u)] = float4(0.0f, 0.0f, 0.0f, SceneAo);
        StoreGiReservoir(ResIndex, ConstructEmptyGiReservoir());
        return;
    }

    // AO is carried in layer 7.w for the spatial resolve (rgb is written there after reuse)
    SurfaceAndWorkAtlas[uint3(TracePx, 7u)] = float4(0.0f, 0.0f, 0.0f, SceneAo);

    bool FullRt = IsFullRaytracingEnabled() || FrameHeader[7].z < 0.5f; // no SDF volume -> BVH candidate (demo parity fallback)
    uint Seed   = SeedPixelStream(TracePx, Push.Counts.z, 11u);

    // (2) Initial candidate: exactly one cosine-weighted ray (source pdf = cos(theta)/pi)
    float2            Xi        = SampleR2Sequence(TracePx, Push.Counts.z, 5u);
    float3            RayDir    = SampleCosineHemisphere(N, T, B, Xi);
    GiReservoirSample Candidate = TraceGiInitialCandidate(WorldPos, N, RayDir, FullRt);
    float             Cosine    = max(dot(N, RayDir), 0.0f);
    float             SourcePdf = Cosine * 0.31830989f; // cos / pi
    GiReservoir       Current   = ConstructEmptyGiReservoir();
    float InitialWeight = (SourcePdf > 1.0e-6f) ? (Luminance709(Candidate.Radiance) * Cosine / SourcePdf) : 0.0f;
    MergeGiReservoir(Current, Candidate, InitialWeight, 1.0f, SampleUniform01(Seed));

    // (3) Temporal reuse: reproject into the prior reservoir through the previous view-projection
    if (Push.Params.w > 0.5f)
    {
        float2 PrevScreenPos;
        float2 MotionUv;
        if (ProjectToPreviousScreenPosition(WorldPos, FullW, FullH, PrevScreenPos, MotionUv, int2(ScreenPx)))
        {
            int2 PriorTrace = int2(floor((PrevScreenPos - 0.5f) / float(TraceScale) + 0.5f));
            if (all(PriorTrace >= int2(0)) && all(PriorTrace < int2(int(TraceW), int(TraceH))))
            {
                float4 PriorGuide  = TemporalHistoryPrev[uint3(uint2(PriorTrace), 2u)];
                float3 PriorNormal = DecodeOctahedral(PriorGuide.xy);
                bool   Similar     = (PriorGuide.w > 0.5f) && (dot(PriorNormal, N) > 0.85f) &&
                                     (abs(PriorGuide.z - HitT) <= (0.10f * HitT + 0.05f));
                if (Similar)
                {
                    GiReservoir Prior = LoadGiReservoirPrev(uint(PriorTrace.y) * TraceW + uint(PriorTrace.x));
                    if (Prior.Tally > 0.0f && Prior.Weight > 0.0f)
                    {
                        float Target = ResolveGiReservoirTarget(Prior.Sample, WorldPos, N);
                        // Rotating quarter of pixels re-traces visibility so movers darken reuse instead of ghosting
                        bool Validate = (((TracePx.x + TracePx.y * 3u + Push.Counts.z) & 3u) == 0u);
                        if (Validate && Target > 0.0f)
                        {
                            Target *= TraceGiVisibility(WorldPos, N, Prior.Sample.Position,
                                                        Prior.Sample.Identity == 0u, FullRt);
                        }
                        float Tally = min(Prior.Tally, clamp(FrameHeader[13].y, 1.0f, 64.0f));
                        MergeGiReservoir(Current, Prior.Sample, Target * Prior.Weight * Tally, Tally, SampleUniform01(Seed));
                    }
                }
            }
        }
    }

    // (4) Unbiased contribution weight W = sum(w) / (M * p(y))
    float Selected = ResolveGiReservoirTarget(Current.Sample, WorldPos, N);
    Current.Weight = (Selected > 0.0f && Current.Tally > 0.0f) ? (Current.WeightSum / (Current.Tally * Selected)) : 0.0f;
    StoreGiReservoir(ResIndex, Current);
}

// Reconnection-shift Jacobian |dWr/dWq| = (cos_r / cos_q) * (|xq-xs|^2 / |xv-xs|^2); sky samples are directional (1).
float ResolveGiReconnectionJacobian(GiReservoirSample Sample, float3 Receiver, float3 Donor)
{
    if (Sample.Identity == 0u)
    {
        return 1.0f;
    }
    float3 ToDonor        = Donor - Sample.Position;
    float3 ToReceiver     = Receiver - Sample.Position;
    float  DonorSq        = max(dot(ToDonor, ToDonor), 1.0e-6f);
    float  ReceiverSq     = max(dot(ToReceiver, ToReceiver), 1.0e-6f);
    float  CosineDonor    = abs(dot(Sample.Normal, ToDonor)) * rsqrt(DonorSq);
    float  CosineReceiver = abs(dot(Sample.Normal, ToReceiver)) * rsqrt(ReceiverSq);
    return (CosineReceiver / max(CosineDonor, 1.0e-4f)) * (DonorSq / ReceiverSq);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ResampleGiSpatialMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint FullW      = Push.Counts.x;
    uint FullH      = Push.Counts.y;
    uint TraceScale = ResolveGiTraceScale();
    uint TraceW     = (FullW + TraceScale - 1u) / TraceScale;
    uint TraceH     = (FullH + TraceScale - 1u) / TraceScale;

    if (DispatchId.x >= TraceW || DispatchId.y >= TraceH)
    {
        return;
    }

    uint2  TracePx = DispatchId.xy;
    float4 Guide   = SurfaceAndWorkAtlas[uint3(TracePx, 13u)];
    if (Guide.w < 0.5f || FrameHeader[7].w < 0.5f)
    {
        return; // sky/glass texel or GI disabled: layer 7 already carries zeros + AO from the temporal kernel
    }

    float3 Normal   = DecodeOctahedral(Guide.xy);
    float  ViewDepth = Guide.z;
    uint2  ScreenPx = min(TracePx * TraceScale, uint2(FullW - 1u, FullH - 1u));
    float3 Position = SurfaceAndWorkAtlas[uint3(ScreenPx, 0u)].xyz;
    bool   FullRt   = IsFullRaytracingEnabled() || FrameHeader[7].z < 0.5f;
    uint   Seed     = SeedPixelStream(TracePx, Push.Counts.z, 23u);

    // (1) Own temporal reservoir seeds the spatial reservoir
    GiReservoir Own     = LoadGiReservoirCurr(TracePx.y * TraceW + TracePx.x);
    GiReservoir Spatial = ConstructEmptyGiReservoir();
    MergeGiReservoir(Spatial, Own.Sample,
                     ResolveGiReservoirTarget(Own.Sample, Position, Normal) * Own.Weight * Own.Tally,
                     Own.Tally, SampleUniform01(Seed));

    // (2) Disk proposals (SpatialNeighbors from FrameHeader[5].w, 12 trace-px radius like the demo default)
    float3 DonorPositions[8];
    float3 DonorNormals[8];
    float  DonorTallies[8];
    uint   DonorCount = 0u;
    if (FrameHeader[7].y > 0.5f)
    {
        uint Count = min(uint(FrameHeader[5].w + 0.5f), 8u);
        for (uint Proposal = 0u; Proposal < Count; ++Proposal)
        {
            float  Angle  = SampleUniform01(Seed) * 6.2831853f;
            float  Radius = sqrt(SampleUniform01(Seed)) * 12.0f;
            int2   Donor  = clamp(int2(TracePx) + int2(round(float2(cos(Angle), sin(Angle)) * Radius)),
                                  int2(0), int2(int(TraceW) - 1, int(TraceH) - 1));
            if (all(Donor == int2(TracePx)))
            {
                continue;
            }
            float4 DonorGuide  = SurfaceAndWorkAtlas[uint3(uint2(Donor), 13u)];
            float3 DonorNormal = DecodeOctahedral(DonorGuide.xy);
            if (DonorGuide.w < 0.5f || dot(DonorNormal, Normal) < 0.9f || abs(DonorGuide.z - ViewDepth) > 0.1f * ViewDepth)
            {
                continue;
            }
            GiReservoir Reservoir = LoadGiReservoirCurr(uint(Donor.y) * TraceW + uint(Donor.x));
            if (Reservoir.Tally <= 0.0f || Reservoir.Weight <= 0.0f)
            {
                continue;
            }
            uint2 DonorScreen   = min(uint2(Donor) * TraceScale, uint2(FullW - 1u, FullH - 1u));
            float3 DonorPosition = SurfaceAndWorkAtlas[uint3(DonorScreen, 0u)].xyz;
            float Jacobian      = ResolveGiReconnectionJacobian(Reservoir.Sample, Position, DonorPosition);
            if (!(Jacobian >= 0.125f && Jacobian <= 8.0f)) // also refuses NaN
            {
                continue;
            }
            float Target = ResolveGiReservoirTarget(Reservoir.Sample, Position, Normal);
            if (MergeGiReservoir(Spatial, Reservoir.Sample, Target * Reservoir.Weight * Reservoir.Tally * Jacobian,
                                 Reservoir.Tally, SampleUniform01(Seed)))
            {
                Spatial.Adopted = true;
            }
            DonorPositions[DonorCount] = DonorPosition;
            DonorNormals[DonorCount]   = DonorNormal;
            DonorTallies[DonorCount]   = Reservoir.Tally;
            DonorCount++;
        }
    }

    // (4) An adopted sample must be visible from this surface, else fall back to the own reservoir
    if (Spatial.Adopted)
    {
        if (TraceGiVisibility(Position, Normal, Spatial.Sample.Position, Spatial.Sample.Identity == 0u, FullRt) < 0.5f)
        {
            Spatial           = Own;
            Spatial.WeightSum = ResolveGiReservoirTarget(Own.Sample, Position, Normal) * Own.Weight * Own.Tally;
            Spatial.Adopted   = false;
            DonorCount        = 0u;
        }
    }

    // (3) 1/Z bias correction over every domain that could have produced the chosen sample
    GiReservoirSample Chosen = Spatial.Sample;
    float Z = (ResolveGiReservoirTarget(Chosen, Position, Normal) > 0.0f) ? Own.Tally : 0.0f;
    for (uint Donor = 0u; Donor < DonorCount; ++Donor)
    {
        if (ResolveGiReservoirTarget(Chosen, DonorPositions[Donor], DonorNormals[Donor]) > 0.0f)
        {
            Z += DonorTallies[Donor];
        }
    }
    float Target = ResolveGiReservoirTarget(Chosen, Position, Normal);
    float Weight = (Target > 0.0f && Z > 0.0f) ? (Spatial.WeightSum / (Z * Target)) : 0.0f;

    // (5) Resolve demodulated irradiance E = L(y) * cos * W / pi (matching this renderer's GI demodulation convention)
    float  Cosine     = max(dot(Normal, normalize(Chosen.Position - Position)), 0.0f);
    float3 Irradiance = Chosen.Radiance * Cosine * Weight * 0.31830989f;
    float  Lum        = Luminance709(Irradiance);
    if (Lum > 20.0f)
    {
        Irradiance *= (20.0f / Lum);
    }

    // Coat Beer-Lambert attenuation & metallic diffuse suppression (albedo-demodulated!)
    float3 N2, T2, B2;
    UnpackedSurfaceMaterial Mat = ReconstructGbufferMaterial(ScreenPx, N2, T2, B2);
    float3 ViewDir   = normalize(FrameHeader[0].xyz - Position);
    float  NdotV     = max(0.05f, dot(Normal, ViewDir));
    float3 CoatTrans = float3(1.0f);
    if (Mat.CoatCoverage > 0.01f)
    {
        float CoatF = EvaluateDielectricFresnelUnpolarized(NdotV, 1.0f, Mat.CoatIor);
        CoatTrans   = lerp(float3(1.0f), (1.0f - CoatF) * exp(-Mat.CoatOpticalDepth * (1.0f / NdotV)), Mat.CoatCoverage);
    }
    float3 DiffModulation = (1.0f - Mat.Metallic) * CoatTrans;
    float  SceneAo        = SurfaceAndWorkAtlas[uint3(TracePx, 7u)].w;
    SurfaceAndWorkAtlas[uint3(TracePx, 7u)] = float4(min(Irradiance * DiffModulation * SceneAo, float3(8.0f)), SceneAo);
}

//------------------------------------------------------------------------------------------------------------------------
// KERNEL 5: RAYTRACED REFLECTION TRAVERSAL (HEITZ 2018 VNDF CWBVH RAYTRACE + SURFACECARDS)
//------------------------------------------------------------------------------------------------------------------------

[shader("compute")]
[numthreads(8, 8, 1)]
void TraceSpecularReflectionsMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint Width  = Push.Counts.x;
    uint Height = Push.Counts.y;
    if (DispatchId.x >= Width || DispatchId.y >= Height)
    {
        return;
    }

    uint2  Px = DispatchId.xy;
    float4 G0 = SurfaceAndWorkAtlas[uint3(Px, 0u)];

    // Sky miss or resolved curved glazing passes straight into Specular layer 8
    if (G0.w <= 0.0f)
    {
        float3 SkyOrGlass = (G0.w == 0.0f)
                                ? EvaluateCoupledEnvironmentSky(ReconstructPrimaryRayDirection(Px, Width, Height))
                                : SurfaceAndWorkAtlas[uint3(Px, 2u)].xyz;
        SurfaceAndWorkAtlas[uint3(Px, 8u)] = float4(SkyOrGlass, 0.0f);
        return;
    }

    if (FrameHeader[7].x < 0.5f)
    {
        SurfaceAndWorkAtlas[uint3(Px, 8u)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float3 WorldPos = G0.xyz;
    float3 ViewDir  = normalize(FrameHeader[0].xyz - WorldPos);
    float3 N, T, B;
    UnpackedSurfaceMaterial Mat = ReconstructGbufferMaterial(Px, N, T, B);

    bool  FullRt       = IsFullRaytracingEnabled();
    float EffRoughness = (Mat.CoatCoverage > 0.25f) ? min(Mat.BaseRoughness, Mat.CoatRoughness) : Mat.BaseRoughness;

    // Hybrid Mode rough-surface optimization (matches ReflectionTraversal.wgsl):
    // Rough matte surfaces (> 0.52 without clearcoat) reuse upsampled diffuse irradiance * EnvBRDF in the resolve pass!
    if (!FullRt && Mat.CoatCoverage < 0.02f && Mat.BaseRoughness > 0.52f)
    {
        SurfaceAndWorkAtlas[uint3(Px, 8u)] = float4(0.0f, 0.0f, 0.0f, -1.0f);
        return;
    }

    float  NdotV      = max(1.0e-4f, dot(N, ViewDir));
    float  DielecF0   = pow((Mat.BaseIor - 1.0f) / (Mat.BaseIor + 1.0f), 2.0f);
    float3 BaseF0     = lerp(float3(DielecF0), Mat.BaseAlbedo, Mat.Metallic);
    float  CoatFres   = Mat.CoatCoverage * EvaluateDielectricFresnelUnpolarized(NdotV, 1.0f, Mat.CoatIor);
    float  BaseWeight = (1.0f - CoatFres) * max(0.08f, Luminance709(BaseF0));
    float  CoatProb   = (Mat.CoatCoverage > 0.01f) ? clamp(CoatFres / max(1.0e-4f, CoatFres + BaseWeight), 0.15f, 0.90f) : 0.0f;

    uint   Rng        = SeedPixelStream(Px, Push.Counts.z, 19u);
    bool   SampleCoat = (SampleUniform01(Rng) < CoatProb);
    float  LobeRough  = SampleCoat ? Mat.CoatRoughness : Mat.BaseRoughness;

    float3 ReflDir;
    if (LobeRough < 0.055f)
    {
        ReflDir = reflect(-ViewDir, N);
    }
    else
    {
        float2 Xi        = SampleR2Sequence(Px, Push.Counts.z, 23u);
        float3 HalfWorld = SampleGgxVndfHalfVector(ViewDir, T, B, N, LobeRough, SampleCoat ? 0.0f : Mat.Anisotropy, Xi);
        ReflDir          = reflect(-ViewDir, HalfWorld);
    }

    if (dot(N, ReflDir) <= 1.0e-4f)
    {
        SurfaceAndWorkAtlas[uint3(Px, 8u)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    // Raytraced reflection against Two-Level 8-Wide CWBVH triangles (OctetVolumeSearch)
    UnifiedOpaqueHit ReflHit = TraceSceneOpaque(WorldPos + N * 0.014f, ReflDir, 1.0e-3f, 500.0f);
    float3           ReflLi  = ReflHit.Hit
                                   ? ApplyCoupledHeightFog(
                                         ShadeSecondarySurface(ReflHit.WorldPos, ReflHit.TexCoord, ReflHit.DistanceM, ReflHit.GeoNormal, ReflHit.ClassId, -ReflDir, ReflHit.InstanceTint, ReflHit.InstanceSelfRadiance, FullRt),
                                         WorldPos, ReflDir, ReflHit.DistanceM)
                                   : EvaluateCoupledEnvironmentSky(ReflDir);

    float3 HalfR = normalize(ViewDir + ReflDir);
    float  VdotH = saturate(dot(ViewDir, HalfR));
    float  NdotL = saturate(dot(N, ReflDir));
    float3 Throughput;
    if (SampleCoat)
    {
        float Fc   = Mat.CoatCoverage * EvaluateDielectricFresnelUnpolarized(VdotH, 1.0f, Mat.CoatIor);
        float G1L  = 2.0f * NdotL / max(1.0e-4f, NdotL + sqrt(Mat.CoatRoughness * Mat.CoatRoughness + (1.0f - Mat.CoatRoughness * Mat.CoatRoughness) * NdotL * NdotL));
        Throughput = float3(Fc * G1L / max(0.15f, CoatProb));
    }
    else
    {
        float3 Fb   = BaseF0 + (1.0f - BaseF0) * pow(1.0f - VdotH, 5.0f);
        float  G1L  = 2.0f * NdotL / max(1.0e-4f, NdotL + sqrt(Mat.BaseRoughness * Mat.BaseRoughness + (1.0f - Mat.BaseRoughness * Mat.BaseRoughness) * NdotL * NdotL));
        float  AttV = 1.0f - CoatFres;
        float  AttL = 1.0f - Mat.CoatCoverage * EvaluateDielectricFresnelUnpolarized(NdotL, 1.0f, Mat.CoatIor);
        float3 Beer = (Mat.CoatCoverage > 0.01f)
                          ? lerp(float3(1.0f), exp(-Mat.CoatOpticalDepth * (1.0f / NdotV + 1.0f / max(0.08f, NdotL))), Mat.CoatCoverage)
                          : float3(1.0f);
        Throughput  = Fb * Beer * (AttV * AttL * G1L / max(0.10f, 1.0f - CoatProb));
    }

    float3 SpecRad  = ReflLi * Throughput * (0.45f + 0.55f * Mat.AmbientOcclusion);
    float  SpecLuma = Luminance709(SpecRad);
    if (SpecLuma > 14.0f)
    {
        SpecRad *= (14.0f / SpecLuma);
    }
    SurfaceAndWorkAtlas[uint3(Px, 8u)] = float4(SpecRad, max(0.05f, ReflHit.Hit ? ReflHit.DistanceM : 200.0f));
}

//------------------------------------------------------------------------------------------------------------------------
// KERNEL 6: SVGF VARIANCE TEMPORAL INTEGRATOR (VarianceTemporal.wgsl PARITY FOR HALF-RES GI & FULL-RES REFLECTIONS)
//------------------------------------------------------------------------------------------------------------------------

[shader("compute")]
[numthreads(8, 8, 1)]
void IntegrateTemporalVarianceMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint Width      = Push.Counts.x;
    uint Height     = Push.Counts.y;
    uint TraceScale = ResolveGiTraceScale();
    uint TraceW     = (Width + TraceScale - 1u) / TraceScale;
    uint TraceH     = (Height + TraceScale - 1u) / TraceScale;

    if (DispatchId.x >= Width || DispatchId.y >= Height)
    {
        return;
    }

    uint2 Px          = DispatchId.xy;
    float MaxHistoryM = clamp(FrameHeader[13].y, 1.0f, 64.0f);
    bool  UseTemporal = (Push.Params.w > 0.5f);

    // 1. Integrate Half-Res (or Full-Res) Diffuse GI + Moments when Px is within TraceExtent
    if (Px.x < TraceW && Px.y < TraceH)
    {
        float4 CurrGuide  = SurfaceAndWorkAtlas[uint3(Px, 13u)];
        float4 RawDiff    = SurfaceAndWorkAtlas[uint3(Px, 7u)];
        float3 Signal     = RawDiff.xyz;
        float  SceneAo    = RawDiff.w;
        float  Luma       = Luminance709(Signal);

        if (CurrGuide.w < 0.5f)
        {
            SurfaceAndWorkAtlas[uint3(Px, 9u)]  = float4(0.0f);
            TemporalHistoryCurr[uint3(Px, 0u)]  = float4(0.0f);
            TemporalHistoryCurr[uint3(Px, 1u)]  = float4(0.0f, 0.0f, 0.0f, 1.0f);
            TemporalHistoryCurr[uint3(Px, 2u)]  = float4(0.0f);
        }
        else
        {
            float3 CurrN    = DecodeOctahedral(CurrGuide.xy);
            float  CurrZ    = CurrGuide.z;
            uint2  ScreenPx = min(Px * TraceScale, uint2(Width - 1u, Height - 1u));
            float3 WorldPos = SurfaceAndWorkAtlas[uint3(ScreenPx, 0u)].xyz;
            if (SurfaceAndWorkAtlas[uint3(ScreenPx, 0u)].w <= 0.0f && TraceScale > 1u)
            {
                for (uint Sub = 1u; Sub < 4u; ++Sub)
                {
                    uint2 Cand = min(Px * TraceScale + uint2(Sub & 1u, Sub >> 1u), uint2(Width - 1u, Height - 1u));
                    if (SurfaceAndWorkAtlas[uint3(Cand, 0u)].w > 0.0f)
                    {
                        WorldPos = SurfaceAndWorkAtlas[uint3(Cand, 0u)].xyz;
                        break;
                    }
                }
            }

            float2 PrevScreenPos = float2(ScreenPx) + 0.5f;
            float2 MotionUv      = float2(0.0f);
            bool   InBounds      = UseTemporal && ProjectToPreviousScreenPosition(WorldPos, Width, Height, PrevScreenPos, MotionUv, int2(ScreenPx));
            float3 PriorColour = float3(0.0f);
            float3 PriorMoment = float3(0.0f);
            float  PriorAo     = 0.0f;
            float  TotalWeight = 0.0f;

            if (InBounds)
            {
                float2 PriorTracePos = PrevScreenPos / float(TraceScale) - 0.5f;
                int2   Corner        = int2(floor(PriorTracePos));
                float2 Frac          = PriorTracePos - float2(Corner);

                for (uint Tap = 0u; Tap < 4u; ++Tap)
                {
                    int2 Offset = int2(int(Tap & 1u), int(Tap >> 1u));
                    int2 Texel  = Corner + Offset;
                    if (Texel.x < 0 || Texel.y < 0 || Texel.x >= int(TraceW) || Texel.y >= int(TraceH))
                    {
                        continue;
                    }
                    float4 PGuide = TemporalHistoryPrev[uint3(uint2(Texel), 2u)];
                    bool   BadTap = (PGuide.w < 0.5f) || (dot(DecodeOctahedral(PGuide.xy), CurrN) < 0.85f) ||
                                    (abs(PGuide.z - CurrZ) > (0.10f * CurrZ + 0.05f));
                    if (BadTap)
                    {
                        continue;
                    }
                    float W      = ((Offset.x == 1) ? Frac.x : (1.0f - Frac.x)) * ((Offset.y == 1) ? Frac.y : (1.0f - Frac.y));
                    PriorColour += TemporalHistoryPrev[uint3(uint2(Texel), 0u)].xyz * W;
                    float4 PMom  = TemporalHistoryPrev[uint3(uint2(Texel), 1u)];
                    PriorMoment += PMom.xyz * W;
                    PriorAo     += PMom.w * W;
                    TotalWeight += W;
                }
            }

            float  Tally   = 1.0f;
            float3 Colour  = Signal;
            float2 Moments = float2(Luma, Luma * Luma);
            float  OutAo   = SceneAo;

            if (TotalWeight > 0.01f)
            {
                PriorColour /= TotalWeight;
                PriorMoment /= TotalWeight;
                PriorAo     /= TotalWeight;
                Tally        = min(PriorMoment.z + 1.0f, MaxHistoryM);
                float Alpha  = max(1.0f / Tally, 0.04f);
                Colour       = lerp(PriorColour, Signal, Alpha);
                Moments      = lerp(PriorMoment.xy, Moments, max(Alpha, 0.20f));
                OutAo        = lerp(PriorAo, SceneAo, Alpha);
            }

            float Spread = max(0.0f, Moments.y - Moments.x * Moments.x);
            // SVGF 3x3 spatial moment bootstrap on newly disoccluded pixels (Tally < 4)
            if (Tally < 4.0f)
            {
                float2 SpatMom = float2(0.0f);
                float  SpatW   = 0.0f;
                for (int Ky = -1; Ky <= 1; ++Ky)
                {
                    for (int Kx = -1; Kx <= 1; ++Kx)
                    {
                        int2 TPx = int2(Px) + int2(Kx, Ky);
                        if (TPx.x < 0 || TPx.y < 0 || TPx.x >= int(TraceW) || TPx.y >= int(TraceH)) continue;
                        float4 TG = SurfaceAndWorkAtlas[uint3(uint2(TPx), 13u)];
                        if (TG.w < 0.5f) continue;
                        float W = pow(max(0.0f, dot(CurrN, DecodeOctahedral(TG.xy))), 32.0f) *
                                  exp(-abs(TG.z - CurrZ) / (0.08f * CurrZ + 0.02f));
                        float TL = Luminance709(SurfaceAndWorkAtlas[uint3(uint2(TPx), 7u)].xyz);
                        SpatMom += float2(TL, TL * TL) * W;
                        SpatW   += W;
                    }
                }
                if (SpatW > 1.0e-4f)
                {
                    SpatMom /= SpatW;
                    Spread   = max(Spread, max(0.0f, SpatMom.y - SpatMom.x * SpatMom.x)) * (4.0f / max(1.0f, Tally));
                }
            }

            SurfaceAndWorkAtlas[uint3(Px, 9u)] = float4(Colour, Spread);
            TemporalHistoryCurr[uint3(Px, 0u)] = float4(Colour, Spread);
            TemporalHistoryCurr[uint3(Px, 1u)] = float4(Moments, Tally, OutAo);
            TemporalHistoryCurr[uint3(Px, 2u)] = CurrGuide;
        }
    }

    // 2. Integrate Full-Res Specular Reflections + Moments across ScreenExtent
    float4 G0   = SurfaceAndWorkAtlas[uint3(Px, 0u)];
    float4 RawS = SurfaceAndWorkAtlas[uint3(Px, 8u)];
    if (G0.w <= 0.0f || RawS.w < 0.0f)
    {
        // Sky, glazing, or rough-fallback marker passes through cleanly
        SurfaceAndWorkAtlas[uint3(Px, 11u)] = RawS;
        TemporalHistoryCurr[uint3(Px, 3u)]  = float4(RawS.xyz, 0.0f);
        TemporalHistoryCurr[uint3(Px, 4u)]  = float4(0.0f, 0.0f, 0.0f, 1.0f);
        TemporalHistoryCurr[uint3(Px, 5u)]  = float4(0.0f);
        return;
    }

    float4 G1           = SurfaceAndWorkAtlas[uint3(Px, 1u)];
    float4 G3           = SurfaceAndWorkAtlas[uint3(Px, 3u)];
    float3 CurrN        = normalize(G1.xyz);
    float  CurrZ        = G0.w;
    float  EffRoughness = (G3.y > 0.25f) ? min(G1.w, G3.z) : G1.w;
    float4 SpecGuide    = float4(EncodeOctahedral(CurrN), CurrZ, 1.0f + EffRoughness);
    float3 SpecSignal   = RawS.xyz;
    float  SpecLuma     = Luminance709(SpecSignal);

    float2 PrevScreenPos = float2(Px) + 0.5f;
    float2 MotionUv      = float2(0.0f);
    bool   InBounds      = UseTemporal && ProjectToPreviousScreenPosition(G0.xyz, Width, Height, PrevScreenPos, MotionUv, int2(Px));
    float3 PriorSpec = float3(0.0f);
    float3 PriorSMom = float3(0.0f);
    float  SpecWTot  = 0.0f;

    // Limit mirror history on fast camera/object motion so curved car reflections never ghost
    float MotionPx     = length(MotionUv * float2(float(Width), float(Height)));
    float SpecMaxTally = lerp(min(MaxHistoryM, 6.0f), MaxHistoryM, saturate(EffRoughness * 3.5f));
    if (EffRoughness < 0.12f && MotionPx > 1.5f)
    {
        SpecMaxTally = min(SpecMaxTally, 3.0f);
    }

    if (InBounds)
    {
        float2 PriorPos = PrevScreenPos - 0.5f;
        int2   Corner   = int2(floor(PriorPos));
        float2 Frac     = PriorPos - float2(Corner);

        for (uint Tap = 0u; Tap < 4u; ++Tap)
        {
            int2 Offset = int2(int(Tap & 1u), int(Tap >> 1u));
            int2 Texel  = Corner + Offset;
            if (Texel.x < 0 || Texel.y < 0 || Texel.x >= int(Width) || Texel.y >= int(Height)) continue;
            float4 PGuide = TemporalHistoryPrev[uint3(uint2(Texel), 5u)];
            bool   BadTap = (PGuide.w < 0.5f) || (dot(DecodeOctahedral(PGuide.xy), CurrN) < 0.92f) ||
                            (abs(PGuide.z - CurrZ) > (0.07f * CurrZ + 0.03f));
            if (BadTap)
            {
                continue;
            }
            float W    = ((Offset.x == 1) ? Frac.x : (1.0f - Frac.x)) * ((Offset.y == 1) ? Frac.y : (1.0f - Frac.y));
            PriorSpec += TemporalHistoryPrev[uint3(uint2(Texel), 3u)].xyz * W;
            PriorSMom += TemporalHistoryPrev[uint3(uint2(Texel), 4u)].xyz * W;
            SpecWTot  += W;
        }
    }

    float  SpecTally   = 1.0f;
    float3 SpecColour  = SpecSignal;
    float2 SpecMoments = float2(SpecLuma, SpecLuma * SpecLuma);
    if (SpecWTot > 0.01f)
    {
        PriorSpec  /= SpecWTot;
        PriorSMom  /= SpecWTot;
        SpecTally   = min(PriorSMom.z + 1.0f, SpecMaxTally);
        float Alpha = max(1.0f / SpecTally, 0.06f);
        SpecColour  = lerp(PriorSpec, SpecSignal, Alpha);
        SpecMoments = lerp(PriorSMom.xy, SpecMoments, max(Alpha, 0.25f));
    }
    float SpecSpread = max(0.0f, SpecMoments.y - SpecMoments.x * SpecMoments.x) * (4.0f / min(SpecTally, 4.0f));

    SurfaceAndWorkAtlas[uint3(Px, 11u)] = float4(SpecColour, SpecSpread);
    TemporalHistoryCurr[uint3(Px, 3u)]  = float4(SpecColour, SpecSpread);
    TemporalHistoryCurr[uint3(Px, 4u)]  = float4(SpecMoments, SpecTally, EffRoughness);
    TemporalHistoryCurr[uint3(Px, 5u)]  = SpecGuide;
}

//------------------------------------------------------------------------------------------------------------------------
// KERNEL 7: SVGF VARIANCE WAVELET + JOINT BILATERAL HALF-RES GI UPSAMPLE + 15 INSPECTION MODES + ACES DISPLAY OUTPUT
//------------------------------------------------------------------------------------------------------------------------

float3 ColorRampPlasma(float T)
{
    float X = saturate(T);
    return saturate(float3(
        0.05f + 2.15f * X - 1.20f * X * X,
        0.02f + 0.35f * X + 0.95f * X * X * X,
        0.53f + 1.10f * X - 1.45f * X * X));
}

float3 UpsampleHalfResDiffuseGi(uint2 ScreenPx, float3 FullNormal, float FullHitT, uint TraceScale, uint TraceW, uint TraceH, uint DiffLayer, out float OutAo, out float OutNormTally)
{
    if (TraceScale <= 1u)
    {
        float4 Mom   = TemporalHistoryCurr[uint3(ScreenPx, 1u)];
        OutAo        = Mom.w;
        OutNormTally = saturate(Mom.z / max(1.0f, FrameHeader[13].y));
        return SurfaceAndWorkAtlas[uint3(ScreenPx, DiffLayer)].xyz;
    }

    float2 Pos          = float2(ScreenPx) / float(TraceScale);
    int2   Corner       = int2(floor(Pos));
    float2 Frac         = Pos - float2(Corner);
    float3 Sum          = float3(0.0f);
    float  AoSum        = 0.0f;
    float  TallySum     = 0.0f;
    float  WeightSum    = 0.0f;
    float3 Nearest      = float3(0.0f);
    float  NearestAo    = 1.0f;
    float  NearestTally = 0.0f;
    float  NearestScore = 1.0e30f;

    for (uint Tap = 0u; Tap < 4u; ++Tap)
    {
        int2  Offset = int2(int(Tap & 1u), int(Tap >> 1u));
        uint2 Texel  = uint2(clamp(Corner + Offset, int2(0), int2(int(TraceW) - 1, int(TraceH) - 1)));
        float4 Guide = SurfaceAndWorkAtlas[uint3(Texel, 13u)];
        if (Guide.w < 0.5f)
        {
            continue;
        }
        float3 Sig      = SurfaceAndWorkAtlas[uint3(Texel, DiffLayer)].xyz;
        float4 Mom      = TemporalHistoryCurr[uint3(Texel, 1u)];
        float  DepthRel = abs(Guide.z - FullHitT) / max(FullHitT, 1.0e-3f);
        float  Facing   = max(0.0f, dot(DecodeOctahedral(Guide.xy), FullNormal));
        float  Bilinear = ((Offset.x == 1) ? Frac.x : (1.0f - Frac.x)) * ((Offset.y == 1) ? Frac.y : (1.0f - Frac.y));
        float  Weight   = (Bilinear + 1.0e-3f) * exp(-DepthRel * 36.0f) * pow(Facing, 16.0f);

        Sum       += Sig * Weight;
        AoSum     += Mom.w * Weight;
        TallySum  += Mom.z * Weight;
        WeightSum += Weight;

        float Score = DepthRel + (1.0f - Facing);
        if (Score < NearestScore)
        {
            NearestScore = Score;
            Nearest      = Sig;
            NearestAo    = Mom.w;
            NearestTally = Mom.z;
        }
    }

    float MaxHist = max(1.0f, FrameHeader[13].y);
    if (WeightSum > 1.0e-4f)
    {
        OutAo        = AoSum / WeightSum;
        OutNormTally = saturate((TallySum / WeightSum) / MaxHist);
        return Sum / WeightSum;
    }
    OutAo        = NearestAo;
    OutNormTally = saturate(NearestTally / MaxHist);
    return Nearest;
}

float3 ResolveInspectionOutput(uint2  Px,
                               uint   Width,
                               uint   Height,
                               uint   Mode,
                               float4 G0,
                               float4 G1,
                               float4 G2,
                               float4 G3,
                               float4 G4,
                               float4 G5,
                               float4 DirectAndShadow,
                               float3 UpsampledGi,
                               float3 ResolvedSpec,
                               float  SceneAo,
                               float  NormHistoryM)
{
    float3 CamOrig = FrameHeader[0].xyz;
    if (G0.w == 0.0f)
    {
        return (Mode == 0u || Mode == 2u) ? ResolvedSpec : (Mode == 9u ? float3(0.02f) : ResolvedSpec * 0.15f);
    }

    bool   IsGlass   = (G0.w < 0.0f);
    float  TravelM   = abs(G0.w);
    float3 WorldPos  = G0.xyz;
    float3 ViewRayD  = normalize(WorldPos - CamOrig);
    float3 BaseAlb   = G5.xyz;
    float3 SelfRad   = IsGlass ? float3(0.0f) : G2.xyz;
    float3 DiffGiRgb = BaseAlb * UpsampledGi;
    float3 LitLinear = IsGlass ? ResolvedSpec : (SelfRad + DirectAndShadow.xyz + DiffGiRgb + ResolvedSpec);
    LitLinear        = ApplyCoupledHeightFog(LitLinear, CamOrig, ViewRayD, TravelM);

    if (Mode == 0u) return LitLinear;
    if (Mode == 1u) return IsGlass ? float3(0.0f) : DiffGiRgb + DirectAndShadow.xyz * 0.35f;
    if (Mode == 2u) return ResolvedSpec;
    if (Mode == 3u) return float3(SceneAo);
    if (Mode == 4u)
    {
        uint PatchCnt = uint(FrameHeader[12].w + 0.5f);
        return (PatchCnt > 0u) ? SampleGpuCombinedSurfaceRadiosity(SurfaceCardPack, PatchCnt, WorldPos, normalize(G1.xyz)) : float3(0.0f);
    }
    if (Mode == 5u)
    {
        if (FrameHeader[7].z < 0.5f) return float3(0.25f, 0.55f, 0.90f);
        uint  ActiveR = 0u;
        float VoxStep = 0.25f;
        SampleHardwareSceneDistance(SdfClipmapVolume, DistanceOrRangeTable, FrameHeader[12].y > 0.5f, FrameHeader[19].z, WorldPos, ActiveR, VoxStep);
        float3 CascadePalette[5] = {
            float3(0.15f, 0.85f, 0.95f),
            float3(0.28f, 0.92f, 0.35f),
            float3(0.96f, 0.72f, 0.16f),
            float3(0.90f, 0.25f, 0.78f),
            float3(0.45f, 0.48f, 0.55f),
        };
        return CascadePalette[min(ActiveR, 4u)] * (0.4f + 0.6f * SceneAo);
    }
    if (Mode == 6u) return 0.5f * normalize(G1.xyz) + 0.5f;
    if (Mode == 7u) return BaseAlb;
    if (Mode == 8u) return float3(G1.w, G3.x, 0.5f * G4.w + 0.5f);
    if (Mode == 9u) return ColorRampPlasma(1.0f - exp(-TravelM * 0.035f));
    if (Mode == 10u) return ColorRampPlasma(1.0f - DirectAndShadow.w);
    if (Mode == 11u)
    {
        float2 PrevPos, MotionUv;
        bool   Valid = ProjectToPreviousScreenPosition(WorldPos, Width, Height, PrevPos, MotionUv, int2(Px));
        return Valid ? float3(saturate(0.5f + MotionUv.x * 12.0f), saturate(0.5f + MotionUv.y * 12.0f), 0.2f)
                     : float3(0.95f, 0.12f, 0.18f);
    }
    if (Mode == 12u) return ColorRampPlasma(NormHistoryM);
    if (Mode == 13u)
    {
        uint  Tier   = uint(floor(G3.w + 1.0e-4f));
        float MipBar = saturate(frac(G3.w) * (100.0f / 96.0f));
        float3 BaseC = (Tier == 0u) ? float3(0.20f, 0.88f, 0.42f) : ((Tier == 1u) ? float3(0.95f, 0.74f, 0.18f) : float3(0.68f, 0.30f, 0.92f));
        return lerp(BaseC, float3(0.95f, 0.22f, 0.28f), MipBar * 0.55f);
    }
    if (Mode == 14u) return IsGlass ? ResolvedSpec : LitLinear * 0.06f;
    return LitLinear;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void FilterSpatiotemporalAtrousMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint Width      = Push.Counts.x;
    uint Height     = Push.Counts.y;
    uint TraceScale = ResolveGiTraceScale();
    uint TraceW     = (Width + TraceScale - 1u) / TraceScale;
    uint TraceH     = (Height + TraceScale - 1u) / TraceScale;

    if (DispatchId.x >= Width || DispatchId.y >= Height)
    {
        return;
    }

    uint2 Px          = DispatchId.xy;
    uint  SrcParity   = Push.Counts.z & 1u;
    uint  DstParity   = 1u - SrcParity;
    bool  FinalIter   = (Push.Counts.w != 0u);
    int   StepSize    = int(Push.Params.y + 0.5f);
    float ColourSigma = max(0.5f, Push.Params.z);

    uint DiffSrcLayer = 9u + SrcParity;
    uint DiffDstLayer = 9u + DstParity;
    uint SpecSrcLayer = 11u + SrcParity;
    uint SpecDstLayer = 11u + DstParity;

    static const float B3Spline[3] = {0.375f, 0.25f, 0.0625f};

    // 1. VarianceWavelet 5x5 B3-spline on Half-Res (or Full-Res) Diffuse GI when !FinalIter
    //    (Note: when FinalIter == true, we convolve into DiffDstLayer only if Iterations == 1; otherwise we convolve first AND upsample?
    //     WAIT: another workgroup might read DiffDstLayer during upsample if we wrote and upsampled in the same dispatch!
    //     So FinalIter == 2u performs the final full-res upsample+compose pass cleanly after all wavelet iterations finish!)
    if (Push.Counts.w != 2u)
    {
        if (Px.x < TraceW && Px.y < TraceH)
        {
            float4 Guide  = SurfaceAndWorkAtlas[uint3(Px, 13u)];
            float4 Centre = SurfaceAndWorkAtlas[uint3(Px, DiffSrcLayer)];
            if (Guide.w < 0.5f)
            {
                SurfaceAndWorkAtlas[uint3(Px, DiffDstLayer)] = Centre;
            }
            else
            {
                float3 Normal    = DecodeOctahedral(Guide.xy);
                float  LumaC     = Luminance709(Centre.xyz);
                float  Deviation = sqrt(max(0.0f, Centre.w));
                float3 SumRgb    = float3(0.0f);
                float  WeightSum = 0.0f;
                float  SpreadSum = 0.0f;

                for (int Ky = -2; Ky <= 2; ++Ky)
                {
                    for (int Kx = -2; Kx <= 2; ++Kx)
                    {
                        int2 TapPx = int2(Px) + int2(Kx * StepSize, Ky * StepSize);
                        if (TapPx.x < 0 || TapPx.y < 0 || TapPx.x >= int(TraceW) || TapPx.y >= int(TraceH)) continue;
                        float4 TapGuide = SurfaceAndWorkAtlas[uint3(uint2(TapPx), 13u)];
                        if (TapGuide.w < 0.5f) continue;
                        float4 Tap     = SurfaceAndWorkAtlas[uint3(uint2(TapPx), DiffSrcLayer)];
                        float  Kernel  = B3Spline[abs(Kx)] * B3Spline[abs(Ky)];
                        float  WDepth  = exp(-abs(TapGuide.z - Guide.z) / (0.02f * float(StepSize) * Guide.z + 1.0e-3f));
                        float  WNorm   = pow(max(0.0f, dot(DecodeOctahedral(TapGuide.xy), Normal)), 64.0f);
                        float  WLuma   = exp(-abs(Luminance709(Tap.xyz) - LumaC) / (ColourSigma * Deviation + 1.0e-2f));
                        float  W       = Kernel * WDepth * WNorm * WLuma;
                        SumRgb        += Tap.xyz * W;
                        SpreadSum     += W * W * Tap.w;
                        WeightSum     += W;
                    }
                }
                SurfaceAndWorkAtlas[uint3(Px, DiffDstLayer)] =
                    (WeightSum > 1.0e-6f) ? float4(SumRgb / WeightSum, SpreadSum / (WeightSum * WeightSum)) : Centre;
            }
        }

        // Roughness-gated VarianceWavelet on Full-Res Specular Reflections (mirror clearcoat < 0.07 bypasses blur!)
        float4 G0      = SurfaceAndWorkAtlas[uint3(Px, 0u)];
        float4 CentreS = SurfaceAndWorkAtlas[uint3(Px, SpecSrcLayer)];
        if (G0.w <= 0.0f || SurfaceAndWorkAtlas[uint3(Px, 8u)].w < 0.0f)
        {
            SurfaceAndWorkAtlas[uint3(Px, SpecDstLayer)] = CentreS;
            return;
        }
        float4 G1           = SurfaceAndWorkAtlas[uint3(Px, 1u)];
        float4 G3           = SurfaceAndWorkAtlas[uint3(Px, 3u)];
        float  EffRoughness = (G3.y > 0.25f) ? min(G1.w, G3.z) : G1.w;
        float  SpecGate     = smoothstep(0.065f, 0.34f, EffRoughness);
        if (SpecGate <= 1.0e-3f)
        {
            SurfaceAndWorkAtlas[uint3(Px, SpecDstLayer)] = CentreS;
            return;
        }

        float3 NormalS   = normalize(G1.xyz);
        float  LumaS     = Luminance709(CentreS.xyz);
        float  DevS      = sqrt(max(0.0f, CentreS.w));
        float3 SumSpec   = float3(0.0f);
        float  WSpecSum  = 0.0f;
        float  VarSpec   = 0.0f;

        for (int Ky = -2; Ky <= 2; ++Ky)
        {
            for (int Kx = -2; Kx <= 2; ++Kx)
            {
                int2 TapPx = int2(Px) + int2(Kx * StepSize, Ky * StepSize);
                if (TapPx.x < 0 || TapPx.y < 0 || TapPx.x >= int(Width) || TapPx.y >= int(Height)) continue;
                float4 TapG0 = SurfaceAndWorkAtlas[uint3(uint2(TapPx), 0u)];
                if (TapG0.w <= 0.0f || SurfaceAndWorkAtlas[uint3(uint2(TapPx), 8u)].w < 0.0f) continue;
                float4 TapG1  = SurfaceAndWorkAtlas[uint3(uint2(TapPx), 1u)];
                float4 TapS   = SurfaceAndWorkAtlas[uint3(uint2(TapPx), SpecSrcLayer)];
                float  Kernel = B3Spline[abs(Kx)] * B3Spline[abs(Ky)];
                float  W      = Kernel;
                if (Kx != 0 || Ky != 0)
                {
                    float WDepth = exp(-abs(TapG0.w - G0.w) / (0.015f * float(StepSize) * G0.w + 1.0e-3f));
                    float WNorm  = pow(max(0.0f, dot(normalize(TapG1.xyz), NormalS)), lerp(256.0f, 64.0f, SpecGate));
                    float WLuma  = exp(-abs(Luminance709(TapS.xyz) - LumaS) / (ColourSigma * 0.75f * DevS + 2.0e-2f));
                    W            = Kernel * SpecGate * WDepth * WNorm * WLuma;
                }
                SumSpec  += TapS.xyz * W;
                VarSpec  += W * W * TapS.w;
                WSpecSum += W;
            }
        }
        SurfaceAndWorkAtlas[uint3(Px, SpecDstLayer)] =
            (WSpecSum > 1.0e-6f) ? float4(SumSpec / WSpecSum, VarSpec / (WSpecSum * WSpecSum)) : CentreS;
        return;
    }

    // 2. Final Joint-Bilateral Half-Res GI Upsample (VarianceUpsample) + Rough Specular Fallback + Display Output (Counts.w == 2u)
    float4 G0 = SurfaceAndWorkAtlas[uint3(Px, 0u)];
    uint   Mode = uint(FrameHeader[13].x + 0.5f);

    if (G0.w <= 0.0f)
    {
        float3 SkyOrGlass = SurfaceAndWorkAtlas[uint3(Px, SpecSrcLayer)].xyz;
        float4 G1         = SurfaceAndWorkAtlas[uint3(Px, 1u)];
        float4 G2         = SurfaceAndWorkAtlas[uint3(Px, 2u)];
        float4 G3         = SurfaceAndWorkAtlas[uint3(Px, 3u)];
        float4 G4         = SurfaceAndWorkAtlas[uint3(Px, 4u)];
        float4 G5         = SurfaceAndWorkAtlas[uint3(Px, 5u)];
        float3 Final      = ResolveInspectionOutput(Px, Width, Height, Mode, G0, G1, G2, G3, G4, G5, float4(0.0f, 0.0f, 0.0f, 1.0f), float3(0.0f), SkyOrGlass, 1.0f, 0.0f);
        OutputImage[uint3(Px, 0u)] = float4(Final, 1.0f);
        return;
    }

    float4 G1           = SurfaceAndWorkAtlas[uint3(Px, 1u)];
    float4 G2           = SurfaceAndWorkAtlas[uint3(Px, 2u)];
    float4 G3           = SurfaceAndWorkAtlas[uint3(Px, 3u)];
    float4 G4           = SurfaceAndWorkAtlas[uint3(Px, 4u)];
    float4 G5           = SurfaceAndWorkAtlas[uint3(Px, 5u)];
    float4 DirectShadow = SurfaceAndWorkAtlas[uint3(Px, 6u)];
    float3 FullN        = normalize(G1.xyz);

    float  SceneAo      = G5.w;
    float  NormHistoryM = 0.0f;
    float3 UpsampledGi  = UpsampleHalfResDiffuseGi(Px, FullN, G0.w, TraceScale, TraceW, TraceH, DiffSrcLayer, SceneAo, NormHistoryM);

    float3 ResolvedSpec = SurfaceAndWorkAtlas[uint3(Px, SpecSrcLayer)].xyz;
    // Rough matte surface reflection substitution (Lumen / ReflectionTraversal.wgsl rough-reflection reuse)
    if (SurfaceAndWorkAtlas[uint3(Px, 8u)].w < 0.0f && FrameHeader[7].x > 0.5f)
    {
        float3 ViewDir = normalize(FrameHeader[0].xyz - G0.xyz);
        float  NdotV   = max(0.05f, dot(FullN, ViewDir));
        float3 F0      = lerp(float3(0.04f), G5.xyz, saturate(G3.x));
        ResolvedSpec   = EvaluateEnvBrdfApprox(F0, clamp(G1.w, 0.045f, 1.0f), NdotV) * UpsampledGi;
    }

    float3 Final = ResolveInspectionOutput(
        Px, Width, Height, Mode, G0, G1, G2, G3, G4, G5, DirectShadow, UpsampledGi, ResolvedSpec, SceneAo, NormHistoryM);
    OutputImage[uint3(Px, 0u)] = float4(Final, 1.0f);
}

//------------------------------------------------------------------------------------------------------------------------
// KERNEL 8: DISPLAY TEMPORAL ANTI-ALIASING + OPTICAL LENS FLARE + DISPLAY OUTPUT
//   (DisplayAntialias.wgsl + LensFlare.wgsl parity: YCoCg 3x3 variance-clamped motion-reprojected TAA in
//    Karis range-compressed space, then depth-occluded sensor-space anamorphic streak / chromatic ghosts / halo,
//    finally ACES + sRGB into the swapchain viewport target.)
//------------------------------------------------------------------------------------------------------------------------

float3 ProjectYCoCg(float3 Colour)
{
    return float3(0.25f * Colour.r + 0.5f * Colour.g + 0.25f * Colour.b,
                  0.5f * Colour.r - 0.5f * Colour.b,
                  -0.25f * Colour.r + 0.5f * Colour.g - 0.25f * Colour.b);
}

float3 ReconstructFromYCoCg(float3 CoCg)
{
    return float3(CoCg.x + CoCg.y - CoCg.z, CoCg.x + CoCg.z, CoCg.x - CoCg.y - CoCg.z);
}

float3 CompressDisplayRange(float3 Colour)
{
    return Colour / (1.0f + Luminance709(Colour));
}

float3 ExpandDisplayRange(float3 Colour)
{
    return Colour / max(1.0f - Luminance709(Colour), 1.0e-4f);
}

// Sensor-space optical flare for the primary directional sun: cool anamorphic streak, warm counter-streak,
// wavelength-dispersed ghost discs along the sun<->centre axis, antipode halo ring. Depth-occluded via G-buffer.
float3 EvaluateOpticalLensFlare(uint2 Px, uint Width, uint Height)
{
    uint LumCount = uint(FrameHeader[3].w + 0.5f);
    uint LumBase  = ResolveLuminaireBaseSlot();
    if (LumCount == 0u || uint(FrameHeader[LumBase].w + 0.5f) != 0u)
    {
        return float3(0.0f);
    }
    float3 SunDir = normalize(FrameHeader[LumBase].xyz);
    float3 SunRad = FrameHeader[LumBase + 3u].xyz;
    float3 Right  = FrameHeader[1].xyz;
    float3 Up     = FrameHeader[2].xyz;
    float3 Fwd    = FrameHeader[3].xyz;
    float  FwdCmp = dot(SunDir, Fwd);
    if (FwdCmp < 0.12f)
    {
        return float3(0.0f);
    }
    float2 SunNdc = float2(dot(SunDir, Right), dot(SunDir, Up)) /
                    (FwdCmp * float2(max(FrameHeader[1].w, 1.0e-4f), max(FrameHeader[0].w, 1.0e-4f)));
    if (any(abs(SunNdc) > float2(1.6f)))
    {
        return float3(0.0f);
    }
    float2 SunUv = float2(SunNdc.x * 0.5f + 0.5f, 0.5f - SunNdc.y * 0.5f);

    // Sun-disc visibility: fraction of sky (G0.w == 0) in a 5-tap cross around the projected sun texel
    int2 SunPx = clamp(int2(SunUv * float2(float(Width), float(Height))), int2(0), int2(int(Width) - 1, int(Height) - 1));
    float Visible = 0.0f;
    [unroll]
    for (int Tap = 0; Tap < 5; ++Tap)
    {
        int2 Off      = (Tap == 0) ? int2(0, 0) : ((Tap == 1) ? int2(2, 0) : ((Tap == 2) ? int2(-2, 0) : ((Tap == 3) ? int2(0, 2) : int2(0, -2))));
        int2 TapPx    = clamp(SunPx + Off, int2(0), int2(int(Width) - 1, int(Height) - 1));
        Visible      += (SurfaceAndWorkAtlas[uint3(uint2(TapPx), 0u)].w == 0.0f) ? 0.2f : 0.0f;
    }
    if (Visible < 0.05f)
    {
        return float3(0.0f);
    }

    float2 Uv     = (float2(Px) + 0.5f) / float2(float(Width), float(Height));
    float2 Centre = float2(0.5f, 0.5f);
    float2 Axis   = SunUv - Centre;
    float  Aspect = float(Width) / float(Height);
    float3 Flare  = float3(0.0f);

    // Anamorphic streaks (cool through the sun, faint warm through the antipode)
    float StreakDist = abs(Uv.y - SunUv.y) * float(Height);
    float StreakSpan = exp(-abs(Uv.x - SunUv.x) * 1.35f);
    Flare += float3(0.35f, 0.55f, 1.0f) * exp(-StreakDist * 0.55f) * StreakSpan * 0.16f;
    float2 Antipode    = Centre - Axis;
    float  WarmDist    = abs(Uv.y - Antipode.y) * float(Height);
    Flare += float3(1.0f, 0.62f, 0.30f) * exp(-WarmDist * 0.40f) * exp(-abs(Uv.x - Antipode.x) * 1.2f) * 0.05f;

    // Chromatic ghost discs distributed along the sun<->antipode axis
    [unroll]
    for (int Ghost = 0; Ghost < 5; ++Ghost)
    {
        float  T       = -0.85f + 0.42f * float(Ghost); // ghost centre = Centre + Axis * T
        float2 GhostC  = Centre + Axis * T;
        float2 Delta   = (Uv - GhostC) * float2(Aspect, 1.0f);
        float  R       = length(Delta);
        float  Radius  = 0.035f + 0.028f * float((Ghost * 7) % 3);
        float3 Tint    = (Ghost % 3 == 0) ? float3(0.55f, 0.45f, 1.0f)
                         : ((Ghost % 3 == 1) ? float3(1.0f, 0.55f, 0.35f) : float3(0.40f, 0.95f, 0.75f));
        float  Ring    = exp(-pow((R - Radius) * 60.0f, 2.0f)) * 0.20f;
        float  Fill    = exp(-R * R / (Radius * Radius * 2.5f)) * 0.045f;
        Flare         += Tint * (Ring + Fill);
    }

    // Antipode halo ring
    float2 HaloDelta = (Uv - Antipode) * float2(Aspect, 1.0f);
    float  HaloR     = length(HaloDelta);
    Flare += float3(0.85f, 0.70f, 0.95f) * exp(-pow((HaloR - 0.30f) * 22.0f, 2.0f)) * 0.07f;

    return Flare * SunRad * Visible * saturate(FwdCmp * 1.4f);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void IntegrateDisplayAntialiasMain(uint3 DispatchId : SV_DispatchThreadID)
{
    uint Width  = Push.Counts.x;
    uint Height = Push.Counts.y;
    if (DispatchId.x >= Width || DispatchId.y >= Height)
    {
        return;
    }
    uint2 Px      = DispatchId.xy;
    uint  Mode    = uint(FrameHeader[13].x + 0.5f);
    float3 Current = OutputImage[uint3(Px, 0u)].xyz;

    // Lens flare is a display-space effect: final lit mode only
    if (Mode == 0u)
    {
        Current += EvaluateOpticalLensFlare(Px, Width, Height);
    }

    float3 Resolved = Current;
    if (Push.Params.w > 0.5f && Mode != 6u && Mode != 7u)
    {
        float3 Compressed = CompressDisplayRange(Current);
        float3 Minimum    = float3(1.0e9f);
        float3 Maximum    = float3(-1.0e9f);
        float3 Mean       = float3(0.0f);
        float3 Square     = float3(0.0f);
        int2   Limit      = int2(int(Width) - 1, int(Height) - 1);
        for (int Y = -1; Y <= 1; ++Y)
        {
            for (int X = -1; X <= 1; ++X)
            {
                int2   TapPx = clamp(int2(Px) + int2(X, Y), int2(0), Limit);
                float3 Tap   = ProjectYCoCg(CompressDisplayRange(OutputImage[uint3(uint2(TapPx), 0u)].xyz));
                Minimum      = min(Minimum, Tap);
                Maximum      = max(Maximum, Tap);
                Mean        += Tap;
                Square      += Tap * Tap;
            }
        }
        Mean /= 9.0f;
        float3 Deviation = sqrt(max(Square / 9.0f - Mean * Mean, float3(0.0f)));
        Minimum          = max(Minimum, Mean - 1.25f * Deviation);
        Maximum          = min(Maximum, Mean + 1.25f * Deviation);

        float4 G0            = SurfaceAndWorkAtlas[uint3(Px, 0u)];
        float2 PrevPos       = float2(Px) + 0.5f;
        float2 MotionUv      = float2(0.0f);
        bool   HaveHistory   = (G0.w > 0.0f) &&
                               ProjectToPreviousScreenPosition(G0.xyz, Width, Height, PrevPos, MotionUv, int2(Px));
        if (HaveHistory)
        {
            // Bilinear history fetch in compressed space, YCoCg-clamped to the current 3x3 neighbourhood
            float2 SamplePos = PrevPos - 0.5f;
            int2   Corner    = int2(floor(SamplePos));
            float2 Frac      = SamplePos - float2(Corner);
            float3 Prior     = float3(0.0f);
            float  PriorW    = 0.0f;
            for (uint Tap = 0u; Tap < 4u; ++Tap)
            {
                int2 Texel = Corner + int2(int(Tap & 1u), int(Tap >> 1u));
                if (Texel.x < 0 || Texel.y < 0 || Texel.x > Limit.x || Texel.y > Limit.y) continue;
                float W  = ((Texel.x == Corner.x + 1) ? Frac.x : (1.0f - Frac.x)) *
                           ((Texel.y == Corner.y + 1) ? Frac.y : (1.0f - Frac.y));
                Prior   += DisplayHistoryPrev[uint3(uint2(Texel), 0u)].xyz * W;
                PriorW  += W;
            }
            if (PriorW > 1.0e-4f)
            {
                Prior    = CompressDisplayRange(Prior / PriorW);
                Prior    = ReconstructFromYCoCg(clamp(ProjectYCoCg(Prior), Minimum, Maximum));
                Resolved = ExpandDisplayRange(lerp(Prior, Compressed, 0.10f));
            }
        }
    }

    DisplayHistoryCurr[uint3(Px, 0u)] = float4(Resolved, 1.0f);
    ViewportDisplayTarget[Px]         = float4(ApplyAcesAndSrgbOetf(Resolved, Push.Params.x), 1.0f);
}
