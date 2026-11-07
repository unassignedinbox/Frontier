/**
 * Strike targets: flat gingerbread bullseyes with a glowing marker beam.
 * Scoring is ring-based: inner third 3 pts, middle third 2 pts, outer ring 1 pt.
 */
import { CircleGeometry, Group, Mesh, TorusGeometry } from 'three';
import { MAT } from '../materials/library';
import { cylinder, mesh } from '../geometry/builders';

export const TARGET_RADIUS = 1.0;

export interface Target {
  id: number;
  x: number;
  z: number;
  groundY: number;
  radius: number;
  group: Group;
  /** Bounce animation timer (seconds since last hit), or -1 when settled. */
  bounce: number;
  hits: number;
  update(dt: number, time: number): void;
  pulse(): void;
  dispose(): void;
}

/** Points for landing `distance` metres from the centre (0 = miss). */
export function scoreForDistance(distance: number, radius = TARGET_RADIUS): 0 | 1 | 2 | 3 {
  if (distance > radius) return 0;
  if (distance < radius / 3) return 3;
  if (distance < (2 * radius) / 3) return 2;
  return 1;
}

export function createTarget(id: number, x: number, z: number, groundY: number): Target {
  const group = new Group();
  group.name = `Target${id}`;
  group.position.set(x, groundY, z);

  const disc = new CircleGeometry(TARGET_RADIUS, 48);
  disc.rotateX(-Math.PI / 2);
  disc.translate(0, 0.05, 0);
  disc.deleteAttribute('uv');
  const discMesh = mesh(disc, MAT.bullseye(), { cast: false });
  group.add(discMesh);

  const rim = new TorusGeometry(TARGET_RADIUS, 0.09, 10, 56);
  rim.rotateX(Math.PI / 2);
  rim.translate(0, 0.07, 0);
  rim.deleteAttribute('uv');
  group.add(mesh(rim, MAT.icingPink()));

  // A thin emissive beam makes the target readable from across the village.
  group.add(mesh(cylinder(0.035, 0.035, 9, [0, 4.5, 0], 6), MAT.sugarGlassCool(), { cast: false }));

  const t: Target = {
    id,
    x,
    z,
    groundY,
    radius: TARGET_RADIUS,
    group,
    bounce: -1,
    hits: 0,
    update(dt: number, time: number) {
      if (t.bounce >= 0) {
        t.bounce += dt;
        const k = Math.exp(-3.2 * t.bounce) * Math.cos(12 * t.bounce);
        discMesh.scale.y = 1 + 0.25 * k;
        discMesh.position.y = 0.2 * Math.max(0, k);
        if (t.bounce > 2.5) {
          t.bounce = -1;
          discMesh.scale.y = 1;
          discMesh.position.y = 0;
        }
      }
      // Beam shimmer.
      group.children[group.children.length - 1].scale.y = 1 + 0.03 * Math.sin(time * 3 + id);
    },
    pulse() {
      t.bounce = 0;
      t.hits++;
    },
    dispose() {
      group.traverse((o) => {
        if (o instanceof Mesh) o.geometry.dispose();
      });
    },
  };
  return t;
}
