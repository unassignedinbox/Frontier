/**
 * Chunk LOD selection.
 *
 * The planet is a quadtree per cube face. Selection has three jobs, in order of
 * importance:
 *
 *  1. Cover the visible sphere exactly once. No overlap, no holes — a hole is
 *     not "less detail", it is a view of space through the ground.
 *  2. Spend a fixed budget where it is visible. Refinement is a *frontier*
 *     algorithm: repeatedly split the chunk that matters most on screen, never
 *     split anything else. Post-hoc truncation of a fine cut is what produces
 *     holes; this cannot, because the cut is valid at every step.
 *  3. Stay inside a hard budget. Detail is capped by resident chunk count, not by
 *     optimism, which is what keeps VRAM and system RAM flat during a long flight
 *     or a teleport.
 *
 * Everything here is pure geometry plus a screen-space error target. It has no
 * idea what an SDF is, which is why it ports straight to C++.
 */

import { type ChunkKey, chunkEdgeMetresLocal, chunkKeyString, FACE_COUNT, chunkChildren } from '../sdf/cubesphere';
import { type Vec3, dist, normalize, dot } from '../math/vec';

export interface LodSettings {
  /** Finest level: 2^maxLod chunks per cube-face edge. 17 ≈ 33 m cells on Earth. */
  maxLod: number;
  /** Target size of one cell projected at the view distance, metres. Lower = finer. */
  screenErrorMetres: number;
  /** Hard cap on resident chunks. */
  maxChunks: number;
  /** Chunks per side of the mesher lattice. */
  cells: number;
  /** Extra horizon cull beyond the geometric tangent, radians. */
  horizonCull: number;
  planetRadius: number;
  /** Relief headroom above the sphere, metres; inflates chunk bounds. */
  reliefHeadroom: number;
}

export const DEFAULT_LOD_SETTINGS: LodSettings = {
  maxLod: 17,
  screenErrorMetres: 4.2,
  maxChunks: 900,
  cells: 32,
  horizonCull: 0.02,
  planetRadius: 6_371_000,
  reliefHeadroom: 9_500,
};

export interface LodRequest {
  key: ChunkKey;
  /** Lower is more important: roughly (distance / on-screen size). */
  priority: number;
  /** Distance from the camera to the chunk's bounding sphere, metres. */
  distance: number;
  /** Real chunk edge length in metres. */
  edge: number;
}

export interface LodSelection {
  requests: LodRequest[];
  /** How many potential splits the budget refused. */
  dropped: number;
  /** Predicted triangles if the whole cut is resident. */
  predictedTriangles: number;
}

/** Min-heap on priority. Small and allocation-free enough for a per-frame cut. */
class MinHeap<T> {
  private items: T[] = [];
  constructor(private readonly key: (t: T) => number) {}

  get size(): number { return this.items.length; }

  push(item: T): void {
    this.items.push(item);
    let i = this.items.length - 1;
    while (i > 0) {
      const parent = (i - 1) >> 1;
      if (this.key(this.items[parent]) <= this.key(this.items[i])) break;
      [this.items[parent], this.items[i]] = [this.items[i], this.items[parent]];
      i = parent;
    }
  }

  pop(): T | undefined {
    const top = this.items[0];
    const last = this.items.pop();
    if (this.items.length > 0 && last !== undefined) {
      this.items[0] = last;
      let i = 0;
      for (;;) {
        const l = i * 2 + 1;
        const r = l + 1;
        let smallest = i;
        if (l < this.items.length && this.key(this.items[l]) < this.key(this.items[smallest])) smallest = l;
        if (r < this.items.length && this.key(this.items[r]) < this.key(this.items[smallest])) smallest = r;
        if (smallest === i) break;
        [this.items[smallest], this.items[i]] = [this.items[i], this.items[smallest]];
        i = smallest;
      }
    }
    return top;
  }
}

/** Centre direction of a chunk (local copy avoids a module cycle in workers). */
function faceDir(face: number, s: number, t: number): Vec3 {
  const u = s * 2 - 1;
  const v = t * 2 - 1;
  switch (face) {
    case 0: return normalize({ x: 1, y: v, z: -u });
    case 1: return normalize({ x: -1, y: v, z: u });
    case 2: return normalize({ x: u, y: 1, z: -v });
    case 3: return normalize({ x: u, y: -1, z: v });
    case 4: return normalize({ x: u, y: v, z: 1 });
    default: return normalize({ x: -u, y: v, z: -1 });
  }
}

/**
 * Finest LOD justified by a distance, from the geometric-error bound
 * `cell size ≈ distance * screenError`.
 */
export function lodForDistance(distance: number, settings: LodSettings, minLod = 0): number {
  if (distance <= 0) return settings.maxLod;
  // Cells we want at this distance:
  const wantedCell = Math.max(0.25, (distance * settings.screenErrorMetres) / 900);
  const faceArc = settings.planetRadius * (Math.PI / 2);
  const wantedEdge = wantedCell * settings.cells;
  const lod = Math.floor(Math.log2(Math.max(1, faceArc / wantedEdge)));
  return Math.max(minLod, Math.min(settings.maxLod, lod));
}

function chunkDistanceAndPriority(
  key: ChunkKey, camera: Vec3, settings: LodSettings,
): { distance: number; edge: number; priority: number; center: Vec3 } {
  const edge = chunkEdgeMetresLocal(key, settings.planetRadius);
  const n = Math.pow(2, key.lod);
  const center = faceDir(key.face, (key.x + 0.5) / n, (key.y + 0.5) / n);
  const p = {
    x: center.x * settings.planetRadius,
    y: center.y * settings.planetRadius,
    z: center.z * settings.planetRadius,
  };
  const boundRadius = edge * 0.72 + settings.reliefHeadroom * 0.4;
  const d = Math.max(0, dist(camera, p) - boundRadius);
  // Priority: subtended size over distance. Smaller = split this first.
  const priority = d / Math.max(1, edge);
  return { distance: d, edge, priority, center };
}

/**
 * Build the visible cut.
 *
 * Frontier refinement: start from the six face roots, repeatedly split the
 * highest-priority chunk while it is still too coarse for its distance and the
 * budget allows three more chunks. The result is always a valid cut of the
 * quadtree, so coverage is exact by construction.
 */
export function selectChunks(
  camera: Vec3,
  _cameraViewDir: Vec3,
  settings: LodSettings,
  previous?: Map<string, number>,
): LodSelection {
  const camDist = Math.sqrt(dot(camera, camera));
  const camDir = normalize(camera);

  // Horizon: on a sphere of radius R seen from distance d you cannot see past
  // the tangent point, so anything below that plane is pure waste.
  const tangentCos = camDist > settings.planetRadius
    ? -Math.sqrt(Math.max(0, 1 - (settings.planetRadius / camDist) ** 2))
    : -1;
  const cosLimit = Math.min(-1 + 1e-6, tangentCos - settings.horizonCull);

  const heap = new MinHeap<{ key: ChunkKey; distance: number; edge: number; priority: number }>(
    (n) => n.priority,
  );
  for (let f = 0; f < FACE_COUNT; f++) {
    const key = { face: f, lod: 0, x: 0, y: 0 };
    const m = chunkDistanceAndPriority(key, camera, settings);
    heap.push({ key, distance: m.distance, edge: m.edge, priority: m.priority });
  }

  const leaves: LodRequest[] = [];
  let dropped = 0;
  // Total chunks the cut would contain right now: leaves + frontier nodes.
  let frontierCount = FACE_COUNT;

  while (heap.size > 0) {
    const node = heap.pop();
    if (!node) break;

    // Backface / behind-the-planet cull. The whole subtree is hidden.
    const m = chunkDistanceAndPriority(node.key, camera, settings);
    if (dot(normalize(m.center), camDir) < cosLimit) { frontierCount--; continue; }

    const want = lodForDistance(node.distance, settings);
    const residency = previous?.get(chunkKeyString(node.key));
    // Hysteresis: do not split one level deeper than a chunk that is already
    // resident, which is what stops LOD churn while flying.
    const target = residency !== undefined
      ? Math.min(want, Math.max(node.key.lod, residency))
      : want;

    const canSplit = node.key.lod < target && node.key.lod < settings.maxLod;
    const keepBudget = leaves.length + (heap.size - 1) + 3 <= settings.maxChunks;

    if (canSplit && keepBudget) {
      for (const child of chunkChildren(node.key)) {
        const cm = chunkDistanceAndPriority(child, camera, settings);
        heap.push({ key: child, distance: cm.distance, edge: cm.edge, priority: cm.priority });
      }
      frontierCount += 3;
    } else {
      if (canSplit) dropped++;
      leaves.push({
        key: node.key,
        distance: node.distance,
        edge: node.edge,
        priority: node.priority,
      });
      frontierCount--;
    }
  }

  // Nearest/most-visible first, so the streamer fills the screen centre before
  // the corners of the view.
  leaves.sort((a, b) => a.priority - b.priority);

  return {
    requests: leaves,
    dropped,
    predictedTriangles: leaves.length * (settings.cells + 1) * (settings.cells + 1) * 2,
  };
}

export { chunkDistanceAndPriority };
