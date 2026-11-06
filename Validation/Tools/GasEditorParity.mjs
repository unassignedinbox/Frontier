// Differential checks against the live exhibit modules, not hand-copied expected values.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { GasParameterCatalogue, ConstructGasParameters, MapPlumeSettings } from '../../Experimental/VehicleHmiPrototype/Source/SmokeSequence.js';
import { GasPresets } from '../../Experimental/VehicleHmiPrototype/Source/InspectorPages.js';

const exe = process.argv[2];
const run = (...args) => execFileSync(exe, args, { encoding: 'utf8' }).trimEnd();
const read = s => Object.fromEntries(s.split('\n').map(line => { const [k, v] = line.split('\t'); return [k, Number(v)]; }));
function equalNumbers(actual, expected, description) {
    assert.deepEqual(Object.keys(actual).sort(), Object.keys(expected).sort(), `${description}: keys`);
    for (const [key, value] of Object.entries(expected)) {
        const a = actual[key], b = Number(value);
        assert.ok(Math.abs(a - b) <= 1e-12 * Math.max(1, Math.abs(b)), `${description}: ${key}: ${a} != ${b}`);
    }
}
const rows = run('catalogue').split('\n').map(line => line.split('\t'));
assert.equal(rows.length, Object.keys(GasParameterCatalogue).length);
for (const [index, [key, spec]] of Object.entries(Object.entries(GasParameterCatalogue))) {
    const row = rows[index];
    assert.deepEqual(row.slice(0, 3), [key, spec.Label, spec.Unit]);
    assert.deepEqual(row.slice(3).map(Number), [spec.Default, spec.Minimum, spec.Maximum, spec.Step]);
}
assert.deepEqual(run('presets').split('\n'), Object.keys(GasPresets));
for (const [name, overrides] of Object.entries(GasPresets))
    equalNumbers(read(run('preset', name)), { ...ConstructGasParameters(), ...overrides, Unrelated: 42 }, `preset ${name}`);
for (const enabled of [false, true]) {
    equalNumbers(read(run('map', '-', '0', enabled ? '1' : '0')),
                 MapPlumeSettings(ConstructGasParameters(), enabled), `default map enabled=${enabled}`);
    for (const [key, spec] of Object.entries(GasParameterCatalogue)) {
        for (const value of [spec.Minimum, (spec.Minimum + spec.Maximum) / 2, spec.Maximum]) {
            const gas = ConstructGasParameters(); gas[key] = value;
            equalNumbers(read(run('map', key, String(value), enabled ? '1' : '0')),
                         MapPlumeSettings(gas, enabled), `map ${key}=${value} enabled=${enabled}`);
        }
    }
}
console.log(`Gas editor parity: ${rows.length} catalogue entries, ${Object.keys(GasPresets).length} presets, 200 map cases match the exhibit`);
