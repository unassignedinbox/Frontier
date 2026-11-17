import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";

import { LobbyVenues, LobbyShapes, LobbyLiftConstants, LobbyVehiclePlacements, LobbyDistanceCutouts,
         ResolveLobbyVenueDescriptor, ResolveLobbyPlacements, ResolveLobbyStudioLights,
         EmitLobbyGroundFacets, EmitLiftPlatformFacets } from "../Source/LobbyStructure.js";
import { LobbyDrivers, EmitLobbyPlateFacets } from "../Source/LobbyDrivers.js";
import { EaseCinematic, InterpolateAngle, InterpolateOrbitPose, ResolveLagBlend, BlendVector, ClampStudioEye,
         LobbyCinematicShots, EvaluateCinematicReelShot, BlendCinematicPoses } from "../Source/LobbyCameraMotion.js";
import { LobbyRosterSequence } from "../Source/LobbyRosterSequence.js";
import { LobbyAudio } from "../Source/LobbyAudio.js";
import { CabinLayout } from "../Source/CabinStructure.js";
import { KindGlassHud, CabinOverlayBindings } from "../Source/CabinSequence.js";
import { GlassHudSequence, GlassHudPixels } from "../Source/Cabin/GlassHudSequence.js";
import { SceneStructure } from "../Source/FacetStructure.js";
import { ConstructMaterialSpecification, CourseMaterial } from "../Source/MaterialSpecification.js";
import { LevelCatalog, ResolveLevelDescriptor } from "../Source/LevelStructure.js";
import { VehicleAppearance, LinearColour } from "../Source/VehicleAppearance.js";

async function BuildHeadlessLobbyScene(VenueKey = "studio")
{
    const Fs   = await import("node:fs/promises");
    const Path = await import("node:path");
    const Root = Path.resolve(Path.dirname(new URL(import.meta.url).pathname), "..");
    const PriorFetch = globalThis.fetch;
    globalThis.fetch = async (Url) =>
    {
        const Rel  = String(Url).replace(/^\.\//, "").split("?")[0];
        const Data = await Fs.readFile(Path.join(Root, Rel));
        return {
            ok: true,
            arrayBuffer: async () => Data.buffer.slice(Data.byteOffset, Data.byteOffset + Data.byteLength)
        };
    };
    try
    {
        const Scene = new SceneStructure();
        await Scene.ConstructScene("Archive", 0, "lobby", VenueKey);
        return Scene;
    }
    finally
    {
        globalThis.fetch = PriorFetch;
    }
}

test("Single-hero Lobby geometry, seamless floors, stage platform, luminaires & driver plates emit valid PBR facets", () =>
{
    const Materials = ConstructMaterialSpecification();
    assert.ok(LobbyShapes.LiftPlatform, "LobbyShapes must include LiftPlatform");
    assert.ok(LobbyShapes.NocturneLightwall, "LobbyShapes must include NocturneLightwall");
    assert.equal(LobbyVehiclePlacements.length, 0, "Single-hero Lobby must have ZERO extra guest cars");
    assert.equal(LobbyDistanceCutouts.length, 0, "Single-hero Lobby must have seamless floors with ZERO pit cutouts");

    for (const [Name, Spec] of Object.entries(LobbyShapes))
    {
        const Facets = Spec.Emit();
        assert.ok(Facets.length >= 12, `${Name} must emit facets`);
        for (const F of Facets)
        {
            assert.ok(Materials[F.Family], `${Name} uses unknown material ${F.Family}`);
            for (const Corner of F.Corners)
                for (const V of Corner)
                    assert.ok(Number.isFinite(V), `${Name} has non-finite vertex`);
        }
    }

    const Ground = EmitLobbyGroundFacets("studio");
    assert.ok(Ground.length >= 60, "Studio seamless floor + hero plinth must emit full geometry");
    assert.ok(ResolveLobbyPlacements("studio").length >= 14);
    assert.equal(ResolveLobbyStudioLights("studio").length, 5, "Softbox + 4 cove washes");

    assert.equal(LobbyDrivers.length, 5);
    for (const Driver of LobbyDrivers)
    {
        const PlateFacets = EmitLobbyPlateFacets(Driver);
        assert.ok(PlateFacets.length > 100, `Plate for ${Driver.NameTag} should emit front + rear geometry`);
        for (const F of PlateFacets)
            assert.ok(Materials[F.Family], `Plate uses unknown material ${F.Family}`);
    }
});

test("All 10 Lobby architectural venues emit valid single-hero geometry, seamless floors & lighting", async () =>
{
    const Materials = ConstructMaterialSpecification();
    assert.equal(LobbyVenues.length, 10);
    assert.deepEqual(LobbyVenues.map((V) => V.Key),
        ["studio", "monolith", "saltflats", "oculus", "nocturne", "ribstudio", "coastal", "courtyard", "gallery", "rotunda"]);

    for (const Venue of LobbyVenues)
    {
        const Desc = ResolveLevelDescriptor("lobby", Venue.Key);
        assert.equal(Desc.Lobby, true);
        assert.equal(Desc.LobbyVenueKey, Venue.Key);
        assert.equal(Desc.Title, Venue.FullTitle);
        assert.equal(Desc.DistanceCutouts.length, 0, `${Venue.Key} must have unbroken seamless floor`);

        const Ground = EmitLobbyGroundFacets(Venue.Key);
        assert.ok(Ground.length >= 16, `${Venue.Key} must emit floor facets`);
        for (const F of Ground)
            assert.ok(Materials[F.Family], `${Venue.Key} ground uses unknown material ${F.Family}`);

        const Placements = ResolveLobbyPlacements(Venue.Key);
        assert.ok(Placements.length >= 1, `${Venue.Key} must emit architectural placements`);
        for (const P of Placements)
            assert.ok(LobbyShapes[P.Shape], `${Venue.Key} references missing shape ${P.Shape}`);

        const Lights = ResolveLobbyStudioLights(Venue.Key);
        assert.ok(Array.isArray(Lights));

        const DeckFacets = EmitLiftPlatformFacets(Venue.LiftDeckMaterial);
        assert.ok(DeckFacets.some((F) => F.Family === Venue.LiftDeckMaterial),
            `${Venue.Key} hero hydraulic stage deck must match venue floor material`);

        const Scene = await BuildHeadlessLobbyScene(Venue.Key);
        assert.equal(Scene.RoadEditor, null, `${Venue.Key} must not allocate Road Studio network`);
        assert.ok(Scene.LobbyBlock, `${Venue.Key} must construct LobbyBlock`);
        assert.equal(Scene.LobbyBlock.Cars.length, 0, `${Venue.Key} must be single-hero (0 extra guest cars)`);
        assert.equal(Scene.LobbyBlock.Count, 2, `${Venue.Key} LobbyBlock must hold HeroPlates + HeroStage only`);
    }

    // Nocturne Lightwall specifics
    const NocturnePlacements = ResolveLobbyPlacements("nocturne");
    assert.ok(NocturnePlacements.some((P) => P.Shape === "NocturneLightwall"), "Nocturne must include floating NocturneLightwall");
    assert.ok(NocturnePlacements.some((P) => P.Shape === "NocturneMountains"), "Nocturne must include twilight mountain horizon");
    const NocturneLights = ResolveLobbyStudioLights("nocturne");
    assert.ok(NocturneLights.length >= 4, "Nocturne must include warm solar-yellow directional fill lights");
    assert.ok(Materials[CourseMaterial.NocturneLightwall].Emission[0] > 1.5
           && Materials[CourseMaterial.NocturneLightwall].Emission[1] > 1.3
           && Materials[CourseMaterial.NocturneLightwall].Emission[0] > Materials[CourseMaterial.NocturneLightwall].Emission[2] * 10,
        "NocturneLightwall must emit saturated luminous solar yellow without clipping to white");

    // New 5 venue specifics
    assert.ok(ResolveLobbyPlacements("ribstudio").some((P) => P.Shape === "RibStudioHall"));
    assert.ok(EmitLobbyGroundFacets("ribstudio").some((F) => F.Family === CourseMaterial.RibPlinthGlow),
        "Linear Rib Studio must include ice-blue plinth halo ring");
    assert.ok(ResolveLobbyPlacements("coastal").some((P) => P.Shape === "CoastalCanopyArch"));
    assert.ok(ResolveLobbyPlacements("coastal").some((P) => P.Shape === "CoastalHeadland"));
    assert.ok(ResolveLobbyPlacements("courtyard").some((P) => P.Shape === "SkyCourtyardHall"));
    assert.ok(ResolveLobbyPlacements("gallery").some((P) => P.Shape === "GalleryBackdrop"));
    assert.ok(ResolveLobbyPlacements("rotunda").some((P) => P.Shape === "RotundaHall"));
    assert.ok(EmitLobbyGroundFacets("rotunda").some((F) => F.Family === CourseMaterial.RotundaLed),
        "Obsidian Reveal Rotunda must include crisp white floor showcase rings");
});

test("3D Top-Left Windshield Glass Lobby Player HUD sits flush inside windshield plane without adding bind group slots", async () =>
{
    assert.equal(CabinOverlayBindings.length, 13, "Windshield Glass HUD shares ClusterTexture atlas — zero new bindings");
    assert.equal(KindGlassHud, 4);
    assert.deepEqual(GlassHudPixels, [768, 512]);

    const Hud = CabinLayout.GlassHud;
    assert.ok(Hud, "CabinLayout.GlassHud fixture must be defined");
    // Flush ~1.3 cm inside physical front windshield plane (0.589*X + 0.808*Z = 1.2886)
    const PlaneVal = 0.589 * Hud.Centre[0] + 0.808 * Hud.Centre[2];
    assert.ok(PlaneVal > 1.26 && PlaneVal < 1.285, `GlassHud plane inset expected ~1.275, got ${PlaneVal}`);
    // Positioned in the driver's top-left windshield corner (+Y driver left, Z above eye level 1.007 m)
    assert.ok(Hud.Centre[1] > 0.55, "GlassHud must sit on the left (driver) side of the windshield");
    assert.ok(Hud.Centre[2] > CabinLayout.DriverEye.Position[2] + 0.08, "GlassHud must sit in the upper windshield above eye level");
    // Normal points inward toward the driver (-X, -Z) so it renders cleanly from inside the cockpit
    assert.ok(Hud.Normal[0] < -0.5 && Hud.Normal[2] < -0.7, "GlassHud normal must face back into the cabin");

    const Scene = await BuildHeadlessLobbyScene("ribstudio");
    const Roster = new LobbyRosterSequence({ Lobby: true, Scene });
    const GlassHud = new GlassHudSequence();
    const SigA = GlassHud.SignatureFor({ Roster: Roster.QueryRosterSummary(), VenueShort: "Linear Rib" });
    assert.match(SigA, /^on\|Linear Rib\|/, "GlassHud signature must reflect active Lobby roster & venue");
    Roster.TogglePlayerConnection(4);
    const SigB = GlassHud.SignatureFor({ Roster: Roster.QueryRosterSummary(), VenueShort: "Linear Rib" });
    assert.notEqual(SigA, SigB, "GlassHud must re-upload whenever a player joins or leaves the Lobby");
    assert.equal(GlassHud.SignatureFor({ Roster: null }), "hidden", "GlassHud must hide cleanly outside Lobby");

    const OverlayWgsl = fs.readFileSync(new URL("../Shader/CabinOverlay.wgsl", import.meta.url), "utf8");
    assert.match(OverlayWgsl, /const KindGlassHud\s*:\s*u32\s*=\s*4u/, "CabinOverlay.wgsl must define KindGlassHud");
    assert.match(OverlayWgsl, /Varying\.Kind == KindGlassHud/, "CabinOverlay.wgsl must shade KindGlassHud");
});

test("LobbyCameraMotion primitives & 8-shot Automotive Film Director obey venue bounds", () =>
{
    assert.equal(EaseCinematic(0), 0);
    assert.equal(EaseCinematic(1), 1);
    assert.ok(Math.abs(EaseCinematic(0.5) - 0.5) < 1e-6);

    const Wrapped = InterpolateAngle(Math.PI * 0.9, -Math.PI * 0.9, 0.5);
    assert.ok(Math.abs(Math.abs(Wrapped) - Math.PI) < 1e-6, "Angle wrap must take shortest arc across ±π");

    const MidPose = InterpolateOrbitPose({ Yaw: 0, Pitch: 0.1, Distance: 10 }, { Yaw: 1.0, Pitch: 0.3, Distance: 6 }, 0.5);
    assert.ok(Math.abs(MidPose.Distance - 8.0) < 1e-6);

    const Lag = ResolveLagBlend(0.34, 0.34);
    assert.ok(Math.abs(Lag - (1.0 - Math.exp(-1))) < 1e-6);
    assert.deepEqual(BlendVector([0, 0, 0], [2, 4, 6], 0.5), [1, 2, 3]);

    const ClampedHigh = ClampStudioEye([30, 0, 10]);
    assert.ok(ClampedHigh[2] <= 5.25 + 1e-6);
    assert.ok(Math.hypot(ClampedHigh[0], ClampedHigh[1]) <= 18.4 + 1e-6);
    const ClampedLow = ClampStudioEye([0, 0, -2]);
    assert.ok(ClampedLow[2] >= 0.35 - 1e-6);

    assert.equal(LobbyCinematicShots.length, 8);
    for (let Shot = 0; Shot < LobbyCinematicShots.length; ++Shot)
    {
        for (const F of [0.0, 0.25, 0.5, 0.75, 1.0])
        {
            const Pose = EvaluateCinematicReelShot(Shot, F, [0, 0, 0.52], null, F * 7.0);
            assert.ok(Pose.Eye[2] >= 0.35 - 1e-6 && Pose.Eye[2] <= 5.25 + 1e-6, `Shot ${Shot} Z out of studio bounds: ${Pose.Eye[2]}`);
            assert.ok(Math.hypot(Pose.Eye[0], Pose.Eye[1]) <= 18.4 + 1e-6, `Shot ${Shot} radius out of studio bounds`);
            assert.ok(Pose.Fov >= 28.0 && Pose.Fov <= 55.0, `Shot ${Shot} FOV out of cinematic range: ${Pose.Fov}`);
        }
    }
});

test("LobbyRosterSequence tracks 5 connected/offline players for cockpit HMI & animates single-hero hydraulic stage", async () =>
{
    const Scene = await BuildHeadlessLobbyScene("nocturne");
    const Roster = new LobbyRosterSequence({ Lobby: true, Scene });
    assert.equal(Roster.Enabled, true);

    const Initial = Roster.QueryRosterSummary();
    assert.equal(Initial.MaxCount, 5);
    assert.equal(Initial.ActiveCount, 4, "4 connected players + 1 offline initially");
    assert.equal(Initial.Players.length, 5);
    assert.equal(Initial.Players[0].IsLocal, true);
    assert.equal(Initial.Players[4].Connected, false);
    assert.equal(Initial.StageRaised, false);
    assert.equal(Scene.Occupants[Scene.LobbyBlock.HeroStage].Inactive, true, "Stage must stay flush/hidden when lowered");

    // Toggle offline peer online
    assert.equal(Roster.TogglePlayerConnection(4), true);
    assert.equal(Roster.QueryRosterSummary().ActiveCount, 5);
    assert.equal(Roster.TogglePlayerConnection(4), false);
    assert.equal(Roster.QueryRosterSummary().ActiveCount, 4);

    // Raise hero showcase stage smoothly
    assert.equal(Roster.ToggleStageLift(true), true);
    let SawRising = false;
    for (let Step = 0; Step < 90; ++Step)
    {
        Roster.Advance(1.0 / 30.0);
        if (Roster.StageMotion.Phase === "rising_stage")
            SawRising = true;
    }
    assert.equal(SawRising, true, "Stage must report rising_stage phase for hydraulic foley");
    assert.ok(Math.abs(Roster.StageZ - 0.42) < 1e-3, `Raised stage must settle at +0.42 m (got ${Roster.StageZ})`);
    assert.equal(Scene.Occupants[Scene.LobbyBlock.HeroStage].Inactive, false, "Raised stage must be visible");

    // Lower back flush
    assert.equal(Roster.ToggleStageLift(false), false);
    for (let Step = 0; Step < 90; ++Step)
        Roster.Advance(1.0 / 30.0);
    assert.ok(Roster.StageZ < 1e-3, "Lowered stage must settle back to 0.0 m");
    assert.equal(Scene.Occupants[Scene.LobbyBlock.HeroStage].Inactive, true, "Lowered stage must retract flush");
});

test("VehicleAppearance updates BodyCoat/Hub materials and notifies host so SDF Surface Cards recapture", () =>
{
    const Materials = ConstructMaterialSpecification();
    const Uploads = [];
    const Appearance = new VehicleAppearance(Materials, (Offset, Data) => Uploads.push({ Offset, Bytes: Data.byteLength }));

    Appearance.Apply({ Colour: "#a4d8be", Matte: false, DarkWheels: false, BlueRims: true });
    const Expected = LinearColour("#a4d8be");
    for (let C = 0; C < 3; ++C)
        assert.ok(Math.abs(Materials[CourseMaterial.BodyCoat].Albedo[C] - Expected[C]) < 1e-6);
    assert.ok(Uploads.some((U) => U.Offset === CourseMaterial.BodyCoat * 48 && U.Bytes === 48),
        "Paint change must upload BodyCoat record and trigger SDF Surface Card recapture callback");
    assert.ok(Uploads.some((U) => U.Offset === CourseMaterial.Hub * 48 && U.Bytes === 48));
});

test("Redesigned minimal HMI supports Home Console, Connected Lobby Players, 5 Live Venues & Circuits", async () =>
{
    const { ScreenSequence } = await import("../Source/Cabin/ScreenSequence.js");
    const Noop = () => {};
    const Grad = { addColorStop: Noop };
    const StubContext = new Proxy({}, {
        get(_Target, Prop)
        {
            if (Prop === "createLinearGradient" || Prop === "createRadialGradient") return () => Grad;
            if (Prop === "measureText") return (Text) => ({ width: String(Text).length * 8 });
            return Noop;
        }
    });
    const StubCanvas = { width: 1280, height: 800, getContext: () => StubContext };
    const Screen = new ScreenSequence(StubCanvas, {}, Noop);

    // 1. Home console dock + live roster badge
    Screen.RasterScreen();
    assert.ok(Screen.Regions.some((R) => R.Identifier === "home-roster-pill"), "Home console must show live connected players pill");
    assert.ok(Screen.Regions.some((R) => R.Identifier === "dock:lobby"), "Home console must include Lobby Players dock card");
    assert.ok(Screen.Regions.some((R) => R.Identifier === "dock:venue"), "Home console must include Lobby Venue dock card");

    // 2. Lobby Players obsidian bento view
    Screen.PresentApplication("drive-room");
    Screen.RasterScreen();
    for (let I = 0; I < 5; ++I)
        assert.ok(Screen.Regions.some((R) => R.Identifier === `peer:${I}`), `Lobby Players view missing peer:${I}`);
    Screen.Regions.find((R) => R.Identifier === "peer:4").Action();
    assert.equal(Screen.TogglePeerRequest, 4, "Tapping offline peer must emit TogglePeerRequest");

    // 3. Venues view with all 5 venues including Nocturne
    Screen.PresentApplication("venues");
    Screen.RasterScreen();
    for (const V of LobbyVenues)
        assert.ok(Screen.Regions.some((R) => R.Identifier === `venue:${V.Key}`), `Venues view missing venue:${V.Key}`);
    Screen.Regions.find((R) => R.Identifier === "venue:nocturne").Action();
    assert.equal(Screen.VenueRequest, "nocturne", "Selecting Nocturne on HMI must request live in-place venue swap");

    // 4. Navigation / Circuits view preserves all route:* IDs
    Screen.PresentApplication("navigation");
    Screen.RasterScreen();
    const RouteIds = Screen.Regions.map((R) => R.Identifier).filter((K) => K.startsWith("route:"));
    for (const Expected of ["route:lobby", "route:inversion", "route:city", "route:dday", "route:candy", "route:proving"])
        assert.ok(RouteIds.includes(Expected), `Navigation screen missing hit region ${Expected}`);
    Screen.Regions.find((R) => R.Identifier === "route:candy").Action();
    assert.equal(Screen.RouteRequest, "candy", "Activating Candy Village on HMI tablet must set RouteRequest='candy'");
});

test("Zero-fade continuous cinematic transitions & LobbyAudio work headless, and switching is zero-reload/zero-recompile", async () =>
{
    const FromPose = EvaluateCinematicReelShot(0, 1.0, [0, 0, 0.52], null, 7.0);
    const ToPose   = EvaluateCinematicReelShot(5, 0.0, [0, 0, 0.52], null, 7.0);
    for (const T of [0.0, 0.25, 0.5, 0.75, 1.0])
    {
        const Mid = BlendCinematicPoses(FromPose, ToPose, T, [0, 0, 0.52], ResolveLobbyVenueDescriptor("nocturne").CameraBounds);
        const HorizDist = Math.hypot(Mid.Eye[0], Mid.Eye[1]);
        assert.ok(HorizDist >= 4.2, `Polar transition at t=${T} clipped too close to hero car (r=${HorizDist.toFixed(2)})`);
        assert.ok(Mid.Eye[2] >= 0.35 && Mid.Eye[2] <= 11.5);
    }

    const MonolithEye = ClampStudioEye([0, 24, 9.5], ResolveLobbyVenueDescriptor("monolith").CameraBounds);
    assert.ok(MonolithEye[2] > 9.0, "Monolith open-air venue should not clamp eye to 5.25m studio ceiling");
    const OculusEye = ClampStudioEye([0, 24, 9.5], ResolveLobbyVenueDescriptor("oculus").CameraBounds);
    assert.ok(OculusEye[2] <= 4.55 + 1e-6, "Oculus pavilion must clamp eye safely below concrete roof");

    const Audio = new LobbyAudio();
    assert.equal(Audio.Muted, false);
    assert.equal(Audio.ToggleMute(), true);
    assert.equal(Audio.ToggleMute(), false);
    const PanRight = Audio.ComputePan([10, 0, 0], { Position: [0, 0, 1], Right: [1, 0, 0] });
    assert.ok(PanRight > 0.7, "Stage on camera right must pan right");
    const Scene = await BuildHeadlessLobbyScene("nocturne");
    const Roster = new LobbyRosterSequence({ Lobby: true, Scene });
    Roster.ToggleStageLift(true);
    Roster.Advance(0.5);
    assert.doesNotThrow(() => Audio.Update(Roster, { Position: [0, -8, 2], Right: [1, 0, 0] }));
    assert.doesNotThrow(() => Audio.Stop());

    // Zero-reload & zero-recompile architectural guards
    const HostSrc = fs.readFileSync(new URL("../Source/IlluminationHost.js", import.meta.url), "utf8");
    assert.match(HostSrc, /async SwitchSceneInPlace\(/, "IlluminationHost must expose SwitchSceneInPlace()");
    assert.match(HostSrc, /const FrameRecordExtent\s*=\s*528/, "FrameRecordExtent must match 528-byte SceneRecords.wgsl");
    assert.doesNotMatch(HostSrc, /BeginLobbyVenue[\s\S]{0,600}location\.href/, "BeginLobbyVenue must NOT reload the page");
    assert.doesNotMatch(HostSrc, /BeginRoute[\s\S]{0,600}location\.href/, "BeginRoute must NOT reload the page");
});
