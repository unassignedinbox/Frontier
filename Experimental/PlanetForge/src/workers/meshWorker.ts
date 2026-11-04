/// <reference lib="webworker" />
/**
 * Mesh worker.
 *
 * Owns its own TerrainField instance, so field evaluation never blocks the
 * render loop. One worker per core is spun up on the main thread; each keeps the
 * current world document and throws away results from stale epochs.
 */

import { TerrainField } from '../core/world/terrainField';
import type { TerrainWorld } from '../core/world/terrainWorld';
import { buildChunkMesh } from '../core/mesh/chunkMesher';
import type {
  MeshDone, ProbeDone, WorkerError, WorkerRequest, WorkerResponse,
} from './protocol';
import { normalize } from '../core/math/vec';

let field: TerrainField | null = null;
let epoch = 0;
const cancelled = new Set<number>();

function post(msg: WorkerResponse, transfer: Transferable[] = []): void {
  (self as unknown as DedicatedWorkerGlobalScope).postMessage(msg, transfer);
}

self.onmessage = (event: MessageEvent<WorkerRequest>) => {
  const msg = event.data;
  switch (msg.type) {
    case 'world': {
      field = new TerrainField(msg.world as TerrainWorld);
      epoch = msg.epoch;
      break;
    }
    case 'cancel': {
      for (const id of msg.ids) cancelled.add(id);
      break;
    }
    case 'mesh': {
      if (!field) return;
      if (cancelled.delete(msg.id) || msg.epoch < epoch) {
        // Superseded: a newer world exists or the requester gave up.
        return;
      }
      try {
        const result = buildChunkMesh(msg.key, field.world, field, {
          cells: msg.cells,
          detail: msg.detail,
          forceVolumetric: msg.forceVolumetric,
        });
        const payload: MeshDone = {
          type: 'mesh',
          id: msg.id,
          epoch: msg.epoch,
          key: msg.key,
          payload: {
            positions: result.positions,
            normals: result.normals,
            colors: result.colors,
            indices: result.indices,
            origin: [result.origin.x, result.origin.y, result.origin.z],
            radius: result.radius,
            minAltitude: result.minAltitude,
            maxAltitude: result.maxAltitude,
            mode: result.mode,
            buildMs: result.buildMs,
            quadCount: result.quadCount,
            triangleCount: result.triangleCount,
          },
        };
        post(payload, [
          result.positions.buffer,
          result.normals.buffer,
          result.colors.buffer,
          result.indices.buffer,
        ]);
      } catch (err) {
        const e: WorkerError = {
          type: 'error', id: msg.id,
          message: err instanceof Error ? `${err.message}\n${err.stack ?? ''}` : String(err),
        };
        post(e);
      }
      break;
    }
    case 'probe': {
      if (!field) return;
      const n = msg.dirs.length / 3;
      const out = new Float32Array(n * 3);
      for (let i = 0; i < n; i++) {
        const dir = normalize({
          x: msg.dirs[i * 3],
          y: msg.dirs[i * 3 + 1],
          z: msg.dirs[i * 3 + 2],
        });
        const info = field.surfaceInfo(dir);
        out[i * 3] = info.altitude;
        out[i * 3 + 1] = info.coast;
        out[i * 3 + 2] = info.landness;
      }
      const done: ProbeDone = { type: 'probe', id: msg.id, epoch: msg.epoch, values: out };
      post(done, [out.buffer]);
      break;
    }
  }
};
