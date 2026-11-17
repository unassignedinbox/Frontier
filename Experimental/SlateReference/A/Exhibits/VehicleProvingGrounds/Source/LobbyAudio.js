//============================================================================================================================================
//                                                             LOBBYAUDIO.JS
//============================================================================================================================================
// 🧩 Procedural Web Audio hydraulic stage/lift soundscape for the Showroom Lobby:
//    synthesizes smooth pressurized hydraulic motor hum, fluid rush, mechanical turntable bearing rumble,
//    pneumatic unlock/lock solenoid clunks, and camera-relative stereo spatial panning as circular lifts rise & lower.

export class LobbyAudio
{
    constructor()
    {
        this.Context   = null;
        this.Master    = null;
        this.MotorGain = null;
        this.MotorOscA = null;
        this.MotorOscB = null;
        this.MotorFilter = null;
        this.FluidGain = null;
        this.FluidFilter = null;
        this.Panner    = null;
        this.NoiseBuffer = null;
        this.Muted     = false;
        this.Ready     = false;
        this.PriorPhases = new Map();
        this.CurrentGain = 0.0;
    }

    EnsureContext()
    {
        if (this.Ready)
            return true;
        const AudioCtx = globalThis.AudioContext || globalThis.webkitAudioContext;
        if (!AudioCtx)
            return false;
        try
        {
            const Ctx = new AudioCtx();
            this.Context = Ctx;

            // Master bus
            const Master = Ctx.createGain();
            Master.gain.value = this.Muted ? 0.0 : 0.42;
            Master.connect(Ctx.destination);
            this.Master = Master;

            // Stereo spatial panner for active lift location relative to camera
            const Panner = Ctx.createStereoPanner ? Ctx.createStereoPanner() : null;
            if (Panner)
                Panner.connect(Master);
            this.Panner = Panner;
            const BusDest = Panner || Master;

            // Continuous hydraulic pump / electric actuator hum (dual harmonic + warm lowpass)
            const MotorFilter = Ctx.createBiquadFilter();
            MotorFilter.type = "lowpass";
            MotorFilter.frequency.value = 230.0;
            MotorFilter.Q.value = 1.4;

            const MotorGain = Ctx.createGain();
            MotorGain.gain.value = 0.0001;
            MotorFilter.connect(MotorGain);
            MotorGain.connect(BusDest);

            const OscA = Ctx.createOscillator();
            OscA.type = "triangle";
            OscA.frequency.value = 68.0;
            OscA.connect(MotorFilter);
            OscA.start();

            const OscB = Ctx.createOscillator();
            OscB.type = "sawtooth";
            OscB.frequency.value = 136.0;
            const SubGainB = Ctx.createGain();
            SubGainB.gain.value = 0.24;
            OscB.connect(SubGainB);
            SubGainB.connect(MotorFilter);
            OscB.start();

            // Filtered noise buffer for hydraulic fluid hiss / pneumatic air bleed
            const Length = Math.floor(Ctx.sampleRate * 2.0);
            const Noise = Ctx.createBuffer(1, Length, Ctx.sampleRate);
            const Data = Noise.getChannelData(0);
            for (let I = 0; I < Length; ++I)
                Data[I] = Math.random() * 2.0 - 1.0;
            this.NoiseBuffer = Noise;

            const NoiseSrc = Ctx.createBufferSource();
            NoiseSrc.buffer = Noise;
            NoiseSrc.loop = true;

            const FluidFilter = Ctx.createBiquadFilter();
            FluidFilter.type = "bandpass";
            FluidFilter.frequency.value = 460.0;
            FluidFilter.Q.value = 1.1;

            const FluidGain = Ctx.createGain();
            FluidGain.gain.value = 0.0001;
            NoiseSrc.connect(FluidFilter);
            FluidFilter.connect(FluidGain);
            FluidGain.connect(BusDest);
            NoiseSrc.start();

            this.MotorOscA = OscA;
            this.MotorOscB = OscB;
            this.MotorFilter = MotorFilter;
            this.MotorGain = MotorGain;
            this.FluidFilter = FluidFilter;
            this.FluidGain = FluidGain;
            this.Ready = true;
            return true;
        }
        catch { return false; }
    }

    Unlock()
    {
        if (!this.EnsureContext())
            return false;
        if (this.Context && this.Context.state === "suspended")
            this.Context.resume().catch(() => {});
        return true;
    }

    SetMuted(Muted)
    {
        this.Muted = Boolean(Muted);
        if (this.Master && this.Context)
            this.Master.gain.setTargetAtTime(this.Muted ? 0.0 : 0.42, this.Context.currentTime, 0.05);
        return this.Muted;
    }

    ToggleMute()
    {
        if (this.Muted)
            this.Unlock();
        return this.SetMuted(!this.Muted);
    }

    Stop()
    {
        if (!this.Ready || !this.Context)
            return;
        const Now = this.Context.currentTime;
        this.MotorGain?.gain.setTargetAtTime(0.0001, Now, 0.08);
        this.FluidGain?.gain.setTargetAtTime(0.0001, Now, 0.08);
    }

    // 📝 Synthesizes a tactile mechanical solenoid latch / hydraulic floor-lock thump + crisp metallic relay click.
    PlayLatch(Kind = "lock", Pan = 0.0)
    {
        if (this.Muted || !this.Ready || !this.Context || this.Context.state !== "running")
            return;
        const Ctx = this.Context;
        const Now = Ctx.currentTime;

        const Dest = Ctx.createStereoPanner ? Ctx.createStereoPanner() : null;
        if (Dest)
        {
            Dest.pan.value = Math.max(-0.85, Math.min(0.85, Pan));
            Dest.connect(this.Master);
        }
        const Out = Dest || this.Master;

        // Low heavy hydraulic thud
        const ThudOsc = Ctx.createOscillator();
        const ThudGain = Ctx.createGain();
        ThudOsc.type = "sine";
        const StartHz = Kind === "unlock" ? 102.0 : 82.0;
        const EndHz   = Kind === "unlock" ? 44.0  : 34.0;
        ThudOsc.frequency.setValueAtTime(StartHz, Now);
        ThudOsc.frequency.exponentialRampToValueAtTime(EndHz, Now + 0.16);
        ThudGain.gain.setValueAtTime(0.001, Now);
        ThudGain.gain.linearRampToValueAtTime(Kind === "lock" ? 0.34 : 0.24, Now + 0.012);
        ThudGain.gain.exponentialRampToValueAtTime(0.0008, Now + 0.22);
        ThudOsc.connect(ThudGain);
        ThudGain.connect(Out);
        ThudOsc.start(Now);
        ThudOsc.stop(Now + 0.24);

        // Crisp metallic pneumatic valve snap (bandpass burst)
        if (this.NoiseBuffer)
        {
            const Burst = Ctx.createBufferSource();
            Burst.buffer = this.NoiseBuffer;
            const Filter = Ctx.createBiquadFilter();
            Filter.type = "bandpass";
            Filter.frequency.value = Kind === "unlock" ? 1350.0 : 1850.0;
            Filter.Q.value = 3.2;
            const Env = Ctx.createGain();
            Env.gain.setValueAtTime(0.001, Now);
            Env.gain.linearRampToValueAtTime(0.18, Now + 0.006);
            Env.gain.exponentialRampToValueAtTime(0.0008, Now + 0.085);
            Burst.connect(Filter);
            Filter.connect(Env);
            Env.connect(Out);
            Burst.start(Now, Math.random() * 1.2, 0.10);
        }
    }

    ComputePan(WorldPos, CameraState)
    {
        if (!CameraState?.Position || !CameraState?.Right)
            return 0.0;
        const DX = WorldPos[0] - CameraState.Position[0];
        const DY = WorldPos[1] - CameraState.Position[1];
        const Dist = Math.max(1.0, Math.hypot(DX, DY));
        const Dot = (DX * CameraState.Right[0] + DY * CameraState.Right[1]) / Dist;
        return Math.max(-0.85, Math.min(0.85, Dot * 0.9));
    }

    // 📝 Updates continuous hydraulic motor + fluid audio & detects phase transitions for latch clunks.
    Update(Roster, CameraState)
    {
        if (!Roster?.Enabled)
            return;

        let PeakSpeed = 0.0;
        let SignedVelocity = 0.0;
        let DominantPos = null;
        let MovingCount = 0;

        // Single-hero showcase stage hydraulic lift audio
        const Stage = Roster.StageMotion;
        if (Stage)
        {
            const Phase = Stage.Phase || "lowered";
            const PrevPhase = this.PriorPhases.get("hero-stage");
            const Pan = this.ComputePan(Stage.Position || [0, 0, Stage.LiftZ || 0], CameraState);
            if (PrevPhase !== undefined && PrevPhase !== Phase)
            {
                if ((PrevPhase === "lowered" || PrevPhase === "raised") && (Phase === "rising_stage" || Phase === "lowering_stage"))
                    this.PlayLatch("unlock", Pan);
                else if ((PrevPhase === "rising_stage" || PrevPhase === "lowering_stage") && (Phase === "raised" || Phase === "lowered"))
                    this.PlayLatch("lock", Pan);
            }
            this.PriorPhases.set("hero-stage", Phase);
            const AbsSpeed = Math.abs(Stage.Speed || 0.0);
            if (AbsSpeed > 0.015)
            {
                MovingCount = 1;
                PeakSpeed = AbsSpeed * 4.5;
                SignedVelocity = Stage.Speed;
                DominantPos = Stage.Position || [0, 0, Stage.LiftZ || 0];
            }
        }

        if (!this.Ready || !this.Context || this.Context.state !== "running")
            return;

        const Now = this.Context.currentTime;
        const Active = !this.Muted && PeakSpeed > 0.05;
        const SpeedNorm = Math.min(1.0, PeakSpeed / 1.5);
        const MultiBoost = Math.min(1.35, 1.0 + Math.max(0, MovingCount - 1) * 0.25);

        if (Active)
        {
            const Rising = SignedVelocity > 0.0;
            const BaseHz = Rising ? (72.0 + SpeedNorm * 26.0) : (64.0 + SpeedNorm * 16.0);
            this.MotorOscA.frequency.setTargetAtTime(BaseHz, Now, 0.08);
            this.MotorOscB.frequency.setTargetAtTime(BaseHz * 2.01, Now, 0.08);
            this.MotorFilter.frequency.setTargetAtTime(Rising ? (210.0 + SpeedNorm * 140.0) : (165.0 + SpeedNorm * 85.0), Now, 0.08);
            this.MotorGain.gain.setTargetAtTime((0.12 + 0.14 * SpeedNorm) * MultiBoost, Now, 0.08);

            // Hydraulic fluid rush & valve bleed hiss
            this.FluidFilter.frequency.setTargetAtTime(Rising ? (420.0 + SpeedNorm * 220.0) : (620.0 - SpeedNorm * 180.0), Now, 0.08);
            this.FluidGain.gain.setTargetAtTime((0.035 + 0.055 * SpeedNorm) * MultiBoost, Now, 0.08);

            if (this.Panner && DominantPos)
                this.Panner.pan.setTargetAtTime(this.ComputePan(DominantPos, CameraState), Now, 0.12);
        }
        else
        {
            this.MotorGain.gain.setTargetAtTime(0.0001, Now, 0.14);
            this.FluidGain.gain.setTargetAtTime(0.0001, Now, 0.14);
        }
    }
}
