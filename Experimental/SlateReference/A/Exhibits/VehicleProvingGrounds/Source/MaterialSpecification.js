//============================================================================================================================================
//                                                         MATERIALSPECIFICATION.JS
//============================================================================================================================================
// 🧩 The surface reflectance schema, index-aligned with DriveCourse.h CourseMaterial and DriveSceneAuthor::AuthorMaterials.

// 📝 Families 0..15 carry the Project-Drive values verbatim (linear Rec.709, read from ControlVehicle.blend by
//    ExtractVehicle.py). Families 16.. are the exhibit's illumination set: walls and a tunnel whose only purpose is to
//    give the indirect term large coloured surfaces to carry, beside a course that is otherwise one horizontal plane.
export const CourseMaterial = Object.freeze({
    CheckerLight:    0,
    CheckerDark:     1,
    Surround:        2,
    Ramp:            3,
    Bump:            4,
    Cone:            5,
    BodyCoat:        6,
    VehicleGlass:    7,
    VehiclePlastic:  8,
    Tyre:            9,
    Hub:             10,
    Brake:           11,
    VehicleTrim:     12,
    Headlight:       13,
    Taillight:       14,
    VehicleDefault:  15,
    WallCrimson:     16,
    WallViridian:    17,
    WallChalk:       18,
    WallCobalt:      19,
    TunnelConcrete:  20,
    StressAmber:     21,
    StressTeal:      22,
    StressViolet:    23,
    StressIvory:     24,
    StressLamp:      25,
    // 📝 The proving grounds. The road families 26–31 are contiguous: the surface raster lays tyre rubber on exactly
    //    that range (MarkedFamilyFirst … MarkedFamilyLast), and TyreRubber is the family a marked pixel becomes.
    Asphalt:         26,
    AsphaltPatch:    27,
    ApronConcrete:   28,
    PaintWhite:      29,
    PaintYellow:     30,
    KerbRed:         31,
    TyreRubber:      32,
    Grass:           33,
    Gravel:          34,
    Cladding:        35,
    ContainerOrange: 36,
    ContainerBlue:   37,
    ContainerGreen:  38,
    ContainerWhite:  39,
    ContainerRed:    40,
    SteelDark:       41,
    LampWarm:        42,
    LampCool:        43,
    BarrierConcrete: 44,
    SeatBlue:        45,
    // 📝 Fracture: each breakable material has its outer skin and the interior a crack exposes.
    Wood:            46,
    WoodInterior:    47,
    Glass:           48,
    GlassEdge:       49,
    Concrete:        50,
    ConcreteInterior:51,
    Stone:           52,
    StoneInterior:   53,
    Plastic:         54,
    PlasticInterior: 55,
    Tempered:        56,
    TemperedEdge:    57,
    // 📝 The training arena. PaintGreen and PaintBlue are road paint too: they form the second marked range
    //    (MarkedFamilyExtraFirst … MarkedFamilyExtraLast) so rubber lands on them like on the white and yellow.
    PaintGreen:      58,
    PaintBlue:       59,
    ConeOrange:      60,
    ConeBand:        61,
    ConeBase:        62,
    // 📝 The derez / rez effect: the glowing cells of a vehicle coming apart or assembling, and the stutter of a glitch.
    DerezGlow:       63,
    DerezHot:        64,
    DerezGlitch:     65,
    GateLamp:        66,
    GateLampRed:     67,
    SignBoard:       68,
    SignLetter:      69,
    // 📝 The cabin (CabinStructure.js): trim, leather and brushed metal; the steering wheel is a family of its own so
    //    the vertex stage can turn it about the column, and the screens and mirrors are dark cover glass the cabin
    //    overlay draws their pictures onto.
    CabinLeather:    70,
    CabinTrim:       71,
    CabinAccent:     72,
    SteeringWheel:   73,
    DisplayGlass:    74,
    MirrorGlass:     75,
    SeatFabric:      76,
    CabinCarpet:     77,
    MirrorTrim:      78,
    TabletHousing:   80,
    RimBase:         81, // original MatTyre finish on METAL geometry; never shredded
    RimScrape:       79, // the rear-view mirror's housing and stem: CabinTrim's finish, fixed to the shell, not the cockpit
    // 📝 The materials garden (showcase props by the start straight): a standing mirror, two metal spheres with
    //    measured F0 colours, and a clear glass sphere whose IOR the inspector drives. Appended — never inserted —
    //    so the Drive-aligned families above keep their indices.
    Mirror:          82,
    Aluminium:       83,
    Gold:            84,
    ShowcaseGlass:   85,
    // 📝 Hovering robotic intersection traffic-signal emitters & highway amber livery (appended so 0..85 stay fixed).
    TrafficRed:      86,
    TrafficYellow:   87,
    TrafficOrange:   88,
    TrafficGreen:    89,
    BannerAmber:     90,
    // 📝 Authentic WWII Normandy D-Day palette (Omaha Beach sand/shingle/mud, US Army OD#9 Olive Drab, Channel surf, rust & burlap).
    OliveDrab:       91,
    OliveDrabDark:   92,
    WetBeachSand:    93,
    DryBeachSand:    94,
    ShinglePebble:   95,
    ScorchedMud:     96,
    ChannelWater:    97,
    RustSteel:       98,
    SandbagBurlap:   99,
    // 📝 Candy Village ("candy") physical confectionery palette (clearcoated gummies, glossy 2.5D soda/syrup, baked gingerbread & icing).
    Gingerbread:     100,
    FrostingCream:   101,
    PeppermintRed:   102,
    GummyCherry:     103,
    GummyLime:       104,
    GummyOrange:     105,
    GummyBerry:      106,
    CaramelSyrup:    107,
    BerrySodaWater:  108,
    WaferCrisp:      109,
    LicoriceBlack:   110,
    SugarCobble:     111,
    WaterShallowCrystal: 112,
    TaffyPink:       113,
    // 📝 The Lobby studio (LobbyStructure.js): seamless cove, glossy configurator floor, suspended softbox.
    StudioFloor:     114,
    StudioSurround:  115,
    StudioWall:      116,
    StudioSoftbox:   117,
    StudioFrame:     118,
    StudioTurntable: 119,
    // 📝 The display fleet's own finishes and wheel lights (lobby guests): four paints, four light colours.
    DisplayPaintA:   120,
    DisplayPaintB:   121,
    DisplayPaintC:   122,
    DisplayPaintD:   123,
    DisplayLightA:   124,
    DisplayLightB:   125,
    DisplayLightC:   126,
    DisplayLightD:   127,
    // 📝 Number plates (LobbyDrivers.js): the plate face and ink, and the Rank badge metals, one per tier.
    PlateFace:       128,
    PlateInk:        129,
    RankBronze:      130,
    RankSilver:      131,
    RankGold:        132,
    RankDiamond:     133,
    // 📝 Additional Lobby venue finishes (Monolith Plaza, Bonneville Salt Flats, Desert Oculus Pavilion).
    MonolithSlab:    134,
    MonolithFloor:   135,
    SaltCrust:       136,
    SaltMesa:        137,
    OculusConcrete:  138,
    OculusFloor:     139,
    OculusWater:     140,
    CanyonRock:      141,
    CanyonFoliage:   142,
    CanyonTrunk:     143,
    NocturneFloor:   144,
    NocturneLightwall: 145,
    NocturneMountain: 146,
    // 📝 Venues 6–10: Linear Rib Studio, Coastal Canopy, Sky Courtyard, Sculpted Gallery, Obsidian Rotunda.
    RibStudioFloor:  147,
    RibStudioWall:   148,
    RibStudioSide:   149,
    RibStudioDado:   150,
    RibStudioLed:    151,
    RibPlinthGlow:   152,
    CoastalCanopy:   153,
    CoastalMountain: 154,
    CoastalSea:      155,
    GalleryFloor:    156,
    RotundaDarkWall: 157,
    RotundaFloor:    158,
    RotundaLed:      159,
    // 📝 Assembly Line · Conveyor Gauntlet ("foundry") industrial PBR finishes & magnetic field emitters.
    FoundryBelt:       160,
    FoundrySlat:       161,
    FoundryYellow:     162,
    FoundryCopper:     163,
    FoundryMagCyan:    164,
    FoundryHazardRed:  165,
    FoundryBoostGreen: 166,
    FoundryMolten:     167,
    FoundryCargoSteel: 168,
    FoundryFloor:      169,
    // 📝 Demolish Run · Fracture City ("demolish") architectural residential, brick, garage, crane & blast PBR finishes.
    DemoBrickRed:       170,
    DemoBrickOchre:     171,
    DemoStuccoCream:    172,
    DemoStuccoSage:     173,
    DemoRoofSlate:      174,
    DemoRoofTile:       175,
    DemoTimber:         176,
    DemoGarageDoor:     177,
    DemoInteriorWall:   178,
    DemoRubbleConcrete: 179,
    DemoCraneYellow:    180,
    DemoIronBall:       181,
    DemoBlastEmissive:  182,
    DemoBeaconAmber:    183,
    DemoDetonatorRed:   184,
    DemoFinishGreen:    185
});

export const MarkedFamilyFirst = CourseMaterial.Asphalt;   // [-]
export const MarkedFamilyLast  = CourseMaterial.KerbRed;   // [-]
export const MarkedFamilyExtraFirst = CourseMaterial.PaintGreen;   // [-]
export const MarkedFamilyExtraLast  = CourseMaterial.PaintBlue;    // [-]

// 📝 The default derez palette (the Particle Editor's "Derez cube": #35d6ff glow, #ffffff hot core; "Glitch
//    dissolve": #ff2d6f), as linear emission. The VFX inspector rewrites these three records when edited.
export const DerezPalette = Object.freeze({
    Glow:   [0.035, 0.67, 1.0],
    Hot:    [1.0, 1.0, 1.0],
    Glitch: [1.0, 0.026, 0.159]
});

// 📝 The flake headroom and sweep follow DriveSceneAuthor: MetalicCoat (0.680005, 0.800458, 0) × 0.78, coat roughness
//    0.03 as authored, flake-layer specular roughness 0.055 + 0.045·0.5 from AuthorAutomotiveShowcase profile 4.
const BodyCoatHeadroom = 0.78;   // [-] - DriveSceneAuthor's flake headroom

function Dielectric(Red, Green, Blue, Roughness)
{
    return { Albedo: [Red, Green, Blue], Roughness, Metalness: 0.0, Emission: [0.0, 0.0, 0.0],
             CoatWeight: 0.0, CoatRoughness: 0.0, SpecularLevel: 0.04, Transmission: 0.0 };
}

// 📝 A metal: the albedo IS the specular F0 colour (linear space), no diffuse survives.
function Metal(Red, Green, Blue, Roughness)
{
    return { Albedo: [Red, Green, Blue], Roughness, Metalness: 1.0, Emission: [0.0, 0.0, 0.0],
             CoatWeight: 0.0, CoatRoughness: 0.0, SpecularLevel: 0.04, Transmission: 0.0 };
}

// 📐 Normal-incidence Fresnel reflectance of an air→dielectric interface: F0 = ((IOR − 1) / (IOR + 1))². The shaders
//    invert this exact formula to recover the IOR from the authored F0, so one number drives both the Schlick split
//    and the Snell bend and the two can never disagree.
export function IorToF0(Ior)
{
    const Ratio = (Ior - 1.0) / (Ior + 1.0);
    return Ratio * Ratio;
}

export function ConstructMaterialSpecification()
{
    const Materials = [];
    Materials[CourseMaterial.CheckerLight] = Dielectric(0.82, 0.82, 0.84, 0.35);
    Materials[CourseMaterial.CheckerDark]  = Dielectric(0.05, 0.05, 0.06, 0.35);
    Materials[CourseMaterial.Surround]     = Dielectric(0.35, 0.36, 0.38, 0.55);
    Materials[CourseMaterial.Ramp]         = Dielectric(0.42, 0.43, 0.45, 0.40);
    Materials[CourseMaterial.Bump]         = Dielectric(0.90, 0.72, 0.08, 0.45);
    Materials[CourseMaterial.Cone]         = Dielectric(0.95, 0.35, 0.05, 0.40);

    const Coat = Dielectric(0.680005 * BodyCoatHeadroom, 0.800458 * BodyCoatHeadroom, 0.0, 0.0775);
    Coat.CoatWeight    = 1.0;
    Coat.CoatRoughness = 0.03;
    Materials[CourseMaterial.BodyCoat] = Coat;

    // 📝 Smoked glazing: 26–30 % transmission over a dark cabin, carried as a near-black gloss. Transmission now has
    //    a real meaning in the reflection pass (a transmitted segment behind the surface), which the cabin light path
    //    is not verified for — so the car's glazing stays opaque until that path is proven, and only the showcase
    //    glass opts in.
    const Glass = Dielectric(0.026, 0.028, 0.030, 0.02);
    Glass.Transmission = 0.0;
    Materials[CourseMaterial.VehicleGlass] = Glass;

    Materials[CourseMaterial.VehiclePlastic] = Dielectric(0.012, 0.014, 0.018, 0.50);
    Materials[CourseMaterial.Tyre]           = Dielectric(0.03741, 0.03741, 0.03741, 0.76087);

    // Drive's rim uses MatTyre on most faces, not the blue mag-accent material.
    // Separate identity keeps that authored dark finish without rubber damage collapse.
    Materials[CourseMaterial.RimBase] = Dielectric(0.03741, 0.03741, 0.03741, 0.76087);

    const Hub = Dielectric(0.06, 0.22, 0.62, 0.22);
    Hub.Metalness = 1.0;
    Hub.Emission  = [0.0, 0.03327756, 0.12];
    Materials[CourseMaterial.Hub] = Hub;

    const Brake = Dielectric(0.30, 0.075, 0.025, 0.29);
    Brake.Metalness = 0.82;
    Materials[CourseMaterial.Brake] = Brake;

    Materials[CourseMaterial.VehicleTrim] = Dielectric(0.8, 0.8, 0.8, 0.50);

    const Headlight = Dielectric(0.8, 0.8, 0.8, 0.10);
    Headlight.Emission = [48.699997, 48.699997, 48.699997];
    Materials[CourseMaterial.Headlight] = Headlight;

    const Taillight = Dielectric(0.8, 0.8, 0.8, 0.10);
    Taillight.Emission = [6.5 * 1.0, 6.5 * 0.0, 6.5 * 0.016442];
    Materials[CourseMaterial.Taillight] = Taillight;

    Materials[CourseMaterial.VehicleDefault] = Dielectric(0.8, 0.8, 0.8, 0.50);

    Materials[CourseMaterial.WallCrimson]    = Dielectric(0.72, 0.05, 0.04, 0.65);
    Materials[CourseMaterial.WallViridian]   = Dielectric(0.05, 0.52, 0.16, 0.65);
    Materials[CourseMaterial.WallChalk]      = Dielectric(0.80, 0.80, 0.78, 0.75);
    Materials[CourseMaterial.WallCobalt]     = Dielectric(0.07, 0.20, 0.72, 0.65);
    Materials[CourseMaterial.TunnelConcrete] = Dielectric(0.56, 0.55, 0.52, 0.85);

    // 📝 The stress-test palette: saturated diffuse colours so the added objects carry visible colour bleeding, and
    //    one warm emitter (one slot in eleven) so a night scene full of objects is lit by the objects themselves.
    Materials[CourseMaterial.StressAmber]  = Dielectric(0.86, 0.46, 0.07, 0.55);
    Materials[CourseMaterial.StressTeal]   = Dielectric(0.05, 0.50, 0.50, 0.50);
    Materials[CourseMaterial.StressViolet] = Dielectric(0.38, 0.12, 0.62, 0.55);
    Materials[CourseMaterial.StressIvory]  = Dielectric(0.80, 0.78, 0.72, 0.60);
    const Lamp = Dielectric(0.90, 0.85, 0.75, 0.40);
    Lamp.Emission = [6.0, 4.1, 1.9];
    Materials[CourseMaterial.StressLamp] = Lamp;

    // 📝 Proving grounds. Asphalt ≈ 0.05–0.08 albedo, weathered concrete ≈ 0.4, road paint ≈ 0.75: measured orders
    //    of magnitude, so the indirect term sees a dark road between bright aprons, as it would outdoors.
    Materials[CourseMaterial.Asphalt]         = Dielectric(0.055, 0.056, 0.060, 0.82);
    Materials[CourseMaterial.AsphaltPatch]    = Dielectric(0.078, 0.076, 0.074, 0.78);
    Materials[CourseMaterial.ApronConcrete]   = Dielectric(0.42, 0.41, 0.39, 0.88);
    Materials[CourseMaterial.PaintWhite]      = Dielectric(0.78, 0.78, 0.76, 0.55);
    Materials[CourseMaterial.PaintYellow]     = Dielectric(0.80, 0.56, 0.05, 0.55);
    Materials[CourseMaterial.KerbRed]         = Dielectric(0.62, 0.05, 0.04, 0.60);
    Materials[CourseMaterial.TyreRubber]      = Dielectric(0.016, 0.016, 0.017, 0.55);
    Materials[CourseMaterial.Grass]           = Dielectric(0.09, 0.19, 0.05, 0.95);
    Materials[CourseMaterial.Gravel]          = Dielectric(0.33, 0.30, 0.26, 0.95);
    const Cladding = Dielectric(0.52, 0.55, 0.58, 0.42);
    Cladding.Metalness = 0.65;
    Materials[CourseMaterial.Cladding]        = Cladding;
    Materials[CourseMaterial.ContainerOrange] = Dielectric(0.76, 0.24, 0.04, 0.55);
    Materials[CourseMaterial.ContainerBlue]   = Dielectric(0.04, 0.15, 0.44, 0.55);
    Materials[CourseMaterial.ContainerGreen]  = Dielectric(0.06, 0.33, 0.13, 0.55);
    Materials[CourseMaterial.ContainerWhite]  = Dielectric(0.74, 0.74, 0.71, 0.55);
    Materials[CourseMaterial.ContainerRed]    = Dielectric(0.56, 0.06, 0.05, 0.55);
    const Steel = Dielectric(0.10, 0.10, 0.11, 0.38);
    Steel.Metalness = 1.0;
    Materials[CourseMaterial.SteelDark]       = Steel;
    const Warm = Dielectric(0.95, 0.90, 0.80, 0.30);
    Warm.Emission = [14.0, 10.2, 6.0];
    Materials[CourseMaterial.LampWarm]        = Warm;
    const Cool = Dielectric(0.90, 0.92, 0.95, 0.30);
    Cool.Emission = [7.0, 8.4, 10.0];
    Materials[CourseMaterial.LampCool]        = Cool;
    Materials[CourseMaterial.BarrierConcrete] = Dielectric(0.60, 0.60, 0.57, 0.85);
    Materials[CourseMaterial.SeatBlue]        = Dielectric(0.05, 0.20, 0.55, 0.50);

    // 📝 Fracture skins and interiors (FractureEditor materials.ts colours, linearised).
    Materials[CourseMaterial.Wood]             = Dielectric(0.46, 0.24, 0.07, 0.75);
    Materials[CourseMaterial.WoodInterior]     = Dielectric(0.69, 0.47, 0.21, 0.82);
    const Pane = Dielectric(0.10, 0.15, 0.17, 0.04);
    Pane.SpecularLevel = 0.06;
    Materials[CourseMaterial.Glass]            = Pane;
    Materials[CourseMaterial.GlassEdge]        = Dielectric(0.45, 0.66, 0.70, 0.20);
    Materials[CourseMaterial.Concrete]         = Dielectric(0.34, 0.34, 0.31, 0.95);
    Materials[CourseMaterial.ConcreteInterior] = Dielectric(0.27, 0.26, 0.23, 0.97);
    Materials[CourseMaterial.Stone]            = Dielectric(0.20, 0.20, 0.19, 0.92);
    Materials[CourseMaterial.StoneInterior]    = Dielectric(0.29, 0.25, 0.21, 0.95);
    Materials[CourseMaterial.Plastic]          = Dielectric(0.69, 0.09, 0.03, 0.42);
    Materials[CourseMaterial.PlasticInterior]  = Dielectric(0.87, 0.48, 0.37, 0.50);
    const Tempered = Dielectric(0.12, 0.18, 0.20, 0.06);
    Tempered.SpecularLevel = 0.06;
    Materials[CourseMaterial.Tempered]         = Tempered;
    Materials[CourseMaterial.TemperedEdge]     = Dielectric(0.58, 0.76, 0.80, 0.25);

    // 📝 Training arena paint and the slalom cone: fluorescent orange PVC (a little self-luminous under daylight, as
    //    fluorescent pigment is), retroreflective collars and a black recycled-rubber base.
    Materials[CourseMaterial.PaintGreen] = Dielectric(0.08, 0.48, 0.16, 0.55);
    Materials[CourseMaterial.PaintBlue]  = Dielectric(0.05, 0.22, 0.62, 0.55);
    const ConeOrange = Dielectric(0.92, 0.22, 0.02, 0.36);
    ConeOrange.Emission = [0.09, 0.022, 0.0];
    ConeOrange.CoatWeight = 0.35;
    ConeOrange.CoatRoughness = 0.18;
    Materials[CourseMaterial.ConeOrange] = ConeOrange;
    const ConeBand = Dielectric(0.86, 0.87, 0.88, 0.22);
    ConeBand.Emission = [0.05, 0.05, 0.052];
    ConeBand.SpecularLevel = 0.08;
    Materials[CourseMaterial.ConeBand] = ConeBand;
    Materials[CourseMaterial.ConeBase] = Dielectric(0.025, 0.025, 0.027, 0.85);
    const Glow = Dielectric(0.05, 0.10, 0.12, 0.30);
    Glow.Emission = DerezPalette.Glow.map((Channel) => Channel * 5.0);
    Materials[CourseMaterial.DerezGlow] = Glow;
    const Hot = Dielectric(0.20, 0.20, 0.20, 0.30);
    Hot.Emission = DerezPalette.Hot.map((Channel) => Channel * 10.0);
    Materials[CourseMaterial.DerezHot] = Hot;
    const Glitch = Dielectric(0.10, 0.03, 0.05, 0.30);
    Glitch.Emission = DerezPalette.Glitch.map((Channel) => Channel * 5.0);
    Materials[CourseMaterial.DerezGlitch] = Glitch;
    const GateLamp = Dielectric(0.30, 0.90, 0.40, 0.30);
    GateLamp.Emission = [1.2, 9.0, 2.4];
    Materials[CourseMaterial.GateLamp] = GateLamp;
    const GateLampRed = Dielectric(0.90, 0.20, 0.15, 0.30);
    GateLampRed.Emission = [9.0, 1.0, 0.6];
    Materials[CourseMaterial.GateLampRed] = GateLampRed;
    Materials[CourseMaterial.SignBoard] = Dielectric(0.035, 0.037, 0.042, 0.60);
    const Letter = Dielectric(0.82, 0.82, 0.80, 0.45);
    Letter.Emission = [1.6, 1.6, 1.55];
    Materials[CourseMaterial.SignLetter] = Letter;
    // 📝 Cabin: charcoal leather and soft-touch trim (≈ 4–6 % albedo, as automotive interiors measure), satin
    //    aluminium accents, near-black cover glass for the displays and a silvered mirror base.
    Materials[CourseMaterial.CabinLeather] = Dielectric(0.045, 0.040, 0.038, 0.55);
    Materials[CourseMaterial.CabinTrim]    = Dielectric(0.030, 0.031, 0.034, 0.72);
    const Accent = Dielectric(0.62, 0.63, 0.65, 0.32);
    Accent.Metalness = 1.0;
    Materials[CourseMaterial.CabinAccent]  = Accent;
    Materials[CourseMaterial.SteeringWheel] = Dielectric(0.040, 0.036, 0.034, 0.50);
    const Cover = Dielectric(0.008, 0.009, 0.010, 0.06);
    Cover.SpecularLevel = 0.05;
    Materials[CourseMaterial.DisplayGlass] = Cover;
    Materials[CourseMaterial.TabletHousing] = Materials[CourseMaterial.CabinTrim];
    const Silver = Dielectric(0.10, 0.10, 0.11, 0.05);
    Silver.Metalness = 1.0;
    Materials[CourseMaterial.MirrorGlass]  = Silver;
    Materials[CourseMaterial.SeatFabric]   = Dielectric(0.055, 0.060, 0.075, 0.85);
    Materials[CourseMaterial.CabinCarpet]  = Dielectric(0.028, 0.028, 0.030, 0.95);
    Materials[CourseMaterial.MirrorTrim]   = Materials[CourseMaterial.CabinTrim];
    // Abraded aggregate and metal transfer: lighter, smoother than rubber, not a chrome strip.
    const RimScrape = Dielectric(0.16, 0.17, 0.18, 0.46);
    RimScrape.Metalness = 0.25;
    Materials[CourseMaterial.RimScrape] = RimScrape;

    // 📝 The materials garden: a standing mirror (glass over silver reads as bright metal at these scales), aluminium
    //    and gold spheres in linear-space F0 colours, and a clear glass sphere — no diffuse at all, its F0 from the
    //    default IOR 1.5 until the slider rewrites it.
    Materials[CourseMaterial.Mirror]    = Metal(1.00, 1.00, 1.00, 0.02);
    Materials[CourseMaterial.Aluminium] = Metal(0.91, 0.92, 0.92, 0.12);
    Materials[CourseMaterial.Gold]      = Metal(1.00, 0.71, 0.29, 0.08);
    const Showcase = Dielectric(0.0, 0.0, 0.0, 0.04);
    Showcase.SpecularLevel = IorToF0(1.5);
    Showcase.Transmission = 1.0;
    Materials[CourseMaterial.ShowcaseGlass] = Showcase;

    const Emitter = (AlbedoR, AlbedoG, AlbedoB, EmitR, EmitG, EmitB) =>
    {
        const Mat = Dielectric(AlbedoR, AlbedoG, AlbedoB, 0.14);
        Mat.Emission = [EmitR, EmitG, EmitB];
        return Mat;
    };
    Materials[CourseMaterial.TrafficRed]    = Emitter(0.85, 0.04, 0.03, 14.5,  0.28, 0.10);
    Materials[CourseMaterial.TrafficYellow] = Emitter(0.95, 0.78, 0.04, 13.5, 10.20, 0.18);
    Materials[CourseMaterial.TrafficOrange] = Emitter(0.95, 0.30, 0.02, 14.2,  3.10, 0.06);
    Materials[CourseMaterial.TrafficGreen]  = Emitter(0.04, 0.85, 0.18,  0.16, 13.80, 1.95);
    Materials[CourseMaterial.BannerAmber]   = Dielectric(0.86, 0.46, 0.03, 0.38);

    // WWII Normandy D-Day measured linear reflectance palette:
    Materials[CourseMaterial.OliveDrab]     = Dielectric(0.102, 0.114, 0.058, 0.86); // US Army OD#9 FS-34087 matte olive drab
    Materials[CourseMaterial.OliveDrabDark] = Dielectric(0.058, 0.066, 0.036, 0.90); // Mud-darkened lower olive drab / Wehrmacht drab
    const WetSand = Dielectric(0.155, 0.132, 0.098, 0.22);
    WetSand.SpecularLevel = 0.055;
    Materials[CourseMaterial.WetBeachSand]  = WetSand;                               // Glossy low-tide wet Normandy sand
    Materials[CourseMaterial.DryBeachSand]  = Dielectric(0.320, 0.272, 0.194, 0.92); // Paleochre dry Normandy sand
    Materials[CourseMaterial.ShinglePebble] = Dielectric(0.205, 0.190, 0.168, 0.94); // Flint/cobble Omaha shingle bank
    Materials[CourseMaterial.ScorchedMud]   = Dielectric(0.078, 0.064, 0.048, 0.72); // Blast-churned wet crater & trench mud
    const SurfWater = Dielectric(0.028, 0.052, 0.058, 0.09);
    SurfWater.SpecularLevel = 0.06;
    Materials[CourseMaterial.ChannelWater]  = SurfWater;                             // Cold English Channel surf
    Materials[CourseMaterial.RustSteel]     = Metal(0.24, 0.14, 0.09, 0.60);         // Salt-corroded Czech hedgehog & barbed wire steel
    Materials[CourseMaterial.SandbagBurlap] = Dielectric(0.275, 0.232, 0.152, 0.96); // Stacked khaki burlap sandbags

    // Candy Village ("candy") physical confectionery PBR palette (with REAL Snell-law ray-traced Transmission on Jelly & 2.5D Water!):
    Materials[CourseMaterial.Gingerbread]   = Dielectric(0.355, 0.155, 0.052, 0.88); // Warm spiced baked gingerbread brick
    Materials[CourseMaterial.FrostingCream] = Dielectric(0.865, 0.825, 0.755, 0.58); // Whipped royal-icing snow & breaking wave whitecaps
    const Peppermint = Dielectric(0.740, 0.042, 0.062, 0.16);
    Peppermint.CoatWeight = 1.0; Peppermint.CoatRoughness = 0.03; Peppermint.SpecularLevel = IorToF0(1.50);
    Materials[CourseMaterial.PeppermintRed] = Peppermint;

    // Translucent Refractive Fruit Jelly (Transmission = 1.0 -> real Snell refraction + Fresnel reflection + soft subsurface fruit glow!):
    const MakeJelly = (R, G, B) => {
        const Jl = Dielectric(R * 0.38, G * 0.38, B * 0.38, 0.055);
        Jl.SpecularLevel = IorToF0(1.49);
        Jl.Transmission  = 1.0;
        Jl.Emission      = [R * 0.28, G * 0.28, B * 0.28];
        return Jl;
    };
    Materials[CourseMaterial.GummyCherry]   = MakeJelly(0.88, 0.045, 0.11);
    Materials[CourseMaterial.GummyLime]     = MakeJelly(0.09, 0.820, 0.15);
    Materials[CourseMaterial.GummyOrange]   = MakeJelly(0.95, 0.280, 0.03);
    Materials[CourseMaterial.GummyBerry]    = MakeJelly(0.06, 0.300, 0.92);

    const Syrup = Dielectric(0.340, 0.125, 0.018, 0.045);
    Syrup.CoatWeight = 1.0; Syrup.CoatRoughness = 0.02; Syrup.SpecularLevel = IorToF0(1.50);
    Materials[CourseMaterial.CaramelSyrup]   = Syrup;

    // Refractive 2.5D Fizzy Lagoon Water (deep sapphire-turquoise + shallow aquamarine crystal, Transmission = 1.0!):
    const SodaDeep = Dielectric(0.014, 0.135, 0.220, 0.032);
    SodaDeep.SpecularLevel = IorToF0(1.335);
    SodaDeep.Transmission  = 1.0;
    Materials[CourseMaterial.BerrySodaWater] = SodaDeep;

    const SodaShallow = Dielectric(0.055, 0.235, 0.245, 0.035);
    SodaShallow.SpecularLevel = IorToF0(1.335);
    SodaShallow.Transmission  = 1.0;
    Materials[CourseMaterial.WaterShallowCrystal] = SodaShallow;

    const Taffy = Dielectric(0.840, 0.210, 0.460, 0.15);
    Taffy.CoatWeight = 1.0; Taffy.CoatRoughness = 0.03; Taffy.SpecularLevel = IorToF0(1.48);
    Materials[CourseMaterial.TaffyPink]      = Taffy;

    Materials[CourseMaterial.WaferCrisp]     = Dielectric(0.545, 0.365, 0.150, 0.80); // Golden waffle-wafer bridge & catapult timber
    const Licorice = Dielectric(0.022, 0.020, 0.026, 0.22);
    Licorice.CoatWeight = 0.85; Licorice.CoatRoughness = 0.06;
    Materials[CourseMaterial.LicoriceBlack]  = Licorice;
    Materials[CourseMaterial.SugarCobble]    = Dielectric(0.485, 0.395, 0.285, 0.78); // Warm biscuit-sugar village cobblestones

    // 📝 Lobby studio finishes: a polished white floor the clearcoat can mirror, a satin dark surround,
    //    a matte cove wall, and the suspended softbox emitter itself.
    Materials[CourseMaterial.StudioFloor]     = Dielectric(0.80, 0.81, 0.82, 0.13);
    Materials[CourseMaterial.StudioSurround]  = Dielectric(0.030, 0.032, 0.036, 0.34);
    Materials[CourseMaterial.StudioWall]      = Dielectric(0.88, 0.89, 0.90, 0.52);
    const Softbox = Dielectric(0.92, 0.93, 0.95, 0.40);
    Softbox.Emission = [6.5, 6.7, 7.0];
    Materials[CourseMaterial.StudioSoftbox]   = Softbox;
    const StudioFrame = Dielectric(0.014, 0.015, 0.017, 0.48);
    StudioFrame.Metalness = 0.4;
    Materials[CourseMaterial.StudioFrame]     = StudioFrame;
    Materials[CourseMaterial.StudioTurntable] = Dielectric(0.76, 0.77, 0.78, 0.08);

    // 📝 Display paints: the Garage palette as clearcoats, each guest car in its own colour.
    const DisplayPaint = (Hex) => {
        const Coat = Dielectric(...[0, 1, 2].map((Channel) => {
            const Value = parseInt(Hex.slice(1 + Channel * 2, 3 + Channel * 2), 16) / 255.0;
            return (Value <= 0.04045 ? Value / 12.92 : ((Value + 0.055) / 1.055) ** 2.4) * BodyCoatHeadroom;
        }), 0.0775);
        Coat.CoatWeight = 1.0; Coat.CoatRoughness = 0.03;
        return Coat;
    };
    Materials[CourseMaterial.DisplayPaintA] = DisplayPaint("#a4d8be");
    Materials[CourseMaterial.DisplayPaintB] = DisplayPaint("#d4bd88");
    Materials[CourseMaterial.DisplayPaintC] = DisplayPaint("#9eb8df");
    Materials[CourseMaterial.DisplayPaintD] = DisplayPaint("#e2ada4");

    // 📝 Display wheel lights: dark metal hubs, each carrying its own emissive colour.
    const DisplayLight = (Emission) => {
        const Hub = Dielectric(0.04, 0.045, 0.05, 0.25);
        Hub.Metalness = 1.0; Hub.Emission = Emission;
        return Hub;
    };
    Materials[CourseMaterial.DisplayLightA] = DisplayLight([0.2, 3.2, 2.4]);
    Materials[CourseMaterial.DisplayLightB] = DisplayLight([3.4, 2.0, 0.3]);
    Materials[CourseMaterial.DisplayLightC] = DisplayLight([0.5, 1.4, 3.6]);
    Materials[CourseMaterial.DisplayLightD] = DisplayLight([3.2, 0.7, 2.4]);

    // 📝 Plates: a retroreflective white face, near-black ink, and the Rank badge metals.
    const PlateFace = Dielectric(0.86, 0.87, 0.88, 0.32);
    PlateFace.Emission = [0.10, 0.10, 0.105];
    Materials[CourseMaterial.PlateFace]  = PlateFace;
    Materials[CourseMaterial.PlateInk]   = Dielectric(0.012, 0.012, 0.014, 0.55);
    const BadgeMetal = (Red, Green, Blue) => { const Metal = Dielectric(Red, Green, Blue, 0.24); Metal.Metalness = 1.0; return Metal; };
    Materials[CourseMaterial.RankBronze]  = BadgeMetal(0.62, 0.32, 0.14);
    Materials[CourseMaterial.RankSilver]  = BadgeMetal(0.72, 0.74, 0.78);
    Materials[CourseMaterial.RankGold]    = BadgeMetal(1.00, 0.72, 0.24);
    Materials[CourseMaterial.RankDiamond] = BadgeMetal(0.62, 0.86, 1.00);

    // 📝 Lobby venue materials: Monolith Plaza, Bonneville Salt Flats, and Desert Oculus Pavilion.
    Materials[CourseMaterial.MonolithSlab]   = Dielectric(0.88, 0.90, 0.93, 0.68);
    const MonolithFloor = Dielectric(0.68, 0.74, 0.82, 0.08);
    MonolithFloor.CoatWeight = 0.92;
    MonolithFloor.CoatRoughness = 0.03;
    Materials[CourseMaterial.MonolithFloor]  = MonolithFloor;

    Materials[CourseMaterial.SaltCrust]      = Dielectric(0.86, 0.84, 0.79, 0.88);
    Materials[CourseMaterial.SaltMesa]       = Dielectric(0.42, 0.40, 0.39, 0.94);

    Materials[CourseMaterial.OculusConcrete] = Dielectric(0.48, 0.43, 0.37, 0.78);
    Materials[CourseMaterial.OculusFloor]    = Dielectric(0.58, 0.54, 0.48, 0.42);
    const OculusWater = Dielectric(0.012, 0.018, 0.024, 0.015);
    OculusWater.CoatWeight = 1.0;
    OculusWater.CoatRoughness = 0.012;
    OculusWater.SpecularLevel = 0.16;
    Materials[CourseMaterial.OculusWater]    = OculusWater;
    Materials[CourseMaterial.CanyonRock]     = Dielectric(0.46, 0.21, 0.13, 0.88);
    Materials[CourseMaterial.CanyonFoliage]  = Dielectric(0.14, 0.19, 0.09, 0.88);
    Materials[CourseMaterial.CanyonTrunk]    = Dielectric(0.09, 0.06, 0.04, 0.92);

    // 📝 Venue 5: Nocturne Monolith Lightwall — dark wet-satin floor, rich solar-yellow lightbox & twilight ridge.
    const NocturneFloor = Dielectric(0.022, 0.025, 0.031, 0.18);
    NocturneFloor.CoatWeight = 0.42;
    NocturneFloor.CoatRoughness = 0.14;
    Materials[CourseMaterial.NocturneFloor]     = NocturneFloor;
    const NocturneLightwall = Dielectric(0.96, 0.86, 0.08, 0.28);
    NocturneLightwall.Emission = [2.15, 1.85, 0.02];
    Materials[CourseMaterial.NocturneLightwall] = NocturneLightwall;
    Materials[CourseMaterial.NocturneMountain]  = Dielectric(0.012, 0.015, 0.022, 0.96);

    // 📝 Venue 6: Linear Rib Studio (curved white LED ribs + dark charcoal acoustic sides + obsidian tiled floor + cyan plinth halo).
    const RibStudioFloor = Dielectric(0.022, 0.024, 0.028, 0.16);
    RibStudioFloor.CoatWeight = 0.55;
    RibStudioFloor.CoatRoughness = 0.10;
    Materials[CourseMaterial.RibStudioFloor] = RibStudioFloor;
    Materials[CourseMaterial.RibStudioWall]  = Dielectric(0.86, 0.86, 0.85, 0.68);
    Materials[CourseMaterial.RibStudioSide]  = Dielectric(0.042, 0.045, 0.050, 0.78);
    const RibStudioDado = Dielectric(0.085, 0.064, 0.040, 0.42);
    RibStudioDado.Metalness = 0.22;
    Materials[CourseMaterial.RibStudioDado]  = RibStudioDado;
    const RibStudioLed = Dielectric(1.0, 0.99, 0.96, 0.22);
    RibStudioLed.Emission = [3.8, 3.7, 3.45];
    Materials[CourseMaterial.RibStudioLed]   = RibStudioLed;
    const RibPlinthGlow = Dielectric(0.35, 0.78, 1.0, 0.20);
    RibPlinthGlow.Emission = [0.60, 1.85, 2.85];
    Materials[CourseMaterial.RibPlinthGlow]  = RibPlinthGlow;

    // 📝 Venue 7 & 8: Coastal Canopy & Sky Courtyard — architectural warm-white cast concrete, distant sea & headland rock.
    const CoastalCanopy = Dielectric(0.84, 0.85, 0.87, 0.54);
    CoastalCanopy.CoatWeight = 0.12;
    CoastalCanopy.CoatRoughness = 0.36;
    Materials[CourseMaterial.CoastalCanopy]   = CoastalCanopy;
    Materials[CourseMaterial.CoastalMountain] = Dielectric(0.032, 0.040, 0.056, 0.94);
    const CoastalSea = Dielectric(0.20, 0.34, 0.48, 0.18);
    CoastalSea.CoatWeight = 0.45;
    CoastalSea.CoatRoughness = 0.10;
    Materials[CourseMaterial.CoastalSea]      = CoastalSea;

    // 📝 Venue 9: Sculpted Horizon Gallery — silky brushed cool-titanium satin reflective floor.
    const GalleryFloor = Dielectric(0.32, 0.35, 0.40, 0.19);
    GalleryFloor.CoatWeight = 0.48;
    GalleryFloor.CoatRoughness = 0.13;
    Materials[CourseMaterial.GalleryFloor]    = GalleryFloor;

    // 📝 Venue 10: Obsidian Reveal Rotunda — stealth matte graphite cylinder, jet-black mirror floor & cool-white halo LED.
    Materials[CourseMaterial.RotundaDarkWall] = Dielectric(0.032, 0.035, 0.041, 0.66);
    const RotundaFloor = Dielectric(0.014, 0.016, 0.020, 0.13);
    RotundaFloor.CoatWeight = 0.62;
    RotundaFloor.CoatRoughness = 0.09;
    Materials[CourseMaterial.RotundaFloor]    = RotundaFloor;
    const RotundaLed = Dielectric(0.95, 0.97, 1.0, 0.18);
    RotundaLed.Emission = [3.2, 3.45, 3.9];
    Materials[CourseMaterial.RotundaLed]      = RotundaLed;

    // 📝 Assembly Line · Conveyor Gauntlet ("foundry") industrial factory, conveyor belt & electromagnet PBR palette.
    Materials[CourseMaterial.FoundryBelt] = Dielectric(0.052, 0.058, 0.066, 0.62);
    const FoundrySlat = Dielectric(0.24, 0.27, 0.32, 0.34);
    FoundrySlat.Metalness = 0.82;
    Materials[CourseMaterial.FoundrySlat] = FoundrySlat;
    const FoundryYellow = Dielectric(0.92, 0.66, 0.04, 0.36);
    FoundryYellow.CoatWeight = 0.28;
    FoundryYellow.CoatRoughness = 0.16;
    Materials[CourseMaterial.FoundryYellow] = FoundryYellow;
    const FoundryCopper = Dielectric(0.88, 0.42, 0.18, 0.26);
    FoundryCopper.Metalness = 0.92;
    Materials[CourseMaterial.FoundryCopper] = FoundryCopper;
    const FoundryMagCyan = Dielectric(0.32, 0.92, 1.0, 0.18);
    FoundryMagCyan.Emission = [0.65, 2.75, 3.60];
    Materials[CourseMaterial.FoundryMagCyan] = FoundryMagCyan;
    const FoundryHazardRed = Dielectric(1.0, 0.18, 0.14, 0.22);
    FoundryHazardRed.Emission = [3.20, 0.32, 0.18];
    Materials[CourseMaterial.FoundryHazardRed] = FoundryHazardRed;
    const FoundryBoostGreen = Dielectric(0.20, 1.0, 0.55, 0.20);
    FoundryBoostGreen.Emission = [0.35, 3.00, 1.35];
    Materials[CourseMaterial.FoundryBoostGreen] = FoundryBoostGreen;
    const FoundryMolten = Dielectric(1.0, 0.42, 0.06, 0.30);
    FoundryMolten.Emission = [3.40, 1.05, 0.08];
    Materials[CourseMaterial.FoundryMolten] = FoundryMolten;
    const FoundryCargoSteel = Dielectric(0.44, 0.48, 0.54, 0.30);
    FoundryCargoSteel.Metalness = 0.85;
    Materials[CourseMaterial.FoundryCargoSteel] = FoundryCargoSteel;
    const FoundryFloor = Dielectric(0.15, 0.16, 0.18, 0.42);
    FoundryFloor.CoatWeight = 0.22;
    FoundryFloor.CoatRoughness = 0.22;
    Materials[CourseMaterial.FoundryFloor] = FoundryFloor;

    // 📝 Demolish Run · Fracture City ("demolish") architectural residential, brick, garage, crane & blast PBR finishes.
    Materials[CourseMaterial.DemoBrickRed]       = Dielectric(0.42, 0.13, 0.085, 0.86);
    Materials[CourseMaterial.DemoBrickOchre]     = Dielectric(0.54, 0.34, 0.17,  0.84);
    Materials[CourseMaterial.DemoStuccoCream]    = Dielectric(0.75, 0.70, 0.61,  0.78);
    Materials[CourseMaterial.DemoStuccoSage]     = Dielectric(0.32, 0.41, 0.43,  0.74);
    Materials[CourseMaterial.DemoRoofSlate]      = Dielectric(0.11, 0.13, 0.17,  0.72);
    Materials[CourseMaterial.DemoRoofTile]       = Dielectric(0.52, 0.19, 0.10,  0.76);
    Materials[CourseMaterial.DemoTimber]         = Dielectric(0.40, 0.24, 0.11,  0.74);
    const DemoGarageDoor = Dielectric(0.81, 0.82, 0.84, 0.38);
    DemoGarageDoor.CoatWeight = 0.26;
    DemoGarageDoor.CoatRoughness = 0.20;
    Materials[CourseMaterial.DemoGarageDoor]     = DemoGarageDoor;
    Materials[CourseMaterial.DemoInteriorWall]   = Dielectric(0.74, 0.71, 0.66,  0.82);
    Materials[CourseMaterial.DemoRubbleConcrete] = Dielectric(0.38, 0.37, 0.35,  0.90);
    const DemoCraneYellow = Dielectric(0.92, 0.64, 0.03, 0.34);
    DemoCraneYellow.Metalness = 0.32;
    DemoCraneYellow.CoatWeight = 0.30;
    DemoCraneYellow.CoatRoughness = 0.16;
    Materials[CourseMaterial.DemoCraneYellow]    = DemoCraneYellow;
    const DemoIronBall = Dielectric(0.09, 0.10, 0.12, 0.32);
    DemoIronBall.Metalness = 0.94;
    Materials[CourseMaterial.DemoIronBall]       = DemoIronBall;
    const DemoBlastEmissive = Dielectric(1.0, 0.38, 0.06, 0.25);
    DemoBlastEmissive.Emission = [3.80, 1.08, 0.12];
    Materials[CourseMaterial.DemoBlastEmissive]  = DemoBlastEmissive;
    const DemoBeaconAmber = Dielectric(1.0, 0.62, 0.06, 0.22);
    DemoBeaconAmber.Emission = [3.40, 1.85, 0.10];
    Materials[CourseMaterial.DemoBeaconAmber]    = DemoBeaconAmber;
    const DemoDetonatorRed = Dielectric(1.0, 0.12, 0.10, 0.20);
    DemoDetonatorRed.Emission = [3.85, 0.18, 0.12];
    Materials[CourseMaterial.DemoDetonatorRed]   = DemoDetonatorRed;
    const DemoFinishGreen = Dielectric(0.18, 1.0, 0.48, 0.20);
    DemoFinishGreen.Emission = [0.28, 3.20, 1.15];
    Materials[CourseMaterial.DemoFinishGreen]    = DemoFinishGreen;

    return Materials;
}

// 📝 48-byte records matching MaterialRecord in SceneRecords.wgsl.
export function EncodeMaterialRecords(Materials)
{
    const Scalars = new Float32Array(Materials.length * 12);
    Materials.forEach((Material, Index) =>
    {
        const Offset = Index * 12;
        Scalars.set([...Material.Albedo, Material.Roughness, ...Material.Emission, Material.Metalness,
                     Material.CoatWeight, Material.CoatRoughness, Material.SpecularLevel, Material.Transmission], Offset);
    });
    return Scalars;
}
