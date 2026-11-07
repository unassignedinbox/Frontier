/**
 * Impact effects: a pooled instanced set of sprinkles and cocoa-dust puffs,
 * plus an expanding shockwave ring. No GPU objects are created per impact
 * (rings reuse one geometry and one material).
 */
import {
  BoxGeometry,
  Color,
  DynamicDrawUsage,
  Euler,
  Group,
  InstancedMesh,
  Matrix4,
  Mesh,
  MeshBasicMaterial,
  MeshPhysicalMaterial,
  Quaternion,
  RingGeometry,
  Vector3,
} from 'three';
import { RNG } from '../core/rng';

const MAX = 700;
const GRAVITY = 14;

interface Particle {
  alive: boolean;
  dust: boolean;
  pos: Vector3;
  vel: Vector3;
  life: number;
  maxLife: number;
  size: number;
  spin: Vector3;
  rot: Quaternion;
  colour: number;
}

interface Ring {
  mesh: Mesh;
  life: number;
  maxLife: number;
  radius: number;
}

const SPRINKLE_COLOURS = ['#ff4d6d', '#ffd23f', '#3dd6a4', '#7b61ff', '#ffffff', '#4cc9f0'].map((c) => new Color(c));
const DUST_COLOUR = new Color('#6b3a1f');

export class ImpactFx {
  readonly group = new Group();
  private readonly sprinkles: InstancedMesh;
  private readonly particles: Particle[] = [];
  private readonly matrix = new Matrix4();
  private readonly scale = new Vector3();
  private readonly deltaQ = new Quaternion();
  private readonly euler = new Euler();
  private readonly rng: RNG;
  private readonly rings: Ring[] = [];
  private readonly ringGeo = new RingGeometry(0.85, 1, 48);
  private readonly ringMat = new MeshBasicMaterial({ color: 0xffd6ec, transparent: true, opacity: 0.9, depthWrite: false });

  constructor(seed: number) {
    this.rng = new RNG(seed);
    this.ringGeo.rotateX(-Math.PI / 2);

    const geo = new BoxGeometry(0.16, 0.04, 0.04);
    const mat = new MeshPhysicalMaterial({ roughness: 0.25, clearcoat: 1, metalness: 0 });
    this.sprinkles = new InstancedMesh(geo, mat, MAX);
    this.sprinkles.instanceMatrix.setUsage(DynamicDrawUsage);
    this.sprinkles.castShadow = false;
    this.sprinkles.receiveShadow = false;
    this.sprinkles.frustumCulled = false;
    this.group.add(this.sprinkles);

    const hidden = new Matrix4().makeScale(0, 0, 0);
    for (let i = 0; i < MAX; i++) {
      this.particles.push({
        alive: false,
        dust: false,
        pos: new Vector3(),
        vel: new Vector3(),
        life: 0,
        maxLife: 1,
        size: 1,
        spin: new Vector3(),
        rot: new Quaternion(),
        colour: i % SPRINKLE_COLOURS.length,
      });
      this.sprinkles.setMatrixAt(i, hidden);
      this.sprinkles.setColorAt(i, SPRINKLE_COLOURS[i % SPRINKLE_COLOURS.length]);
    }
    if (this.sprinkles.instanceColor) this.sprinkles.instanceColor.needsUpdate = true;
  }

  private free(): Particle | null {
    for (const p of this.particles) if (!p.alive) return p;
    return null;
  }

  /** Burst of sprinkles plus a cloud of cocoa dust at `origin`. */
  burst(origin: Vector3, strength = 1): void {
    const sprinkleCount = Math.round(60 * strength);
    for (let i = 0; i < sprinkleCount; i++) {
      const p = this.free();
      if (!p) break;
      p.alive = true;
      p.dust = false;
      p.pos.copy(origin);
      const a = this.rng.range(0, Math.PI * 2);
      const up = this.rng.range(2, 7) * strength;
      const out = this.rng.range(1.5, 5) * strength;
      p.vel.set(Math.cos(a) * out, up, Math.sin(a) * out);
      p.life = 0;
      p.maxLife = this.rng.range(0.9, 1.6);
      p.size = this.rng.range(0.8, 1.3);
      p.spin.set(this.rng.range(-12, 12), this.rng.range(-12, 12), this.rng.range(-12, 12));
      p.rot.identity();
      p.colour = this.rng.int(0, SPRINKLE_COLOURS.length - 1);
      this.setColour(p);
    }

    const puffs = Math.round(10 * strength);
    for (let i = 0; i < puffs; i++) {
      const p = this.free();
      if (!p) break;
      p.alive = true;
      p.dust = true;
      p.pos.copy(origin).add(new Vector3(this.rng.gauss(0.3), 0.2, this.rng.gauss(0.3)));
      p.vel.set(this.rng.gauss(0.6), this.rng.range(0.3, 1.2), this.rng.gauss(0.6));
      p.life = 0;
      p.maxLife = this.rng.range(1.2, 2.2);
      p.size = this.rng.range(0.5, 0.9) * strength;
      p.spin.set(0, 0, 0);
      p.colour = -1;
      this.setColour(p);
    }

    this.spawnRing(origin, 2.4 * strength);
  }

  private setColour(p: Particle): void {
    const idx = this.particles.indexOf(p);
    const c = p.colour < 0 ? DUST_COLOUR : SPRINKLE_COLOURS[p.colour];
    this.sprinkles.setColorAt(idx, c);
    if (this.sprinkles.instanceColor) this.sprinkles.instanceColor.needsUpdate = true;
  }

  private spawnRing(origin: Vector3, radius: number): void {
    const mesh = new Mesh(this.ringGeo, this.ringMat.clone());
    mesh.position.set(origin.x, origin.y + 0.05, origin.z);
    mesh.scale.setScalar(0.2);
    this.group.add(mesh);
    this.rings.push({ mesh, life: 0, maxLife: 0.7, radius });
  }

  update(dt: number, groundAt: (x: number, z: number) => number): void {
    const hidden = new Matrix4().makeScale(0, 0, 0);
    for (let i = 0; i < this.particles.length; i++) {
      const p = this.particles[i];
      if (!p.alive) continue;
      p.life += dt;
      if (p.life >= p.maxLife) {
        p.alive = false;
        this.sprinkles.setMatrixAt(i, hidden);
        continue;
      }
      const t = p.life / p.maxLife;
      if (p.dust) {
        p.vel.multiplyScalar(Math.exp(-2.2 * dt));
        p.pos.addScaledVector(p.vel, dt);
        const s = p.size * (0.4 + 1.6 * Math.sqrt(t)) * (1 - t * 0.5);
        this.matrix.makeScale(s, s, s).setPosition(p.pos);
      } else {
        p.vel.y -= GRAVITY * dt;
        p.pos.addScaledVector(p.vel, dt);
        const g = groundAt(p.pos.x, p.pos.z);
        if (p.pos.y < g + 0.02) {
          p.pos.y = g + 0.02;
          p.vel.y *= -0.35;
          p.vel.x *= 0.7;
          p.vel.z *= 0.7;
          p.spin.multiplyScalar(0.6);
        }
        this.euler.set(p.spin.x * dt, p.spin.y * dt, p.spin.z * dt);
        this.deltaQ.setFromEuler(this.euler);
        p.rot.premultiply(this.deltaQ);
        const fade = t > 0.8 ? 1 - (t - 0.8) / 0.2 : 1;
        this.scale.setScalar(p.size * Math.max(0.05, fade));
        this.matrix.compose(p.pos, p.rot, this.scale);
      }
      this.sprinkles.setMatrixAt(i, this.matrix);
    }
    this.sprinkles.instanceMatrix.needsUpdate = true;

    for (let i = this.rings.length - 1; i >= 0; i--) {
      const r = this.rings[i];
      r.life += dt;
      const t = Math.min(1, r.life / r.maxLife);
      r.mesh.scale.setScalar(0.2 + (r.radius - 0.2) * (1 - Math.pow(1 - t, 3)));
      (r.mesh.material as MeshBasicMaterial).opacity = 0.9 * (1 - t);
      if (t >= 1) {
        this.group.remove(r.mesh);
        (r.mesh.material as MeshBasicMaterial).dispose();
        this.rings.splice(i, 1);
      }
    }
  }
}
