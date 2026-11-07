#!/usr/bin/env node
/**
 * tools/simrun.js — headless run harness.
 *
 * Flies a generated corridor with the rival pursuit brain and prints a
 * diagnostics trace. Use it for balance checks and to prove a seed is flyable
 * without opening a browser:
 *
 *   node tools/simrun.js [seed] [seconds]
 */
import { CFG } from '../src/config.js';
import { createCourse } from '../src/world/course.js';
import { Race } from '../src/sim/race.js';
import { AiDriver } from '../src/sim/ai.js';
import { Rng } from '../src/core/mathx.js';

const DT = 1 / 120;
const seed = parseInt(process.argv[2] || String(CFG.course.seed), 10) >>> 0;
const maxSeconds = parseFloat(process.argv[3] || '300');

const cfg = structuredClone(CFG);
cfg.course.seed = seed;

const t0 = performance.now();
const course = createCourse(cfg);
const genMs = performance.now() - t0;
const race = new Race(cfg, course);
const driver = new AiDriver(
  cfg,
  { name: 'AUTO', tag: 'HARNESS', color: 0xffffff, skill: 0.995, aggression: 0, boostUse: 0.6, lineOffset: 0 },
  course, race.player, new Rng(seed + 1)
);

let impacts = 0, misses = 0, skims = 0, surges = 0;
race.events.onImpact = (v) => { if (v === race.player) impacts++; };
race.events.onMiss = () => { misses++; console.log(`   ! MISS gate ${race.nextGateIndex} t=${race.elapsed.toFixed(1)}s  off-plane ${(race.lastMissRadial ?? 0).toFixed(1)}m vs ring r=${(race.lastMissRadius ?? 0).toFixed(1)}m`); };
race.events.onSkim = () => { skims++; };
race.events.onSurge = () => { surges++; };

console.log(`seed ${seed} · ${course.gates.length} gates · ${(course.totalLength / 1000).toFixed(2)} km · ${course.pylons.length} structures · gen ${genMs.toFixed(0)} ms`);
console.log('  t     spd   arc      lat     alt  gate   toGate  steer brk impacts slip');

race.start();
let t = 0;
while (t < maxSeconds && race.state === 'racing') {
  const cmd = driver.computeCommand(DT, null);
  race.step(DT, cmd);
  t += DT;
  if (Math.round(t / DT) % 600 === 0) {
    const p = race.player;
    const g = course.gates[race.nextGateIndex];
    const dg = g ? p.position.distanceTo(g.center) : -1;
    console.log(
      `${t.toFixed(1).padStart(5)} ${p.speed.toFixed(0).padStart(5)} ${p.arc.toFixed(0).padStart(8)} ` +
      `${(p.lateral ?? 0).toFixed(0).padStart(7)} ${p.position.y.toFixed(0).padStart(6)} ` +
      `${String(race.nextGateIndex).padStart(5)} ${dg.toFixed(0).padStart(8)} ` +
      `${cmd.steer.toFixed(2).padStart(6)} ${String(cmd.brake).padStart(4)} ${String(impacts).padStart(8)} ${p.slipRatio.toFixed(2)}`
    );
  }
}

const claimed = course.gates.filter((g) => g.claimed).length;
race.settleResults();
console.log('---');
console.log(`finished   : ${race.player.finished} in ${race.player.finishTime.toFixed(2)}s`);
console.log(`gates      : ${claimed}/${course.gates.length}  (missed ${misses})`);
console.log(`top speed  : ${(race.player.topSpeedReached * 3.6).toFixed(0)} km/h`);
console.log(`impacts    : ${impacts}   skims: ${skims}   surges: ${surges}`);
console.log(`standings  : ${race.results().standings.map((s) => `${s.name} ${s.finished ? s.time.toFixed(1) + 's' : 'DNF'}`).join(' | ')}`);
