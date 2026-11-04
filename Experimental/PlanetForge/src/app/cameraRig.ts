/**
 * Fly camera in planet-space doubles.
 *
 * The camera position lives in JS doubles, planet-centred, in metres, and is
 * never handed to Three.js directly: every renderer translates its geometry by
 * -camera, so the GPU only ever sees local offsets. That one rule is what makes
 * a continuous flight from the ground to orbit possible without a floating
 * origin rebase and without float32 jitter.
 */

import * as THREE from 'three';
import {
  type Vec3, add, cross, mul, normalize, sub, v3, clamp, dot, rotateAround, DEG, dist,
} from '../core/math/vec';

export interface PickResult {
  dir: Vec3;
  point: Vec3;
  altitude: number;
  distance: number;
}

export class CameraRig {
  /** Planet-centred position, metres. */
  position: Vec3 = v3(0, 0, 0);
  /** Unit forward direction. */
  forward: Vec3 = normalize(v3(0, 0, -1));
  /** World up reference (usually the planet normal under the camera). */
  up: Vec3 = v3(0, 1, 0);

  fov = 64;
  near = 0.35;
  far = 6.0e7;

  speed = 90;
  boost = 14;
  slow = 0.12;

  readonly camera: THREE.PerspectiveCamera;
  private surfaceRadiusAt: (dir: Vec3) => number;

  constructor(surfaceRadiusAt: (dir: Vec3) => number) {
    this.surfaceRadiusAt = surfaceRadiusAt;
    this.camera = new THREE.PerspectiveCamera(this.fov, 1, this.near, this.far);
  }

  /* ------------------------------------------------------------- placement */

  placeAtDirection(dir: Vec3, radius: number): void {
    const n = normalize(dir);
    this.position = mul(n, radius);
    this.up = n;
    const east = normalize(cross(Math.abs(n.y) > 0.99 ? v3(0, 0, 1) : v3(0, 1, 0), n));
    // Look slightly down toward the horizon, like an editor's default view.
    this.forward = normalize(add(mul(east, 0.92), mul(n, -0.18)));
  }

  /** Body radius relative to sea level is `surfaceRadiusAt(n)`. */
  get radius(): number {
    return Math.sqrt(dot(this.position, this.position));
  }

  get surfaceRadius(): number {
    return this.surfaceRadiusAt(normalize(this.position));
  }

  get altitude(): number {
    return this.radius - this.surfaceRadius;
  }

  /* ----------------------------------------------------------------- input */

  /** Rotate the view about the local planet normal. */
  yaw(radians: number): void {
    const n = normalize(this.position);
    this.forward = normalize(rotateAround(this.forward, n, radians));
  }

  /** Rotate the view about the camera's right vector. */
  pitch(radians: number): void {
    const right = normalize(cross(this.up, this.forward));
    this.forward = normalize(rotateAround(this.forward, right, radians));
    this.up = normalize(cross(this.forward, right));
    this.up = mul(this.up, -1);
    // Keep the up vector roughly planet-aligned so the horizon stays level.
    const n = normalize(this.position);
    if (dot(this.up, n) < 0.15) this.up = n;
  }

  /** Move by a camera-local offset, never entering the ground. */
  move(right: number, forward: number, up: number): void {
    const r = normalize(cross(this.up, this.forward));
    const u = normalize(cross(this.forward, r));
    this.position = add(this.position, add(add(mul(r, right), mul(this.forward, forward)), mul(u, up)));
    this.clampToGround();
  }

  /** Move along the current forward direction at `metres`. */
  fly(metres: number): void {
    this.position = add(this.position, mul(this.forward, metres));
    this.clampToGround();
  }

  private clampToGround(): void {
    const n = normalize(this.position);
    const minR = this.surfaceRadiusAt(n) + 2.5;
    if (dot(this.position, this.position) < minR * minR) this.position = mul(n, minR);
  }

  /* ---------------------------------------------------------------- output */

  /** Orient the Three.js camera at the origin (all world geometry is shifted). */
  syncToThree(aspect: number): void {
    this.camera.aspect = aspect;
    this.camera.fov = this.fov;
    this.camera.near = this.near;
    this.camera.far = this.far;
    this.camera.updateProjectionMatrix();
    this.camera.position.set(0, 0, 0);

    const f = this.forward;
    const r = normalize(cross(this.up, f));
    const u = normalize(cross(f, r));
    const m = new THREE.Matrix4().set(
      r.x, u.x, -f.x, 0,
      r.y, u.y, -f.y, 0,
      r.z, u.z, -f.z, 0,
      0, 0, 0, 1,
    );
    this.camera.quaternion.setFromRotationMatrix(m);
    this.camera.updateMatrixWorld(true);
  }

  /** Normalised-device coord -> planet-space ray. */
  rayFromNDC(ndcX: number, ndcY: number): { origin: Vec3; dir: Vec3 } {
    const f = this.forward;
    const r = normalize(cross(this.up, f));
    const u = normalize(cross(f, r));
    const tanHalf = Math.tan((this.fov * DEG) / 2);
    const aspect = this.camera.aspect || 1;
    return {
      origin: this.position,
      dir: normalize(add(add(mul(f, 1), mul(r, ndcX * tanHalf * aspect)), mul(u, ndcY * tanHalf))),
    };
  }

  /**
   * March a ray until it meets the SDF surface.
   *
   * The height field is bounded (relief is at most a few kilometres), so the
   * only region where a hit is possible is between two spheres. March that band
   * with expanding steps, then bisect the first sign change — robust even when
   * the camera is 20 000 km away looking at a mountain range 50 km across.
   */
  pickTerrain(
    ndcX: number, ndcY: number,
    heightAt: (dir: Vec3) => number,
    reliefBounds = 12_000,
  ): PickResult | null {
    const ray = this.rayFromNDC(ndcX, ndcY);
    const o = ray.origin;
    const d = ray.dir;
    const r0 = this.surfaceRadius;

    const tOuter = raySphere(o, d, r0 + reliefBounds);
    const tInner = raySphere(o, d, Math.max(1, r0 - reliefBounds));
    const tStart = Math.max(0, tOuter ?? 0);
    const tEnd = tInner ?? tStart + Math.max(40_000, this.altitude * 3);

    let t = tStart;
    let step = Math.max(1.0, (tEnd - tStart) / 320);
    let prevT = t;
    let prevDiff = signedGap(add(o, mul(d, t)), r0, heightAt);

    while (t <= tEnd) {
      t += step;
      const p = add(o, mul(d, t));
      const diff = signedGap(p, r0, heightAt);
      if (diff <= 0) {
        // Bisect between prevT (outside) and t (inside).
        let lo = prevT;
        let hi = t;
        for (let k = 0; k < 28; k++) {
          const mid = (lo + hi) * 0.5;
          const md = signedGap(add(o, mul(d, mid)), r0, heightAt);
          if (md > 0) lo = mid; else hi = mid;
        }
        const tt = (lo + hi) * 0.5;
        const hit = add(o, mul(d, tt));
        const nd = normalize(hit);
        return {
          dir: nd,
          point: hit,
          altitude: Math.sqrt(dot(hit, hit)) - this.surfaceRadiusAt(nd),
          distance: dist(o, hit),
        };
      }
      prevDiff = diff;
      prevT = t;
      // Accelerate while far from any surface, decelerate when approaching.
      step *= prevDiff > 4_000 ? 1.14 : 1.03;
    }
    return null;
  }

  resize(width: number, height: number): void {
    this.camera.aspect = width / Math.max(1, height);
    this.camera.updateProjectionMatrix();
  }
}

function signedGap(p: Vec3, fallbackRadius: number, heightAt: (dir: Vec3) => number): number {
  const r = Math.sqrt(dot(p, p));
  if (r < 1) return -1;
  const nd = mul(p, 1 / r);
  return r - (fallbackRadius + heightAt(nd));
}

/** Nearest positive ray-sphere hit. */
export function raySphere(o: Vec3, d: Vec3, radius: number): number | null {
  const b = 2 * dot(o, d);
  const c = dot(o, o) - radius * radius;
  const disc = b * b - 4 * c;
  if (disc < 0) return null;
  const sq = Math.sqrt(disc);
  const t0 = (-b - sq) * 0.5;
  const t1 = (-b + sq) * 0.5;
  if (t0 > 0) return t0;
  if (t1 > 0) return t1;
  return null;
}

export { clamp, sub };
