/**
 * Game rules: target placement, guard orders, impact scoring and the per-frame
 * update that ties the world, guards, effects and HUD together.
 */
import { PerspectiveCamera, Raycaster, Scene, Vector2, Vector3 } from 'three';
import { ImpactFx } from '../fx/particles';
import type { ImpactEvent, JellyGuard } from '../guard/guardController';
import type { Hud, Stats } from '../ui/hud';
import { createTarget, scoreForDistance, type Target } from '../world/targets';
import type { World } from '../world/world';

const MAX_TARGETS = 6;
const DEFAULT_SIGMA = 0.45;
const MAX_TIP_HEIGHT = 1.2;
const MIN_TIP_HEIGHT = -0.5;

export class Game {
  private readonly targets: Target[] = [];
  private nextTargetId = 1;
  private active: Target | null = null;
  private sigma = DEFAULT_SIGMA;
  private readonly stats: Stats = { shots: 0, hits: 0, misses: 0, score: 0, bullseyes: 0 };
  private readonly fx: ImpactFx;
  private readonly raycaster = new Raycaster();
  private readonly ndc = new Vector2();
  private readonly tmp = new Vector3();

  constructor(
    private readonly world: World,
    scene: Scene,
    private readonly camera: PerspectiveCamera,
    private readonly hud: Hud,
  ) {
    this.fx = new ImpactFx(world.seed);
    scene.add(this.fx.group);

    for (const g of world.guards) {
      g.onImpact = (e) => this.handleImpact(e);
      g.onStatus = (msg) => this.hud.log(msg);
      g.setAccuracyError(this.sigma);
    }
    this.hud.updateStats(this.stats);
    const spot = this.defaultSpot();
    this.placeTarget(spot.x, spot.z, false);
  }

  /** Accuracy slider: standard deviation of the aim error in metres. */
  setAccuracy(sigma: number): void {
    this.sigma = sigma;
    for (const g of this.world.guards) g.setAccuracyError(sigma);
  }

  /** Converts a pointer position (client px) into a ground target. Shift adds to the existing set. */
  pick(clientX: number, clientY: number, add: boolean): void {
    this.ndc.set((clientX / window.innerWidth) * 2 - 1, -(clientY / window.innerHeight) * 2 + 1);
    this.raycaster.setFromCamera(this.ndc, this.camera);
    const hit = this.raycaster.intersectObject(this.world.terrain, false)[0];
    if (!hit) return;
    // Targets must sit on dry, walkable ground; clicks in the river snap to the bank.
    const spot = this.world.nav.nearestWalkable(hit.point.x, hit.point.z, 6);
    if (!spot) {
      this.hud.log('No dry ground there.');
      return;
    }
    if (spot.x !== hit.point.x || spot.z !== hit.point.z) this.hud.log('Target snapped to the river bank.');
    this.placeTarget(spot.x, spot.z, add);
  }

  /** Places a target. `add` keeps existing targets; otherwise they are cleared. */
  placeTarget(x: number, z: number, add: boolean): void {
    if (!add) this.clearTargets();
    const groundY = this.world.layout.heightAt(x, z);
    const t = createTarget(this.nextTargetId++, x, z, groundY);
    this.world.root.add(t.group);
    this.targets.push(t);
    while (this.targets.length > MAX_TARGETS) {
      const old = this.targets.shift()!;
      this.world.root.remove(old.group);
      old.dispose();
      if (this.active === old) this.active = null;
    }
    this.active = t;
    this.hud.log(`Target ${t.id} placed at ${x.toFixed(1)}, ${z.toFixed(1)}.`);
  }

  /** Clears all targets, keeping the scoreboard. */
  clearTargets(): void {
    for (const t of this.targets) {
      this.world.root.remove(t.group);
      t.dispose();
    }
    this.targets.length = 0;
    this.active = null;
  }

  sendNearest(): void {
    const target = this.active;
    if (!target) {
      this.hud.log('Place a target first.');
      return;
    }
    const idle = this.world.guards.filter((g) => !g.busy);
    if (idle.length === 0) {
      this.hud.log('All guards are busy.');
      return;
    }
    idle.sort((a, b) => dist2(a.pos, target) - dist2(b.pos, target));
    this.order(idle[0], target);
  }

  sendAll(): void {
    const target = this.active;
    if (!target) {
      this.hud.log('Place a target first.');
      return;
    }
    for (const g of this.world.guards) if (!g.busy) this.order(g, target);
  }

  private order(guard: JellyGuard, target: Target): void {
    guard.setAccuracyError(this.sigma);
    if (!guard.order(target, this.world.nav)) {
      this.hud.log(`${guard.rig.name} has no route to the target.`);
    }
  }

  private handleImpact(e: ImpactEvent): void {
    const { target, tip, guard } = e;
    const dx = tip.x - target.x;
    const dz = tip.z - target.z;
    const distance = Math.hypot(dx, dz);
    const tipHeight = tip.y - target.groundY;
    const inRange = tipHeight <= MAX_TIP_HEIGHT && tipHeight >= MIN_TIP_HEIGHT;
    const points = inRange ? scoreForDistance(distance, target.radius) : 0;

    this.stats.shots++;
    const landing = this.tmp.set(tip.x, target.groundY + 0.05, tip.z);
    if (points > 0) {
      this.stats.hits++;
      this.stats.score += points;
      if (points === 3) this.stats.bullseyes++;
      target.pulse();
      this.fx.burst(landing, 1.4);
      const label = points === 3 ? 'BULLSEYE' : points === 2 ? 'HIT' : 'EDGE';
      this.hud.popup(`${label} +${points}`, landing.clone().setY(target.groundY + 2.5), points === 3 ? 'gold' : 'hit');
      this.hud.log(`${guard.rig.name} ${label.toLowerCase()}s target ${target.id} for ${points} (${distance.toFixed(2)} m off).`);
    } else {
      this.stats.misses++;
      this.fx.burst(landing, 0.7);
      this.hud.popup('MISS', landing.clone().setY(target.groundY + 2.5), 'miss');
      const reason = inRange ? `${distance.toFixed(2)} m off` : `club ${tipHeight > 0 ? 'too high' : 'too low'}`;
      this.hud.log(`${guard.rig.name} misses target ${target.id} (${reason}).`);
    }
    this.hud.updateStats(this.stats);
  }

  private defaultSpot(): { x: number; z: number } {
    const nav = this.world.nav;
    let spot = { x: 0, z: 12 };
    for (let a = 0; a < 16; a++) {
      const ang = (a / 16) * Math.PI * 2;
      const w = nav.nearestWalkable(Math.cos(ang) * 13, Math.sin(ang) * 13, 6);
      if (w) {
        spot = w;
        break;
      }
    }
    return spot;
  }

  update(dt: number, time: number): void {
    this.world.update(time);
    const heightAt = (x: number, z: number) => this.world.layout.heightAt(x, z);
    for (const g of this.world.guards) g.update(dt, time, heightAt);
    for (const t of this.targets) t.update(dt, time);
    this.fx.update(dt, heightAt);
    this.hud.updatePopups(dt, this.camera);
  }
}

function dist2(a: Vector3, b: { x: number; z: number }): number {
  const dx = a.x - b.x;
  const dz = a.z - b.z;
  return dx * dx + dz * dz;
}
