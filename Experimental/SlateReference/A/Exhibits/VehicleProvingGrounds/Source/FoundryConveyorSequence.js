//============================================================================================================================================
//                                                      FOUNDRYCONVEYORSEQUENCE.JS
//============================================================================================================================================
// ◆ Assembly Line · Conveyor Gauntlet ("foundry") — Real-Time Gameplay Simulation:
//    • TRUE physical moving-conveyor tyre contact mechanics via Vehicle.SurfaceVelocityQuery (every wheel reads the live
//      conveyor speed under its patch, visibly spinning & dragging/boosting the chassis!) + 72 scrolling ribbed 3D steel
//      conveyor tread slats conveying exact fast/slow/reverse/express lane motion right beneath your wheels.
//    • 42 live processed industrial cargo objects (18 ferromagnetic Steel Crates, 14 ferromagnetic Gear Drums, 10 glowing
//      Molten Steel Ingots) riding the 3 belts, colliding physically with the vehicle, AND lifting off the belt into
//      energized Electromagnet Robots!
//    • 6 Articulated Electromagnet Robots (4 track-side robotic turrets + 2 overhead traveling bridge gantry cranes) that
//      attract BOTH ferromagnetic cargo parts AND the player's vehicle with inverse-distance pull forces, part-capture
//      saturation counterplay, dynamic cyan lights, and crackling 3D-projected electromagnetic tractor-beam arcs!
//    • 4 Hydraulic Stamping Presses, 4 Pneumatic Side-Pusher Rejectors, 3 Spinning Rotary Sweeper Drums, and 1 Thermal
//      Eddy-Brake Scanner Gate engineered to slow down & stop players before the Quality Control Finish Gate (X = +238 m).

import { LightType } from "./LightingSystem.js";
import {
    ClampScalar,
    ConstructAxisQuaternion,
    ConstructRigidTransform,
    MultiplyQuaternion
} from "./TransformProjection.js";
import {
    FoundryGoalX,
    FoundryMagRobotSites,
    FoundryPoolCounts,
    FoundryPressSites,
    FoundryPusherSites,
    FoundryScannerSites,
    FoundrySpawn,
    FoundrySweeperSites,
    QueryFoundryConveyorVelocity
} from "./FoundryConveyorStructure.js";

const SlatCountPerLane = Math.floor(FoundryPoolCounts.BeltSlats / 3); // 24 scrolling cleats × 3 lanes = 72
const SlatSpacing      = 6.5;                                         // [m] tight cleat pitch so belt speed pops visually
const SlatWindowSpan   = SlatCountPerLane * SlatSpacing;              // 156 m active scrolling window around the car
const CargoCrateCount  = FoundryPoolCounts.CargoCrates;               // 18
const CargoDrumCount   = FoundryPoolCounts.CargoDrums;                // 14
const CargoIngotCount  = FoundryPoolCounts.CargoIngots;               // 10

const LaneCenterY = Object.freeze([-8.7, 0.0, 8.7]); // South, Center, North lane centers [m]

function SmoothStep01(T)
{
    const C = Math.max(0.0, Math.min(1.0, T));
    return C * C * (3.0 - 2.0 * C);
}

export class FoundryConveyorSequence
{
    constructor(Host)
    {
        this.Host    = Host;
        this.Scene   = Host?.Scene || null;
        this.Enabled = (Host?.LevelKey === "foundry") && Boolean(this.Scene?.FoundryBlock);
        this.Clock   = 0.0;
        this.RunTimer   = 0.0;
        this.BestRunSec = null;
        this.Finished   = false;
        this.SpeedMultiplier = 1.0; // User toggle: 0.65 Slow / 1.0 Standard / 1.4 Overdrive

        // Live telemetry for HUD & tests
        this.CurrentBelt        = { OnBelt: true, SpeedX: -7.2, Lane: "center", PhaseLabel: "FAST COUNTER-BELT", Surge: 0.0 };
        this.ActiveMagnetPull   = 0.0;   // [m/s²] current magnetic pull strength on player's car
        this.PullingRobotIndex  = -1;    // index of strongest MagBot pulling the car
        this.EddyBrakeTimer     = 0.0;   // [s] temporary slowdown after crossing a RED scanner gate
        this.HitCooldown        = 0.0;   // [s] brief hit-stun immunity so one obstacle doesn't multi-trigger
        this.HitsTaken          = 0;
        this.BannerMessage      = "";
        this.BannerTimer        = 0.0;
        this.ControlStrip       = null;

        // 72 Scrolling 3D steel+yellow ribbed belt cleats (24 per lane, tightly spaced at 6.5 m)
        this.Slats = [];
        for (let LaneIdx = 0; LaneIdx < 3; ++LaneIdx)
        {
            for (let S = 0; S < SlatCountPerLane; ++S)
            {
                this.Slats.push({
                    LaneIdx,
                    Y: LaneCenterY[LaneIdx],
                    X: -148.0 + S * SlatSpacing,
                    Wrapped: true
                });
            }
        }

        // 42 Processed industrial cargo objects riding along the 3 conveyor lanes
        // (18 Steel Crates + 14 Gear Drums are strongly ferromagnetic and lifted by Electromagnet Robots!)
        this.CargoItems = [];
        const MakeCargo = (Kind, PoolIdx, Index, Total, Magnetic) =>
        {
            const LaneIdx = (Index * 2 + (Kind === "drum" ? 1 : (Kind === "ingot" ? 2 : 0))) % 3;
            const BaseX   = -126.0 + (Index / Total) * 354.0;
            const LatOff  = ((Index % 3) - 1) * 1.15;
            return {
                Kind,          // "crate" | "drum" | "ingot"
                PoolIdx,
                Magnetic,
                Radius: Kind === "crate" ? 1.38 : (Kind === "drum" ? 1.05 : 1.18),
                BaseZ:  0.02,
                Pos:    [BaseX, LaneCenterY[LaneIdx] + LatOff, 0.02],
                Vel:    [0.0, 0.0, 0.0],
                Yaw:    (Index * 0.7) % (Math.PI * 2.0),
                State:  "belt",      // "belt" | "attracted" | "captured" | "falling"
                MagnetIndex: -1,
                OrbitAngle:  (Index * 1.9) % (Math.PI * 2.0),
                OrbitRadius: 1.35 + (Index % 3) * 0.55,
                Wrapped: true
            };
        };
        for (let I = 0; I < CargoCrateCount; ++I) this.CargoItems.push(MakeCargo("crate", I, I, CargoCrateCount, true));
        for (let I = 0; I < CargoDrumCount;  ++I) this.CargoItems.push(MakeCargo("drum",  I, I, CargoDrumCount,  true));
        for (let I = 0; I < CargoIngotCount; ++I) this.CargoItems.push(MakeCargo("ingot", I, I, CargoIngotCount, false));

        // 6 Articulated Electromagnet Robots
        this.MagRobots = FoundryMagRobotSites.map((Site, Index) => ({
            Index,
            Site,
            State:         "idle",       // "idle" | "charging" | "active"
            Charge:        0.0,          // [0..1]
            BasePos:       [Site.X, Site.Y, Site.BaseZ],
            ArmYaw:        Site.Side > 0 ? -Math.PI * 0.5 : Math.PI * 0.5,
            HeadPos:       [Site.X, Site.Kind === "turret" ? Site.ReachY : 0.0, Site.HeadZ],
            RingRadius:    0.0,
            CapturedCount: 0,
            PullingCar:    false
        }));

        // 4 Hydraulic Stamping Presses
        this.Presses = FoundryPressSites.map((Site) => ({
            Site,
            RamZ:    4.6,
            Slamming: false,
            Warning:  false
        }));

        // 4 Pneumatic Side-Pusher Rejector Arms
        this.Pushers = FoundryPusherSites.map((Site) => ({
            Site,
            Extend: 0.0,
            HeadY:  Site.BaseY,
            Punching: false
        }));

        // 3 Spinning Rotary Sweeper Gates
        this.Sweepers = FoundrySweeperSites.map((Site) => ({
            Site,
            Angle: Site.Phase
        }));

        // 1 Thermal Eddy-Current Brake Scanner Gate
        this.Scanners = FoundryScannerSites.map((Site) => ({
            Site,
            RedActive: false,
            BeamZ:     4.6
        }));

        if (this.Enabled)
        {
            this.AttachVehicleBeltPhysics();
            this.CreateControlStrip();
            // Advance 1 initial tick so MagBot-1 & presses start in live readable motion immediately on Frame 0!
            this.Advance(0.016);
        }
    }

    // 📝 Hooks real per-wheel moving-belt surface velocity directly into VehicleSolver's tyre contact solver!
    AttachVehicleBeltPhysics()
    {
        const Car = this.Host?.Vehicle;
        if (!Car)
            return;
        Car.SurfaceVelocityQuery = (X, Y, Z) =>
        {
            if (Z > 1.45)
                return null;
            const B = QueryFoundryConveyorVelocity(X, Y, this.Clock, this.SpeedMultiplier);
            return B.OnBelt ? [B.SpeedX, 0.0, 0.0] : null;
        };
    }

    CreateControlStrip()
    {
        if (typeof document === "undefined")
            return;
        const Existing = document.getElementById("foundry-controls");
        if (Existing)
            Existing.remove();

        const Wrap = document.createElement("div");
        Wrap.id = "foundry-controls";
        Wrap.style.cssText = [
            "position:fixed", "left:50%", "bottom:18px", "transform:translateX(-50%)",
            "z-index:22", "display:flex", "align-items:center", "gap:6px", "padding:6px 10px",
            "border-radius:10px", "background:rgba(12,14,18,0.84)", "border:1px solid rgba(255,204,72,0.32)",
            "backdrop-filter:blur(10px)", "font:600 11.5px 'Inter',system-ui,sans-serif", "color:#e8eef6"
        ].join(";");

        const MakeBtn = (Label, OnClick, Highlight = false) =>
        {
            const B = document.createElement("button");
            B.type = "button";
            B.textContent = Label;
            B.style.cssText = [
                "appearance:none", "padding:5px 10px", "border-radius:6px", "cursor:pointer",
                `border:1px solid ${Highlight ? "#ffcc48" : "rgba(255,255,255,0.16)"}`,
                `background:${Highlight ? "#ffcc48" : "rgba(255,255,255,0.06)"}`,
                `color:${Highlight ? "#0c0e12" : "#e6edf6"}`, "font:inherit"
            ].join(";");
            B.addEventListener("click", OnClick);
            return B;
        };

        const SpeedBtns = [];
        const SetSpeed = (Mult, ActiveIdx) =>
        {
            this.SpeedMultiplier = Mult;
            SpeedBtns.forEach((Btn, Idx) =>
            {
                const On = Idx === ActiveIdx;
                Btn.style.background  = On ? "#ffcc48" : "rgba(255,255,255,0.06)";
                Btn.style.color       = On ? "#0c0e12" : "#e6edf6";
                Btn.style.borderColor = On ? "#ffcc48" : "rgba(255,255,255,0.16)";
            });
            this.Announce(`CONVEYOR SPEED: ${Mult < 0.9 ? "SLOW" : (Mult > 1.1 ? "OVERDRIVE" : "STANDARD")}`);
        };

        const Label = document.createElement("span");
        Label.textContent = "BELT";
        Label.style.cssText = "color:#9aa6b8;font-size:10.5px;letter-spacing:0.08em;margin-right:2px;";
        Wrap.appendChild(Label);

        const SlowBtn = MakeBtn("Slow", () => SetSpeed(0.65, 0), false);
        const NormBtn = MakeBtn("Standard", () => SetSpeed(1.0, 1), true);
        const FastBtn = MakeBtn("Overdrive", () => SetSpeed(1.4, 2), false);
        SpeedBtns.push(SlowBtn, NormBtn, FastBtn);
        Wrap.append(SlowBtn, NormBtn, FastBtn);

        const RestartBtn = MakeBtn("↺ Restart Run", () => this.RestartRun(), false);
        RestartBtn.style.marginLeft = "6px";
        Wrap.appendChild(RestartBtn);

        document.body.appendChild(Wrap);
        this.ControlStrip = Wrap;
    }

    Announce(Text, Duration = 2.0)
    {
        this.BannerMessage = Text;
        this.BannerTimer   = Duration;
    }

    RestartRun()
    {
        const Car = this.Host?.Vehicle;
        if (Car)
            Car.Respawn([FoundrySpawn.Position[0], FoundrySpawn.Position[1], 0.56], FoundrySpawn.Yaw);
        this.RunTimer       = 0.0;
        this.Finished       = false;
        this.HitsTaken      = 0;
        this.EddyBrakeTimer = 0.0;
        this.HitCooldown    = 0.0;
        this.Announce("◆ ASSEMBLY LINE RUN STARTED — REACH QC FINISH GATE!", 2.6);
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                          MAIN SIMULATION & PHYSICS STEP
    //--------------------------------------------------------------------------------------------------------------------

    Advance(Delta)
    {
        if (!this.Enabled)
            return false;

        const Dt = Math.min(Math.max(Delta, 0.0), 0.05);
        this.Clock += Dt;
        if (!this.Finished)
            this.RunTimer += Dt;
        if (this.BannerTimer > 0.0)
            this.BannerTimer = Math.max(0.0, this.BannerTimer - Dt);
        if (this.HitCooldown > 0.0)
            this.HitCooldown = Math.max(0.0, this.HitCooldown - Dt);
        if (this.EddyBrakeTimer > 0.0)
            this.EddyBrakeTimer = Math.max(0.0, this.EddyBrakeTimer - Dt);

        // Keep fallback fuel/battery topped up cleanly in Foundry (no Service Bays on the conveyor!)
        if (this.Host?.DrivingHud)
        {
            this.Host.DrivingHud.FallbackFuel    = Math.max(0.75, this.Host.DrivingHud.FallbackFuel ?? 0.85);
            this.Host.DrivingHud.FallbackBattery = Math.max(0.75, this.Host.DrivingHud.FallbackBattery ?? 0.85);
        }

        const Car = this.Host?.Vehicle || null;
        if (Car && !Car.SurfaceVelocityQuery)
            this.AttachVehicleBeltPhysics();

        const CarPos = Car ? Car.Position : [FoundrySpawn.Position[0], 0.0, 0.55];
        const CarVel = Car ? (Car.LinearVelocity || Car.Velocity) : [0.0, 0.0, 0.0];

        // ① Query live conveyor telemetry at car's center — ALL physical conveyor forces enter purely through the
        //    4 tyre contact patches via Vehicle.SurfaceVelocityQuery inside VehicleSolver (ZERO fake chassis pushing!).
        this.CurrentBelt = QueryFoundryConveyorVelocity(CarPos[0], CarPos[1], this.Clock, this.SpeedMultiplier);

        // Eddy-current brake drag after crossing a RED scanner gate
        if (Car && this.EddyBrakeTimer > 0.0)
        {
            const Damp = Math.exp(-2.8 * Dt);
            CarVel[0] *= Damp;
            CarVel[1] *= Damp;
        }

        // ② Advance 72 scrolling 3D steel+yellow conveyor cleats around the car's active corridor (24 per lane!)
        const WinMinX = ClampScalar(CarPos[0] - 28.0, -150.0, 234.0 - SlatWindowSpan);
        const WinMaxX = WinMinX + SlatWindowSpan;
        for (const Slat of this.Slats)
        {
            const B = QueryFoundryConveyorVelocity(Slat.X, Slat.Y, this.Clock, this.SpeedMultiplier);
            Slat.X += (B.OnBelt ? B.SpeedX : -5.5) * Dt;
            let Wrapped = false;
            while (Slat.X < WinMinX) { Slat.X += SlatWindowSpan; Wrapped = true; }
            while (Slat.X > WinMaxX) { Slat.X -= SlatWindowSpan; Wrapped = true; }
            Slat.X = ClampScalar(Slat.X, -150.0, 234.0);
            Slat.Wrapped = Wrapped;
        }

        // ③ Update 6 Articulated Electromagnet Robots (state machine, gantry sweep, arm tracking & vehicle attraction!)
        this.ActiveMagnetPull  = 0.0;
        this.PullingRobotIndex = -1;
        for (const Bot of this.MagRobots)
        {
            const S = Bot.Site;
            const CycleFrac = ((this.Clock + S.Phase) % S.Cycle) / S.Cycle;
            const [ActStart, ActEnd] = S.ActiveWindow;
            const ChargeStart = Math.max(0.0, ActStart - 0.14);

            if (CycleFrac >= ActStart && CycleFrac <= ActEnd)
            {
                Bot.State  = "active";
                Bot.Charge = 1.0;
            }
            else if (CycleFrac >= ChargeStart && CycleFrac < ActStart)
            {
                Bot.State  = "charging";
                Bot.Charge = SmoothStep01((CycleFrac - ChargeStart) / Math.max(0.01, ActStart - ChargeStart));
            }
            else
            {
                Bot.State  = "idle";
                Bot.Charge = 0.0;
            }

            if (S.Kind === "gantry")
            {
                const SweepY = S.SweepY * Math.sin(this.Clock * 0.72 + S.Phase);
                Bot.BasePos = [S.X, SweepY, S.BaseZ];
                Bot.HeadPos = [S.X, SweepY, S.HeadZ + (Bot.State === "active" ? -0.35 : 0.25)];
                Bot.ArmYaw  = 0.0;
            }
            else
            {
                const DefaultTargetX = S.X + 2.8 * Math.sin(this.Clock * 0.85 + S.Phase);
                const DefaultTargetY = S.ReachY;
                let TargetX = DefaultTargetX, TargetY = DefaultTargetY;
                const DistCar = Math.hypot(CarPos[0] - S.X, CarPos[1] - S.ReachY);
                if (Bot.State !== "idle" && DistCar < S.PullRadius + 4.0)
                {
                    TargetX = S.X + ClampScalar(CarPos[0] - S.X, -6.5, 6.5);
                    TargetY = S.Y + ClampScalar(CarPos[1] - S.Y, -9.5, 9.5);
                }
                const DX = TargetX - S.X;
                const DY = TargetY - S.Y;
                const DesiredYaw = Math.atan2(DY, DX);
                let Diff = DesiredYaw - Bot.ArmYaw;
                while (Diff >  Math.PI) Diff -= 2.0 * Math.PI;
                while (Diff < -Math.PI) Diff += 2.0 * Math.PI;
                Bot.ArmYaw += Diff * Math.min(1.0, Dt * 4.5);

                const ArmLen = 6.8;
                Bot.BasePos = [S.X, S.Y, S.BaseZ];
                Bot.HeadPos = [
                    S.X + Math.cos(Bot.ArmYaw) * ArmLen,
                    S.Y + Math.sin(Bot.ArmYaw) * ArmLen,
                    S.HeadZ
                ];
            }

            Bot.RingRadius = Bot.State === "active"
                ? (0.95 + 0.14 * Math.sin(this.Clock * 9.0 + Bot.Index))
                : (Bot.State === "charging" ? 0.45 * Bot.Charge : 0.0);

            Bot.CapturedCount = 0;
            Bot.PullingCar    = false;
        }

        // ④ Advance 42 Processed Conveyor Cargo Items (belt ride + magnetic lift/orbit/drop + car collisions!)
        for (const Item of this.CargoItems)
        {
            Item.Wrapped = false;
            if (Item.Magnetic)
            {
                if (Item.State === "captured" || Item.State === "attracted")
                {
                    const Bot = this.MagRobots[Item.MagnetIndex];
                    if (!Bot || Bot.State !== "active")
                    {
                        Item.State = "falling";
                        Item.MagnetIndex = -1;
                        Item.Vel[2] = -1.2;
                    }
                    else
                    {
                        Bot.CapturedCount++;
                        Item.OrbitAngle += Dt * 2.4;
                        const TargetX = Bot.HeadPos[0] + Math.cos(Item.OrbitAngle) * Item.OrbitRadius;
                        const TargetY = Bot.HeadPos[1] + Math.sin(Item.OrbitAngle) * Item.OrbitRadius;
                        const TargetZ = Bot.HeadPos[2] - 1.35;
                        const DX = TargetX - Item.Pos[0];
                        const DY = TargetY - Item.Pos[1];
                        const DZ = TargetZ - Item.Pos[2];
                        const Dist = Math.hypot(DX, DY, DZ);
                        if (Dist < 0.75)
                        {
                            Item.State  = "captured";
                            Item.Pos[0] = TargetX;
                            Item.Pos[1] = TargetY;
                            Item.Pos[2] = TargetZ;
                            Item.Vel[0] = Item.Vel[1] = Item.Vel[2] = 0.0;
                        }
                        else
                        {
                            const Pull = 20.5;
                            Item.Vel[0] += (DX / Dist) * Pull * Dt;
                            Item.Vel[1] += (DY / Dist) * Pull * Dt;
                            Item.Vel[2] += (DZ / Dist) * Pull * Dt;
                            const Damp = Math.exp(-2.2 * Dt);
                            Item.Vel[0] *= Damp; Item.Vel[1] *= Damp; Item.Vel[2] *= Damp;
                            Item.Pos[0] += Item.Vel[0] * Dt;
                            Item.Pos[1] += Item.Vel[1] * Dt;
                            Item.Pos[2] += Item.Vel[2] * Dt;
                        }
                        Item.Yaw += Dt * 1.8;
                    }
                }
                else if (Item.State === "belt")
                {
                    for (const Bot of this.MagRobots)
                    {
                        if (Bot.State !== "active" || Bot.CapturedCount >= 4)
                            continue;
                        const Dist = Math.hypot(Item.Pos[0] - Bot.HeadPos[0], Item.Pos[1] - Bot.HeadPos[1]);
                        if (Dist < Bot.Site.PullRadius * 0.92)
                        {
                            Item.State = "attracted";
                            Item.MagnetIndex = Bot.Index;
                            Item.Vel[2] = 4.2;
                            Bot.CapturedCount++;
                            break;
                        }
                    }
                }
            }

            if (Item.State === "falling")
            {
                Item.Vel[2] -= 15.0 * Dt;
                Item.Pos[0] += Item.Vel[0] * Dt;
                Item.Pos[1] += Item.Vel[1] * Dt;
                Item.Pos[2] += Item.Vel[2] * Dt;
                if (Item.Pos[2] <= Item.BaseZ)
                {
                    Item.Pos[2] = Item.BaseZ;
                    Item.Vel[0] = Item.Vel[1] = Item.Vel[2] = 0.0;
                    Item.Pos[1] = ClampScalar(Item.Pos[1], -12.0, 12.0);
                    Item.State  = "belt";
                }
            }
            else if (Item.State === "belt")
            {
                const B = QueryFoundryConveyorVelocity(Item.Pos[0], Item.Pos[1], this.Clock, this.SpeedMultiplier);
                const BeltVX = B.OnBelt ? B.SpeedX : -5.0;
                Item.Pos[0] += (BeltVX + Item.Vel[0]) * Dt;
                Item.Pos[1] = ClampScalar(Item.Pos[1] + Item.Vel[1] * Dt, -12.2, 12.2);
                Item.Vel[0] *= Math.exp(-3.2 * Dt);
                Item.Vel[1] *= Math.exp(-3.2 * Dt);
                if (Item.Kind === "drum")
                    Item.Yaw += BeltVX * 0.25 * Dt;

                // Continuous assembly-line wraparound so processed objects always stream along the belt!
                if (Item.Pos[0] < -148.0)      { Item.Pos[0] = 232.0; Item.Wrapped = true; }
                else if (Item.Pos[0] > 234.0)  { Item.Pos[0] = -146.0; Item.Wrapped = true; }

                // Car-vs-Cargo solid impact!
                if (Car && Item.Pos[2] < 1.4)
                {
                    const DX = CarPos[0] - Item.Pos[0];
                    const DY = CarPos[1] - Item.Pos[1];
                    const Dist = Math.hypot(DX, DY);
                    const MinSep = Item.Radius + 1.55;
                    if (Dist < MinSep && Dist > 1e-4)
                    {
                        const NX = DX / Dist, NY = DY / Dist;
                        const Overlap = MinSep - Dist;
                        CarVel[0] = CarVel[0] * 0.62 + NX * 4.5;
                        CarVel[1] = CarVel[1] * 0.70 + NY * 6.0;
                        Item.Vel[0] -= NX * 9.5;
                        Item.Vel[1] -= NY * 10.5;
                        Item.Pos[0] -= NX * Overlap * 0.7;
                        Item.Pos[1] = ClampScalar(Item.Pos[1] - NY * Overlap * 0.7, -12.2, 12.2);
                        if (this.HitCooldown <= 0.0)
                        {
                            this.HitCooldown = 0.45;
                            this.HitsTaken++;
                            this.Announce(Item.Kind === "ingot" ? "▲ MOLTEN INGOT IMPACT!" : "■ CARGO COLLISION — MOMENTUM LOST!", 1.4);
                        }
                    }
                }
            }
        }

        // ⑤ Apply REAL Magnetic Pull Force to the Player's Vehicle from Active Electromagnet Robots!
        if (Car)
        {
            for (const Bot of this.MagRobots)
            {
                if (Bot.State !== "active")
                    continue;
                const S = Bot.Site;
                const DX = Bot.HeadPos[0] - CarPos[0];
                const DY = Bot.HeadPos[1] - CarPos[1];
                const Dist = Math.hypot(DX, DY);
                if (Dist < S.PullRadius)
                {
                    const Falloff = SmoothStep01(1.0 - Dist / S.PullRadius);
                    const SaturationRelief = 1.0 - Math.min(0.40, Bot.CapturedCount * 0.12);
                    const PullMag = S.PullStrength * (0.28 + 0.72 * Falloff) * SaturationRelief;
                    const InvD = 1.0 / Math.max(1.2, Dist);
                    CarVel[0] += DX * InvD * PullMag * Dt;
                    CarVel[1] += DY * InvD * PullMag * Dt;
                    if (Dist < 6.5 && CarPos[2] < 1.25)
                        CarVel[2] += PullMag * 0.16 * Dt;
                    if (Dist < 5.5)
                    {
                        const CoreDrag = Math.exp(-1.15 * Dt);
                        CarVel[0] *= CoreDrag;
                    }
                    Bot.PullingCar = true;
                    if (PullMag > this.ActiveMagnetPull)
                    {
                        this.ActiveMagnetPull  = PullMag;
                        this.PullingRobotIndex = Bot.Index;
                    }
                }
            }
        }

        // ⑥ 4 Hydraulic Stamping Presses (periodic warning → violent slam → slow retract!)
        for (const P of this.Presses)
        {
            const S = P.Site;
            const Frac = ((this.Clock + S.Phase) % S.Period) / S.Period;
            P.Warning  = Frac >= 0.46 && Frac <= 0.82;
            P.Slamming = Frac >= 0.64 && Frac <= 0.82;
            if (Frac < 0.64)
                P.RamZ = 4.6;
            else if (Frac < 0.72)
                P.RamZ = 4.6 - 4.45 * SmoothStep01((Frac - 0.64) / 0.08);
            else if (Frac < 0.82)
                P.RamZ = 0.15;
            else
                P.RamZ = 0.15 + 4.45 * SmoothStep01((Frac - 0.82) / 0.18);

            if (Car && P.RamZ < 1.85 && Math.abs(CarPos[0] - S.X) < S.HalfX + 1.4 && Math.abs(CarPos[1] - S.Y) < S.HalfY + 1.0)
            {
                CarVel[0] = Math.min(CarVel[0] * 0.20, -4.5);
                CarVel[1] += (CarPos[1] >= S.Y ? 5.5 : -5.5);
                if (this.HitCooldown <= 0.0)
                {
                    this.HitCooldown = 0.75;
                    this.HitsTaken++;
                    this.Announce("▲ STAMPING PRESS SLAMMED — MOMENTUM CRUSHED!", 1.8);
                }
            }
        }

        // ⑦ 4 Pneumatic Side-Pusher Rejector Pistons (punches cargo & cars sideways across lanes!)
        for (const Push of this.Pushers)
        {
            const S = Push.Site;
            const Frac = ((this.Clock + S.Phase) % S.Period) / S.Period;
            let Ext = 0.0;
            if (Frac >= 0.55 && Frac < 0.68)       Ext = SmoothStep01((Frac - 0.55) / 0.13);
            else if (Frac >= 0.68 && Frac < 0.76)  Ext = 1.0;
            else if (Frac >= 0.76 && Frac < 0.95)  Ext = 1.0 - SmoothStep01((Frac - 0.76) / 0.19);
            Push.Extend   = Ext;
            Push.Punching = Frac >= 0.55 && Frac < 0.76;
            const PushDirY = -S.Side;
            Push.HeadY = S.BaseY + PushDirY * (Ext * S.Stroke);

            for (const Item of this.CargoItems)
            {
                if (Item.State === "belt" && Math.abs(Item.Pos[0] - S.X) < 2.2 && Math.abs(Item.Pos[1] - Push.HeadY) < 1.8)
                    Item.Vel[1] += PushDirY * 14.0 * Dt;
            }
            if (Car && Math.abs(CarPos[0] - S.X) < 2.4 && Math.abs(CarPos[1] - Push.HeadY) < 1.9)
            {
                CarVel[1] += PushDirY * (Push.Punching ? 16.5 : 7.0);
                CarVel[0] *= 0.78;
                if (this.HitCooldown <= 0.0)
                {
                    this.HitCooldown = 0.65;
                    this.HitsTaken++;
                    this.Announce("↔ PNEUMATIC REJECTOR SHOVE!", 1.5);
                }
            }
        }

        // ⑧ 3 Spinning Rotary Sweeper Drum Gates
        for (const Sw of this.Sweepers)
        {
            const S = Sw.Site;
            Sw.Angle = (this.Clock * S.Omega + S.Phase) % (Math.PI * 2.0);
            if (Car)
            {
                const DX = CarPos[0] - S.X;
                const DY = CarPos[1] - S.Y;
                const Dist = Math.hypot(DX, DY);
                if (Dist < S.Radius + 1.2)
                {
                    const CA = Math.cos(Sw.Angle), SA = Math.sin(Sw.Angle);
                    const Along = DX * CA + DY * SA;
                    const Perp  = Math.abs(-DX * SA + DY * CA);
                    if (Math.abs(Along) < S.Radius + 0.9 && Perp < 1.65)
                    {
                        const TanSign = S.Omega >= 0 ? 1 : -1;
                        const TangX = -SA * TanSign * (Along >= 0 ? 1 : -1);
                        const TangY =  CA * TanSign * (Along >= 0 ? 1 : -1);
                        CarVel[0] = CarVel[0] * 0.50 + TangX * 9.5;
                        CarVel[1] = CarVel[1] * 0.55 + TangY * 10.5;
                        if (this.HitCooldown <= 0.0)
                        {
                            this.HitCooldown = 0.60;
                            this.HitsTaken++;
                            this.Announce("◎ ROTARY SWEEPER DEFLECTION!", 1.4);
                        }
                    }
                }
            }
        }

        // ⑨ Thermal Eddy-Current Brake Scanner Gate (guards the Turbo Express Center Strip!)
        for (const Sc of this.Scanners)
        {
            const S = Sc.Site;
            const Frac = ((this.Clock + S.Phase) % S.Period) / S.Period;
            Sc.RedActive = Frac >= 0.45 && Frac <= 0.88;
            Sc.BeamZ     = Sc.RedActive ? (1.25 + 0.55 * Math.sin(this.Clock * 7.5)) : 5.35;

            if (Car && Sc.RedActive && Math.abs(CarPos[0] - S.X) < S.HalfX && Math.abs(CarPos[1] - S.Y) < S.HalfY)
            {
                if (this.EddyBrakeTimer < 0.8)
                {
                    this.EddyBrakeTimer = 1.65;
                    this.HitsTaken++;
                    this.Announce("▲ EDDY-CURRENT BRAKE FIELD — SPEED CUT!", 1.8);
                }
            }
        }

        // ⑩ Quality Control Finish Line Crossing (X >= 238 m)!
        if (!this.Finished && CarPos[0] >= FoundryGoalX)
        {
            this.Finished = true;
            if (this.BestRunSec === null || this.RunTimer < this.BestRunSec)
                this.BestRunSec = this.RunTimer;
            this.Announce(`◆ QC FINISH CROSSED IN ${this.RunTimer.toFixed(2)}s!`, 5.0);
        }

        this.ApplyToScene();
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                    DYNAMIC GPU OCCUPANTS & MAGNETIC LIGHTS
    //--------------------------------------------------------------------------------------------------------------------

    ApplyToScene()
    {
        const Block = this.Scene?.FoundryBlock;
        if (!Block)
            return;

        const Parked = ConstructRigidTransform([0.0, 0.0, -320.0], [0, 0, 0, 1]);
        const IdQ    = [0, 0, 0, 1];
        let DrawChanged = false;

        // 📝 Writes Occ.Transform & Occ.PreviousTransform (exact SceneStructure.EncodeOccupantRange contract!)
        const SetOcc = (Idx, Xf, Active = true, ResetHistory = false) =>
        {
            const Occ = this.Scene.Occupants[Idx];
            if (!Occ)
                return false;
            const VisChanged = Boolean(Occ.Inactive) !== !Active;
            Occ.Inactive = !Active;
            if (Occ.PreviousTransform && Occ.Transform)
            {
                if (ResetHistory || VisChanged)
                    Occ.PreviousTransform.set(Xf);
                else
                    Occ.PreviousTransform.set(Occ.Transform);
            }
            Occ.Transform = Xf;
            Occ.Moving    = Active;
            return VisChanged;
        };

        // 1. 72 Scrolling 3D steel+yellow ribbed conveyor tread cleats
        for (let I = 0; I < this.Slats.length && I < Block.BeltSlats.length; ++I)
        {
            const S = this.Slats[I];
            DrawChanged = SetOcc(Block.BeltSlats[I], ConstructRigidTransform([S.X, S.Y, 0.015], IdQ), true, S.Wrapped) || DrawChanged;
        }

        // 2. 42 Processed Cargo Items (Steel Crates, Gear Drums, Glowing Molten Ingots)
        for (const Item of this.CargoItems)
        {
            const Pool = Item.Kind === "crate" ? Block.CargoCrates
                       : Item.Kind === "drum"  ? Block.CargoDrums
                       : Block.CargoIngots;
            if (Item.PoolIdx < Pool.length)
            {
                const Q = ConstructAxisQuaternion([0, 0, 1], Item.Yaw);
                DrawChanged = SetOcc(Pool[Item.PoolIdx], ConstructRigidTransform(Item.Pos, Q), true, Item.Wrapped) || DrawChanged;
            }
        }

        // 3. 4 Stamping Press Rams + Glowing Floor Danger Pads
        for (let I = 0; I < this.Presses.length; ++I)
        {
            const P = this.Presses[I];
            DrawChanged = SetOcc(Block.PressRams[I], ConstructRigidTransform([P.Site.X, P.Site.Y, P.RamZ], IdQ), true) || DrawChanged;
            DrawChanged = SetOcc(Block.DangerPads[I], P.Warning ? ConstructRigidTransform([P.Site.X, P.Site.Y, 0.025], IdQ) : Parked, P.Warning) || DrawChanged;
        }

        // 4. 4 Pneumatic Side-Pusher Rams
        for (let I = 0; I < this.Pushers.length; ++I)
        {
            const Push = this.Pushers[I];
            const Yaw  = Push.Site.Side < 0 ? 0.0 : Math.PI;
            DrawChanged = SetOcc(Block.PusherRams[I], ConstructRigidTransform([Push.Site.X, Push.HeadY, 0.06], ConstructAxisQuaternion([0, 0, 1], Yaw)), true) || DrawChanged;
        }

        // 5. 3 Spinning Rotary Sweeper Gates
        for (let I = 0; I < this.Sweepers.length; ++I)
        {
            const Sw = this.Sweepers[I];
            DrawChanged = SetOcc(Block.SweeperBars[I], ConstructRigidTransform([Sw.Site.X, Sw.Site.Y, 0.02], ConstructAxisQuaternion([0, 0, 1], Sw.Angle)), true) || DrawChanged;
        }

        // 6. 6 Articulated Electromagnet Robots (Base, Tracking Boom Arm, Glowing Magnet Bell Head, Pulsing Field Ring!)
        for (let I = 0; I < this.MagRobots.length; ++I)
        {
            const Bot = this.MagRobots[I];
            const BaseYaw = Bot.Site.Kind === "turret" ? Bot.ArmYaw : 0.0;
            DrawChanged = SetOcc(Block.MagBases[I], ConstructRigidTransform(Bot.BasePos, ConstructAxisQuaternion([0, 0, 1], BaseYaw)), true) || DrawChanged;

            if (Bot.Site.Kind === "turret")
            {
                const ArmOrigin = [Bot.BasePos[0], Bot.BasePos[1], Bot.HeadPos[2] + 0.85];
                const ArmQ = MultiplyQuaternion(ConstructAxisQuaternion([0, 0, 1], Bot.ArmYaw), ConstructAxisQuaternion([0, 1, 0], 0.10));
                DrawChanged = SetOcc(Block.MagArms[I], ConstructRigidTransform(ArmOrigin, ArmQ), true) || DrawChanged;
            }
            else
            {
                const MastPos = [Bot.HeadPos[0], Bot.HeadPos[1], Bot.HeadPos[2] + 0.95];
                DrawChanged = SetOcc(Block.MagArms[I], ConstructRigidTransform(MastPos, ConstructAxisQuaternion([0, 1, 0], -Math.PI * 0.5)), true) || DrawChanged;
            }

            const HeadQ = ConstructAxisQuaternion([0, 0, 1], this.Clock * (Bot.State === "active" ? 3.5 : 0.5));
            DrawChanged = SetOcc(Block.MagHeads[I], ConstructRigidTransform(Bot.HeadPos, HeadQ), true) || DrawChanged;

            const ShowRing = Bot.RingRadius > 0.05;
            const RingXf = ShowRing
                ? ConstructRigidTransform([Bot.HeadPos[0], Bot.HeadPos[1], 0.03], ConstructAxisQuaternion([0, 0, 1], this.Clock * 1.4))
                : Parked;
            DrawChanged = SetOcc(Block.MagRings[I], RingXf, ShowRing) || DrawChanged;
        }

        // 7. Thermal Eddy-Brake Scanner Beam
        for (let I = 0; I < this.Scanners.length; ++I)
        {
            const Sc = this.Scanners[I];
            DrawChanged = SetOcc(Block.ScannerBeams[I], Sc.RedActive ? ConstructRigidTransform([Sc.Site.X, Sc.Site.Y, Sc.BeamZ], IdQ) : Parked, Sc.RedActive) || DrawChanged;
        }

        if (DrawChanged)
            this.Scene.ConstructDrawRuns?.();
    }

    QueryDynamicLightRecords()
    {
        if (!this.Enabled)
            return [];
        const Lights = [];
        for (const Bot of this.MagRobots)
        {
            if (Bot.State === "idle")
                continue;
            const Intensity = Bot.State === "active" ? 280.0 : (100.0 * Bot.Charge);
            Lights.push({
                Type:      LightType.Floodlight,
                Position:  [Bot.HeadPos[0], Bot.HeadPos[1], Bot.HeadPos[2] - 0.15],
                Direction: [0.0, 0.0, -1.0],
                Colour:    [0.24, 0.88, 1.0],
                Intensity,
                Reach:     26.0,
                Params:    [Math.cos(30.0 * Math.PI / 180.0), Math.cos(74.0 * Math.PI / 180.0), 0.0, 1.0]
            });
        }
        Lights.push({
            Type:      LightType.Omni,
            Position:  [239.0, 0.0, 7.2],
            Direction: [0.0, 0.0, -1.0],
            Colour:    [0.25, 1.0, 0.55],
            Intensity: 240.0,
            Reach:     34.0,
            Params:    [0.0, 0.0, 0.0, 1.0]
        });
        return Lights;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                       DIEGETIC AR HUD: MAGNETIC TRACTOR ARCS, BELT SPEED & FINISH TELEMETRY
    //--------------------------------------------------------------------------------------------------------------------

    RenderHudOverlay(Ctx, W, H, Cam)
    {
        if (!this.Enabled)
            return;
        const Hud = this.Host?.DrivingHud;
        const Car = this.Host?.Vehicle;
        if (!Hud || !Car)
            return;

        // ① Draw crackling electric-cyan magnetic tractor-beam arcs from active MagBots to attracted parts AND the car!
        Ctx.save();
        for (const Bot of this.MagRobots)
        {
            if (Bot.State !== "active")
                continue;
            const HeadScr = Hud.ProjectWorld([Bot.HeadPos[0], Bot.HeadPos[1], Bot.HeadPos[2] - 0.25], W, H, Cam);
            if (!HeadScr)
                continue;

            const DrawArc = (TargetWorld, Color, Width) =>
            {
                const EndScr = Hud.ProjectWorld(TargetWorld, W, H, Cam);
                if (!EndScr)
                    return;
                Ctx.strokeStyle = Color;
                Ctx.lineWidth   = Width;
                Ctx.beginPath();
                Ctx.moveTo(HeadScr[0], HeadScr[1]);
                const Segs = 6;
                for (let S = 1; S < Segs; ++S)
                {
                    const T = S / Segs;
                    const Jitter = Math.sin(this.Clock * 38.0 + S * 2.9 + Bot.Index) * 9.0 * Math.sin(T * Math.PI);
                    const MX = HeadScr[0] + (EndScr[0] - HeadScr[0]) * T + Jitter;
                    const MY = HeadScr[1] + (EndScr[1] - HeadScr[1]) * T + Jitter * 0.5;
                    Ctx.lineTo(MX, MY);
                }
                Ctx.lineTo(EndScr[0], EndScr[1]);
                Ctx.stroke();
            };

            if (Bot.PullingCar)
            {
                DrawArc([Car.Position[0], Car.Position[1], Car.Position[2] + 0.55], "rgba(110, 240, 255, 0.92)", 3.0);
                DrawArc([Car.Position[0], Car.Position[1], Car.Position[2] + 0.55], "rgba(255, 255, 255, 0.85)", 1.2);
            }

            for (const Item of this.CargoItems)
            {
                if (Item.MagnetIndex === Bot.Index && (Item.State === "attracted" || Item.State === "captured"))
                    DrawArc([Item.Pos[0], Item.Pos[1], Item.Pos[2] + 0.5], "rgba(92, 224, 255, 0.72)", 1.8);
            }

            if (Math.hypot(Bot.HeadPos[0] - Car.Position[0], Bot.HeadPos[1] - Car.Position[1]) < 65.0)
            {
                Ctx.shadowColor = "rgba(0,0,0,0.85)";
                Ctx.shadowBlur  = 6;
                Ctx.fillStyle   = Bot.PullingCar ? "#78f3ff" : "#a8dcff";
                Ctx.font        = "800 11px 'JetBrains Mono', monospace";
                Ctx.textAlign   = "center";
                Ctx.fillText(Bot.PullingCar ? "⚡ MAG-BOT LOCK!" : `◎ ${Bot.Site.Id.toUpperCase()} · ${Bot.CapturedCount} PARTS`, HeadScr[0], HeadScr[1] - 18);
            }
        }
        Ctx.restore();

        // ② Quality Control Finish Gate Beacon
        const DistToGoal = Math.max(0, Math.round(FoundryGoalX - Car.Position[0]));
        const GateScr = Hud.ProjectWorld([240.0, 0.0, 12.2], W, H, Cam);
        if (GateScr)
        {
            Ctx.save();
            Ctx.shadowColor = "rgba(0,0,0,0.9)";
            Ctx.shadowBlur  = 8;
            Ctx.fillStyle   = "#6bffb3";
            Ctx.font        = "800 12.5px 'Inter', system-ui, sans-serif";
            Ctx.textAlign   = "center";
            Ctx.fillText(`▶ QC FINISH GATE (${DistToGoal}m)`, GateScr[0], GateScr[1]);
            Ctx.restore();
        }

        // ③ Top Optical Telemetry Strip — positioned cleanly below the AR-HUD pill strip (Y = 58 / 76 / 100)!
        Ctx.save();
        Ctx.shadowColor = "rgba(0,0,0,0.92)";
        Ctx.shadowBlur  = 7;
        Ctx.textAlign   = "center";

        Ctx.fillStyle = this.Finished ? "#6bffb3" : "#ffe28a";
        Ctx.font      = "800 13.5px 'Inter', system-ui, sans-serif";
        const Header  = this.Finished
            ? `◆ ASSEMBLY LINE CLEARED · ${this.RunTimer.toFixed(2)}s${this.BestRunSec !== null ? ` (BEST ${this.BestRunSec.toFixed(2)}s)` : ""}`
            : `◆ ASSEMBLY LINE · ${DistToGoal}m TO FINISH · ${this.RunTimer.toFixed(1)}s`;
        Ctx.fillText(Header, W * 0.5, 58);

        const BeltSpd = this.CurrentBelt.SpeedX;
        const BeltArrow = BeltSpd > 0.5 ? "▶▶" : (BeltSpd < -0.5 ? "◀◀" : "—");
        let StatusLine = `${BeltArrow} BELT ${BeltSpd >= 0 ? "+" : ""}${BeltSpd.toFixed(1)} m/s (${this.CurrentBelt.PhaseLabel})`;
        let StatusColor = BeltSpd > 1.0 ? "#6bffb3" : (BeltSpd < -8.0 ? "#ff6b6b" : "#ffd875");

        if (this.ActiveMagnetPull > 1.0)
        {
            StatusLine  = `⚡ MAG-PULL ${this.ActiveMagnetPull.toFixed(0)} m/s² — COUNTER-STEER!`;
            StatusColor = "#6ef4ff";
        }
        else if (this.EddyBrakeTimer > 0.05)
        {
            StatusLine  = `▲ EDDY-CURRENT BRAKE (${this.EddyBrakeTimer.toFixed(1)}s)`;
            StatusColor = "#ff7070";
        }

        Ctx.fillStyle = StatusColor;
        Ctx.font      = "700 11.5px 'JetBrains Mono', monospace";
        Ctx.fillText(StatusLine, W * 0.5, 76);

        if (this.BannerTimer > 0.0 && this.BannerMessage)
        {
            Ctx.fillStyle = "#ffffff";
            Ctx.font      = "800 13.5px 'Inter', system-ui, sans-serif";
            Ctx.fillText(this.BannerMessage, W * 0.5, 100);
        }
        Ctx.restore();
    }

    Dispose()
    {
        if (this.Host?.Vehicle)
            this.Host.Vehicle.SurfaceVelocityQuery = null;
        if (this.ControlStrip)
        {
            this.ControlStrip.remove();
            this.ControlStrip = null;
        }
    }
}
