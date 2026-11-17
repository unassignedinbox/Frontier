//============================================================================================================================================
//                                                       PISTONLABYRINTHSTRUCTURE.JS
//============================================================================================================================================
// 🧱 The Piston Labyrinth · Shifting Concrete Maze ("labyrinth") — Procedurally Generated Brutalist Monolith Maze with 7 Hidden Traps:
//    • Procedurally randomized every single entry (fresh seeded PRNG + ?seed=... reproducible override + N-key live reroll).
//    • Towering 18–22 m brutalist board-formed concrete canyon walls, cantilevered overhead catwalk bridges carving shafts of
//      sunlight, recessed machined hydraulic floor guide rails, and a 42 m pulsing Emerald Extraction Spire + Twin Blind Split
//      Portals (Acid Wasteland vs. Orbital Rocket Trench) rising above the labyrinth skyline so players navigate by sight.
//    • All 7 Hidden Brutalist Traps & Shifting Architecture wired to real physics:
//        1. 8 Giant Sliding Hydraulic Megawalls (16.5 m tall monoliths grinding along floor rails)
//        2. 6 False-Slab Pitfalls (flush concrete slabs hinging downward into real 3.6 m cut-out concrete pits w/ escape ramps!)
//        3. 6 Pneumatic Slam-Plates (camouflaged floor kickers snapping upward to 36° to launch cars skyward!)
//        4. 6 Snap-Ambush Crusher Walls (recessed wall pistons punching sideways out of blind alcoves!)
//        5. 6 Drop-Gate Guillotine Shutters (overhead slabs dropping on glowing crimson infrared trip-beams!)
//        6. 5 Concealed Reverse-Roller Drag Strips (Pacejka contact-patch reverse belts — hackable [Q] into emerald turbo slings!)
//        7. 5 Corner Ambush Acid-Mist Vents (Pacejka per-wheel tyre grip μ drop + toxic chartreuse mist — clearable w/ Horn [H]!)
//      + 6 Overhead Swinging Concrete Pendulums + 12–16 Procedural Jolt 3D Fracture Mirage / Cheat Shortcut Partitions!

import { CourseMaterial } from "./MaterialSpecification.js";
import { EmitBoxFacets, EmitQuad } from "./CourseStructure.js";
import { ConstructBox, SplitPart, MeasureMass, MeasureFaceNormal, TriangulateFace, ConstructRandom } from "./FractureStructure.js";
import { ConstructTabletopRampSurface, ConstructSurfaceSegmentDescriptors } from "./RampSurfaceProjection.js";

//------------------------------------------------------------------------------------------------------------------------
//                                             POOL COUNTS & GLOBAL CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

export const LabyrinthPoolCounts = Object.freeze({
    SlidingWalls:   8,
    SlidingBeacons: 8,
    HeadingWalls:   4,  // Longitudinal pistons surging directly along player heading (±X)!
    PerpDoors:      6,  // Perpendicular sliding blast doors opening tight alternative-route squeeze gaps!
    TrapSlabs:      6,
    SlamPlates:     6,
    AmbushWalls:    6,
    DropGates:      6,
    TripBeams:      6,
    RollerSlats:    20, // 5 strips × 4 animated roller bars
    AcidSlicks:     5,
    PendulumArms:   6,
    PendulumBlocks: 6,
    AbilityRings:   2   // [0] Acoustic Resonance Horn shock-cone, [1] Vehicle Hack cyber-pulse ring
});

export const LabyrinthBounds = Object.freeze({
    Minimum: [-162.0, -40.0],
    Maximum: [ 226.0,  40.0]
});

export const LabyrinthFinishX = 194.0;
export const LabyrinthLaneY   = Object.freeze([-21.0, 0.0, 21.0]); // South (-1), Center (0), North (+1) — 16.6 m clear canyons!
export const LabyrinthRingX0  = Object.freeze([-104.0, -48.0, 8.0, 64.0, 120.0]);
export const LabyrinthRingLen = 56.0;

//------------------------------------------------------------------------------------------------------------------------
//                                  WATERTIGHT 2-MANIFOLD BRUTALIST ARCHITECTURAL HELPERS
//------------------------------------------------------------------------------------------------------------------------

const Gap = 0.018; // 18 mm sub-solid separation guarantees strict closed 2-manifold meshes with 0 shared seam edges

function EmitBoxAt(Out, CX, CY, BottomZ, SX, SY, SZ, Material)
{
    const Raw = EmitBoxFacets(SX, SY, SZ, Material);
    for (const F of Raw)
    {
        Out.push({
            Family:  F.Family,
            Corners: F.Corners.map((P) => [P[0] + CX, P[1] + CY, P[2] + BottomZ])
        });
    }
}

function EmitRotBoxAt(Out, CX, CY, BottomZ, SX, SY, SZ, Yaw, Material)
{
    const C = Math.cos(Yaw), S = Math.sin(Yaw);
    const Raw = EmitBoxFacets(SX, SY, SZ, Material);
    for (const F of Raw)
    {
        Out.push({
            Family:  F.Family,
            Corners: F.Corners.map((P) => [
                CX + P[0] * C - P[1] * S,
                CY + P[0] * S + P[1] * C,
                BottomZ + P[2]
            ])
        });
    }
}

// 📝 Watertight 3D plane-cut fractured concrete rubble / brutalist monolith chunk via real FractureStructure.SplitPart.
function EmitFracturedConcreteAt(Out, CX, CY, CenterZ, SX, SY, SZ, Yaw, SkinMat, InteriorMat, Rng, CutCount = 3)
{
    let Part = ConstructBox(SX, SY, SZ);
    for (let Cut = 0; Cut < CutCount; ++Cut)
    {
        const Nx = (Rng() - 0.5) * 1.9;
        const Ny = (Rng() - 0.5) * 1.9;
        const Nz = 0.35 + Rng() * 0.95;
        const Len = Math.hypot(Nx, Ny, Nz) || 1.0;
        const Dir = [Nx / Len, Ny / Len, Nz / Len];
        const HalfProj = 0.5 * (Math.abs(Dir[0]) * SX + Math.abs(Dir[1]) * SY + Math.abs(Dir[2]) * SZ);
        const Dist = HalfProj * (0.28 + Rng() * 0.40);
        const Halves = SplitPart(Part, Dir, Dist);
        if (Halves && Halves[0] && Halves[0].Faces.length >= 4)
        {
            const M = MeasureMass(Halves[0]);
            if (M.Volume > 0.05 * SX * SY * SZ)
                Part = Halves[0];
        }
    }
    const C = Math.cos(Yaw), S = Math.sin(Yaw);
    const ToW = (P) => [CX + P[0] * C - P[1] * S, CY + P[0] * S + P[1] * C, CenterZ + P[2]];
    for (const Face of Part.Faces)
    {
        if (!Face.Indices || Face.Indices.length < 3)
            continue;
        const Mat = Face.Interior ? InteriorMat : SkinMat;
        const { Triangles } = TriangulateFace(Part, Face);
        for (const [I0, I1, I2] of Triangles)
        {
            Out.push({
                Family: Mat,
                Corners: [
                    ToW(Part.Vertices[Face.Indices[I0]]),
                    ToW(Part.Vertices[Face.Indices[I1]]),
                    ToW(Part.Vertices[Face.Indices[I2]])
                ]
            });
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                      STATIC BRUTALIST ARCHITECTURAL SHAPE BUILDERS
//------------------------------------------------------------------------------------------------------------------------

// ① Towering 20 m Fluted Concrete Monolith Wall (Length L along X, Thickness T along Y, Height H along Z)
function EmitMonolithWall(LengthX, ThickY, HeightZ, MainMat, TrimMat, AccentLight = true)
{
    const F = [];
    // Heavy chamfered plinth foot (Z = 0..1.8 m)
    EmitBoxAt(F, 0.0, 0.0, 0.0, LengthX, ThickY, 1.78, CourseMaterial.LabConcreteDark);
    // Main cast brutalist concrete monolith shaft (Z = 1.8..HeightZ - 1.6)
    const CoreH = HeightZ - 3.45;
    EmitBoxAt(F, 0.0, 0.0, 1.80, LengthX - 0.16, ThickY - 0.34, CoreH, MainMat);
    // Vertical architectural concrete buttress ribs along both sides
    const RibCount = Math.max(2, Math.round(LengthX / 6.0));
    const Span = LengthX - 2.4;
    for (let I = 0; I < RibCount; ++I)
    {
        const RX = RibCount === 1 ? 0.0 : (-0.5 * Span + (I / (RibCount - 1)) * Span);
        EmitBoxAt(F, RX,  0.5 * (ThickY - 0.34) + 0.18 + Gap, 1.82, 0.72, 0.32, CoreH - 0.12, TrimMat);
        EmitBoxAt(F, RX, -0.5 * (ThickY - 0.34) - 0.18 - Gap, 1.82, 0.72, 0.32, CoreH - 0.12, TrimMat);
    }
    // Cast upper cantilevered cornice cap (Z = HeightZ - 1.62 .. HeightZ)
    EmitBoxAt(F, 0.0, 0.0, HeightZ - 1.62, LengthX + 0.18, ThickY + 0.44, 1.62, CourseMaterial.LabConcreteDark);
    // Recessed industrial tie-rod horizontal belt + warm brutalist sconce light strips at Z = 4.6 m
    EmitBoxAt(F, 0.0,  0.5 * (ThickY - 0.34) + 0.10 + Gap, 7.8, LengthX - 0.8, 0.14, 0.44, CourseMaterial.LabCatwalkSteel);
    EmitBoxAt(F, 0.0, -0.5 * (ThickY - 0.34) - 0.10 - Gap, 7.8, LengthX - 0.8, 0.14, 0.44, CourseMaterial.LabCatwalkSteel);
    if (AccentLight && LengthX >= 10.0)
    {
        EmitBoxAt(F, 0.0,  0.5 * (ThickY - 0.34) + 0.09 + Gap, 4.4, 3.2, 0.12, 0.24, CourseMaterial.LabSodiumLamp);
        EmitBoxAt(F, 0.0, -0.5 * (ThickY - 0.34) - 0.09 - Gap, 4.4, 3.2, 0.12, 0.24, CourseMaterial.LabSodiumLamp);
    }
    return F;
}

// ② Colossal 21 m Corner Pylon / Monolith Junction Pier (armored steel base + amber sector marker stripe)
function EmitMonolithPillar()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0,  5.2, 5.2, 1.6, CourseMaterial.LabCatwalkSteel);
    EmitBoxAt(F, 0.0, 0.0, 1.62, 4.7, 4.7, 17.6, CourseMaterial.LabConcreteMid);
    // Amber brutalist corner stripe bands at Z = 2.6 m (height of headlights!)
    EmitBoxAt(F,  2.42, 0.0, 2.2, 0.12, 3.6, 0.55, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, -2.42, 0.0, 2.2, 0.12, 3.6, 0.55, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, 0.0,  2.42, 2.2, 3.6, 0.12, 0.55, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, 0.0, -2.42, 2.2, 3.6, 0.12, 0.55, CourseMaterial.LabHazardAmber);
    // Heavy overhang crown (Z = 19.24..21.2)
    EmitBoxAt(F, 0.0, 0.0, 19.24, 5.5, 5.5, 1.96, CourseMaterial.LabConcreteDark);
    return F;
}

// ③ Overhead Stalker Catwalk Bridge & Skylight Shadow Truss (spans 18.2 m across Y at Z = 12.8 m)
function EmitCatwalkBridge()
{
    const F = [];
    // Concrete deck slab spanning Y = -9.1..+9.1
    EmitBoxAt(F, 0.0, 0.0, 0.0, 3.0, 18.2, 0.52, CourseMaterial.LabConcreteDark);
    // Heavy steel edge I-beams & side guardrails
    EmitBoxAt(F,  1.42, 0.0, 0.54, 0.18, 18.0, 1.15, CourseMaterial.LabCatwalkSteel);
    EmitBoxAt(F, -1.42, 0.0, 0.54, 0.18, 18.0, 1.15, CourseMaterial.LabCatwalkSteel);
    // Underside twin downward sodium floodlight canisters illuminating the canyon floor
    EmitBoxAt(F, 0.0, -4.0, -0.34, 1.1, 1.6, 0.28, CourseMaterial.LabCatwalkSteel);
    EmitBoxAt(F, 0.0, -4.0, -0.46, 0.8, 1.2, 0.10, CourseMaterial.LabSodiumLamp);
    EmitBoxAt(F, 0.0,  4.0, -0.34, 1.1, 1.6, 0.28, CourseMaterial.LabCatwalkSteel);
    EmitBoxAt(F, 0.0,  4.0, -0.46, 0.8, 1.2, 0.10, CourseMaterial.LabSodiumLamp);
    return F;
}

// ④ Overhead Guillotine Drop-Gate Soffit Header (conceals heavy concrete guillotine slab at Z = 6.2..9.6 m)
function EmitGuillotineSoffit()
{
    const F = [];
    // Massive concrete overhead lintel beam spanning full 17.6 m corridor width
    EmitBoxAt(F, 0.0, 0.0, 0.0, 3.6, 17.6, 3.4, CourseMaterial.LabConcreteDark);
    // Subtle dark steel trip-beam sensor pods mounted low on both side walls (tell for sharp drivers!)
    EmitBoxAt(F, 0.0, -8.1, -6.15, 0.65, 0.42, 0.85, CourseMaterial.LabCatwalkSteel);
    EmitBoxAt(F, 0.0,  8.1, -6.15, 0.65, 0.42, 0.85, CourseMaterial.LabCatwalkSteel);
    // Subtle amber ceiling warning trim on the lintel faces
    EmitBoxAt(F, -1.86, 0.0, 0.28, 0.10, 14.8, 0.34, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F,  1.86, 0.0, 0.28, 0.10, 14.8, 0.34, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑤ Recessed Hydraulic Floor Rail Track Pair (visual tell for Giant Sliding Megawalls!)
function EmitFloorRailTrack()
{
    const F = [];
    EmitBoxAt(F, -0.82, 0.0, 0.0, 0.28, 16.4, 0.045, CourseMaterial.LabRailSteel);
    EmitBoxAt(F,  0.82, 0.0, 0.0, 0.28, 16.4, 0.045, CourseMaterial.LabRailSteel);
    // Cross tie bolts along rail
    for (let Y = -7.0; Y <= 7.0; Y += 3.5)
        EmitBoxAt(F, 0.0, Y, 0.0, 1.32, 0.22, 0.03, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑥ Recessed Reverse-Roller Bay Frame (flush perimeter tracks framing each 17 m × 10 m hidden drag strip)
function EmitRollerBayFrame()
{
    const F = [];
    // Flush steel side curbs
    EmitBoxAt(F, 0.0,  5.18, 0.0, 17.6, 0.38, 0.09, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 0.0, -5.18, 0.0, 17.6, 0.38, 0.09, CourseMaterial.LabRailSteel);
    // Subtle corner hazard dashes
    EmitBoxAt(F, -8.55,  5.18, 0.095, 0.6, 0.34, 0.04, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F,  8.55,  5.18, 0.095, 0.6, 0.34, 0.04, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, -8.55, -5.18, 0.095, 0.6, 0.34, 0.04, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F,  8.55, -5.18, 0.095, 0.6, 0.34, 0.04, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑦ Corner Acid-Mist Ambush Vent Housing (vertical brutalist nozzle slit tucked into wall corners)
function EmitAcidVentHousing()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0, 1.35, 1.15, 4.4, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F, 0.0, 0.62, 0.35, 0.95, 0.14, 3.6, CourseMaterial.LabCatwalkSteel);
    // Glowing chartreuse acid nozzle core slit
    EmitBoxAt(F, 0.0, 0.72, 0.55, 0.26, 0.08, 3.1, CourseMaterial.LabAcidGlow);
    return F;
}

// ⑧ 42-Meter Towering Emerald Extraction Beacon Spire (visible OVER the 20 m concrete skyline from spawn!)
function EmitExtractionSpire()
{
    const F = [];
    // Monumental stepped brutalist pedestal
    EmitBoxAt(F, 0.0, 0.0,  0.0,  9.2, 9.2,  4.0, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F, 0.0, 0.0,  4.02, 7.2, 7.2, 18.0, CourseMaterial.LabConcreteMid);
    EmitBoxAt(F, 0.0, 0.0, 22.04, 5.2, 5.2, 12.0, CourseMaterial.LabConcreteLight);
    // Blazing 8 m emerald sky-beacon lantern cage at Z = 34..42 m
    EmitBoxAt(F, 0.0, 0.0, 34.06, 3.6, 3.6, 7.6, CourseMaterial.LabBeaconEmerald);
    // Vertical steel beacon corner fins
    for (const [SX, SY] of [[-2.0, -2.0], [2.0, -2.0], [2.0, 2.0], [-2.0, 2.0]])
        EmitBoxAt(F, SX, SY, 33.8, 0.48, 0.48, 8.6, CourseMaterial.LabCatwalkSteel);
    EmitBoxAt(F, 0.0, 0.0, 42.42, 4.8, 4.8, 0.85, CourseMaterial.LabConcreteDark);
    return F;
}

// ⑨ Shimmering Blind Split Portal Arch (Left: Acid Wasteland Emerald-Acid, Right: Orbital Rocket Trench Violet-Amber)
function EmitSplitPortalArch(EnergyMat)
{
    const F = [];
    // Left & right monolithic portal piers (Z = 0..10.5 m)
    EmitBoxAt(F, 0.0, -5.8, 0.0, 3.4, 2.0, 10.6, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F, 0.0,  5.8, 0.0, 3.4, 2.0, 10.6, CourseMaterial.LabConcreteDark);
    // Overhead heavy concrete lintel
    EmitBoxAt(F, 0.0,  0.0, 10.62, 3.8, 13.8, 2.4, CourseMaterial.LabConcreteLight);
    // Blazing portal energy border ring strips (interior face)
    EmitBoxAt(F, 0.0, -4.68, 0.2, 0.72, 0.22, 10.2, EnergyMat);
    EmitBoxAt(F, 0.0,  4.68, 0.2, 0.72, 0.22, 10.2, EnergyMat);
    EmitBoxAt(F, 0.0,  0.0, 10.30, 0.72, 9.2, 0.28, EnergyMat);
    return F;
}

//------------------------------------------------------------------------------------------------------------------------
//                                      DYNAMIC TRAP & ABILITY SHAPE BUILDERS
//------------------------------------------------------------------------------------------------------------------------

// ① Giant Sliding Hydraulic Megawall (16.5 m tall × 11.4 m wide × 2.4 m thick sliding concrete titan!)
function EmitSlidingMegawall()
{
    const F = [];
    // Heavy machined steel bottom slide shoe riding on floor rails
    EmitBoxAt(F, 0.0, 0.0, 0.0, 2.55, 11.6, 0.75, CourseMaterial.LabRailSteel);
    // Yellow-black hazard bumper skirt (Z = 0.77..1.65 m)
    EmitBoxAt(F, 0.0, 0.0, 0.77, 2.42, 11.4, 0.88, CourseMaterial.LabHazardAmber);
    // Towering brutalist concrete hydraulic monolith body (Z = 1.67..16.2 m)
    EmitBoxAt(F, 0.0, 0.0, 1.67, 2.34, 11.3, 14.5, CourseMaterial.LabConcreteLight);
    // Recessed dark structural cross-ribs on ±X faces
    for (const Y of [-3.6, 0.0, 3.6])
    {
        EmitBoxAt(F,  1.24, Y, 2.2, 0.14, 0.55, 13.2, CourseMaterial.LabConcreteDark);
        EmitBoxAt(F, -1.24, Y, 2.2, 0.14, 0.55, 13.2, CourseMaterial.LabConcreteDark);
    }
    // Concrete crown cap
    EmitBoxAt(F, 0.0, 0.0, 16.20, 2.65, 11.7, 0.75, CourseMaterial.LabConcreteDark);
    return F;
}

// ② Megawall Status Beacon Strip (glows red/amber when crushing, flips green when hacked!)
function EmitStatusBeacon(Material)
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0, 2.75, 4.4, 0.42, Material);
    return F;
}

// ③ False-Slab Pitfall Deck (10.0 m along X × 9.6 m along Y × 0.44 m thick; pivot at West edge X = 0, top flush Z = 0!)
function EmitTrapSlab()
{
    const F = [];
    // Flush concrete top slab matching LabFloorSlate (X = 0..10.0, Z = -0.44..0.0)
    EmitBoxAt(F, 5.0, 0.0, -0.44, 9.94, 9.54, 0.44, CourseMaterial.LabFloorSlate);
    // Subtle dark steel corner hinge rivets (the sharp-eyed visual tell!)
    for (const [RX, RY] of [[0.45, -4.3], [0.45, 4.3], [9.55, -4.3], [9.55, 4.3]])
        EmitBoxAt(F, RX, RY, 0.005, 0.48, 0.48, 0.035, CourseMaterial.LabRailSteel);
    // Underside dark steel counter-girder ribs (exposed dramatically as the slab swings into the pit!)
    EmitBoxAt(F, 5.0, -3.2, -0.78, 9.4, 0.42, 0.32, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, 5.0,  3.2, -0.78, 9.4, 0.42, 0.32, CourseMaterial.LabHazardAmber);
    return F;
}

// ④ Pneumatic Slam-Plate (8.2 m along X × 8.8 m along Y × 0.32 m thick; hinged on West edge X = 0, snaps upward!)
function EmitSlamPlate()
{
    const F = [];
    EmitBoxAt(F, 4.1, 0.0, -0.32, 8.16, 8.76, 0.32, CourseMaterial.LabConcreteMid);
    // Subtle twin hydraulic seam plates on top
    EmitBoxAt(F, 0.38, 0.0, 0.005, 0.32, 8.2, 0.03, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 7.82, 0.0, 0.005, 0.36, 6.4, 0.035, CourseMaterial.LabHazardAmber);
    // Underside glowing pneumatic cylinder bar (revealed as it launches upward!)
    EmitBoxAt(F, 4.1, 0.0, -0.68, 7.2, 1.2, 0.34, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑤ Snap-Ambush Crusher Wall (7.2 m long × 2.0 m thick × 5.4 m tall concealed alcove piston!)
function EmitAmbushWall()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0, 7.2, 1.92, 5.4, CourseMaterial.LabConcreteMid);
    // Armored steel impact face + subtle warning corner ticks
    EmitBoxAt(F, 0.0,  1.02, 0.12, 6.8, 0.14, 5.1, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 0.0, -1.02, 0.12, 6.8, 0.14, 5.1, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 0.0,  0.0,  5.42, 7.1, 1.85, 0.24, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑥ Drop-Gate Guillotine Shutter (1.55 m thick along X × 16.0 m wide along Y × 5.8 m tall heavy brutalist drop slab)
function EmitDropGate()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0,  1.52, 16.0, 0.56, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, 0.0, 0.0, 0.58, 1.48, 15.9, 5.22, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F,  0.80, 0.0, 1.2, 0.10, 13.8, 0.38, CourseMaterial.LabWarningRed);
    EmitBoxAt(F, -0.80, 0.0, 1.2, 0.10, 13.8, 0.38, CourseMaterial.LabWarningRed);
    return F;
}

// ⑦ Crimson Infrared Trip-Beam (spans 16.2 m across Y at Z = 0.85 m)
function EmitTripBeam()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, -0.05, 0.10, 16.2, 0.10, CourseMaterial.LabWarningRed);
    return F;
}

// ⑧ Animated Reverse-Roller Slat Bar (rolls along X inside each 17 m × 9.6 m concealed drag strip!)
function EmitRollerSlat()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0, 0.82, 9.6, 0.065, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 0.0, 0.0, 0.070, 0.28, 9.0, 0.025, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑨ Corrosive Acid-Mist Floor Slick Patch (wet shimmering green puddle coating corner asphalt!)
function EmitAcidSlick()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0, 14.2, 10.6, 0.045, CourseMaterial.LabAcidGlow);
    return F;
}

// ⑩ Overhead Swinging Concrete Pendulum Arm + Counterweight Block (pivot at local Z = 0, hangs along -Z)
function EmitPendulumArm()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, -10.8, 0.34, 0.34, 10.8, CourseMaterial.LabCatwalkSteel);
    return F;
}

function EmitPendulumBlock()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, -1.35, 2.35, 3.2, 2.7, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F, 0.0, 0.0, -1.48, 2.48, 3.32, 0.32, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑪ Expanding Ability VFX Ring (Acoustic Resonance Horn Shock-Cone / Vehicle Hack Pulse Ring)
function EmitAbilityRing(Mat)
{
    const F = [];
    // 8-segment watertight octagonal glowing ring in XY plane (radius ~1.0 m; scaled via animation/multi-ring feel)
    const R = 3.2;
    const Segs = 8;
    for (let I = 0; I < Segs; ++I)
    {
        const A = (I / Segs) * Math.PI * 2.0;
        const CX = Math.cos(A) * R;
        const CY = Math.sin(A) * R;
        EmitRotBoxAt(F, CX, CY, 0.0, 0.32, 2.35, 0.34, A, Mat);
    }
    return F;
}

// ⑫ Heavy Fluted Concrete Bridge Pier Supporting Top/Bottom Overpass Spans (4.38 m tall under the 5.0 m deck!)
function EmitBridgePier()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0,  1.65, 1.85, 0.42, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F, 0.0, 0.0, 0.44, 1.32, 1.48, 3.60, CourseMaterial.LabConcreteLight);
    EmitBoxAt(F, 0.0, 0.0, 4.06, 1.56, 1.72, 0.32, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 0.0, 0.0, 1.85, 1.38, 0.24, 1.60, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑬ Recessed Longitudinal Steel Floor Rail Track for Heading-Direction Moving Piston Walls (32 m along X!)
function EmitLongRailTrack()
{
    const F = [];
    EmitBoxAt(F, 0.0, -0.58, 0.0, 32.0, 0.24, 0.055, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 0.0,  0.58, 0.0, 32.0, 0.24, 0.055, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, -15.6, 0.0, 0.06, 0.65, 1.65, 0.16, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F,  15.6, 0.0, 0.06, 0.65, 1.65, 0.16, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑭ Heading-Direction Moving Concrete Piston Wall (9.0 m long along X × 3.0 m wide × 15.6 m tall — surges along ±X!)
function EmitHeadingWall()
{
    const F = [];
    // Main monumental brutalist concrete core
    EmitBoxAt(F, 0.0, 0.0, 0.06, 8.2, 2.86, 15.2, CourseMaterial.LabConcreteDark);
    // Aggressive steel hydraulic ramming bumpers at BOTH front (+X) & rear (-X) ends!
    EmitBoxAt(F,  4.28, 0.0, 0.16, 0.32, 2.76, 3.6, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, -4.28, 0.0, 0.16, 0.32, 2.76, 3.6, CourseMaterial.LabRailSteel);
    // High-contrast amber warning chevron plates + crimson beacon strips on nose & tail
    EmitBoxAt(F,  4.48, 0.0, 0.72, 0.10, 2.40, 0.56, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F,  4.48, 0.0, 2.10, 0.10, 2.40, 0.56, CourseMaterial.LabWarningRed);
    EmitBoxAt(F, -4.48, 0.0, 0.72, 0.10, 2.40, 0.56, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, -4.48, 0.0, 2.10, 0.10, 2.40, 0.56, CourseMaterial.LabWarningRed);
    // Upper concrete crown rib + side structural pilasters
    EmitBoxAt(F, 0.0, 0.0, 15.28, 8.4, 3.04, 0.42, CourseMaterial.LabConcreteLight);
    EmitBoxAt(F, 0.0,  1.49, 1.2, 7.2, 0.12, 0.46, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, 0.0, -1.49, 1.2, 7.2, 0.12, 0.46, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑮ Perpendicular Blast Doorway Static Surround: Solid Concrete Jamb (Collider: true) & Overhead Lintel (Collider: false)
function EmitDoorJamb()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.0, 1.85, 4.42, 6.82, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F, 0.0,  2.26, 0.4, 1.72, 0.10, 5.8, CourseMaterial.LabRailSteel);
    EmitBoxAt(F, 0.0, -2.26, 0.4, 1.72, 0.10, 5.8, CourseMaterial.LabRailSteel);
    return F;
}

function EmitDoorLintel()
{
    const F = [];
    // Heavy brutalist concrete mass spanning Z = 6.84..19.20 m right over the 7.4 m blast doorway opening!
    EmitBoxAt(F, 0.0, 0.0, 6.84, 11.4, 4.40, 12.36, CourseMaterial.LabConcreteMid);
    // Glowing cyan/amber doorway portal header bars on both canyon sides so drivers spot the alternative opening from afar!
    EmitBoxAt(F, 0.0,  2.26, 6.92, 8.2, 0.14, 0.42, CourseMaterial.LabSonicCyan);
    EmitBoxAt(F, 0.0, -2.26, 6.92, 8.2, 0.14, 0.42, CourseMaterial.LabSonicCyan);
    EmitBoxAt(F, 0.0,  2.26, 7.52, 9.6, 0.12, 0.34, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, 0.0, -2.26, 7.52, 9.6, 0.12, 0.34, CourseMaterial.LabHazardAmber);
    return F;
}

// ⑯ Dynamic Perpendicular Sliding Blast Door Blade (7.2 m × 1.1 m × 6.65 m — slides perpendicularly across aperture!)
function EmitPerpDoor()
{
    const F = [];
    EmitBoxAt(F, 0.0, 0.0, 0.04, 7.2, 1.06, 6.62, CourseMaterial.LabRailSteel);
    // Recessed concrete blast inset panels on both faces
    EmitBoxAt(F, 0.0,  0.58, 0.45, 6.3, 0.12, 5.65, CourseMaterial.LabConcreteDark);
    EmitBoxAt(F, 0.0, -0.58, 0.45, 6.3, 0.12, 5.65, CourseMaterial.LabConcreteDark);
    // Blazing amber leading-edge & trailing-edge vertical pinch warning strips!
    EmitBoxAt(F,  3.64, 0.0, 0.12, 0.14, 1.18, 6.42, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F, -3.64, 0.0, 0.12, 0.14, 1.18, 6.42, CourseMaterial.LabHazardAmber);
    EmitBoxAt(F,  0.0,  0.66, 3.10, 5.6, 0.08, 0.36, CourseMaterial.LabWarningRed);
    EmitBoxAt(F,  0.0, -0.66, 3.10, 5.6, 0.08, 0.36, CourseMaterial.LabWarningRed);
    return F;
}

//------------------------------------------------------------------------------------------------------------------------
//                                   DYNAMIC FLOOR HEIGHT ORACLE (FALSE-SLAB PITFALLS & SLAM-PLATES)
//------------------------------------------------------------------------------------------------------------------------

let ActiveMaze = null;
let LiveHeightOracle = null;

export function AssignLabyrinthHeightOracle(Fn = null)
{
    LiveHeightOracle = typeof Fn === "function" ? Fn : null;
}

// 📝 Default floor height when traps are closed is flat Z = 0.0; when PistonLabyrinthSequence attaches LiveHeightOracle,
//    open False-Slab Pitfalls dynamically drop the floor to -3.6 m (with a smooth 23° East rescue ramp back to Z = 0.0!),
//    and triggered Pneumatic Slam-Plates dynamically tilt into a real angled launch ramp!
export function QueryLabyrinthFloorHeight(X, Y)
{
    if (LiveHeightOracle)
        return LiveHeightOracle(X, Y);
    return 0.0;
}

//------------------------------------------------------------------------------------------------------------------------
//                                  SEEDED PROCEDURAL SHIFTING CONCRETE MAZE GENERATOR
//------------------------------------------------------------------------------------------------------------------------

function ResolveRandomSeed(RequestedSeed = null)
{
    if (RequestedSeed !== null && RequestedSeed !== undefined && Number.isFinite(Number(RequestedSeed)))
        return ((Number(RequestedSeed) >>> 0) || 0x20261AB1);
    const CryptoVal = (typeof crypto !== "undefined" && crypto.getRandomValues)
        ? crypto.getRandomValues(new Uint32Array(1))[0]
        : ((Date.now() ^ Math.floor(Math.random() * 0xFFFFFFFF)) >>> 0);
    return (CryptoVal >>> 0) || 0x20261AB1;
}

export function GenerateLabyrinthMaze(RequestedSeed = null)
{
    const Seed    = ResolveRandomSeed(RequestedSeed);
    const SeedTag = (Seed & 0xFFFFFF).toString(16).toUpperCase().padStart(6, "0");
    const Rng     = ConstructRandom(Seed ^ 0x51A7B9E3);

    // ① Procedural Ring-to-Ring Route Graph (5 Rings × 3 Lanes = 15 concrete sectors)
    //    Ring 0 ALWAYS opens on Center Lane (Index 1, Y=0) straight ahead of spawn so players never hit a wall at start!
    //    Subsequent rings shift randomly across South(0), Center(1), North(2) while keeping guaranteed cross-hallways AND
    //    placing Breakable Mirage Shortcut Walls across alternate routes!
    const Rings = [];
    let PrevLane = 1; // Center (Y = 0)
    for (let R = 0; R < 5; ++R)
    {
        const X0 = LabyrinthRingX0[R];
        const X1 = X0 + LabyrinthRingLen;
        let MainLane = 1;
        if (R === 0)
            MainLane = 1;
        else if (R === 4)
            MainLane = 1; // Converges cleanly into the Center Extraction Portal Hall!
        else
        {
            // Pick a lane that shifts from PrevLane ~70% of the time so every seed has genuine maze turns!
            const Choices = [0, 1, 2].filter((L) => L !== PrevLane);
            MainLane = Rng() < 0.76 ? Choices[Math.floor(Rng() * Choices.length)] : PrevLane;
        }

        // For each of the 3 lanes in this Ring, decide entry gate state: "open" | "mirage" | "solid"
        // MainLane is ALWAYS "open"; one off-lane becomes a "mirage" breakable cheat shortcut, the other "mirage" or "solid".
        const LaneEntry = ["solid", "solid", "solid"];
        LaneEntry[MainLane] = "open";
        const OtherLanes = [0, 1, 2].filter((L) => L !== MainLane);
        const CheatIdx   = OtherLanes[Math.floor(Rng() * OtherLanes.length)];
        const ThirdIdx   = OtherLanes.find((L) => L !== CheatIdx);
        LaneEntry[CheatIdx] = "mirage";
        LaneEntry[ThirdIdx] = Rng() < 0.45 ? "mirage" : "solid";

        // Cross-divider portals between Center(1)↔North(2) and Center(1)↔South(0) inside this ring:
        // At Ring's East cross-hall [X1 - 15..X1], dividers ALWAYS open for clean turning, PLUS a mid-ring mirage shortcut!
        const MidShortcutSide = Rng() < 0.5 ? 1 : -1; // +1 north divider, -1 south divider

        Rings.push({
            Index: R,
            X0,
            X1,
            MidX: 0.5 * (X0 + X1),
            MainLane,
            MainY: LabyrinthLaneY[MainLane],
            PrevLane,
            LaneEntry,
            MidShortcutSide
        });
        PrevLane = MainLane;
    }

    // ② Allocate ALL 7 Hidden Trap Families + Lateral Sliding Megawalls + Heading-Direction Pistons + Perpendicular Doors + Bridges!
    const SlidingWalls = [];
    const HeadingWalls = [];
    const PerpDoors    = [];
    const Pitfalls     = [];
    const SlamPlates   = [];
    const AmbushWalls  = [];
    const DropGates    = [];
    const RollerStrips = [];
    const AcidVents    = [];
    const Pendulums    = [];
    const MirageWalls  = [];

    // Helper: pick an alternate non-overlapping lane Y
    const AltLaneY = (MainIdx, Offset = 1) => LabyrinthLaneY[(MainIdx + Offset) % 3];

    // -- 8 Giant Sliding Hydraulic Megawalls (grinding laterally ±Y across active corridors on steel floor rails!)
    const SlideSpecs = [
        { R: 0, XOff: 38.0, Lane: Rings[0].MainLane, Freq: 1.05 }, // Frames Cross-Bridge A cleanly past HeadingWall[0]!
        { R: 1, XOff:  8.0, Lane: Rings[1].MainLane, Freq: 1.25 },
        { R: 1, XOff: 36.0, Lane: (Rings[1].MainLane + 2) % 3, Freq: 1.15 },
        { R: 2, XOff:  8.0, Lane: Rings[2].MainLane, Freq: 1.38 },
        { R: 2, XOff: 38.0, Lane: (Rings[2].MainLane + 2) % 3, Freq: 1.38 },
        { R: 3, XOff:  8.0, Lane: Rings[3].MainLane, Freq: 1.45 },
        { R: 4, XOff: 14.0, Lane: Rings[4].MainLane, Freq: 1.55 },
        { R: 4, XOff: 36.0, Lane: Rings[4].MainLane, Freq: 1.65 }
    ];
    SlideSpecs.forEach((S, Idx) =>
    {
        const Ring = Rings[S.R];
        const X    = Ring.X0 + S.XOff;
        const CY   = LabyrinthLaneY[S.Lane];
        const Dir  = (Idx % 2 === 0 ? 1.0 : -1.0) * (Rng() < 0.5 ? 1.0 : -1.0);
        SlidingWalls.push({
            Id:         Idx,
            Ring:       S.R,
            X,
            CenterY:    CY,
            TravelHalf: 7.4, // Slides fully clear of lane center at peak, blocks lane completely at center!
            Phase:      (Idx * 1.15 + Rng() * 2.2) % (Math.PI * 2.0),
            Rate:       S.Freq,
            Dir
        });
    });

    // -- 4 Heading-Direction Moving Concrete Piston Walls (grinding LONGITUDINALLY ±X along player heading!)
    //    Offset at Y = LaneY ± 6.5 (HalfY = 1.52 -> stays strictly outside Pitfall Y ∈ [-4.8,+4.8] & inside ±8.3 walls!)
    const HeadingSpecs = [
        { R: 0, XMinOff:  6.0, XMaxOff: 30.0, Lane: Rings[0].MainLane, Rate: 0.92 }, // Immediately visible surging down Ring 0!
        { R: 1, XMinOff: 18.0, XMaxOff: 38.0, Lane: Rings[1].MainLane, Rate: 1.04 },
        { R: 2, XMinOff: 18.0, XMaxOff: 38.0, Lane: Rings[2].MainLane, Rate: 1.14 },
        { R: 3, XMinOff: 18.0, XMaxOff: 38.0, Lane: Rings[3].MainLane, Rate: 1.22 }
    ];
    const HeadingSideByRing = [];
    HeadingSpecs.forEach((HS, Idx) =>
    {
        const Ring = Rings[HS.R];
        const Side = (Idx % 2 === 0 ? 1 : -1) * (Rng() < 0.5 ? 1 : -1);
        HeadingSideByRing[HS.R] = Side;
        const XMin = Ring.X0 + HS.XMinOff;
        const XMax = Ring.X0 + HS.XMaxOff;
        HeadingWalls.push({
            Id:         Idx,
            Ring:       HS.R,
            LaneY:      LabyrinthLaneY[HS.Lane],
            Side,
            Y:          LabyrinthLaneY[HS.Lane] + Side * 6.5,
            CenterX:    0.5 * (XMin + XMax),
            TravelHalf: 0.5 * (XMax - XMin),
            HalfX:      4.45,
            HalfY:      1.52,
            Rate:       HS.Rate,
            Phase:      (Idx * 1.45 + Rng() * 1.8) % (Math.PI * 2.0)
        });
    });

    // -- 6 False-Slab Pitfalls (real 2.8 m cut-out subterranean shafts with 23.3° East rescue ramps!)
    //    Strictly spaced away from sliding wall rails & roller bays so every trap bay is crisp and unambiguous.
    const PitSpecs = [
        { R: 0, XOff: 22.0, Lane: Rings[0].MainLane }, // First hidden floor trap right in Ring 0!
        { R: 1, XOff: 22.0, Lane: (Rings[1].MainLane + 2) % 3 },
        { R: 2, XOff: 18.0, Lane: (Rings[2].MainLane + 1) % 3 },
        { R: 3, XOff: 26.0, Lane: Rings[3].MainLane },
        { R: 3, XOff: 10.0, Lane: (Rings[3].MainLane + 1) % 3 },
        { R: 4, XOff: 22.0, Lane: Rings[4].MainLane }
    ];
    PitSpecs.forEach((P, Idx) =>
    {
        const Ring = Rings[P.R];
        const X0   = Ring.X0 + P.XOff; // West hinge edge of the 10.0 m tilting slab
        const CY   = LabyrinthLaneY[P.Lane];
        Pitfalls.push({
            Id:       Idx,
            Ring:     P.R,
            X0,                // West edge of 10.0 m camouflaged trap bay
            SlabLen:  10.0,    // Flush tilting slab covers 100% of X0..X0+10.0 when closed!
            DropLen:  3.5,     // Deep drop well X0..X0+3.5 at Z = -2.8 m
            RampLen:  6.5,     // Smooth 23.3° rescue ramp rising from -2.8 m back to 0.0 m across X0+3.5..X0+10.0
            Depth:    2.8,
            CenterY:  CY,
            HalfW:    4.8      // Pit width 9.6 m inside 15 m corridor (side concrete ledges allow skilled edge-hugging!)
        });
    });

    // -- 6 Pneumatic Slam-Plates (camouflaged flush kickers snapping upward to 36°!)
    const SlamSpecs = [
        { R: 0, XOff: 6.0,  Lane: (Rings[0].MainLane + 1) % 3 },
        { R: 1, XOff: 24.0, Lane: Rings[1].MainLane },
        { R: 2, XOff: 21.0, Lane: Rings[2].MainLane },
        { R: 2, XOff: 22.0, Lane: (Rings[2].MainLane + 2) % 3 },
        { R: 3, XOff: 24.0, Lane: (Rings[3].MainLane + 1) % 3 },
        { R: 4, XOff: 4.0,  Lane: Rings[4].MainLane }
    ];
    SlamSpecs.forEach((S, Idx) =>
    {
        const Ring = Rings[S.R];
        SlamPlates.push({
            Id:      Idx,
            Ring:    S.R,
            X0:      Ring.X0 + S.XOff,
            Len:     8.2,
            CenterY: LabyrinthLaneY[S.Lane],
            HalfW:   4.4
        });
    });

    // -- 6 Snap-Ambush Crusher Walls (recessed concrete pistons exploding sideways out of blind walls!)
    //    Mounted on the opposite wall from any HeadingWall in the same lane so zero mechanical clipping!
    const AmbushSpecs = [
        { R: 0, XOff: 16.0, Lane: Rings[0].MainLane, Side: -(HeadingSideByRing[0] || 1) },
        { R: 1, XOff: 33.0, Lane: Rings[1].MainLane, Side: -(HeadingSideByRing[1] || 1) },
        { R: 2, XOff: 27.0, Lane: (Rings[2].MainLane + 1) % 3, Side: 1 },
        { R: 3, XOff: 12.0, Lane: Rings[3].MainLane, Side: -(HeadingSideByRing[3] || 1) },
        { R: 3, XOff: 38.0, Lane: Rings[3].MainLane, Side: -(HeadingSideByRing[3] || 1) },
        { R: 4, XOff: 30.0, Lane: Rings[4].MainLane, Side: -1 }
    ];
    AmbushSpecs.forEach((A, Idx) =>
    {
        const Ring = Rings[A.R];
        const CY   = LabyrinthLaneY[A.Lane];
        const WallSide = A.Side;
        AmbushWalls.push({
            Id:        Idx,
            Ring:      A.R,
            X:         Ring.X0 + A.XOff,
            CenterY:   CY,
            Side:      WallSide,        // +1 sits in North wall recess, punches toward -Y; -1 sits in South recess
            RetractY:  CY + WallSide * 8.4,
            PunchY:    CY + WallSide * 2.6
        });
    });

    // -- 6 Drop-Gate Guillotine Shutters (heavy overhead slabs dropping on glowing crimson infrared trip-beams!)
    const GateSpecs = [
        { R: 0, XOff: 40.0, Lane: Rings[0].MainLane },
        { R: 1, XOff: 8.0,  Lane: Rings[1].MainLane },
        { R: 2, XOff: 6.0,  Lane: Rings[2].MainLane },
        { R: 2, XOff: 40.0, Lane: (Rings[2].MainLane + 1) % 3 },
        { R: 3, XOff: 42.0, Lane: Rings[3].MainLane },
        { R: 4, XOff: 44.0, Lane: Rings[4].MainLane }
    ];
    GateSpecs.forEach((G, Idx) =>
    {
        const Ring = Rings[G.R];
        DropGates.push({
            Id:      Idx,
            Ring:    G.R,
            X:       Ring.X0 + G.XOff,
            CenterY: LabyrinthLaneY[G.Lane],
            TripX:   Ring.X0 + G.XOff - 10.5 // Trip-beam sits 10.5 m ahead of the falling concrete blade!
        });
    });

    // -- 5 Concealed Reverse-Roller Drag Strips (1 per Ring; real Pacejka SurfaceVelocityQuery — hack [Q] to flip emerald!)
    const RollerSpecs = [
        { R: 0, XOff: 14.0, Lane: (Rings[0].MainLane + 2) % 3 },
        { R: 1, XOff: 18.0, Lane: Rings[1].MainLane },
        { R: 2, XOff: 16.0, Lane: (Rings[2].MainLane + 2) % 3 },
        { R: 3, XOff: 14.0, Lane: (Rings[3].MainLane + 1) % 3 },
        { R: 4, XOff: 14.0, Lane: (Rings[4].MainLane + 1) % 3 }
    ];
    RollerSpecs.forEach((RS, Idx) =>
    {
        const Ring = Rings[RS.R];
        RollerStrips.push({
            Id:           Idx,
            Ring:         RS.R,
            CenterX:      Ring.X0 + RS.XOff + 8.5,
            CenterY:      LabyrinthLaneY[RS.Lane],
            HalfL:        8.5,
            HalfW:        4.8,
            ReverseSpeed: -16.5, // Real Pacejka backward belt drag [m/s]
            HackedSpeed:   22.0  // Flips forward into emerald turbo sling when hacked [Q]!
        });
    });

    // -- 5 Corner Ambush Acid-Mist Vent Slits (1 per Ring cross-hall turn; real Pacejka tyre grip μ drop!)
    for (let R = 0; R < 5; ++R)
    {
        const Ring = Rings[R];
        const TurnY = Ring.MainY;
        const WallSide = TurnY > 10.0 ? 1 : (TurnY < -10.0 ? -1 : (Rng() < 0.5 ? 1 : -1));
        AcidVents.push({
            Id:       R,
            Ring:     R,
            X:        Ring.X1 - 8.5, // Right inside the turning cross-hall!
            CenterY:  TurnY,
            WallY:    TurnY + WallSide * 7.9,
            Side:     WallSide,
            HalfL:    7.2,
            HalfW:    5.3,
            AcidGrip: 0.36 // Real Pacejka μ multiplier when soaked in acid mist!
        });
    }

    // -- 6 Overhead Swinging Brutalist Concrete Pendulums
    const PendSpecs = [
        { R: 0, XOff: 34.0, Lane: (Rings[0].MainLane + 1) % 3 },
        { R: 1, XOff: 28.0, Lane: Rings[1].MainLane },
        { R: 2, XOff: 24.0, Lane: Rings[2].MainLane },
        { R: 3, XOff: 19.0, Lane: Rings[3].MainLane },
        { R: 3, XOff: 33.0, Lane: (Rings[3].MainLane + 2) % 3 },
        { R: 4, XOff: 30.0, Lane: (Rings[4].MainLane + 2) % 3 }
    ];
    PendSpecs.forEach((PS, Idx) =>
    {
        const Ring = Rings[PS.R];
        Pendulums.push({
            Id:      Idx,
            Ring:    PS.R,
            X:       Ring.X0 + PS.XOff,
            CenterY: LabyrinthLaneY[PS.Lane],
            PivotZ:  13.2,
            ArmLen:  10.8,
            MaxAng:  0.48,
            Rate:    1.35 + (Idx % 3) * 0.22,
            Phase:   Idx * 1.1 + Rng() * 2.0
        });
    });

    // ③ Build Static Brutalist Concrete Placements & Procedural Mirage Breakable Walls!
    const StaticPlacements = [];
    const Place = (Shape, Name, X, Y, Z = 0.0, Yaw = 0.0, Collider = true) =>
        StaticPlacements.push({ Shape, Name, Position: [X, Y, Z], Yaw, Material: -1, Collider });

    // Outer colossal perimeter walls (20 m tall concrete enclosure around entire 63 m wide labyrinth!)
    for (let X = -140.0; X <= 204.0; X += 24.0)
    {
        Place("LabMonolithLong", `Perimeter North ${X}`, X,  31.5, 0.0, 0.0, true);
        Place("LabMonolithLong", `Perimeter South ${X}`, X, -31.5, 0.0, 0.0, true);
    }
    // West spawn back wall & East extraction hall flanking monoliths
    for (const Y of [-21.0, 0.0, 21.0])
        Place("LabMonolithLong", `Perimeter West ${Y}`, -154.0, Y, 0.0, 0.5 * Math.PI, true);
    Place("LabMonolithLong",  "Extraction Rear Wall", 218.0,   0.0, 0.0, 0.5 * Math.PI, true);
    Place("LabMonolithShort", "Extraction Rear N",    218.0,  21.0, 0.0, 0.5 * Math.PI, true);
    Place("LabMonolithShort", "Extraction Rear S",    218.0, -21.0, 0.0, 0.5 * Math.PI, true);

    // Spawn Atrium flanking divider monoliths (guides player cleanly into Ring 0 Center with 16.6 m clear canyon sightline!)
    Place("LabMonolithLong", "Spawn Atrium North", -128.0,  10.5, 0.0, 0.0, true);
    Place("LabMonolithLong", "Spawn Atrium South", -128.0, -10.5, 0.0, 0.0, true);
    Place("LabMonolithPillar", "Spawn Portal Pylon N", -114.0,  8.2, 0.0, 0.0, true);
    Place("LabMonolithPillar", "Spawn Portal Pylon S", -114.0, -8.2, 0.0, Math.PI, true);
    Place("LabCatwalkBridge", "Spawn Entry Catwalk", -114.0, 0.0, 12.8, 0.0, false);

    // Build all 5 procedural concrete rings!
    for (let R = 0; R < 5; ++R)
    {
        const Ring = Rings[R];
        const X0   = Ring.X0;
        const X1   = Ring.X1;

        // Longitudinal concrete divider walls separating South(-21) / Center(0) / North(+21) along X0..X1-16
        // Each divider has a 38 m solid brutalist spine leaving a 16 m wide North-South Cross-Passage Hall at X1-16..X1!
        for (const DivY of [-10.5, 10.5])
        {
            const SideSign = DivY > 0.0 ? 1 : -1;
            if (SideSign === Ring.MidShortcutSide)
            {
                // Split divider with a 10 m mid-ring MIRAGE CHEAT SHORTCUT opening!
                Place("LabMonolithShort", `R${R} Div ${DivY > 0 ? "N" : "S"} West`, X0 + 7.2,  DivY, 0.0, 0.0, true);
                Place("LabMonolithShort", `R${R} Div ${DivY > 0 ? "N" : "S"} East`, X0 + 32.8, DivY, 0.0, 0.0, true);
                // Place breakable Mirage Shortcut Partition spanning X0+14.5..X0+25.5 (across Y = DivY)!
                MirageWalls.push({
                    Name:     `Ring ${R + 1} Divider Mirage Shortcut`,
                    Tag:      "mirage_shortcut",
                    Centre:   [X0 + 20.0, DivY, 2.3],
                    Size:     [11.4, 0.64, 4.6],
                    Yaw:      0.0,
                    Material: "mirage",
                    Seed:     (Seed ^ (R * 3917 + (SideSign > 0 ? 101 : 211))) >>> 0
                });
            }
            else
            {
                // Split divider with a 7.4 m steel-framed PERPENDICULAR SLIDING BLAST DOOR creating small alternative-route openings!
                Place("LabMonolithShort", `R${R} Div ${DivY > 0 ? "N" : "S"} West`, X0 + 7.2,  DivY, 0.0, 0.0, true);
                Place("LabMonolithShort", `R${R} Div ${DivY > 0 ? "N" : "S"} East`, X0 + 32.8, DivY, 0.0, 0.0, true);
                // Solid left/right doorway concrete jambs (Collider: true) + elevated overhead concrete wall mass (Z = 6.84..19.2 m, Collider: false)!
                Place("LabDoorJamb",   `R${R} Door Jamb W`, X0 + 15.35, DivY, 0.0, 0.0, true);
                Place("LabDoorJamb",   `R${R} Door Jamb E`, X0 + 24.65, DivY, 0.0, 0.0, true);
                Place("LabDoorLintel", `R${R} Door Lintel`, X0 + 20.0,  DivY, 0.0, 0.0, false);
                PerpDoors.push({
                    Id:        PerpDoors.length,
                    Ring:      R,
                    Axis:      "x",           // Slides along X (perpendicular to the N↔S cross-lane passage!)
                    ClosedX:   X0 + 20.0,
                    ClosedY:   DivY,
                    Yaw:       0.0,
                    SlideDir:  (R % 2 === 0 ? 1.0 : -1.0),
                    MaxSlide:  6.4,           // Opens a crisp tight car-width squeeze opening (4.0..6.4 m) into the parallel lane!
                    HalfLen:   3.6,
                    HalfThick: 0.62,
                    Rate:      0.85 + R * 0.14,
                    Phase:     (R * 1.3 + Rng() * 2.0) % (Math.PI * 2.0)
                });
            }
            // Iconic corner brutalist pylon framing the cross-passage hall turn!
            Place("LabMonolithPillar", `R${R} Pylon ${DivY > 0 ? "N" : "S"}`, X0 + 40.5, DivY, 0.0, 0.0, true);
        }

        // Ring Entry Bulkheads (separating off-lanes into open, mirage cheat wall, or solid dead-end!)
        if (R > 0)
        {
            for (let L = 0; L < 3; ++L)
            {
                const State = Ring.LaneEntry[L];
                const LY    = LabyrinthLaneY[L];
                if (State === "solid")
                {
                    // Solid 20 m concrete dead-end wall spanning wall-to-wall across the 16.6 m lane!
                    Place("LabMonolithGate", `R${R} Solid Wall L${L}`, X0 - 0.5, LY, 0.0, 0.5 * Math.PI, true);
                }
                else if (State === "mirage")
                {
                    // Camouflaged Breakable Mirage Concrete Partition (shatter w/ bumper OR Acoustic Horn [H]!)
                    MirageWalls.push({
                        Name:     `Ring ${R + 1} Mirage Bulkhead`,
                        Tag:      "mirage_bulkhead",
                        Centre:   [X0 - 0.5, LY, 2.4],
                        Size:     [0.64, 16.2, 4.8],
                        Yaw:      0.0,
                        Material: "mirage",
                        Seed:     (Seed ^ (R * 7919 + L * 431)) >>> 0
                    });
                }
            }
        }

        // Overhead Stalker Catwalk Bridges (dramatic sunlight shafts across every lane!)
        Place("LabCatwalkBridge", `R${R} Catwalk Center`, X0 + 22.0,   0.0, 12.8, 0.0, false);
        Place("LabCatwalkBridge", `R${R} Catwalk North`,  X0 + 34.0,  21.0, 12.8, 0.0, false);
        Place("LabCatwalkBridge", `R${R} Catwalk South`,  X0 + 12.0, -21.0, 12.8, 0.0, false);
    }

    // Guaranteed Hero Mirage Wall near end of Ring 4 framing the Extraction Hall entry (fun Acoustic Horn finale!)
    MirageWalls.push({
        Name:     "Extraction Gate Mirage Slab",
        Tag:      "mirage_finale",
        Centre:   [176.5, 0.0, 2.4],
        Size:     [0.64, 16.2, 4.8],
        Yaw:      0.0,
        Material: "mirage",
        Seed:     (Seed ^ 0xF14A1E99) >>> 0
    });

    // 6th Perpendicular Sliding Blast Door: guards a narrow alternative side-lane entry into the Extraction Portal Hall!
    const PortalDoorSide = (Rng() < 0.5 ? 1 : -1);
    const PortalDoorY    = PortalDoorSide * 21.0;
    Place("LabDoorJamb",   "Extraction Side Door Jamb N", 176.5, PortalDoorY + 4.65, 0.0, 0.5 * Math.PI, true);
    Place("LabDoorJamb",   "Extraction Side Door Jamb S", 176.5, PortalDoorY - 4.65, 0.0, 0.5 * Math.PI, true);
    Place("LabDoorLintel", "Extraction Side Door Lintel", 176.5, PortalDoorY,        0.0, 0.5 * Math.PI, false);
    PerpDoors.push({
        Id:        PerpDoors.length,
        Ring:      4,
        Axis:      "y",           // Slides along Y (perpendicular to the +X alternative extraction entry!)
        ClosedX:   176.5,
        ClosedY:   PortalDoorY,
        Yaw:       0.5 * Math.PI,
        SlideDir:  -PortalDoorSide,
        MaxSlide:  6.4,
        HalfLen:   3.6,
        HalfThick: 0.62,
        Rate:      1.25,
        Phase:     Rng() * Math.PI * 2.0
    });

    // Multi-Level Concrete Ramps & Top/Bottom Overpass Bridges (5 real 480 Hz drivable viaducts via CastSurfaceRay!)
    const Viaduct1Lane = (Rings[1].MainLane + 1) % 3;
    const Viaduct2Lane = (Rings[3].MainLane + 2) % 3;
    const Launch4Lane  = (Rings[4].MainLane + (Rng() < 0.5 ? 1 : 2)) % 3; // North (+21) or South (-21) high-road!
    const Surfaces = [
        // ① Ring 0 Cross-Canyon Top/Bottom Overpass Bridge (spans S→N at X=-56 RIGHT OVER the Center Spawn Canyon!)
        ConstructTabletopRampSurface({
            Key:            "LabCrossBridgeA",
            ShapePrefix:    "LabCrossBridge50",
            Origin:         [-56.0, -27.0, 0.0],
            Yaw:            0.5 * Math.PI,
            UpLength:       16.0,
            DeckLength:     22.0,
            DownLength:     16.0,
            Height:         5.0,
            Width:          7.4,
            Thickness:      0.58,
            AccentMaterial: CourseMaterial.LabSonicCyan
        }),
        // ② Ring 2 Cross-Canyon Top/Bottom Overpass Bridge (spans N→S at X=+56 RIGHT OVER Center Ring 2!)
        ConstructTabletopRampSurface({
            Key:            "LabCrossBridgeB",
            ShapePrefix:    "LabCrossBridge50",
            Origin:         [56.0, 27.0, 0.0],
            Yaw:            -0.5 * Math.PI,
            UpLength:       16.0,
            DeckLength:     22.0,
            DownLength:     16.0,
            Height:         5.0,
            Width:          7.4,
            Thickness:      0.58,
            AccentMaterial: CourseMaterial.LabHazardAmber
        }),
        // ③ Ring 1 Longitudinal Elevated Concrete Viaduct (climbs UP & OVER Ring 1 Cross-Hall on high-road side lane!)
        ConstructTabletopRampSurface({
            Key:            "LabViaductRamp1",
            ShapePrefix:    "LabViaduct49",
            Origin:         [-46.0, LabyrinthLaneY[Viaduct1Lane], 0.0],
            Yaw:            0.0,
            UpLength:       16.0,
            DeckLength:     20.0,
            DownLength:     16.0,
            Height:         4.9,
            Width:          6.8,
            Thickness:      0.58,
            AccentMaterial: CourseMaterial.LabSonicCyan
        }),
        // ④ Ring 3 Longitudinal Elevated Concrete Viaduct (climbs UP & OVER Ring 3 Cross-Hall on high-road side lane!)
        ConstructTabletopRampSurface({
            Key:            "LabViaductRamp2",
            ShapePrefix:    "LabViaduct49",
            Origin:         [66.0, LabyrinthLaneY[Viaduct2Lane], 0.0],
            Yaw:            0.0,
            UpLength:       16.0,
            DeckLength:     20.0,
            DownLength:     16.0,
            Height:         4.9,
            Width:          6.8,
            Thickness:      0.58,
            AccentMaterial: CourseMaterial.LabHazardAmber
        }),
        // ⑤ Ring 4 Extraction Overpass Ramp (elevated emerald deck racing straight into the Blind Split Portals!)
        ConstructTabletopRampSurface({
            Key:            "LabLaunchRamp4",
            ShapePrefix:    "LabLaunch46",
            Origin:         [122.0, LabyrinthLaneY[Launch4Lane], 0.0],
            Yaw:            0.0,
            UpLength:       14.0,
            DeckLength:     14.0,
            DownLength:     14.0,
            Height:         4.6,
            Width:          7.2,
            Thickness:      0.56,
            AccentMaterial: CourseMaterial.LabBeaconEmerald
        })
    ];
    const SurfaceBundle = ConstructSurfaceSegmentDescriptors(Surfaces);
    for (const P of SurfaceBundle.Placements)
        StaticPlacements.push(P);

    // Fluted brutalist concrete bridge piers tucked cleanly at outer divider edges (Y = ±10.2) framing both Cross-Bridge underpasses!
    Place("LabBridgePier", "Bridge A Pier N", -56.0,  10.2, 0.0, 0.0, true);
    Place("LabBridgePier", "Bridge A Pier S", -56.0, -10.2, 0.0, 0.0, true);
    Place("LabBridgePier", "Bridge B Pier N",  56.0,  10.2, 0.0, 0.0, true);
    Place("LabBridgePier", "Bridge B Pier S",  56.0, -10.2, 0.0, 0.0, true);

    // Static hardware telling players where Sliding Walls, Heading Pistons, Guillotines, Roller Bays & Acid Vents live!
    for (const SW of SlidingWalls)
        Place("LabFloorRail", `Slide Rail ${SW.Id}`, SW.X, SW.CenterY, 0.0, 0.0, false);
    for (const HW of HeadingWalls)
        Place("LabLongRail", `Heading Rail ${HW.Id}`, HW.CenterX, HW.Y, 0.0, 0.0, false);
    for (const DG of DropGates)
        Place("LabOverheadSoffit", `Guillotine Soffit ${DG.Id}`, DG.X, DG.CenterY, 6.2, 0.0, false);
    for (const RS of RollerStrips)
        Place("LabRollerFrame", `Roller Frame ${RS.Id}`, RS.CenterX, RS.CenterY, 0.0, 0.0, false);
    for (const AV of AcidVents)
        Place("LabVentHousing", `Acid Vent ${AV.Id}`, AV.X, AV.WallY, 0.0, AV.Side > 0 ? Math.PI : 0.0, false);

    // Towering 42 m Emerald Extraction Spire + Twin Blind Split Portals at East Finish Hall!
    Place("LabExtractionSpire",  "Extraction Beacon Spire",     206.0,   0.0, 0.0, 0.0, true);
    Place("LabSplitPortalAcid",  "Blind Portal: Acid Wasteland", 196.0,  14.0, 0.0, -0.22, false);
    Place("LabSplitPortalRocket","Blind Portal: Rocket Trench",  196.0, -14.0, 0.0,  0.22, false);

    // Decorative fractured brutalist rubble boulders tucked strictly against outer corners (zero lane blockage!)
    for (let B = 0; B < 6; ++B)
    {
        const RX = -100.0 + B * 52.0 + Rng() * 14.0;
        const RY = (B % 2 === 0 ? 1 : -1) * 27.6;
        Place("LabFractureRubble", `Corner Rubble ${B}`, RX, RY, 0.0, Rng() * Math.PI * 2.0, false);
    }

    // ④ Teleport Zones for Inspector / Keys 1–9
    const Zones = [
        { Name: "Labyrinth Spawn Atrium",   Note: "Towering 20 m concrete canyon start",           Position: [-134.0,   0.0, 0.65], Yaw: 0.0 },
        { Name: "Heading Piston & Door",    Note: "Surging ±X piston wall & perp blast door",      Position: [ -98.0,   0.0, 0.65], Yaw: 0.0 },
        { Name: "Top/Bottom Bridge Ramp",   Note: "Climb +5.0 m overpass bridge over canyon",      Position: [ -56.0, -24.5, 0.72], Yaw: 0.5 * Math.PI },
        { Name: "Ring 2 · Viaduct & Rollers", Note: "Elevated viaduct & hackable reverse belt",    Position: [ -44.0, Rings[1].MainY, 0.65], Yaw: 0.0 },
        { Name: "Ring 3 · Slam-Plate Core", Note: "Pneumatic kicker & cross-bridge overpass",      Position: [  12.0, Rings[2].MainY, 0.65], Yaw: 0.0 },
        { Name: "Ring 4 · Acid Ambush",     Note: "Corner acid-mist slick & guillotine",           Position: [  68.0, Rings[3].MainY, 0.65], Yaw: 0.0 },
        { Name: "Ring 5 · Mirage Finale",   Note: "Blast final mirage slab w/ Horn [H]",           Position: [ 124.0,   0.0, 0.65], Yaw: 0.0 },
        { Name: "Blind Split Portals",      Note: "42 m Extraction Beacon & twin portals",         Position: [ 182.0,   0.0, 0.65], Yaw: 0.0 }
    ];

    return {
        Seed,
        SeedTag,
        Rings,
        SlidingWalls,
        HeadingWalls,
        PerpDoors,
        Surfaces,
        SurfaceBundle,
        Pitfalls,
        SlamPlates,
        AmbushWalls,
        DropGates,
        RollerStrips,
        AcidVents,
        Pendulums,
        MirageWalls,
        StaticPlacements,
        Zones
    };
}

export function PrepareLabyrinthMaze(Seed = null)
{
    ActiveMaze = GenerateLabyrinthMaze(Seed);
    LiveHeightOracle = null;
    return ActiveMaze;
}

export function GetActiveLabyrinthMaze()
{
    if (!ActiveMaze)
        ActiveMaze = GenerateLabyrinthMaze(0x20261AB1);
    return ActiveMaze;
}

//------------------------------------------------------------------------------------------------------------------------
//                                    EXPORTS FOR LEVELSTRUCTURE & FACETSTRUCTURE
//------------------------------------------------------------------------------------------------------------------------

export const LabyrinthSpawn = Object.freeze({
    Position: [-134.0, 0.0, 0.65],
    Yaw:      0.0
});

export function ResolveLabyrinthSurfaces()
{
    return GetActiveLabyrinthMaze().Surfaces.slice();
}

export function ResolveLabyrinthShapes()
{
    const Maze = GetActiveLabyrinthMaze();
    const Rng  = ConstructRandom(Maze.Seed ^ 0x9E3779B9);
    const RubbleSeed = (Maze.Seed ^ 0xA55A1234) >>> 0;
    return {
        // Multi-level Top/Bottom concrete overpass bridges & sky-ramps (480 Hz CastSurfaceRay surfaces!)
        ...Maze.SurfaceBundle.Shapes,
        // Static brutalist monoliths & hardware
        LabMonolithLong:      { Emit: () => EmitMonolithWall(24.0, 4.4, 20.0, CourseMaterial.LabConcreteMid, CourseMaterial.LabConcreteLight, true), Voxel: 0.28, Cards: 5 },
        LabMonolithShort:     { Emit: () => EmitMonolithWall(14.5, 4.4, 19.2, CourseMaterial.LabConcreteLight, CourseMaterial.LabConcreteMid, true), Voxel: 0.26, Cards: 5 },
        LabMonolithGate:      { Emit: () => EmitMonolithWall(16.8, 4.2, 19.6, CourseMaterial.LabConcreteDark, CourseMaterial.LabConcreteMid, true), Voxel: 0.26, Cards: 5 },
        LabMonolithPillar:    { Emit: () => EmitMonolithPillar(), Voxel: 0.24, Cards: 6 },
        LabBridgePier:        { Emit: () => EmitBridgePier(), Voxel: 0.20, Cards: 6 },
        LabDoorJamb:          { Emit: () => EmitDoorJamb(), Voxel: 0.22, Cards: 6 },
        LabDoorLintel:        { Emit: () => EmitDoorLintel(), Voxel: 0.26, Cards: 5, Distance: false },
        LabCatwalkBridge:     { Emit: () => EmitCatwalkBridge(), Voxel: 0.26, Cards: 5, Distance: false },
        LabOverheadSoffit:    { Emit: () => EmitGuillotineSoffit(), Voxel: 0.24, Cards: 5, Distance: false },
        LabFloorRail:         { Emit: () => EmitFloorRailTrack(), Voxel: 0.16, Cards: 6, Distance: false },
        LabLongRail:          { Emit: () => EmitLongRailTrack(), Voxel: 0.16, Cards: 6, Distance: false },
        LabRollerFrame:       { Emit: () => EmitRollerBayFrame(), Voxel: 0.18, Cards: 6, Distance: false },
        LabVentHousing:       { Emit: () => EmitAcidVentHousing(), Voxel: 0.14, Cards: 8, Distance: false },
        LabExtractionSpire:   { Emit: () => EmitExtractionSpire(), Voxel: 0.34, Cards: 4 },
        LabSplitPortalAcid:   { Emit: () => EmitSplitPortalArch(CourseMaterial.LabAcidGlow), Voxel: 0.24, Cards: 5, Distance: false },
        LabSplitPortalRocket: { Emit: () => EmitSplitPortalArch(CourseMaterial.LabPortalViolet), Voxel: 0.24, Cards: 5, Distance: false },
        LabFractureRubble:    {
            Emit: () =>
            {
                const F = [];
                const LocalRng = ConstructRandom(RubbleSeed);
                EmitFracturedConcreteAt(F, 0.0, 0.0, 0.72, 2.2, 1.8, 1.4, 0.2, CourseMaterial.LabConcreteLight, CourseMaterial.LabConcreteDark, LocalRng, 3);
                return F;
            },
            Voxel: 0.16, Cards: 6, Distance: false
        },
        // Dynamic shifting megawalls, heading pistons, perpendicular blast doors, hidden traps & ability VFX
        LabSlidingWall:       { Emit: () => EmitSlidingMegawall(), Voxel: 0.26, Cards: 4, Distance: false },
        LabBeaconRed:         { Emit: () => EmitStatusBeacon(CourseMaterial.LabWarningRed), Voxel: 0.16, Cards: 4, Distance: false },
        LabHeadingWall:       { Emit: () => EmitHeadingWall(), Voxel: 0.24, Cards: 4, Distance: false },
        LabPerpDoor:          { Emit: () => EmitPerpDoor(), Voxel: 0.20, Cards: 4, Distance: false },
        LabTrapSlab:          { Emit: () => EmitTrapSlab(), Voxel: 0.22, Cards: 5, Distance: false },
        LabSlamPlate:         { Emit: () => EmitSlamPlate(), Voxel: 0.20, Cards: 5, Distance: false },
        LabAmbushWall:        { Emit: () => EmitAmbushWall(), Voxel: 0.22, Cards: 5, Distance: false },
        LabDropGate:          { Emit: () => EmitDropGate(), Voxel: 0.22, Cards: 5, Distance: false },
        LabTripBeam:          { Emit: () => EmitTripBeam(), Voxel: 0.12, Cards: 4, Distance: false },
        LabRollerSlat:        { Emit: () => EmitRollerSlat(), Voxel: 0.16, Cards: 4, Distance: false },
        LabAcidSlick:         { Emit: () => EmitAcidSlick(), Voxel: 0.20, Cards: 4, Distance: false },
        LabPendulumArm:       { Emit: () => EmitPendulumArm(), Voxel: 0.18, Cards: 4, Distance: false },
        LabPendulumBlock:     { Emit: () => EmitPendulumBlock(), Voxel: 0.18, Cards: 5, Distance: false },
        LabSonicRing:         { Emit: () => EmitAbilityRing(CourseMaterial.LabSonicCyan), Voxel: 0.18, Cards: 4, Distance: false },
        LabHackRing:          { Emit: () => EmitAbilityRing(CourseMaterial.LabBeaconEmerald), Voxel: 0.18, Cards: 4, Distance: false }
    };
}

export function ResolveLabyrinthPlacements()
{
    return GetActiveLabyrinthMaze().StaticPlacements.slice();
}

export function ResolveLabyrinthZones()
{
    return GetActiveLabyrinthMaze().Zones.slice();
}

export function ResolveLabyrinthCones()
{
    // Subtle industrial marker cones strictly at the outer spawn & extraction framing edges (zero lane blockage!)
    return [
        { Name: "Spawn Gate Cone N", Position: [-126.0,  6.4, 0.0], Yaw: 0.0 },
        { Name: "Spawn Gate Cone S", Position: [-126.0, -6.4, 0.0], Yaw: 0.0 },
        { Name: "Portal Cone N",     Position: [ 188.0,  6.8, 0.0], Yaw: 0.0 },
        { Name: "Portal Cone S",     Position: [ 188.0, -6.8, 0.0], Yaw: 0.0 }
    ];
}

export function ResolveLabyrinthBreakables()
{
    const Maze = GetActiveLabyrinthMaze();
    return Maze.MirageWalls.map((W) => ({
        Name:         W.Name,
        Material:     W.Material,
        Shape:        { Kind: "box", Extent: W.Size },
        Position:     [W.Centre[0], W.Centre[1], 0.04],
        Yaw:          W.Yaw,
        Approach:     W.Size[0] < W.Size[1] ? [-1.0, 0.0, 0.0] : [0.0, -1.0, 0.0],
        Capacity:     18,
        Ceiling:      16,
        DensityScale: 0.06,
        Threshold:    1400,
        Seed:         W.Seed,
        LabyrinthTag: W.Tag
    }));
}

// 📝 Brutalist concrete floor plane with REAL physical cut-out shafts underneath all 6 False-Slab Pitfalls
//    (100% concealed beneath the flush 10 m LabTrapSlab when closed; reveals a 2.8 m deep shaft + 23.3° rescue ramp when opened!)
export function EmitLabyrinthGroundFacets()
{
    const Maze = GetActiveLabyrinthMaze();
    const F = [];
    const PitRects = Maze.Pitfalls.map((P) => ({
        X0:     P.X0,
        X1:     P.X0 + P.SlabLen,
        SplitX: P.X0 + P.DropLen,
        Y0:     P.CenterY - P.HalfW,
        Y1:     P.CenterY + P.HalfW,
        Depth:  P.Depth
    }));

    // Build exact sorted X & Y grid partitions aligned to every pit edge so zero triangles overlap a pit hole!
    const XSet = new Set([-166.0, -152.0, -104.0, -48.0, 8.0, 64.0, 120.0, 176.0, 230.0]);
    const YSet = new Set([-40.0, -29.3, -12.7, -8.3, 8.3, 12.7, 29.3, 40.0]);
    for (const P of PitRects)
    {
        XSet.add(P.X0);
        XSet.add(P.X1);
        YSet.add(P.Y0);
        YSet.add(P.Y1);
    }
    const Xs = Array.from(XSet).sort((A, B) => A - B);
    const Ys = Array.from(YSet).sort((A, B) => A - B);

    for (let IX = 0; IX < Xs.length - 1; ++IX)
    {
        const XA = Xs[IX], XB = Xs[IX + 1];
        if (XB - XA < 0.05) continue;
        const MX = 0.5 * (XA + XB);
        for (let IY = 0; IY < Ys.length - 1; ++IY)
        {
            const YA = Ys[IY], YB = Ys[IY + 1];
            if (YB - YA < 0.05) continue;
            const MY = 0.5 * (YA + YB);

            // Check if this cell lies inside one of the 6 False-Slab Pitfall shafts
            const InsidePit = PitRects.some((P) => MX > P.X0 && MX < P.X1 && MY > P.Y0 && MY < P.Y1);
            if (InsidePit)
                continue;

            const IsCorridor = Math.abs(MY) < 8.3 || (Math.abs(MY) > 12.7 && Math.abs(MY) < 29.3);
            const Mat = IsCorridor ? CourseMaterial.LabFloorSlate : CourseMaterial.LabConcreteDark;
            EmitQuad(F, [XA, YA, 0.0], [XB, YA, 0.0], [XB, YB, 0.0], [XA, YB, 0.0], Mat);
        }
    }

    // Emit the 3.6 m deep concrete pit interior + smooth East escape ramp for every False-Slab Pitfall!
    for (const P of PitRects)
    {
        const ZB = -P.Depth;
        // Flat deep pit floor under tilting slab (X0..SplitX)
        EmitQuad(F, [P.X0, P.Y0, ZB], [P.SplitX, P.Y0, ZB], [P.SplitX, P.Y1, ZB], [P.X0, P.Y1, ZB], CourseMaterial.LabConcreteDark);
        // Smooth inclined steel-concrete rescue ramp rising back to Z = 0.0 across (SplitX..X1)
        EmitQuad(F, [P.SplitX, P.Y0, ZB], [P.X1, P.Y0, 0.0], [P.X1, P.Y1, 0.0], [P.SplitX, P.Y1, ZB], CourseMaterial.LabConcreteMid);
        // Amber safety strip along the ramp centerline
        EmitQuad(F, [P.SplitX + 0.2, 0.5 * (P.Y0 + P.Y1) - 0.5, ZB + 0.04],
                    [P.X1 - 0.2,     0.5 * (P.Y0 + P.Y1) - 0.5, 0.02],
                    [P.X1 - 0.2,     0.5 * (P.Y0 + P.Y1) + 0.5, 0.02],
                    [P.SplitX + 0.2, 0.5 * (P.Y0 + P.Y1) + 0.5, ZB + 0.04], CourseMaterial.LabHazardAmber);
        // West vertical pit wall (normal +X)
        EmitQuad(F, [P.X0, P.Y1, ZB], [P.X0, P.Y0, ZB], [P.X0, P.Y0, 0.0], [P.X0, P.Y1, 0.0], CourseMaterial.LabConcreteDark);
        // South vertical pit wall (normal +Y)
        EmitQuad(F, [P.X0, P.Y0, ZB], [P.SplitX, P.Y0, ZB], [P.SplitX, P.Y0, 0.0], [P.X0, P.Y0, 0.0], CourseMaterial.LabConcreteDark);
        F.push({ Family: CourseMaterial.LabConcreteDark, Corners: [[P.SplitX, P.Y0, ZB], [P.X1, P.Y0, 0.0], [P.SplitX, P.Y0, 0.0]] });
        // North vertical pit wall (normal -Y)
        EmitQuad(F, [P.SplitX, P.Y1, ZB], [P.X0, P.Y1, ZB], [P.X0, P.Y1, 0.0], [P.SplitX, P.Y1, 0.0], CourseMaterial.LabConcreteDark);
        F.push({ Family: CourseMaterial.LabConcreteDark, Corners: [[P.X1, P.Y1, 0.0], [P.SplitX, P.Y1, ZB], [P.SplitX, P.Y1, 0.0]] });
    }

    // Subtle recessed glowing cyan runway chevrons & side curbing in the Spawn Atrium (guides player into Ring 0!)
    for (const CX of [-125.0, -119.0, -113.0, -107.0])
    {
        EmitQuad(F, [CX - 0.85, -1.6, 0.012], [CX + 0.15, -1.6, 0.012], [CX + 1.05, 0.0, 0.012], [CX + 0.05, 0.0, 0.012], CourseMaterial.LabSonicCyan);
        EmitQuad(F, [CX + 0.05,  0.0, 0.012], [CX + 1.05,  0.0, 0.012], [CX + 0.15, 1.6, 0.012], [CX - 0.85, 1.6, 0.012], CourseMaterial.LabSonicCyan);
    }
    EmitQuad(F, [-140.0, -7.75, 0.012], [-104.0, -7.75, 0.012], [-104.0, -7.40, 0.012], [-140.0, -7.40, 0.012], CourseMaterial.LabHazardAmber);
    EmitQuad(F, [-140.0,  7.40, 0.012], [-104.0,  7.40, 0.012], [-104.0,  7.75, 0.012], [-140.0,  7.75, 0.012], CourseMaterial.LabHazardAmber);

    // Subtle recessed centerline guidance strip along the Extraction Portal Approach (X = 176..204)
    EmitQuad(F, [176.5, -0.45, 0.015], [202.0, -0.45, 0.015], [202.0, 0.45, 0.015], [176.5, 0.45, 0.015], CourseMaterial.LabBeaconEmerald);

    return F;
}
