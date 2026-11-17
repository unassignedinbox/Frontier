//============================================================================================================================================
//                                                          LOBBYCAMERAMOTION.JS
//============================================================================================================================================
// 🧩 Pure camera-motion primitives and the 8-shot Continuous One-Take Automotive Film Director for the Lobby:
//    smootherstep cinematic easing, shortest-arc angle blending, exponential lag, venue-aware containment,
//    and ZERO-FADE continuous polar-arc Steadicam transitions between all 8 film shots!

export const StudioCeilingEyeLimit = 5.25;   // [m] below the studio softbox (z = 5.95 m) with a clean buffer
export const StudioRadiusEyeLimit  = 18.4;   // [m] inside the studio cyclorama wall (r = 20 m) with cove margin
export const StudioFloorEyeLimit   = 0.35;   // [m] above the studio floor / turntable

// 📝 Quintic smootherstep — zero velocity and zero acceleration at both ends of a cinematic dolly or shot glide.
export function EaseCinematic(Fraction)
{
    const T = Math.max(0.0, Math.min(1.0, Fraction));
    return T * T * T * (T * (T * 6.0 - 15.0) + 10.0);
}

// 📝 Shortest-arc angle interpolation in radians.
export function InterpolateAngle(From, To, Fraction)
{
    let Delta = (To - From) % (2.0 * Math.PI);
    if (Delta > Math.PI)  Delta -= 2.0 * Math.PI;
    if (Delta < -Math.PI) Delta += 2.0 * Math.PI;
    return From + Delta * Fraction;
}

// 📝 Interpolates two orbit poses { Yaw, Pitch, Distance }.
export function InterpolateOrbitPose(From, To, Fraction)
{
    return {
        Yaw:      InterpolateAngle(From.Yaw, To.Yaw, Fraction),
        Pitch:    From.Pitch    + (To.Pitch    - From.Pitch)    * Fraction,
        Distance: From.Distance + (To.Distance - From.Distance) * Fraction
    };
}

// 📝 Frame-rate-independent exponential lag factor for silky tripod inertia.
export function ResolveLagBlend(DeltaSeconds, TimeConstant)
{
    if (DeltaSeconds <= 0.0) return 0.0;
    if (TimeConstant <= 0.0) return 1.0;
    return 1.0 - Math.exp(-DeltaSeconds / TimeConstant);
}

export function BlendVector(Current, Target, Factor)
{
    return [
        Current[0] + (Target[0] - Current[0]) * Factor,
        Current[1] + (Target[1] - Current[1]) * Factor,
        Current[2] + (Target[2] - Current[2]) * Factor
    ];
}

// 📝 Clamps any Lobby camera eye strictly inside the active venue's enclosure bounds (defaults to Studio).
export function ClampStudioEye(Eye, Bounds = null)
{
    const MinZ = Bounds?.MinZ ?? StudioFloorEyeLimit;
    const MaxZ = Bounds?.MaxZ ?? StudioCeilingEyeLimit;
    const MaxR = Bounds?.MaxRadius ?? StudioRadiusEyeLimit;
    let [X, Y, Z] = Eye;
    Z = Math.max(MinZ, Math.min(MaxZ, Z));
    const R = Math.hypot(X, Y);
    if (R > MaxR)
    {
        const Scale = MaxR / R;
        X *= Scale;
        Y *= Scale;
    }
    return [X, Y, Z];
}

//------------------------------------------------------------------------------------------------------------------------
//                                      8-SHOT CONTINUOUS ONE-TAKE FILM DIRECTOR
//------------------------------------------------------------------------------------------------------------------------

function Lerp(A, B, T) { return A + (B - A) * T; }

function Lerp3(A, B, T)
{
    return [Lerp(A[0], B[0], T), Lerp(A[1], B[1], T), Lerp(A[2], B[2], T)];
}

function PolarEye(Origin, Angle, Radius, Height)
{
    return [Origin[0] + Radius * Math.cos(Angle), Origin[1] + Radius * Math.sin(Angle), Origin[2] + Height];
}

export const LobbyCinematicShots = Object.freeze([
    Object.freeze({ Id: "hero-low-dolly",     Title: "01 · Hero Low Dolly",         Duration: 7.4 }),
    Object.freeze({ Id: "architectural-hero", Title: "02 · Architectural Backdrop", Duration: 7.6 }),
    Object.freeze({ Id: "roofline-softbox",   Title: "03 · Roofline Reflection",    Duration: 6.8 }),
    Object.freeze({ Id: "showroom-crane",     Title: "04 · Architectural Crane",    Duration: 8.2 }),
    Object.freeze({ Id: "telephoto-profile",  Title: "05 · Telephoto Profile",      Duration: 7.2 }),
    Object.freeze({ Id: "rear-stance",        Title: "06 · Rear Plate & Stance",    Duration: 6.8 }),
    Object.freeze({ Id: "cockpit-glass-arc",  Title: "07 · Cockpit Glass Arc",      Duration: 6.8 }),
    Object.freeze({ Id: "constellation-spin", Title: "08 · Hero Orbit Finale",      Duration: 7.6 })
]);

// 📝 Evaluates one frame of the 8-shot Automotive Film Director reel, adapted cleanly to the active venue bounds.
export function EvaluateCinematicReelShot(ShotIndex, Fraction, HeroPos = [0, 0, 0.52], ActiveLift = null, ClockSeconds = 0.0, Bounds = null)
{
    const Shot = ((ShotIndex % LobbyCinematicShots.length) + LobbyCinematicShots.length) % LobbyCinematicShots.length;
    const RawT = Math.max(0.0, Math.min(1.0, Fraction));
    const S    = EaseCinematic(RawT);
    const [HX, HY] = HeroPos;
    const Center = [HX, HY, 0.0];
    const CeilingMax = Bounds?.MaxZ ?? StudioCeilingEyeLimit;
    const OpenAir = CeilingMax > 7.0;

    let Eye, Target, Fov;

    switch (Shot)
    {
        case 0:
        {
            // 01 · Hero Low Dolly: ground-skimming front-quarter push-in sweeping past nose, wheel & illuminated hub.
            const Angle  = Lerp(0.46, 1.18, S);
            const Radius = Lerp(6.4, 4.9, S);
            const Height = Lerp(0.48, 0.78, S);
            Eye    = PolarEye(Center, Angle, Radius, Height);
            Target = [HX + Lerp(1.35, 0.15, S), HY + Lerp(-0.18, 0.08, S), Lerp(0.56, 0.76, S)];
            Fov    = Lerp(36.0, 32.5, S);
            break;
        }
        case 1:
        {
            // 02 · Architectural Backdrop: dramatic low front-three-quarter sweep framing the hero car against the
            //      venue's signature architecture (solar-yellow lightwall, sky oculus & canyon, monoliths, or salt horizon!).
            const Angle  = Lerp(0.54, 0.88, S);
            const Radius = Lerp(8.8, 7.2, S);
            const Height = Lerp(0.62, 1.05, S);
            Eye    = PolarEye(Center, Angle, Radius, Height);
            Target = [HX + Lerp(-0.35, 0.25, S), HY + Lerp(-0.45, 0.05, S), Lerp(0.72, 0.84, S)];
            Fov    = Lerp(42.0, 38.0, S);
            break;
        }
        case 2:
        {
            // 03 · Roofline Reflection: elevated longitudinal sweep pulling studio/sky reflections across hood & roof.
            Eye = [
                HX + Lerp(3.9, -2.8, S),
                HY + Lerp(2.7,  3.2, S),
                Lerp(2.35, 1.55, S)
            ];
            Target = [HX + Lerp(0.7, -0.45, S), HY + Lerp(-0.1, 0.05, S), Lerp(0.88, 0.96, S)];
            Fov    = Lerp(40.0, 44.0, S);
            break;
        }
        case 3:
        {
            // 04 · Architectural Crane: wide sweeping jib rise unveiling all 5 cars & the venue's signature architecture
            //      (oriented to face the red-rock canyon vista in Oculus, or the white monoliths / salt horizon!).
            const Angle  = Lerp(-1.95, -0.95, S);
            const Radius = Lerp(OpenAir ? 17.5 : 14.4, OpenAir ? 14.5 : 12.2, S);
            const Height = Lerp(1.15, Math.min(OpenAir ? 6.2 : 4.15, CeilingMax - 0.35), S);
            Eye    = PolarEye(Center, Angle, Radius, Height);
            Target = [HX, HY + Lerp(0.4, 2.2, S), Lerp(0.75, 1.05, S)];
            Fov    = Lerp(48.0, 44.0, S);
            break;
        }
        case 4:
        {
            // 05 · Telephoto Profile: compressed 29° long-lens lateral track along the hero silhouette for lush parallax.
            const Angle  = Lerp(1.30, 1.84, S);
            const Radius = Lerp(10.8, 10.2, S);
            const Height = Lerp(0.84, 0.94, S);
            Eye    = PolarEye(Center, Angle, Radius, Height);
            Target = [HX + Lerp(0.95, -0.95, S), HY, 0.76];
            Fov    = Lerp(29.0, 31.5, S);
            break;
        }
        case 5:
        {
            // 06 · Rear Plate & Stance: low rear three-quarter push-in highlighting NOVA-7 3D plate & tail posture.
            const Angle  = Lerp(2.58, 3.66, S);
            const Radius = Lerp(5.8, 4.7, S);
            const Height = Lerp(0.56, 1.12, S);
            Eye    = PolarEye(Center, Angle, Radius, Height);
            Target = [HX + Lerp(-1.55, -0.65, S), HY + Lerp(0.18, -0.10, S), Lerp(0.64, 0.80, S)];
            Fov    = Lerp(38.0, 34.0, S);
            break;
        }
        case 6:
        {
            // 07 · Cockpit Glass Arc: intimate shoulder-height curve peering across cabin glass & glowing HMI tablet.
            const Angle  = Lerp(-1.18, -0.58, S);
            const Radius = Lerp(3.65, 3.10, S);
            const Height = Lerp(1.48, 1.32, S);
            Eye    = PolarEye(Center, Angle, Radius, Height);
            Target = [HX + Lerp(0.20, 0.35, S), HY + Lerp(0.05, -0.04, S), Lerp(1.02, 0.96, S)];
            Fov    = Lerp(38.0, 35.0, S);
            break;
        }
        default:
        {
            // 08 · Showroom Constellation: high architectural rotation over all 5 geometric circular lifts.
            const Angle  = Lerp(0.25, 1.45, S);
            const Radius = Lerp(OpenAir ? 15.0 : 12.2, OpenAir ? 13.0 : 10.8, S);
            const HighZ  = Math.min(OpenAir ? 8.2 : 4.45, CeilingMax - 0.15);
            const Height = Lerp(HighZ - 0.25, HighZ, S);
            Eye    = PolarEye(Center, Angle, Radius, Height);
            Target = [HX, HY, 0.30];
            Fov    = Lerp(50.0, 46.0, S);
            break;
        }
    }

    // Subtle organic fluid-head breathing (zero jitter, purely slow damped cinema drift)
    const BreathX = Math.sin(ClockSeconds * 0.42) * 0.012;
    const BreathY = Math.cos(ClockSeconds * 0.35) * 0.012;
    const BreathZ = Math.sin(ClockSeconds * 0.58) * 0.008;

    return {
        ShotIndex: Shot,
        ShotTitle: LobbyCinematicShots[Shot].Title,
        Duration:  LobbyCinematicShots[Shot].Duration,
        Eye:       ClampStudioEye([Eye[0] + BreathX, Eye[1] + BreathY, Eye[2] + BreathZ], Bounds),
        Target,
        Fov
    };
}

// 📝 ZERO-FADE continuous one-take camera bridge: sweeps smoothly in a polar crane arc around HeroPos from FromPose
//    to ToPose (maintaining safe clearance around the hero car so the lens NEVER clips the body and NEVER fades black!).
export function BlendCinematicPoses(FromPose, ToPose, Fraction, HeroPos = [0, 0, 0.52], Bounds = null)
{
    const S = EaseCinematic(Fraction);
    const [HX, HY] = HeroPos;
    const A0 = Math.atan2(FromPose.Eye[1] - HY, FromPose.Eye[0] - HX);
    const A1 = Math.atan2(ToPose.Eye[1] - HY,   ToPose.Eye[0] - HX);
    const R0 = Math.hypot(FromPose.Eye[0] - HX, FromPose.Eye[1] - HY);
    const R1 = Math.hypot(ToPose.Eye[0] - HX,   ToPose.Eye[1] - HY);

    const Angle = InterpolateAngle(A0, A1, S);
    // Gentle outward swing at mid-arc so close shots gracefully clear the hero car while sweeping around!
    const MinClearR = Math.max(Math.min(R0, R1), 4.6);
    const BaseR = Lerp(R0, R1, S);
    const ArcBoost = Math.sin(Math.PI * S) * Math.max(0.0, MinClearR - BaseR + 0.65);
    const Radius = BaseR + ArcBoost;
    const Height = Lerp(FromPose.Eye[2], ToPose.Eye[2], S) + Math.sin(Math.PI * S) * 0.35;

    const Eye = ClampStudioEye([HX + Radius * Math.cos(Angle), HY + Radius * Math.sin(Angle), Height], Bounds);
    const Target = Lerp3(FromPose.Target, ToPose.Target, S);
    const Fov = Lerp(FromPose.Fov, ToPose.Fov, S);

    return {
        ShotIndex: ToPose.ShotIndex,
        ShotTitle: ToPose.ShotTitle,
        Duration:  ToPose.Duration,
        Eye,
        Target,
        Fov
    };
}
