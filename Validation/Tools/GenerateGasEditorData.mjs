// Generate native data from the exhibit rather than maintaining a second handwritten catalogue.
// Run from the repository root with: node Validation/Tools/GenerateGasEditorData.mjs
import { writeFile } from 'node:fs/promises';
import { GasParameterCatalogue } from '../../Experimental/VehicleHmiPrototype/Source/SmokeSequence.js';
import { PlumeDefaults } from '../../Experimental/VehicleHmiPrototype/Source/PlumeSpace.js';
import { GasPresets } from '../../Experimental/VehicleHmiPrototype/Source/InspectorPages.js';
const q = s => JSON.stringify(s);
let out = '// Generated from the experimental exhibit by Validation/Tools/GenerateGasEditorData.mjs. Do not edit.\n';
out += 'static const std::vector<GasParameterSpec> Catalogue = {\n';
for (const [key, s] of Object.entries(GasParameterCatalogue))
    out += `    { ${q(key)}, ${q(s.Label)}, ${q(s.Unit)}, ${s.Default}, ${s.Minimum}, ${s.Maximum}, ${s.Step} },\n`;
out += '};\nstatic const std::map<std::string, double> DefaultPlumeValues = {\n';
for (const [key, value] of Object.entries(PlumeDefaults))
    if (key !== 'PlumeEnabled') out += `    { ${q(key)}, ${value} },\n`;
out += '};\nstatic const std::vector<GasPreset> Presets = {\n';
for (const [name, values] of Object.entries(GasPresets))
    out += `    { ${q(name)}, { ${Object.entries(values).map(([key, value]) => `{ ${q(key)}, ${value} }`).join(', ')} } },\n`;
out += '};\n';
await writeFile('Frontier/Engine/Effects/GasEditorData.inl', out);
