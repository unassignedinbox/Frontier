/*============================================================================================================================================
                                                        DETERMINISTICQUADRATURE.SLANG
============================================================================================================================================*/
// 🧩 Shared Bit-Exact Owen-Scrambled Sobol (0,2) Quadrature & Dupuy-Benyoub (2023) Spherical-Cap GGX VNDF (Zero RNG, Zero Denoiser)

module DeterministicQuadrature;

static const float PiConst  = 3.14159265359;
static const float TauConst = 6.28318530718;

public uint ReverseBits32(uint Bits)
{
    Bits = ((Bits & 0x55555555u) << 1u) | ((Bits >> 1u) & 0x55555555u);
    Bits = ((Bits & 0x33333333u) << 2u) | ((Bits >> 2u) & 0x33333333u);
    Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits >> 4u) & 0x0F0F0F0Fu);
    Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits >> 8u) & 0x00FF00FFu);
    return (Bits << 16u) | (Bits >> 16u);
}

public uint AvalancheKey32(uint Input)
{
    uint Mixed = Input;
    Mixed ^= Mixed >> 16u;
    Mixed *= 0x7FEB352Du;
    Mixed ^= Mixed >> 15u;
    Mixed *= 0x846CA68Bu;
    Mixed ^= Mixed >> 16u;
    return Mixed;
}

public uint LaineKarrasPermutation(uint Input, uint Key)
{
    uint Permuted = Input + Key;
    Permuted ^= Permuted * 0x6C50B47Cu;
    Permuted ^= Permuted * 0xB82F1E52u;
    Permuted ^= Permuted * 0xC7AFE638u;
    Permuted ^= Permuted * 0x8D22F6E6u;
    return Permuted;
}

public uint NestedDigitPermutation(uint Input, uint Key)
{
    return ReverseBits32(LaineKarrasPermutation(ReverseBits32(Input), Key));
}

public uint SobolSecondDimension(uint Index)
{
    uint Bits = Index;
    Bits ^= (Bits & 0xAAAAAAAAu) >> 1u;
    Bits ^= (Bits & 0xCCCCCCCCu) >> 2u;
    Bits ^= (Bits & 0xF0F0F0F0u) >> 4u;
    Bits ^= (Bits & 0xFF00FF00u) >> 8u;
    Bits ^= (Bits & 0xFFFF0000u) >> 16u;
    return ReverseBits32(Bits);
}

public float2 Sobol02Sample(uint SampleIndex, uint PixelKey, uint PairIndex)
{
    uint PairKey       = AvalancheKey32(PairIndex * 0x9E3779B9u + 0x85EBCA6Bu) ^ PixelKey;
    uint ShuffledIndex = NestedDigitPermutation(SampleIndex, PairKey);
    uint FirstDigits   = NestedDigitPermutation(ReverseBits32(ShuffledIndex), AvalancheKey32(PairKey ^ 0xA511E9B3u));
    uint SecondDigits  = NestedDigitPermutation(SobolSecondDimension(ShuffledIndex), AvalancheKey32(PairKey ^ 0x63D83595u));
    const float Inv24  = 1.0 / 16777216.0;
    return float2(float(FirstDigits >> 8u) * Inv24, float(SecondDigits >> 8u) * Inv24);
}

public float3 SampleSphericalCapGgxVndfLocal(float3 LocalWo, float AlphaX, float AlphaY, float U1, float U2)
{
    float  SafeZ     = max(1.0e-5, LocalWo.z);
    float3 Stretched = normalize(float3(AlphaX * LocalWo.x, AlphaY * LocalWo.y, SafeZ));
    float  Phi       = TauConst * U1;
    float  CapZ      = (1.0 - U2) * (1.0 + Stretched.z) - Stretched.z;
    float  SinTheta  = sqrt(max(0.0, 1.0 - CapZ * CapZ));
    float3 HalfStretched = float3(
        SinTheta * cos(Phi) + Stretched.x,
        SinTheta * sin(Phi) + Stretched.y,
        max(0.0, CapZ + Stretched.z));
    float3 LocalHalf = normalize(float3(AlphaX * HalfStretched.x, AlphaY * HalfStretched.y, max(1.0e-5, HalfStretched.z)));
    float  TwoWoDotH = 2.0 * dot(LocalWo, LocalHalf);
    return normalize(LocalHalf * TwoWoDotH - LocalWo);
}

public float EvaluateSphericalCapGgxVndfPdf(float3 LocalWo, float3 LocalWi, float AlphaX, float AlphaY)
{
    if (LocalWo.z <= 1.0e-5 || LocalWi.z <= 1.0e-5)
    {
        return 0.0;
    }
    float3 LocalHalf = normalize(LocalWo + LocalWi);
    if (LocalHalf.z <= 1.0e-5)
    {
        return 0.0;
    }
    float Ax    = max(0.0015, AlphaX);
    float Ay    = max(0.0015, AlphaY);
    float TermH = (LocalHalf.x * LocalHalf.x) / (Ax * Ax) +
                  (LocalHalf.y * LocalHalf.y) / (Ay * Ay) +
                  LocalHalf.z * LocalHalf.z;
    float NdfD  = 1.0 / max(1.0e-8, PiConst * Ax * Ay * TermH * TermH);
    float Tan2V = ((LocalWo.x * Ax) * (LocalWo.x * Ax) + (LocalWo.y * Ay) * (LocalWo.y * Ay)) /
                  max(1.0e-6, LocalWo.z * LocalWo.z);
    float G1V   = 2.0 / (1.0 + sqrt(1.0 + Tan2V));
    return (G1V * NdfD) / (4.0 * LocalWo.z);
}

public float PowerHeuristicMis(float PdfA, float PdfB)
{
    float A2 = PdfA * PdfA;
    return A2 / max(1.0e-10, A2 + PdfB * PdfB);
}

public void BuildDuffBasis(float3 Normal, out float3 OutTangent, out float3 OutBitangent)
{
    float SignZ  = (Normal.z >= 0.0) ? 1.0 : -1.0;
    float Factor = -1.0 / (SignZ + Normal.z);
    float CrossD = Normal.x * Normal.y * Factor;
    OutTangent   = float3(1.0 + SignZ * Normal.x * Normal.x * Factor, SignZ * CrossD, -SignZ * Normal.x);
    OutBitangent = float3(CrossD, SignZ + Normal.y * Normal.y * Factor, -Normal.y);
}
