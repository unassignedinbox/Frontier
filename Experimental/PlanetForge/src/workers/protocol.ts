/**
 * Message contract between the main thread and mesh workers.
 *
 * Buffers are transferred, never copied: a chunk mesh is up to a few hundred
 * kilobytes and copying them across the worker boundary is what makes naive
 * implementations drop frames.
 */

import type { ChunkKey } from '../core/sdf/cubesphere';
import type { TerrainWorld } from '../core/world/terrainWorld';

export interface WorldSync {
  type: 'world';
  world: TerrainWorld;
  /** Document hash, echoed back so stale results can be discarded. */
  hash: string;
  /** Incremented on every edit; worker drops jobs from older epochs. */
  epoch: number;
}

export interface MeshJob {
  type: 'mesh';
  id: number;
  key: ChunkKey;
  epoch: number;
  cells: number;
  detail: boolean;
  forceVolumetric?: boolean;
}

export interface ProbeJob {
  type: 'probe';
  id: number;
  /** Unit directions to evaluate. */
  dirs: Float32Array;
  epoch: number;
}

export interface CancelJob {
  type: 'cancel';
  ids: number[];
}

export type WorkerRequest = WorldSync | MeshJob | ProbeJob | CancelJob;

export interface MeshPayload {
  positions: Float32Array;
  normals: Float32Array;
  colors: Float32Array;
  indices: Uint32Array;
  origin: [number, number, number];
  radius: number;
  minAltitude: number;
  maxAltitude: number;
  mode: 'radial' | 'volumetric';
  buildMs: number;
  quadCount: number;
  triangleCount: number;
}

export interface MeshDone {
  type: 'mesh';
  id: number;
  epoch: number;
  key: ChunkKey;
  payload: MeshPayload;
}

export interface ProbeDone {
  type: 'probe';
  id: number;
  epoch: number;
  /** altitude, coast distance, landness per direction. */
  values: Float32Array;
}

export interface WorkerError {
  type: 'error';
  id: number;
  message: string;
}

export type WorkerResponse = MeshDone | ProbeDone | WorkerError;
