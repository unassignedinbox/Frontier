//============================================================================================================================================
//                                                        PHOTOMETRICPROFILES.JS
//============================================================================================================================================
// 🧩 IES photometric files: a parser for IESNA LM-63 text, the resampling that turns one into the GPU profile every real
//    luminaire carries, and the built-in profiles shipped with the exhibit.

// 📝 LM-63 luminaire files list candela against vertical angle (0° along the luminaire's axis, 180° opposite it) for one or
//    more horizontal planes. This renderer evaluates a rotationally symmetric distribution: the planes are averaged, so a
//    file with several planes is reduced to its mean vertical curve. Samples are normalised to the peak, so the light's
//    own intensity carries the absolute scale.

export const IesProfileSamples = 32;   // [-] per profile: uniform vertical angles 0°…180°, one float each

// 📐 Parses LM-63-19 (and LM-63-02) text. Numbers are read as one token stream after the keyword block, which tolerates
//    the line breaks real files scatter the tables across.
export function ParseIesProfile(Text)
{
    const Lines = Text.split(/\r?\n/);
    const Keywords = {};
    let TiltIndex = -1;
    for (let Index = 0; Index < Lines.length; ++Index)
    {
        const Line = Lines[Index].trim();
        if (Line.startsWith("TILT="))
        {
            TiltIndex = Index;
            break;
        }
        const Match = /^\[(\w+)\]\s*(.*)$/.exec(Line);
        if (Match) Keywords[Match[1]] = Match[2];
    }
    if (TiltIndex < 0)
        throw new Error("IES file has no TILT= line");
    if (!/^TILT=NONE\s*$/.test(Lines[TiltIndex].trim()))
        throw new Error(`IES tilt data is not supported: ${Lines[TiltIndex].trim()}`);

    const Tokens = Lines.slice(TiltIndex + 1).join(" ").trim().split(/\s+/).map(Number);
    if (Tokens.some((Value) => !Number.isFinite(Value)))
        throw new Error("IES numeric table is malformed");
    let Cursor = 0;
    const Take = (Count) => { const Slice = Tokens.slice(Cursor, Cursor + Count); Cursor += Count; return Slice; };
    const [LampCount, LumensPerLamp, CandelaMultiplier, VerticalCount, HorizontalCount, PhotometricType] = Take(6);
    Take(4);                      // units, width, length, height
    Take(3);                      // ballast factor, ballast-lamp factor, input watts
    if (!(VerticalCount >= 2) || !(HorizontalCount >= 1))
        throw new Error("IES table has no angles");
    if (PhotometricType !== 1)
        throw new Error("IES photometric type must be C for the axial distribution");
    const Vertical = Take(VerticalCount);
    const Horizontal = Take(HorizontalCount);
    const Candela = [];
    for (let Plane = 0; Plane < HorizontalCount; ++Plane)
        Candela.push(Take(VerticalCount).map((Value) => Value * CandelaMultiplier));
    if (Cursor > Tokens.length)
        throw new Error("IES candela table is truncated");

    // 📝 Average the horizontal planes at each vertical angle: one symmetric curve.
    const Mean = Vertical.map((_, Row) => Candela.reduce((Sum, Plane) => Sum + Plane[Row], 0.0) / Candela.length);
    return { Name: Keywords.TEST ?? "", Vertical, Horizontal, Mean, LampCount, LumensPerLamp, Samples: ResampleIesCurve(Vertical, Mean) };
}

// 📐 Linear resampling of the mean vertical curve onto IesProfileSamples uniform angles over 0°…180°, normalised to the peak.
export function ResampleIesCurve(Vertical, Mean)
{
    const Samples = new Float32Array(IesProfileSamples);
    for (let Sample = 0; Sample < IesProfileSamples; ++Sample)
    {
        const Angle = 180.0 * Sample / (IesProfileSamples - 1);
        let Upper = 1;
        while (Upper < Vertical.length - 1 && Vertical[Upper] < Angle) Upper++;
        const Lower = Upper - 1;
        const Span = Math.max(1.0e-9, Vertical[Upper] - Vertical[Lower]);
        const T = Math.min(1.0, Math.max(0.0, (Angle - Vertical[Lower]) / Span));
        Samples[Sample] = Mean[Lower] * (1.0 - T) + Mean[Upper] * T;
    }
    // 📝 Normalised by the resampled peak, so the brightest sample is exactly 1 whichever side of a sample the file peaks on.
    const Peak = Math.max(1.0e-9, ...Samples);
    for (let Sample = 0; Sample < IesProfileSamples; ++Sample)
        Samples[Sample] /= Peak;
    return Samples;
}

// 📐 Authors an LM-63-19 file from a vertical distribution sampled every five degrees. The built-in profiles below are
//    written with it, so each has a real .ies file on disk that a manufacturer's file can replace one for one.
export function WriteIesProfile({ Name, Description, Distribution, Lumens = 1000.0, Watts = 25.0 })
{
    const Vertical = [];
    for (let Angle = 0; Angle <= 180; Angle += 5) Vertical.push(Angle);
    const Candela = Vertical.map((Angle) => Math.max(0.0, Distribution(Angle * Math.PI / 180.0)));
    const Peak = Math.max(...Candela);
    const Scale = Peak > 0 ? 1000.0 / Peak : 1.0;
    const Rows = [];
    for (let Index = 0; Index < Vertical.length; Index += 10)
        Rows.push(Candela.slice(Index, Index + 10).map((Value) => (Value * Scale).toFixed(2)).join(" "));
    return [
        "IESNA:LM-63-2019",
        `[TEST] ${Name}`,
        `[MANUFAC] Frontier exhibit`,
        `[REMARKS] ${Description}`,
        `[LUMCAT] ${Name}`,
        "TILT=NONE",
        `1 ${Lumens} 1 ${Vertical.length} 1 1 2 0.2 0.2 0.05`,
        `1 1 ${Watts}`,
        Vertical.join(" "),
        "0",
        Rows.join("\n"),
        ""
    ].join("\n");
}

// 📝 Built-in distributions, as candela against the angle from the luminaire axis (0 = along the axis):
//    • roadway batwing: a pole luminaire aimed straight down, the batwing lobes peaking about 70° from the nadir;
//    • automotive low beam: broad, falling away steeply beyond about 60° from the axis;
//    • automotive high beam: a narrow collimated pencil beam;
//    • fog lamp: a wide, low fan;
//    • roof flood: a wide multi-lens bar.
export const BuiltinIesDistributions = Object.freeze({
    RoadwayBatwing: { Description: "Roadway batwing luminaire, pole-mounted", Distribution: (A) => Math.exp(-Math.pow((A - 1.2) / 0.5, 2.0)) * Math.sin(Math.min(Math.PI, A * 1.3)) + 0.02 },
    AutomotiveLowBeam: { Description: "Automotive low beam, broad with fall-off beyond 60 degrees", Distribution: (A) => Math.pow(Math.max(0.0, Math.cos(A * 0.9)), 2.0) * (A < 1.6 ? 1.0 : Math.max(0.0, 1.0 - (A - 1.6) * 3.0)) },
    AutomotiveHighBeam: { Description: "Automotive high beam, collimated pencil", Distribution: (A) => Math.exp(-Math.pow(A / 0.12, 2.0)) + 0.02 * Math.exp(-A * 2.0) },
    FogLamp: { Description: "Automotive fog lamp, wide low fan", Distribution: (A) => Math.exp(-Math.pow(A / 0.55, 2.0)) },
    RoofFlood: { Description: "Roof light bar, wide multi-lens flood", Distribution: (A) => Math.exp(-Math.pow(A / 0.65, 4.0)) }
});

export function BuildBuiltinIesText(Key)
{
    const Entry = BuiltinIesDistributions[Key];
    if (!Entry)
        throw new Error(`No built-in IES distribution named ${Key}`);
    return WriteIesProfile({ Name: Key, Description: Entry.Description, Distribution: Entry.Distribution });
}
