//============================================================================================================================================
//                                                       PISTONLABYRINTHSEQUENCE.JS
//============================================================================================================================================
// 🧱 The Piston Labyrinth · Shifting Concrete Maze ("labyrinth") — Real-Time Brutalist Trap & Ability Simulation:
//    • Runs all 7 Hidden Brutalist Traps + 8 Giant Sliding Hydraulic Megawalls + 6 Swinging Concrete Pendulums +
//      12–16 Procedural Jolt 3D Fracture Mirage Cheat Walls:
//        1. Sliding Hydraulic Megawalls (real dynamic collision + rail grinding + hackable wide open)
//        2. False-Slab Pitfalls (real dynamic QueryLabyrinthFloorHeight drop to -2.8 m w/ 23° escape ramp + hackable shut)
//        3. Pneumatic Slam-Plates (real dynamic rising ramp height + upward pneumatic launch kick + hackable flat)
//        4. Snap-Ambush Crusher Walls (recessed alcove pistons punching across blind lanes + horn-jammable / hackable)
//        5. Drop-Gate Guillotine Shutters (overhead blades falling on red infrared trip-beams + hackable hoisted)
//        6. Concealed Reverse-Roller Drag Strips (100% real Pacejka SurfaceVelocityQuery backward drag — hack [Q] into +22 m/s turbo!)
//        7. Corner Ambush Acid-Mist Vents (100% real Pacejka SurfaceGripQuery μ = 0.36× slick + green mist — clear w/ Horn [H]!)
//    • Two Live Interactive Signature Vehicle Abilities:
//        - [H] Acoustic Resonance Horn ("Shock-Wave Grille"): pre-shatters Mirage Walls 26 m ahead via real Fracture.BreakProp,
//          recoils Snap-Ambush Alcoves, and blasts Acid-Mist slicks dry!
//        - [Q] Vehicle Hacking ("Signal Hijacker"): hacks every mechanical trap within 34 m — flips Reverse Rollers into
//          emerald Turbo Slings, opens Sliding Megawalls, locks False-Slab Pitfalls solid, and kills Guillotine trip-beams!
//        - [N] New Random Maze: regenerates a fresh procedural shifting concrete labyrinth seed in place!

import { CourseMaterial } from "./MaterialSpecification.js";
import {
    ConstructRigidTransform,
    ConstructAxisQuaternion,
    MultiplyQuaternion,
    RotateVector,
    ClampScalar
} from "./TransformProjection.js";
import {
    GetActiveLabyrinthMaze,
    AssignLabyrinthHeightOracle,
    LabyrinthSpawn,
    LabyrinthFinishX
} from "./PistonLabyrinthStructure.js";

const Gravity = 9.81;

export class PistonLabyrinthSequence
{
    constructor(Host)
    {
        this.Host    = Host;
        this.Scene   = Host?.Scene || null;
        this.Enabled = (Host?.LevelKey === "labyrinth") && Boolean(this.Scene?.LabyrinthBlock);
        this.Maze    = this.Enabled ? GetActiveLabyrinthMaze() : null;

        this.Clock          = 0.0;
        this.RunTime        = 0.0;
        this.TrapsTriggered = 0;
        this.MirageSmashed  = 0;
        this.SystemsHacked  = 0;
        this.Finished       = false;
        this.FinishTime     = 0.0;
        this.FinishPortal   = "";
        this.BannerText     = "";
        this.BannerTone     = "warn"; // warn | cyan | emerald | acid
        this.BannerTimer    = 0.0;

        this.HornCooldown   = 0.0;
        this.HackCooldown   = 0.0;
        this.HornPulse      = null; // { Origin, Dir, Age, Duration }
        this.HackPulse      = null; // { Origin, Age, Duration }
        this.ActiveSurfaceTag = ""; // "" | "REVERSE ROLLERS" | "TURBO SLING" | "ACID SLICK"

        this.SlidingWalls   = [];
        this.Pitfalls       = [];
        this.SlamPlates     = [];
        this.AmbushWalls    = [];
        this.DropGates      = [];
        this.RollerStrips   = [];
        this.AcidVents      = [];
        this.Pendulums      = [];
        this.FlashLights    = [];
        this.PlumeQueue     = [];

        this.ControlStrip   = null;
        this._KeyHandler    = null;

        if (!this.Enabled)
            return;

        this.InitSimulationState();
        this.AttachVehicleOracles();
        this.BuildControlStrip();
        this.WriteAllDynamicOccupants();
    }

    AttachVehicleOracles()
    {
        AssignLabyrinthHeightOracle((X, Y) => this.QueryDynamicFloorHeight(X, Y));
        if (this.Host?.Vehicle)
        {
            this.Host.Vehicle.SurfaceVelocityQuery = (X, Y, Z) => this.QueryRollerSurfaceVelocity(X, Y, Z);
            this.Host.Vehicle.SurfaceGripQuery     = (X, Y, Z) => this.QueryAcidSurfaceGrip(X, Y, Z);
        }
    }

    InitSimulationState()
    {
        const Maze = this.Maze;
        this.Clock          = 0.0;
        this.RunTime        = 0.0;
        this.TrapsTriggered = 0;
        this.MirageSmashed  = 0;
        this.SystemsHacked  = 0;
        this.Finished       = false;
        this.FinishTime     = 0.0;
        this.FinishPortal   = "";
        this.HornCooldown   = 0.0;
        this.HackCooldown   = 0.0;
        this.HornPulse      = null;
        this.HackPulse      = null;
        this.FlashLights    = [];
        this.PlumeQueue     = [];
        this.ActiveSurfaceTag = "";

        this.SlidingWalls = Maze.SlidingWalls.map((S) => ({
            ...S,
            CurrentY:   S.CenterY + Math.sin(S.Phase) * S.TravelHalf * S.Dir,
            HackedLeft: 0.0,
            HitCool:    0.0
        }));

        this.Pitfalls = Maze.Pitfalls.map((P) => ({
            ...P,
            State:      "armed", // armed | snapping | open | hacked
            ArmTimer:   0.0,
            Tilt:       0.0,     // 0 = flush closed, 1.32 rad = dropped open!
            ResetTimer: 0.0
        }));

        this.SlamPlates = Maze.SlamPlates.map((S) => ({
            ...S,
            State:      "armed", // armed | sprung | hacked
            Tilt:       0.0,     // 0 = flush closed, 0.56 rad (~32°) = launched ramp!
            ResetTimer: 0.0,
            Kicked:     false
        }));

        this.AmbushWalls = Maze.AmbushWalls.map((A) => ({
            ...A,
            State:      "armed", // armed | punching | extended | jammed | hacked
            CurrentY:   A.RetractY,
            HoldTimer:  0.0,
            HitCool:    0.0
        }));

        this.DropGates = Maze.DropGates.map((G) => ({
            ...G,
            State:      "armed", // armed | falling | closed | raising | hacked
            Z:          6.2,
            VelZ:       0.0,
            HoldTimer:  0.0,
            HitCool:    0.0
        }));

        this.RollerStrips = Maze.RollerStrips.map((RS) => ({
            ...RS,
            Revealed:   false,
            Hacked:     false,
            Phase:      0.0
        }));

        this.AcidVents = Maze.AcidVents.map((AV) => ({
            ...AV,
            Active:     false,
            Cleared:    false,
            BurstTimer: 0.0
        }));

        this.Pendulums = Maze.Pendulums.map((PS) => ({
            ...PS,
            CurPhase:   PS.Phase,
            Angle:      Math.sin(PS.Phase) * PS.MaxAng,
            BlockPos:   [PS.X, PS.CenterY + Math.sin(PS.Phase) * PS.MaxAng * PS.ArmLen, PS.PivotZ - Math.cos(Math.sin(PS.Phase) * PS.MaxAng) * PS.ArmLen],
            HitCool:    0.0
        }));

        this.ShowBanner("PISTON LABYRINTH · [H] HORN SHATTERS MIRAGE · [Q] HACK FLIPS TRAPS", "cyan", 4.2);
    }

    Reset()
    {
        if (!this.Enabled)
            return;
        this.InitSimulationState();
        this.AttachVehicleOracles();
        if (this.Host?.Vehicle)
        {
            this.Host.Vehicle.Respawn(LabyrinthSpawn.Position.slice(), LabyrinthSpawn.Yaw);
            if (this.Host.Chase)
                this.Host.Chase.Seeded = false;
        }
        this.Host?.Fracture?.Reset?.();
        this.WriteAllDynamicOccupants();
        this.Host?.UploadLabyrinthOccupants?.();
    }

    Dispose()
    {
        AssignLabyrinthHeightOracle(null);
        if (this.Host?.Vehicle)
        {
            if (this.Host.Vehicle.SurfaceVelocityQuery)
                this.Host.Vehicle.SurfaceVelocityQuery = null;
            if (this.Host.Vehicle.SurfaceGripQuery)
                this.Host.Vehicle.SurfaceGripQuery = null;
        }
        if (this.ControlStrip && this.ControlStrip.parentNode)
            this.ControlStrip.parentNode.removeChild(this.ControlStrip);
        this.ControlStrip = null;
        if (this._KeyHandler && typeof window !== "undefined")
            window.removeEventListener("keydown", this._KeyHandler);
        this._KeyHandler = null;
    }

    ShowBanner(Text, Tone = "warn", Duration = 2.6)
    {
        this.BannerText  = Text;
        this.BannerTone  = Tone;
        this.BannerTimer = Duration;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                  LIVE PHYSICS ORACLES (HEIGHT, PACEJKA ROLLERS & ACID GRIP)
    //--------------------------------------------------------------------------------------------------------------------

    QueryDynamicFloorHeight(X, Y)
    {
        // ① False-Slab Pitfalls: when opened (Tilt > 0.25), floor drops cleanly into -2.8 m concrete shaft w/ 23° East rescue ramp!
        for (const P of this.Pitfalls)
        {
            if (P.Tilt > 0.25 && X >= P.X0 && X <= P.X0 + P.SlabLen && Math.abs(Y - P.CenterY) <= P.HalfW)
            {
                const LocalX = X - P.X0;
                if (LocalX <= P.DropLen)
                    return -P.Depth;
                const T = ClampScalar((LocalX - P.DropLen) / P.RampLen, 0.0, 1.0);
                return -P.Depth * (1.0 - T);
            }
        }
        // ② Pneumatic Slam-Plates: when sprung upward, dynamically form a real hinged concrete launch ramp!
        for (const S of this.SlamPlates)
        {
            if (S.Tilt > 0.02 && X >= S.X0 && X <= S.X0 + S.Len && Math.abs(Y - S.CenterY) <= S.HalfW)
                return (X - S.X0) * Math.sin(S.Tilt);
        }
        return 0.0;
    }

    // 📝 100% Real Pacejka contact-patch surface velocity for Concealed Reverse-Roller Strips (NEVER fakes chassis velocity!)
    QueryRollerSurfaceVelocity(X, Y, Z = 0.0)
    {
        if (Z > 1.25)
            return null;
        for (const RS of this.RollerStrips)
        {
            if (Math.abs(X - RS.CenterX) <= RS.HalfL && Math.abs(Y - RS.CenterY) <= RS.HalfW)
            {
                const SpeedX = RS.Hacked ? RS.HackedSpeed : RS.ReverseSpeed;
                return [SpeedX, 0.0, 0.0];
            }
        }
        return null;
    }

    // 📝 100% Real Pacejka per-wheel tyre friction coefficient (μ) modulation on Corner Ambush Acid-Mist Slicks!
    QueryAcidSurfaceGrip(X, Y, Z = 0.0)
    {
        if (Z > 1.25)
            return 1.0;
        for (const AV of this.AcidVents)
        {
            if (AV.Active && !AV.Cleared && Math.abs(X - AV.X) <= AV.HalfL && Math.abs(Y - AV.CenterY) <= AV.HalfW)
                return AV.AcidGrip;
        }
        return 1.0;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                  ACTIVE VEHICLE ABILITIES: [H] ACOUSTIC HORN & [Q] HACK
    //--------------------------------------------------------------------------------------------------------------------

    TriggerAcousticHorn()
    {
        if (!this.Enabled || this.HornCooldown > 0.0)
            return { Fired: false, Shattered: 0, Jammed: 0, ClearedAcid: 0 };

        const Car = this.Host?.Vehicle;
        const Pos = Car ? Car.Position.slice() : [LabyrinthSpawn.Position[0], 0.0, 0.65];
        const Fwd = Car ? RotateVector(Car.Orientation, [1.0, 0.0, 0.0]) : [1.0, 0.0, 0.0];
        const FLen = Math.hypot(Fwd[0], Fwd[1]) || 1.0;
        const Dir = [Fwd[0] / FLen, Fwd[1] / FLen, 0.0];

        this.HornCooldown = 2.6;
        this.HornPulse = {
            Origin:   [Pos[0] + Dir[0] * 2.4, Pos[1] + Dir[1] * 2.4, Math.max(1.0, Pos[2] + 0.35)],
            Dir,
            Age:      0.0,
            Duration: 0.48
        };

        let Shattered   = 0;
        let Jammed      = 0;
        let ClearedAcid = 0;

        // ① Pre-shatter ALL Mirage / Cheat Concrete Partitions inside the 28 m forward pressure cone via real Fracture.BreakProp!
        const Props = this.Host?.Fracture?.Props || [];
        for (const Prop of Props)
        {
            if (Prop.State !== "intact")
                continue;
            const C = Prop.Definition?.Centre;
            if (!C)
                continue;
            const DX = C[0] - Pos[0], DY = C[1] - Pos[1];
            const Dist = Math.hypot(DX, DY);
            if (Dist > 30.0)
                continue;
            const Dot = Dist > 0.1 ? (DX * Dir[0] + DY * Dir[1]) / Dist : 1.0;
            if (Dot < 0.35 && Dist > 6.5)
                continue;

            this.Host.Fracture.BreakProp(Prop, {
                Point:    [C[0] - Dir[0] * 0.4, C[1] - Dir[1] * 0.4, C[2]],
                Normal:   [-Dir[0], -Dir[1], 0.0],
                Velocity: [Dir[0] * 38.0, Dir[1] * 38.0, 3.2],
                Energy:   54000.0,
                Approach: 38.0,
                Vehicle:  null // Zero vehicle slowdown!
            });
            Shattered++;
        }

        // ② Recoil & jam any Snap-Ambush Crusher Walls inside the 28 m forward cone!
        for (const A of this.AmbushWalls)
        {
            const DX = A.X - Pos[0], DY = A.CurrentY - Pos[1];
            const Dist = Math.hypot(DX, DY);
            if (Dist <= 28.0)
            {
                A.State     = "jammed";
                A.CurrentY  = A.RetractY;
                A.HoldTimer = 8.0;
                Jammed++;
            }
        }

        // ③ Blow away active Acid-Mist Vent clouds & acid slicks within 30 m ahead!
        for (const AV of this.AcidVents)
        {
            const Dist = Math.hypot(AV.X - Pos[0], AV.CenterY - Pos[1]);
            if (Dist <= 30.0 && (AV.Active || !AV.Cleared))
            {
                AV.Active  = false;
                AV.Cleared = true;
                ClearedAcid++;
            }
        }

        // Dramatic cyan shockwave light flash + dust burst + sonic boom!
        this.FlashLights.push({
            Pos:      [Pos[0] + Dir[0] * 6.0, Pos[1] + Dir[1] * 6.0, Pos[2] + 1.4],
            Colour:   [0.32, 0.88, 1.0],
            Power:    26.0,
            Age:      0.0,
            Duration: 0.45
        });
        this.Host?.Smoke?.Sources?.QueueBurst?.({
            Position: [Pos[0] + Dir[0] * 5.0, Pos[1] + Dir[1] * 5.0, Math.max(0.8, Pos[2] + 0.4)],
            Radius:   1.2,
            Velocity: [Dir[0] * 16.0, Dir[1] * 16.0, 2.0],
            Smoke:    24.0,
            Heat:     6.0,
            Flame:    0.0,
            Soot:     6.0,
            Seconds:  0.42
        });
        this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.52);

        const Hits = [];
        if (Shattered > 0)   Hits.push(`${Shattered} MIRAGE WALL${Shattered > 1 ? "S" : ""} SHATTERED`);
        if (Jammed > 0)      Hits.push(`${Jammed} AMBUSH JAMMED`);
        if (ClearedAcid > 0) Hits.push("ACID MIST BLOWN CLEAR");
        this.ShowBanner(
            Hits.length > 0 ? `◉ ACOUSTIC HORN — ${Hits.join(" · ")}!` : "◉ ACOUSTIC RESONANCE SHOCKWAVE FIRED!",
            "cyan",
            2.4
        );
        return { Fired: true, Shattered, Jammed, ClearedAcid };
    }

    TriggerVehicleHack()
    {
        if (!this.Enabled || this.HackCooldown > 0.0)
            return { Fired: false, Count: 0, FlippedRollers: 0 };

        const Car = this.Host?.Vehicle;
        const Pos = Car ? Car.Position.slice() : [LabyrinthSpawn.Position[0], 0.0, 0.65];

        this.HackCooldown = 3.0;
        this.HackPulse = {
            Origin:   [Pos[0], Pos[1], Math.max(0.8, Pos[2] + 0.5)],
            Age:      0.0,
            Duration: 0.62
        };

        const Radius = 36.0;
        let Count = 0;
        let FlippedRollers = 0;

        // ① Force Sliding Hydraulic Megawalls wide OPEN!
        for (const SW of this.SlidingWalls)
        {
            if (Math.hypot(SW.X - Pos[0], SW.CenterY - Pos[1]) <= Radius)
            {
                SW.HackedLeft = 12.0;
                Count++;
            }
        }
        // ② Lock False-Slab Pitfalls SOLID flat!
        for (const P of this.Pitfalls)
        {
            if (Math.hypot((P.X0 + 5.0) - Pos[0], P.CenterY - Pos[1]) <= Radius)
            {
                P.State = "hacked";
                P.Tilt  = 0.0;
                Count++;
            }
        }
        // ③ Lock Pneumatic Slam-Plates FLAT!
        for (const S of this.SlamPlates)
        {
            if (Math.hypot((S.X0 + 4.0) - Pos[0], S.CenterY - Pos[1]) <= Radius)
            {
                S.State = "hacked";
                S.Tilt  = 0.0;
                Count++;
            }
        }
        // ④ Retract Snap-Ambush Crusher Walls!
        for (const A of this.AmbushWalls)
        {
            if (Math.hypot(A.X - Pos[0], A.CenterY - Pos[1]) <= Radius)
            {
                A.State    = "hacked";
                A.CurrentY = A.RetractY;
                Count++;
            }
        }
        // ⑤ Kill Drop-Gate Guillotine trip-beams & hoist blades open!
        for (const G of this.DropGates)
        {
            if (Math.hypot(G.X - Pos[0], G.CenterY - Pos[1]) <= Radius)
            {
                G.State = "hacked";
                G.VelZ  = 0.0;
                Count++;
            }
        }
        // ⑥ FLIP Concealed Reverse-Roller Drag Strips into +22 m/s EMERALD TURBO SLINGS!
        for (const RS of this.RollerStrips)
        {
            if (Math.hypot(RS.CenterX - Pos[0], RS.CenterY - Pos[1]) <= Radius + 8.0)
            {
                RS.Revealed = true;
                RS.Hacked   = true;
                FlippedRollers++;
                Count++;
            }
        }

        this.SystemsHacked += Count;
        this.FlashLights.push({
            Pos:      [Pos[0], Pos[1], Pos[2] + 2.2],
            Colour:   [0.20, 1.0, 0.55],
            Power:    24.0,
            Age:      0.0,
            Duration: 0.55
        });

        const Msg = FlippedRollers > 0
            ? `⚡ HACKED ${Count} SYSTEMS — REVERSE ROLLERS FLIPPED TO EMERALD TURBO SLING!`
            : (Count > 0 ? `⚡ HACKED ${Count} LABYRINTH TRAPS — GATES OPEN & SLABS LOCKED!` : "⚡ HACK SCAN — NO TRAPS WITHIN 36 M");
        this.ShowBanner(Msg, "emerald", 2.6);
        return { Fired: true, Count, FlippedRollers };
    }

    OnWallBroken(Prop)
    {
        if (!this.Enabled || !Prop?.Definition)
            return;
        this.MirageSmashed++;
        const C = Prop.Definition.Centre || [0.0, 0.0, 2.0];
        this.Host?.Smoke?.Sources?.QueueBurst?.({
            Position: [C[0], C[1], Math.max(0.9, C[2])],
            Radius:   1.0,
            Velocity: [0.0, 0.0, 2.4],
            Smoke:    34.0,
            Heat:     5.0,
            Flame:    0.0,
            Soot:     14.0,
            Seconds:  0.60
        });
        this.Host?.Effects?.BurstTyre?.(
            ConstructRigidTransform([C[0], C[1], Math.max(0.8, C[2])], [0.0, 0.0, 0.0, 1.0]),
            [0.5, 2.2, 1.2],
            [2.0, 0.0, 2.4],
            CourseMaterial.LabMirageWall
        );
        this.ShowBanner(`MIRAGE WALL SHATTERED! (${Prop.Pieces.length} CONCRETE SHARDS — SHORTCUT OPEN!)`, "cyan", 2.3);
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                             PER-FRAME SIMULATION STEP
    //--------------------------------------------------------------------------------------------------------------------

    EmitImpactSparks(Point, Normal, Count = 7)
    {
        const Sparks = [];
        for (let K = 0; K < Count; ++K)
        {
            const Ang = (K / Count) * Math.PI * 2.0 + this.Clock * 3.0;
            Sparks.push({
                Point:    [Point[0], Point[1], Math.max(0.5, Point[2])],
                Velocity: [Normal[0] * 5.0 + Math.cos(Ang) * 4.0, Normal[1] * 5.0 + Math.sin(Ang) * 4.0, 2.2 + (K % 3) * 1.4],
                Normal,
                Strength: 2.1,
                Rim:      null
            });
        }
        this.Host?.Sparks?.Emit?.(Sparks, 1.0 / 60.0);
    }

    Advance(Seconds)
    {
        if (!this.Enabled || !this.Maze)
            return false;
        const Dt = Math.min(0.05, Math.max(0.0001, Seconds));
        this.Clock += Dt;
        if (!this.Finished)
            this.RunTime += Dt;
        if (this.BannerTimer > 0.0)
            this.BannerTimer = Math.max(0.0, this.BannerTimer - Dt);
        if (this.HornCooldown > 0.0)
            this.HornCooldown = Math.max(0.0, this.HornCooldown - Dt);
        if (this.HackCooldown > 0.0)
            this.HackCooldown = Math.max(0.0, this.HackCooldown - Dt);

        // Keep demo fuel/HV topped up so HUD stays crisp and distraction-free
        if (this.Host?.DrivingHud)
        {
            if (this.Host.DrivingHud.FallbackFuel < 0.78)    this.Host.DrivingHud.FallbackFuel    = 0.78;
            if (this.Host.DrivingHud.FallbackBattery < 0.78) this.Host.DrivingHud.FallbackBattery = 0.78;
        }

        const Car = this.Host?.Vehicle;
        const CarPos = Car ? Car.Position : [LabyrinthSpawn.Position[0], 0.0, 0.65];

        // ① 8 Giant Sliding Hydraulic Megawalls
        for (const SW of this.SlidingWalls)
        {
            if (SW.HitCool > 0.0) SW.HitCool = Math.max(0.0, SW.HitCool - Dt);
            if (SW.HackedLeft > 0.0)
            {
                SW.HackedLeft = Math.max(0.0, SW.HackedLeft - Dt);
                const OpenY = SW.CenterY + SW.Dir * 10.6;
                SW.CurrentY += (OpenY - SW.CurrentY) * Math.min(1.0, Dt * 5.5);
            }
            else
            {
                SW.Phase += Dt * SW.Rate;
                SW.CurrentY = SW.CenterY + Math.sin(SW.Phase) * SW.TravelHalf * SW.Dir;
            }
            // Dynamic AABB collision vs player car (wall spans X ± 1.3 m, Y ± 5.75 m, Z = 0..16.5 m)
            if (Car && SW.HitCool <= 0.0)
            {
                const DX = CarPos[0] - SW.X;
                const DY = CarPos[1] - SW.CurrentY;
                if (Math.abs(DX) < 2.9 && Math.abs(DY) < 6.8 && CarPos[2] < 16.0)
                {
                    const PenX = 2.9 - Math.abs(DX);
                    const PenY = 6.8 - Math.abs(DY);
                    if (PenX < PenY)
                    {
                        const Sgn = DX >= 0.0 ? 1.0 : -1.0;
                        Car.Position[0] += Sgn * (PenX + 0.12);
                        Car.LinearVelocity[0] = Sgn * Math.max(6.0, Math.abs(Car.LinearVelocity[0]) * 0.55);
                        this.EmitImpactSparks([SW.X + Sgn * 1.4, CarPos[1], 1.1], [Sgn, 0.0, 0.0]);
                    }
                    else
                    {
                        const Sgn = DY >= 0.0 ? 1.0 : -1.0;
                        Car.Position[1] += Sgn * (PenY + 0.15);
                        Car.LinearVelocity[1] = Sgn * Math.max(9.5, Math.abs(Car.LinearVelocity[1]) * 0.65);
                        this.EmitImpactSparks([CarPos[0], SW.CurrentY + Sgn * 5.8, 1.1], [0.0, Sgn, 0.0]);
                    }
                    SW.HitCool = 0.25;
                    this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.38);
                }
            }
        }

        // ② 6 False-Slab Pitfalls (real dynamic floor drop to -2.8 m!)
        for (const P of this.Pitfalls)
        {
            if (P.State === "hacked")
            {
                P.Tilt = Math.max(0.0, P.Tilt - Dt * 4.0);
                continue;
            }
            const InLane = Math.abs(CarPos[1] - P.CenterY) < (P.HalfW + 1.8);
            const NearApproach = CarPos[0] >= (P.X0 - 9.5) && CarPos[0] <= (P.X0 + P.SlabLen + 1.0);
            if (P.State === "armed" && InLane && NearApproach)
            {
                P.State    = "snapping";
                P.ArmTimer = 0.30;
                this.TrapsTriggered++;
                this.ShowBanner("⚠ FALSE-SLAB PITFALL DROPPING! FLOOR IT OR HACK [Q]!", "warn", 2.2);
                this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.32);
            }
            else if (P.State === "snapping")
            {
                P.ArmTimer -= Dt;
                P.Tilt = Math.min(0.14, P.Tilt + Dt * 0.5);
                if (P.ArmTimer <= 0.0)
                {
                    P.State      = "open";
                    P.ResetTimer = 7.5;
                    this.Host?.Smoke?.Sources?.QueueBurst?.({
                        Position: [P.X0 + 3.5, P.CenterY, -0.3],
                        Radius:   1.4,
                        Velocity: [0.0, 0.0, 3.2],
                        Smoke:    28.0,
                        Heat:     2.0,
                        Flame:    0.0,
                        Soot:     16.0,
                        Seconds:  0.55
                    });
                }
            }
            else if (P.State === "open")
            {
                P.Tilt = Math.min(1.28, P.Tilt + Dt * 4.8);
                P.ResetTimer -= Dt;
                if (P.ResetTimer <= 0.0 && Math.abs(CarPos[0] - (P.X0 + 5.0)) > 14.0)
                {
                    P.State = "armed";
                    P.Tilt  = 0.0;
                }
            }
        }

        // ③ 6 Pneumatic Slam-Plates (camouflaged floor kickers launching car airborne!)
        for (const S of this.SlamPlates)
        {
            if (S.State === "hacked")
            {
                S.Tilt = Math.max(0.0, S.Tilt - Dt * 4.5);
                continue;
            }
            const OnPlate = CarPos[0] >= (S.X0 - 1.2) && CarPos[0] <= (S.X0 + S.Len) && Math.abs(CarPos[1] - S.CenterY) <= (S.HalfW + 0.8);
            if (S.State === "armed" && OnPlate)
            {
                S.State      = "sprung";
                S.ResetTimer = 5.5;
                S.Kicked     = false;
                this.TrapsTriggered++;
                this.ShowBanner("▲ PNEUMATIC SLAM-PLATE LAUNCH!", "warn", 2.0);
                this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.42);
                this.EmitImpactSparks([S.X0 + 4.0, S.CenterY, 0.4], [0.0, 0.0, 1.0], 8);
            }
            if (S.State === "sprung")
            {
                S.Tilt = Math.min(0.54, S.Tilt + Dt * 3.6);
                // Impart crisp upward pneumatic kick when snapping beneath the chassis!
                if (!S.Kicked && Car && OnPlate && CarPos[2] < 2.2)
                {
                    S.Kicked = true;
                    Car.LinearVelocity[2] = Math.max(Car.LinearVelocity[2], 7.8);
                }
                S.ResetTimer -= Dt;
                if (S.ResetTimer <= 0.0 && !OnPlate)
                {
                    S.Tilt = Math.max(0.0, S.Tilt - Dt * 1.4);
                    if (S.Tilt <= 0.01)
                        S.State = "armed";
                }
            }
        }

        // ④ 6 Snap-Ambush Crusher Walls (recessed concrete pistons punching out of blind walls!)
        for (const A of this.AmbushWalls)
        {
            if (A.HitCool > 0.0) A.HitCool = Math.max(0.0, A.HitCool - Dt);
            if (A.State === "hacked" || A.State === "jammed")
            {
                A.CurrentY += (A.RetractY - A.CurrentY) * Math.min(1.0, Dt * 6.0);
                if (A.State === "jammed")
                {
                    A.HoldTimer -= Dt;
                    if (A.HoldTimer <= 0.0) A.State = "armed";
                }
                continue;
            }
            const InLane = Math.abs(CarPos[1] - A.CenterY) < 7.8;
            if (A.State === "armed" && InLane && CarPos[0] >= (A.X - 11.5) && CarPos[0] <= (A.X + 2.0))
            {
                A.State = "punching";
                this.TrapsTriggered++;
                this.ShowBanner("⚠ SNAP-AMBUSH WALL! DODGE OR HORN [H]!", "warn", 2.0);
                this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.45);
            }
            else if (A.State === "punching")
            {
                const StepY = -A.Side * 24.0 * Dt;
                A.CurrentY += StepY;
                if ((A.Side > 0 && A.CurrentY <= A.PunchY) || (A.Side < 0 && A.CurrentY >= A.PunchY))
                {
                    A.CurrentY  = A.PunchY;
                    A.State     = "extended";
                    A.HoldTimer = 2.8;
                }
            }
            else if (A.State === "extended")
            {
                A.HoldTimer -= Dt;
                if (A.HoldTimer <= 0.0)
                {
                    A.CurrentY += A.Side * 5.0 * Dt;
                    if ((A.Side > 0 && A.CurrentY >= A.RetractY) || (A.Side < 0 && A.CurrentY <= A.RetractY))
                    {
                        A.CurrentY = A.RetractY;
                        A.State    = "armed";
                    }
                }
            }
            // Dynamic T-bone collision vs player car
            if (Car && A.HitCool <= 0.0 && Math.abs(CarPos[0] - A.X) < 4.5 && Math.abs(CarPos[1] - A.CurrentY) < 2.2 && CarPos[2] < 5.6)
            {
                const Sgn = (CarPos[1] - A.CurrentY) >= 0.0 ? 1.0 : -1.0;
                Car.Position[1] += Sgn * 1.1;
                Car.LinearVelocity[1] = Sgn * 13.5;
                A.HitCool = 0.35;
                this.EmitImpactSparks([CarPos[0], A.CurrentY + Sgn * 1.1, 1.1], [0.0, Sgn, 0.0], 9);
                this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.5);
            }
        }

        // ⑤ 6 Drop-Gate Guillotine Shutters + Crimson Infrared Trip-Beams
        for (const G of this.DropGates)
        {
            if (G.HitCool > 0.0) G.HitCool = Math.max(0.0, G.HitCool - Dt);
            if (G.State === "hacked")
            {
                G.Z = Math.min(6.4, G.Z + Dt * 6.0);
                continue;
            }
            const InLane = Math.abs(CarPos[1] - G.CenterY) < 8.0;
            if (G.State === "armed" && InLane && CarPos[0] >= (G.TripX - 1.2) && CarPos[0] <= (G.TripX + 3.0))
            {
                G.State = "falling";
                G.VelZ  = -3.5;
                this.TrapsTriggered++;
                this.ShowBanner("⚠ GUILLOTINE TRIP-BEAM BROKEN! BLAST UNDER OR HACK [Q]!", "warn", 2.1);
            }
            else if (G.State === "falling")
            {
                G.VelZ -= 19.5 * Dt;
                G.Z    += G.VelZ * Dt;
                if (G.Z <= 0.0)
                {
                    G.Z         = 0.0;
                    G.VelZ      = 0.0;
                    G.State     = "closed";
                    G.HoldTimer = 3.2;
                    this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.58);
                    this.Host?.Smoke?.Sources?.QueueBurst?.({
                        Position: [G.X, G.CenterY, 0.45],
                        Radius:   1.5,
                        Velocity: [0.0, 0.0, 2.8],
                        Smoke:    38.0,
                        Heat:     4.0,
                        Flame:    0.0,
                        Soot:     16.0,
                        Seconds:  0.65
                    });
                }
            }
            else if (G.State === "closed")
            {
                G.HoldTimer -= Dt;
                if (G.HoldTimer <= 0.0)
                    G.State = "raising";
            }
            else if (G.State === "raising")
            {
                G.Z += 2.6 * Dt;
                if (G.Z >= 6.2)
                {
                    G.Z     = 6.2;
                    G.State = "armed";
                }
            }
            // Dynamic collision when blade descends below vehicle roof (Z < 1.85 m)
            if (Car && G.HitCool <= 0.0 && G.Z < 1.85 && Math.abs(CarPos[0] - G.X) < 2.2 && Math.abs(CarPos[1] - G.CenterY) < 8.0)
            {
                const Sgn = CarPos[0] < G.X ? -1.0 : 1.0;
                Car.Position[0] = G.X + Sgn * 2.45;
                Car.LinearVelocity[0] = Sgn * Math.max(5.5, Math.abs(Car.LinearVelocity[0]) * 0.45);
                G.HitCool = 0.28;
                this.EmitImpactSparks([G.X + Sgn * 0.9, CarPos[1], 1.0], [Sgn, 0.0, 0.0], 8);
            }
        }

        // ⑥ 5 Concealed Reverse-Roller Drag Strips + ⑦ 5 Corner Ambush Acid-Mist Vents
        this.ActiveSurfaceTag = "";
        for (const RS of this.RollerStrips)
        {
            const Speed = RS.Hacked ? RS.HackedSpeed : RS.ReverseSpeed;
            RS.Phase = (RS.Phase + Speed * Dt) % (RS.HalfL * 2.0);
            if (RS.Phase < 0.0) RS.Phase += RS.HalfL * 2.0;

            if (Math.abs(CarPos[0] - RS.CenterX) <= RS.HalfL && Math.abs(CarPos[1] - RS.CenterY) <= RS.HalfW)
            {
                if (!RS.Revealed)
                {
                    RS.Revealed = true;
                    this.TrapsTriggered++;
                    if (!RS.Hacked)
                        this.ShowBanner("⚠ CONCEALED REVERSE ROLLERS! HACK [Q] TO FLIP TURBO SLING!", "warn", 2.4);
                }
                this.ActiveSurfaceTag = RS.Hacked ? "EMERALD TURBO SLING (+22 m/s)" : "REVERSE ROLLERS (-16.5 m/s)";
            }
        }

        for (const AV of this.AcidVents)
        {
            if (AV.BurstTimer > 0.0) AV.BurstTimer -= Dt;
            const Dist = Math.hypot(AV.X - CarPos[0], AV.CenterY - CarPos[1]);
            if (!AV.Active && !AV.Cleared && Dist < 17.5)
            {
                AV.Active = true;
                this.TrapsTriggered++;
                this.ShowBanner("☣ ACID-MIST AMBUSH! TYRE GRIP 36% — CLEAR W/ HORN [H]!", "acid", 2.6);
            }
            if (AV.Active && !AV.Cleared)
            {
                if (AV.BurstTimer <= 0.0 && Dist < 38.0)
                {
                    AV.BurstTimer = 0.55;
                    this.Host?.Smoke?.Sources?.QueueBurst?.({
                        Position: [AV.X, AV.WallY - AV.Side * 1.2, 1.4],
                        Radius:   1.3,
                        Velocity: [0.0, -AV.Side * 8.5, 0.6],
                        Smoke:    28.0,
                        Heat:     4.0,
                        Flame:    0.0,
                        Soot:     2.0,
                        Seconds:  0.70
                    });
                }
                if (Math.abs(CarPos[0] - AV.X) <= AV.HalfL && Math.abs(CarPos[1] - AV.CenterY) <= AV.HalfW)
                    this.ActiveSurfaceTag = "ACID MIST SLICK (36% μ GRIP)";
            }
        }

        // ⑧ 6 Overhead Swinging Concrete Pendulums
        for (const PS of this.Pendulums)
        {
            if (PS.HitCool > 0.0) PS.HitCool = Math.max(0.0, PS.HitCool - Dt);
            PS.CurPhase += Dt * PS.Rate;
            PS.Angle     = Math.sin(PS.CurPhase) * PS.MaxAng;
            const BY     = PS.CenterY + Math.sin(PS.Angle) * PS.ArmLen;
            const BZ     = PS.PivotZ  - Math.cos(PS.Angle) * PS.ArmLen;
            PS.BlockPos  = [PS.X, BY, BZ];
            if (Car && PS.HitCool <= 0.0 && BZ < 3.4)
            {
                const D = Math.hypot(CarPos[0] - PS.X, CarPos[1] - BY);
                if (D < 2.85)
                {
                    const SwingDir = Math.cos(PS.CurPhase) >= 0.0 ? 1.0 : -1.0;
                    Car.LinearVelocity[1] += SwingDir * 14.0;
                    PS.HitCool = 0.45;
                    this.EmitImpactSparks([PS.X, BY, 1.4], [0.0, SwingDir, 0.0], 8);
                    this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.45);
                }
            }
        }

        // ⑨ Advance Ability VFX Pulses & Dynamic Lights
        if (this.HornPulse)
        {
            this.HornPulse.Age += Dt;
            if (this.HornPulse.Age >= this.HornPulse.Duration)
                this.HornPulse = null;
        }
        if (this.HackPulse)
        {
            this.HackPulse.Age += Dt;
            if (this.HackPulse.Age >= this.HackPulse.Duration)
                this.HackPulse = null;
        }
        for (let I = this.FlashLights.length - 1; I >= 0; --I)
        {
            this.FlashLights[I].Age += Dt;
            if (this.FlashLights[I].Age >= this.FlashLights[I].Duration)
                this.FlashLights.splice(I, 1);
        }

        // ⑩ Extraction & Blind Split Portal Arrival!
        if (!this.Finished && CarPos[0] >= LabyrinthFinishX)
        {
            this.Finished     = true;
            this.FinishTime   = this.RunTime;
            this.FinishPortal = CarPos[1] >= 0.0 ? "ACID WASTELAND PORTAL" : "ORBITAL LAUNCH TRENCH PORTAL";
            this.ShowBanner(`★ LABYRINTH ESCAPED IN ${this.FinishTime.toFixed(1)}s — ENTERED ${this.FinishPortal}!`, "emerald", 5.5);
        }

        this.WriteAllDynamicOccupants();
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                       GPU DYNAMIC OCCUPANT TRANSFORM SYNC
    //--------------------------------------------------------------------------------------------------------------------

    AssignDynamicOcc(OccIdx, Transform, Moving = true, Inactive = false)
    {
        const Occ = this.Scene?.Occupants?.[OccIdx];
        if (!Occ)
            return;
        Occ.PreviousTransform = Occ.Transform ? Occ.Transform.slice() : Transform.slice();
        Occ.Transform = Transform;
        Occ.Moving    = Moving;
        Occ.Inactive  = Inactive;
    }

    WriteAllDynamicOccupants()
    {
        const Block = this.Scene?.LabyrinthBlock;
        if (!Block)
            return;
        const IdentityQ = [0.0, 0.0, 0.0, 1.0];
        const Parked    = ConstructRigidTransform([0.0, 0.0, -45.0], IdentityQ);

        // ① 8 Giant Sliding Hydraulic Megawalls + Top Warning Beacons
        for (let I = 0; I < Block.SlidingWalls.length; ++I)
        {
            const SW = this.SlidingWalls[I];
            if (!SW)
            {
                this.AssignDynamicOcc(Block.SlidingWalls[I],   Parked, false, true);
                this.AssignDynamicOcc(Block.SlidingBeacons[I], Parked, false, true);
                continue;
            }
            this.AssignDynamicOcc(Block.SlidingWalls[I], ConstructRigidTransform([SW.X, SW.CurrentY, 0.0], IdentityQ), true, false);
            // Hide red warning beacon when hacked wide open!
            if (SW.HackedLeft > 0.0)
                this.AssignDynamicOcc(Block.SlidingBeacons[I], Parked, false, true);
            else
                this.AssignDynamicOcc(Block.SlidingBeacons[I], ConstructRigidTransform([SW.X, SW.CurrentY, 16.96], IdentityQ), true, false);
        }

        // ② 6 False-Slab Pitfall Decks (hinge DOWNWARD around local +Y axis at West edge X0!)
        for (let I = 0; I < Block.TrapSlabs.length; ++I)
        {
            const P = this.Pitfalls[I];
            if (!P)
            {
                this.AssignDynamicOcc(Block.TrapSlabs[I], Parked, false, true);
                continue;
            }
            // Positive pitch around +Y swings +X downward into -Z abyss!
            const QTilt = ConstructAxisQuaternion([0.0, 1.0, 0.0], P.Tilt);
            this.AssignDynamicOcc(Block.TrapSlabs[I], ConstructRigidTransform([P.X0, P.CenterY, 0.0], QTilt), true, false);
        }

        // ③ 6 Pneumatic Slam-Plates (hinge UPWARD around local +Y axis at West edge X0!)
        for (let I = 0; I < Block.SlamPlates.length; ++I)
        {
            const S = this.SlamPlates[I];
            if (!S)
            {
                this.AssignDynamicOcc(Block.SlamPlates[I], Parked, false, true);
                continue;
            }
            // Negative pitch around +Y lifts +X upward into +Z launch ramp!
            const QLift = ConstructAxisQuaternion([0.0, 1.0, 0.0], -S.Tilt);
            this.AssignDynamicOcc(Block.SlamPlates[I], ConstructRigidTransform([S.X0, S.CenterY, 0.0], QLift), true, false);
        }

        // ④ 6 Snap-Ambush Crusher Walls
        for (let I = 0; I < Block.AmbushWalls.length; ++I)
        {
            const A = this.AmbushWalls[I];
            if (!A)
            {
                this.AssignDynamicOcc(Block.AmbushWalls[I], Parked, false, true);
                continue;
            }
            this.AssignDynamicOcc(Block.AmbushWalls[I], ConstructRigidTransform([A.X, A.CurrentY, 0.0], IdentityQ), true, false);
        }

        // ⑤ 6 Drop-Gate Guillotine Shutters + Crimson Infrared Trip-Beams
        for (let I = 0; I < Block.DropGates.length; ++I)
        {
            const G = this.DropGates[I];
            if (!G)
            {
                this.AssignDynamicOcc(Block.DropGates[I], Parked, false, true);
                this.AssignDynamicOcc(Block.TripBeams[I], Parked, false, true);
                continue;
            }
            this.AssignDynamicOcc(Block.DropGates[I], ConstructRigidTransform([G.X, G.CenterY, G.Z], IdentityQ), true, false);
            if (G.State === "armed")
                this.AssignDynamicOcc(Block.TripBeams[I], ConstructRigidTransform([G.TripX, G.CenterY, 0.88], IdentityQ), false, false);
            else
                this.AssignDynamicOcc(Block.TripBeams[I], Parked, false, true);
        }

        // ⑥ 20 Animated Reverse-Roller Slat Bars (4 per strip)
        for (let RIdx = 0; RIdx < this.RollerStrips.length; ++RIdx)
        {
            const RS = this.RollerStrips[RIdx];
            const Span = RS.HalfL * 2.0;
            for (let K = 0; K < 4; ++K)
            {
                const Slot = RIdx * 4 + K;
                if (Slot >= Block.RollerSlats.length) break;
                const Offset = ((RS.Phase + (K * Span) * 0.25) % Span) - RS.HalfL;
                this.AssignDynamicOcc(Block.RollerSlats[Slot], ConstructRigidTransform([RS.CenterX + Offset, RS.CenterY, 0.015], IdentityQ), true, false);
            }
        }

        // ⑦ 5 Corner Ambush Acid-Mist Slick Patches
        for (let I = 0; I < Block.AcidSlicks.length; ++I)
        {
            const AV = this.AcidVents[I];
            if (!AV || !AV.Active || AV.Cleared)
                this.AssignDynamicOcc(Block.AcidSlicks[I], Parked, false, true);
            else
                this.AssignDynamicOcc(Block.AcidSlicks[I], ConstructRigidTransform([AV.X, AV.CenterY, 0.01], IdentityQ), false, false);
        }

        // ⑧ 6 Overhead Swinging Brutalist Concrete Pendulums
        for (let I = 0; I < Block.PendulumArms.length; ++I)
        {
            const PS = this.Pendulums[I];
            if (!PS)
            {
                this.AssignDynamicOcc(Block.PendulumArms[I],   Parked, false, true);
                this.AssignDynamicOcc(Block.PendulumBlocks[I], Parked, false, true);
                continue;
            }
            // Rotate around X axis by -PS.Angle so local -Z swings toward +Y when PS.Angle > 0
            const QSwing = ConstructAxisQuaternion([1.0, 0.0, 0.0], -PS.Angle);
            this.AssignDynamicOcc(Block.PendulumArms[I],   ConstructRigidTransform([PS.X, PS.CenterY, PS.PivotZ], QSwing), true, false);
            this.AssignDynamicOcc(Block.PendulumBlocks[I], ConstructRigidTransform(PS.BlockPos, QSwing), true, false);
        }

        // ⑨ 2 Expanding Ability VFX Rings ([0] Acoustic Resonance Horn Shock-Cone, [1] Vehicle Hack Pulse Ring)
        if (this.HornPulse)
        {
            const T = this.HornPulse.Age / this.HornPulse.Duration;
            const Dist = 2.0 + T * 24.0;
            const HX = this.HornPulse.Origin[0] + this.HornPulse.Dir[0] * Dist;
            const HY = this.HornPulse.Origin[1] + this.HornPulse.Dir[1] * Dist;
            const Yaw = Math.atan2(this.HornPulse.Dir[1], this.HornPulse.Dir[0]);
            // Tilt ring vertical facing forward along Dir!
            const QVert = MultiplyQuaternion(
                ConstructAxisQuaternion([0.0, 0.0, 1.0], Yaw),
                ConstructAxisQuaternion([0.0, 1.0, 0.0], 0.5 * Math.PI)
            );
            this.AssignDynamicOcc(Block.AbilityRings[0], ConstructRigidTransform([HX, HY, 2.2], QVert), true, false);
        }
        else
            this.AssignDynamicOcc(Block.AbilityRings[0], Parked, false, true);

        if (this.HackPulse)
        {
            const T = this.HackPulse.Age / this.HackPulse.Duration;
            const SpinQ = ConstructAxisQuaternion([0.0, 0.0, 1.0], T * Math.PI * 3.0);
            this.AssignDynamicOcc(Block.AbilityRings[1], ConstructRigidTransform([this.HackPulse.Origin[0], this.HackPulse.Origin[1], 0.8 + T * 4.5], SpinQ), true, false);
        }
        else
            this.AssignDynamicOcc(Block.AbilityRings[1], Parked, false, true);
    }

    QueryPlumeEmissions()
    {
        return [];
    }

    QueryDynamicLightRecords()
    {
        if (!this.Enabled || !this.Maze)
            return [];
        const Lights = [];
        const CarX = this.Host?.Vehicle?.Position?.[0] ?? LabyrinthSpawn.Position[0];

        // ① Active Acoustic Horn / Vehicle Hack flash lights!
        for (const F of this.FlashLights)
        {
            const Fade = ClampScalar(1.0 - F.Age / F.Duration, 0.0, 1.0);
            const I = F.Power * Fade * Fade;
            Lights.push({
                Position:  F.Pos,
                Direction: [0.0, 0.0, -1.0],
                Colour:    [F.Colour[0] * I, F.Colour[1] * I, F.Colour[2] * I],
                Range:     34.0,
                InnerCos: -1.0,
                OuterCos: -1.0
            });
        }
        // ② Nearby Roller Bays (red angry glow vs hacked emerald turbo glow!)
        for (const RS of this.RollerStrips)
        {
            if (Math.abs(RS.CenterX - CarX) < 55.0)
            {
                Lights.push({
                    Position:  [RS.CenterX, RS.CenterY, 2.4],
                    Direction: [0.0, 0.0, -1.0],
                    Colour:    RS.Hacked ? [0.6, 8.5, 3.6] : [8.0, 1.0, 0.5],
                    Range:     18.0,
                    InnerCos: -1.0,
                    OuterCos: -1.0
                });
            }
        }
        // ③ Active Acid-Mist Slick glow + Towering Emerald Extraction Spire Beacon!
        for (const AV of this.AcidVents)
        {
            if (AV.Active && !AV.Cleared && Math.abs(AV.X - CarX) < 55.0)
            {
                Lights.push({
                    Position:  [AV.X, AV.CenterY, 2.2],
                    Direction: [0.0, 0.0, -1.0],
                    Colour:    [2.6, 8.2, 0.8],
                    Range:     18.0,
                    InnerCos: -1.0,
                    OuterCos: -1.0
                });
            }
        }
        // Always illuminate the towering 42 m Emerald Extraction Spire & Blind Split Portals!
        const BeaconPulse = 14.0 + 6.0 * Math.sin(this.Clock * 3.2);
        Lights.push({
            Position:  [206.0, 0.0, 38.0],
            Direction: [0.0, 0.0, -1.0],
            Colour:    [0.25 * BeaconPulse, 1.0 * BeaconPulse, 0.48 * BeaconPulse],
            Range:     65.0,
            InnerCos: -1.0,
            OuterCos: -1.0
        });
        return Lights.slice(0, 14);
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                 CLEAN ON-SCREEN HUD & BOTTOM-LEFT CONTROLS
    //--------------------------------------------------------------------------------------------------------------------

    BuildControlStrip()
    {
        if (typeof document === "undefined")
            return;
        const Strip = document.createElement("div");
        Strip.className = "labyrinth-control-strip";
        Object.assign(Strip.style, {
            position:       "fixed",
            left:           "16px",
            bottom:         "16px",
            zIndex:         "32",
            display:        "flex",
            gap:            "8px",
            alignItems:     "center",
            padding:        "8px 10px",
            borderRadius:   "10px",
            background:     "rgba(12, 16, 22, 0.86)",
            border:         "1px solid rgba(56, 214, 255, 0.42)",
            backdropFilter: "blur(8px)",
            fontFamily:     "ui-monospace, SFMono-Regular, Menlo, monospace",
            fontSize:       "11px",
            color:          "#eaf2fa",
            pointerEvents:  "auto"
        });

        const MkBtn = (Label, AccentBg, OnClick) =>
        {
            const Btn = document.createElement("button");
            Btn.textContent = Label;
            Object.assign(Btn.style, {
                cursor:        "pointer",
                padding:       "6px 10px",
                borderRadius:  "6px",
                border:        "1px solid rgba(255,255,255,0.18)",
                background:    AccentBg,
                color:         "#ffffff",
                fontWeight:    "700",
                fontSize:      "11px",
                letterSpacing: "0.04em"
            });
            Btn.addEventListener("click", (E) => { E.stopPropagation(); OnClick(); });
            return Btn;
        };

        const SeedBadge = document.createElement("span");
        SeedBadge.textContent = `◆ MAZE #${this.Maze.SeedTag}`;
        Object.assign(SeedBadge.style, { color: "#4ae4ff", fontWeight: "700", paddingRight: "4px" });
        Strip.appendChild(SeedBadge);

        Strip.appendChild(MkBtn("◉ HORN [H]",       "rgba(18, 148, 198, 0.90)", () => this.TriggerAcousticHorn()));
        Strip.appendChild(MkBtn("⚡ HACK [Q]",       "rgba(24, 168, 96, 0.90)",  () => this.TriggerVehicleHack()));
        Strip.appendChild(MkBtn("▶ NEW MAZE [N]",   "rgba(218, 114, 22, 0.88)", () => this.Host?.RegenerateLabyrinthMaze?.()));
        Strip.appendChild(MkBtn("↺ RESTART",        "rgba(52, 62, 76, 0.85)",   () => this.Reset()));

        document.body.appendChild(Strip);
        this.ControlStrip = Strip;

        this._KeyHandler = (E) =>
        {
            if (!this.Enabled || E.repeat)
                return;
            const Tag = (E.target?.tagName || "").toUpperCase();
            if (Tag === "INPUT" || Tag === "TEXTAREA" || Tag === "SELECT")
                return;
            if (E.key === "h" || E.key === "H")
                this.TriggerAcousticHorn();
            else if (E.key === "q" || E.key === "Q")
                this.TriggerVehicleHack();
            else if (E.key === "n" || E.key === "N")
                this.Host?.RegenerateLabyrinthMaze?.();
        };
        window.addEventListener("keydown", this._KeyHandler);
    }

    RenderHudOverlay(Ctx, W, H, CameraState, Mode)
    {
        if (!this.Enabled || !this.Maze)
            return;
        if (Mode === "FreeCamera" || Mode === "HMI" || Mode === "RoadStudio")
            return;

        const CarX = this.Host?.Vehicle?.Position?.[0] ?? LabyrinthSpawn.Position[0];
        const Progress = Math.round(ClampScalar((CarX - LabyrinthSpawn.Position[0]) / (LabyrinthFinishX - LabyrinthSpawn.Position[0]), 0.0, 1.0) * 100.0);

        const BarW = Math.min(500, W - 28);
        const BarH = 58;
        const BX   = 0.5 * (W - BarW);
        const BY   = 44; // Cleanly below top AR HUD pill strip — zero overlap!

        Ctx.save();
        Ctx.fillStyle   = "rgba(12, 16, 22, 0.82)";
        Ctx.strokeStyle = this.ActiveSurfaceTag.includes("ACID")
            ? "rgba(130, 245, 48, 0.78)"
            : (this.ActiveSurfaceTag.includes("TURBO") ? "rgba(56, 238, 148, 0.78)" : "rgba(74, 218, 255, 0.58)");
        Ctx.lineWidth = 1.25;
        Ctx.beginPath();
        Ctx.roundRect(BX, BY, BarW, BarH, 10);
        Ctx.fill();
        Ctx.stroke();

        // Row 1: Title + Seed (left) | Progress + Timer (right)
        Ctx.font      = "700 11px ui-monospace, SFMono-Regular, Menlo, monospace";
        Ctx.fillStyle = "#5ae6ff";
        Ctx.textAlign = "left";
        Ctx.fillText(`◆ PISTON LABYRINTH · #${this.Maze.SeedTag}`, BX + 12, BY + 17);

        Ctx.textAlign = "right";
        Ctx.fillStyle = "#e8edf4";
        Ctx.fillText(`${Progress}%  ·  ${this.RunTime.toFixed(1)}s`, BX + BarW - 12, BY + 17);

        // Row 2: Telemetry + Live Horn/Hack Readiness
        const HornTag = this.HornCooldown <= 0.0 ? "HORN[H] READY" : `HORN ${this.HornCooldown.toFixed(1)}s`;
        const HackTag = this.HackCooldown <= 0.0 ? "HACK[Q] READY" : `HACK ${this.HackCooldown.toFixed(1)}s`;
        Ctx.font      = "600 10.5px ui-monospace, SFMono-Regular, Menlo, monospace";
        Ctx.fillStyle = "#cfd9e6";
        Ctx.textAlign = "center";
        Ctx.fillText(
            `TRAPS ${this.TrapsTriggered} · MIRAGE ${this.MirageSmashed} · HACKED ${this.SystemsHacked}  |  ${HornTag} · ${HackTag}`,
            0.5 * W,
            BY + 35
        );

        // Row 3: Extraction progress bar
        const TrackX = BX + 12, TrackY = BY + 44, TrackW = BarW - 24, TrackH = 6;
        Ctx.fillStyle = "rgba(255, 255, 255, 0.14)";
        Ctx.fillRect(TrackX, TrackY, TrackW, TrackH);
        const Grad = Ctx.createLinearGradient(TrackX, 0, TrackX + TrackW, 0);
        Grad.addColorStop(0.0, "#32c8ff");
        Grad.addColorStop(0.6, "#ffb830");
        Grad.addColorStop(1.0, "#34f294");
        Ctx.fillStyle = Grad;
        Ctx.fillRect(TrackX, TrackY, TrackW * (Progress / 100.0), TrackH);

        // Active Surface Hazard Pill or Event Banner cleanly below progress bar
        const PillText = this.BannerTimer > 0.0 && this.BannerText ? this.BannerText : this.ActiveSurfaceTag;
        if (PillText)
        {
            const Alpha = (this.BannerTimer > 0.0 && this.BannerText) ? ClampScalar(this.BannerTimer / 0.45, 0.0, 1.0) : 0.94;
            Ctx.globalAlpha = Alpha;
            const MaxPillW = Math.max(180, W - 24);
            let FontSize = 11.0;
            Ctx.font = `700 ${FontSize}px ui-monospace, SFMono-Regular, Menlo, monospace`;
            let TextW = Ctx.measureText(PillText).width;
            while (TextW > MaxPillW - 22 && FontSize > 8.5)
            {
                FontSize -= 0.5;
                Ctx.font = `700 ${FontSize}px ui-monospace, SFMono-Regular, Menlo, monospace`;
                TextW = Ctx.measureText(PillText).width;
            }
            const TW = Math.min(MaxPillW, TextW + 24);
            const PX = 0.5 * (W - TW);
            const PY = BY + BarH + 8;
            let Bg = "rgba(206, 60, 26, 0.90)";
            let Bd = "rgba(255, 206, 118, 0.80)";
            if (this.BannerTone === "cyan" && this.BannerTimer > 0.0)
            {
                Bg = "rgba(14, 118, 168, 0.90)";
                Bd = "rgba(120, 232, 255, 0.82)";
            }
            else if (this.BannerTone === "emerald" && this.BannerTimer > 0.0 || PillText.includes("TURBO"))
            {
                Bg = "rgba(18, 134, 78, 0.90)";
                Bd = "rgba(110, 255, 178, 0.82)";
            }
            else if (this.BannerTone === "acid" && this.BannerTimer > 0.0 || PillText.includes("ACID"))
            {
                Bg = "rgba(86, 136, 16, 0.92)";
                Bd = "rgba(186, 255, 76, 0.86)";
            }
            Ctx.fillStyle   = Bg;
            Ctx.strokeStyle = Bd;
            Ctx.beginPath();
            Ctx.roundRect(PX, PY, TW, 26, 7);
            Ctx.fill();
            Ctx.stroke();
            Ctx.fillStyle = "#ffffff";
            Ctx.textAlign = "center";
            Ctx.fillText(PillText, 0.5 * W, PY + 17, TW - 16);
        }
        Ctx.restore();
    }
}
