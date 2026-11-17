//============================================================================================================================================
//                                                        DEMOLISHRUNSTRUCTURE.JS
//============================================================================================================================================
// 🏗️ Demolish Run · Fracture City ("demolish") — Procedurally Generated & Procedurally Fractured Demolition Neighborhood:
//    • Generates a brand-new random city layout, house mix, color palette, route forks & 3D plane-cut fractured ruins
//      on EVERY entry into the level (seeded PRNG + optional ?seed=... reproducible override).
//    • Richly detailed watertight 2-manifold architecture sculpting a natural winding residential/mixed-use pathway:
//        - Hollow Drive-Through 2-Story Garage Houses (driveway apron, timber ceiling rafters, warm glowing interior shop light,
//          side tool chest/workbench, 2nd-story living floor w/ recessed windows & pitched gable roof + chimney, breakable front
//          garage door & breakable rear brick wall!).
//        - Hollow Drive-Through Corner Storefronts / Parlors (brick piers, awning, warm lit interior, breakable tempered glass
//          storefront, breakable interior drywall partition, breakable rear wall, upper stories w/ balconies & fractured parapets!).
//        - 4 Flanking Residential House Archetypes (Gable House, Red-Brick Brownstone, Terracotta Duplex, Fractured Skeletal Ruin)
//          sculpting natural sunlit crevices, alleys, and shortcuts along the curving demolition avenue.
//    • Real 3D FractureStructure.SplitPart convex-hull plane-cutting creates unique jagged fractured building corners, crumbling
//      parapets, collapsing facade slabs, and tumbling masonry boulders every single run.
//    • 5 Articulated Giant Wrecking-Ball Demolition Cranes, 12 C4 Demolition Charges, 12 Collapsing Fractured Facade Slabs &
//      36 Tumbling Fractured Rubble Chunks driven by DemolishRunSequence.

import { CourseMaterial } from "./MaterialSpecification.js";
import { EmitBoxFacets, EmitQuad } from "./CourseStructure.js";
import { ConstructBox, SplitPart, MeasureMass, MeasureFaceNormal, TriangulateFace, ConstructRandom } from "./FractureStructure.js";

//------------------------------------------------------------------------------------------------------------------------
//                                             POOL COUNTS & GLOBAL CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

export const DemolishPoolCounts = Object.freeze({
    CraneBases:     5,
    CraneTowers:    5,
    CraneJibs:      5,
    CraneCables:    5,
    WreckingBalls:  5,
    ChargePacks:    12,
    CollapseWalls:  12,
    FallingChunks:  36
});

export const DemolishBounds = Object.freeze({
    Minimum: [-168.0, -56.0],
    Maximum: [ 252.0,  56.0]
});

export const DemolishFinishX = 216.0;

const BlockCentersX = Object.freeze([-92.0, -38.0, 16.0, 70.0, 124.0, 178.0]);

//------------------------------------------------------------------------------------------------------------------------
//                                  WATERTIGHT 2-MANIFOLD ARCHITECTURAL PRIMITIVE HELPERS
//------------------------------------------------------------------------------------------------------------------------

const Gap = 0.018; // 18 mm separation guarantees strict disjoint watertight 2-manifold sub-solids

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

// 📝 Watertight triangular pitched gable roof prism: ridge runs along X (LengthX), gable span along Y (WidthY), peak height H.
function EmitGableRoofAt(Out, CX, CY, BaseZ, LengthX, WidthY, PeakH, Yaw, RoofMat, GableMat)
{
    const HX = 0.5 * LengthX;
    const HY = 0.5 * WidthY;
    const C = Math.cos(Yaw), S = Math.sin(Yaw);
    const ToW = (Lx, Ly, Lz) => [CX + Lx * C - Ly * S, CY + Lx * S + Ly * C, BaseZ + Lz];

    const A0 = ToW(-HX, -HY, 0.0);
    const A1 = ToW( HX, -HY, 0.0);
    const A2 = ToW( HX,  HY, 0.0);
    const A3 = ToW(-HX,  HY, 0.0);
    const R0 = ToW(-HX, 0.0, PeakH);
    const R1 = ToW( HX, 0.0, PeakH);

    // Bottom horizontal cap (-Z outward)
    EmitQuad(Out, A0, A3, A2, A1, GableMat);
    // -Y roof slope (outward normal points -Y, +Z)
    EmitQuad(Out, A0, A1, R1, R0, RoofMat);
    // +Y roof slope (outward normal points +Y, +Z)
    EmitQuad(Out, A2, A3, R0, R1, RoofMat);
    // -X gable triangle (outward normal -X)
    Out.push({ Family: GableMat, Corners: [A3, A0, R0] });
    // +X gable triangle (outward normal +X)
    Out.push({ Family: GableMat, Corners: [A1, A2, R1] });
}

// 📝 Watertight 3D plane-cut fractured convex polyhedron via real FractureStructure.SplitPart!
//    Starts from a box [SX, SY, SZ], applies CutCount randomized diagonal fracture planes using Rng,
//    and emits closed outward-wound triangles with exterior SkinMat and raw cut InteriorMat.
function EmitFracturedBlockAt(Out, CX, CY, CenterZ, SX, SY, SZ, Yaw, SkinMat, InteriorMat, Rng, CutCount = 3, BiasTop = true)
{
    let Part = ConstructBox(SX, SY, SZ);
    for (let Cut = 0; Cut < CutCount; ++Cut)
    {
        const Nx = (Rng() - 0.5) * 1.8;
        const Ny = (Rng() - 0.5) * 1.8;
        const Nz = BiasTop ? (0.45 + Rng() * 0.95) : ((Rng() - 0.5) * 1.6);
        const Len = Math.hypot(Nx, Ny, Nz) || 1.0;
        const Dir = [Nx / Len, Ny / Len, Nz / Len];
        const HalfProj = 0.5 * (Math.abs(Dir[0]) * SX + Math.abs(Dir[1]) * SY + Math.abs(Dir[2]) * SZ);
        const Dist = HalfProj * (0.28 + Rng() * 0.38);
        const Halves = SplitPart(Part, Dir, Dist);
        if (Halves && Halves[0] && Halves[0].Faces.length >= 4)
        {
            const M = MeasureMass(Halves[0]);
            if (M.Volume > 0.04 * SX * SY * SZ)
                Part = Halves[0];
        }
    }

    const C = Math.cos(Yaw), S = Math.sin(Yaw);
    const ToW = (P) => [
        CX + P[0] * C - P[1] * S,
        CY + P[0] * S + P[1] * C,
        CenterZ + P[2]
    ];

    for (const Face of Part.Faces)
    {
        if (!Face.Indices || Face.Indices.length < 3)
            continue;
        const Family = Face.Interior ? InteriorMat : SkinMat;
        const { Triangles } = TriangulateFace(Part, Face);
        for (const [I0, I1, I2] of Triangles)
        {
            Out.push({
                Family,
                Corners: [
                    ToW(Part.Vertices[Face.Indices[I0]]),
                    ToW(Part.Vertices[Face.Indices[I1]]),
                    ToW(Part.Vertices[Face.Indices[I2]])
                ]
            });
        }
    }
}

// 📝 Watertight faceted geodesic/octahedral sphere for heavy hammered cast-iron wrecking ball.
function EmitWreckingSphereFacets(Radius, Material)
{
    const Out = [];
    const LatRings = 6;
    const LonSteps = 10;
    const Ring = (Lat) =>
    {
        const Phi = (Lat / LatRings) * Math.PI;
        const Z = Radius * Math.cos(Phi);
        const R = Radius * Math.sin(Phi);
        const Pts = [];
        for (let K = 0; K < LonSteps; ++K)
        {
            const Th = (K / LonSteps) * Math.PI * 2.0;
            Pts.push([R * Math.cos(Th), R * Math.sin(Th), Z]);
        }
        return Pts;
    };
    const North = [0.0, 0.0,  Radius];
    const South = [0.0, 0.0, -Radius];
    const Rings = [];
    for (let I = 1; I < LatRings; ++I)
        Rings.push(Ring(I));

    // Top cap
    for (let K = 0; K < LonSteps; ++K)
    {
        const N = (K + 1) % LonSteps;
        Out.push({ Family: Material, Corners: [North, Rings[0][K], Rings[0][N]] });
    }
    // Middle bands
    for (let R = 0; R < Rings.length - 1; ++R)
    {
        for (let K = 0; K < LonSteps; ++K)
        {
            const N = (K + 1) % LonSteps;
            EmitQuad(Out, Rings[R][K], Rings[R + 1][K], Rings[R + 1][N], Rings[R][N], Material);
        }
    }
    // Bottom cap
    const Last = Rings[Rings.length - 1];
    for (let K = 0; K < LonSteps; ++K)
    {
        const N = (K + 1) % LonSteps;
        Out.push({ Family: Material, Corners: [South, Last[N], Last[K]] });
    }
    return Out;
}

// 📝 Helper to add an axis-aligned solid collider box.
function AddBoxCollider(Colliders, CX, CY, BottomZ, SX, SY, SZ)
{
    Colliders.push({
        Minimum: [CX - 0.5 * SX, CY - 0.5 * SY, BottomZ],
        Maximum: [CX + 0.5 * SX, CY + 0.5 * SY, BottomZ + SZ]
    });
}

//------------------------------------------------------------------------------------------------------------------------
//                          HIGH-DETAIL ARCHITECTURAL BUILDING GENERATORS (HOLLOW + FLANKING)
//------------------------------------------------------------------------------------------------------------------------

const WallPalettes = Object.freeze([
    { Wall: CourseMaterial.DemoBrickRed,    Trim: CourseMaterial.DemoStuccoCream, Roof: CourseMaterial.DemoRoofSlate, Name: "Brick" },
    { Wall: CourseMaterial.DemoBrickOchre,  Trim: CourseMaterial.DemoStuccoCream, Roof: CourseMaterial.DemoRoofTile,  Name: "Ochre" },
    { Wall: CourseMaterial.DemoStuccoCream, Trim: CourseMaterial.DemoTimber,      Roof: CourseMaterial.DemoRoofTile,  Name: "Stucco" },
    { Wall: CourseMaterial.DemoStuccoSage,  Trim: CourseMaterial.DemoStuccoCream, Roof: CourseMaterial.DemoRoofSlate, Name: "Sage" }
]);

// 📝 HOLLOW DRIVE-THROUGH 2-STORY GARAGE HOUSE (aligned along +X so player drives straight through!):
//    • Clear interior bay: WidthY = 5.4 m, DepthX = 9.4 m, ClearHeight = 3.45 m
//    • Detailed concrete driveway aprons, brick/stucco side walls, interior timber ceiling rafters, warm glowing
//      garage shop light tube, tucked-away side workbench + red tool chest, 2nd-story living floor with recessed
//      windows, sills & lintels, pitched gable roof, brick chimney, and procedurally fractured roof/parapet corner!
//    • Registers exact left/right/upper AABBs in Colliders (tunnel remains 100% physically open!).
function EmitDriveThroughGarageHouse(Out, Colliders, CX, CY, Palette, Rng, BlockIdx)
{
    const DepthX   = 9.4;
    const ClearW   = 5.4;
    const WallT    = 0.72;
    const OuterW   = ClearW + 2.0 * WallT; // 6.84 m
    const ClearH   = 3.45;
    const Floor2H  = 3.35;
    const Floor2Z  = ClearH + 0.28;        // 3.73 m
    const RoofBase = Floor2Z + Floor2H + Gap;

    // ① Concrete driveway aprons & thin interior garage floor pad (Z = 0.00..0.04, drivable smoothly!)
    EmitBoxAt(Out, CX, CY, 0.00, DepthX + 5.2, ClearW + 0.4, 0.04, CourseMaterial.Concrete);

    // ② Left & Right Structural Ground-Floor Garage Walls (hollow center tunnel!)
    for (const Side of [-1, 1])
    {
        const WY = CY + Side * (0.5 * ClearW + 0.5 * WallT);
        // Stone foundation base + masonry wall
        EmitBoxAt(Out, CX, WY, 0.00, DepthX, WallT + 0.10, 0.45, CourseMaterial.DemoRubbleConcrete);
        EmitBoxAt(Out, CX, WY, 0.47, DepthX, WallT, ClearH - 0.47, Palette.Wall);
        // Interior drywall lining inside garage bay
        const InnerY = CY + Side * (0.5 * ClearW + 0.08);
        EmitBoxAt(Out, CX, InnerY, 0.48, DepthX - 0.45, 0.10, ClearH - 0.56, CourseMaterial.DemoInteriorWall);
        // Exterior decorative corner brick pilasters
        for (const SX of [-1, 1])
            EmitBoxAt(Out, CX + SX * (0.5 * DepthX - 0.22), CY + Side * (0.5 * OuterW + 0.09), 0.00, 0.46, 0.16, ClearH + 0.28, Palette.Trim);

        // Exact solid physics collider for each side wall!
        AddBoxCollider(Colliders, CX, WY, 0.0, DepthX, WallT, Floor2Z + Floor2H);
    }

    // ③ Overhead Garage Header Lintel Beams (front -X & rear +X) + Interior Exposed Timber Ceiling Rafters
    for (const SX of [-1, 1])
        EmitBoxAt(Out, CX + SX * (0.5 * DepthX - 0.30), CY, ClearH - 0.22, 0.58, ClearW - 0.06, 0.46, Palette.Trim);

    for (let R = -3; R <= 3; ++R)
    {
        const RX = CX + R * 1.25;
        EmitBoxAt(Out, RX, CY, ClearH - 0.14, 0.16, ClearW - 0.12, 0.18, CourseMaterial.DemoTimber);
    }
    // Warm glowing interior garage shop fluorescent light fixture hanging safely at Z = 3.22 m
    EmitBoxAt(Out, CX, CY, ClearH - 0.26, 4.6, 0.32, 0.10, CourseMaterial.LampWarm);

    // ④ Interior Garage Details tucked strictly flush along side wall (ClearW = 5.4 m -> tucked inside |y| = 2.32..2.66)
    const BenchSide = Rng() < 0.5 ? 1 : -1;
    const BenchY = CY + BenchSide * (0.5 * ClearW - 0.26);
    EmitBoxAt(Out, CX - 1.2, BenchY, 0.05, 2.4, 0.44, 0.88, CourseMaterial.DemoTimber);            // workbench
    EmitBoxAt(Out, CX + 1.8, BenchY, 0.05, 1.15, 0.42, 1.15, CourseMaterial.DemoBrickRed);       // red rolling tool chest
    EmitBoxAt(Out, CX - 1.2, BenchY, 1.65, 2.2, 0.28, 0.12, CourseMaterial.SteelDark);           // upper shelf

    // ⑤ 2nd-Story Structural Floor Deck & Living Floor Directly Above Garage (Z = 3.47 .. 7.08 m)
    EmitBoxAt(Out, CX, CY, ClearH + Gap, DepthX + 0.24, OuterW + 0.24, 0.24, CourseMaterial.DemoRubbleConcrete);
    EmitBoxAt(Out, CX, CY, Floor2Z, DepthX, OuterW, Floor2H, Palette.Wall);
    AddBoxCollider(Colliders, CX, CY, ClearH + 0.02, DepthX, OuterW, Floor2H + 2.4);

    // 2nd-Story Recessed Dark-Glass Windows + White Stone Sills & Lintels (Front, Rear & Both Sides!)
    for (const SX of [-1, 1])
    {
        const WX = CX + SX * (0.5 * DepthX + 0.06);
        for (const WYOff of [-1.75, 1.75])
        {
            EmitBoxAt(Out, WX, CY + WYOff, Floor2Z + 0.85, 0.10, 1.45, 1.55, CourseMaterial.VehicleGlass);
            EmitBoxAt(Out, WX + SX * 0.05, CY + WYOff, Floor2Z + 0.72, 0.22, 1.68, 0.12, Palette.Trim);
            EmitBoxAt(Out, WX + SX * 0.05, CY + WYOff, Floor2Z + 2.42, 0.20, 1.68, 0.16, Palette.Trim);
        }
    }
    for (const Side of [-1, 1])
    {
        const WY = CY + Side * (0.5 * OuterW + 0.06);
        for (const WXOff of [-2.4, 0.0, 2.4])
        {
            EmitBoxAt(Out, CX + WXOff, WY, Floor2Z + 0.90, 1.35, 0.10, 1.45, CourseMaterial.VehicleGlass);
            EmitBoxAt(Out, CX + WXOff, WY + Side * 0.05, Floor2Z + 0.76, 1.56, 0.20, 0.12, Palette.Trim);
        }
    }

    // ⑥ Pitched Gable Roof + Brick Chimney + Procedurally Fractured Roof/Wall Damage Chunk!
    EmitGableRoofAt(Out, CX, CY, RoofBase, DepthX + 0.75, OuterW + 0.85, 2.45, 0.0, Palette.Roof, Palette.Wall);
    const ChimSide = Rng() < 0.5 ? 1 : -1;
    EmitBoxAt(Out, CX + (Rng() - 0.5) * 3.0, CY + ChimSide * 1.9, RoofBase + 0.6, 0.95, 0.95, 2.65, CourseMaterial.DemoBrickRed);
    // Procedurally fractured jagged parapet/masonry rubble block perched on one corner (unique per seed!)
    EmitFracturedBlockAt(
        Out,
        CX + (Rng() < 0.5 ? -3.4 : 3.4),
        CY + (-ChimSide) * (0.5 * OuterW - 0.5),
        RoofBase + 0.65,
        2.1, 1.6, 1.7,
        (Rng() - 0.5) * 0.35,
        Palette.Wall, CourseMaterial.DemoRubbleConcrete, Rng, 3, true
    );

    // ⑦ Exterior Caution Bollards / Rubble Skirts flanking garage mouth (outside driving lane!)
    for (const Side of [-1, 1])
    {
        EmitBoxAt(Out, CX - 0.5 * DepthX - 0.45, CY + Side * (0.5 * ClearW + 0.45), 0.00, 0.42, 0.42, 0.95, CourseMaterial.DemoCraneYellow);
        EmitFracturedBlockAt(Out, CX + (Rng() - 0.5) * 4.0, CY + Side * (0.5 * OuterW + 0.95), 0.45, 1.6, 1.2, 0.95, Rng() * 3.0,
                             Palette.Wall, CourseMaterial.DemoRubbleConcrete, Rng, 3, true);
    }

    return {
        Kind: "garage",
        BlockIdx,
        Center: [CX, CY],
        EntryX: CX - 0.5 * DepthX + 0.22,
        ExitX:  CX + 0.5 * DepthX - 0.22,
        ClearW,
        ClearH
    };
}

// 📝 HOLLOW DRIVE-THROUGH CORNER STOREFRONT & PARLOR (2–3 Story Urban Rowhouse):
//    • Clear interior bay: WidthY = 5.8 m, DepthX = 9.2 m, ClearHeight = 3.60 m
//    • Thick corner brick piers, steel storefront beam, projecting storefront canopy/awning, warm glowing ceiling strips,
//      upper stories with recessed multi-pane windows & Juliet steel balconies, rooftop AC units + Voronoi-fractured top corner!
function EmitDriveThroughStorefront(Out, Colliders, CX, CY, Palette, Rng, BlockIdx)
{
    const DepthX   = 9.2;
    const ClearW   = 5.8;
    const WallT    = 0.85;
    const OuterW   = ClearW + 2.0 * WallT; // 7.50 m
    const ClearH   = 3.60;
    const Stories  = Rng() < 0.55 ? 2 : 3;
    const UpperH   = Stories * 3.15;
    const UpperZ   = ClearH + 0.30;

    // ① Polished concrete shop floor slab (Z = 0.00..0.04)
    EmitBoxAt(Out, CX, CY, 0.00, DepthX + 3.6, OuterW + 0.5, 0.04, CourseMaterial.Concrete);

    // ② Ground-floor Structural Side Piers + Interior Drywall + Tucked Display Shelves
    for (const Side of [-1, 1])
    {
        const WY = CY + Side * (0.5 * ClearW + 0.5 * WallT);
        EmitBoxAt(Out, CX, WY, 0.00, DepthX, WallT, ClearH, Palette.Wall);
        // Dark steel corner columns framing the glass storefront mouth
        for (const SX of [-1, 1])
            EmitBoxAt(Out, CX + SX * (0.5 * DepthX - 0.34), CY + Side * (0.5 * ClearW + 0.36), 0.00, 0.68, 0.72, ClearH, CourseMaterial.SteelDark);
        // Interior plaster + low shop display counter tucked tightly along side wall
        EmitBoxAt(Out, CX, CY + Side * (0.5 * ClearW + 0.06), 0.06, DepthX - 1.2, 0.10, ClearH - 0.25, CourseMaterial.DemoInteriorWall);
        EmitBoxAt(Out, CX, CY + Side * (0.5 * ClearW - 0.24), 0.05, 4.6, 0.40, 0.92, CourseMaterial.DemoTimber);

        AddBoxCollider(Colliders, CX, WY, 0.0, DepthX, WallT, UpperZ + UpperH);
    }

    // ③ Projecting Steel Storefront Awnings + Warm Illuminated Shop Interior Ceiling
    for (const SX of [-1, 1])
    {
        EmitBoxAt(Out, CX + SX * (0.5 * DepthX + 0.58), CY, ClearH - 0.15, 1.12, OuterW - 0.2, 0.24, CourseMaterial.SteelDark);
        EmitBoxAt(Out, CX + SX * (0.5 * DepthX + 0.42), CY, ClearH - 0.34, 0.68, ClearW - 0.2, 0.12, CourseMaterial.LampWarm);
    }
    EmitBoxAt(Out, CX - 1.8, CY, ClearH - 0.20, 2.4, 0.36, 0.10, CourseMaterial.LampWarm);
    EmitBoxAt(Out, CX + 1.8, CY, ClearH - 0.20, 2.4, 0.36, 0.10, CourseMaterial.LampWarm);

    // ④ Upper Stories (2nd & optional 3rd floor) Directly Overhead
    EmitBoxAt(Out, CX, CY, ClearH + Gap, DepthX + 0.28, OuterW + 0.28, 0.26, CourseMaterial.SteelDark);
    EmitBoxAt(Out, CX, CY, UpperZ, DepthX, OuterW, UpperH, Palette.Wall);
    AddBoxCollider(Colliders, CX, CY, ClearH + 0.02, DepthX, OuterW, UpperH + 1.2);

    // Recessed window bands, Juliet balconies & cornice molding per upper floor
    for (let Fl = 0; Fl < Stories; ++Fl)
    {
        const FZ = UpperZ + Fl * 3.15;
        for (const SX of [-1, 1])
        {
            const WX = CX + SX * (0.5 * DepthX + 0.06);
            for (const WYOff of [-2.1, 0.0, 2.1])
            {
                EmitBoxAt(Out, WX, CY + WYOff, FZ + 0.72, 0.10, 1.35, 1.68, CourseMaterial.VehicleGlass);
                EmitBoxAt(Out, WX + SX * 0.05, CY + WYOff, FZ + 2.42, 0.20, 1.56, 0.14, Palette.Trim);
            }
            // Projecting Juliet balcony deck + dark steel handrail on 2nd floor
            if (Fl === 0)
            {
                const BX = CX + SX * (0.5 * DepthX + 0.46);
                EmitBoxAt(Out, BX, CY, FZ + 0.08, 0.85, 4.4, 0.14, CourseMaterial.DemoRubbleConcrete);
                EmitBoxAt(Out, BX + SX * 0.36, CY, FZ + 0.92, 0.08, 4.3, 0.10, CourseMaterial.SteelDark);
                for (const PostY of [-2.0, -0.7, 0.7, 2.0])
                    EmitBoxAt(Out, BX + SX * 0.36, CY + PostY, FZ + 0.24, 0.08, 0.08, 0.68, CourseMaterial.SteelDark);
            }
        }
        for (const Side of [-1, 1])
        {
            const WY = CY + Side * (0.5 * OuterW + 0.06);
            for (const WXOff of [-2.5, 0.0, 2.5])
                EmitBoxAt(Out, CX + WXOff, WY, FZ + 0.82, 1.45, 0.10, 1.50, CourseMaterial.VehicleGlass);
        }
    }

    // ⑤ Crown Cornice + Rooftop Mechanical HVAC + Procedurally Fractured Broken Parapet Corner!
    const RoofZ = UpperZ + UpperH + Gap;
    EmitBoxAt(Out, CX, CY, RoofZ, DepthX + 0.55, OuterW + 0.55, 0.32, Palette.Trim);
    EmitBoxAt(Out, CX + 1.4, CY - 1.2, RoofZ + 0.34, 2.1, 1.8, 1.25, CourseMaterial.SteelDark);
    // Procedurally plane-cut fractured parapet blocks (unique jagged silhouette each seed!)
    for (let K = 0; K < 2; ++K)
    {
        EmitFracturedBlockAt(
            Out,
            CX + (K === 0 ? -3.1 : 2.9),
            CY + (K === 0 ? 2.4 : -2.3),
            RoofZ + 0.95,
            2.6, 1.5, 1.4,
            (Rng() - 0.5) * 0.25,
            Palette.Wall, CourseMaterial.DemoRubbleConcrete, Rng, 3, true
        );
    }

    return {
        Kind: "storefront",
        BlockIdx,
        Center: [CX, CY],
        EntryX: CX - 0.5 * DepthX + 0.20,
        MidX:   CX + (Rng() - 0.5) * 1.4,
        ExitX:  CX + 0.5 * DepthX - 0.20,
        ClearW,
        ClearH
    };
}

// 📝 DETAILED FLANKING RESIDENTIAL HOUSES & FRACTURED SKELETAL RUINS (sculpting the winding canyon pathway & crevices!)
function EmitDetailedFlankingHouse(Out, Colliders, CX, CY, Yaw, Archetype, Palette, Rng)
{
    const WidthX  = 7.2 + Rng() * 3.4;
    const DepthY  = 6.4 + Rng() * 2.8;
    const Stories = Archetype === 3 ? 2 : (Rng() < 0.45 ? 3 : 2);
    const HeightZ = Stories * 3.25 + 0.45;

    // Conservative axis-aligned collider footprint rotated by Yaw (hugs house without sealing neighboring crevices)
    const C = Math.abs(Math.cos(Yaw)), S = Math.abs(Math.sin(Yaw));
    const CollX = Math.max(4.6, (WidthX * C + DepthY * S) * 0.90);
    const CollY = Math.max(4.4, (WidthX * S + DepthY * C) * 0.90);
    AddBoxCollider(Colliders, CX, CY, 0.0, CollX, CollY, HeightZ);

    // ① Raised Stone Foundation Plinth
    EmitRotBoxAt(Out, CX, CY, 0.00, WidthX + 0.32, DepthY + 0.32, 0.42, Yaw, CourseMaterial.DemoRubbleConcrete);

    if (Archetype === 3)
    {
        // ARCHETYPE 3: ACTIVELY DEMOLISHED & PROCEDURALLY VORONOI-FRACTURED SKELETAL RUIN!
        // Ground story intact masonry + exposed timber floor joists + jagged SplitPart-cut upper walls + rubble mound!
        const LowerH = 3.25;
        EmitRotBoxAt(Out, CX, CY, 0.44, WidthX, DepthY, LowerH, Yaw, Palette.Wall);
        // Exposed floor deck timber joists jutting out of the sheared 2nd floor
        for (let J = -2; J <= 2; ++J)
        {
            const LocalY = J * (DepthY * 0.19);
            const CosY = Math.cos(Yaw), SinY = Math.sin(Yaw);
            EmitRotBoxAt(Out, CX - LocalY * SinY, CY + LocalY * CosY, 0.44 + LowerH + Gap, WidthX + 1.15, 0.22, 0.26, Yaw, CourseMaterial.DemoTimber);
        }
        // Two massive procedurally plane-cut fractured upper masonry wall chunks (unique every seed!)
        const CosY = Math.cos(Yaw), SinY = Math.sin(Yaw);
        for (const Wing of [-1, 1])
        {
            const Lx = Wing * (0.26 * WidthX);
            const Ly = (Rng() - 0.5) * (0.28 * DepthY);
            const WX = CX + Lx * CosY - Ly * SinY;
            const WY = CY + Lx * SinY + Ly * CosY;
            EmitFracturedBlockAt(
                Out, WX, WY, 0.44 + LowerH + 1.85,
                WidthX * 0.46, DepthY * 0.78, 3.45,
                Yaw + (Rng() - 0.5) * 0.12,
                Palette.Wall, CourseMaterial.DemoRubbleConcrete, Rng, 4, true
            );
        }
        // Crumbling fractured brick & concrete rubble skirt around base
        for (let R = 0; R < 3; ++R)
        {
            const Ang = Rng() * Math.PI * 2.0;
            const RX = CX + Math.cos(Ang) * (0.5 * WidthX + 0.65);
            const RY = CY + Math.sin(Ang) * (0.5 * DepthY + 0.65);
            EmitFracturedBlockAt(Out, RX, RY, 0.52, 1.7 + Rng(), 1.4 + Rng() * 0.8, 1.05 + Rng() * 0.5, Rng() * 3.1,
                                 Palette.Wall, CourseMaterial.DemoRubbleConcrete, Rng, 3, true);
        }
        // Steel tubular demolition scaffolding frame hugging one facade
        const ScaffY = (Rng() < 0.5 ? 1 : -1) * (0.5 * DepthY + 0.65);
        for (const PostX of [-0.38 * WidthX, 0.0, 0.38 * WidthX])
        {
            const PX = CX + PostX * CosY - ScaffY * SinY;
            const PY = CY + PostX * SinY + ScaffY * CosY;
            EmitRotBoxAt(Out, PX, PY, 0.00, 0.12, 0.12, LowerH + 2.8, Yaw, CourseMaterial.SteelDark);
        }
        const PlatX = CX - ScaffY * SinY, PlatY = CY + ScaffY * CosY;
        EmitRotBoxAt(Out, PlatX, PlatY, LowerH + 0.25, WidthX * 0.86, 0.85, 0.12, Yaw, CourseMaterial.DemoTimber);
        return;
    }

    // ARCHETYPES 0, 1, 2: RICH RESIDENTIAL GABLE HOUSE / RED-BRICK BROWNSTONE / TERRACOTTA DUPLEX
    EmitRotBoxAt(Out, CX, CY, 0.44, WidthX, DepthY, HeightZ - 0.44, Yaw, Palette.Wall);

    // Recessed multi-story windows + sills + lintels along both long faces
    const CosY = Math.cos(Yaw), SinY = Math.sin(Yaw);
    const WinCols = WidthX > 8.6 ? [-2.7, 0.0, 2.7] : [-1.85, 1.85];
    for (let Fl = 0; Fl < Stories; ++Fl)
    {
        const WZ = 1.25 + Fl * 3.15;
        for (const Side of [-1, 1])
        {
            const Ly = Side * (0.5 * DepthY + 0.06);
            for (const Lx of WinCols)
            {
                const WX = CX + Lx * CosY - Ly * SinY;
                const WY = CY + Lx * SinY + Ly * CosY;
                const WindowMat = (Fl === 0 && Rng() < 0.28) ? CourseMaterial.LampWarm : CourseMaterial.VehicleGlass;
                EmitRotBoxAt(Out, WX, WY, WZ, 1.28, 0.10, 1.48, Yaw, WindowMat);
                const SillLy = Side * (0.5 * DepthY + 0.11);
                EmitRotBoxAt(Out, CX + Lx * CosY - SillLy * SinY, CY + Lx * SinY + SillLy * CosY, WZ - 0.12, 1.48, 0.20, 0.10, Yaw, Palette.Trim);
                EmitRotBoxAt(Out, CX + Lx * CosY - SillLy * SinY, CY + Lx * SinY + SillLy * CosY, WZ + 1.50, 1.48, 0.18, 0.14, Yaw, Palette.Trim);
            }
        }
    }

    // Archetype 0: Front Covered Porch Stoop + Columns + Pitched Gable Roof + Chimney
    if (Archetype === 0)
    {
        const PorchSide = Rng() < 0.5 ? 1 : -1;
        const PorchLy = PorchSide * (0.5 * DepthY + 0.72);
        const PX = CX - PorchLy * SinY, PY = CY + PorchLy * CosY;
        EmitRotBoxAt(Out, PX, PY, 0.00, 3.8, 1.35, 0.44, Yaw, CourseMaterial.Concrete);
        EmitRotBoxAt(Out, PX, PY, 3.18, 4.1, 1.55, 0.22, Yaw, Palette.Roof);
        for (const ColX of [-1.65, 1.65])
        {
            const ColY = PorchSide * (0.5 * DepthY + 1.22);
            EmitRotBoxAt(Out, CX + ColX * CosY - ColY * SinY, CY + ColX * SinY + ColY * CosY, 0.46, 0.22, 0.22, 2.70, Yaw, Palette.Trim);
        }
        EmitGableRoofAt(Out, CX, CY, HeightZ + Gap, WidthX + 0.65, DepthY + 0.75, 2.45 + Rng() * 0.6, Yaw, Palette.Roof, Palette.Wall);
        const ChimX = (Rng() - 0.5) * (WidthX * 0.45);
        const ChimY = -PorchSide * (DepthY * 0.25);
        EmitRotBoxAt(Out, CX + ChimX * CosY - ChimY * SinY, CY + ChimX * SinY + ChimY * CosY, HeightZ + 0.5, 0.92, 0.92, 2.8, Yaw, CourseMaterial.DemoBrickRed);
    }
    // Archetype 1: Urban Brownstone Parapet + Cornice + Rooftop Water Tank / Fractured Corner
    else if (Archetype === 1)
    {
        EmitRotBoxAt(Out, CX, CY, HeightZ + Gap, WidthX + 0.48, DepthY + 0.48, 0.34, Yaw, Palette.Trim);
        EmitFracturedBlockAt(Out, CX + 0.24 * WidthX * CosY, CY + 0.24 * WidthX * SinY, HeightZ + 1.05,
                             WidthX * 0.45, DepthY * 0.82, 1.45, Yaw, Palette.Wall, CourseMaterial.DemoRubbleConcrete, Rng, 3, true);
        // Rooftop timber water tank on dark steel stilts
        const TankLx = -0.22 * WidthX, TankLy = 0.0;
        const TX = CX + TankLx * CosY - TankLy * SinY, TY = CY + TankLx * SinY + TankLy * CosY;
        EmitRotBoxAt(Out, TX, TY, HeightZ + 0.38, 1.85, 1.85, 0.95, Yaw, CourseMaterial.SteelDark);
        EmitRotBoxAt(Out, TX, TY, HeightZ + 1.35, 2.15, 2.15, 2.10, Yaw, CourseMaterial.DemoTimber);
        EmitGableRoofAt(Out, TX, TY, HeightZ + 3.47, 2.35, 2.35, 0.75, Yaw, CourseMaterial.DemoRoofSlate, CourseMaterial.DemoTimber);
    }
    // Archetype 2: Terracotta Duplex w/ 2nd-Floor Balcony + Clay Tile Gable Roof + Fractured Roof Bite
    else
    {
        const BalcSide = Rng() < 0.5 ? 1 : -1;
        const BalcLy = BalcSide * (0.5 * DepthY + 0.56);
        const BX = CX - BalcLy * SinY, BY = CY + BalcLy * CosY;
        EmitRotBoxAt(Out, BX, BY, 3.65, WidthX * 0.62, 1.05, 0.16, Yaw, CourseMaterial.DemoRubbleConcrete);
        const RailLy = BalcSide * (0.5 * DepthY + 0.98);
        EmitRotBoxAt(Out, CX - RailLy * SinY, CY + RailLy * CosY, 4.48, WidthX * 0.60, 0.08, 0.10, Yaw, CourseMaterial.SteelDark);
        EmitGableRoofAt(Out, CX, CY, HeightZ + Gap, WidthX + 0.55, DepthY + 0.65, 2.15, Yaw, CourseMaterial.DemoRoofTile, Palette.Wall);
        // Fractured masonry corner impact bite
        EmitFracturedBlockAt(Out, CX - 0.25 * WidthX * CosY, CY - 0.25 * WidthX * SinY, HeightZ + 1.15,
                             2.2, 2.0, 1.8, Yaw + 0.2, Palette.Wall, CourseMaterial.DemoRubbleConcrete, Rng, 3, true);
    }
}

// 📝 Demolition Clear Zone Extraction Finish Gantry at X = +216 m
function EmitDemolishFinishGantry(Out, Colliders, X, Y)
{
    const ClearHalf = 8.4;
    for (const Side of [-1, 1])
    {
        const PY = Y + Side * (ClearHalf + 0.75);
        EmitBoxAt(Out, X, PY, 0.00, 1.65, 1.45, 9.2, CourseMaterial.DemoCraneYellow);
        EmitBoxAt(Out, X, PY - Side * 0.78, 1.4, 0.45, 0.14, 7.2, CourseMaterial.DemoFinishGreen);
        AddBoxCollider(Colliders, X, PY, 0.0, 1.65, 1.45, 9.2);
    }
    EmitBoxAt(Out, X, Y, 8.20, 2.1, 2.0 * ClearHalf + 2.8, 1.35, CourseMaterial.SteelDark);
    EmitBoxAt(Out, X - 1.10, Y, 8.45, 0.18, 2.0 * ClearHalf - 1.0, 0.72, CourseMaterial.DemoFinishGreen);
    EmitBoxAt(Out, X + 1.10, Y, 8.45, 0.18, 2.0 * ClearHalf - 1.0, 0.72, CourseMaterial.DemoFinishGreen);
}

//------------------------------------------------------------------------------------------------------------------------
//                          DYNAMIC DEMOLITION MACHINERY & FRACTURED DEBRIS SHAPES
//------------------------------------------------------------------------------------------------------------------------

function EmitCraneBaseFacets()
{
    const Out = [];
    // Tracked caterpillar treads (left & right) + heavy concrete ballast undercarriage
    for (const SY of [-1.75, 1.75])
        EmitBoxAt(Out, 0.0, SY, 0.00, 6.2, 1.15, 1.25, CourseMaterial.SteelDark);
    EmitBoxAt(Out, 0.0, 0.0, 0.55, 5.2, 2.35, 0.95, CourseMaterial.DemoRubbleConcrete);
    EmitBoxAt(Out, 0.0, 0.0, 1.52, 3.8, 3.6, 0.65, CourseMaterial.DemoCraneYellow);
    return Out;
}

function EmitCraneTowerFacets()
{
    const Out = [];
    // Heavy slew-ring machinery house + glazed operator cab + yellow lattice mast + amber beacon!
    EmitBoxAt(Out, -1.2, 0.0, 0.00, 4.4, 3.1, 2.1, CourseMaterial.DemoCraneYellow);
    EmitBoxAt(Out, -3.5, 0.0, 0.15, 1.35, 3.2, 1.85, CourseMaterial.DemoRubbleConcrete); // counterweight
    // Glazed cab on front-right
    EmitBoxAt(Out, 1.35, 1.05, 0.15, 1.85, 1.45, 2.15, CourseMaterial.SteelDark);
    EmitBoxAt(Out, 2.30, 1.05, 0.65, 0.12, 1.15, 1.35, CourseMaterial.VehicleGlass);
    // Rotating Amber Warning Beacon on roof
    EmitBoxAt(Out, -0.6, 0.0, 2.14, 0.52, 0.52, 0.45, CourseMaterial.DemoBeaconAmber);
    // 4 Corner Lattice Mast Chords rising to Z = 15.5 m + cross braces
    const MastH = 15.2;
    for (const SX of [-0.72, 0.72])
        for (const SY of [-0.72, 0.72])
            EmitBoxAt(Out, SX, SY, 2.12, 0.24, 0.24, MastH, CourseMaterial.DemoCraneYellow);
    for (let Z = 3.2; Z < 16.5; Z += 2.2)
    {
        EmitBoxAt(Out, 0.0, -0.72, Z, 1.20, 0.16, 0.16, CourseMaterial.DemoCraneYellow);
        EmitBoxAt(Out, 0.0,  0.72, Z, 1.20, 0.16, 0.16, CourseMaterial.DemoCraneYellow);
        EmitBoxAt(Out, -0.72, 0.0, Z + 1.1, 0.16, 1.20, 0.16, CourseMaterial.DemoCraneYellow);
        EmitBoxAt(Out,  0.72, 0.0, Z + 1.1, 0.16, 1.20, 0.16, CourseMaterial.DemoCraneYellow);
    }
    return Out;
}

function EmitCraneJibFacets()
{
    const Out = [];
    // Long yellow steel lattice boom extending +X from 0 to +16.5 m + rear counter-jib to -6.5 m!
    const JibLen = 16.5;
    for (const SY of [-0.55, 0.55])
        EmitBoxAt(Out, 5.0, SY, -0.45, JibLen + 6.5, 0.20, 0.20, CourseMaterial.DemoCraneYellow);
    EmitBoxAt(Out, 5.0, 0.0, 0.75, JibLen + 6.2, 0.22, 0.22, CourseMaterial.DemoCraneYellow);
    // Cross lattice struts along boom
    for (let X = -5.5; X <= 16.0; X += 2.4)
    {
        EmitBoxAt(Out, X, 0.0, -0.45, 0.16, 0.90, 0.16, CourseMaterial.DemoCraneYellow);
        EmitBoxAt(Out, X, -0.28, 0.15, 0.16, 0.16, 1.00, CourseMaterial.DemoCraneYellow);
        EmitBoxAt(Out, X,  0.28, 0.15, 0.16, 0.16, 1.00, CourseMaterial.DemoCraneYellow);
    }
    // Concrete counterweight blocks on rear jib
    EmitBoxAt(Out, -5.6, 0.0, -0.95, 2.2, 1.65, 1.45, CourseMaterial.DemoRubbleConcrete);
    // Tip pulley sheave & warning amber light at X = +16.4 m
    EmitBoxAt(Out, 16.4, 0.0, -0.75, 0.65, 0.65, 0.65, CourseMaterial.SteelDark);
    EmitBoxAt(Out, 16.4, 0.0,  0.98, 0.42, 0.42, 0.42, CourseMaterial.DemoBeaconAmber);
    return Out;
}

function EmitCraneCableFacets()
{
    // 10.5 m tensioned dark steel hoist cable centered vertically along Z ∈ [-5.25 .. +5.25]
    const Out = [];
    EmitBoxAt(Out, 0.0, 0.0, -5.25, 0.13, 0.13, 10.5, CourseMaterial.SteelDark);
    return Out;
}

function EmitWreckingBallFacets()
{
    const Out = EmitWreckingSphereFacets(0.88, CourseMaterial.DemoIronBall);
    // Top forged steel lifting shackle + caution yellow equatorial stripe ring
    EmitBoxAt(Out, 0.0, 0.0, 0.84, 0.34, 0.34, 0.42, CourseMaterial.DemoCraneYellow);
    return Out;
}

function EmitChargePackFacets()
{
    const Out = [];
    // C4 Demolition Satchel block + blinking bright red LED detonator beacon
    EmitBoxAt(Out, 0.0, 0.0, -0.24, 0.68, 0.44, 0.48, CourseMaterial.SteelDark);
    EmitBoxAt(Out, 0.0, 0.0,  0.26, 0.44, 0.44, 0.34, CourseMaterial.DemoDetonatorRed);
    return Out;
}

function EmitCollapseWallFacets(Seed = 0x51A8)
{
    // Procedurally plane-cut jagged brick/concrete facade wall slab pivoted near Z = 0 so it tips dramatically!
    const Out = [];
    const Rng = ConstructRandom(Seed);
    EmitFracturedBlockAt(Out, 0.0, 0.0, 2.35, 4.4, 0.68, 4.7, 0.0, CourseMaterial.DemoBrickRed, CourseMaterial.DemoRubbleConcrete, Rng, 4, true);
    // Embedded dark steel I-beam lintel & broken window frame inside slab
    EmitBoxAt(Out, 0.0, 0.0, 0.05, 4.1, 0.78, 0.32, CourseMaterial.SteelDark);
    return Out;
}

function EmitFallingChunkFacets(Seed = 0xC401)
{
    // Jagged Voronoi-fractured masonry boulder chunk (brick exterior + dusty concrete fracture interior!)
    const Out = [];
    const Rng = ConstructRandom(Seed);
    EmitFracturedBlockAt(Out, 0.0, 0.0, 0.0, 1.05, 0.92, 0.82, 0.0, CourseMaterial.DemoBrickOchre, CourseMaterial.DemoRubbleConcrete, Rng, 4, false);
    return Out;
}

//------------------------------------------------------------------------------------------------------------------------
//                           SEEDED PROCEDURAL DEMOLITION CITY GENERATOR (RANDOM EVERY ENTRY!)
//------------------------------------------------------------------------------------------------------------------------

let ActiveDemolishCity = null;

export function GenerateRandomDemolishSeed()
{
    let Rand = (Math.floor(Math.random() * 0xFFFFFFFF) ^ (Date.now() >>> 0)) >>> 0;
    if (typeof crypto !== "undefined" && crypto.getRandomValues)
    {
        const Buf = new Uint32Array(1);
        crypto.getRandomValues(Buf);
        Rand = (Rand ^ Buf[0]) >>> 0;
    }
    return (Rand & 0x7FFFFFFF) || 0x4D30115;
}

// 📝 Smooth organic winding neighborhood street centerline Y(X), anchored smoothly to Y = 0 at Spawn (-132) & Finish (+216).
export function SampleDemolishPathY(City, X)
{
    const T = Math.max(0.0, Math.min(1.0, (X - (-132.0)) / (DemolishFinishX - (-132.0))));
    const Envelope = Math.sin(Math.PI * T); // 0 at start & finish, 1 through central city
    const Wave1 = City.PathAmp1 * Math.sin(T * Math.PI * City.PathFreq1 + City.PathPhase1);
    const Wave2 = City.PathAmp2 * Math.sin(T * Math.PI * City.PathFreq2 + City.PathPhase2);
    return Envelope * (Wave1 + Wave2);
}

// 📝 Generates a complete, guaranteed-navigable, richly detailed, uniquely fractured Demolition City from Seed!
export function BuildDemolishCity(SeedInput = null)
{
    const Seed = (SeedInput !== null && SeedInput !== undefined && Number.isFinite(Number(SeedInput)))
        ? ((Number(SeedInput) >>> 0) || 1)
        : GenerateRandomDemolishSeed();

    const Rng = ConstructRandom(Seed);

    const City = {
        Seed,
        SeedTag:    Seed.toString(16).toUpperCase().padStart(8, "0").slice(-6),
        PathAmp1:   (Rng() < 0.5 ? 1 : -1) * (4.8 + Rng() * 4.4),
        PathAmp2:   (Rng() < 0.5 ? 1 : -1) * (2.2 + Rng() * 2.8),
        PathFreq1:  2.0 + Math.floor(Rng() * 2),
        PathFreq2:  4.0 + Math.floor(Rng() * 2),
        PathPhase1: (Rng() - 0.5) * 1.1,
        PathPhase2: Rng() * Math.PI * 2.0,
        Blocks:         [],
        HeroBuildings:  [],
        Cranes:         [],
        Charges:        [],
        CollapseWalls:  [],
        Breakables:     [],
        ExtraObstacles: [],
        BlockFacets:    Array.from({ length: BlockCentersX.length }, () => []),
        PropsFacets:    []
    };

    const PathY = (X) => SampleDemolishPathY(City, X);
    const PathSlopeYaw = (X) =>
    {
        const DY = PathY(X + 4.0) - PathY(X - 4.0);
        return Math.atan2(DY, 8.0);
    };

    // ① Spawn Entry Flanking Houses framing the opening neighborhood canyon sightline (X = -122 .. -108)
    for (const Side of [-1, 1])
    {
        const Pal1 = WallPalettes[Math.floor(Rng() * WallPalettes.length)];
        const HX1 = -121.0 + (Rng() - 0.5) * 3.0;
        const HY1 = PathY(HX1) + Side * (10.4 + Rng() * 1.4);
        EmitDetailedFlankingHouse(City.BlockFacets[0], City.ExtraObstacles, HX1, HY1, Side * 0.06, Math.floor(Rng() * 3), Pal1, Rng);

        const Pal2 = WallPalettes[Math.floor(Rng() * WallPalettes.length)];
        const HX2 = -108.5 + (Rng() - 0.5) * 2.5;
        const HY2 = PathY(HX2) + Side * (10.8 + Rng() * 1.4);
        EmitDetailedFlankingHouse(City.BlockFacets[0], City.ExtraObstacles, HX2, HY2, Side * -0.05, (Side > 0 ? 3 : Math.floor(Rng() * 3)), Pal2, Rng);
    }

    // ② Build all 6 Procedural Neighborhood Demolition Blocks!
    const GarageCountTarget = 3;
    let BuiltGarages = 0;

    for (let B = 0; B < BlockCentersX.length; ++B)
    {
        const CX = BlockCentersX[B];
        const CY = PathY(CX);
        const OutFacets = City.BlockFacets[B];
        const HeroSide = (B === 0)
            ? (Rng() < 0.5 ? 1 : -1)
            : ((Rng() < 0.65 ? -1 : 1) * (City.Blocks[B - 1]?.HeroSide || 1));

        // Ensure EVERY run has BOTH Drive-Through Garages AND Drive-Through Storefronts!
        const RemainBlocks = BlockCentersX.length - B;
        const MustGarage = (GarageCountTarget - BuiltGarages) >= RemainBlocks;
        const IsGarage = MustGarage ? true : (B === 0 ? true : Rng() < 0.54);
        if (IsGarage)
            BuiltGarages++;

        const HeroPalette = WallPalettes[Math.floor(Rng() * WallPalettes.length)];
        const HeroY = CY + HeroSide * (3.9 + Rng() * 0.7);

        // A) Hollow Drive-Through Hero Building (Garage House OR Corner Storefront!)
        const Hero = IsGarage
            ? EmitDriveThroughGarageHouse(OutFacets, City.ExtraObstacles, CX, HeroY, HeroPalette, Rng, B)
            : EmitDriveThroughStorefront(OutFacets, City.ExtraObstacles, CX, HeroY, HeroPalette, Rng, B);
        City.HeroBuildings.push(Hero);

        // Register REAL Scene.Breakables inside the Drive-Through Building!
        const DoorSeedBase = (Seed ^ ((B + 1) * 0x9E3779B1)) >>> 0;
        if (IsGarage)
        {
            // Front Sectional Garage Door (breakable into flying planks & panels!)
            City.Breakables.push({
                Name:         `Block ${B + 1} Garage Door`,
                Material:     "garage",
                Shape:        { Kind: "box", Extent: [0.16, Hero.ClearW - 0.36, 2.85] },
                Position:     [Hero.EntryX, HeroY, 0.05],
                Yaw:          0.0,
                Approach:     [-1.0, 0.0, 0.0],
                Capacity:     16,
                Ceiling:      14,
                DensityScale: 0.14,
                Threshold:    1100,
                Seed:         (DoorSeedBase + 11) >>> 0,
                DemolishTag:  "garage_door",
                BlockIdx:     B
            });
            // Rear Clay-Brick / Timber Exit Wall (smash straight out the back of the garage!)
            City.Breakables.push({
                Name:         `Block ${B + 1} Garage Rear Wall`,
                Material:     Rng() < 0.65 ? "brick" : "wood",
                Shape:        { Kind: "box", Extent: [0.22, Hero.ClearW - 0.36, 2.95] },
                Position:     [Hero.ExitX, HeroY, 0.05],
                Yaw:          0.0,
                Approach:     [-1.0, 0.0, 0.0],
                Capacity:     16,
                Ceiling:      14,
                DensityScale: 0.055,
                Threshold:    1800,
                Seed:         (DoorSeedBase + 23) >>> 0,
                DemolishTag:  "brick_wall",
                BlockIdx:     B
            });
        }
        else
        {
            // Front Tempered Glass Storefront Pane
            City.Breakables.push({
                Name:         `Block ${B + 1} Storefront Glass`,
                Material:     "tempered",
                Shape:        { Kind: "box", Extent: [0.06, Hero.ClearW - 0.36, 3.05] },
                Position:     [Hero.EntryX, HeroY, 0.05],
                Yaw:          0.0,
                Approach:     [-1.0, 0.0, 0.0],
                Capacity:     16,
                Ceiling:      14,
                DensityScale: 0.10,
                Threshold:    550,
                Seed:         (DoorSeedBase + 31) >>> 0,
                DemolishTag:  "storefront_glass",
                BlockIdx:     B
            });
            // Interior Drywall Partition inside the parlor
            City.Breakables.push({
                Name:         `Block ${B + 1} Interior Partition`,
                Material:     "drywall",
                Shape:        { Kind: "box", Extent: [0.16, Hero.ClearW - 0.44, 3.00] },
                Position:     [Hero.MidX, HeroY, 0.05],
                Yaw:          0.0,
                Approach:     [-1.0, 0.0, 0.0],
                Capacity:     14,
                Ceiling:      12,
                DensityScale: 0.09,
                Threshold:    1200,
                Seed:         (DoorSeedBase + 41) >>> 0,
                DemolishTag:  "interior_wall",
                BlockIdx:     B
            });
            // Rear Storefront Brick/Glass Exit Wall
            City.Breakables.push({
                Name:         `Block ${B + 1} Parlor Exit Wall`,
                Material:     Rng() < 0.60 ? "brick" : "glass",
                Shape:        { Kind: "box", Extent: [0.20, Hero.ClearW - 0.36, 2.95] },
                Position:     [Hero.ExitX, HeroY, 0.05],
                Yaw:          0.0,
                Approach:     [-1.0, 0.0, 0.0],
                Capacity:     16,
                Ceiling:      14,
                DensityScale: 0.06,
                Threshold:    1600,
                Seed:         (DoorSeedBase + 53) >>> 0,
                DemolishTag:  "brick_wall",
                BlockIdx:     B
            });
        }

        // B) Parallel Crevice / Alley Route on the Opposite Side (sculpted by flanking houses!)
        const CreviceSide = -HeroSide;
        const CreviceY = CY + CreviceSide * (4.1 + Rng() * 0.7);
        // Optional breakable wooden demolition fence / cracked brick shortcut barricade across the crevice (always >= 2 per run!)
        if (Rng() < 0.58 || B === 1 || B === 4)
        {
            const FenceMat = Rng() < 0.5 ? "wood" : "brick";
            City.Breakables.push({
                Name:         `Block ${B + 1} Alley Barricade`,
                Material:     FenceMat,
                Shape:        { Kind: "box", Extent: [0.20, 4.4, 2.45] },
                Position:     [CX + (Rng() - 0.5) * 3.0, CreviceY, 0.05],
                Yaw:          (Rng() - 0.5) * 0.16,
                Approach:     [-1.0, 0.0, 0.0],
                Capacity:     14,
                Ceiling:      12,
                DensityScale: FenceMat === "brick" ? 0.055 : 0.16,
                Threshold:    1400,
                Seed:         (DoorSeedBase + 67) >>> 0,
                DemolishTag:  "alley_wall",
                BlockIdx:     B
            });
        }

        // C) Flanking Residential Houses & Skeletal Fractured Ruins framing BOTH sides of the winding canyon!
        //    1) House bounding the far side of the crevice (creates the sunlit 5.0-5.8 m crevice gap!)
        const CreviceWallY = CreviceY + CreviceSide * (6.8 + Rng() * 1.1);
        const PalCrev = WallPalettes[Math.floor(Rng() * WallPalettes.length)];
        EmitDetailedFlankingHouse(OutFacets, City.ExtraObstacles, CX + (Rng() - 0.5) * 3.5, CreviceWallY,
                                  PathSlopeYaw(CX) + (Rng() - 0.5) * 0.18, (B % 2 === 0) ? 3 : Math.floor(Rng() * 3), PalCrev, Rng);

        //    2) House behind/beside the Hero Drive-Through building forming the neighborhood block mass
        const OuterHeroY = HeroY + HeroSide * (9.2 + Rng() * 1.6);
        const PalOuter = WallPalettes[Math.floor(Rng() * WallPalettes.length)];
        EmitDetailedFlankingHouse(OutFacets, City.ExtraObstacles, CX + (Rng() - 0.5) * 5.0, OuterHeroY,
                                  (Rng() - 0.5) * 0.14, Math.floor(Rng() * 4), PalOuter, Rng);

        //    3) Transition corridor houses between Block B and Block B+1 sculpting the natural S-curve street!
        const MidX = CX + 25.5;
        const MidPathY = PathY(MidX);
        const MidYaw = PathSlopeYaw(MidX);
        for (const Side of [-1, 1])
        {
            const GapHalf = 8.4 + Rng() * 2.2; // leaves >= 6.8 m open curving avenue + tight crevices!
            const HY = MidPathY + Side * GapHalf;
            const PalMid = WallPalettes[Math.floor(Rng() * WallPalettes.length)];
            const ArchMid = (Side === HeroSide && Rng() < 0.45) ? 3 : Math.floor(Rng() * 3);
            EmitDetailedFlankingHouse(OutFacets, City.ExtraObstacles, MidX + (Rng() - 0.5) * 3.2, HY,
                                      MidYaw + (Rng() - 0.5) * 0.16, ArchMid, PalMid, Rng);
        }

        // D) Giant Active Demolition Crane (blocks 0..4 => 5 Cranes!)
        if (B < DemolishPoolCounts.CraneBases)
        {
            const CraneSide = (B % 2 === 0) ? CreviceSide : HeroSide;
            const CraneX = CX + 13.5 + (Rng() - 0.5) * 4.0;
            const CraneY = PathY(CraneX) + CraneSide * (14.6 + Rng() * 1.8);
            const TargetX = CraneX - 1.5;
            const TargetY = PathY(CraneX) + CraneSide * 7.8;
            // Solid collider for crane tracked base outside the street
            AddBoxCollider(City.ExtraObstacles, CraneX, CraneY, 0.0, 5.8, 4.6, 3.2);
            City.Cranes.push({
                Index:       B,
                BlockIdx:    B,
                BasePos:     [CraneX, CraneY, 0.0],
                TargetPos:   [TargetX, TargetY, 7.2 + Rng() * 1.8],
                BaseYaw:     Math.atan2(TargetY - CraneY, TargetX - CraneX),
                SwingPhase:  Rng() * Math.PI * 2.0,
                SwingRate:   0.85 + Rng() * 0.35,
                CableLength: 9.8 + Rng() * 1.4
            });
        }

        // E) 2 Controlled Demolition C4 Charges + 2 Pre-Fractured Collapsing Facade Walls per block (12 total!)
        for (let Sub = 0; Sub < 2; ++Sub)
        {
            const ChargeIdx = B * 2 + Sub;
            const ChargeSide = (Sub === 0) ? CreviceSide : -CreviceSide;
            const ChargeX = CX + (Sub === 0 ? -9.5 : 15.5) + (Rng() - 0.5) * 3.0;
            const LocalY = PathY(ChargeX);
            const WallY = LocalY + ChargeSide * (6.2 + Rng() * 1.2);
            const FallDirY = -ChargeSide; // tips dramatically inward toward street edge!
            City.Charges.push({
                Index:        ChargeIdx,
                BlockIdx:     B,
                Position:     [ChargeX, WallY - ChargeSide * 0.55, 2.25 + Rng() * 1.4],
                TriggerDist:  54.0 + Rng() * 12.0,
                DelaySeconds: 0.45 + Sub * 0.65 + Rng() * 0.45
            });
            City.CollapseWalls.push({
                Index:        ChargeIdx,
                BlockIdx:     B,
                AnchorPos:    [ChargeX, WallY, 0.05],
                Yaw:          (ChargeSide > 0 ? 0.0 : Math.PI) + (Rng() - 0.5) * 0.14,
                FallSign:     FallDirY,
                ChargeIndex:  ChargeIdx
            });
        }

        // F) Street-side Micro-Detail: Demolition Dumpsters tucked strictly on outer sidewalk edge (never blocking driveways!)
        for (const Side of [-1, 1])
        {
            const DX = CX - 15.5 + Rng() * 4.0;
            let DY = PathY(DX) + Side * (9.2 + Rng() * 1.2);
            if (Side === HeroSide && Math.abs(DY - HeroY) < 5.0)
                DY = HeroY + Side * 5.6;
            // Yellow/dark industrial demolition dumpster outside driving & driveway lanes
            EmitRotBoxAt(City.PropsFacets, DX, DY, 0.00, 3.2, 1.55, 1.35, PathSlopeYaw(DX), CourseMaterial.SteelDark);
            EmitRotBoxAt(City.PropsFacets, DX, DY, 1.22, 3.3, 1.62, 0.18, PathSlopeYaw(DX), CourseMaterial.DemoCraneYellow);
            AddBoxCollider(City.ExtraObstacles, DX, DY, 0.0, 3.0, 1.5, 1.35);
        }

        City.Blocks.push({
            Index:     B,
            CenterX:   CX,
            CenterY:   CY,
            HeroSide,
            HeroKind:  Hero.Kind,
            HeroY,
            CreviceY
        });
    }

    // ③ Extraction Finish Gantry & Clear Zone Plaza at X = +216 m
    EmitDemolishFinishGantry(City.PropsFacets, City.ExtraObstacles, DemolishFinishX, 0.0);

    // ④ Generate Seed-Unique Fractured Dynamic Shapes for Collapsing Walls & Tumbling Rubble Chunks
    City.CollapseWallSeed  = (Seed ^ 0xA51F3219) >>> 0;
    City.FallingChunkSeed  = (Seed ^ 0x7C9E1043) >>> 0;

    return City;
}

export function PrepareDemolishCity(Seed = null)
{
    ActiveDemolishCity = BuildDemolishCity(Seed);
    return ActiveDemolishCity;
}

export function GetActiveDemolishCity()
{
    if (!ActiveDemolishCity)
        ActiveDemolishCity = BuildDemolishCity(null);
    return ActiveDemolishCity;
}

//------------------------------------------------------------------------------------------------------------------------
//                           LEVEL DESCRIPTOR ACCESSORS (CONSUMED BY LEVELSTRUCTURE & FACETSTRUCTURE)
//------------------------------------------------------------------------------------------------------------------------

export const DemolishSpawn = Object.freeze({
    Position: [-132.0, 0.0, 0.0],
    Yaw:      0.0
});

export function ResolveDemolishZones()
{
    const City = GetActiveDemolishCity();
    const Zones = [
        { Name: "Demolition Start", Position: [-132.0, 0.0], Yaw: 0.0, Note: `Seed #${City.SeedTag} entry` }
    ];
    for (const B of City.Blocks)
    {
        const Label = B.HeroKind === "garage" ? "Drive-Thru Garage" : "Glass Storefront";
        Zones.push({
            Name:     `Block ${B.Index + 1} · ${Label}`,
            Position: [B.CenterX - 15.0, B.CenterY],
            Yaw:      0.0,
            Note:     `Crevice + ${Label}`
        });
    }
    Zones.push({ Name: "Extraction Arch", Position: [DemolishFinishX - 18.0, 0.0], Yaw: 0.0, Note: "Demolition clear zone" });
    return Zones;
}

export function ResolveDemolishShapes()
{
    const City = GetActiveDemolishCity();
    const Shapes = {};
    for (let B = 0; B < City.BlockFacets.length; ++B)
    {
        const Facets = City.BlockFacets[B];
        Shapes[`DemolishBlock${B}`] = {
            Emit:  () => Facets,
            Voxel: 0.42,
            Cards: 4
        };
    }
    Shapes.DemolishStreetProps = {
        Emit:  () => City.PropsFacets,
        Voxel: 0.36,
        Cards: 4
    };

    // Dynamic animated demolition shapes (Distance: false => instant zero-lag GPU transform updates!)
    Shapes.DemoCraneBase    = { Emit: () => EmitCraneBaseFacets(),                         Voxel: 0.28, Cards: 5, Distance: false };
    Shapes.DemoCraneTower   = { Emit: () => EmitCraneTowerFacets(),                        Voxel: 0.28, Cards: 5, Distance: false };
    Shapes.DemoCraneJib     = { Emit: () => EmitCraneJibFacets(),                          Voxel: 0.30, Cards: 5, Distance: false };
    Shapes.DemoCraneCable   = { Emit: () => EmitCraneCableFacets(),                        Voxel: 0.16, Cards: 4, Distance: false };
    Shapes.DemoWreckingBall = { Emit: () => EmitWreckingBallFacets(),                      Voxel: 0.18, Cards: 6, Distance: false };
    Shapes.DemoChargePack   = { Emit: () => EmitChargePackFacets(),                        Voxel: 0.14, Cards: 4, Distance: false };
    Shapes.DemoCollapseWall = { Emit: () => EmitCollapseWallFacets(City.CollapseWallSeed), Voxel: 0.26, Cards: 5, Distance: false };
    Shapes.DemoFallingChunk = { Emit: () => EmitFallingChunkFacets(City.FallingChunkSeed), Voxel: 0.16, Cards: 5, Distance: false };
    return Shapes;
}

export function ResolveDemolishPlacements()
{
    const City = GetActiveDemolishCity();
    const List = [];
    // Static architectural blocks use Collider: false because exact wall/pier AABBs live in ExtraObstacles
    // so EVERY hollow garage bay, storefront parlor & sunlit crevice stays 100% open & drivable!
    for (let B = 0; B < City.BlockFacets.length; ++B)
    {
        List.push({
            Shape:    `DemolishBlock${B}`,
            Name:     `Demolition District Block ${B + 1}`,
            Position: [0.0, 0.0, 0.0],
            Yaw:      0.0,
            Material: -1,
            Collider: false
        });
    }
    List.push({
        Shape:    "DemolishStreetProps",
        Name:     "Demolition Street Props & Finish Arch",
        Position: [0.0, 0.0, 0.0],
        Yaw:      0.0,
        Material: -1,
        Collider: false
    });
    return List;
}

export function ResolveDemolishExtraObstacles()
{
    return GetActiveDemolishCity().ExtraObstacles;
}

export function ResolveDemolishBreakables()
{
    return GetActiveDemolishCity().Breakables;
}

export function ResolveDemolishCones()
{
    const City = GetActiveDemolishCity();
    const Cones = [];
    // Warning cones lining the Spawn start corridor & driveway mouths
    for (let X = -126.0; X <= -106.0; X += 5.0)
    {
        const Y = SampleDemolishPathY(City, X);
        Cones.push({ Position: [X, Y - 5.2, 0.0], Yaw: 0.0 });
        Cones.push({ Position: [X, Y + 5.2, 0.0], Yaw: 0.0 });
    }
    return Cones;
}

// 📝 Grounded urban demolition avenue: wide dusty cracked asphalt bed + winding concrete residential sidewalks + clear zone pad!
export function EmitDemolishGroundFacets()
{
    const City = GetActiveDemolishCity();
    const Facets = [];
    // Outer urban concrete/earth surround at Z = 0.0
    EmitQuad(Facets, [-340.0, -240.0, 0.0], [420.0, -240.0, 0.0], [420.0, 240.0, 0.0], [-340.0, 240.0, 0.0], CourseMaterial.DemoRubbleConcrete);
    // Main demolition corridor asphalt ribbon following the procedural winding neighborhood path!
    const Step = 12.0;
    for (let X0 = -152.0; X0 < 236.0; X0 += Step)
    {
        const X1 = X0 + Step;
        const Y0 = SampleDemolishPathY(City, X0);
        const Y1 = SampleDemolishPathY(City, X1);
        // Sidewalk concrete shoulder strips (Z = 0.004)
        EmitQuad(Facets, [X0, Y0 - 15.5, 0.004], [X1, Y1 - 15.5, 0.004], [X1, Y1 + 15.5, 0.004], [X0, Y0 + 15.5, 0.004], CourseMaterial.Concrete);
        // Dark cracked asphalt demolition roadway (Z = 0.008)
        EmitQuad(Facets, [X0, Y0 - 8.5, 0.008], [X1, Y1 - 8.5, 0.008], [X1, Y1 + 8.5, 0.008], [X0, Y0 + 8.5, 0.008], CourseMaterial.Asphalt);
        // Dashed yellow road centerline markers (Z = 0.012)
        if (((Math.round((X0 + 152.0) / Step)) % 2) === 0)
            EmitQuad(Facets, [X0 + 2.0, Y0 - 0.14, 0.012], [X0 + 8.0, 0.5 * (Y0 + Y1) - 0.14, 0.012], [X0 + 8.0, 0.5 * (Y0 + Y1) + 0.14, 0.012], [X0 + 2.0, Y0 + 0.14, 0.012], CourseMaterial.PaintYellow);
    }
    // Emerald extraction finish stripe on asphalt at DemolishFinishX
    EmitQuad(Facets, [DemolishFinishX - 1.2, -9.0, 0.014], [DemolishFinishX + 1.2, -9.0, 0.014], [DemolishFinishX + 1.2, 9.0, 0.014], [DemolishFinishX - 1.2, 9.0, 0.014], CourseMaterial.DemoFinishGreen);
    return Facets;
}
