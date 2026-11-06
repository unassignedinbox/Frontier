// Compare C++ schedule/record directly with the original PlumeSpace, without creating GPU resources.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { ConstructGasParameters, MapPlumeSettings } from '../../Experimental/VehicleHmiPrototype/Source/SmokeSequence.js';
import { PlumeSpace, PlumeGrades } from '../../Experimental/VehicleHmiPrototype/Source/PlumeSpace.js';
const lines = execFileSync(process.argv[2], { encoding: 'utf8' }).trim().split('\n').map(x => x.split(' ').map(Number));
let line = 0;
const pass = { setPipeline(){}, setBindGroup(){}, dispatchWorkgroups(){}, end(){} };
const encoder = { beginComputePass(){ return pass; } };
for (const grade of PlumeGrades) {
    const p = new PlumeSpace({Device:{queue:{writeBuffer(){}}}}, {}, grade);
    p.AdvectVelocityGroups = [0,0]; p.VorticityGroups = [0,0]; p.PressureGroups = [[0,0],[0,0]];
    p.AdvectThermoGroups = [0,0]; p.LightGroups = [0,0];
    for (let i=0; i<11; i++) {
        if (i===9) p.Clear();
        const gas = ConstructGasParameters(); if(i===5){gas.WindSpeed=5.5;gas.FlameLoss=0.995;}
        const settings = MapPlumeSettings(gas, i!==8); if(i===5) settings.Inspection=10;
        const count = [1,2,3,5,8,10].includes(i) ? 2 : 0;
        const sources = {Count:count, Records:new Float32Array(24), ResolveBounds:()=>({
            Minimum:count ? [-3.4,-2.5,0.1] : [Infinity,Infinity,Infinity],
            Maximum:count ? [4.8,5.5,6.6] : [-Infinity,-Infinity,-Infinity]})};
        const time = [0.01,0.008,0.008,0.008,0.02,0.1,0.025,0.04,0.016,0.01,0.02][i];
        p.Schedule(time,time,i>=5?[40,-20,0]:[-1,2,0],sources,settings,[0.1,0.2,0.9]);
        const row=lines[line++], expected=[Number(p.Resident),Number(p.StepPending),p.State,p.StepsTaken,...p.Origin];
        assert.deepEqual(row.slice(0,6),expected,`${grade.Name} frame ${i}: schedule`);
        const u = new Uint32Array(p.RecordBytes), f = new Float32Array(p.RecordBytes);
        for(let j=0;j<64;j++) {
            const got=row[6+j];
            if(j<16) assert.equal(got,u[j],`${grade.Name} frame ${i} word ${j}`);
            else {
                const view=new DataView(new ArrayBuffer(4));view.setUint32(0,got,true);const actual=view.getFloat32(0,true);
                assert.ok(Math.abs(actual-f[j])<=1e-6*Math.max(1,Math.abs(f[j])),`${grade.Name} frame ${i} float ${j}: ${actual} != ${f[j]}`);
            }
        }
        p.RecordTransport(encoder, null);
    }
}
assert.equal(line,lines.length);
console.log(`Plume schedule parity: ${line} frames and all 64 uniform words compared with the exhibit`);
