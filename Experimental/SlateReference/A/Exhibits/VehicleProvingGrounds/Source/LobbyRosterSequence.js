//============================================================================================================================================
//                                                          LOBBYROSTERSEQUENCE.JS
//============================================================================================================================================
// 🧩 Single-Hero Lobby session roster & hero hydraulic showcase stage controller:
//    • Tracks the 5 Lobby players (Host + 4 peers) with live Connected / Ready / Offline status & ping displayed
//      inside the Cockpit 3D HMI — keeping the physical showroom pure with ONLY the single hero vehicle!
//    • Drives smooth vertical hydraulic elevation of the single hero showcase stage (0.00 m ↔ 0.42 m) with
//      synchronized velocity/phase telemetry for procedural Web Audio hydraulic pump & latch synthesis.

import { LobbyLiftConstants } from "./LobbyStructure.js";
import { LobbyDrivers } from "./LobbyDrivers.js";
import { EaseCinematic } from "./LobbyCameraMotion.js";
import { ConstructRigidTransform } from "./TransformProjection.js";
import { VehicleGeometry } from "./VehicleSpecification.js";

const StageTravelDuration = 2.6;   // [s] smooth hydraulic stage elevation / lowering cycle

export function EvaluateHeroStageMotion(Progress)
{
    const P = Math.max(0.0, Math.min(1.0, Progress));
    const LiftZ = EaseCinematic(P) * LobbyLiftConstants.StageRaisedZ;
    return { LiftZ, Visible: P > 0.002 };
}

export class LobbyRosterSequence
{
    constructor(Host)
    {
        this.Host    = Host;
        this.Enabled = Boolean(Host?.Lobby && Host?.Scene?.LobbyBlock);
        this.Clock   = 0.0;

        // Single-hero hydraulic stage elevation state (0 = flush seamless floor, 1 = elevated 0.42 m showcase plinth)
        this.StageRaised   = false;
        this.StageProgress = 0.0;
        this.StageZ        = 0.0;
        this.StageVelocity = 0.0;
        this.StagePhase    = "lowered";
        this.StageMotion   = { Phase: "lowered", LiftZ: 0.0, Speed: 0.0, Position: [0.0, 0.0, 0.0] };

        // Connected lobby players (shown cleanly inside the Cockpit HMI!)
        const InitialConnected = [true, true, true, true, false];
        const InitialReady     = [true, true, false, true, false];
        const InitialPings     = [8, 19, 32, 24, 0];
        this.Players = LobbyDrivers.map((Driver, Index) => ({
            Index,
            Id:        Driver.NameTag,
            NameTag:   Driver.NameTag,
            Rank:      Driver.Rank,
            Accent:    Driver.Accent,
            IsLocal:   Index === 0,
            Connected: InitialConnected[Index],
            Ready:     InitialReady[Index],
            PingMs:    InitialPings[Index],
            Role:      Index === 0 ? "Host · In Cockpit" : (InitialConnected[Index] ? "Connected · Lobby" : "Offline"),
            NextFlip:  16.0 + Index * 11.0
        }));

        this.LastEvent = "4 of 5 connected";
        this.ApplyToScene();
    }

    ToggleStageLift(Raise = null)
    {
        const Next = Raise === null ? !this.StageRaised : Boolean(Raise);
        if (Next !== this.StageRaised)
        {
            this.StageRaised = Next;
            this.LastEvent = Next ? "Stage hydraulic lift rising" : "Stage hydraulic lift lowering";
        }
        return this.StageRaised;
    }

    ToggleHeroReady()
    {
        const Hero = this.Players[0];
        if (!Hero) return false;
        Hero.Ready = !Hero.Ready;
        Hero.Role = Hero.Ready ? "Host · Ready" : "Host · In Cockpit";
        return Hero.Ready;
    }

    TogglePlayerConnection(Index)
    {
        const Player = this.Players[Index];
        if (!Player || Player.IsLocal)
            return false;
        Player.Connected = !Player.Connected;
        Player.Ready = Player.Connected ? true : false;
        Player.PingMs = Player.Connected ? (14 + ((Index * 11) % 28)) : 0;
        Player.Role = Player.Connected ? "Connected · Lobby" : "Offline";
        this.LastEvent = Player.Connected ? `${Player.NameTag} connected` : `${Player.NameTag} disconnected`;
        return Player.Connected;
    }

    Advance(DeltaSeconds)
    {
        if (!this.Enabled)
            return false;

        const Dt = Math.min(0.1, Math.max(0.0, DeltaSeconds));
        this.Clock += Dt;

        // Subtle realistic online/offline lobby peer presence updates (shown inside Cockpit HMI)
        for (let I = 1; I < this.Players.length; ++I)
        {
            const P = this.Players[I];
            if (P.Connected)
                P.PingMs = Math.max(10, Math.round(18 + I * 4 + Math.sin(this.Clock * 0.4 + I) * 3));
            if (this.Clock >= P.NextFlip)
            {
                P.NextFlip = this.Clock + 26.0 + (I * 9.0);
                // Keep between 3 and 5 players connected so cockpit roster always feels alive
                const OnlineCount = this.Players.filter((X) => X.Connected).length;
                if (P.Connected && OnlineCount > 3)
                {
                    P.Connected = false;
                    P.Ready = false;
                    P.PingMs = 0;
                    P.Role = "Offline";
                    this.LastEvent = `${P.NameTag} left lobby`;
                }
                else if (!P.Connected)
                {
                    P.Connected = true;
                    P.Ready = (I % 2 === 1);
                    P.PingMs = 15 + I * 5;
                    P.Role = P.Ready ? "Connected · Ready" : "Connected · Lobby";
                    this.LastEvent = `${P.NameTag} joined lobby`;
                }
            }
        }

        // Smooth single-hero hydraulic showcase stage elevation
        const Target = this.StageRaised ? 1.0 : 0.0;
        const PrevZ = this.StageZ;
        let Moved = false;
        if (Math.abs(this.StageProgress - Target) > 1e-4)
        {
            const Step = Dt / StageTravelDuration;
            if (this.StageProgress < Target)
                this.StageProgress = Math.min(Target, this.StageProgress + Step);
            else
                this.StageProgress = Math.max(Target, this.StageProgress - Step);
            Moved = true;
        }
        const Eval = EvaluateHeroStageMotion(this.StageProgress);
        this.StageZ = Eval.LiftZ;
        this.StageVelocity = Dt > 1e-4 ? (this.StageZ - PrevZ) / Dt : 0.0;
        if (Math.abs(this.StageVelocity) > 0.01)
            this.StagePhase = this.StageVelocity > 0 ? "rising_stage" : "lowering_stage";
        else
            this.StagePhase = this.StageRaised ? "raised" : "lowered";

        this.StageMotion = {
            Phase: this.StagePhase,
            LiftZ: this.StageZ,
            Speed: this.StageVelocity,
            Position: [0.0, 0.0, this.StageZ]
        };

        if (Moved)
            this.ApplyToScene();
        return Moved;
    }

    ApplyToScene()
    {
        if (!this.Enabled)
            return;
        const Scene = this.Host.Scene;
        const Block = Scene.LobbyBlock;
        if (!Block || Block.HeroStage === undefined || Block.HeroStage < 0)
            return;

        const Eval = EvaluateHeroStageMotion(this.StageProgress);
        if (Block.HeroPlates !== undefined && Block.HeroPlates >= 0)
        {
            const PlateOcc = Scene.Occupants[Block.HeroPlates];
            PlateOcc.PreviousTransform = PlateOcc.Transform.slice();
            PlateOcc.Transform = ConstructRigidTransform([0.0, 0.0, VehicleGeometry.CoMHeight + Eval.LiftZ], [0.0, 0.0, 0.0, 1.0]);
        }

        const StageOcc = Scene.Occupants[Block.HeroStage];
        const WasInactive = StageOcc.Inactive;
        StageOcc.Inactive = !Eval.Visible;
        StageOcc.Moving = Math.abs(this.StageVelocity) > 0.005;
        StageOcc.PreviousTransform = StageOcc.Transform.slice();
        StageOcc.Transform = ConstructRigidTransform([0.0, 0.0, Eval.LiftZ], [0.0, 0.0, 0.0, 1.0]);
        if (WasInactive !== StageOcc.Inactive)
        {
            Scene.ConstructDrawRuns();
            Scene.ConstructDistanceList();
        }
    }

    QueryActiveLiftFocus()
    {
        return null;
    }

    QueryRosterSummary()
    {
        const ConnectedCount = this.Players.filter((P) => P.Connected).length;
        const ReadyCount = this.Players.filter((P) => P.Connected && P.Ready).length;
        return {
            ActiveCount: ConnectedCount,
            ReadyCount,
            MaxCount:    this.Players.length,
            StageRaised: this.StageRaised,
            StageZ:      this.StageZ,
            StagePhase:  this.StagePhase,
            StatusLine:  this.LastEvent,
            Hero:        this.Players[0],
            Players:     this.Players.map((P) => ({ ...P }))
        };
    }
}
