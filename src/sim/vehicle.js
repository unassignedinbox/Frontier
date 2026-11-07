/**
 * vehicle — hover-craft flight model.
 *
 * The single most important design decision in the prototype: the craft has a
 * real velocity vector that is *separate* from where it is pointing. Steering
 * rotates the heading; grip scrubs lateral velocity off over time. That gap is
 * the drift, and it is what makes this a driving sim rather than a lane runner:
 *
 *   - a runner moves you to a discrete lane and you are there;
 *   - here, flick the stick at 400 km/h and the nose comes round immediately
 *     while the mass keeps going straight for a third of a second. You have to
 *     plan the arc, bleed speed, or hold drift and slide it through.
 *
 * No dependency on the course beyond a height sampler, so it unit-tests cleanly.
 */

import * as THREE from 'three';
import { CFG } from '../config.js';
import { clamp, clamp01, damp, dampAngle, lerp } from '../core/mathx.js';

export class Vehicle {
  constructor(cfg = CFG, opts = {}) {
    this.cfg = cfg;
    this.craft = cfg.craft;
    this.name = opts.name || 'PLAYER';
    this.color = opts.color ?? 0x37e6ff;
    this.isPlayer = !!opts.isPlayer;

    this.position = new THREE.Vector3();
    this.velocity = new THREE.Vector3();
    this.heading = new THREE.Vector3(0, 0, 1);
    this.right = new THREE.Vector3(1, 0, 0);
    this.up = new THREE.Vector3(0, 1, 0);

    this.yaw = 0;
    this.pitch = 0;
    this.roll = 0;         // visual bank
    this.rollTarget = 0;

    this.speed = 0;
    this.forwardSpeed = 0;
    this.slipRatio = 0;    // 0 = tracking perfectly, 1 = fully sideways
    this.scrape = 0;       // 0..1 terrain contact intensity
    this.drifting = false;

    this.boost = cfg.boost.max;
    this.flow = 0;
    this.boosting = false;
    this.surge = 0;        // seconds of Tachyon Surge remaining
    this.surgeNorm = 0;

    this.impactFlash = 0;
    this.impactCooldown = 0; // stops contact from re-triggering a full hit each frame
    this.distanceTravelled = 0;
    this.topSpeedReached = 0;
    this.alive = true;
    this.finished = false;
    this.finishTime = 0;

    this.autoThrottle = true;
    this.speedCapMul = 1;   // rivals scale their ceiling by their `skill`
    this._terrain = opts.terrain || (() => 0);
    this._q = new THREE.Quaternion();
    this._tmp = new THREE.Vector3();
    this._tmp2 = new THREE.Vector3();
    this._fwd = new THREE.Vector3();
    this._lat = new THREE.Vector3();
  }

  reset(position, yaw = 0, pitch = 0) {
    this.position.copy(position);
    this.yaw = yaw;
    this.pitch = pitch;
    this.roll = 0;
    this.rollTarget = 0;
    this.velocity.set(0, 0, 0);
    this.speed = 0;
    this.forwardSpeed = 0;
    this.slipRatio = 0;
    this.scrape = 0;
    this.boost = this.cfg.boost.max;
    this.flow = 0;
    this.boosting = false;
    this.surge = 0;
    this.surgeNorm = 0;
    this.impactFlash = 0;
    this.impactCooldown = 0;
    this.distanceTravelled = 0;
    this.topSpeedReached = 0;
    this.alive = true;
    this.finished = false;
    this.finishTime = 0;
    this._refreshBasis();
  }

  get topSpeed() {
    return this.surge > 0 ? this.craft.topSpeedSurge : this.craft.topSpeed;
  }

  get speedNorm() {
    return clamp01(this.speed / this.craft.topSpeed);
  }

  get surgeActive() { return this.surge > 0; }

  _refreshBasis() {
    const cp = Math.cos(this.pitch);
    this.heading.set(Math.sin(this.yaw) * cp, Math.sin(this.pitch), Math.cos(this.yaw) * cp).normalize();
    this.right.set(Math.cos(this.yaw), 0, -Math.sin(this.yaw));
    this.up.copy(this._tmp.crossVectors(this.right, this.heading)).normalize();
  }

  /** Rotation for rendering: yaw -> pitch -> bank roll. */
  get orientation() {
    const e = new THREE.Euler(this.pitch, this.yaw, this.roll, 'YXZ');
    return this._q.setFromEuler(e);
  }

  addFlow(amount) {
    this.flow = clamp(this.flow + amount, 0, this.cfg.flow.max);
  }

  addBoost(amount) {
    this.boost = clamp(this.boost + amount, 0, this.cfg.boost.max);
  }

  /**
   * @param {number} dt seconds (already time-scaled by the caller)
   * @param {object} cmd { steer, pitch, throttle, brake, boost, drift }
   */
  update(dt, cmd) {
    if (!this.alive) return;
    const c = this.craft;

    // --- Tachyon Surge lifecycle ------------------------------------------
    if (this.surge > 0) {
      this.surge = Math.max(0, this.surge - dt);
      this.surgeNorm = clamp01(this.surge / this.cfg.flow.surgeDuration);
    } else if (this.surgeNorm > 0) {
      this.surgeNorm = 0;
    }
    if (this.flow >= this.cfg.flow.max && this.surge <= 0) {
      this.surge = this.cfg.flow.surgeDuration;
      this.surgeNorm = 1;
      this.flow = 0;
      this.onSurge && this.onSurge();
    }

    // --- steering ----------------------------------------------------------
    // Yaw authority falls with speed: tight at 100 km/h, committed at 460.
    const speedT = clamp01(this.speed / (c.yawRateSpeedRef * 2.2));
    let yawRate = lerp(c.yawRate, c.yawRateHighSpeed, speedT);
    const wantDrift = !!cmd.drift && this.speed > 18;
    if (wantDrift) yawRate *= 1.4;
    this.drifting = wantDrift;

    this.yaw += cmd.steer * yawRate * dt;
    if (this.yaw > Math.PI) this.yaw -= Math.PI * 2;
    else if (this.yaw < -Math.PI) this.yaw += Math.PI * 2;

    const targetPitch = clamp(cmd.pitch, -1, 1) * c.pitchLimit;
    this.pitch = dampAngle(this.pitch, targetPitch, 6.5, dt);

    this._refreshBasis();

    // --- thrust ------------------------------------------------------------
    const throttle = this.autoThrottle ? 1 : (cmd.throttle ? 1 : 0);
    this.boosting = !!cmd.boost && this.boost > 0.5 && this.surge <= 0;
    if (this.boosting) this.boost = Math.max(0, this.boost - this.cfg.boost.drain * dt);

    let accel = c.thrust * throttle;
    if (this.boosting) accel *= this.cfg.boost.thrustMul;
    if (this.surge > 0) accel *= 1.45;
    if (cmd.brake) accel = -c.brakeForce * cmd.brake;

    this.velocity.addScaledVector(this.heading, accel * dt);

    // --- drag (linear + quadratic gives a natural terminal speed) ----------
    const sp = this.velocity.length();
    const dragK = c.linearDrag + c.quadDrag * sp;
    this.velocity.multiplyScalar(Math.exp(-dragK * dt));

    // --- grip: scrub lateral velocity, keep the longitudinal component ------
    this.forwardSpeed = this.velocity.dot(this.heading);
    this._lat.copy(this.velocity).addScaledVector(this.heading, -this.forwardSpeed);
    const latMag = this._lat.length();
    const grip = (wantDrift ? c.gripDrift : c.grip) * lerp(1, c.gripFloor, this.scrape);
    if (latMag > 1e-5) {
      const keep = Math.exp(-grip * dt);
      this._lat.multiplyScalar(keep);
      this.velocity.copy(this.heading).multiplyScalar(this.forwardSpeed).add(this._lat);
    } else {
      this.velocity.copy(this.heading).multiplyScalar(this.forwardSpeed);
    }

    // --- clamp to top speed -------------------------------------------------
    this.speed = this.velocity.length();
    const cap = this.topSpeed * (this.speedCapMul || 1) * (this.boosting ? this.cfg.boost.capMul : 1);
    if (this.speed > cap) {
      this.velocity.multiplyScalar(cap / this.speed);
      this.speed = cap;
    }
    this.forwardSpeed = this.velocity.dot(this.heading);
    this.slipRatio = this.speed > 1 ? clamp01(this._lat.length() / this.speed) : 0;

    // --- bank into the turn (visual, but reads as feel) ---------------------
    const load = clamp(cmd.steer, -1, 1) * clamp01(this.speed / 45);
    this.rollTarget = -load * c.bankAmount * (wantDrift ? 1.35 : 1);
    this.roll = damp(this.roll, this.rollTarget, c.bankResponse, dt);

    // --- integrate ----------------------------------------------------------
    this._tmp2.copy(this.velocity).multiplyScalar(dt);
    this.position.add(this._tmp2);
    this.distanceTravelled += this._tmp2.length();
    this.topSpeedReached = Math.max(this.topSpeedReached, this.speed);

    // --- hover / terrain ----------------------------------------------------
    const floor = this._terrain(this.position.x, this.position.z) + c.hoverHeight;
    const penetration = floor - this.position.y;
    if (penetration > 0) {
      this.position.y = floor;
      if (this.velocity.y < 0) {
        const hit = -this.velocity.y;
        this.velocity.y = hit * 0.18;           // bounce off the deck
        this.scrape = clamp01(hit / 22);
        if (hit > 12) this.applyImpact(0.35 + this.scrape * 0.4);
      }
      // cushion spring so hugging the deck feels supported, not sticky
      this.velocity.y += Math.min(penetration * 6, 26) * dt;
    } else {
      this.scrape = damp(this.scrape, 0, 6, dt);
      // Hover cushion: gentle lift when skimming just above the floor.
      if (penetration > -c.hoverHeight * 1.5) {
        this.velocity.y += (-penetration / (c.hoverHeight * 1.5)) * 9 * dt;
      }
    }

    // --- flow decay ---------------------------------------------------------
    if (this.surge <= 0) {
      this.flow = Math.max(0, this.flow - this.cfg.flow.decay * dt);
    }
    // Sustained drift earns flow — the reward for committing to a slide.
    if (this.drifting && this.slipRatio > 0.16 && this.speed > 30) {
      this.addFlow(c.driftFlowRate * clamp01(this.slipRatio / 0.5) * dt);
    }

    this.impactFlash = Math.max(0, this.impactFlash - dt * 3.2);
    this.impactCooldown = Math.max(0, this.impactCooldown - dt);
  }

  /**
   * Solid impact: kill the velocity component going into the surface and bleed
   * a chunk of speed. `severity` 0..1 scales how much.
   */
  applyImpact(severity = 0.5, normal = null) {
    const bleed = 1 - this.craft.crashBleed * severity * 1.6;
    if (normal) {
      const into = this.velocity.dot(normal);
      if (into < 0) this.velocity.addScaledVector(normal, -into * 1.35);
    }
    this.velocity.multiplyScalar(Math.max(0.25, bleed));
    this.speed = this.velocity.length();
    this.impactFlash = 1;
    this.flow = Math.max(0, this.flow - 8 * severity);
    this.comboBroken = true;
    this.onImpact && this.onImpact(severity);
  }

  /** Nearest-miss scoring hook, called by the race when a skim is detected. */
  registerSkim(intensity) {
    this.addFlow(this.craft.nearMissFlow * intensity);
  }
}

export function yawFromDirection(dir) {
  return Math.atan2(dir.x, dir.z);
}
