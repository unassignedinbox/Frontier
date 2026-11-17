//============================================================================================================================================
//                                                        DEMOLISHRUNSEQUENCE.JS
//============================================================================================================================================
// 🏗️ Demolish Run · Fracture City ("demolish") — Real-Time Demolition Simulation:
//    • 5 Articulated Giant Wrecking-Ball Cranes swinging cast-iron balls into building facades, triggering real
//      3D Navier-Stokes volumetric dust/fire plumes, 216-chunk physical masonry bursts, bouncing sparks, dynamic
//      blast lights, and cascading jagged Voronoi-fractured boulders!
//    • 12 Proximity-Armed C4 Demolition Charges & 12 Pre-Fractured Collapsing Building Facade Slabs that crack,
//      pitch under rotational gravity, and slam onto the street with shockwave dust clouds & suspension shake!
//    • Direct FractureSequence.OnBreak coupling: smashing through residential garage doors, tempered glass
//      storefronts, interior drywall partitions, and rear brick walls triggers rich material dust/debris clouds
//      and live HUD breach tracking.
//    • One-click "NEW RANDOM CITY" button (and N key) triggers a brand-new procedural & fractured city seed on demand!

import { CourseMaterial } from "./MaterialSpecification.js";
import {
    ConstructRigidTransform,
    ConstructAxisQuaternion,
    MultiplyQuaternion,
    ClampScalar
} from "./TransformProjection.js";
import {
    GetActiveDemolishCity,
    DemolishSpawn,
    DemolishFinishX
} from "./DemolishRunStructure.js";

const Gravity = 9.81;

function ComposeYawPitchRoll(Yaw, Pitch, Roll)
{
    const QZ = ConstructAxisQuaternion([0.0, 0.0, 1.0], Yaw);
    const QY = ConstructAxisQuaternion([0.0, 1.0, 0.0], Pitch);
    const QX = ConstructAxisQuaternion([1.0, 0.0, 0.0], Roll);
    return MultiplyQuaternion(QZ, MultiplyQuaternion(QY, QX));
}

// 📝 Aligns local -Z axis [0, 0, -1] onto unit vector Dir [dx, dy, dz].
function AlignNegZToVector(Dx, Dy, Dz)
{
    const Len = Math.hypot(Dx, Dy, Dz) || 1.0;
    const Ux = Dx / Len, Uy = Dy / Len, Uz = Dz / Len;
    // Cross([0, 0, -1], [Ux, Uy, Uz]) = [Uy, -Ux, 0]
    const Ax = Uy, Ay = -Ux;
    const SinLen = Math.hypot(Ax, Ay);
    if (SinLen < 1.0e-4)
        return Uz < 0.0 ? [0.0, 0.0, 0.0, 1.0] : ConstructAxisQuaternion([1.0, 0.0, 0.0], Math.PI);
    const Angle = Math.acos(ClampScalar(-Uz, -1.0, 1.0));
    return ConstructAxisQuaternion([Ax / SinLen, Ay / SinLen, 0.0], Angle);
}

export class DemolishRunSequence
{
    constructor(Host)
    {
        this.Host    = Host;
        this.Scene   = Host?.Scene || null;
        this.Enabled = (Host?.LevelKey === "demolish") && Boolean(this.Scene?.DemolishBlock);
        this.City    = this.Enabled ? GetActiveDemolishCity() : null;

        this.Clock           = 0.0;
        this.RunTime         = 0.0;
        this.ChaosScale      = 1.0;
        this.CraneSpeedScale = 1.0;
        this.WallsSmashed    = 0;
        this.GaragesBreached = 0;
        this.ExplosionsFired = 0;
        this.Finished        = false;
        this.FinishTime      = 0.0;
        this.BannerText      = "";
        this.BannerTimer     = 0.0;

        this.Cranes        = [];
        this.Charges       = [];
        this.CollapseWalls = [];
        this.FallingChunks = [];
        this.DustPlumes    = [];
        this.FlashLights   = [];
        this.ChunkCursor   = 0;

        this.ControlStrip = null;
        this._KeyHandler  = null;

        if (!this.Enabled)
            return;

        this.InitSimulationState();
        this.BuildControlStrip();
        this.WriteAllDynamicOccupants();
    }

    InitSimulationState()
    {
        const City = this.City;
        this.Clock           = 0.0;
        this.RunTime         = 0.0;
        this.WallsSmashed    = 0;
        this.GaragesBreached = 0;
        this.ExplosionsFired = 0;
        this.Finished        = false;
        this.FinishTime      = 0.0;
        this.DustPlumes      = [];
        this.FlashLights     = [];
        this.ChunkCursor     = 0;

        // ① 5 Giant Articulated Wrecking-Ball Cranes (exact initial jib tip & suspended wrecking ball pose on Frame 0!)
        this.Cranes = City.Cranes.map((C) =>
        {
            const JibYaw = C.BaseYaw + 0.34 * Math.sin(C.SwingPhase);
            const TipX   = C.BasePos[0] + Math.cos(JibYaw) * 16.4;
            const TipY   = C.BasePos[1] + Math.sin(JibYaw) * 16.4;
            const TipZ   = 17.6;
            const Tilt   = 0.42 * Math.sin(C.SwingPhase - 0.55);
            const TangX  = -Math.sin(JibYaw), TangY = Math.cos(JibYaw);
            return {
                ...C,
                Phase:        C.SwingPhase,
                PrevSin:      Math.sin(C.SwingPhase),
                LastHitClock: -5.0,
                JibYaw,
                JibTip:       [TipX, TipY, TipZ],
                BallPos:      [TipX + TangX * 10.5 * Math.sin(Tilt), TipY + TangY * 10.5 * Math.sin(Tilt), TipZ - 10.5 * Math.cos(Tilt)]
            };
        });

        // ② 12 Proximity-Armed C4 Demolition Charges
        this.Charges = City.Charges.map((Ch) => ({
            ...Ch,
            State:     "idle", // idle | armed | detonated
            Countdown: Ch.DelaySeconds,
            Blink:     0.0
        }));

        // ③ 12 Pre-Fractured Collapsing Building Facade Slabs
        this.CollapseWalls = City.CollapseWalls.map((W) => ({
            ...W,
            State:      "standing", // standing | falling | fallen
            TiltAngle:  0.0,
            TiltVel:    0.0,
            HitGround:  false
        }));

        // ④ 36 Pooled Jagged Voronoi-Fractured Rubble Chunks
        this.FallingChunks = Array.from({ length: this.Scene.DemolishBlock.FallingChunks.length }, () => ({
            Active:   false,
            Pos:      [0.0, 0.0, -45.0],
            Vel:      [0.0, 0.0, 0.0],
            Rot:      [0.0, 0.0, 0.0],
            AngVel:   [0.0, 0.0, 0.0],
            Age:      0.0,
            Life:     16.0,
            Bounces:  0
        }));

        this.ShowBanner(`DEMOLISH RUN · SEED #${City.SeedTag} — BREAK GARAGES & THREAD CREVICES!`, 4.2);
    }

    ShowBanner(Text, Duration = 2.6)
    {
        this.BannerText  = Text;
        this.BannerTimer = Duration;
    }

    Reset()
    {
        if (!this.Enabled)
            return;
        this.InitSimulationState();
        this.Host?.Fracture?.ReassembleAll?.();
        this.Host?.Vehicle?.Respawn?.([DemolishSpawn.Position[0], DemolishSpawn.Position[1], 0.65], DemolishSpawn.Yaw);
        this.WriteAllDynamicOccupants();
    }

    Dispose()
    {
        if (this.ControlStrip && this.ControlStrip.parentNode)
            this.ControlStrip.parentNode.removeChild(this.ControlStrip);
        this.ControlStrip = null;
        if (this._KeyHandler && typeof window !== "undefined")
            window.removeEventListener("keydown", this._KeyHandler);
        this._KeyHandler = null;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                       REAL ENGINE DEMOLITION EXPLOSIONS
    //--------------------------------------------------------------------------------------------------------------------

    TriggerDemolitionBlast(X, Y, Z, Scale = 1.4, BlastRadius = 9.5, BlastImpulse = 4.5, DebrisMat = CourseMaterial.DemoBrickRed, Label = null)
    {
        this.ExplosionsFired++;
        const GroundZ = Math.max(0.25, Z);

        // ① Real 3D Navier-Stokes Volumetric Fire + Thick Concrete Dust + Soot Plume!
        this.Host?.Smoke?.Sources?.QueueBurst?.({
            Position: [X, Y, GroundZ + 0.35],
            Radius:   0.78 * Scale,
            Velocity: [0.0, 0.0, 4.4 * Scale],
            Smoke:    42.0 * Scale,
            Heat:     58.0 * Scale,
            Flame:    48.0 * Scale,
            Soot:     38.0 * Scale,
            Seconds:  0.75 + 0.25 * Scale
        });

        // ② Lingering volumetric concrete dust & smoldering embers
        this.DustPlumes.push({
            Pos:      [X, Y, Math.max(0.4, GroundZ)],
            Age:      0.0,
            Duration: 3.4 + 0.8 * Scale,
            Scale
        });
        if (this.DustPlumes.length > 10)
            this.DustPlumes.shift();

        // ③ Dynamic fiery orange point light flash
        this.FlashLights.push({
            Pos:      [X, Y, GroundZ + 1.2],
            Age:      0.0,
            Duration: 0.72,
            Power:    26.0 * Scale
        });
        if (this.FlashLights.length > 8)
            this.FlashLights.shift();

        // ④ Real 216-Chunk Physical Brick / Masonry Debris Fountain (EffectSequence)
        this.Host?.Effects?.BurstTyre?.(
            ConstructRigidTransform([X, Y, GroundZ + 0.3], [0.0, 0.0, 0.0, 1.0]),
            [0.95 * Scale, 0.95 * Scale, 0.65 * Scale],
            [0.0, 0.0, 4.8 * Scale],
            DebrisMat
        );

        // ⑤ Swept-Collision Bouncing Sparks
        const Sparks = [];
        for (let K = 0; K < 10; ++K)
        {
            const Ang = (K / 10) * Math.PI * 2.0 + this.Clock * 2.7;
            const OutSpd = (5.0 + (K % 3) * 2.6) * Scale;
            Sparks.push({
                Point:    [X + Math.cos(Ang) * 0.4, Y + Math.sin(Ang) * 0.4, GroundZ + 0.45],
                Velocity: [Math.cos(Ang) * OutSpd, Math.sin(Ang) * OutSpd, (4.8 + (K % 2) * 3.2) * Scale],
                Normal:   [0.0, 0.0, 1.0],
                Strength: 3.0 * Scale,
                Rim:      null
            });
        }
        this.Host?.Sparks?.Emit?.(Sparks, 1.0 / 60.0);

        // ⑥ Spawn 3–4 Jagged Voronoi-Fractured Masonry Boulders tumbling from the blast!
        const SpawnCount = Math.min(4, Math.max(2, Math.round(2.5 * Scale)));
        for (let K = 0; K < SpawnCount; ++K)
        {
            const Chunk = this.FallingChunks[this.ChunkCursor % this.FallingChunks.length];
            this.ChunkCursor++;
            const Ang = (K / SpawnCount) * Math.PI * 2.0 + this.Clock * 1.9;
            const Horiz = (3.5 + (K % 3) * 2.2) * Scale;
            Chunk.Active  = true;
            Chunk.Pos     = [X + Math.cos(Ang) * 0.6, Y + Math.sin(Ang) * 0.6, Math.max(0.8, GroundZ + 0.5)];
            Chunk.Vel     = [Math.cos(Ang) * Horiz, Math.sin(Ang) * Horiz, (3.8 + (K % 2) * 2.8) * Scale];
            Chunk.Rot     = [Ang, K * 0.7, this.Clock];
            Chunk.AngVel  = [(K % 2 ? 1 : -1) * 4.5, 3.2, -3.8];
            Chunk.Age     = 0.0;
            Chunk.Bounces = 0;
        }

        // ⑦ Trigger nearby intact FractureSequence breakable walls if caught inside blast radius!
        if (this.Host?.Fracture?.Props)
        {
            for (const Prop of this.Host.Fracture.Props)
            {
                if (Prop.State !== "intact")
                    continue;
                const C = Prop.Definition.Centre;
                const Dist = Math.hypot(C[0] - X, C[1] - Y, C[2] - GroundZ);
                if (Dist < BlastRadius * 0.75)
                {
                    const DirX = (C[0] - X) / Math.max(0.5, Dist);
                    const DirY = (C[1] - Y) / Math.max(0.5, Dist);
                    this.Host.Fracture.BreakProp(Prop, {
                        Point:    [C[0], C[1], C[2]],
                        Normal:   [-DirX, -DirY, 0.0],
                        Velocity: [DirX * 12.0, DirY * 12.0, 2.5],
                        Approach: 12.0,
                        Energy:   Math.max(Prop.Recipe.Threshold * 2.2, 18000.0),
                        Vehicle:  null
                    });
                }
            }
        }

        // ⑧ Procedural Sub-Bass Boom Audio + Tactile Suspension Shockwave!
        const Car = this.Host?.Vehicle;
        if (Car)
        {
            const DistCar = Math.hypot(Car.Position[0] - X, Car.Position[1] - Y);
            if (DistCar < 155.0)
            {
                this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(
                    ClampScalar(1.0 - DistCar / 155.0, 0.14, 1.0) * ClampScalar(Scale * 0.65, 0.45, 1.3)
                );
            }
            if (DistCar < BlastRadius * 2.4)
            {
                const Shock = ClampScalar(1.0 - DistCar / (BlastRadius * 2.4), 0.0, 1.0) * Scale * this.ChaosScale;
                const SideSign = (Car.Position[1] - Y) >= 0.0 ? 1.0 : -1.0;
                Car.AngularVelocity[0] += SideSign * 0.55 * Shock;
                Car.AngularVelocity[1] -= 0.36 * Shock;
                if (DistCar < BlastRadius && BlastImpulse > 0.0)
                {
                    const Fall = (1.0 - DistCar / BlastRadius) * this.ChaosScale;
                    const DX = DistCar > 0.3 ? (Car.Position[0] - X) / DistCar : 0.2;
                    const DY = DistCar > 0.3 ? (Car.Position[1] - Y) / DistCar : SideSign;
                    Car.LinearVelocity[0] += DX * BlastImpulse * Fall * 0.45;
                    Car.LinearVelocity[1] += DY * BlastImpulse * Fall * 0.65;
                    Car.LinearVelocity[2] += BlastImpulse * 0.35 * Fall;
                }
            }
            if (Label && DistCar < 68.0)
                this.ShowBanner(Label, 2.2);
        }
    }

    // 📝 Invoked immediately when FractureSequence breaks ANY garage door, glass storefront, drywall or brick wall!
    OnWallBroken(Prop)
    {
        if (!this.Enabled || !Prop?.Definition)
            return;
        this.WallsSmashed++;
        const Tag = Prop.Definition.DemolishTag || "";
        const C = Prop.Definition.Centre || [0.0, 0.0, 1.4];

        let DustMat = CourseMaterial.DemoRubbleConcrete;
        let Banner  = `WALL SMASHED! (${Prop.Pieces.length} FRACTURE SHARDS)`;
        if (Tag === "garage_door")
        {
            this.GaragesBreached++;
            DustMat = CourseMaterial.DemoTimber;
            Banner  = `GARAGE BREACHED! (${Prop.Pieces.length} PANELS) — BLAST OUT THE REAR WALL!`;
        }
        else if (Tag === "storefront_glass")
        {
            this.GaragesBreached++;
            DustMat = CourseMaterial.Glass;
            Banner  = `GLASS STOREFRONT SHATTERED! (${Prop.Pieces.length} SHARDS)`;
        }
        else if (Tag === "interior_wall")
        {
            DustMat = CourseMaterial.DemoInteriorWall;
            Banner  = `INTERIOR PARTITION SMASHED! (${Prop.Pieces.length} CHUNKS)`;
        }
        else if (Tag === "brick_wall" || Tag === "alley_wall")
        {
            DustMat = CourseMaterial.DemoBrickRed;
            Banner  = `BRICK WALL DEMOLISHED! (${Prop.Pieces.length} BRICK SLABS)`;
        }

        this.ShowBanner(Banner, 2.3);

        // Stack billowing plaster/brick dust burst + debris puff + sparks right at the breached doorway!
        this.Host?.Smoke?.Sources?.QueueBurst?.({
            Position: [C[0], C[1], Math.max(0.6, C[2])],
            Radius:   0.85,
            Velocity: [0.0, 0.0, 2.2],
            Smoke:    32.0,
            Heat:     8.0,
            Flame:    0.0,
            Soot:     14.0,
            Seconds:  0.65
        });
        this.Host?.Effects?.BurstTyre?.(
            ConstructRigidTransform([C[0], C[1], Math.max(0.7, C[2])], [0.0, 0.0, 0.0, 1.0]),
            [0.45, 1.4, 0.9],
            [2.0, 0.0, 2.4],
            DustMat
        );
        const Sparks = [];
        for (let K = 0; K < 6; ++K)
        {
            const Ang = (K / 6) * Math.PI * 2.0 + this.Clock;
            Sparks.push({
                Point:    [C[0], C[1] + (K - 2.5) * 0.45, Math.max(0.8, C[2])],
                Velocity: [3.5 + Math.cos(Ang) * 3.0, Math.sin(Ang) * 4.5, 2.5 + (K % 2) * 2.5],
                Normal:   [1.0, 0.0, 0.0],
                Strength: 1.9,
                Rim:      null
            });
        }
        this.Host?.Sparks?.Emit?.(Sparks, 1.0 / 60.0);
        this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.45);
    }

    DetonateNearestCharge()
    {
        if (!this.Enabled)
            return false;
        const CarX = this.Host?.Vehicle?.Position?.[0] ?? -132.0;
        let Best = null;
        let BestDist = Infinity;
        for (const Ch of this.Charges)
        {
            if (Ch.State === "detonated")
                continue;
            const D = Math.abs(Ch.Position[0] - (CarX + 18.0));
            if (D < BestDist)
            {
                BestDist = D;
                Best = Ch;
            }
        }
        if (!Best)
            return false;
        Best.State = "armed";
        Best.Countdown = 0.0;
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                             PER-FRAME SIMULATION STEP
    //--------------------------------------------------------------------------------------------------------------------

    Advance(Seconds)
    {
        if (!this.Enabled || !this.City)
            return false;
        const Dt = Math.min(0.05, Math.max(0.0001, Seconds));
        this.Clock += Dt;
        if (!this.Finished)
            this.RunTime += Dt;
        if (this.BannerTimer > 0.0)
            this.BannerTimer = Math.max(0.0, this.BannerTimer - Dt);

        // Keep demo vehicle fuel & HV battery cleanly topped up so no low-battery warning overlaps HUD
        if (this.Host?.DrivingHud)
        {
            if (this.Host.DrivingHud.FallbackFuel < 0.75)    this.Host.DrivingHud.FallbackFuel    = 0.75;
            if (this.Host.DrivingHud.FallbackBattery < 0.75) this.Host.DrivingHud.FallbackBattery = 0.75;
        }

        const Car = this.Host?.Vehicle;
        const CarPos = Car ? Car.Position : [DemolishSpawn.Position[0], 0.0, 0.5];

        // ① Animate all 5 Giant Demolition Cranes & Swinging Wrecking Balls!
        for (const Crane of this.Cranes)
        {
            const PrevPhase = Crane.Phase;
            Crane.Phase += Dt * Crane.SwingRate * this.CraneSpeedScale;
            const SwingSin = Math.sin(Crane.Phase);
            const SwingCos = Math.cos(Crane.Phase);

            // Crane jib slews smoothly ±0.34 rad toward the targeted building facade
            Crane.JibYaw = Crane.BaseYaw + 0.34 * SwingSin;
            const JibReach = 16.4;
            const TipX = Crane.BasePos[0] + Math.cos(Crane.JibYaw) * JibReach;
            const TipY = Crane.BasePos[1] + Math.sin(Crane.JibYaw) * JibReach;
            const TipZ = 17.6;
            Crane.JibTip = [TipX, TipY, TipZ];

            // Wrecking ball swings as a real trailing pendulum along the crane tangent!
            const PendulumTilt = 0.42 * Math.sin(Crane.Phase - 0.55);
            const TangX = -Math.sin(Crane.JibYaw);
            const TangY =  Math.cos(Crane.JibYaw);
            const CableL = 10.5;
            const BallX = TipX + TangX * (CableL * Math.sin(PendulumTilt));
            const BallY = TipY + TangY * (CableL * Math.sin(PendulumTilt));
            const BallZ = TipZ - CableL * Math.cos(PendulumTilt);
            Crane.BallPos = [BallX, BallY, BallZ];

            // Wrecking ball strikes building facade at peak swing!
            const CrossedPeak = (Math.cos(PrevPhase) > 0.0 && SwingCos <= 0.0);
            if (CrossedPeak && (this.Clock - Crane.LastHitClock) > 2.2)
            {
                Crane.LastHitClock = this.Clock;
                const DistToCar = Math.hypot(CarPos[0] - BallX, CarPos[1] - BallY);
                if (DistToCar < 135.0)
                {
                    this.TriggerDemolitionBlast(
                        BallX, BallY, Math.max(4.5, BallZ),
                        1.25 * this.ChaosScale, 8.5, 3.2,
                        CourseMaterial.DemoBrickRed,
                        DistToCar < 55.0 ? "WRECKING BALL IMPACT — WATCH FOR FALLING MASONRY!" : null
                    );
                }
            }
        }

        // ② Advance Proximity-Armed C4 Demolition Charges -> Detonate & Drop Linked Fractured Facade Slabs!
        for (const Ch of this.Charges)
        {
            if (Ch.State === "idle")
            {
                const Dist = Math.hypot(CarPos[0] - Ch.Position[0], CarPos[1] - Ch.Position[1]);
                if (Dist < Ch.TriggerDist && CarPos[0] > Ch.Position[0] - Ch.TriggerDist)
                {
                    Ch.State = "armed";
                    Ch.Countdown = Ch.DelaySeconds;
                }
            }
            if (Ch.State === "armed")
            {
                Ch.Countdown -= Dt * this.ChaosScale;
                Ch.Blink += Dt * (8.0 + 18.0 * ClampScalar(1.0 - Ch.Countdown / Math.max(0.2, Ch.DelaySeconds), 0.0, 1.0));
                if (Ch.Countdown <= 0.0)
                {
                    Ch.State = "detonated";
                    this.TriggerDemolitionBlast(
                        Ch.Position[0], Ch.Position[1], Ch.Position[2],
                        1.55 * this.ChaosScale, 10.5, 4.8,
                        CourseMaterial.DemoRubbleConcrete,
                        "DEMOLITION CHARGE DETONATED — FACADE COLLAPSING!"
                    );
                    const Wall = this.CollapseWalls[Ch.Index];
                    if (Wall && Wall.State === "standing")
                    {
                        Wall.State   = "falling";
                        Wall.TiltVel = 0.55;
                    }
                }
            }
        }

        // ③ Physically Tip & Slam Pre-Fractured Collapsing Building Facade Slabs Under Rotational Gravity!
        for (const Wall of this.CollapseWalls)
        {
            if (Wall.State === "falling")
            {
                // Inverted pendulum angular acceleration: α = (g / L_eff) * sin(θ) + initial blast kick
                const Alpha = (Gravity / 2.2) * Math.sin(Math.max(0.18, Wall.TiltAngle)) + 0.85;
                Wall.TiltVel   += Alpha * Dt;
                Wall.TiltAngle += Wall.TiltVel * Dt;
                if (Wall.TiltAngle >= 1.46) // ~84° flat onto street rubble!
                {
                    Wall.TiltAngle = 1.46;
                    Wall.TiltVel   = 0.0;
                    Wall.State     = "fallen";
                    if (!Wall.HitGround)
                    {
                        Wall.HitGround = true;
                        const ImpactY = Wall.AnchorPos[1] + Wall.FallSign * 2.4;
                        this.Host?.Smoke?.Sources?.QueueBurst?.({
                            Position: [Wall.AnchorPos[0], ImpactY, 0.45],
                            Radius:   1.35,
                            Velocity: [0.0, Wall.FallSign * 2.5, 3.2],
                            Smoke:    44.0,
                            Heat:     10.0,
                            Flame:    0.0,
                            Soot:     22.0,
                            Seconds:  0.85
                        });
                        this.Host?.DrivingHud?.Audio?.PlayExplosionBoom?.(0.58);
                    }
                }
            }
            // Soft physical shove if player clips a newly fallen facade slab (never hard-traps the car!)
            if (Car && Wall.State !== "standing")
            {
                const SlabCenterY = Wall.AnchorPos[1] + Wall.FallSign * (2.3 * Math.sin(Wall.TiltAngle));
                const DX = CarPos[0] - Wall.AnchorPos[0];
                const DY = CarPos[1] - SlabCenterY;
                if (Math.abs(DX) < 2.6 && Math.abs(DY) < 1.8 && CarPos[2] < 2.4)
                {
                    const PushSign = DY >= 0.0 ? 1.0 : -1.0;
                    Car.LinearVelocity[1] += PushSign * 14.0 * Dt;
                    Car.LinearVelocity[2] += 1.4 * Dt;
                    Car.AngularVelocity[0] += PushSign * 0.9 * Dt;
                }
            }
        }

        // ④ Simulate 36 Tumbling Fractured Masonry Rubble Chunks with Gravity & Road Bounces!
        for (const Chunk of this.FallingChunks)
        {
            if (!Chunk.Active)
                continue;
            Chunk.Age += Dt;
            if (Chunk.Age > Chunk.Life)
            {
                Chunk.Active = false;
                Chunk.Pos[2] = -45.0;
                continue;
            }
            Chunk.Vel[2] -= Gravity * Dt;
            Chunk.Pos[0] += Chunk.Vel[0] * Dt;
            Chunk.Pos[1] += Chunk.Vel[1] * Dt;
            Chunk.Pos[2] += Chunk.Vel[2] * Dt;
            Chunk.Rot[0] += Chunk.AngVel[0] * Dt;
            Chunk.Rot[1] += Chunk.AngVel[1] * Dt;
            Chunk.Rot[2] += Chunk.AngVel[2] * Dt;

            if (Chunk.Pos[2] < 0.42)
            {
                Chunk.Pos[2] = 0.42;
                if (Math.abs(Chunk.Vel[2]) > 2.2)
                {
                    Chunk.Vel[2] = -Chunk.Vel[2] * 0.28;
                    Chunk.Vel[0] *= 0.62;
                    Chunk.Vel[1] *= 0.62;
                    Chunk.AngVel[0] *= 0.55;
                    Chunk.AngVel[1] *= 0.55;
                    Chunk.Bounces++;
                }
                else
                {
                    Chunk.Vel[0] *= Math.max(0.0, 1.0 - 4.5 * Dt);
                    Chunk.Vel[1] *= Math.max(0.0, 1.0 - 4.5 * Dt);
                    Chunk.Vel[2]  = 0.0;
                    Chunk.AngVel[0] *= 0.85;
                    Chunk.AngVel[1] *= 0.85;
                    Chunk.AngVel[2] *= 0.85;
                }
            }
            // Car knocks loose masonry boulders aside with sparks!
            if (Car)
            {
                const DX = Chunk.Pos[0] - CarPos[0];
                const DY = Chunk.Pos[1] - CarPos[1];
                const D2 = DX * DX + DY * DY;
                if (D2 < 2.25 && Chunk.Pos[2] < 1.8)
                {
                    const D = Math.sqrt(D2) || 0.5;
                    Chunk.Vel[0] = Car.LinearVelocity[0] * 1.15 + (DX / D) * 6.5;
                    Chunk.Vel[1] = Car.LinearVelocity[1] * 1.15 + (DY / D) * 6.5;
                    Chunk.Vel[2] = 3.8;
                }
            }
        }

        // ⑤ Decay Lingering Volumetric Dust Plumes & Fiery Blast Lights
        for (let I = this.DustPlumes.length - 1; I >= 0; --I)
        {
            this.DustPlumes[I].Age += Dt;
            if (this.DustPlumes[I].Age >= this.DustPlumes[I].Duration)
                this.DustPlumes.splice(I, 1);
        }
        for (let I = this.FlashLights.length - 1; I >= 0; --I)
        {
            this.FlashLights[I].Age += Dt;
            if (this.FlashLights[I].Age >= this.FlashLights[I].Duration)
                this.FlashLights.splice(I, 1);
        }

        // ⑥ Check Extraction Finish Arch at DemolishFinishX (+216 m)
        if (!this.Finished && CarPos[0] >= DemolishFinishX)
        {
            this.Finished   = true;
            this.FinishTime = this.RunTime;
            this.ShowBanner(
                `EXTRACTION CLEAR! ${this.FinishTime.toFixed(1)}s · ${this.WallsSmashed} WALLS · ${this.GaragesBreached} GARAGES — PRESS NEW CITY!`,
                6.5
            );
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
        const Block = this.Scene?.DemolishBlock;
        if (!Block)
            return;
        const Parked = ConstructRigidTransform([0.0, 0.0, -45.0], [0.0, 0.0, 0.0, 1.0]);

        // ① 5 Articulated Demolition Cranes (Base, Rotating Tower, Jib, Tensioned Hoist Cable, Cast-Iron Wrecking Ball)
        for (let I = 0; I < Block.CraneBases.length; ++I)
        {
            const Crane = this.Cranes[I];
            if (!Crane)
            {
                this.AssignDynamicOcc(Block.CraneBases[I],    Parked, false, true);
                this.AssignDynamicOcc(Block.CraneTowers[I],   Parked, false, true);
                this.AssignDynamicOcc(Block.CraneJibs[I],     Parked, false, true);
                this.AssignDynamicOcc(Block.CraneCables[I],   Parked, false, true);
                this.AssignDynamicOcc(Block.WreckingBalls[I], Parked, false, true);
                continue;
            }
            const QBase  = ConstructAxisQuaternion([0.0, 0.0, 1.0], Crane.BaseYaw);
            const QJib   = ConstructAxisQuaternion([0.0, 0.0, 1.0], Crane.JibYaw);
            const BaseT  = ConstructRigidTransform(Crane.BasePos, QBase);
            const TowerT = ConstructRigidTransform([Crane.BasePos[0], Crane.BasePos[1], 2.15], QJib);
            const JibT   = ConstructRigidTransform([Crane.BasePos[0], Crane.BasePos[1], 17.6], QJib);

            const MidCable = [
                0.5 * (Crane.JibTip[0] + Crane.BallPos[0]),
                0.5 * (Crane.JibTip[1] + Crane.BallPos[1]),
                0.5 * (Crane.JibTip[2] + Crane.BallPos[2])
            ];
            const QCable = AlignNegZToVector(
                Crane.BallPos[0] - Crane.JibTip[0],
                Crane.BallPos[1] - Crane.JibTip[1],
                Crane.BallPos[2] - Crane.JibTip[2]
            );
            const CableT = ConstructRigidTransform(MidCable, QCable);
            const BallT  = ConstructRigidTransform(Crane.BallPos, QCable);

            this.AssignDynamicOcc(Block.CraneBases[I],    BaseT,  false, false);
            this.AssignDynamicOcc(Block.CraneTowers[I],   TowerT, true,  false);
            this.AssignDynamicOcc(Block.CraneJibs[I],     JibT,   true,  false);
            this.AssignDynamicOcc(Block.CraneCables[I],   CableT, true,  false);
            this.AssignDynamicOcc(Block.WreckingBalls[I], BallT,  true,  false);
        }

        // ② 12 C4 Demolition Charge Packs (hidden once detonated!)
        for (let I = 0; I < Block.ChargePacks.length; ++I)
        {
            const Ch = this.Charges[I];
            if (!Ch || Ch.State === "detonated")
            {
                this.AssignDynamicOcc(Block.ChargePacks[I], Parked, false, true);
                continue;
            }
            const Q = ConstructAxisQuaternion([0.0, 0.0, 1.0], this.Clock * (Ch.State === "armed" ? 5.0 : 0.6) + I);
            this.AssignDynamicOcc(Block.ChargePacks[I], ConstructRigidTransform(Ch.Position, Q), true, false);
        }

        // ③ 12 Pre-Fractured Collapsing Building Facade Slabs
        for (let I = 0; I < Block.CollapseWalls.length; ++I)
        {
            const Wall = this.CollapseWalls[I];
            if (!Wall)
            {
                this.AssignDynamicOcc(Block.CollapseWalls[I], Parked, false, true);
                continue;
            }
            // Tip forward around local X axis (Roll = -FallSign * TiltAngle) so top falls inward toward street!
            const QWall = ComposeYawPitchRoll(Wall.Yaw, 0.0, -Wall.TiltAngle);
            this.AssignDynamicOcc(Block.CollapseWalls[I], ConstructRigidTransform(Wall.AnchorPos, QWall), Wall.State === "falling", false);
        }

        // ④ 36 Tumbling Fractured Masonry Rubble Chunks
        for (let I = 0; I < Block.FallingChunks.length; ++I)
        {
            const Chunk = this.FallingChunks[I];
            if (!Chunk || !Chunk.Active)
            {
                this.AssignDynamicOcc(Block.FallingChunks[I], Parked, false, true);
                continue;
            }
            const QChunk = ComposeYawPitchRoll(Chunk.Rot[2], Chunk.Rot[1], Chunk.Rot[0]);
            this.AssignDynamicOcc(Block.FallingChunks[I], ConstructRigidTransform(Chunk.Pos, QChunk), true, false);
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                           VOLUMETRIC SMOKE PLUMES & DYNAMIC DEMOLITION BLAST LIGHTS
    //--------------------------------------------------------------------------------------------------------------------

    QueryPlumeEmissions()
    {
        if (!this.Enabled || this.DustPlumes.length === 0)
            return [];
        const Out = [];
        for (const P of this.DustPlumes)
        {
            const LifeFrac = ClampScalar(1.0 - P.Age / P.Duration, 0.0, 1.0);
            Out.push({
                Position:        [P.Pos[0], P.Pos[1], P.Pos[2]],
                Radius:          0.75 * P.Scale,
                Expansion:       3.2,
                SmokeRate:       24.0 * LifeFrac * P.Scale,
                SootRate:        12.0 * LifeFrac * P.Scale,
                FlameRate:       LifeFrac > 0.65 ? (18.0 * (LifeFrac - 0.65) * P.Scale) : 0.0,
                HeatRate:        22.0 * LifeFrac * P.Scale,
                Velocity:        [0.0, 0.0, 2.2],
                Plume:           null,
                CombustionPower: 0.0,
                Tread:           [0.0, 0.0, 0.0]
            });
        }
        return Out;
    }

    QueryDynamicLightRecords()
    {
        if (!this.Enabled)
            return [];
        const Lights = [];
        // ① Active demolition explosion flash point lights!
        for (const F of this.FlashLights)
        {
            const Fade = ClampScalar(1.0 - F.Age / F.Duration, 0.0, 1.0);
            const Intensity = F.Power * Fade * Fade;
            Lights.push({
                Position:  F.Pos,
                Direction: [0.0, 0.0, -1.0],
                Colour:    [1.0 * Intensity, 0.42 * Intensity, 0.10 * Intensity],
                Range:     28.0,
                InnerCos: -1.0,
                OuterCos: -1.0
            });
        }
        // ② Blinking armed C4 red warning strobes + warm interior garage lights near car!
        const CarX = this.Host?.Vehicle?.Position?.[0] ?? -132.0;
        for (const Ch of this.Charges)
        {
            if (Ch.State !== "armed")
                continue;
            const Pulse = Math.sin(Ch.Blink) > 0.0 ? 8.5 : 1.0;
            Lights.push({
                Position:  [Ch.Position[0], Ch.Position[1], Ch.Position[2] + 0.5],
                Direction: [0.0, 0.0, -1.0],
                Colour:    [Pulse, 0.08 * Pulse, 0.05 * Pulse],
                Range:     14.0,
                InnerCos: -1.0,
                OuterCos: -1.0
            });
        }
        for (const Hero of this.City.HeroBuildings)
        {
            if (Math.abs(Hero.Center[0] - CarX) < 55.0)
            {
                Lights.push({
                    Position:  [Hero.Center[0], Hero.Center[1], 2.9],
                    Direction: [0.0, 0.0, -1.0],
                    Colour:    [5.2, 3.8, 2.2],
                    Range:     16.0,
                    InnerCos: -1.0,
                    OuterCos: -1.0
                });
            }
        }
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
        Strip.className = "demolish-control-strip";
        Object.assign(Strip.style, {
            position:      "fixed",
            left:          "16px",
            bottom:        "16px",
            zIndex:        "32",
            display:       "flex",
            gap:           "8px",
            alignItems:    "center",
            padding:       "8px 10px",
            borderRadius:  "10px",
            background:    "rgba(14, 16, 20, 0.84)",
            border:        "1px solid rgba(255, 168, 42, 0.42)",
            backdropFilter: "blur(8px)",
            fontFamily:    "ui-monospace, SFMono-Regular, Menlo, monospace",
            fontSize:      "11px",
            color:         "#f4efe6",
            pointerEvents: "auto"
        });

        const MkBtn = (Label, AccentBg, OnClick) =>
        {
            const Btn = document.createElement("button");
            Btn.textContent = Label;
            Object.assign(Btn.style, {
                cursor:       "pointer",
                padding:      "6px 10px",
                borderRadius: "6px",
                border:       "1px solid rgba(255,255,255,0.18)",
                background:   AccentBg,
                color:        "#ffffff",
                fontWeight:   "700",
                fontSize:     "11px",
                letterSpacing: "0.04em"
            });
            Btn.addEventListener("click", (E) => { E.stopPropagation(); OnClick(); });
            return Btn;
        };

        const SeedBadge = document.createElement("span");
        SeedBadge.textContent = `◆ SEED #${this.City.SeedTag}`;
        Object.assign(SeedBadge.style, { color: "#ffb84d", fontWeight: "700", paddingRight: "4px" });
        Strip.appendChild(SeedBadge);

        Strip.appendChild(MkBtn("▶ NEW RANDOM CITY", "rgba(235, 108, 22, 0.88)", () => this.Host?.RegenerateDemolishCity?.()));
        Strip.appendChild(MkBtn("▲ BLAST",           "rgba(196, 38, 34, 0.85)",  () => this.DetonateNearestCharge()));
        Strip.appendChild(MkBtn("↺ RESTART",         "rgba(52, 62, 76, 0.85)",   () => this.Reset()));

        document.body.appendChild(Strip);
        this.ControlStrip = Strip;

        this._KeyHandler = (E) =>
        {
            if (!this.Enabled || E.repeat)
                return;
            const Tag = (E.target?.tagName || "").toUpperCase();
            if (Tag === "INPUT" || Tag === "TEXTAREA" || Tag === "SELECT")
                return;
            if (E.key === "n" || E.key === "N")
                this.Host?.RegenerateDemolishCity?.();
            else if (E.key === "b" || E.key === "B")
                this.DetonateNearestCharge();
        };
        window.addEventListener("keydown", this._KeyHandler);
    }

    RenderHudOverlay(Ctx, W, H, CameraState, Mode)
    {
        if (!this.Enabled || !this.City)
            return;
        if (Mode === "FreeCamera" || Mode === "HMI" || Mode === "RoadStudio")
            return;

        const CarX = this.Host?.Vehicle?.Position?.[0] ?? DemolishSpawn.Position[0];
        const Progress = Math.round(ClampScalar((CarX - DemolishSpawn.Position[0]) / (DemolishFinishX - DemolishSpawn.Position[0]), 0.0, 1.0) * 100.0);

        // Sleek compact top-center Demolition Run bar cleanly below the top AR HUD pill bar (zero overlap at any width!)
        const BarW = Math.min(460, W - 28);
        const BarH = 56;
        const BX   = 0.5 * (W - BarW);
        const BY   = 44;

        Ctx.save();
        Ctx.fillStyle = "rgba(14, 16, 20, 0.80)";
        Ctx.strokeStyle = "rgba(255, 164, 48, 0.58)";
        Ctx.lineWidth = 1.25;
        Ctx.beginPath();
        Ctx.roundRect(BX, BY, BarW, BarH, 10);
        Ctx.fill();
        Ctx.stroke();

        // Row 1: Title + Seed (left) and Progress + Timer (right)
        Ctx.font = "700 11px ui-monospace, SFMono-Regular, Menlo, monospace";
        Ctx.fillStyle = "#ffb84d";
        Ctx.textAlign = "left";
        Ctx.fillText(`◆ DEMOLISH RUN · #${this.City.SeedTag}`, BX + 12, BY + 17);

        Ctx.textAlign = "right";
        Ctx.fillStyle = "#e8edf4";
        Ctx.fillText(`${Progress}%  ·  ${this.RunTime.toFixed(1)}s`, BX + BarW - 12, BY + 17);

        // Row 2: Compact demolition breach counters (centered, zero collision!)
        Ctx.font = "600 10.5px ui-monospace, SFMono-Regular, Menlo, monospace";
        Ctx.fillStyle = "#d2dae6";
        Ctx.textAlign = "center";
        Ctx.fillText(`WALLS ${this.WallsSmashed}   ·   GARAGES ${this.GaragesBreached}   ·   BLASTS ${this.ExplosionsFired}`, 0.5 * W, BY + 34);

        // Row 3: Sleek amber-to-emerald extraction progress bar
        const TrackX = BX + 12, TrackY = BY + 42, TrackW = BarW - 24, TrackH = 6;
        Ctx.fillStyle = "rgba(255, 255, 255, 0.14)";
        Ctx.fillRect(TrackX, TrackY, TrackW, TrackH);
        const Grad = Ctx.createLinearGradient(TrackX, 0, TrackX + TrackW, 0);
        Grad.addColorStop(0.0, "#ff7b1a");
        Grad.addColorStop(0.7, "#ffbe33");
        Grad.addColorStop(1.0, "#3be88c");
        Ctx.fillStyle = Grad;
        Ctx.fillRect(TrackX, TrackY, TrackW * (Progress / 100.0), TrackH);

        // Live event / breach / blast alert pill below bar
        if (this.BannerTimer > 0.0 && this.BannerText)
        {
            const Alpha = ClampScalar(this.BannerTimer / 0.45, 0.0, 1.0);
            Ctx.globalAlpha = Alpha;
            Ctx.font = "700 11px ui-monospace, SFMono-Regular, Menlo, monospace";
            const TW = Math.min(W - 36, Ctx.measureText(this.BannerText).width + 26);
            const PX = 0.5 * (W - TW);
            const PY = BY + BarH + 8;
            Ctx.fillStyle = "rgba(210, 62, 24, 0.88)";
            Ctx.strokeStyle = "rgba(255, 214, 130, 0.78)";
            Ctx.beginPath();
            Ctx.roundRect(PX, PY, TW, 26, 7);
            Ctx.fill();
            Ctx.stroke();
            Ctx.fillStyle = "#ffffff";
            Ctx.textAlign = "center";
            Ctx.fillText(this.BannerText, 0.5 * W, PY + 17);
        }
        Ctx.restore();
    }
}
