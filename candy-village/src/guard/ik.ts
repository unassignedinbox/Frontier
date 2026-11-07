/**
 * Analytic two-bone inverse kinematics and bone-orientation helpers.
 *
 * Convention: every limb bone's local +Y points from its joint to its child
 * joint, so "aiming" a bone means choosing a world rotation whose +Y is the
 * desired segment direction.
 */
import { Matrix4, Object3D, Quaternion, Vector3 } from 'three';

const UP = new Vector3(0, 1, 0);

/** Any unit vector perpendicular to v. */
function anyPerpendicular(v: Vector3): Vector3 {
  const axis = Math.abs(v.y) < 0.9 ? UP : new Vector3(1, 0, 0);
  return new Vector3().crossVectors(v, axis).normalize();
}

export interface TwoBoneSolution {
  /** Elbow / knee joint position. */
  elbow: Vector3;
  /** Achieved end-effector position (clamped to reach). */
  end: Vector3;
  /** True if the target was within reach without clamping. */
  reachable: boolean;
}

/**
 * Solves a planar two-bone chain from `root` to `target`. `pole` selects which
 * side the elbow/knee bends toward.
 */
export function solveTwoBone(root: Vector3, target: Vector3, l1: number, l2: number, pole: Vector3): TwoBoneSolution {
  const toTarget = new Vector3().subVectors(target, root);
  const rawD = toTarget.length();
  const minD = Math.abs(l1 - l2) + 1e-3;
  const maxD = l1 + l2 - 1e-3;
  const d = Math.min(maxD, Math.max(minD, rawD));
  const u = rawD > 1e-6 ? toTarget.multiplyScalar(1 / rawD) : new Vector3(0, -1, 0);
  const end = root.clone().addScaledVector(u, d);

  const cosA = (l1 * l1 + d * d - l2 * l2) / (2 * l1 * d);
  const a = Math.acos(Math.min(1, Math.max(-1, cosA)));
  let perp = pole.clone().addScaledVector(u, -pole.dot(u));
  if (perp.lengthSq() < 1e-8) perp = anyPerpendicular(u);
  perp.normalize();
  const elbow = root.clone()
    .addScaledVector(u, l1 * Math.cos(a))
    .addScaledVector(perp, l1 * Math.sin(a));
  return { elbow, end, reachable: rawD <= maxD };
}

/** World quaternion whose +Y is `dir`, with +Z roughly along `hint`. */
export function aimQuaternion(dir: Vector3, hint: Vector3): Quaternion {
  const y = dir.clone().normalize();
  let zHint = hint.clone().addScaledVector(y, -hint.dot(y));
  if (zHint.lengthSq() < 1e-8) zHint = anyPerpendicular(y);
  zHint.normalize();
  const x = new Vector3().crossVectors(y, zHint).normalize();
  const z = new Vector3().crossVectors(x, y);
  const m = new Matrix4().makeBasis(x, y, z);
  return new Quaternion().setFromRotationMatrix(m);
}

/**
 * Sets `bone.quaternion` so that the bone's world orientation equals
 * `worldQ`, given the parent's world orientation `parentWorldQ`.
 */
export function setLocalFromWorld(bone: Object3D, worldQ: Quaternion, parentWorldQ: Quaternion): void {
  const inv = parentWorldQ.clone().invert();
  bone.quaternion.copy(inv.multiply(worldQ));
}

/** Rotation about the Y axis. */
export function yawQuaternion(angle: number): Quaternion {
  return new Quaternion().setFromAxisAngle(UP, angle);
}
