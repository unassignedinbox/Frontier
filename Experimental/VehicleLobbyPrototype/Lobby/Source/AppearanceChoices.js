//====================================================================================================================================
//                                                    APPEARANCECHOICES.JS
//====================================================================================================================================
// 🧩 Maps the configurator's choices to the arguments of VehicleAppearance.Apply. Pure data, no DOM, so it runs under node --test.

// 📝 Paint choices. Black, silver and graphite follow the three studio reference images; the last is the renderer's own pale green.
export const PaintChoices = Object.freeze([
    Object.freeze({ Label: "Gloss black", Hex: "#101114" }),
    Object.freeze({ Label: "Silver", Hex: "#c4c8ce" }),
    Object.freeze({ Label: "Graphite", Hex: "#3d4248" }),
    Object.freeze({ Label: "Pale green", Hex: "#a4d8be" })
]);

export const DefaultChoice = Object.freeze({ PaintIndex: 0, Matte: false, DarkWheels: false, BlueRims: false });

const HexPattern = /^#[0-9a-fA-F]{6}$/;

// 📝 Out-of-range or malformed input falls back to the default choice instead of throwing, so a bad control never blanks the car.
export function ResolveAppearance(Choice = DefaultChoice) {
    const Index = Number.isInteger(Choice.PaintIndex) && Choice.PaintIndex >= 0 && Choice.PaintIndex < PaintChoices.length
        ? Choice.PaintIndex
        : DefaultChoice.PaintIndex;
    const Hex = PaintChoices[Index].Hex;
    if (!HexPattern.test(Hex))
        throw new Error(`Paint choice ${Index} has no valid hex colour.`);
    return {
        Colour: Hex,
        Matte: Boolean(Choice.Matte),
        DarkWheels: Boolean(Choice.DarkWheels),
        BlueRims: Boolean(Choice.BlueRims)
    };
}
