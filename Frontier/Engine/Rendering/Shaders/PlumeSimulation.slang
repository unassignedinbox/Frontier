/*==========================================================================================================================================
                                                        PLUME SIMULATION.SLANG
                                     ------------------------------------------------------------------
                   Toroidal Eulerian smoke-and-fire simulation & raymarch:
                     - PlumeRecord, PlumeSource, FireLightRecord, toroidal addressing (PlumeRecords.wgsl)
                     - Semi-Lagrangian velocity & thermo advection + sun transmittance (PlumeAdvection.wgsl)
                     - Curl & vorticity confinement + analytical curl turbulence (PlumeVorticity.wgsl)
                     - Divergence, warm-started Jacobi pressure relaxation, & velocity projection (PlumePressure.wgsl)
                     - Dual-particulate (rubber smoke + black soot) + blackbody flame raymarch (PlumeMarch.wgsl)
                     - Fire point-light surface illumination (FireIllumination.wgsl)
==========================================================================================================================================*/

struct PlumeRecord
{
    int4   VelocityOrigin;      // [cell] xyz world coarse cell of window minimum corner, w step ordinal
    int4   PriorVelocityOrigin; // [cell] xyz prior window origin, w source count
    uint4  VelocityExtent;      // [cell] xyz coarse lattice extent, w unused
    uint4  ThermoExtent;        // [cell] xyz fine lattice extent, w inspection
    float4 Lattice;             // x fine edge h [m], y coarse edge 2h [m], z floor height [m], w step duration [s]
    float4 Forces;              // x buoyancy [m/s²/K], y smoke weight [m/s²], z vorticity confinement [-], w curl turbulence [m/s]
    float4 Retention;           // [-] x smoke, y heat, z flame, w velocity multipliers
    float4 Wind;                // xyz ambient flow [m/s], w elapsed seconds [s]
    float4 SourceMinimum;       // xyz bounds min [m], w frame duration [s]
    float4 SourceMaximum;       // xyz bounds max [m], w flame emission scale [-]
    float4 Optical;             // x smoke extinction [1/m], y smoke albedo [-], z soot extinction [1/m], w march samples
    float4 Sun;                 // xyz unit sun direction, w soot albedo [-]
    float4 Combustion;          // x soot shed [1/s], y heat released [K/s], z soot retention [-], w unused
    float4 Scattering;          // x multiple-scattering [-], y phase anisotropy g [-], z ambient scale [-], w fire-light scale [-]
    float4 Detail;              // x detail strength [-], y detail frequency [1/m], z turbulence frequency [1/m], w unused
    float4 Flame;               // x core temperature [K], y tip temperature [K], z core heat [K], w unused
};

struct PlumeSource
{
    float4 PositionRadius;      // xyz world centre [m], w Gaussian radius [m]
    float4 Velocity;            // xyz injected momentum [m/s]
    float4 Rates;               // x smoke [1/s], y heat [K/s], z flame [1/s], w soot [1/s]
};

struct FireLightRecord
{
    uint4  Count;               // x lights in use
    float4 PositionRadius[4];   // xyz [m], w source radius [m]
    float4 Radiance[4];         // rgb intensity
};

static const float PlumeSpeedCeiling = 40.0;

//------------------------------------------------------------------------------------------------------------------------
//                                                 TOROIDAL ADDRESSING
//------------------------------------------------------------------------------------------------------------------------

int3 WrapLattice(int3 Cell, uint3 Extent)
{
    int3 Edge = int3(Extent);
    return int3(((Cell.xy % Edge.xy) + Edge.xy) % Edge.xy, Cell.z);
}

int3 ResolveVelocityTexel(PlumeRecord Plume, int3 Local)
{
    return WrapLattice(Plume.VelocityOrigin.xyz + Local, Plume.VelocityExtent.xyz);
}

int3 ResolveThermoTexel(PlumeRecord Plume, int3 Local)
{
    return WrapLattice(int3(2 * Plume.VelocityOrigin.xy, 0) + Local, Plume.ThermoExtent.xyz);
}

float3 ProjectVelocityWorld(PlumeRecord Plume, uint3 Local)
{
    float3 Cell = float3(Plume.VelocityOrigin.xyz + int3(Local)) + 0.5;
    return float3(Cell.xy * Plume.Lattice.y, Plume.Lattice.z + Cell.z * Plume.Lattice.y);
}

float3 ProjectThermoWorld(PlumeRecord Plume, uint3 Local)
{
    float3 Cell = float3(int3(2 * Plume.VelocityOrigin.xy, 0) + int3(Local)) + 0.5;
    return float3(Cell.xy * Plume.Lattice.x, Plume.Lattice.z + Cell.z * Plume.Lattice.x);
}

float3 ProjectVelocityCoordinates(PlumeRecord Plume, float3 Position)
{
    float3 Span = float3(Plume.VelocityExtent.xyz) * Plume.Lattice.y;
    return float3(Position.xy / Span.xy, (Position.z - Plume.Lattice.z) / Span.z);
}

float3 ProjectThermoCoordinates(PlumeRecord Plume, float3 Position)
{
    float3 Span = float3(Plume.ThermoExtent.xyz) * Plume.Lattice.x;
    return float3(Position.xy / Span.xy, (Position.z - Plume.Lattice.z) / Span.z);
}

bool ClassifyWindowInterior(float3 Position, int3 Origin, uint3 Extent, float Edge, float Floor)
{
    float3 Local = float3(Position.xy / Edge - float2(Origin.xy), (Position.z - Floor) / Edge);
    return all(Local >= float3(0.5, 0.5, 0.5)) && all(Local <= float3(Extent) - float3(0.5, 0.5, 0.5));
}

bool ClassifyPriorVelocity(PlumeRecord Plume, float3 Position)
{
    return ClassifyWindowInterior(Position, Plume.PriorVelocityOrigin.xyz, Plume.VelocityExtent.xyz, Plume.Lattice.y, Plume.Lattice.z);
}

bool ClassifyPriorThermo(PlumeRecord Plume, float3 Position)
{
    return ClassifyWindowInterior(Position, int3(2 * Plume.PriorVelocityOrigin.xy, 0), Plume.ThermoExtent.xyz, Plume.Lattice.x, Plume.Lattice.z);
}

uint ResolveCoarseLinear(PlumeRecord Plume, int3 Local)
{
    int3 Wrapped = ResolveVelocityTexel(Plume, Local);
    uint3 Extent = Plume.VelocityExtent.xyz;
    return uint(Wrapped.x) + Extent.x * (uint(Wrapped.y) + Extent.y * uint(Wrapped.z));
}

//------------------------------------------------------------------------------------------------------------------------
//                                           BLACKBODY & HENYEY-GREENSTEIN OPTICS
//------------------------------------------------------------------------------------------------------------------------

float3 EvaluatePlumeBlackbody(float Kelvin)
{
    float T = clamp(Kelvin, 800.0, 4000.0) / 1000.0;
    float3 Chromaticity = saturate(float3(
        1.0,
        0.39 * log(max(T, 1.0)) - 0.63 + 0.45 * saturate((T - 1.0) * 0.7),
        max(0.0, 0.54 * log(max(T - 0.8, 0.01)) - 1.18)));
    floatBoltzmann = pow(max(T - 0.75, 0.0), 3.4);
    return Chromaticity * floatBoltzmann;
}

float EvaluateHenyeyGreenstein(floatCosTheta, float G)
{
    float G2 = G * G;
    float Denom = max(1.0 + G2 - 2.0 * G * floatCosTheta, 1.0e-4);
    return (1.0 - G2) / (12.566370614359172 * Denom * sqrt(Denom));
}

float3 EvaluateFireIllumination(
    FireLightRecord FireLights,
    float3          WorldPosition,
    float3          WorldNormal,
    float3          DiffuseAlbedo)
{
    uint Count = min(FireLights.Count.x, 4u);
    float3 Sum = float3(0.0, 0.0, 0.0);
    for (uint Index = 0u; Index < Count; ++Index)
    {
        float4 PosRadius = FireLights.PositionRadius[Index];
        float3 ToLight   = PosRadius.xyz - WorldPosition;
        float  DistSq    = dot(ToLight, ToLight);
        float  Dist      = sqrt(max(DistSq, 1.0e-6));
        float3 Dir       = ToLight / Dist;
        float  NoL       = saturate(dot(WorldNormal, Dir));
        float  SoftSq    = DistSq + PosRadius.w * PosRadius.w;
        Sum += FireLights.Radiance[Index].rgb * (NoL / max(SoftSq, 1.0e-4));
    }
    return DiffuseAlbedo * (Sum * 0.3183098861837907);
}
