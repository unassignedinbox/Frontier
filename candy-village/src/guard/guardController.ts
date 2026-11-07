/**
 * Behaviour and animation controller for a jellybean guard.
 *
 * State machine:  idle -> walk -> windup -> strike -> recover -> idle
 *
 *  - walk:    follows an A* path (world/navgrid.ts) and stops within striking standoff
 *  - windup:   raises the lollipop mace behind and above the head, turns to face the target
 *  - strike:   the mace head travels on a quadratic Bezier arc and lands on an aim point
 *              (target centre plus Gaussian error set by the accuracy slider)
 *  - recover:  returns the mace to rest
 *
 * Limbs are driven by two-bone IK every frame. Feet use a planted-step gait so
 * they stay on the ground instead of sliding. A damped spring on the body gives
 * squash-and-stretch jelly response, and the jelly material gets a wobble
 * amplitude from the same spring.
 */
import { Quaternion, Vector3 } from 'three';
import { RIG, type GuardRig, type Limb } from './jellyGuard';
import { aimQuaternion, setLocalFromWorld, solveTwoBone, yawQuaternion } from './ik';
import { NavGrid } from '../world/navgrid';
import { RNG } from '../core/rng';
import { angleDiff, easeInOut, lerp, clamp } from '../core/math';
import type { Target } from '../world/targets';

export type GuardState = 'idle' | 'walk' | 'windup' | 'strike' | 'recover';

export interface ImpactEvent {
  guard: JellyGuard;
  target: Target;
  /** Where the mace head actually landed. */
  tip: Vector3;
  /** Where the guard aimed (target centre plus error). */
  aim: Vector3;
}

const WALK_SPEED = 2.2;
const STANDOFF = 1.8;
const WINDUP_TIME = 0.6;
const STRIKE_TIME = 0.36;
const RECOVER_TIME = 0.95;
const LEG_FORWARD = 0.75;
const LEG_SIDE = RIG.hipX * RIG.scale;
const STEP_TIME = 0.3;
const STEP_LIFT = 0.45;
const ANKLE_HEIGHT = RIG.ankleHeight * RIG.scale;

const UP = new Vector3(0, 1, 0);

interface Foot {
  limb: Limb;
  side: number;
  pos: Vector3;
  from: Vector3;
  to: Vector3;
  swinging: boolean;
  t: number;
}

export class JellyGuard {
  readonly rig: GuardRig;
  readonly id: number;
  readonly pos = new Vector3();
  heading = 0;
  state: GuardState = 'idle';
  /** Current horizontal speed (m/s). */
  speed = 0;
  /** Accumulated walked distance, drives the gait cycle. */
  distance = 0;
  /** Current mace-head position and axis (world). */
  readonly tip = new Vector3();
  readonly axis = new Vector3(0, 1, 0);
  target: Target | null = null;
  onImpact: ((e: ImpactEvent) => void) | null = null;
  onStatus: ((msg: string) => void) | null = null;

  private path: { x: number; z: number }[] = [];
  private pathIdx = 0;
  private timer = 0;
  private aim = new Vector3();
  private windupTip = new Vector3();
  private windupAxis = new Vector3();
  private strikeFrom = new Vector3();
  private strikeCtrl = new Vector3();
  private strikeTo = new Vector3();
  private strikeAxisStart = new Vector3();
  private strikeT = 0;
  private recoverFrom = new Vector3();
  private recoverAxisFrom = new Vector3();
  private pendingImpact = false;
  private sigma = 0;
  private readonly achievedWrist = new Vector3();
  private jelly = { x: 0, v: 0 };
  private feet: Foot[];
  private readonly rng: RNG;
  private readonly tmp = new Vector3();

  constructor(rig: GuardRig, id: number, rng: RNG) {
    this.rig = rig;
    this.id = id;
    this.rng = rng;
    this.feet = [
      { limb: rig.legA, side: 1, pos: new Vector3(), from: new Vector3(), to: new Vector3(), swinging: false, t: 0 },
      { limb: rig.legB, side: -1, pos: new Vector3(), from: new Vector3(), to: new Vector3(), swinging: false, t: 0 },
    ];
  }

  /** Teleport to a ground position, facing `heading`. */
  placeAt(x: number, z: number, heading: number, heightAt: (x: number, z: number) => number): void {
    this.pos.set(x, heightAt(x, z), z);
    this.heading = heading;
    const right = this.right();
    for (const f of this.feet) {
      f.pos.set(x + right.x * LEG_SIDE * f.side, heightAt(x, z), z + right.z * LEG_SIDE * f.side);
      f.to.copy(f.pos);
      f.from.copy(f.pos);
    }
    this.state = 'idle';
  }

  /** Begin walking to a target. Returns false when no route exists. */
  order(target: Target, nav: NavGrid): boolean {
    if (this.state !== 'idle') return false;
    const path = nav.findPath(this.pos, { x: target.x, z: target.z });
    if (!path) return false;
    this.path = path;
    this.pathIdx = 0;
    this.target = target;
    this.state = 'walk';
    this.onStatus?.(`${this.rig.name} is on the way`);
    return true;
  }

  get busy(): boolean {
    return this.state !== 'idle';
  }

  forward(): Vector3 {
    return new Vector3(Math.sin(this.heading), 0, Math.cos(this.heading));
  }

  right(): Vector3 {
    return new Vector3(Math.cos(this.heading), 0, -Math.sin(this.heading));
  }

  /** Advance the guard by dt seconds. `time` is the global clock. */
  update(dt: number, time: number, heightAt: (x: number, z: number) => number): void {
    const fwd = this.forward();
    const right = this.right();

    // 1. Behaviour state machine (movement and phase timers).
    let speedGoal = 0;
    switch (this.state) {
      case 'walk': {
        const target = this.target!;
        const dCentre = Math.hypot(target.x - this.pos.x, target.z - this.pos.z);
        if (dCentre <= STANDOFF) {
          this.beginWindup();
          break;
        }
        const wp = this.path[this.pathIdx];
        if (!wp) {
          this.beginWindup(); // reached the end of the route
          break;
        }
        const dx = wp.x - this.pos.x;
        const dz = wp.z - this.pos.z;
        const d = Math.hypot(dx, dz);
        if (d < 0.4) {
          this.pathIdx++;
          break;
        }
        const step = Math.min(d, WALK_SPEED * dt);
        this.pos.x += (dx / d) * step;
        this.pos.z += (dz / d) * step;
        this.distance += step;
        this.heading += angleDiff(this.heading, Math.atan2(dx, dz)) * Math.min(1, dt * 5);
        speedGoal = WALK_SPEED;
        break;
      }
      case 'windup': {
        this.timer += dt;
        this.faceTarget(dt);
        if (this.timer >= WINDUP_TIME) this.beginStrike();
        break;
      }
      case 'strike': {
        this.faceTarget(dt);
        this.strikeT = Math.min(1, this.strikeT + dt / STRIKE_TIME);
        if (this.strikeT >= 1) this.pendingImpact = true;
        break;
      }
      case 'recover': {
        this.timer += dt;
        if (this.timer >= RECOVER_TIME) {
          this.state = 'idle';
          this.target = null;
        }
        break;
      }
      case 'idle':
        break;
    }
    this.speed += (speedGoal - this.speed) * (1 - Math.exp(-6 * dt));
    const walkFrac = clamp(this.speed / WALK_SPEED, 0, 1);

    // 2. Root and body transforms.
    const groundY = heightAt(this.pos.x, this.pos.z);
    this.rig.root.position.set(this.pos.x, groundY, this.pos.z);
    this.rig.root.rotation.set(0, this.heading, 0);

    // Jelly spring (squash & stretch).
    const k = 90;
    const c = 7;
    this.jelly.v += (-k * this.jelly.x - c * this.jelly.v) * dt;
    this.jelly.x += this.jelly.v * dt;
    const breath = 1 + 0.012 * Math.sin(time * 2.4);
    this.rig.body.scale.set(1 + 0.1 * this.jelly.x, (1 - 0.14 * this.jelly.x) * breath, 1 + 0.1 * this.jelly.x);

    const stepBob = 0.05 * Math.abs(Math.sin((this.distance * Math.PI) / LEG_FORWARD)) * walkFrac;
    this.rig.hips.position.y = RIG.hipHeight + stepBob + 0.02 * Math.sin(time * 2.4) * (1 - walkFrac);

    // Chest lean: wind back, then drive forward into the strike.
    let lean = 0;
    if (this.state === 'windup') lean = -0.12 * easeInOut(clamp(this.timer / WINDUP_TIME, 0, 1));
    if (this.state === 'strike') lean = lerp(-0.12, 0.35, easeInOut(this.strikeT));
    if (this.state === 'recover') lean = lerp(0.2, 0, easeInOut(clamp(this.timer / RECOVER_TIME, 0, 1)));
    this.rig.chest.quaternion.setFromAxisAngle(new Vector3(1, 0, 0), lean);

    // Head looks where the guard is heading (or at the target while striking).
    let yawWanted = this.heading;
    if (this.target && this.state !== 'idle' && this.state !== 'walk') {
      yawWanted = Math.atan2(this.target.x - this.pos.x, this.target.z - this.pos.z);
    }
    const yawRel = clamp(angleDiff(this.heading, yawWanted), -0.9, 0.9);
    this.rig.head.quaternion.copy(yawQuaternion(yawRel));

    this.rig.root.updateMatrixWorld(true);

    // 3. Legs: planted-step gait, then IK.
    this.updateFeet(dt, fwd, right, heightAt, walkFrac);
    for (const f of this.feet) this.solveLeg(f);

    // 4. Arms: mace arm follows the strike phase, free arm swings with the gait.
    this.rig.root.updateMatrixWorld(true);
    this.updateTipTargets(fwd, right, time);
    this.solveArm(this.rig.armClub, this.tip, this.axis, RIG.club * RIG.scale, fwd, right, 1, true);
    this.solveArm(this.rig.armFree, null, null, 0, fwd, right, -1, false, walkFrac);

    // 5. Impact, after the solve so the reported tip is the real achieved tip.
    if (this.pendingImpact) {
      this.pendingImpact = false;
      this.resolveImpact();
    }

    // 6. Jelly wobble, per guard material.
    this.rig.jelly.setWobble(Math.min(0.12, 0.012 + 0.035 * walkFrac + 0.02 * Math.abs(this.jelly.v)));
  }

  /* ----------------------------- phase helpers --------------------------- */

  private beginWindup(): void {
    this.state = 'windup';
    this.timer = 0;
    const target = this.target!;
    const sigma = this.sigma;
    this.aim.set(target.x + this.rng.gauss(sigma), target.groundY + 0.35, target.z + this.rng.gauss(sigma));
    this.onStatus?.(`${this.rig.name} winds up`);
  }

  /** Sets the aim error for the next strike (metres, one standard deviation). */
  setAccuracyError(sigma: number): void {
    this.sigma = Math.max(0, sigma);
  }

  private beginStrike(): void {
    this.state = 'strike';
    this.strikeT = 0;
    this.strikeFrom.copy(this.windupTip);
    this.strikeTo.copy(this.aim);
    // Control point high above the midpoint gives an overhead arc.
    this.strikeCtrl.addVectors(this.strikeFrom, this.strikeTo).multiplyScalar(0.5);
    this.strikeCtrl.y += 1.6;
    this.strikeAxisStart.copy(this.windupAxis);
  }

  private resolveImpact(): void {
    const target = this.target!;
    this.state = 'recover';
    this.timer = 0;
    const tip = this.actualTip();
    this.recoverFrom.copy(tip);
    this.recoverAxisFrom.copy(this.axis);
    this.jelly.v += 6; // hard squash on landing
    this.onImpact?.({
      guard: this,
      target,
      tip,
      aim: this.aim.clone(),
    });
  }

  private faceTarget(dt: number): void {
    if (!this.target) return;
    const want = Math.atan2(this.target.x - this.pos.x, this.target.z - this.pos.z);
    this.heading += angleDiff(this.heading, want) * Math.min(1, dt * 6);
  }

  /** Computes the desired mace-head position and axis for the current state. */
  private updateTipTargets(fwd: Vector3, right: Vector3, time: number): void {
    const shoulder = this.rig.armClub.root.getWorldPosition(this.tmp.clone());
    const clubLen = RIG.club * RIG.scale;
    const idleAxis = (fwd.clone().multiplyScalar(0.55).addScaledVector(UP, 0.85)).normalize();
    const idleTip = shoulder.clone().addScaledVector(fwd, 0.9).addScaledVector(UP, -1.1).addScaledVector(idleAxis, clubLen);
    // Idle sway keeps the mace alive while standing.
    idleTip.addScaledVector(right, 0.08 * Math.sin(time * 1.7));

    switch (this.state) {
      case 'idle':
      case 'walk': {
        this.tip.copy(idleTip);
        this.axis.copy(idleAxis);
        break;
      }
      case 'windup': {
        const t = easeInOut(clamp(this.timer / WINDUP_TIME, 0, 1));
        const raised = shoulder.clone().addScaledVector(UP, 2.6).addScaledVector(fwd, -0.9);
        const raisedAxis = (fwd.clone().multiplyScalar(-0.6).addScaledVector(UP, 0.8)).normalize();
        this.tip.lerpVectors(idleTip, raised, t);
        this.axis.lerpVectors(idleAxis, raisedAxis, t).normalize();
        this.windupTip.copy(raised);
        this.windupAxis.copy(raisedAxis);
        break;
      }
      case 'strike': {
        const t = this.strikeT;
        const u = 1 - t;
        // Quadratic Bezier from windup to aim.
        this.tip.set(0, 0, 0)
          .addScaledVector(this.strikeFrom, u * u)
          .addScaledVector(this.strikeCtrl, 2 * u * t)
          .addScaledVector(this.strikeTo, t * t);
        // Tangent for the club axis.
        const tan = new Vector3()
          .addScaledVector(this.strikeCtrl.clone().sub(this.strikeFrom), 2 * u)
          .addScaledVector(this.strikeTo.clone().sub(this.strikeCtrl), 2 * t);
        tan.normalize();
        this.axis.lerpVectors(this.strikeAxisStart, tan, Math.min(1, t * 3)).normalize();
        this.tip.y = Math.max(this.tip.y, this.strikeTo.y - 0.1);
        break;
      }
      case 'recover': {
        const t = easeInOut(clamp(this.timer / RECOVER_TIME, 0, 1));
        this.tip.lerpVectors(this.recoverFrom, idleTip, t);
        this.axis.lerpVectors(this.recoverAxisFrom, idleAxis, t).normalize();
        break;
      }
    }
  }

  /** Foot placement and swing controller. */
  private updateFeet(dt: number, fwd: Vector3, right: Vector3, heightAt: (x: number, z: number) => number, walkFrac: number): void {
    const moving = walkFrac > 0.05;
    const trigger = moving ? 0.9 : 0.6;
    for (const f of this.feet) {
      const ideal = new Vector3(
        this.pos.x + right.x * LEG_SIDE * f.side + fwd.x * LEG_FORWARD * 0.5 * walkFrac,
        0,
        this.pos.z + right.z * LEG_SIDE * f.side + fwd.z * LEG_FORWARD * 0.5 * walkFrac,
      );
      ideal.y = heightAt(ideal.x, ideal.z);
      if (f.swinging) {
        f.t = Math.min(1, f.t + dt / STEP_TIME);
        const e = easeInOut(f.t);
        f.pos.lerpVectors(f.from, f.to, e);
        f.pos.y = lerp(f.from.y, f.to.y, e) + Math.sin(Math.PI * e) * STEP_LIFT * RIG.scale;
        if (f.t >= 1) {
          f.swinging = false;
          this.jelly.v += 1.0;
        }
      } else {
        f.pos.y = heightAt(f.pos.x, f.pos.z);
      }
    }
    // Only one foot swings at a time: start the one that is furthest behind.
    if (!this.feet.some((f) => f.swinging)) {
      let worst: Foot | null = null;
      let worstD = trigger;
      for (const f of this.feet) {
        const ideal = new Vector3(
          this.pos.x + right.x * LEG_SIDE * f.side + fwd.x * LEG_FORWARD * 0.5 * walkFrac,
          0,
          this.pos.z + right.z * LEG_SIDE * f.side + fwd.z * LEG_FORWARD * 0.5 * walkFrac,
        );
        const d = Math.hypot(f.pos.x - ideal.x, f.pos.z - ideal.z);
        if (d > worstD) {
          worstD = d;
          worst = f;
        }
      }
      if (worst) {
        const ideal = new Vector3(
          this.pos.x + right.x * LEG_SIDE * worst.side + fwd.x * LEG_FORWARD * 0.5 * walkFrac,
          0,
          this.pos.z + right.z * LEG_SIDE * worst.side + fwd.z * LEG_FORWARD * 0.5 * walkFrac,
        );
        ideal.y = heightAt(ideal.x, ideal.z);
        worst.from.copy(worst.pos);
        worst.to.copy(ideal);
        worst.swinging = true;
        worst.t = 0;
      }
    }
  }

  /** Two-bone IK for one leg. The foot position is the ankle target. */
  private solveLeg(f: Foot): void {
    const limb = f.limb;
    const l1 = RIG.thigh * RIG.scale;
    const l2 = RIG.shin * RIG.scale;
    const hip = limb.root.getWorldPosition(new Vector3());
    const ankleTarget = f.pos.clone().addScaledVector(UP, ANKLE_HEIGHT);
    const fwd = this.forward();
    const sol = solveTwoBone(hip, ankleTarget, l1, l2, fwd);
    const hipsQ = this.rig.hips.getWorldQuaternion(new Quaternion());
    const thighQ = aimQuaternion(sol.elbow.clone().sub(hip), fwd);
    const kneeQ = aimQuaternion(sol.end.clone().sub(sol.elbow), fwd);
    setLocalFromWorld(limb.root, thighQ, hipsQ);
    setLocalFromWorld(limb.mid, kneeQ, thighQ);
  }

  /** Two-bone IK for an arm; sets the bones so the hand meets `tip` with the club on `axis`. */
  private solveArm(
    limb: Limb,
    tip: Vector3 | null,
    axis: Vector3 | null,
    clubLen: number,
    fwd: Vector3,
    right: Vector3,
    side: number,
    isClub: boolean,
    walkFrac = 0,
  ): void {
    const l1 = RIG.upperArm * RIG.scale;
    const l2 = RIG.foreArm * RIG.scale;
    const chestQ = this.rig.chest.getWorldQuaternion(new Quaternion());
    const shoulder = limb.root.getWorldPosition(new Vector3());
    let wrist: Vector3;
    let handAxis: Vector3;
    let pole: Vector3;
    if (isClub && tip && axis) {
      handAxis = axis.clone();
      wrist = tip.clone().addScaledVector(axis, -clubLen);
      pole = right.clone().multiplyScalar(0.6).addScaledVector(UP, 0.2).addScaledVector(fwd, -0.3);
    } else {
      const swing = Math.sin((this.distance * Math.PI) / LEG_FORWARD) * walkFrac * 0.35 * side;
      wrist = shoulder.clone().addScaledVector(fwd, 0.2 + swing).addScaledVector(UP, -1.25);
      handAxis = wrist.clone().sub(shoulder).normalize();
      pole = fwd.clone().multiplyScalar(-0.8).addScaledVector(right, side * 0.3);
    }
    const sol = solveTwoBone(shoulder, wrist, l1, l2, pole);
    const shoulderQ = aimQuaternion(sol.elbow.clone().sub(shoulder), fwd);
    const elbowQ = aimQuaternion(sol.end.clone().sub(sol.elbow), pole);
    const handQ = aimQuaternion(handAxis, fwd);
    setLocalFromWorld(limb.root, shoulderQ, chestQ);
    setLocalFromWorld(limb.mid, elbowQ, shoulderQ);
    setLocalFromWorld(limb.end, handQ, elbowQ);

    if (isClub) {
      // Store the achieved wrist for impact reporting.
      this.achievedWrist.copy(sol.end);
    }
  }

  /** Impact tip = achieved wrist plus club length along the club axis. */
  private actualTip(): Vector3 {
    return this.achievedWrist.clone().addScaledVector(this.axis, RIG.club * RIG.scale);
  }
}

