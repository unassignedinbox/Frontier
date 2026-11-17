//============================================================================================================================================
//                                                            LOBBYSTRUCTURE.JS
//============================================================================================================================================
// 🧩 Multi-venue Frontier Showroom Lobby architecture — five pure single-hero architectural locations:
//      1. studio    — Seamless 360° white photography cyclorama & suspended luminous softbox
//      2. monolith  — Sculptural tilted white architectural monoliths on an unbroken ice-mirror plaza
//      3. saltflats — Infinite sunlit Bonneville salt flats ringed by razor-low distant mesas
//      4. oculus    — Warm travertine concrete pavilion with circular open sky oculus, dark obsidian reflecting
//                     pool beneath the hero car, balustrade terrace, and sunlit terracotta red-rock canyon vista
//      5. nocturne  — Moody twilight dark wet-satin plaza illuminated by a floating solar-yellow architectural
//                     lightwall against dark distant mountain silhouettes

import { CourseMaterial } from "./MaterialSpecification.js";
import { EmitQuad, EmitBoxFacets } from "./CourseStructure.js";
import { LightType } from "./LightingSystem.js";

//------------------------------------------------------------------------------------------------------------------------
//                                                 LOBBY VENUE CATALOG
//------------------------------------------------------------------------------------------------------------------------

export const LobbyVenues = Object.freeze([
    Object.freeze({
        Key: "studio",
        Title: "Studio",
        FullTitle: "Photography Studio",
        Subtitle: "360° white cyclorama · overhead softbox",
        FloorMaterial: CourseMaterial.StudioFloor,
        LiftDeckMaterial: CourseMaterial.StudioTurntable,
        CenterDiscMaterial: CourseMaterial.StudioTurntable,
        ShowCenterDisc: true,
        Lighting: Object.freeze({ SunElevation: 72.0, SunAzimuth: 145.0, SolarIlluminance: 1.1, SkyScale: 0.35, EmissionScale: 1.25 }),
        CameraBounds: Object.freeze({ MaxRadius: 18.4, MaxZ: 5.25, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: 2.10, Pitch: 0.14, Distance: 11.8 }),
            To:   Object.freeze({ Yaw: 0.85, Pitch: 0.16, Distance: 8.2 })
        })
    }),
    Object.freeze({
        Key: "monolith",
        Title: "Monolith",
        FullTitle: "White Monolith Plaza",
        Subtitle: "Sculptural tilted white slabs · mirror plaza",
        FloorMaterial: CourseMaterial.MonolithFloor,
        LiftDeckMaterial: CourseMaterial.MonolithFloor,
        CenterDiscMaterial: CourseMaterial.MonolithFloor,
        ShowCenterDisc: false,
        Lighting: Object.freeze({ SunElevation: 24.0, SunAzimuth: 118.0, SolarIlluminance: 13.5, SkyScale: 1.05, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 30.0, MaxZ: 11.5, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: -1.12, Pitch: 0.08, Distance: 11.6 }),
            To:   Object.freeze({ Yaw: -1.36, Pitch: 0.08, Distance: 8.6 })
        })
    }),
    Object.freeze({
        Key: "saltflats",
        Title: "Salt Flats",
        FullTitle: "Bonneville Salt Flats",
        Subtitle: "Open-air sunlit white salt pan · distant mesas",
        FloorMaterial: CourseMaterial.SaltCrust,
        LiftDeckMaterial: CourseMaterial.SaltCrust,
        CenterDiscMaterial: CourseMaterial.SaltCrust,
        ShowCenterDisc: false,
        Lighting: Object.freeze({ SunElevation: 34.0, SunAzimuth: 118.0, SolarIlluminance: 14.5, SkyScale: 1.08, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 34.0, MaxZ: 13.5, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: 0.48, Pitch: 0.08, Distance: 11.4 }),
            To:   Object.freeze({ Yaw: 0.88, Pitch: 0.09, Distance: 8.4 })
        })
    }),
    Object.freeze({
        Key: "oculus",
        Title: "Oculus",
        FullTitle: "Desert Oculus Pavilion",
        Subtitle: "Circular sky oculus · reflecting pool · red-rock vista",
        FloorMaterial: CourseMaterial.OculusFloor,
        LiftDeckMaterial: CourseMaterial.OculusWater,
        CenterDiscMaterial: CourseMaterial.OculusWater,
        ShowCenterDisc: true,
        Lighting: Object.freeze({ SunElevation: 36.0, SunAzimuth: 226.0, SolarIlluminance: 11.5, SkyScale: 0.82, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 16.4, MaxZ: 4.55, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: -1.40, Pitch: 0.06, Distance: 13.4 }),
            To:   Object.freeze({ Yaw: -1.56, Pitch: 0.05, Distance: 11.8 })
        })
    }),
    Object.freeze({
        Key: "nocturne",
        Title: "Nocturne",
        FullTitle: "Nocturne Lightwall",
        Subtitle: "Twilight obsidian plaza · solar-yellow monolith lightwall",
        FloorMaterial: CourseMaterial.NocturneFloor,
        LiftDeckMaterial: CourseMaterial.NocturneFloor,
        CenterDiscMaterial: CourseMaterial.NocturneFloor,
        ShowCenterDisc: false,
        Lighting: Object.freeze({ SunElevation: -4.5, SunAzimuth: 210.0, SolarIlluminance: 0.30, SkyScale: 0.65, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 30.0, MaxZ: 11.5, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: 0.46, Pitch: 0.06, Distance: 10.6 }),
            To:   Object.freeze({ Yaw: 0.64, Pitch: 0.06, Distance: 8.2 })
        })
    }),
    Object.freeze({
        Key: "ribstudio",
        Title: "Linear Rib",
        FullTitle: "Linear Rib Studio",
        Subtitle: "Curved white LED light ribs · charcoal acoustic gallery · plinth halo",
        FloorMaterial: CourseMaterial.RibStudioFloor,
        LiftDeckMaterial: CourseMaterial.StudioTurntable,
        CenterDiscMaterial: CourseMaterial.StudioTurntable,
        ShowCenterDisc: true,
        Lighting: Object.freeze({ SunElevation: 55.0, SunAzimuth: 145.0, SolarIlluminance: 0.0, SkyScale: 0.05, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 10.2, MaxZ: 4.15, MinZ: 0.40 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: 1.32, Pitch: 0.09, Distance: 9.8 }),
            To:   Object.freeze({ Yaw: 1.57, Pitch: 0.07, Distance: 8.8 })
        })
    }),
    Object.freeze({
        Key: "coastal",
        Title: "Coastal",
        FullTitle: "Coastal Canopy Pavilion",
        Subtitle: "Cantilevered concrete crescent · sunlit azure bay & headland",
        FloorMaterial: CourseMaterial.CoastalCanopy,
        LiftDeckMaterial: CourseMaterial.CoastalCanopy,
        CenterDiscMaterial: CourseMaterial.CoastalCanopy,
        ShowCenterDisc: false,
        Lighting: Object.freeze({ SunElevation: 26.0, SunAzimuth: 218.0, SolarIlluminance: 13.8, SkyScale: 1.15, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 24.0, MaxZ: 4.35, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: -1.82, Pitch: 0.09, Distance: 11.2 }),
            To:   Object.freeze({ Yaw: -1.52, Pitch: 0.08, Distance: 9.6 })
        })
    }),
    Object.freeze({
        Key: "courtyard",
        Title: "Courtyard",
        FullTitle: "Sky Courtyard Rotunda",
        Subtitle: "Minimalist circular concrete wall · reveal slot · open azure sky",
        FloorMaterial: CourseMaterial.MonolithFloor,
        LiftDeckMaterial: CourseMaterial.MonolithFloor,
        CenterDiscMaterial: CourseMaterial.MonolithFloor,
        ShowCenterDisc: false,
        Lighting: Object.freeze({ SunElevation: 42.0, SunAzimuth: 128.0, SolarIlluminance: 15.2, SkyScale: 1.25, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 15.4, MaxZ: 4.15, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: 0.55, Pitch: 0.12, Distance: 12.6 }),
            To:   Object.freeze({ Yaw: 0.92, Pitch: 0.10, Distance: 10.6 })
        })
    }),
    Object.freeze({
        Key: "gallery",
        Title: "Gallery",
        FullTitle: "Sculpted Horizon Gallery",
        Subtitle: "Pure matte-white horizon wall · diagonal sun wedge · brushed silver floor",
        FloorMaterial: CourseMaterial.GalleryFloor,
        LiftDeckMaterial: CourseMaterial.GalleryFloor,
        CenterDiscMaterial: CourseMaterial.GalleryFloor,
        ShowCenterDisc: false,
        Lighting: Object.freeze({ SunElevation: 28.0, SunAzimuth: 38.0, SolarIlluminance: 13.5, SkyScale: 1.20, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 24.0, MaxZ: 9.5, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: 1.34, Pitch: 0.07, Distance: 10.6 }),
            To:   Object.freeze({ Yaw: 1.57, Pitch: 0.06, Distance: 9.2 })
        })
    }),
    Object.freeze({
        Key: "rotunda",
        Title: "Rotunda",
        FullTitle: "Obsidian Reveal Rotunda",
        Subtitle: "Stealth dark circular hall · coffered sky drum · glowing floor halo",
        FloorMaterial: CourseMaterial.RotundaFloor,
        LiftDeckMaterial: CourseMaterial.RotundaDarkWall,
        CenterDiscMaterial: CourseMaterial.RotundaDarkWall,
        ShowCenterDisc: true,
        Lighting: Object.freeze({ SunElevation: -8.0, SunAzimuth: 180.0, SolarIlluminance: 0.0, SkyScale: 0.06, EmissionScale: 1.0 }),
        CameraBounds: Object.freeze({ MaxRadius: 13.6, MaxZ: 4.15, MinZ: 0.35 }),
        EntryCamera: Object.freeze({
            From: Object.freeze({ Yaw: 1.24, Pitch: 0.11, Distance: 11.4 }),
            To:   Object.freeze({ Yaw: 1.54, Pitch: 0.08, Distance: 10.4 })
        })
    })
]);

export function ResolveLobbyVenueDescriptor(VenueKey = "studio")
{
    return LobbyVenues.find((V) => V.Key === VenueKey) || LobbyVenues[0];
}

export function ResolveLobbyVenue()
{
    try
    {
        const Query = new URLSearchParams((globalThis.location || {}).search || "");
        const FromQuery = Query.get("venue");
        if (FromQuery && LobbyVenues.some((V) => V.Key === FromQuery))
            return FromQuery;
        const Saved = globalThis.sessionStorage?.getItem("frontier.lobbyVenue");
        if (Saved && LobbyVenues.some((V) => V.Key === Saved))
            return Saved;
    }
    catch {}
    return "studio";
}

// 📝 Requested by Lobby.html (its path), ?lobby=1, ?level=lobby, or default entry on index.html (no ?level= param).
export function ResolveLobbyRequested()
{
    try
    {
        const Location = globalThis.location || {};
        const Query = new URLSearchParams(Location.search || "");
        if (Query.get("lobby") === "1")
            return true;
        const Level = Query.get("level");
        if (Level === "lobby")
            return true;
        if (Level)
            return false;
        return /(?:^|\/)lobby\.html$/i.test(Location.pathname || "") || !Level;
    }
    catch
    {
        return false;
    }
}

export const LobbyLiftConstants = Object.freeze({
    HeroTurntableRadius: 4.65,   // [m] center hero turntable / obsidian reflecting pool radius
    LiftRadius:          3.45,   // [m] subtle single-hero hydraulic showcase stage radius
    StageRaisedZ:        0.42,   // [m] elevated showcase plinth height when raised
    PlatformThickness:   0.46    // [m] solid stage plinth height (retracted flush at z = 0 when lowered)
});

// 📝 Pure single-hero Lobby: no extra guest cars or open dark floor pit cutouts cluttering the architecture.
export const LobbyVehiclePlacements = Object.freeze([]);
export const LobbyDistanceCutouts   = Object.freeze([]);

//------------------------------------------------------------------------------------------------------------------------
//                                                 SHARED GEOMETRY HELPERS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Closed watertight oriented prism given 4 base corners [B0,B1,B2,B3] (CCW from +Z) and 4 top corners [T0,T1,T2,T3].
function EmitSlantedSlab(Facets, Base, Top, Family)
{
    EmitQuad(Facets, Base[0], Base[3], Base[2], Base[1], Family);
    EmitQuad(Facets, Top[0], Top[1], Top[2], Top[3], Family);
    for (let I = 0; I < 4; ++I)
    {
        const J = (I + 1) % 4;
        EmitQuad(Facets, Base[I], Base[J], Top[J], Top[I], Family);
    }
}

function RayToBoxBoundary(CX, CY, DX, DY, MinX, MaxX, MinY, MaxY)
{
    let T = Infinity;
    if (DX > 1e-8)  T = Math.min(T, (MaxX - CX) / DX);
    if (DX < -1e-8) T = Math.min(T, (MinX - CX) / DX);
    if (DY > 1e-8)  T = Math.min(T, (MaxY - CY) / DY);
    if (DY < -1e-8) T = Math.min(T, (MinY - CY) / DY);
    return [CX + DX * T, CY + DY * T];
}

//------------------------------------------------------------------------------------------------------------------------
//                                                VENUE 1: STUDIO GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

function EmitCycloramaFacets()
{
    const Facets = [];
    const Segments = 96;
    const Profile = [];
    for (let Step = 0; Step <= 8; ++Step)
    {
        const T = Step / 8, U = 1.0 - T;
        Profile.push([U * U * 17.0 + 2.0 * U * T * 20.0 + T * T * 20.0,
                      U * U * 0.0 + 2.0 * U * T * 0.0 + T * T * 3.4]);
    }
    Profile.push([20.0, 5.2], [20.0, 7.4]);

    const Point = (Angle, Radius, Z) => [Radius * Math.cos(Angle), Radius * Math.sin(Angle), Z];
    for (let Segment = 0; Segment < Segments; ++Segment)
    {
        const A0 = 2.0 * Math.PI * Segment / Segments;
        const A1 = 2.0 * Math.PI * (Segment + 1) / Segments;
        for (let Step = 0; Step + 1 < Profile.length; ++Step)
        {
            const [R0, Z0] = Profile[Step], [R1, Z1] = Profile[Step + 1];
            EmitQuad(Facets, Point(A0, R0, Z0), Point(A0, R1, Z1), Point(A1, R1, Z1), Point(A1, R0, Z0), CourseMaterial.StudioWall);
        }
    }
    return Facets;
}

function EmitSoftboxFacets()
{
    return EmitBoxFacets(24.0, 11.0, 0.22, CourseMaterial.StudioFrame,
        [CourseMaterial.StudioFrame, CourseMaterial.StudioSoftbox, CourseMaterial.StudioFrame,
         CourseMaterial.StudioFrame, CourseMaterial.StudioFrame, CourseMaterial.StudioFrame]);
}

//------------------------------------------------------------------------------------------------------------------------
//                                            VENUE 2: WHITE MONOLITH PLAZA
//------------------------------------------------------------------------------------------------------------------------

function EmitMonolithTowerFacets()
{
    const Facets = [];
    const LeanX = 4.2;
    const Base = [[-3.4, -2.4, 0.0], [3.4, -2.4, 0.0], [3.4, 2.4, 0.0], [-3.4, 2.4, 0.0]];
    const Top  = [[-2.6 + LeanX, -2.4, 19.5], [2.8 + LeanX, -2.4, 19.5], [2.8 + LeanX, 2.4, 19.5], [-2.6 + LeanX, 2.4, 19.5]];
    EmitSlantedSlab(Facets, Base, Top, CourseMaterial.MonolithSlab);
    return Facets;
}

function EmitMonolithWedgeFacets(Length = 34.0, Thickness = 2.4, PeakHeight = 13.0, CantY = -2.6, MirrorX = false)
{
    const Facets = [];
    const S = MirrorX ? -1 : 1;
    const X0 = -0.5 * Length * S;
    const X1 =  0.5 * Length * S;
    const LowX = Math.min(X0, X1), HighX = Math.max(X0, X1);
    const ZLow = MirrorX ? PeakHeight : 0.12;
    const ZHigh = MirrorX ? 0.12 : PeakHeight;
    const T = 0.5 * Thickness;

    const Base = [[LowX, -T, 0.0], [HighX, -T, 0.0], [HighX, T, 0.0], [LowX, T, 0.0]];
    const Top  = [
        [LowX,  -T + (ZLow / PeakHeight) * CantY, ZLow],
        [HighX, -T + (ZHigh / PeakHeight) * CantY, ZHigh],
        [HighX,  T + (ZHigh / PeakHeight) * CantY, ZHigh],
        [LowX,   T + (ZLow / PeakHeight) * CantY, ZLow]
    ];
    EmitSlantedSlab(Facets, Base, Top, CourseMaterial.MonolithSlab);
    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                           VENUE 3: BONNEVILLE SALT FLATS
//------------------------------------------------------------------------------------------------------------------------

function EmitSaltHorizonFacets()
{
    const Facets = [];
    const Segments = 96;
    const InnerR = 122.0;
    const OuterR = 139.0;
    const HeightAt = (A) =>
    {
        const Wave = 0.5 + 0.5 * Math.sin(A * 3.0 + 0.4) * Math.cos(A * 2.0 - 0.7)
                   + 0.30 * Math.sin(A * 7.0 + 1.2)
                   + 0.18 * Math.cos(A * 13.0 - 0.5);
        return Math.max(0.25, Math.min(3.8, 0.7 + Wave * 2.1));
    };
    const Pt = (A, R, Z) => [R * Math.cos(A), R * Math.sin(A), Z];
    for (let S = 0; S < Segments; ++S)
    {
        const A0 = 2.0 * Math.PI * S / Segments;
        const A1 = 2.0 * Math.PI * (S + 1) / Segments;
        const H0 = HeightAt(A0), H1 = HeightAt(A1);
        const MidR = 0.5 * (InnerR + OuterR);
        EmitQuad(Facets, Pt(A0, InnerR, -0.05), Pt(A0, MidR, H0), Pt(A1, MidR, H1), Pt(A1, InnerR, -0.05), CourseMaterial.SaltMesa);
        EmitQuad(Facets, Pt(A0, MidR, H0), Pt(A0, OuterR, -0.05), Pt(A1, OuterR, -0.05), Pt(A1, MidR, H1), CourseMaterial.SaltMesa);
    }
    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                           VENUE 4: DESERT OCULUS PAVILION
//------------------------------------------------------------------------------------------------------------------------

function EmitOculusRoofFacets()
{
    const Facets = [];
    const Half = 19.5;
    const R = 8.4;
    const ZBot = 5.05;
    const ZTop = 6.20;
    const Segs = 72;

    const Angles = [];
    for (let S = 0; S < Segs; ++S) Angles.push(2.0 * Math.PI * S / Segs);
    for (const [KX, KY] of [[Half, Half], [-Half, Half], [-Half, -Half], [Half, -Half]])
    {
        let A = Math.atan2(KY, KX);
        if (A < 0) A += 2.0 * Math.PI;
        Angles.push(A);
    }
    Angles.sort((A, B) => A - B);
    const Clean = [];
    for (const A of Angles)
        if (Clean.length === 0 || (A - Clean[Clean.length - 1]) > 1e-4) Clean.push(A);

    const N = Clean.length;
    for (let I = 0; I < N; ++I)
    {
        const A0 = Clean[I], A1 = Clean[(I + 1) % N];
        const DX0 = Math.cos(A0), DY0 = Math.sin(A0);
        const DX1 = Math.cos(A1), DY1 = Math.sin(A1);
        const In0Bot = [R * DX0, R * DY0, ZBot], In1Bot = [R * DX1, R * DY1, ZBot];
        const In0Top = [R * DX0, R * DY0, ZTop], In1Top = [R * DX1, R * DY1, ZTop];
        const [BX0, BY0] = RayToBoxBoundary(0, 0, DX0, DY0, -Half, Half, -Half, Half);
        const [BX1, BY1] = RayToBoxBoundary(0, 0, DX1, DY1, -Half, Half, -Half, Half);

        EmitQuad(Facets, In0Bot, In1Bot, [BX1, BY1, ZBot], [BX0, BY0, ZBot], CourseMaterial.OculusConcrete);
        EmitQuad(Facets, In0Top, [BX0, BY0, ZTop], [BX1, BY1, ZTop], In1Top, CourseMaterial.OculusConcrete);
        EmitQuad(Facets, In0Bot, In0Top, In1Top, In1Bot, CourseMaterial.MonolithSlab);
    }

    for (let I = 0; I < 28; ++I)
    {
        const A = (I / 28) * 2.0 * Math.PI + (I % 3) * 0.06;
        const Rad = R + 0.48 + (I % 3) * 0.28;
        const CX = Rad * Math.cos(A), CY = Rad * Math.sin(A);
        const W = 0.42 + (I % 3) * 0.16;
        const H = 0.34 + (I % 4) * 0.16;
        const HW = 0.5 * W;
        EmitSlantedSlab(Facets,
            [[CX - HW, CY - HW, ZTop - 0.04], [CX + HW, CY - HW, ZTop - 0.04], [CX + HW, CY + HW, ZTop - 0.04], [CX - HW, CY + HW, ZTop - 0.04]],
            [[CX - HW * 0.45, CY - HW * 0.45, ZTop + H], [CX + HW * 0.45, CY - HW * 0.45, ZTop + H], [CX + HW * 0.45, CY + HW * 0.45, ZTop + H], [CX - HW * 0.45, CY + HW * 0.45, ZTop + H]],
            CourseMaterial.CanyonFoliage);
    }
    return Facets;
}

function EmitOculusWallsFacets()
{
    const Facets = [];
    const Half = 18.8;
    const WallH = 5.15;
    const Thick = 0.85;
    const AddBox = (CX, CY, BaseZ, SX, SY, SZ, Mat = CourseMaterial.OculusConcrete) =>
    {
        for (const F of EmitBoxFacets(SX, SY, SZ, Mat))
            Facets.push({ Family: F.Family, Corners: F.Corners.map(([x, y, z]) => [x + CX, y + CY, z + BaseZ]) });
    };

    AddBox( Half, 0.0, 0.0, Thick, Half * 2.0, WallH);
    AddBox(-Half, 0.0, 0.0, Thick, Half * 2.0, WallH);
    AddBox( 0.0, -Half, 0.0, Half * 2.0, Thick, WallH);

    const OpenHalf = 8.8;
    const PierW = Half - OpenHalf;
    AddBox(-(OpenHalf + 0.5 * PierW), Half, 0.0, PierW, Thick, WallH);
    AddBox( (OpenHalf + 0.5 * PierW), Half, 0.0, PierW, Thick, WallH);
    AddBox(0.0, Half, WallH - 0.55, OpenHalf * 2.0 + 0.2, Thick, 0.55);

    const RailY = Half + 0.14;
    const RailH = 1.04;
    AddBox(0.0, RailY, RailH - 0.04, OpenHalf * 2.0, 0.06, 0.05, CourseMaterial.StudioFrame);
    AddBox(0.0, RailY, 0.02,         OpenHalf * 2.0, 0.06, 0.05, CourseMaterial.StudioFrame);
    const BarCount = 44;
    for (let I = 0; I <= BarCount; ++I)
    {
        const X = -OpenHalf + (2.0 * OpenHalf * I) / BarCount;
        AddBox(X, RailY, 0.0, 0.032, 0.032, RailH, CourseMaterial.StudioFrame);
    }
    return Facets;
}

function EmitOculusCanyonVistaFacets()
{
    const Facets = [];
    const AddSlab = (CX, CY, BaseZ, W, D, H, Taper = 0.72, Mat = CourseMaterial.CanyonRock) =>
    {
        const HW = 0.5 * W, HD = 0.5 * D;
        const THW = HW * Taper, THD = HD * Taper;
        const Base = [[CX - HW, CY - HD, BaseZ], [CX + HW, CY - HD, BaseZ], [CX + HW, CY + HD, BaseZ], [CX - HW, CY + HD, BaseZ]];
        const Top  = [[CX - THW, CY - THD, BaseZ + H], [CX + THW, CY - THD, BaseZ + H], [CX + THW, CY + THD, BaseZ + H], [CX - THW, CY + THD, BaseZ + H]];
        EmitSlantedSlab(Facets, Base, Top, Mat);
    };

    AddSlab(0.0, 23.0, -3.5, 44.0, 10.0, 3.3, 0.94, CourseMaterial.CanyonRock);
    AddSlab(-4.0, 21.8, -0.3, 14.0, 2.8, 1.0, 0.68, CourseMaterial.CanyonFoliage);
    AddSlab( 5.5, 22.0, -0.3, 10.0, 2.4, 0.85, 0.65, CourseMaterial.CanyonFoliage);

    AddSlab(-9.5,  33.0, -2.5, 22.0, 11.0, 11.0, 0.74, CourseMaterial.CanyonRock);
    AddSlab(-4.8,  30.5, -2.5, 11.0,  7.5,  8.2, 0.68, CourseMaterial.CanyonRock);
    AddSlab(-13.5, 36.0, -2.5, 18.0, 12.0, 13.5, 0.66, CourseMaterial.CanyonRock);
    AddSlab( 11.5, 41.0, -2.5, 22.0, 12.0,  9.5, 0.68, CourseMaterial.CanyonRock);
    AddSlab(  5.5, 54.0, -2.5, 34.0, 14.0, 13.5, 0.60, CourseMaterial.SaltMesa);

    const TX = 0.8, TY = 24.0;
    EmitSlantedSlab(Facets,
        [[TX - 0.22, TY - 0.22, -0.4], [TX + 0.22, TY - 0.22, -0.4], [TX + 0.22, TY + 0.22, -0.4], [TX - 0.22, TY + 0.22, -0.4]],
        [[TX - 0.04, TY - 0.14,  2.5], [TX + 0.26, TY - 0.14,  2.5], [TX + 0.26, TY + 0.14,  2.5], [TX - 0.04, TY + 0.14,  2.5]],
        CourseMaterial.CanyonTrunk);
    EmitSlantedSlab(Facets,
        [[TX - 0.02, TY - 0.11, 2.3], [TX + 0.20, TY - 0.11, 2.3], [TX + 0.20, TY + 0.11, 2.3], [TX - 0.02, TY + 0.11, 2.3]],
        [[TX - 1.9,  TY - 0.09, 4.1], [TX - 1.68, TY - 0.09, 4.1], [TX - 1.68, TY + 0.09, 4.1], [TX - 1.9,  TY + 0.09, 4.1]],
        CourseMaterial.CanyonTrunk);
    EmitSlantedSlab(Facets,
        [[TX + 0.04, TY - 0.11, 2.4], [TX + 0.24, TY - 0.11, 2.4], [TX + 0.24, TY + 0.11, 2.4], [TX + 0.04, TY + 0.11, 2.4]],
        [[TX + 1.85, TY - 0.09, 4.3], [TX + 2.08, TY - 0.09, 4.3], [TX + 2.08, TY + 0.09, 4.3], [TX + 1.85, TY + 0.09, 4.3]],
        CourseMaterial.CanyonTrunk);
    EmitSlantedSlab(Facets,
        [[TX + 0.00, TY - 0.10, 2.4], [TX + 0.18, TY - 0.10, 2.4], [TX + 0.18, TY + 0.10, 2.4], [TX + 0.00, TY + 0.10, 2.4]],
        [[TX - 0.35, TY - 0.08, 4.6], [TX - 0.15, TY - 0.08, 4.6], [TX - 0.15, TY + 0.08, 4.6], [TX - 0.35, TY + 0.08, 4.6]],
        CourseMaterial.CanyonTrunk);

    const Clusters = [
        [TX - 2.0, TY - 0.1, 3.8, 3.2, 2.4, 1.3],
        [TX + 0.0, TY + 0.1, 4.3, 3.8, 2.6, 1.5],
        [TX + 2.0, TY - 0.1, 4.0, 3.1, 2.3, 1.3],
        [TX - 0.9, TY + 0.3, 3.4, 2.4, 2.0, 1.1],
        [TX + 1.0, TY - 0.2, 3.5, 2.3, 1.9, 1.1]
    ];
    for (const [CX, CY, CZ, W, D, H] of Clusters)
        AddSlab(CX, CY, CZ, W, D, H, 0.72, CourseMaterial.CanyonFoliage);

    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                           VENUE 5: NOCTURNE LIGHTWALL
//------------------------------------------------------------------------------------------------------------------------

// 📝 Towering floating solar-yellow monolithic architectural lightbox (matching Lotus Eletre dark studio Ref).
function EmitNocturneLightwallFacets()
{
    const Facets = [];
    // Crisp solid glowing monolith lightbox floating cleanly 0.65 m above the reflective dark floor
    for (const F of EmitBoxFacets(12.5, 0.38, 4.85, CourseMaterial.NocturneLightwall))
        Facets.push({ Family: F.Family, Corners: F.Corners.map(([x, y, z]) => [x, y, z + 0.65]) });
    return Facets;
}

// 📝 Moody pitch-dark twilight mountain range encircling the dark wet-satin horizon.
function EmitNocturneMountainsFacets()
{
    const Facets = [];
    const Segments = 96;
    const InnerR = 92.0;
    const OuterR = 136.0;
    const HeightAt = (A) =>
    {
        const Peak = 0.5 + 0.5 * Math.sin(A * 2.0 - 0.9) * Math.cos(A * 3.0 + 0.4)
                   + 0.38 * Math.sin(A * 5.0 + 1.1)
                   + 0.22 * Math.cos(A * 11.0 - 0.3);
        return Math.max(3.2, Math.min(22.0, 5.5 + Peak * 12.5));
    };
    const Pt = (A, R, Z) => [R * Math.cos(A), R * Math.sin(A), Z];
    for (let S = 0; S < Segments; ++S)
    {
        const A0 = 2.0 * Math.PI * S / Segments;
        const A1 = 2.0 * Math.PI * (S + 1) / Segments;
        const H0 = HeightAt(A0), H1 = HeightAt(A1);
        const MidR = 0.5 * (InnerR + OuterR);
        EmitQuad(Facets, Pt(A0, InnerR, -0.05), Pt(A0, MidR, H0), Pt(A1, MidR, H1), Pt(A1, InnerR, -0.05), CourseMaterial.NocturneMountain);
        EmitQuad(Facets, Pt(A0, MidR, H0), Pt(A0, OuterR, -0.05), Pt(A1, OuterR, -0.05), Pt(A1, MidR, H1), CourseMaterial.NocturneMountain);
    }
    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                          VENUE 6: LINEAR RIB STUDIO
//------------------------------------------------------------------------------------------------------------------------

// 📝 Luxury enclosed automotive gallery with curved parallel ceiling-to-wall white LED ribs, bronze dado & acoustic sides.
function EmitRibStudioHallFacets()
{
    const Facets = [];
    const HalfX = 10.6;
    const BackY = -5.60;
    const FrontY = 12.60;
    const DadoZ = 1.22;
    const CoveR = 1.25;
    const CoveStartZ = 3.50;
    const CeilZ = CoveStartZ + CoveR; // 4.75 m
    const CoveStartY = BackY + CoveR; // -4.35 m

    // ① Warm dark architectural bronze-espresso lower back dado (Z = 0..1.22 m)
    EmitQuad(Facets, [-HalfX, BackY, 0.0], [HalfX, BackY, 0.0], [HalfX, BackY, DadoZ], [-HalfX, BackY, DadoZ], CourseMaterial.RibStudioDado);

    // ② Bright matte-white vertical rear wall above dado (Z = 1.22..3.50 m)
    EmitQuad(Facets, [-HalfX, BackY, DadoZ], [HalfX, BackY, DadoZ], [HalfX, BackY, CoveStartZ], [-HalfX, BackY, CoveStartZ], CourseMaterial.RibStudioWall);

    // ③ Smooth 12-segment quarter-cylinder white cove sweeping from back wall up into the ceiling
    const CoveSegs = 12;
    for (let S = 0; S < CoveSegs; ++S)
    {
        const T0 = (S / CoveSegs) * (Math.PI * 0.5);
        const T1 = ((S + 1) / CoveSegs) * (Math.PI * 0.5);
        const Y0 = CoveStartY - CoveR * Math.cos(T0), Z0 = CoveStartZ + CoveR * Math.sin(T0);
        const Y1 = CoveStartY - CoveR * Math.cos(T1), Z1 = CoveStartZ + CoveR * Math.sin(T1);
        EmitQuad(Facets, [-HalfX, Y0, Z0], [HalfX, Y0, Z0], [HalfX, Y1, Z1], [-HalfX, Y1, Z1], CourseMaterial.RibStudioWall);
    }

    // ④ Flat matte-white ceiling forward over the car (downward -Z normal!) + enclosing dark front wall behind camera
    EmitQuad(Facets, [-HalfX, CoveStartY, CeilZ], [-HalfX, FrontY, CeilZ], [HalfX, FrontY, CeilZ], [HalfX, CoveStartY, CeilZ], CourseMaterial.RibStudioWall);
    EmitQuad(Facets, [HalfX, FrontY, 0.0], [-HalfX, FrontY, 0.0], [-HalfX, FrontY, CeilZ], [HalfX, FrontY, CeilZ], CourseMaterial.RibStudioSide);

    // ⑤ 15 parallel continuous glowing white LED light ribs sweeping from ceiling down the cove onto the white backdrop!
    const RibHalfW = 0.045;
    const Offset = 0.025;
    for (let I = -7; I <= 7; ++I)
    {
        const RX = I * 1.35;
        // Vertical back-wall LED strip
        EmitQuad(Facets, [RX - RibHalfW, BackY + Offset, DadoZ + 0.04], [RX + RibHalfW, BackY + Offset, DadoZ + 0.04],
                         [RX + RibHalfW, BackY + Offset, CoveStartZ], [RX - RibHalfW, BackY + Offset, CoveStartZ], CourseMaterial.RibStudioLed);
        // Curved cove LED strip
        for (let S = 0; S < CoveSegs; ++S)
        {
            const T0 = (S / CoveSegs) * (Math.PI * 0.5);
            const T1 = ((S + 1) / CoveSegs) * (Math.PI * 0.5);
            const R = CoveR - Offset;
            const Y0 = CoveStartY - R * Math.cos(T0), Z0 = CoveStartZ + R * Math.sin(T0);
            const Y1 = CoveStartY - R * Math.cos(T1), Z1 = CoveStartZ + R * Math.sin(T1);
            EmitQuad(Facets, [RX - RibHalfW, Y0, Z0], [RX + RibHalfW, Y0, Z0], [RX + RibHalfW, Y1, Z1], [RX - RibHalfW, Y1, Z1], CourseMaterial.RibStudioLed);
        }
        // Horizontal ceiling LED strip (downward -Z normal!)
        EmitQuad(Facets, [RX - RibHalfW, CoveStartY, CeilZ - Offset], [RX - RibHalfW, FrontY - 0.5, CeilZ - Offset],
                         [RX + RibHalfW, FrontY - 0.5, CeilZ - Offset], [RX + RibHalfW, CoveStartY, CeilZ - Offset], CourseMaterial.RibStudioLed);
    }

    // ⑥ Sculpted dark charcoal horizontal-louvered acoustic side walls (Left +X and Right -X)
    const SlatCount = 14;
    for (const Side of [-1, 1])
    {
        const WX = Side * HalfX;
        const Inward = -Side;
        // Base side wall backing
        if (Side === 1)
            EmitQuad(Facets, [WX, FrontY, 0.0], [WX, BackY, 0.0], [WX, BackY, CeilZ], [WX, FrontY, CeilZ], CourseMaterial.RibStudioSide);
        else
            EmitQuad(Facets, [WX, BackY, 0.0], [WX, FrontY, 0.0], [WX, FrontY, CeilZ], [WX, BackY, CeilZ], CourseMaterial.RibStudioSide);

        // Horizontal sculpted acoustic bevel ribs along each dark side wall
        for (let R = 0; R < SlatCount; ++R)
        {
            const Z0 = 0.20 + R * 0.31;
            const ZMid = Z0 + 0.13;
            const Z1 = Z0 + 0.26;
            const ProtrudeX = WX + Inward * 0.18;
            if (Side === 1)
            {
                EmitQuad(Facets, [WX, FrontY, Z0], [WX, BackY, Z0], [ProtrudeX, BackY, ZMid], [ProtrudeX, FrontY, ZMid], CourseMaterial.RibStudioSide);
                EmitQuad(Facets, [ProtrudeX, FrontY, ZMid], [ProtrudeX, BackY, ZMid], [WX, BackY, Z1], [WX, FrontY, Z1], CourseMaterial.RibStudioSide);
            }
            else
            {
                EmitQuad(Facets, [WX, BackY, Z0], [WX, FrontY, Z0], [ProtrudeX, FrontY, ZMid], [ProtrudeX, BackY, ZMid], CourseMaterial.RibStudioSide);
                EmitQuad(Facets, [ProtrudeX, BackY, ZMid], [ProtrudeX, FrontY, ZMid], [WX, FrontY, Z1], [WX, BackY, Z1], CourseMaterial.RibStudioSide);
            }
        }
    }

    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                       VENUE 7: COASTAL CANOPY PAVILION
//------------------------------------------------------------------------------------------------------------------------

// 📝 Sculptural white cantilevered concrete crescent arch opening onto a sunlit bay (sweeps down to floor on -X left).
function EmitCoastalCanopyFacets()
{
    const Facets = [];
    const Segs = 40;
    const TopZ = 5.05;
    const SlabT = 0.42;

    // ① Sweeping curved roof canopy cantilevered overhead & wrapping down to ground on the -X left flank!
    // Parametric horseshoe/crescent opening facing (+X, +Y) ocean vista (camera sits around Y = -9.6 looking +Y)
    const OuterR = 26.0;
    const InnerCenterX = 5.8;
    const InnerCenterY = 7.4;
    const InnerRX = 18.8;
    const InnerRY = 11.4;
    // Arc spans from angle +0.58 rad (front-right sky edge) around the back-left (-X, -Y) to +5.15 rad
    const AStart = 0.72 * Math.PI;
    const AEnd   = 1.88 * Math.PI;

    for (let S = 0; S < Segs; ++S)
    {
        const U0 = S / Segs, U1 = (S + 1) / Segs;
        const A0 = AStart + (AEnd - AStart) * U0;
        const A1 = AStart + (AEnd - AStart) * U1;

        const In0 = [InnerCenterX + InnerRX * Math.cos(A0), InnerCenterY + InnerRY * Math.sin(A0)];
        const In1 = [InnerCenterX + InnerRX * Math.cos(A1), InnerCenterY + InnerRY * Math.sin(A1)];
        const Out0 = [InnerCenterX + (InnerRX + OuterR * 0.55) * Math.cos(A0), InnerCenterY + (InnerRY + OuterR * 0.55) * Math.sin(A0)];
        const Out1 = [InnerCenterX + (InnerRX + OuterR * 0.55) * Math.cos(A1), InnerCenterY + (InnerRY + OuterR * 0.55) * Math.sin(A1)];

        // Roof soffit (underside — downward -Z normal!) + top deck (+Z normal!) + razor-clean inner fascia rim
        EmitQuad(Facets, [In0[0], In0[1], TopZ], [In1[0], In1[1], TopZ], [Out1[0], Out1[1], TopZ], [Out0[0], Out0[1], TopZ], CourseMaterial.CoastalCanopy);
        EmitQuad(Facets, [In0[0], In0[1], TopZ + SlabT], [Out0[0], Out0[1], TopZ + SlabT], [Out1[0], Out1[1], TopZ + SlabT], [In1[0], In1[1], TopZ + SlabT], CourseMaterial.CoastalCanopy);
        EmitQuad(Facets, [In0[0], In0[1], TopZ], [In1[0], In1[1], TopZ], [In1[0], In1[1], TopZ + SlabT], [In0[0], In0[1], TopZ + SlabT], CourseMaterial.CoastalCanopy);
    }

    // ② Sculptural curved wing-wall sweeping down from the canopy on the -X left flank to touch the plaza cleanly!
    const WallSegs = 18;
    const WStart = 0.78 * Math.PI;
    const WEnd   = 1.24 * Math.PI;
    for (let S = 0; S < WallSegs; ++S)
    {
        const T0 = S / WallSegs, T1 = (S + 1) / WallSegs;
        const A0 = WStart + (WEnd - WStart) * T0;
        const A1 = WStart + (WEnd - WStart) * T1;
        // Curved inward taper from roof lip down to floor curve
        const Top0 = [InnerCenterX + (InnerRX + 1.2) * Math.cos(A0), InnerCenterY + (InnerRY + 1.2) * Math.sin(A0), TopZ + 0.05];
        const Top1 = [InnerCenterX + (InnerRX + 1.2) * Math.cos(A1), InnerCenterY + (InnerRY + 1.2) * Math.sin(A1), TopZ + 0.05];
        const Bot0 = [InnerCenterX + (InnerRX + 4.4) * Math.cos(A0), InnerCenterY + (InnerRY + 4.4) * Math.sin(A0), -0.05];
        const Bot1 = [InnerCenterX + (InnerRX + 4.4) * Math.cos(A1), InnerCenterY + (InnerRY + 4.4) * Math.sin(A1), -0.05];
        EmitQuad(Facets, Bot0, Bot1, Top1, Top0, CourseMaterial.CoastalCanopy);
        EmitQuad(Facets, Bot1, Bot0, Top0, Top1, CourseMaterial.CoastalCanopy);
    }

    return Facets;
}

// 📝 Sunlit azure ocean bay + dramatic jagged dark-slate coastal mountain headland along the +Y / +X horizon!
function EmitCoastalHeadlandFacets()
{
    const Facets = [];
    // Calm coastal ocean plane stretching out beyond the concrete terrace (Y = 34..155 m)
    EmitQuad(Facets, [-145.0, 34.0, -0.04], [145.0, 34.0, -0.04], [145.0, 155.0, -0.04], [-145.0, 155.0, -0.04], CourseMaterial.CoastalSea);

    // Dramatic jagged Cape/Mediterranean coastal mountain headland jutting into the right side of the bay!
    const RidgePts = [
        [ 6.0, 96.0,  0.0], [11.0, 94.0,  3.2], [16.5, 92.0,  2.1], [23.0, 90.0,  9.8],
        [29.5, 89.0,  7.4], [37.0, 88.0, 12.6], [44.0, 87.0, 14.2], [53.0, 86.0, 18.8],
        [62.0, 85.0, 16.5], [73.0, 84.0, 21.6], [85.0, 84.0, 19.2], [98.0, 85.0, 23.0],
        [115.0, 86.0, 18.0], [135.0, 88.0, 12.0]
    ];
    for (let I = 0; I < RidgePts.length - 1; ++I)
    {
        const [X0, Y0, Z0] = RidgePts[I];
        const [X1, Y1, Z1] = RidgePts[I + 1];
        // Front slope facing bay + rear ridge slope
        EmitQuad(Facets, [X0, Y0 - 11.0, -0.1], [X1, Y1 - 11.0, -0.1], [X1, Y1, Z1], [X0, Y0, Z0], CourseMaterial.CoastalMountain);
        EmitQuad(Facets, [X0, Y0, Z0], [X1, Y1, Z1], [X1, Y1 + 18.0, -0.1], [X0, Y0 + 18.0, -0.1], CourseMaterial.CoastalMountain);
    }
    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                      VENUE 8: SKY COURTYARD ROTUNDA
//------------------------------------------------------------------------------------------------------------------------

// 📝 Minimalist Tadao Ando / James Turrell circular concrete courtyard with horizontal skylight reveal slot & sky oculus.
function EmitSkyCourtyardFacets()
{
    const Facets = [];
    const Segs = 64;
    const WallR = 16.5;
    const WallTopZ = 4.22;
    const RoofBotZ = 4.58; // 0.36 m continuous perimeter sun reveal gap!
    const RoofTopZ = 5.15;
    const OuterR = 17.6;
    const HoleR = 10.6;
    const HoleCX = -2.2;
    const HoleCY = 1.6;
    const Pt = (A, R, Z) => [R * Math.cos(A), R * Math.sin(A), Z];
    const HolePt = (A, Z) => [HoleCX + HoleR * Math.cos(A), HoleCY + HoleR * Math.sin(A), Z];

    for (let S = 0; S < Segs; ++S)
    {
        const A0 = 2.0 * Math.PI * S / Segs;
        const A1 = 2.0 * Math.PI * (S + 1) / Segs;

        // Inner curved concrete drum wall + outer backing + flat cap
        EmitQuad(Facets, Pt(A1, WallR, 0.0), Pt(A0, WallR, 0.0), Pt(A0, WallR, WallTopZ), Pt(A1, WallR, WallTopZ), CourseMaterial.CoastalCanopy);
        EmitQuad(Facets, Pt(A0, OuterR, 0.0), Pt(A1, OuterR, 0.0), Pt(A1, OuterR, WallTopZ), Pt(A0, OuterR, WallTopZ), CourseMaterial.CoastalCanopy);
        EmitQuad(Facets, Pt(A0, WallR, WallTopZ), Pt(A1, WallR, WallTopZ), Pt(A1, OuterR, WallTopZ), Pt(A0, OuterR, WallTopZ), CourseMaterial.CoastalCanopy);

        // Cantilevered sweeping circular roof ring floating 0.36 m above the perimeter wall! (soffit -Z, top +Z)
        EmitQuad(Facets, HolePt(A0, RoofBotZ), HolePt(A1, RoofBotZ), Pt(A1, OuterR, RoofBotZ), Pt(A0, OuterR, RoofBotZ), CourseMaterial.CoastalCanopy);
        EmitQuad(Facets, HolePt(A0, RoofTopZ), Pt(A0, OuterR, RoofTopZ), Pt(A1, OuterR, RoofTopZ), HolePt(A1, RoofTopZ), CourseMaterial.CoastalCanopy);
        // Inner sky-oculus curved rim fascia
        EmitQuad(Facets, HolePt(A1, RoofBotZ), HolePt(A0, RoofBotZ), HolePt(A0, RoofTopZ), HolePt(A1, RoofTopZ), CourseMaterial.CoastalCanopy);
        // Outer roof rim fascia
        EmitQuad(Facets, Pt(A0, OuterR, RoofBotZ), Pt(A1, OuterR, RoofBotZ), Pt(A1, OuterR, RoofTopZ), Pt(A0, OuterR, RoofTopZ), CourseMaterial.CoastalCanopy);
    }

    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                     VENUE 9: SCULPTED HORIZON GALLERY
//------------------------------------------------------------------------------------------------------------------------

// 📝 Pure soaring matte-white studio horizon wall under crisp cerulean sky + off-camera shadow fin for diagonal wedge!
function EmitGalleryBackdropFacets()
{
    const Facets = [];
    // Tall seamless white architectural backdrop wall at Y = -7.6 m (leaves upper sky band pure cerulean blue!)
    for (const F of EmitBoxFacets(84.0, 0.60, 5.20, CourseMaterial.StudioWall))
        Facets.push({ Family: F.Family, Corners: F.Corners.map(([x, y, z]) => [x, y - 7.6, z]) });

    // Off-camera diagonal architectural shadow wall positioned behind/right of camera (never enters side-profile FOV)
    // to cast a crisp geometric diagonal shadow wedge across the right third of the white rear wall!
    const FinPts = [
        [-14.5, 4.5, 0.0], [-22.5, 14.0, 0.0], [-22.5, 14.0, 9.8], [-14.5, 4.5, 7.6]
    ];
    EmitQuad(Facets, FinPts[0], FinPts[1], FinPts[2], FinPts[3], CourseMaterial.StudioWall);
    EmitQuad(Facets, FinPts[1], FinPts[0], FinPts[3], FinPts[2], CourseMaterial.StudioWall);
    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                      VENUE 10: OBSIDIAN REVEAL ROTUNDA
//------------------------------------------------------------------------------------------------------------------------

// 📝 Dark stealth circular automotive reveal theater with massive coffered ceiling drum, perimeter LED & vertical monolith emblem!
function EmitRotundaHallFacets()
{
    const Facets = [];
    const Segs = 64;
    const WallR = 14.8;
    const OuterR = 15.6;
    const LowCeilZ = 4.55;
    const CofferR = 9.0;
    const HighCeilZ = 6.30;
    const Pt = (A, R, Z) => [R * Math.cos(A), R * Math.sin(A), Z];

    for (let S = 0; S < Segs; ++S)
    {
        const A0 = 2.0 * Math.PI * S / Segs;
        const A1 = 2.0 * Math.PI * (S + 1) / Segs;

        // Cylindrical matte dark charcoal auditorium wall
        EmitQuad(Facets, Pt(A1, WallR, 0.0), Pt(A0, WallR, 0.0), Pt(A0, WallR, LowCeilZ), Pt(A1, WallR, LowCeilZ), CourseMaterial.RotundaDarkWall);

        // Crisp continuous floor-seam white perimeter cove LED along the rear semicircle
        EmitQuad(Facets, Pt(A1, WallR - 0.05, 0.04), Pt(A0, WallR - 0.05, 0.04), Pt(A0, WallR - 0.05, 0.11), Pt(A1, WallR - 0.05, 0.11), CourseMaterial.RotundaLed);

        // Upper ceiling-wall seam hairline LED cove
        EmitQuad(Facets, Pt(A1, WallR - 0.06, LowCeilZ - 0.09), Pt(A0, WallR - 0.06, LowCeilZ - 0.09), Pt(A0, WallR - 0.06, LowCeilZ - 0.03), Pt(A1, WallR - 0.06, LowCeilZ - 0.03), CourseMaterial.RotundaLed);

        // Outer ceiling soffit ring (CofferR..OuterR at LowCeilZ — downward -Z normal!)
        EmitQuad(Facets, Pt(A0, CofferR, LowCeilZ), Pt(A1, CofferR, LowCeilZ), Pt(A1, OuterR, LowCeilZ), Pt(A0, OuterR, LowCeilZ), CourseMaterial.RotundaDarkWall);

        // Recessed circular ceiling coffer drum rising from LowCeilZ to HighCeilZ
        EmitQuad(Facets, Pt(A1, CofferR, LowCeilZ), Pt(A0, CofferR, LowCeilZ), Pt(A0, CofferR, HighCeilZ), Pt(A1, CofferR, HighCeilZ), CourseMaterial.RotundaDarkWall);

        // Razor-sharp circular white ceiling halo LED tucked around the coffer lip
        EmitQuad(Facets, Pt(A1, CofferR - 0.04, LowCeilZ + 0.05), Pt(A0, CofferR - 0.04, LowCeilZ + 0.05),
                         Pt(A0, CofferR - 0.04, LowCeilZ + 0.12), Pt(A1, CofferR - 0.04, LowCeilZ + 0.12), CourseMaterial.RotundaLed);

        // Recessed upper coffer dome ceiling cap (downward -Z normal!)
        Facets.push({ Corners: [[0.0, 0.0, HighCeilZ], Pt(A1, CofferR, HighCeilZ), Pt(A0, CofferR, HighCeilZ)], Family: CourseMaterial.RotundaDarkWall });
    }

    // Minimalist glowing vertical geometric Frontier monolith emblem centered on the rear wall (Y = -14.48 m)!
    const EY = -14.48;
    const Bar = (X0, Z0, X1, Z1) =>
        EmitQuad(Facets, [X0, EY, Z0], [X1, EY, Z0], [X1, EY, Z1], [X0, EY, Z1], CourseMaterial.RotundaLed);
    // Crisp tall hexagonal/octagonal open emblem frame (vertical reflection shimmers down the jet-black mirror floor!)
    Bar(-0.62, 1.48, -0.42, 3.22); // Left vertical pillar
    Bar( 0.42, 1.48,  0.62, 3.22); // Right vertical pillar
    // Upper chevron cap
    EmitQuad(Facets, [-0.62, EY, 3.22], [-0.42, EY, 3.22], [0.0, EY, 3.58], [0.0, EY, 3.78], CourseMaterial.RotundaLed);
    EmitQuad(Facets, [ 0.42, EY, 3.22], [ 0.62, EY, 3.22], [0.0, EY, 3.78], [0.0, EY, 3.58], CourseMaterial.RotundaLed);
    // Lower chevron base
    EmitQuad(Facets, [-0.62, EY, 1.48], [0.0, EY, 0.92], [0.0, EY, 1.12], [-0.42, EY, 1.48], CourseMaterial.RotundaLed);
    EmitQuad(Facets, [ 0.62, EY, 1.48], [0.42, EY, 1.48], [0.0, EY, 1.12], [0.0, EY, 0.92], CourseMaterial.RotundaLed);
    // Subtle center horizontal bar accent
    Bar(-0.26, 2.30, 0.26, 2.40);

    // Architectural triple miniature pin-spot clusters in the dark ceiling flanks (downward -Z normal!)
    for (const Side of [-1, 1])
        for (let P = 0; P < 3; ++P)
        {
            const PX = Side * (10.8 + P * 0.48), PY = -3.4 + P * 0.35;
            EmitQuad(Facets, [PX - 0.08, PY - 0.08, LowCeilZ - 0.02], [PX - 0.08, PY + 0.08, LowCeilZ - 0.02],
                             [PX + 0.08, PY + 0.08, LowCeilZ - 0.02], [PX + 0.08, PY - 0.08, LowCeilZ - 0.02], CourseMaterial.RotundaLed);
        }

    return Facets;
}

//------------------------------------------------------------------------------------------------------------------------
//                                      SINGLE-HERO CIRCULAR HYDRAULIC STAGE PLINTH
//------------------------------------------------------------------------------------------------------------------------

export function EmitLiftPlatformFacets(DeckMaterial = CourseMaterial.StudioTurntable)
{
    const Facets = [];
    const Segments = 64;
    const R = LobbyLiftConstants.LiftRadius;
    const TopZ = 0.0;
    const BotZ = TopZ - LobbyLiftConstants.PlatformThickness;
    const RimInner = R - 0.10;

    const Pt = (A, Rad, Z) => [Rad * Math.cos(A), Rad * Math.sin(A), Z];
    for (let S = 0; S < Segments; ++S)
    {
        const A0 = 2.0 * Math.PI * S / Segments;
        const A1 = 2.0 * Math.PI * (S + 1) / Segments;

        Facets.push({ Corners: [[0.0, 0.0, TopZ], Pt(A0, RimInner, TopZ), Pt(A1, RimInner, TopZ)], Family: DeckMaterial });
        EmitQuad(Facets, Pt(A0, RimInner, TopZ), Pt(A0, R, TopZ), Pt(A1, R, TopZ), Pt(A1, RimInner, TopZ), CourseMaterial.Surround);
        EmitQuad(Facets, Pt(A0, R, BotZ), Pt(A1, R, BotZ), Pt(A1, R, TopZ), Pt(A0, R, TopZ), CourseMaterial.StudioFrame);
        Facets.push({ Corners: [[0.0, 0.0, BotZ], Pt(A1, R, BotZ), Pt(A0, R, BotZ)], Family: CourseMaterial.StudioFrame });
    }
    return Facets;
}

export const LobbyShapes = Object.freeze({
    // Venue 1: Studio
    Cyclorama:         { Emit: () => EmitCycloramaFacets(), Voxel: 0.30, Cards: 9 },
    Softbox:           { Emit: () => EmitSoftboxFacets(), Voxel: 0.12, Cards: 6 },
    Ceiling:           { Emit: () => EmitBoxFacets(46.0, 46.0, 0.2, CourseMaterial.StudioFrame), Voxel: 0.35, Cards: 2 },
    TrussBeam:         { Emit: () => EmitBoxFacets(38.0, 0.28, 0.28, CourseMaterial.StudioFrame), Voxel: 0.3, Cards: 2 },
    TrussCross:        { Emit: () => EmitBoxFacets(0.18, 38.0, 0.18, CourseMaterial.StudioFrame), Voxel: 0.3, Cards: 2 },
    Cable:             { Emit: () => EmitBoxFacets(0.035, 0.035, 1.6, CourseMaterial.StudioFrame), Voxel: 0.1, Cards: 0, Distance: false },
    // Venue 2: White Monolith Plaza
    MonolithTower:     { Emit: () => EmitMonolithTowerFacets(), Voxel: 0.32, Cards: 6 },
    MonolithWedgeR:    { Emit: () => EmitMonolithWedgeFacets(38.0, 2.5, 13.5, -2.8, false), Voxel: 0.35, Cards: 6 },
    MonolithWedgeL:    { Emit: () => EmitMonolithWedgeFacets(36.0, 2.2, 10.5,  2.2, true),  Voxel: 0.35, Cards: 6 },
    MonolithBlade:     { Emit: () => EmitMonolithWedgeFacets(26.0, 1.8,  7.5, -1.6, false), Voxel: 0.32, Cards: 5 },
    // Venue 3: Bonneville Salt Flats
    SaltHorizon:       { Emit: () => EmitSaltHorizonFacets(), Voxel: 1.4, Cards: 0, Distance: false },
    // Venue 4: Desert Oculus Pavilion
    OculusRoof:        { Emit: () => EmitOculusRoofFacets(), Voxel: 0.32, Cards: 8 },
    OculusWalls:       { Emit: () => EmitOculusWallsFacets(), Voxel: 0.32, Cards: 7 },
    OculusCanyonVista: { Emit: () => EmitOculusCanyonVistaFacets(), Voxel: 0.85, Cards: 4, Distance: false },
    // Venue 5: Nocturne Lightwall
    NocturneLightwall: { Emit: () => EmitNocturneLightwallFacets(), Voxel: 0.18, Cards: 8 },
    NocturneMountains: { Emit: () => EmitNocturneMountainsFacets(), Voxel: 1.5, Cards: 0, Distance: false },
    // Venue 6: Linear Rib Studio
    RibStudioHall:     { Emit: () => EmitRibStudioHallFacets(),     Voxel: 0.58, Cards: 9 },
    // Venue 7: Coastal Canopy Pavilion
    CoastalCanopyArch: { Emit: () => EmitCoastalCanopyFacets(),     Voxel: 0.72, Cards: 8 },
    CoastalHeadland:   { Emit: () => EmitCoastalHeadlandFacets(),   Voxel: 1.40, Cards: 0, Distance: false },
    // Venue 8: Sky Courtyard Rotunda
    SkyCourtyardHall:  { Emit: () => EmitSkyCourtyardFacets(),      Voxel: 0.65, Cards: 9 },
    // Venue 9: Sculpted Horizon Gallery
    GalleryBackdrop:   { Emit: () => EmitGalleryBackdropFacets(),   Voxel: 0.65, Cards: 8 },
    // Venue 10: Obsidian Reveal Rotunda
    RotundaHall:       { Emit: () => EmitRotundaHallFacets(),       Voxel: 0.65, Cards: 9 },
    // Single-hero hydraulic showcase stage plinth
    LiftPlatform:      { Emit: () => EmitLiftPlatformFacets(CourseMaterial.StudioTurntable), Voxel: 0.18, Cards: 8 }
});

export function ResolveLobbyPlacements(VenueKey = "studio")
{
    const List = [];
    const Place = (Shape, Name, X, Y, Z = 0.0, Yaw = 0.0) =>
        List.push({ Shape, Name, Position: [X, Y, Z], Yaw, Material: -1, Collider: false });

    if (VenueKey === "monolith")
    {
        Place("MonolithTower",  "Monolith left tower",      -16.5,  10.5, 0.0,  0.14);
        Place("MonolithWedgeL", "Monolith left wing",       -24.0,  26.5, 0.0, -0.18);
        Place("MonolithWedgeR", "Monolith right fin",        21.5,  24.0, 0.0,  0.16);
        Place("MonolithBlade",  "Monolith right secondary",  25.5,  -8.0, 0.0, -0.42);
        Place("MonolithBlade",  "Monolith south wing",       -8.0, -28.0, 0.0,  0.22);
        return List;
    }

    if (VenueKey === "saltflats")
    {
        Place("SaltHorizon", "Bonneville horizon ridges", 0.0, 0.0, 0.0, 0.0);
        return List;
    }

    if (VenueKey === "oculus")
    {
        Place("OculusRoof",        "Pavilion sky oculus roof",  0.0, 0.0, 0.0, 0.0);
        Place("OculusWalls",       "Pavilion travertine walls", 0.0, 0.0, 0.0, 0.0);
        Place("OculusCanyonVista", "Red-rock desert canyon",    0.0, 0.0, 0.0, 0.0);
        return List;
    }

    if (VenueKey === "nocturne")
    {
        // Dramatic floating yellow lightwall positioned left-rear of hero car from front-3/4 view (Yaw ~ 0.64 rad)
        Place("NocturneLightwall", "Solar-yellow monolith lightwall", -2.6, -7.8, 0.0, -0.34);
        Place("NocturneMountains", "Twilight mountain horizon",        0.0,  0.0, 0.0,  0.0);
        return List;
    }

    if (VenueKey === "ribstudio")
    {
        Place("RibStudioHall", "Linear LED rib gallery", 0.0, 0.0, 0.0, 0.0);
        return List;
    }

    if (VenueKey === "coastal")
    {
        Place("CoastalCanopyArch", "Cantilevered coastal concrete canopy", 0.0, 0.0, 0.0, 0.0);
        Place("CoastalHeadland",   "Coastal azure bay & jagged headland",  0.0, 0.0, 0.0, 0.0);
        return List;
    }

    if (VenueKey === "courtyard")
    {
        Place("SkyCourtyardHall", "Minimalist sky courtyard rotunda", 0.0, 0.0, 0.0, 0.0);
        return List;
    }

    if (VenueKey === "gallery")
    {
        Place("GalleryBackdrop", "Sculpted white horizon gallery wall", 0.0, 0.0, 0.0, 0.0);
        return List;
    }

    if (VenueKey === "rotunda")
    {
        Place("RotundaHall", "Obsidian reveal theater rotunda", 0.0, 0.0, 0.0, 0.0);
        return List;
    }

    Place("Cyclorama", "Studio cyclorama", 0.0, 0.0, 0.0);
    Place("Ceiling", "Studio ceiling", 0.0, 0.0, 7.7);
    for (const Y of [-14.0, -7.0, 0.0, 7.0, 14.0])
        Place("TrussBeam", `Ceiling truss ${Y}`, 0.0, Y, 7.35);
    for (const X of [-14.0, 0.0, 14.0])
        Place("TrussCross", `Ceiling cross truss ${X}`, X, 0.0, 7.18);
    Place("Softbox", "Overhead softbox", 0.0, 0.0, 5.95);
    for (const X of [-10.5, 10.5])
        for (const Y of [-4.6, 4.6])
            Place("Cable", `Softbox cable ${X},${Y}`, X, Y, 6.10);
    return List;
}

export function ResolveLobbyStudioLights(VenueKey = "studio")
{
    if (VenueKey === "monolith")
    {
        return [
            { Type: LightType.Floodlight, Position: [0.0, 0.0, 9.5], Direction: [0.0, 0.0, -1.0],
              Colour: [0.90, 0.95, 1.0], Intensity: 220.0, Reach: 48.0,
              Params: [Math.cos(30.0 * Math.PI / 180.0), Math.cos(78.0 * Math.PI / 180.0), 0.0, 1.0] }
        ];
    }
    if (VenueKey === "saltflats")
    {
        return [];
    }
    if (VenueKey === "oculus")
    {
        const Lights = [{
            Type: LightType.Floodlight,
            Position: [0.0, 0.0, 5.6],
            Direction: [0.0, 0.0, -1.0],
            Colour: [1.0, 0.96, 0.90],
            Intensity: 340.0,
            Reach: 34.0,
            Params: [Math.cos(24.0 * Math.PI / 180.0), Math.cos(74.0 * Math.PI / 180.0), 0.0, 1.0]
        }];
        for (const [X, Y] of [[10.0, 10.0], [-10.0, 10.0], [-10.0, -10.0], [10.0, -10.0]])
        {
            Lights.push({
                Type: LightType.Omni,
                Position: [X, Y, 3.1],
                Direction: [0.0, 0.0, -1.0],
                Colour: [0.98, 0.91, 0.82],
                Intensity: 190.0,
                Reach: 28.0,
                Params: [0.0, 0.0, 0.0, 1.0]
            });
        }
        return Lights;
    }
    if (VenueKey === "nocturne")
    {
        // Dual rich solar-yellow directional washes across the 12.5 m monolith lightbox + cool twilight rim + soft overhead fill!
        return [
            { Type: LightType.Floodlight, Position: [-4.2, -6.2, 2.5], Direction: [0.36, 0.88, -0.30],
              Colour: [1.0, 0.90, 0.08], Intensity: 215.0, Reach: 34.0,
              Params: [Math.cos(34.0 * Math.PI / 180.0), Math.cos(78.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [1.2, -7.2, 2.6], Direction: [-0.14, 0.94, -0.30],
              Colour: [1.0, 0.88, 0.08], Intensity: 195.0, Reach: 34.0,
              Params: [Math.cos(34.0 * Math.PI / 180.0), Math.cos(78.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [6.5, 6.2, 5.2], Direction: [-0.62, -0.60, -0.50],
              Colour: [0.46, 0.64, 0.92], Intensity: 175.0, Reach: 32.0,
              Params: [Math.cos(28.0 * Math.PI / 180.0), Math.cos(72.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [0.0, 0.0, 6.4], Direction: [0.0, 0.0, -1.0],
              Colour: [0.62, 0.74, 0.94], Intensity: 120.0, Reach: 26.0,
              Params: [Math.cos(30.0 * Math.PI / 180.0), Math.cos(75.0 * Math.PI / 180.0), 0.0, 1.0] }
        ];
    }
    if (VenueKey === "ribstudio")
    {
        // Balanced overhead linear rib softbox wash + soft cove bounce (rich car reflections without clipping)
        return [
            { Type: LightType.Floodlight, Position: [0.0, 0.8, 4.45], Direction: [0.0, -0.05, -0.99],
              Colour: [1.0, 0.99, 0.96], Intensity: 215.0, Reach: 26.0,
              Params: [Math.cos(36.0 * Math.PI / 180.0), Math.cos(78.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [-4.8, 0.6, 4.40], Direction: [0.18, -0.10, -0.97],
              Colour: [1.0, 0.98, 0.94], Intensity: 145.0, Reach: 24.0,
              Params: [Math.cos(34.0 * Math.PI / 180.0), Math.cos(78.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [ 4.8, 0.6, 4.40], Direction: [-0.18, -0.10, -0.97],
              Colour: [1.0, 0.98, 0.94], Intensity: 145.0, Reach: 24.0,
              Params: [Math.cos(34.0 * Math.PI / 180.0), Math.cos(78.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [0.0, -1.8, 3.30], Direction: [0.0, -0.88, -0.47],
              Colour: [0.98, 0.98, 1.0], Intensity: 175.0, Reach: 22.0,
              Params: [Math.cos(40.0 * Math.PI / 180.0), Math.cos(82.0 * Math.PI / 180.0), 0.0, 1.0] }
        ];
    }
    if (VenueKey === "coastal")
    {
        // Crisp sunlit plaza bounce illuminating the hero car flank + cantilevered white canopy soffit
        return [
            { Type: LightType.Floodlight, Position: [1.5, -5.2, 3.6], Direction: [-0.10, 0.78, -0.62],
              Colour: [0.94, 0.97, 1.0], Intensity: 210.0, Reach: 34.0,
              Params: [Math.cos(36.0 * Math.PI / 180.0), Math.cos(80.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [-3.5, 4.5, 1.8], Direction: [0.0, -0.25, 0.96],
              Colour: [0.92, 0.95, 1.0], Intensity: 140.0, Reach: 26.0,
              Params: [Math.cos(45.0 * Math.PI / 180.0), Math.cos(84.0 * Math.PI / 180.0), 0.0, 1.0] }
        ];
    }
    if (VenueKey === "courtyard")
    {
        // Soft sunlit sky bounce from the wide circular courtyard oculus
        return [
            { Type: LightType.Floodlight, Position: [-1.5, 1.2, 6.2], Direction: [0.0, 0.0, -1.0],
              Colour: [0.92, 0.96, 1.0], Intensity: 185.0, Reach: 38.0,
              Params: [Math.cos(38.0 * Math.PI / 180.0), Math.cos(80.0 * Math.PI / 180.0), 0.0, 1.0] }
        ];
    }
    if (VenueKey === "gallery")
    {
        // Clean flagship studio fill balancing crisp sunlight on the cool brushed-silver floor
        return [
            { Type: LightType.Floodlight, Position: [0.0, 4.5, 6.8], Direction: [0.0, -0.45, -0.89],
              Colour: [0.94, 0.97, 1.0], Intensity: 225.0, Reach: 38.0,
              Params: [Math.cos(36.0 * Math.PI / 180.0), Math.cos(78.0 * Math.PI / 180.0), 0.0, 1.0] }
        ];
    }
    if (VenueKey === "rotunda")
    {
        // Dramatic focused overhead coffer reveal spotlight + subtle rear silhouette wash on dark curved rotunda wall
        return [
            { Type: LightType.Floodlight, Position: [0.0, 0.0, 5.95], Direction: [0.0, 0.0, -1.0],
              Colour: [0.92, 0.96, 1.0], Intensity: 310.0, Reach: 26.0,
              Params: [Math.cos(24.0 * Math.PI / 180.0), Math.cos(66.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Floodlight, Position: [0.0, 4.6, 3.8], Direction: [0.0, -0.78, -0.62],
              Colour: [0.78, 0.88, 1.0], Intensity: 140.0, Reach: 24.0,
              Params: [Math.cos(30.0 * Math.PI / 180.0), Math.cos(72.0 * Math.PI / 180.0), 0.0, 1.0] },
            { Type: LightType.Omni, Position: [0.0, -10.5, 1.8], Direction: [0.0, 0.0, -1.0],
              Colour: [0.85, 0.92, 1.0], Intensity: 110.0, Reach: 22.0,
              Params: [0.0, 0.0, 0.0, 1.0] }
        ];
    }

    const Lights = [{
        Type: LightType.Floodlight,
        Position: [0.0, 0.0, 5.85],
        Direction: [0.0, 0.0, -1.0],
        Colour: [0.96, 0.98, 1.0],
        Intensity: 950.0,
        Reach: 38.0,
        Params: [Math.cos(18.0 * Math.PI / 180.0), Math.cos(72.0 * Math.PI / 180.0), 0.0, 1.0]
    }];
    for (const [X, Y] of [[12.0, 0.0], [-12.0, 0.0], [0.0, 12.0], [0.0, -12.0]])
    {
        const Length = Math.hypot(X, Y, 2.2);
        Lights.push({
            Type: LightType.Floodlight,
            Position: [X * 0.5, Y * 0.5, 5.2],
            Direction: [X / Length, Y / Length, -2.2 / Length],
            Colour: [0.94, 0.96, 1.0],
            Intensity: 430.0,
            Reach: 44.0,
            Params: [Math.cos(22.0 * Math.PI / 180.0), Math.cos(72.0 * Math.PI / 180.0), 0.0, 1.0]
        });
    }
    return Lights;
}

//------------------------------------------------------------------------------------------------------------------------
//                                             SEAMLESS ARCHITECTURAL FLOORS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Pure, unbroken architectural floors for all 10 single-hero Lobby venues (ZERO dark floor pit holes!).
export function EmitLobbyGroundFacets(VenueKey = "studio")
{
    const Venue = ResolveLobbyVenueDescriptor(VenueKey);
    const Facets = [];
    const InnerHalf = 18.8;
    const OpenExpanse = VenueKey === "saltflats" || VenueKey === "monolith" || VenueKey === "nocturne"
                     || VenueKey === "coastal"   || VenueKey === "gallery";

    // ① Seamless outer expanse outside [-InnerHalf, InnerHalf]²
    const OuterMin  = OpenExpanse ? -140.0 : -60.0;
    const OuterMax  = OpenExpanse ?  140.0 : 160.0;
    const OuterMinY = OpenExpanse ? -140.0 : -60.0;
    // Coastal plaza ends cleanly at the bay water edge (Y = +34.0 m) so the azure sea opens uninterrupted!
    const OuterMaxY = VenueKey === "coastal" ? 34.0 : (OpenExpanse ? 140.0 : 60.0);
    const OuterMat  = VenueKey === "oculus" ? CourseMaterial.CanyonRock
                    : OpenExpanse ? Venue.FloorMaterial
                    : CourseMaterial.StudioSurround;
    const OuterZ    = OpenExpanse ? 0.0 : (VenueKey === "oculus" ? -2.2 : -0.02);

    const SurroundStrips = [
        [OuterMin,  OuterMinY, -InnerHalf, OuterMaxY],
        [InnerHalf, OuterMinY,  OuterMax,  OuterMaxY],
        [-InnerHalf, OuterMinY, InnerHalf, -InnerHalf],
        [-InnerHalf, InnerHalf, InnerHalf,  OuterMaxY]
    ];
    const TileStep = 20.0;
    for (const [AX0, AY0, AX1, AY1] of SurroundStrips)
    {
        for (let X = AX0; X < AX1 - 1e-4; X += TileStep)
        {
            const X1 = Math.min(X + TileStep, AX1);
            for (let Y = AY0; Y < AY1 - 1e-4; Y += TileStep)
            {
                const Y1 = Math.min(Y + TileStep, AY1);
                EmitQuad(Facets, [X, Y, OuterZ], [X1, Y, OuterZ], [X1, Y1, OuterZ], [X, Y1, OuterZ], OuterMat);
            }
        }
    }

    // ② Seamless unbroken inner floor grid [-InnerHalf, InnerHalf]² at Z = 0.0
    const InnerStep = 4.7;
    for (let X = -InnerHalf; X < InnerHalf - 1e-4; X += InnerStep)
    {
        const X1 = Math.min(X + InnerStep, InnerHalf);
        for (let Y = -InnerHalf; Y < InnerHalf - 1e-4; Y += InnerStep)
        {
            const Y1 = Math.min(Y + InnerStep, InnerHalf);
            EmitQuad(Facets, [X, Y, 0.0], [X1, Y, 0.0], [X1, Y1, 0.0], [X, Y1, 0.0], Venue.FloorMaterial);
        }
    }

    // ③ Center hero disc & signature architectural floor halo rings
    if (Venue.ShowCenterDisc)
    {
        const HeroR = VenueKey === "ribstudio" ? 3.55 : LobbyLiftConstants.HeroTurntableRadius;
        const HeroSegs = 96;
        const CollarMat = VenueKey === "oculus" ? CourseMaterial.StudioFrame
                        : VenueKey === "rotunda" ? CourseMaterial.RotundaDarkWall
                        : CourseMaterial.Surround;
        const CollarW = VenueKey === "oculus" ? 0.14 : 0.05;
        const P = (A, Rad, Z) => [Rad * Math.cos(A), Rad * Math.sin(A), Z];
        for (let S = 0; S < HeroSegs; ++S)
        {
            const A0 = 2.0 * Math.PI * S / HeroSegs;
            const A1 = 2.0 * Math.PI * (S + 1) / HeroSegs;
            Facets.push({ Corners: [[0.0, 0.0, 0.006], P(A0, HeroR, 0.006), P(A1, HeroR, 0.006)], Family: Venue.CenterDiscMaterial });
            EmitQuad(Facets, P(A0, HeroR, 0.009), P(A0, HeroR + CollarW, 0.009),
                             P(A1, HeroR + CollarW, 0.009), P(A1, HeroR, 0.009), CollarMat);

            // Linear Rib Studio: recessed subtle ice-blue perimeter halo ring near outer plinth lip!
            if (VenueKey === "ribstudio")
            {
                EmitQuad(Facets, P(A0, 3.16, 0.011), P(A0, 3.23, 0.011),
                                 P(A1, 3.23, 0.011), P(A1, 3.16, 0.011), CourseMaterial.RibPlinthGlow);
            }

            // Obsidian Reveal Rotunda: razor-sharp outer glowing white floor ring + crisp inner hairline ring!
            if (VenueKey === "rotunda")
            {
                EmitQuad(Facets, P(A0, 5.28, 0.010), P(A0, 5.36, 0.010),
                                 P(A1, 5.36, 0.010), P(A1, 5.28, 0.010), CourseMaterial.RotundaLed);
                EmitQuad(Facets, P(A0, HeroR + 0.06, 0.010), P(A0, HeroR + 0.11, 0.010),
                                 P(A1, HeroR + 0.11, 0.010), P(A1, HeroR + 0.06, 0.010), CourseMaterial.RotundaLed);
            }
        }
    }

    return Facets;
}
