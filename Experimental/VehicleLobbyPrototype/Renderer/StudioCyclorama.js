//====================================================================================================================================
//                                                       STUDIOCYCLORAMA.JS
//====================================================================================================================================
// 🧩 Car studio set for the VehicleHMI renderer: a round cyclorama (floor, cove and wall) and one overhead softbox, as facets

// 📝 Active only when the page address carries ?scene=studio. The proving ground, its course lamps, cones and breakables are then
//    not built, so the studio light comes from the softbox alone.
import { CourseMaterial } from "./MaterialSpecification.js";

export const StudioSceneEnabled = new URLSearchParams(location.search).get("scene") === "studio";

// 📝 The centre matches CourseStructure SpawnPoint, so the car stands in the middle of the cyclorama without moving the spawn.
export const StudioCentre = Object.freeze([-40.0, 0.0]);

export const StudioCycloramaLayout = Object.freeze({
    FloorRadius: 10.0,     // [m] - Flat floor disc before the cove begins
    CoveRadius:   8.0,     // [m] - Quarter-circle radius blending the floor into the wall
    WallHeight:  20.0,     // [m] - Wall top above the floor
    Segments:    64,       // [idx] - Facet columns around the axis
    CoveSteps:    8,       // [idx] - Arc steps through the cove
    FloorFamily: CourseMaterial.ApronConcrete,
    WallFamily:  CourseMaterial.WallChalk
});

export const StudioSoftboxLayout = Object.freeze({
    Height:     7.0,                       // [m] - Panel height above the floor
    HalfX:      3.0,                       // [m] - Half width
    HalfY:      2.0,                       // [m] - Half depth
    Family:     CourseMaterial.Headlight   // Neutral white emission (48.7 linear), facing down onto the car
});

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Each facet is wound so that its geometric normal (B - A) x (C - A) points toward the requested side, whatever the input
//    order. The renderer classifies front faces by that normal.
function EmitOrientedTriangle(Facets, A, B, C, Family, TargetNormal)
{
    const Edge1 = [B[0] - A[0], B[1] - A[1], B[2] - A[2]];
    const Edge2 = [C[0] - A[0], C[1] - A[1], C[2] - A[2]];
    const Normal = [Edge1[1] * Edge2[2] - Edge1[2] * Edge2[1],
                    Edge1[2] * Edge2[0] - Edge1[0] * Edge2[2],
                    Edge1[0] * Edge2[1] - Edge1[1] * Edge2[0]];
    const Facing = Normal[0] * TargetNormal[0] + Normal[1] * TargetNormal[1] + Normal[2] * TargetNormal[2];
    if (Facing >= 0.0)
        Facets.push({ Corners: [A, B, C], Family });
    else
        Facets.push({ Corners: [A, C, B], Family });
}

function EmitOrientedQuad(Facets, A, B, C, D, Family, TargetNormal)
{
    EmitOrientedTriangle(Facets, A, B, C, Family, TargetNormal);
    EmitOrientedTriangle(Facets, A, C, D, Family, TargetNormal);
}

// 📝 Profile point (radius, height) on the ring at angle Angle, centred on StudioCentre.
function ResolveRingPoint(Centre, Angle, Radius, Height)
{
    return [Centre[0] + Radius * Math.cos(Angle), Centre[1] + Radius * Math.sin(Angle), Height];
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Profile from the axis outward: flat floor, quarter-circle cove, vertical wall. Each profile segment's normal points
//    inward and up, so the set reads as a lit studio cove from the car's side.
export function EmitStudioCycloramaFacets(Centre = StudioCentre)
{
    const Layout = StudioCycloramaLayout;
    const Facets = [];
    const Profile = [[Layout.FloorRadius, 0.0]];
    for (let Step = 1; Step <= Layout.CoveSteps; ++Step)
    {
        const Angle = 0.5 * Math.PI * Step / Layout.CoveSteps;
        Profile.push([Layout.FloorRadius + Layout.CoveRadius * Math.sin(Angle), Layout.CoveRadius * (1.0 - Math.cos(Angle))]);
    }
    Profile.push([Layout.FloorRadius + Layout.CoveRadius, Layout.WallHeight]);

    const Up = [0.0, 0.0, 1.0];
    const Centre0 = [Centre[0], Centre[1], 0.0];
    for (let Column = 0; Column < Layout.Segments; ++Column)
    {
        const Angle0 = 2.0 * Math.PI * Column / Layout.Segments;
        const Angle1 = 2.0 * Math.PI * (Column + 1) / Layout.Segments;
        const Ring0 = ResolveRingPoint(Centre, Angle0, Layout.FloorRadius, 0.0);
        const Ring1 = ResolveRingPoint(Centre, Angle1, Layout.FloorRadius, 0.0);
        EmitOrientedTriangle(Facets, Centre0, Ring0, Ring1, Layout.FloorFamily, Up);

        for (let Segment = 0; Segment + 1 < Profile.length; ++Segment)
        {
            const [Radius0, Height0] = Profile[Segment];
            const [Radius1, Height1] = Profile[Segment + 1];
            const DeltaRadius = Radius1 - Radius0;
            const DeltaHeight = Height1 - Height0;
            const Length = Math.hypot(DeltaRadius, DeltaHeight);
            // 📝 Inward-and-up normal in the (radius, height) plane: rotate the profile tangent by +90 degrees.
            const NormalRadius = -DeltaHeight / Length;
            const NormalHeight = DeltaRadius / Length;
            const MiddleAngle = 0.5 * (Angle0 + Angle1);
            const Target = [NormalRadius * Math.cos(MiddleAngle), NormalRadius * Math.sin(MiddleAngle), NormalHeight];
            // 📝 The cove and the wall share one chalk family; only the floor disc uses the concrete family.
            const Family = Layout.WallFamily;
            EmitOrientedQuad(Facets,
                             ResolveRingPoint(Centre, Angle0, Radius0, Height0), ResolveRingPoint(Centre, Angle1, Radius0, Height0),
                             ResolveRingPoint(Centre, Angle1, Radius1, Height1), ResolveRingPoint(Centre, Angle0, Radius1, Height1),
                             Family, Target);
        }
    }
    return Facets;
}

// 📝 One rectangular panel, emitting downward. It is the only light source of the studio.
export function EmitStudioSoftboxFacets(Centre = StudioCentre)
{
    const Layout = StudioSoftboxLayout;
    const Facets = [];
    const Height = Layout.Height;
    const Down = [0.0, 0.0, -1.0];
    EmitOrientedQuad(Facets,
                     [Centre[0] - Layout.HalfX, Centre[1] - Layout.HalfY, Height], [Centre[0] + Layout.HalfX, Centre[1] - Layout.HalfY, Height],
                     [Centre[0] + Layout.HalfX, Centre[1] + Layout.HalfY, Height], [Centre[0] - Layout.HalfX, Centre[1] + Layout.HalfY, Height],
                     Layout.Family, Down);
    return Facets;
}
