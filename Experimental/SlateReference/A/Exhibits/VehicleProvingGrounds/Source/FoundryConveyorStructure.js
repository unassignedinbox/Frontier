//============================================================================================================================================
//                                                      FOUNDRYCONVEYORSTRUCTURE.JS
//============================================================================================================================================
// 🏭 Assembly Line · Conveyor Gauntlet ("foundry") — Industrial Conveyor Belt Race & Electromagnet Robot Level:
//    • 390-meter 3-lane variable-speed industrial conveyor gauntlet (X = -142 m Loading Dock → X = +240 m QC Finish Gate).
//    • Multi-speed belts (Counter-Current Surge, Variable Wave, Slow Inspection, Turbo Express Strip) with live scrolling
//      3D steel tread slats & processed industrial cargo (ferromagnetic Steel Crates, Gear Drums, glowing Molten Ingots).
//    • Hazard machinery engineered to slow/stop drivers: 4 Hydraulic Stamping Presses, 4 Pneumatic Side-Pusher Rejectors,
//      3 Spinning Rotary Sweeper Drums, and 1 Thermal Eddy-Brake Scanner Gate.
//    • 6 Articulated Electromagnet Robots (4 track-side robotic turrets + 2 overhead traveling bridge gantry cranes) with
//      glowing copper/cyan electromagnetic coils that visibly attract BOTH ferromagnetic conveyor parts AND the player's car!

import { CourseMaterial } from "./MaterialSpecification.js";
import { EmitQuad, EmitBoxFacets, EmitPrismFacets } from "./CourseStructure.js";

//------------------------------------------------------------------------------------------------------------------------
//                                           LEVEL BOUNDS, SPAWN & SECTOR LAYOUT
//------------------------------------------------------------------------------------------------------------------------

export const FoundryBounds = Object.freeze({ Minimum: [-168.0, -34.0], Maximum: [272.0, 34.0] });
export const FoundryGoalX  = 238.0; // [m] Quality Control Finish Gantry crossing plane

export const FoundrySpawn = Object.freeze({
    Position: [-132.0, 0.0, 0.0],
    Yaw:      0.0
});

export const FoundryZones = Object.freeze([
    { Name: "Sector 1 · Intake Speed-Belt Split",    Position: [-132.0, 0.0], Yaw: 0.0, Note: "Slow/fast surge belts, Press #1 & MagBot-1" },
    { Name: "Sector 2 · Magnetic Sorting Bay",       Position: [ -66.0, 0.0], Yaw: 0.0, Note: "MagBot-2 & Pushers lifting cargo + pulling cars!" },
    { Name: "Sector 3 · Stamping & Crusher Line",    Position: [  16.0, 0.0], Yaw: 0.0, Note: "Overhead Gantry MagBot-3, Presses & Sweepers" },
    { Name: "Sector 4 · Express Twin-Magnet Zone",   Position: [ 100.0, 0.0], Yaw: 0.0, Note: "Turbo Express strip, Eddy-Brake & MagBot-4/5" },
    { Name: "Sector 5 · Overdrive Finish Sprint",    Position: [ 184.0, 0.0], Yaw: 0.0, Note: "Counter-surge, MagBot-6 & QC Finish Gate!" }
]);

// 📝 Conveyor belt speed zones along X with 3 parallel lanes:
//    South (Y ∈ [-15.2, -4.25]), Center (Y ∈ [-4.25, +4.25]), North (Y ∈ [+4.25, +15.2]).
//    Negative SpeedX pushes BACKWARD (-X) against the driver; positive SpeedX boosts FORWARD (+X)!
export const FoundryConveyorSectors = Object.freeze([
    // Sector 1: Fast/Slow Counter-Belt center vs alternating side cargo lanes (active right under spawn!)
    {
        Key: "intake", X0: -152.0, X1: -72.0,
        WavePeriod: 6.0, WavePhase: 0.0,
        CenterBase: -7.2, CenterAmp: 3.6,   // oscillates -3.6 m/s (slow) ↔ -10.8 m/s (fast counter surge!)
        SideBase:   -2.2, SideAmp:   4.2    // alternates slow counter (-6.4) ↔ mild forward assist (+2.0)!
    },
    // Sector 2: Magnetic Sorting Bay — opposing north/south speeds around MagBots #1 & #2
    {
        Key: "sorting", X0: -72.0, X1: 12.0,
        WavePeriod: 7.2, WavePhase: 1.1,
        CenterBase: -5.8, CenterAmp: 3.4,
        SideBase:   -4.2, SideAmp:   3.2
    },
    // Sector 3: Heavy Stamping & Crusher Line — synchronized slow-inspection ↔ high-speed reject wave!
    {
        Key: "stamping", X0: 12.0, X1: 96.0,
        WavePeriod: 6.5, WavePhase: 2.3,
        CenterBase: -7.8, CenterAmp: 4.6,   // oscillates -3.2 m/s ↔ -12.4 m/s OVERDRIVE REJECT!
        SideBase:   -5.2, SideAmp:   3.4
    },
    // Sector 4: Express Twin-Magnet Zone — tempting TURBO FORWARD Center Strip guarded by twin magnets & brake gate!
    {
        Key: "express", X0: 96.0, X1: 178.0,
        WavePeriod: 5.8, WavePhase: 0.5,
        CenterBase:  8.2, CenterAmp: 2.2,   // +6.0 m/s ↔ +10.4 m/s EXPRESS BOOST!
        SideBase:   -4.8, SideAmp:   2.4    // safe outer lanes fight moderate backward belt
    },
    // Sector 5: Final Overdrive Counter-Belt Sprint into QC Finish Gate!
    {
        Key: "overdrive", X0: 178.0, X1: 236.0,
        WavePeriod: 5.2, WavePhase: 1.7,
        CenterBase: -8.6, CenterAmp: 3.4,   // heavy -5.2 ↔ -12.0 m/s final test!
        SideBase:   -6.6, SideAmp:   3.0
    }
]);

// 📝 Pure deterministic conveyor belt velocity query at world (X, Y) and simulation time Clock [s].
export function QueryFoundryConveyorVelocity(X, Y, Clock = 0.0, SpeedMultiplier = 1.0)
{
    if (Math.abs(Y) > 15.2 || X < -152.0 || X > 236.0)
        return { OnBelt: false, SpeedX: 0.0, Lane: "dock", PhaseLabel: "FACTORY DOCK", Surge: 0.0 };

    const Lane = Math.abs(Y) <= 4.25 ? "center" : (Y > 0.0 ? "north" : "south");
    let Sector = FoundryConveyorSectors[0];
    for (const S of FoundryConveyorSectors)
    {
        if (X >= S.X0 && X <= S.X1)
        {
            Sector = S;
            break;
        }
    }

    const Omega = (2.0 * Math.PI) / Sector.WavePeriod;
    const PhaseOffset = Lane === "north" ? 1.4 : (Lane === "south" ? -1.4 : 0.0);
    const Wave = Math.sin(Clock * Omega + Sector.WavePhase + PhaseOffset); // [-1, +1]
    const RawSpeed = Lane === "center"
        ? (Sector.CenterBase + Sector.CenterAmp * Wave)
        : (Sector.SideBase   + Sector.SideAmp   * Wave);
    const SpeedX = RawSpeed * SpeedMultiplier;

    let PhaseLabel = "STEADY BELT";
    if (SpeedX >= 4.5)       PhaseLabel = "EXPRESS BOOST";
    else if (SpeedX > 0.8)   PhaseLabel = "FORWARD ASSIST";
    else if (SpeedX <= -9.0) PhaseLabel = "OVERDRIVE COUNTER-SURGE";
    else if (SpeedX <= -5.0) PhaseLabel = "FAST COUNTER-BELT";
    else                     PhaseLabel = "SLOW CRAWL";

    return {
        OnBelt: true,
        SectorKey: Sector.Key,
        Lane,
        SpeedX,
        Surge: Wave,
        PhaseLabel
    };
}

//------------------------------------------------------------------------------------------------------------------------
//                                  MACHINES, ELECTROMAGNET ROBOTS & HAZARD SITES
//------------------------------------------------------------------------------------------------------------------------

// 📝 6 Articulated Electromagnet Robots (4 track-side robotic turrets + 2 overhead traveling bridge cranes).
//    Each cycles through IDLE → CHARGING → ACTIVE MAGNETIC PULL (attracting ferromagnetic parts + vehicles!) → RELEASE.
export const FoundryMagRobotSites = Object.freeze([
    { Id: "MagBot-1", Kind: "turret", X:  -92.0, Y:  13.8, Side:  1, ReachY:  5.8, BaseZ: 0.0, HeadZ: 5.4, PullRadius: 17.5, PullStrength: 21.5, Cycle: 7.4, Phase: 2.4, ActiveWindow: [0.22, 0.85] },
    { Id: "MagBot-2", Kind: "turret", X:  -22.0, Y: -13.8, Side: -1, ReachY: -5.8, BaseZ: 0.0, HeadZ: 5.4, PullRadius: 17.5, PullStrength: 22.0, Cycle: 7.0, Phase: 3.6, ActiveWindow: [0.25, 0.84] },
    { Id: "MagBot-3", Kind: "gantry", X:   48.0, Y:   0.0, Side:  0, SweepY:  9.2, BaseZ: 7.6, HeadZ: 5.2, PullRadius: 16.5, PullStrength: 23.0, Cycle: 6.8, Phase: 1.5, ActiveWindow: [0.25, 0.82] },
    { Id: "MagBot-4", Kind: "turret", X:  122.0, Y:  13.6, Side:  1, ReachY:  4.8, BaseZ: 0.0, HeadZ: 5.3, PullRadius: 17.8, PullStrength: 24.0, Cycle: 6.4, Phase: 0.0, ActiveWindow: [0.28, 0.80] },
    { Id: "MagBot-5", Kind: "turret", X:  148.0, Y: -13.6, Side: -1, ReachY: -4.8, BaseZ: 0.0, HeadZ: 5.3, PullRadius: 17.8, PullStrength: 24.0, Cycle: 6.4, Phase: 3.2, ActiveWindow: [0.28, 0.80] },
    { Id: "MagBot-6", Kind: "gantry", X:  198.0, Y:   0.0, Side:  0, SweepY:  9.8, BaseZ: 7.6, HeadZ: 5.2, PullRadius: 18.0, PullStrength: 25.5, Cycle: 6.0, Phase: 0.9, ActiveWindow: [0.25, 0.84] }
]);

// 📝 4 Heavy Hydraulic Stamping Presses cycling down onto glowing floor danger pads!
export const FoundryPressSites = Object.freeze([
    { Id: "Press-1", X: -108.0, Y:  0.0, HalfX: 2.3, HalfY: 3.7, Period: 3.4, Phase: 0.8 },
    { Id: "Press-2", X:   32.0, Y:  4.6, HalfX: 2.3, HalfY: 3.7, Period: 3.1, Phase: 1.1 },
    { Id: "Press-3", X:   66.0, Y: -4.6, HalfX: 2.3, HalfY: 3.7, Period: 3.1, Phase: 2.4 },
    { Id: "Press-4", X:  214.0, Y:  0.0, HalfX: 2.4, HalfY: 3.9, Period: 2.8, Phase: 0.6 }
]);

// 📝 4 Pneumatic Side-Pusher Rejector Pistons punching laterally across outer/center lanes!
export const FoundryPusherSites = Object.freeze([
    { Id: "Pusher-1", X: -76.0, Side: -1, BaseY: -14.2, Stroke: 8.4, Period: 3.8, Phase: 0.3 },
    { Id: "Pusher-2", X: -36.0, Side:  1, BaseY:  14.2, Stroke: 8.4, Period: 3.6, Phase: 1.9 },
    { Id: "Pusher-3", X: 110.0, Side: -1, BaseY: -14.2, Stroke: 8.8, Period: 3.4, Phase: 0.7 },
    { Id: "Pusher-4", X: 164.0, Side:  1, BaseY:  14.2, Stroke: 8.8, Period: 3.4, Phase: 2.2 }
]);

// 📝 3 Spinning Industrial Rotary Sweeper Drum Gates!
export const FoundrySweeperSites = Object.freeze([
    { Id: "Sweeper-1", X:  52.0, Y: -4.2, Radius: 4.5, Omega:  2.2, Phase: 0.0 },
    { Id: "Sweeper-2", X:  84.0, Y:  4.2, Radius: 4.5, Omega: -2.4, Phase: 1.4 },
    { Id: "Sweeper-3", X: 225.0, Y:  0.0, Radius: 4.8, Omega:  2.6, Phase: 0.8 }
]);

// 📝 1 Thermal Eddy-Current Brake Scanner Gate guarding the Center Express Strip!
export const FoundryScannerSites = Object.freeze([
    { Id: "Scanner-1", X: 136.0, Y: 0.0, HalfX: 2.4, HalfY: 4.2, Period: 5.4, Phase: 0.4 }
]);

export const FoundryPoolCounts = Object.freeze({
    BeltSlats:     72,                           // 24 scrolling ribbed steel cleats × 3 conveyor lanes
    CargoCrates:   18,
    CargoDrums:    14,
    CargoIngots:   10,
    PressRams:     FoundryPressSites.length,     // 4
    DangerPads:    FoundryPressSites.length,     // 4
    PusherRams:    FoundryPusherSites.length,    // 4
    SweeperBars:   FoundrySweeperSites.length,   // 3
    MagBases:      FoundryMagRobotSites.length,  // 6
    MagArms:       FoundryMagRobotSites.length,  // 6
    MagHeads:      FoundryMagRobotSites.length,  // 6
    MagRings:      FoundryMagRobotSites.length,  // 6
    ScannerBeams:  FoundryScannerSites.length    // 1
});

//------------------------------------------------------------------------------------------------------------------------
//                                        WATERTIGHT PBR SHAPE BUILDERS
//------------------------------------------------------------------------------------------------------------------------

function ShiftFacets(Facets, DX, DY, DZ)
{
    return Facets.map((F) => ({
        Family: F.Family,
        Corners: F.Corners.map(([x, y, z]) => [x + DX, y + DY, z + DZ])
    }));
}

// 📝 Factory structural steel arch portal framing the mega-conveyor hall.
function EmitFoundryArchFacets()
{
    const Facets = [];
    // Left & right structural steel I-columns (outside guardrails at Y = ±17.8 m)
    Facets.push(...ShiftFacets(EmitBoxFacets(1.4, 1.4, 11.4, CourseMaterial.SteelDark), 0.0, -17.8, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(1.4, 1.4, 11.4, CourseMaterial.SteelDark), 0.0,  17.8, 0.0));
    // Safety yellow lower collision plinth bumpers on columns
    Facets.push(...ShiftFacets(EmitBoxFacets(1.7, 1.7, 1.6, CourseMaterial.FoundryYellow), 0.0, -17.8, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(1.7, 1.7, 1.6, CourseMaterial.FoundryYellow), 0.0,  17.8, 0.0));
    // Overhead steel roof truss beam spanning 37 m across the hall
    Facets.push(...ShiftFacets(EmitBoxFacets(1.2, 37.2, 1.1, CourseMaterial.SteelDark), 0.0, 0.0, 10.6));
    // Warm overhead high-bay industrial light fixture bar
    Facets.push(...ShiftFacets(EmitBoxFacets(0.45, 22.0, 0.22, CourseMaterial.LampWarm), 0.0, 0.0, 10.35));
    return Facets;
}

// 📝 Heavy yellow-and-graphite conveyor containment guardrail segment (12 m long along X).
function EmitFoundryRailFacets()
{
    const Facets = [];
    Facets.push(...ShiftFacets(EmitBoxFacets(12.0, 0.72, 0.85, CourseMaterial.SteelDark), 0.0, 0.0, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(11.8, 0.82, 0.26, CourseMaterial.FoundryYellow), 0.0, 0.0, 0.85));
    // Glowing industrial strip inset along the rail face
    Facets.push(...ShiftFacets(EmitBoxFacets(8.5, 0.86, 0.10, CourseMaterial.LampWarm), 0.0, 0.0, 0.48));
    return Facets;
}

// 📝 Towering static steel frame for each Hydraulic Stamping Press (ram slides dynamically inside!).
function EmitFoundryPressFrameFacets()
{
    const Facets = [];
    // Twin heavy yellow/dark hydraulic legs straddling one lane (width 8.6 m along Y)
    Facets.push(...ShiftFacets(EmitBoxFacets(4.2, 0.95, 7.6, CourseMaterial.FoundryYellow), 0.0, -4.3, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(4.2, 0.95, 7.6, CourseMaterial.FoundryYellow), 0.0,  4.3, 0.0));
    // Massive overhead hydraulic crown header
    Facets.push(...ShiftFacets(EmitBoxFacets(4.8, 9.8, 1.8, CourseMaterial.SteelDark), 0.0, 0.0, 7.4));
    Facets.push(...ShiftFacets(EmitBoxFacets(5.0, 9.6, 0.35, CourseMaterial.FoundryHazardRed), 0.0, 0.0, 7.2));
    return Facets;
}

// 📝 Overhead steel bridge track spanning the conveyor for Traveling Gantry MagBots (#3 & #6).
function EmitFoundryBridgeTrackFacets()
{
    const Facets = [];
    Facets.push(...ShiftFacets(EmitBoxFacets(2.0, 1.3, 9.6, CourseMaterial.FoundryYellow), 0.0, -16.2, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.0, 1.3, 9.6, CourseMaterial.FoundryYellow), 0.0,  16.2, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.4, 34.0, 1.1, CourseMaterial.FoundryYellow), 0.0, 0.0, 8.6));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.6, 33.4, 0.24, CourseMaterial.FoundryMagCyan), 0.0, 0.0, 8.4));
    return Facets;
}

// 📝 Thermal Eddy-Current Brake Scanner Arch spanning the Center Express Strip!
function EmitFoundryScannerArchFacets()
{
    const Facets = [];
    Facets.push(...ShiftFacets(EmitBoxFacets(2.8, 0.85, 6.2, CourseMaterial.SteelDark), 0.0, -4.4, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.8, 0.85, 6.2, CourseMaterial.SteelDark), 0.0,  4.4, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(3.2, 9.8, 1.1, CourseMaterial.SteelDark), 0.0, 0.0, 6.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(3.3, 9.2, 0.26, CourseMaterial.FoundryCopper), 0.0, 0.0, 5.85));
    return Facets;
}

// 📝 Grand Quality Control Dispatch Finish Gantry at X = +240 m!
function EmitFoundryFinishGantryFacets()
{
    const Facets = [];
    Facets.push(...ShiftFacets(EmitBoxFacets(2.4, 2.0, 10.2, CourseMaterial.SteelDark), 0.0, -15.6, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.4, 2.0, 10.2, CourseMaterial.SteelDark), 0.0,  15.6, 0.0));
    // Glowing emerald victory strips running up both pillars
    Facets.push(...ShiftFacets(EmitBoxFacets(2.55, 0.55, 9.6, CourseMaterial.FoundryBoostGreen), 0.0, -14.7, 0.3));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.55, 0.55, 9.6, CourseMaterial.FoundryBoostGreen), 0.0,  14.7, 0.3));
    // Overhead massive checkered finish header beam
    Facets.push(...ShiftFacets(EmitBoxFacets(2.8, 33.4, 2.2, CourseMaterial.SteelDark), 0.0, 0.0, 9.4));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.95, 31.6, 0.55, CourseMaterial.FoundryBoostGreen), 0.0, 0.0, 9.95));
    // Crisp checkered white/black blocks across the beam face
    for (let I = -6; I <= 6; ++I)
    {
        const Mat = (I & 1) === 0 ? CourseMaterial.StudioWall : CourseMaterial.SteelDark;
        Facets.push(...ShiftFacets(EmitBoxFacets(2.96, 2.1, 0.72, Mat), 0.0, I * 2.2, 10.72));
    }
    return Facets;
}

// --- Dynamic Animated Shapes ---

// 📝 Moving heavy brushed-steel conveyor tread cleat with raised high-contrast traction rib (visibly shows fast/slow/reverse speed!).
function EmitBeltSlatFacets()
{
    const Facets = [];
    Facets.push(...EmitBoxFacets(0.88, 7.5, 0.08, CourseMaterial.FoundrySlat));
    Facets.push(...ShiftFacets(EmitBoxFacets(0.24, 7.1, 0.04, CourseMaterial.FoundryYellow), 0.0, 0.0, 0.08));
    return Facets;
}

// 📝 Ferromagnetic stamped steel industrial engine crate (processed on conveyor & attracted by MagBots!).
function EmitCargoCrateFacets()
{
    const Facets = [];
    Facets.push(...EmitBoxFacets(2.2, 1.9, 1.55, CourseMaterial.FoundryCargoSteel));
    // Safety yellow reinforced industrial corner bands
    Facets.push(...ShiftFacets(EmitBoxFacets(2.3, 2.0, 0.24, CourseMaterial.FoundryYellow), 0.0, 0.0, 0.12));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.3, 2.0, 0.24, CourseMaterial.FoundryYellow), 0.0, 0.0, 1.24));
    return Facets;
}

// 📝 Ferromagnetic cylindrical steel gear/cable drum (rolls/rides conveyor & lifts into MagBot coils!).
function EmitCargoDrumFacets()
{
    const Facets = [];
    Facets.push(...EmitPrismFacets(0.88, 1.45, 12, CourseMaterial.FoundryCargoSteel));
    Facets.push(...ShiftFacets(EmitPrismFacets(0.95, 0.22, 12, CourseMaterial.FoundryCopper), 0.0, 0.0, 0.62));
    return Facets;
}

// 📝 Glowing hot freshly-forged molten steel ingot processed under the stamping presses!
function EmitCargoIngotFacets()
{
    const Facets = [];
    Facets.push(...EmitBoxFacets(2.5, 1.35, 0.72, CourseMaterial.SteelDark));
    Facets.push(...ShiftFacets(EmitBoxFacets(2.1, 1.02, 0.36, CourseMaterial.FoundryMolten), 0.0, 0.0, 0.56));
    return Facets;
}

// 📝 Heavy stamping press moving hammer ram!
function EmitPressRamFacets()
{
    const Facets = [];
    Facets.push(...EmitBoxFacets(3.8, 7.2, 1.5, CourseMaterial.FoundryYellow));
    Facets.push(...ShiftFacets(EmitBoxFacets(3.95, 7.35, 0.35, CourseMaterial.SteelDark), 0.0, 0.0, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(1.4, 1.4, 3.8, CourseMaterial.FoundrySlat), 0.0, -2.0, 1.5));
    Facets.push(...ShiftFacets(EmitBoxFacets(1.4, 1.4, 3.8, CourseMaterial.FoundrySlat), 0.0,  2.0, 1.5));
    return Facets;
}

// 📝 Pneumatic hydraulic side-pusher sorting piston head & rod!
function EmitPusherRamFacets()
{
    const Facets = [];
    // Broad pusher bumper shield face (aligned along X, pushing along Y)
    Facets.push(...ShiftFacets(EmitBoxFacets(3.4, 0.75, 1.55, CourseMaterial.FoundryYellow), 0.0, 0.0, 0.25));
    Facets.push(...ShiftFacets(EmitBoxFacets(3.2, 0.82, 0.25, CourseMaterial.FoundryHazardRed), 0.0, 0.0, 0.90));
    // Telescoping chrome hydraulic cylinder shaft extending behind bumper
    Facets.push(...ShiftFacets(EmitBoxFacets(0.55, 8.2, 0.55, CourseMaterial.FoundrySlat), 0.0, -4.2, 0.78));
    return Facets;
}

// 📝 Spinning rotary sweeper drum arm!
function EmitSweeperBarFacets()
{
    const Facets = [];
    // Central armored rotor hub
    Facets.push(...EmitPrismFacets(0.85, 2.2, 12, CourseMaterial.SteelDark));
    // Twin opposing heavy hazard sweeper paddles sweeping 9.0 m diameter!
    Facets.push(...ShiftFacets(EmitBoxFacets(8.8, 0.65, 0.92, CourseMaterial.FoundryYellow), 0.0, 0.0, 0.42));
    Facets.push(...ShiftFacets(EmitBoxFacets(8.9, 0.70, 0.20, CourseMaterial.FoundryHazardRed), 0.0, 0.0, 0.82));
    return Facets;
}

// 📝 Electromagnet Robot heavy armored rotating turret base / overhead gantry carriage!
function EmitMagBaseFacets()
{
    const Facets = [];
    Facets.push(...EmitPrismFacets(1.85, 2.2, 12, CourseMaterial.SteelDark));
    Facets.push(...ShiftFacets(EmitPrismFacets(1.45, 1.4, 12, CourseMaterial.FoundryYellow), 0.0, 0.0, 2.2));
    return Facets;
}

// 📝 Electromagnet Robot articulated boom arm (extends from origin along +X toward its magnet head!).
function EmitMagArmFacets()
{
    const Facets = [];
    Facets.push(...ShiftFacets(EmitBoxFacets(6.8, 0.85, 0.95, CourseMaterial.FoundryYellow), 3.4, 0.0, 0.0));
    Facets.push(...ShiftFacets(EmitBoxFacets(6.4, 0.92, 0.24, CourseMaterial.SteelDark), 3.4, 0.0, 0.35));
    return Facets;
}

// 📝 Electromagnet Bell Head: heavy industrial steel shroud + copper electromagnet winding + glowing cyan core!
function EmitMagHeadFacets()
{
    const Facets = [];
    // Upper armored magnet housing
    Facets.push(...ShiftFacets(EmitPrismFacets(2.05, 1.15, 16, CourseMaterial.SteelDark), 0.0, 0.0, 0.50));
    // Bare wound electromagnetic copper coil ring
    Facets.push(...ShiftFacets(EmitPrismFacets(2.25, 0.50, 16, CourseMaterial.FoundryCopper), 0.0, 0.0, 0.14));
    // Glowing electric-cyan active magnetic emitter core underneath!
    Facets.push(...ShiftFacets(EmitPrismFacets(1.85, 0.28, 16, CourseMaterial.FoundryMagCyan), 0.0, 0.0, -0.08));
    return Facets;
}

// 📝 Pulsing electromagnetic capture field ring projected cleanly on the conveyor beneath an active MagBot!
function EmitMagFieldRingFacets()
{
    const Facets = [];
    const Segs = 28;
    const RIn = 4.2, ROut = 4.85, Z0 = 0.01, Z1 = 0.08;
    const P = (A, R, Z) => [R * Math.cos(A), R * Math.sin(A), Z];
    for (let S = 0; S < Segs; ++S)
    {
        const A0 = 2.0 * Math.PI * S / Segs;
        const A1 = 2.0 * Math.PI * (S + 1) / Segs;
        // Top (+Z) and Bottom (-Z) rings
        EmitQuad(Facets, P(A0, RIn, Z1), P(A0, ROut, Z1), P(A1, ROut, Z1), P(A1, RIn, Z1), CourseMaterial.FoundryMagCyan);
        EmitQuad(Facets, P(A0, RIn, Z0), P(A1, RIn, Z0), P(A1, ROut, Z0), P(A0, ROut, Z0), CourseMaterial.FoundryMagCyan);
        // Outer (+R) and Inner (-R) closed cylinder walls
        EmitQuad(Facets, P(A0, ROut, Z0), P(A1, ROut, Z0), P(A1, ROut, Z1), P(A0, ROut, Z1), CourseMaterial.FoundryMagCyan);
        EmitQuad(Facets, P(A0, RIn, Z0), P(A0, RIn, Z1), P(A1, RIn, Z1), P(A1, RIn, Z0), CourseMaterial.FoundryMagCyan);
    }
    return Facets;
}

// 📝 Glowing stamping press floor danger pad (signals imminent press slam!).
function EmitDangerPadFacets()
{
    return EmitBoxFacets(4.6, 7.6, 0.07, CourseMaterial.FoundryHazardRed);
}

// 📝 Thermal scanner plasma curtain beam (flips RED / GREEN above the Express Center Strip!).
function EmitScannerBeamFacets()
{
    return EmitBoxFacets(0.45, 7.8, 0.32, CourseMaterial.FoundryHazardRed);
}

//------------------------------------------------------------------------------------------------------------------------
//                                             SHAPE REGISTRY & PLACEMENTS
//------------------------------------------------------------------------------------------------------------------------

export const FoundryStaticShapes = Object.freeze({
    FoundryArch:         { Emit: () => EmitFoundryArchFacets(),         Voxel: 0.45, Cards: 5 },
    FoundryRail:         { Emit: () => EmitFoundryRailFacets(),         Voxel: 0.22, Cards: 6 },
    FoundryPressFrame:   { Emit: () => EmitFoundryPressFrameFacets(),   Voxel: 0.32, Cards: 6 },
    FoundryBridgeTrack:  { Emit: () => EmitFoundryBridgeTrackFacets(),  Voxel: 0.42, Cards: 6 },
    FoundryScannerArch:  { Emit: () => EmitFoundryScannerArchFacets(),  Voxel: 0.30, Cards: 6 },
    FoundryFinishGantry: { Emit: () => EmitFoundryFinishGantryFacets(), Voxel: 0.42, Cards: 6 }
});

export const FoundryDynamicShapes = Object.freeze({
    FoundryBeltSlat:     { Emit: () => EmitBeltSlatFacets(),     Voxel: 0.16, Cards: 4, Distance: false },
    FoundryCargoCrate:   { Emit: () => EmitCargoCrateFacets(),   Voxel: 0.14, Cards: 8, Distance: false },
    FoundryCargoDrum:    { Emit: () => EmitCargoDrumFacets(),    Voxel: 0.14, Cards: 8, Distance: false },
    FoundryCargoIngot:   { Emit: () => EmitCargoIngotFacets(),   Voxel: 0.14, Cards: 8, Distance: false },
    FoundryPressRam:     { Emit: () => EmitPressRamFacets(),     Voxel: 0.22, Cards: 6, Distance: false },
    FoundryPusherRam:    { Emit: () => EmitPusherRamFacets(),    Voxel: 0.22, Cards: 6, Distance: false },
    FoundrySweeperBar:   { Emit: () => EmitSweeperBarFacets(),   Voxel: 0.22, Cards: 6, Distance: false },
    FoundryMagBase:      { Emit: () => EmitMagBaseFacets(),      Voxel: 0.24, Cards: 6, Distance: false },
    FoundryMagArm:       { Emit: () => EmitMagArmFacets(),       Voxel: 0.22, Cards: 6, Distance: false },
    FoundryMagHead:      { Emit: () => EmitMagHeadFacets(),      Voxel: 0.16, Cards: 8, Distance: false },
    FoundryMagFieldRing: { Emit: () => EmitMagFieldRingFacets(), Voxel: 0.24, Cards: 4, Distance: false },
    FoundryDangerPad:    { Emit: () => EmitDangerPadFacets(),    Voxel: 0.20, Cards: 4, Distance: false },
    FoundryScannerBeam:  { Emit: () => EmitScannerBeamFacets(),  Voxel: 0.16, Cards: 4, Distance: false }
});

const StaticColliderSet = new Set(["FoundryRail", "FoundryPressFrame", "FoundryScannerArch", "FoundryFinishGantry"]);

export function ResolveFoundryPlacements()
{
    const List = [];
    const Place = (Shape, Name, X, Y, Z = 0.0, Yaw = 0.0, Collider = StaticColliderSet.has(Shape)) =>
        List.push({ Shape, Name, Position: [X, Y, Z], Yaw, Material: -1, Collider });

    // ① Monumental mega-foundry overhead structural arches every 28 m down the 390 m assembly line
    for (let X = -146.0; X <= 246.0; X += 28.0)
        Place("FoundryArch", `Foundry Truss Arch ${X}`, X, 0.0, 0.0, 0.0, false);

    // ② Continuous conveyor safety containment guardrails along both sides (Y = ±15.2 m, Collider = true)
    for (let X = -148.0; X <= 248.0; X += 12.0)
    {
        Place("FoundryRail", `South Conveyor Guardrail ${X}`, X, -15.2, 0.0, 0.0, true);
        Place("FoundryRail", `North Conveyor Guardrail ${X}`, X,  15.2, 0.0, 0.0, true);
    }

    // ③ Static structural frames for the 4 Hydraulic Stamping Presses (do not block drivable passage under the ram!)
    for (const P of FoundryPressSites)
        Place("FoundryPressFrame", `${P.Id} Frame`, P.X, P.Y, 0.0, 0.0, false);

    // ④ Overhead traveling gantry crane bridge tracks for MagBot #3 & #6
    for (const M of FoundryMagRobotSites)
        if (M.Kind === "gantry")
            Place("FoundryBridgeTrack", `${M.Id} Bridge Crane Track`, M.X, 0.0, 0.0, 0.0, false);

    // ⑤ Thermal Eddy-Brake Scanner Arch spanning Center Express Strip
    for (const S of FoundryScannerSites)
        Place("FoundryScannerArch", `${S.Id} Arch`, S.X, S.Y, 0.0, 0.0, false);

    // ⑥ Grand Quality Control Finish Gantry at X = +240 m
    Place("FoundryFinishGantry", "Quality Control Finish Gantry", 240.0, 0.0, 0.0, 0.0, false);

    return List;
}

export function ResolveFoundrySurfaces()   { return []; }
export function ResolveFoundryCones()      { return []; }
export function ResolveFoundryBreakables() { return []; }

// 📝 Unbroken multi-lane industrial conveyor deck + factory floor + glowing lane dividers & start/finish markings!
export function EmitFoundryGroundFacets()
{
    const Facets = [];

    // ① Sealed industrial concrete mega-factory surround floor ([-170..274] × [-36..36])
    const TileX = 24.0, TileY = 18.0;
    for (let X = -170.0; X < 274.0; X += TileX)
    {
        const X1 = Math.min(X + TileX, 274.0);
        for (let Y = -36.0; Y < 36.0; Y += TileY)
        {
            const Y1 = Math.min(Y + TileY, 36.0);
            EmitQuad(Facets, [X, Y, 0.0], [X1, Y, 0.0], [X1, Y1, 0.0], [X, Y1, 0.0], CourseMaterial.FoundryFloor);
        }
    }

    // ② Three distinct dark ribbed rubber conveyor belt lanes (X = -152..236 m, slightly proud at Z = 0.01 m)
    const Lanes = [
        { Name: "south",  Y0: -12.8, Y1: -4.6 },
        { Name: "center", Y0:  -3.9, Y1:  3.9 },
        { Name: "north",  Y0:   4.6, Y1: 12.8 }
    ];
    for (const Lane of Lanes)
    {
        for (let X = -152.0; X < 236.0; X += 12.0)
        {
            const X1 = Math.min(X + 12.0, 236.0);
            EmitQuad(Facets, [X, Lane.Y0, 0.012], [X1, Lane.Y0, 0.012], [X1, Lane.Y1, 0.012], [X, Lane.Y1, 0.012], CourseMaterial.FoundryBelt);
        }
    }

    // ③ Illuminated conveyor lane curb strips & Sector 4 glowing green Turbo Express guide rails!
    for (let X = -152.0; X < 236.0; X += 14.0)
    {
        const X1 = Math.min(X + 12.5, 236.0);
        const InExpress = X >= 94.0 && X <= 176.0;
        const InOverdrive = X >= 176.0;
        const CenterRailMat = InExpress ? CourseMaterial.FoundryBoostGreen
                            : InOverdrive ? CourseMaterial.FoundryHazardRed
                            : CourseMaterial.FoundryYellow;
        EmitQuad(Facets, [X, -4.35, 0.018], [X1, -4.35, 0.018], [X1, -4.05, 0.018], [X, -4.05, 0.018], CenterRailMat);
        EmitQuad(Facets, [X,  4.05, 0.018], [X1,  4.05, 0.018], [X1,  4.35, 0.018], [X,  4.35, 0.018], CenterRailMat);
        // Outer belt border yellow safety strips
        EmitQuad(Facets, [X, -13.25, 0.018], [X1, -13.25, 0.018], [X1, -12.95, 0.018], [X, -12.95, 0.018], CourseMaterial.FoundryYellow);
        EmitQuad(Facets, [X,  12.95, 0.018], [X1,  12.95, 0.018], [X1,  13.25, 0.018], [X,  13.25, 0.018], CourseMaterial.FoundryYellow);
    }

    // ④ Crisp Start Loading Dock line (X = -132 m) & Checkered Finish Strip (X = 238 m)
    EmitQuad(Facets, [-132.5, -13.0, 0.020], [-131.5, -13.0, 0.020], [-131.5, 13.0, 0.020], [-132.5, 13.0, 0.020], CourseMaterial.FoundryYellow);
    for (let Row = 0; Row < 2; ++Row)
    {
        for (let C = -6; C < 6; ++C)
        {
            const X0 = 237.2 + Row * 1.4, X1 = X0 + 1.4;
            const Y0 = C * 2.15, Y1 = Y0 + 2.15;
            const Mat = ((Row + C) & 1) === 0 ? CourseMaterial.StudioWall : CourseMaterial.SteelDark;
            EmitQuad(Facets, [X0, Y0, 0.022], [X1, Y0, 0.022], [X1, Y1, 0.022], [X0, Y1, 0.022], Mat);
        }
    }

    return Facets;
}
