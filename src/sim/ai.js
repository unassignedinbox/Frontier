/**
 * ai — rival drivers.
 *
 * Rivals run the *same* flight model as the player, driven by a look-ahead
 * pursuit controller that aims at a point on the gate chain rather than at the
 * corridor centreline. Because they share the physics, they drift, overshoot and
 * scrub speed in corners exactly like the player does, which is what makes the
 * race position changes read as real overtakes instead of scripted rubber-banding.
 *
 * `skill` shapes their top speed and their line noise; `aggression` shapes how
 * tightly they cut toward the player's line; `boostUse` shapes boost discipline.
 */

import * as THREE from 'three';
import { clamp, clamp01, angleDelta, damp } from '../core/mathx.js';

export class AiDriver {
  constructor(cfg, profile, course, vehicle, rng) {
    this.cfg = cfg;
    this.profile = profile;
    this.course = course;
    this.vehicle = vehicle;
    this.rng = rng;

    this.arc = 0;
    this.sampleIndex = 0;
    this.nextGate = 0;
    this.steer = 0;
    this.pitchCmd = 0;
    this.throttle = 1;
    this.brake = 0;
    this.boost = false;
    this.drift = false;

    this.noisePhase = rng.range(0, 100);
    this.lineBias = rng.range(-0.18, 0.18);
    this.reaction = 0;

    // Gate lateral offsets are constant per course — resolve them once instead
    // of re-running a nearest-sample search every frame per rival.
    this.gateLaterals = course.gates.map(
      (g) => course.nearest(g.center, 0, course.sampleCount).lateral
    );

    vehicle.name = profile.name;
    vehicle.color = profile.color;
    vehicle.speedCapMul = profile.skill;
  }

  reset() {
    this.arc = 0;
    this.sampleIndex = 0;
    this.nextGate = 0;
    this.reaction = 0;
  }

  /** Racing-line lateral offset at a given arc length, interpolated gate-to-gate. */
  _desiredLateral(arc) {
    const gates = this.course.gates;
    if (!gates.length) return 0;
    let g = 0;
    while (g < gates.length - 1 && gates[g + 1].arc < arc) g++;
    const a = gates[g];
    const b = gates[Math.min(gates.length - 1, g + 1)];
    const latA = this.gateLaterals[a.index] ?? 0;
    const latB = this.gateLaterals[b.index] ?? 0;
    const span = Math.max(1e-3, b.arc - a.arc);
    const t = clamp((arc - a.arc) / span, 0, 1);
    // Ease through the middle of the span so rivals arc between gates.
    const e = t * t * (3 - 2 * t);
    return (latA + (latB - latA) * e) * 0.86;
  }

  _desiredHeight(arc) {
    const gates = this.course.gates;
    if (!gates.length) return 0;
    let g = 0;
    while (g < gates.length - 1 && gates[g + 1].arc < arc) g++;
    const a = gates[g];
    const b = gates[Math.min(gates.length - 1, g + 1)];
    const span = Math.max(1e-3, b.arc - a.arc);
    const t = clamp((arc - a.arc) / span, 0, 1);
    return (a.center.y + (b.center.y - a.center.y) * t);
  }

  /**
   * Produce this frame's control command without touching the vehicle, so the
   * same brain can drive a rival *or* be fed into Race.step for tests/ghosts.
   */
  computeCommand(dt, player) {
    const v = this.vehicle;
    const course = this.course;
    const p = this.profile;

    // Track progress along the corridor.
    const near = course.nearest(v.position, this.sampleIndex, 70);
    this.sampleIndex = near.index;
    this.arc = near.arc;

    // Look-ahead scales with speed so they brake late but not suicidally.
    const look = 60 + v.speed * (0.85 + (1 - p.skill) * 1.4);
    const targetArc = Math.min(course.totalLength, this.arc + look);

    // Line noise: worse drivers wander and correct.
    this.noisePhase += dt * (0.9 + (1 - p.skill) * 2.6);
    const wobble =
      Math.sin(this.noisePhase * 1.7) * (1 - p.skill) * 34 +
      Math.sin(this.noisePhase * 0.41 + 1.3) * (1 - p.skill) * 22;

    const lat = this._desiredLateral(targetArc) + wobble + this.lineBias * course.cfg.course.halfWidth;
    const at = course.pointAtArc(targetArc, lat);
    const targetY = this._desiredHeight(targetArc) + Math.sin(this.noisePhase * 0.8) * (1 - p.skill) * 10;

    this._target = this._target || new THREE.Vector3();
    this._target.copy(at.pos);
    this._target.y = targetY;

    // Recovery: if the pending ring is downstream of us we overshot it. Aim at
    // an approach point upstream of the ring and fly back through it, which is
    // exactly the manoeuvre the HUD asks a human player to make.
    this.recovering = false;
    const pending = course.gates[v.gateIndex | 0];
    if (pending && !pending.claimed) {
      const sx = v.position.x - pending.center.x;
      const sy = v.position.y - pending.center.y;
      const sz = v.position.z - pending.center.z;
      const signed = sx * pending.normal.x + sy * pending.normal.y + sz * pending.normal.z;
      if (signed > 14) {
        this.recovering = true;
        this._target.copy(pending.center).addScaledVector(pending.normal, -110);
      }
    }

    // Aggressive rivals tuck onto the player's line to pressure them.
    if (player && p.aggression > 0.3) {
      const d = this._target.distanceTo(player.position);
      if (d < 120) {
        const k = p.aggression * (1 - d / 120) * 0.45;
        this._target.lerp(player.position, k);
      }
    }

    const dir = this._tmp || (this._tmp = new THREE.Vector3());
    dir.subVectors(this._target, v.position).normalize();

    const wantYaw = Math.atan2(dir.x, dir.z);
    const yawErr = angleDelta(v.yaw, wantYaw);

    // Proportional controller with a touch of rate damping.
    const kp = 1.9 + p.skill * 0.9;
    let steer = clamp(yawErr * kp, -1, 1);
    steer = damp(this.steer, steer, 9 + p.skill * 6, dt);
    this.steer = clamp(steer, -1, 1);

    const wantPitch = Math.asin(clamp(dir.y, -1, 1));
    const pitchErr = wantPitch - v.pitch;
    this.pitchCmd = clamp(damp(this.pitchCmd, clamp(pitchErr * 2.2, -1, 1), 7, dt), -1, 1);

    // Brake when the upcoming line demands more yaw than the craft can deliver.
    const yawRate = 1.15 + (1 - clamp01(v.speed / 250)) * 0.7;
    const needed = Math.abs(yawErr) * v.speed / Math.max(look * 0.55, 1);
    this.brake = (needed > yawRate * 1.25 && v.speed > 60) || (this.recovering && v.speed > 70) ? 1 : 0;

    // Drift through the tight stuff instead of braking it all away.
    this.drift = Math.abs(yawErr) > 0.42 && v.speed > 55;

    // Boost discipline: save it for straights, dump it for the finish.
    const straight = 1 - clamp01(Math.abs(yawErr) / 0.25);
    const toFinish = course.totalLength - this.arc;
    const finishDump = toFinish < 900 ? 1 : 0;
    this.boost =
      (straight > 0.55 && v.boost > 18 && this.rng.next() < p.boostUse * 0.35) ||
      (finishDump && v.boost > 8) ||
      (v.surgeActive && false);

    this.throttle = 1;

    this.cmd = {
      steer: this.steer,
      pitch: this.pitchCmd,
      throttle: this.throttle,
      brake: this.brake,
      boost: this.boost,
      drift: this.drift
    };
    return this.cmd;
  }

  update(dt, player) {
    const cmd = this.computeCommand(dt, player);
    this.vehicle.update(dt, cmd);
  }
}
