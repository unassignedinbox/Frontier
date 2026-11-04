/**
 * Chunk streamer — the thing that keeps a planet-sized world inside a laptop.
 *
 * Responsibilities, in priority order:
 *
 *  1. NEVER grow without bound. There is a hard resident budget in chunks and in
 *     uploaded bytes. Eviction is LRU by *importance*, not by insertion order, so
 *     the far side of the planet cannot push out the terrain you are standing on.
 *
 *  2. Keep the main thread free. Field evaluation and meshing happen in a pool
 *     of workers; results arrive as transferred buffers. Nothing in this file
 *     does per-sample maths.
 *
 *  3. Work well on a spinning disk. Meshes are cached *sequentially and by size
 *     class* in IndexedDB, never with random small reads, and never with a write
 *     per chunk. See MeshCache for the layout. In practice: build once, reuse
 *     across sessions, and a teleport back to a region you have visited is a
 *     read of one contiguous blob per chunk instead of a re-mesh.
 *
 *  4. Cancel aggressively. When the camera moves, queued jobs that are no longer
 *     in the cut are dropped before they cost a millisecond.
 */

import type { ChunkKey } from '../sdf/cubesphere';
import { chunkKeyString, parseChunkKey } from '../sdf/cubesphere';
import type { TerrainWorld } from '../world/terrainWorld';
import { worldHash } from '../world/terrainWorld';
import type { MeshPayload, WorkerRequest, WorkerResponse } from '../../workers/protocol';
import type { LodRequest, LodSettings } from './lod';
import { MeshCache } from './meshCache';

export interface StreamerOptions {
  workerCount: number;
  settings: LodSettings;
  /** Max resident chunks. */
  maxResident: number;
  /** Max bytes of vertex data uploaded (positions+normals+colors+indices). */
  maxBytes: number;
  /** Detail meshing (strata + coast tint) only within this distance. */
  detailDistance: number;
  cache?: MeshCache | null;
}

export interface StreamerStats {
  resident: number;
  pending: number;
  inflight: number;
  bytes: number;
  cacheHits: number;
  cacheMisses: number;
  meshed: number;
  evicted: number;
  cancelled: number;
  avgBuildMs: number;
  triangles: number;
  lastError: string | null;
}

export interface ResidentChunk {
  key: ChunkKey;
  payload: MeshPayload;
  bytes: number;
  lastUsed: number;
  importance: number;
  fromCache: boolean;
}

const WORKER_BYTES = (p: MeshPayload): number =>
  p.positions.byteLength + p.normals.byteLength + p.colors.byteLength + p.indices.byteLength;

export class ChunkStreamer {
  private workers: Worker[] = [];
  private opts: StreamerOptions;
  private world: TerrainWorld | null = null;
  private hash = '';
  private epoch = 0;

  private resident = new Map<string, ResidentChunk>();
  private queue: LodRequest[] = [];
  private inflight = new Map<number, LodRequest>();
  private nextJobId = 1;
  private bytes = 0;
  private tick = 0;

  private stats: StreamerStats = {
    resident: 0, pending: 0, inflight: 0, bytes: 0,
    cacheHits: 0, cacheMisses: 0, meshed: 0, evicted: 0,
    cancelled: 0, avgBuildMs: 0, triangles: 0, lastError: null,
  };

  private onChunk: ((chunk: ResidentChunk) => void) | null = null;
  private onEvict: ((keyString: string) => void) | null = null;
  private buildTimes: number[] = [];

  constructor(opts: StreamerOptions) {
    this.opts = opts;
    this.spawnWorkers(opts.workerCount);
  }

  private spawnWorkers(count: number): void {
    for (let i = 0; i < count; i++) {
      const w = new Worker(new URL('../../workers/meshWorker.ts', import.meta.url), { type: 'module' });
      w.onmessage = (e: MessageEvent<WorkerResponse>) => this.handle(e.data);
      w.onerror = (e) => { this.stats.lastError = e.message || 'worker error'; };
      this.workers.push(w);
    }
  }

  setWorld(world: TerrainWorld): void {
    this.world = world;
    this.hash = worldHash(world);
    this.epoch++;
    // Change a single parameter that affects every chunk → drop everything.
    // Change a local feature → the caller passes the affected bounds instead.
    const msg: WorkerRequest = { type: 'world', world, hash: this.hash, epoch: this.epoch };
    for (const w of this.workers) w.postMessage(msg);
  }

  /** Cancel everything queued or in flight for chunks a local edit touched. */
  invalidate(predicate: (key: ChunkKey) => boolean): void {
    const ids: number[] = [];
    for (const [id, req] of this.inflight) {
      if (predicate(req.key)) { ids.push(id); this.inflight.delete(id); }
    }
    for (const req of this.queue) {
      if (predicate(req.key)) this.stats.cancelled++;
    }
    this.queue = this.queue.filter((r) => !predicate(r.key));
    if (ids.length) {
      for (const w of this.workers) {
        const msg: WorkerRequest = { type: 'cancel', ids };
        w.postMessage(msg);
      }
    }
    for (const [keyStr, chunk] of this.resident) {
      if (predicate(chunk.key)) this.evict(keyStr);
    }
  }

  onChunkReady(cb: (chunk: ResidentChunk) => void): void { this.onChunk = cb; }
  onChunkEvicted(cb: (keyString: string) => void): void { this.onEvict = cb; }

  /**
   * Feed the cut for this frame. Chunks already resident are touched; missing
   * ones are queued by priority; stale ones are evicted.
   */
  update(requests: LodRequest[], camera: { x: number; y: number; z: number }): void {
    this.tick++;
    const wanted = new Set<string>();

    for (const req of requests) {
      const ks = chunkKeyString(req.key);
      wanted.add(ks);
      const hit = this.resident.get(ks);
      if (hit) {
        hit.lastUsed = this.tick;
        hit.importance = req.priority;
        continue;
      }
      // Skip if it is already in flight.
      let pending = false;
      for (const r of this.inflight.values()) {
        if (chunkKeyString(r.key) === ks) { pending = true; break; }
      }
      if (!pending) this.queue.push(req);
    }

    // Evict anything no longer in the cut (cheap, and frees VRAM immediately).
    for (const [ks, chunk] of this.resident) {
      if (!wanted.has(ks)) this.evict(ks);
      void chunk;
    }

    // Sort by importance and cap the queue: a shorter queue means a camera move
    // costs less cancellation, and the newest cut is always what gets built.
    this.queue.sort((a, b) => a.priority - b.priority);
    const maxQueue = this.workers.length * 6;
    if (this.queue.length > maxQueue) {
      this.stats.cancelled += this.queue.length - maxQueue;
      this.queue.length = maxQueue;
    }

    this.enforceBudget();
    this.pump(camera);
    this.publishStats();
  }

  /** One in-flight job per worker slot; slots are tracked explicitly. */
  private inflightPerWorker: number[] = [];
  private workerOf = new Map<number, number>();

  private pump(_camera: { x: number; y: number; z: number }): void {
    if (!this.world) return;
    if (this.inflightPerWorker.length !== this.workers.length) {
      this.inflightPerWorker = new Array(this.workers.length).fill(0);
    }
    for (let wi = 0; wi < this.workers.length; wi++) {
      if (this.inflightPerWorker[wi] > 0) continue;
      const req = this.queue.shift();
      if (!req) return;
      this.dispatch(wi, req);
    }
  }

  private dispatch(wi: number, req: LodRequest): void {
    if (!this.world) return;
    const id = this.nextJobId++;
    this.inflight.set(id, req);
    this.workerOf.set(id, wi);
    this.inflightPerWorker[wi] = (this.inflightPerWorker[wi] ?? 0) + 1;
    const msg: WorkerRequest = {
      type: 'mesh',
      id,
      key: req.key,
      epoch: this.epoch,
      cells: this.opts.settings.cells,
      detail: req.distance < this.opts.detailDistance,
    };
    this.workers[wi].postMessage(msg);
  }

  private handle(msg: WorkerResponse): void {
    if (msg.type === 'error') {
      this.stats.lastError = msg.message;
      this.releaseSlot(msg.id);
      return;
    }
    if (msg.type !== 'mesh') return;
    if (msg.epoch < this.epoch) return;

    const req = this.inflight.get(msg.id);
    this.releaseSlot(msg.id);

    const ks = chunkKeyString(msg.key);
    if (this.resident.has(ks)) return;
    if (req) this.queue = this.queue.filter((r) => chunkKeyString(r.key) !== ks);

    const bytes = WORKER_BYTES(msg.payload);
    const chunk: ResidentChunk = {
      key: msg.key,
      payload: msg.payload,
      bytes,
      lastUsed: this.tick,
      importance: req?.priority ?? 1e9,
      fromCache: false,
    };
    this.bytes += bytes;
    this.resident.set(ks, chunk);
    this.stats.meshed++;
    this.buildTimes.push(msg.payload.buildMs);
    if (this.buildTimes.length > 40) this.buildTimes.shift();

    this.onChunk?.(chunk);
    this.cacheWrite(chunk);
    this.enforceBudget();
    this.publishStats();
  }

  private releaseSlot(id: number): void {
    const wi = this.workerOf.get(id);
    if (wi !== undefined && this.inflightPerWorker[wi] > 0) this.inflightPerWorker[wi]--;
    this.workerOf.delete(id);
    this.inflight.delete(id);
  }

  private cacheWrite(chunk: ResidentChunk): void {
    if (!this.opts.cache || !this.world) return;
    this.opts.cache.put(this.hash, chunk.key, chunk.payload);
  }

  /** Evict least-important chunks until both budgets are satisfied. */
  private enforceBudget(): void {
    if (this.resident.size <= this.opts.maxResident && this.bytes <= this.opts.maxBytes) return;
    const list = [...this.resident.entries()];
    // Worst first: small on screen (high priority number), then least recently used.
    list.sort((a, b) => (b[1].importance - a[1].importance) || (a[1].lastUsed - b[1].lastUsed));
    for (const [ks] of list) {
      if (this.resident.size <= this.opts.maxResident * 0.9 && this.bytes <= this.opts.maxBytes * 0.9) break;
      this.evict(ks);
    }
  }

  private evict(ks: string): void {
    const chunk = this.resident.get(ks);
    if (!chunk) return;
    this.bytes -= chunk.bytes;
    this.resident.delete(ks);
    this.stats.evicted++;
    this.onEvict?.(ks);
  }

  /** Try the disk cache for a whole batch of keys before queueing a mesh. */
  async warmFromCache(keys: ChunkKey[]): Promise<number> {
    if (!this.opts.cache) return 0;
    const got = await this.opts.cache.getMany(this.hash, keys);
    let n = 0;
    for (const [ks, payload] of got) {
      if (this.resident.has(ks)) continue;
      const bytes = WORKER_BYTES(payload);
      const chunk: ResidentChunk = {
        key: parseChunkKey(ks), payload, bytes,
        lastUsed: this.tick, importance: 1e8, fromCache: true,
      };
      this.resident.set(ks, chunk);
      this.bytes += bytes;
      n++;
      this.stats.cacheHits++;
      this.onChunk?.(chunk);
    }
    if (n === 0) this.stats.cacheMisses++;
    this.enforceBudget();
    this.publishStats();
    return n;
  }

  private publishStats(): void {
    this.stats.resident = this.resident.size;
    this.stats.pending = this.queue.length;
    this.stats.inflight = this.inflight.size;
    this.stats.bytes = this.bytes;
    this.stats.triangles = [...this.resident.values()].reduce((a, c) => a + c.payload.triangleCount, 0);
    this.stats.avgBuildMs = this.buildTimes.length
      ? this.buildTimes.reduce((a, b) => a + b, 0) / this.buildTimes.length
      : 0;
  }

  getStats(): StreamerStats { return { ...this.stats }; }

  has(keyString: string): boolean { return this.resident.has(keyString); }

  dispose(): void {
    for (const w of this.workers) w.terminate();
    this.workers = [];
    this.resident.clear();
    this.inflight.clear();
    this.queue = [];
    this.bytes = 0;
  }
}
