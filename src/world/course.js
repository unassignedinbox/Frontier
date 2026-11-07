/**
 * course — procedural "flow corridor" generation.
 *
 * This is the deliberate answer to "no track, no lanes, not a runner":
 *
 *  - There is no closed circuit. Every race is a single point-to-point RUN from
 *    a start line to a finish line through generated space.
 *  - There is no road surface and no lane snapping. The corridor is an open
 *    canyon ~230 m wide; the only structure is the gate chain, which weaves
 *    laterally AND vertically, so a correct line is a 3-D curve the player has
 *    to fly, not a lane they fall into.
 *  - Everything derives from a seed, so a run is reproducible and shareable.
 *
 * Pure ES module (only `three` for its math types) so it runs headless in tests.
 */

import * as THREE from 'three';
import { Noise, Rng, clamp, smoothstep } from '../core/mathx.js';

export const SAMPLE_STEP = 6; // metres between dense polyline samples
export const GATE_DECK_CLEARANCE = 16; // min gap between a ring's rim and the deck

export class Course {
  constructor(cfg) {
    this.cfg = cfg;
    const c = cfg.course;
    this.rng = new Rng(c.seed);
    this.noise = new Noise(c.seed + c.scenerySeedOffset);
    this.wanderNoise = new Noise(c.seed ^ 0x9e3779b9);

    this._buildNodes();
    this._buildCurve();
    // Must come before anything that samples terrainHeight() — gates, scenery
    // and the terrain mesh all sit on top of the centreline index.
    this._buildCenterlineIndex();
    this._buildGates();
    this._buildScenery();
    this._buildSectors();
  }

  // -------------------------------------------------------------------------
  _buildNodes() {
    const c = this.cfg.course;
    const n = this.wanderNoise;
    this.nodes = [];
    for (let i = 0; i < c.nodes; i++) {
      const t = i;
      const z = i * c.nodeSpacing;
      // Two octaves of sweep: a long lazy S plus a tighter wiggle for rhythm.
      const x =
        n.fbm1(t * 0.028, 4) * 205 * c.wander +
        n.fbm1(t * 0.093 + 41, 2) * 38 * c.wander;
      const y =
        70 +
        n.fbm1(t * 0.019 + 137, 3) * 118 * c.climb +
        n.fbm1(t * 0.071 + 311, 2) * 14 * c.climb;
      this.nodes.push(new THREE.Vector3(x, y, z));
    }
  }

  _buildCurve() {
    this.curve = new THREE.CatmullRomCurve3(this.nodes, false, 'centripetal', 0.5);
    this.totalLength = this.curve.getLength();

    const count = Math.max(8, Math.ceil(this.totalLength / SAMPLE_STEP));
    this.samples = new Array(count);
    this.tangents = new Array(count);
    this.rights = new Array(count);
    this.arclen = new Float64Array(count);

    const up = new THREE.Vector3(0, 1, 0);
    let acc = 0;
    let prev = null;
    for (let i = 0; i < count; i++) {
      const u = i / (count - 1);
      const p = this.curve.getPointAt(u);
      const tan = this.curve.getTangentAt(u).normalize();
      const right = new THREE.Vector3().crossVectors(tan, up).normalize();
      if (right.lengthSq() < 1e-6) right.set(1, 0, 0);
      if (prev) acc += p.distanceTo(prev);
      prev = p;
      this.samples[i] = p;
      this.tangents[i] = tan;
      this.rights[i] = right;
      this.arclen[i] = acc;
    }
    this.sampleCount = count;

    this.startPos = this.samples[0].clone();
    this.startTangent = this.tangents[0].clone();
    this.finishPos = this.samples[count - 1].clone();
  }

  /** Uniform-in-z lookup of the centreline, used by the terrain height field. */
  _buildCenterlineIndex() {
    const z0 = this.samples[0].z;
    const z1 = this.samples[this.sampleCount - 1].z;
    this.czStep = 12;
    this.cz0 = z0;
    this.czCount = Math.max(2, Math.ceil((z1 - z0) / this.czStep) + 2);
    this.czX = new Float32Array(this.czCount);
    this.czY = new Float32Array(this.czCount);
    let si = 0;
    for (let i = 0; i < this.czCount; i++) {
      const z = z0 + i * this.czStep;
      while (si < this.sampleCount - 2 && this.samples[si + 1].z < z) si++;
      const a = this.samples[si], b = this.samples[Math.min(this.sampleCount - 1, si + 1)];
      const span = Math.max(1e-3, b.z - a.z);
      const t = clamp((z - a.z) / span, 0, 1);
      this.czX[i] = a.x + (b.x - a.x) * t;
      this.czY[i] = a.y + (b.y - a.y) * t;
    }
  }

  /** Centreline X/Y at a world Z (clamped to the corridor extents). */
  centerlineAtZ(z) {
    const raw = (z - this.cz0) / this.czStep;
    const i = Number.isFinite(raw) ? clamp(raw, 0, this.czCount - 1) : 0;
    const i0 = Math.floor(i);
    const i1 = Math.min(this.czCount - 1, i0 + 1);
    const t = i - i0;
    return {
      x: this.czX[i0] + (this.czX[i1] - this.czX[i0]) * t,
      y: this.czY[i0] + (this.czY[i1] - this.czY[i0]) * t
    };
  }

  /**
   * Terrain height at (x, z). A canyon whose floor tracks the racing line and
   * whose walls rise away from it — structure and drama without a road.
   */
  terrainHeight(x, z) {
    const cl = this.centerlineAtZ(z);
    const hw = this.cfg.course.halfWidth;
    const d = Math.abs(x - cl.x);
    // Floor sits ~60 m under the racing line: close enough that the deck rushes
    // past for speed reference, deep enough that gates can hang below it.
    const base = cl.y - 60;
    const wall = smoothstep(hw * 0.95, hw * 2.5, d);
    const wallH = 300 * Math.pow(wall, 1.35);
    const broad = this.noise.fbm2(x * 0.0016, z * 0.0016, 4) * 52;
    const fine = this.noise.fbm2(x * 0.0075, z * 0.0075, 3) * 8;
    const ridge = smoothstep(hw * 1.5, hw * 3.4, d) * this.noise.fbm2(x * 0.0009, z * 0.0009, 3) * 190;
    return base + (broad + fine) * (0.20 + wall * 1.2) + wallH + ridge;
  }

  // -------------------------------------------------------------------------
  _buildGates() {
    const c = this.cfg.course;
    const n = this.noise;
    this.gates = [];

    const nodeToU = (i) => clamp((i * c.nodeSpacing) / this.totalLength, 0, 1);
    const up = new THREE.Vector3(0, 1, 0);

    const idxs = [];
    for (let i = 4; i < c.nodes - 2; i += c.gateEvery) idxs.push(i);

    // First pass: raw centres, so rings can face the actual approach line.
    const raw = idxs.map((i) => {
      const u = nodeToU(i);
      const p = this.curve.getPointAt(u);
      const tan = this.curve.getTangentAt(u).normalize();
      const right = new THREE.Vector3().crossVectors(tan, up).normalize();
      // Gate placement walks a slow sine so consecutive gates force real turns.
      const lat =
        n.fbm1(i * 0.21 + 7, 3) * c.gateLateral * c.halfWidth +
        n.fbm1(i * 0.61 + 3, 1) * 0.22 * c.gateLateral * c.halfWidth;
      const vert = n.fbm1(i * 0.17 + 55, 3) * c.gateVertical;
      return {
        node: i,
        u,
        center: p.clone().addScaledVector(right, lat).add(new THREE.Vector3(0, vert, 0)),
        radius: c.gateRadiusMin + (0.5 + 0.5 * n.n1(i * 0.37)) * (c.gateRadiusMax - c.gateRadiusMin)
      };
    });

    // Never let a ring spawn buried in the deck: lift it clear if the noise put
    // the canyon floor through it. Terrain is queryable because the centreline
    // index is built before gates.
    for (const r of raw) {
      const floor = this.terrainHeight(r.center.x, r.center.z);
      const minCentreY = floor + GATE_DECK_CLEARANCE + r.radius;
      if (r.center.y < minCentreY) r.center.y = minCentreY;
    }

    for (let g = 0; g < raw.length; g++) {
      const cur = raw[g];
      const nxt = raw[Math.min(raw.length - 1, g + 1)];
      const prv = raw[Math.max(0, g - 1)];
      const normal = new THREE.Vector3().subVectors(nxt.center, cur.center);
      if (normal.lengthSq() < 1e-4) normal.copy(this.curve.getTangentAt(cur.u));
      normal.normalize();
      // Blend with the local tangent so the ring is never edge-on to the flow.
      normal.lerp(this.curve.getTangentAt(cur.u).normalize(), 0.35).normalize();
      const upv = new THREE.Vector3().crossVectors(normal, new THREE.Vector3(1, 0, 0));
      if (upv.lengthSq() < 1e-6) upv.set(0, 1, 0);
      upv.normalize();
      this.gates.push({
        index: g,
        node: cur.node,
        center: cur.center,
        normal,
        up: upv,
        radius: cur.radius,
        u: cur.u,
        arc: this.arcLengthAt(cur.u),
        claimed: false,
        missed: false
      });
      void prv;
    }
  }

  arcLengthAt(u) {
    return clamp(u, 0, 1) * this.totalLength;
  }

  // -------------------------------------------------------------------------
  _buildScenery() {
    const c = this.cfg.course;
    const n = this.noise;
    const rng = new Rng(c.seed + 7717);
    const up = new THREE.Vector3(0, 1, 0);

    this.pylons = [];
    this.boostPads = [];
    this.shards = [];

    for (let i = 2; i < c.nodes - 2; i += c.pylonEvery) {
      const u = clamp((i * c.nodeSpacing) / this.totalLength, 0, 1);
      const p = this.curve.getPointAt(u);
      const tan = this.curve.getTangentAt(u).normalize();
      const right = new THREE.Vector3().crossVectors(tan, up).normalize();

      // Wall pylons: line the canyon shoulders for speed reference.
      for (const side of [-1, 1]) {
        const off = side * c.halfWidth * (0.82 + rng.range(0, 0.5));
        const pos = p.clone().addScaledVector(right, off);
        const groundY = this.terrainHeight(pos.x, pos.z);
        this.pylons.push({
          x: pos.x, z: pos.z,
          y: groundY,
          r: rng.range(3.2, 7.5),
          h: rng.range(48, 130) * (0.6 + Math.abs(off) / (c.halfWidth * 1.3)),
          kind: 'wall'
        });
      }

      // Slalom pylons: sit INSIDE the corridor. These are the skimming targets
      // that feed the flow meter — the risk/reward layer a lane runner lacks.
      if (rng.chance(0.62)) {
        const count = rng.int(1, 3);
        for (let k = 0; k < count; k++) {
          const off = rng.range(-0.72, 0.72) * c.halfWidth;
          const along = rng.range(-18, 18);
          const pos = p.clone().addScaledVector(right, off).addScaledVector(tan, along);
          const groundY = this.terrainHeight(pos.x, pos.z);
          this.pylons.push({
            x: pos.x, z: pos.z, y: groundY,
            r: rng.range(3.0, 5.6),
            h: (pos.y - groundY) + rng.range(-8, 30),
            kind: 'slalom'
          });
        }
      }

      // Spires poking up through the flight band.
      if (rng.chance(0.3)) {
        const off = rng.range(-0.55, 0.55) * c.halfWidth;
        const pos = p.clone().addScaledVector(right, off);
        const groundY = this.terrainHeight(pos.x, pos.z);
        this.pylons.push({
          x: pos.x, z: pos.z, y: groundY,
          r: rng.range(5, 11),
          h: (pos.y - groundY) * rng.range(0.7, 1.35),
          kind: 'spire'
        });
      }

      // Floating shards above the corridor (parallax + sense of altitude).
      if (rng.chance(0.5)) {
        for (let k = 0; k < rng.int(1, 3); k++) {
          const off = rng.range(-1.5, 1.5) * c.halfWidth;
          const pos = p.clone().addScaledVector(right, off);
          this.shards.push({
            x: pos.x + rng.range(-30, 30),
            y: pos.y + rng.range(60, 240),
            z: pos.z + rng.range(-40, 40),
            s: rng.range(6, 26),
            rot: rng.range(0, Math.PI * 2)
          });
        }
      }

      // Boost pads: placed just before gates so a good line is rewarded.
      if (i % (c.gateEvery * 2) === 0 && i > 8) {
        const gateIdx = Math.floor((i - 4) / c.gateEvery);
        const gate = this.gates[clamp(gateIdx, 0, this.gates.length - 1)];
        const pos = gate ? gate.center.clone().addScaledVector(tan, -70) : p.clone();
        pos.y -= 4;
        this.boostPads.push({
          center: pos,
          radius: 22,
          amount: 46,
          dir: tan.clone()
        });
      }
    }

    void n;
  }

  _buildSectors() {
    const SECTORS = 4;
    this.sectors = [];
    for (let s = 0; s < SECTORS; s++) {
      const fromGate = Math.floor((this.gates.length * s) / SECTORS);
      const toGate = Math.floor((this.gates.length * (s + 1)) / SECTORS) - 1;
      this.sectors.push({
        index: s,
        firstGate: fromGate,
        lastGate: Math.max(fromGate, toGate),
        arcFrom: this.gates[fromGate] ? this.gates[fromGate].arc : 0
      });
    }
    this.sectorCount = SECTORS;
  }

  // -------------------------------------------------------------------------
  /**
   * Nearest centreline sample to a world position, searched outward from a
   * hint index. Returns { index, arc, lateral, dist } where `arc` is the
   * distance travelled along the corridor — this is what makes race positions
   * meaningful in a course with no laps.
   */
  nearest(pos, hint = 0, window = 90) {
    const n = this.sampleCount;
    const lo = Math.max(0, hint - window);
    const hi = Math.min(n - 1, hint + window);
    let best = hint, bestD = Infinity;
    for (let i = lo; i <= hi; i++) {
      const s = this.samples[i];
      const dx = s.x - pos.x, dy = s.y - pos.y, dz = s.z - pos.z;
      const d = dx * dx + dy * dy + dz * dz;
      if (d < bestD) { bestD = d; best = i; }
    }
    // If the hint window failed (e.g. after a huge deviation), widen once.
    if (bestD > 400 * 400) {
      bestD = Infinity;
      for (let i = 0; i < n; i++) {
        const s = this.samples[i];
        const dx = s.x - pos.x, dy = s.y - pos.y, dz = s.z - pos.z;
        const d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) { bestD = d; best = i; }
      }
    }
    const s = this.samples[best];
    const right = this.rights[best];
    const dx = pos.x - s.x, dz = pos.z - s.z;
    return {
      index: best,
      arc: this.arclen[best],
      lateral: dx * right.x + dz * right.z,
      dist: Math.sqrt(bestD),
      point: s,
      tangent: this.tangents[best]
    };
  }

  /** Centreline world position + tangent at an arc length, with lateral offset. */
  pointAtArc(arc, lateral = 0) {
    const u = clamp(arc / this.totalLength, 0, 1);
    const i = clamp(Math.round(u * (this.sampleCount - 1)), 0, this.sampleCount - 1);
    const p = this.samples[i];
    if (!lateral) return { pos: p, tangent: this.tangents[i], right: this.rights[i], index: i };
    return {
      pos: p.clone().addScaledVector(this.rights[i], lateral),
      tangent: this.tangents[i],
      right: this.rights[i],
      index: i
    };
  }

  get gateCount() { return this.gates.length; }
  get finishArc() { return this.totalLength; }
}

export function createCourse(cfg) {
  return new Course(cfg);
}
