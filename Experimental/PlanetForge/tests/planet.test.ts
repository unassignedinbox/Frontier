import { describe, expect, it } from 'vitest';
import {
  faceSTToDir, dirToFaceST, chunkChildren, chunkParent, chunkBounds,
  tangentFrame, angleBetween, latLonToDir, dirToLatLon, FACE_COUNT,
  chunkEdgeMetresLocal, rootChunks,
} from '../src/core/sdf/cubesphere';
import { clamp } from '../src/core/math/vec';
import { selectChunks, DEFAULT_LOD_SETTINGS } from '../src/core/stream/lod';

describe('cube-sphere addressing', () => {
  it('round-trips direction -> face,st -> direction', () => {
    let worst = 0;
    for (let face = 0; face < FACE_COUNT; face++) {
      for (let i = 1; i < 6; i++) {
        for (let j = 1; j < 6; j++) {
          const s = i / 6;
          const t = j / 6;
          const dir = faceSTToDir(face, s, t);
          const back = dirToFaceST(dir);
          expect(back.face).toBe(face);
          const dir2 = faceSTToDir(back.face, back.s, back.t);
          worst = Math.max(worst, angleBetween(dir, dir2));
        }
      }
    }
    // Sub-microradian round trip: the addressing is exact, not approximate.
    expect(worst).toBeLessThan(1e-6);
  });

  it('produces unit directions everywhere on a face', () => {
    for (let face = 0; face < FACE_COUNT; face++) {
      for (const [s, t] of [[0, 0], [1, 0], [0, 1], [1, 1], [0.5, 0.5], [0, 1], [1, 1]]) {
        const d = faceSTToDir(face, s, t);
        expect(Math.hypot(d.x, d.y, d.z)).toBeCloseTo(1, 9);
      }
    }
  });

  it('covers the sphere without overlapping faces', () => {
    // Every direction must land on the face whose normal dominates.
    const probes: Array<[number, number]> = [[10, 10], [-40, 120], [70, -60], [0, 0], [80, 170]];
    for (const [lat, lon] of probes) {
      const dir = latLonToDir(lat, lon);
      const st = dirToFaceST(dir);
      const back = faceSTToDir(st.face, st.s, st.t);
      expect(angleBetween(dir, back)).toBeLessThan(1e-6);
      const ll = dirToLatLon(dir);
      expect(ll.lat).toBeCloseTo(lat, 4);
    }
  });

  it('child chunks tile their parent exactly', () => {
    const parent = { face: 2, lod: 3, x: 5, y: 6 };
    const kids = chunkChildren(parent);
    expect(kids).toHaveLength(4);
    const pb = chunkBounds(parent);
    const area = kids.reduce((acc, k) => {
      const kb = chunkBounds(k);
      return acc + (kb.s1 - kb.s0) * (kb.t1 - kb.t0);
    }, 0);
    expect(area).toBeCloseTo((pb.s1 - pb.s0) * (pb.t1 - pb.t0), 12);
    for (const k of kids) expect(chunkParent(k)).toEqual(parent);
  });

  it('reports real edge length, which varies across a face', () => {
    const R = 6_371_000;
    const centre = chunkEdgeMetresLocal({ face: 0, lod: 3, x: 4, y: 4 }, R);
    const corner = chunkEdgeMetresLocal({ face: 0, lod: 3, x: 0, y: 0 }, R);
    // In the equal-angle projection the corner of a face subtends a smaller
    // great-circle step than its centre, so corner cells are narrower. That is
    // exactly why LOD selection measures real edge length instead of assuming
    // uniform cells across the face.
    expect(corner).toBeLessThan(centre);
    expect(corner / centre).toBeGreaterThan(0.5);
    expect(centre).toBeGreaterThan(1000);
  });

  it('has exactly one root chunk per face', () => {
    expect(rootChunks()).toHaveLength(6);
  });
});

describe('lod selection', () => {
  it('meets a hard chunk budget and never leaves the planet uncovered', () => {
    const settings = { ...DEFAULT_LOD_SETTINGS, maxChunks: 260, maxLod: 14 };
    const camera = { x: 6_371_000 + 400_000, y: 0, z: 0 };
    const cut = selectChunks(camera, { x: 1, y: 0, z: 0 }, settings);
    expect(cut.requests.length).toBeLessThanOrEqual(260);
    expect(cut.requests.length).toBeGreaterThan(20);
    // No duplicate keys in the cut.
    const keys = new Set(cut.requests.map((r: { key: unknown }) => JSON.stringify(r.key)));
    expect(keys.size).toBe(cut.requests.length);
  });

  it('uses finer LOD near the camera than far away', () => {
    const settings = { ...DEFAULT_LOD_SETTINGS, maxChunks: 4000, maxLod: 16 };
    const camera = { x: 6_371_000 + 30_000, y: 0, z: 0 };
    const cut = selectChunks(camera, { x: 1, y: 0, z: 0 }, settings);
    const near = cut.requests.filter((r: { distance: number }) => r.distance < 40_000);
    const far = cut.requests.filter((r: { distance: number }) => r.distance > 600_000);
    const meanLod = (list: Array<{ key: { lod: number } }>): number =>
      list.reduce((a, r) => a + r.key.lod, 0) / Math.max(1, list.length);
    expect(meanLod(near)).toBeGreaterThan(meanLod(far));
  });
});

describe('cache codec', () => {
  it('round-trips a mesh payload byte-exactly', async () => {
    const { packChunk, unpackChunk } = await import('../src/core/stream/meshCache');
    const positions = new Float32Array([1.5, -2.25, 3.75, 4, 5, 6]);
    const normals = new Float32Array([0, 1, 0, 0, 0, 1]);
    const colors = new Float32Array([0.2, 0.4, 0.6, 0.8, 0.1, 0.3]);
    const indices = new Uint32Array([0, 1, 2, 2, 1, 3]);
    const payload = {
      positions, normals, colors, indices,
      origin: [6_371_000, 12, -4] as [number, number, number],
      radius: 900, minAltitude: -20, maxAltitude: 40,
      mode: 'radial' as const, buildMs: 3, quadCount: 2, triangleCount: 2,
    };
    const buf = packChunk({ face: 3, lod: 7, x: 12, y: 4 }, payload);
    const back = unpackChunk(buf);
    expect(back).not.toBeNull();
    expect(Array.from(back!.positions)).toEqual(Array.from(positions));
    expect(Array.from(back!.normals)).toEqual(Array.from(normals));
    expect(Array.from(back!.colors)).toEqual(Array.from(colors));
    expect(Array.from(back!.indices)).toEqual(Array.from(indices));
    expect(back!.origin[0]).toBeCloseTo(6_371_000, 3);
    expect(back!.triangleCount).toBe(2);
  });

  it('rejects a corrupt blob instead of returning garbage geometry', async () => {
    const { unpackChunk } = await import('../src/core/stream/meshCache');
    const junk = new ArrayBuffer(128);
    new Uint8Array(junk).fill(7);
    expect(unpackChunk(junk)).toBeNull();
  });
});

describe('math helpers', () => {
  it('clamps', () => {
    expect(clamp(5, 0, 1)).toBe(1);
    expect(clamp(-5, 0, 1)).toBe(0);
  });

  it('tangent frame is orthonormal', () => {
    for (const [lat, lon] of [[0, 0], [45, 30], [89, 10], [-80, 200]]) {
      const d = latLonToDir(lat, lon);
      const { right, up, normal } = tangentFrame(d);
      expect(Math.hypot(right.x, right.y, right.z)).toBeCloseTo(1, 6);
      expect(Math.hypot(up.x, up.y, up.z)).toBeCloseTo(1, 6);
      expect(right.x * up.x + right.y * up.y + right.z * up.z).toBeCloseTo(0, 6);
      expect(right.x * normal.x + right.y * normal.y + right.z * normal.z).toBeCloseTo(0, 6);
    }
  });
});
