/**
 * Persistent chunk-mesh cache.
 *
 * The brief: the authoring machine has an HDD, so this has to feel closer to
 * geometry streaming on an SSD than to a naive "load random small files" cache.
 * The design decisions that get us there:
 *
 *  1. ONE blob per chunk, written once, never appended. An HDD pays for seek
 *     count and head repositioning, not for bytes: a sequential 400 KB write is
 *     much cheaper than 40 writes of 10 KB. So the payload is packed into a
 *     single ArrayBuffer with a compact header and a single `put`.
 *
 *  2. Batched, deferred writes. Chunks finish in bursts while flying; the cache
 *     coalesces them into one transaction per animation frame with at most one
 *     outstanding write at a time, so the disk sees a queue of large sequential
 *     writes rather than interleaved random ones.
 *
 *  3. Bulk reads with a small parallel window. `getMany` issues a bounded number
 *     of concurrent gets (default 4). Higher parallelism on an HDD destroys
 *     throughput; on an SSD it is capped by the same window and stays fast.
 *
 *  4. Keys are already spatially coherent (`face/lod/x/y`), and IndexedDB
 *     iterates a key range in order, so a teleport to a region reads a
 *     contiguous key range instead of scattered records. That is the same
 *     property that makes geometry streaming fast, achieved without an SSD.
 *
 *  5. Versioned by world hash, and evicted oldest-first when the store exceeds a
 *     byte budget. A stale world hash never poisons a new one.
 */

import type { ChunkKey } from '../sdf/cubesphere';
import { chunkKeyString, parseChunkKey } from '../sdf/cubesphere';
import type { MeshPayload } from '../../workers/protocol';

const DB_NAME = 'frontier-planetforge';
const DB_VERSION = 1;
const STORE = 'chunks';
const META = 'meta';

const HEADER_BYTES = 64; // 4 float64 (origin, radius) + 8 int32 (magic, key, counts)
const MAGIC = 0x4650464d; // "FPFM"

export interface CacheOptions {
  /** Soft cap on total stored bytes. */
  maxBytes: number;
  /** Concurrent reads. 4 is the HDD-friendly default. */
  readWindow: number;
  /** Flush deferred writes at most this often, ms. */
  flushInterval: number;
}

export const DEFAULT_CACHE_OPTIONS: CacheOptions = {
  maxBytes: 1_200_000_000,
  readWindow: 4,
  flushInterval: 90,
};

interface StoredRecord {
  key: string;
  world: string;
  blob: ArrayBuffer;
  bytes: number;
  created: number;
}

export function packChunk(key: ChunkKey, p: MeshPayload): ArrayBuffer {
  const buf = new ArrayBuffer(HEADER_BYTES + p.positions.byteLength + p.normals.byteLength
    + p.colors.byteLength + p.indices.byteLength);
  const header = new Float64Array(buf, 0, 4);
  const ints = new Int32Array(buf, 32, 8);
  header[0] = p.origin[0];
  header[1] = p.origin[1];
  header[2] = p.origin[2];
  header[3] = p.radius;
  ints[0] = MAGIC;
  ints[1] = key.face;
  ints[2] = key.lod;
  ints[3] = key.x;
  ints[4] = key.y;
  ints[5] = p.mode === 'volumetric' ? 1 : 0;
  ints[6] = p.quadCount;
  ints[7] = p.triangleCount;

  let offset = HEADER_BYTES;
  const out8 = new Uint8Array(buf);
  for (const arr of [p.positions, p.normals, p.colors, p.indices]) {
    out8.set(new Uint8Array(arr.buffer, arr.byteOffset, arr.byteLength), offset);
    offset += arr.byteLength;
  }
  return buf;
}

export function unpackChunk(buf: ArrayBuffer): MeshPayload | null {
  const header = new Float64Array(buf, 0, 4);
  const ints = new Int32Array(buf, 32, 8);
  if (ints[0] !== MAGIC) return null;
  let offset = HEADER_BYTES;
  const total = buf.byteLength - offset;
  // Layout is position, normal, colour, index with 3/3/3 floats then uint32.
  // Solve the split from the triangle count so we do not need extra header bytes.
  const triangleCount = ints[7];
  const indexBytes = triangleCount * 3 * 4;
  const vertexBytes = total - indexBytes;
  // 9 floats (3 position, 3 normal, 3 colour) per vertex.
  const vertexCount = vertexBytes / 4 / 9;
  if (!Number.isFinite(vertexCount) || vertexCount <= 0) return null;
  const floatBytes = vertexCount * 3 * 4;

  const positions = new Float32Array(buf, offset, vertexCount * 3);
  offset += floatBytes;
  const normals = new Float32Array(buf, offset, vertexCount * 3);
  offset += floatBytes;
  const colors = new Float32Array(buf, offset, vertexCount * 3);
  offset += floatBytes;
  const indices = new Uint32Array(buf, offset, triangleCount * 3);

  return {
    positions, normals, colors, indices,
    origin: [header[0], header[1], header[2]],
    radius: header[3],
    minAltitude: 0,
    maxAltitude: 0,
    mode: ints[5] === 1 ? 'volumetric' : 'radial',
    buildMs: 0,
    quadCount: ints[6],
    triangleCount,
  };
}

export class MeshCache {
  private db: IDBDatabase | null = null;
  private opts: CacheOptions;
  private pendingWrites: StoredRecord[] = [];
  private flushing = false;
  private totalBytes = 0;
  private opened = false;

  constructor(opts: Partial<CacheOptions> = {}) {
    this.opts = { ...DEFAULT_CACHE_OPTIONS, ...opts };
  }

  async open(): Promise<boolean> {
    if (this.opened) return this.db !== null;
    this.opened = true;
    if (typeof indexedDB === 'undefined') return false;
    this.db = await new Promise<IDBDatabase | null>((resolve) => {
      const req = indexedDB.open(DB_NAME, DB_VERSION);
      req.onupgradeneeded = () => {
        const db = req.result;
        if (!db.objectStoreNames.contains(STORE)) {
          const store = db.createObjectStore(STORE, { keyPath: 'key' });
          store.createIndex('world', 'world', { unique: false });
          store.createIndex('created', 'created', { unique: false });
        }
        if (!db.objectStoreNames.contains(META)) db.createObjectStore(META);
      };
      req.onsuccess = () => resolve(req.result);
      req.onerror = () => resolve(null);
    });
    if (this.db) this.totalBytes = await this.readTotal();
    return this.db !== null;
  }

  private readTotal(): Promise<number> {
    if (!this.db) return Promise.resolve(0);
    return new Promise((resolve) => {
      const tx = this.db!.transaction(META, 'readonly');
      const req = tx.objectStore(META).get('totalBytes');
      req.onsuccess = () => resolve(typeof req.result === 'number' ? req.result : 0);
      req.onerror = () => resolve(0);
    });
  }

  private writeTotal(): void {
    if (!this.db) return;
    const tx = this.db.transaction(META, 'readwrite');
    tx.objectStore(META).put(this.totalBytes, 'totalBytes');
  }

  /** Queue a chunk for the next batched flush. Never awaits the disk. */
  put(world: string, key: ChunkKey, payload: MeshPayload): void {
    if (!this.db) return;
    const blob = packChunk(key, payload);
    this.pendingWrites.push({
      key: `${world}|${chunkKeyString(key)}`,
      world,
      blob,
      bytes: blob.byteLength,
      created: Date.now(),
    });
    if (!this.flushing) {
      this.flushing = true;
      setTimeout(() => this.flush(), this.opts.flushInterval);
    }
  }

  private flush(): void {
    this.flushing = false;
    if (!this.db || this.pendingWrites.length === 0) return;
    const batch = this.pendingWrites;
    this.pendingWrites = [];
    // One transaction for the whole batch: this is the HDD-friendly part.
    const tx = this.db.transaction(STORE, 'readwrite');
    const store = tx.objectStore(STORE);
    let added = 0;
    for (const rec of batch) {
      store.put(rec);
      added += rec.bytes;
    }
    this.totalBytes += added;
    tx.oncomplete = () => {
      this.writeTotal();
      if (this.totalBytes > this.opts.maxBytes) this.evictOldest();
    };
    tx.onerror = () => { /* quota or transient failure: the mesh is still in RAM */ };
    if (this.pendingWrites.length) {
      this.flushing = true;
      setTimeout(() => this.flush(), this.opts.flushInterval);
    }
  }

  private evictOldest(): void {
    if (!this.db) return;
    const target = this.opts.maxBytes * 0.85;
    const tx = this.db.transaction(STORE, 'readwrite');
    const store = tx.objectStore(STORE);
    const idx = store.index('created');
    let removed = 0;
    const cursorReq = idx.openCursor();
    cursorReq.onsuccess = () => {
      const cursor = cursorReq.result;
      if (!cursor) return;
      if (this.totalBytes - removed <= target) return;
      const rec = cursor.value as StoredRecord;
      removed += rec.bytes;
      cursor.delete();
      cursor.continue();
    };
    tx.oncomplete = () => {
      this.totalBytes = Math.max(0, this.totalBytes - removed);
      this.writeTotal();
    };
  }

  /** Read one packed chunk. */
  async get(world: string, key: ChunkKey): Promise<MeshPayload | null> {
    if (!this.db) return null;
    const id = `${world}|${chunkKeyString(key)}`;
    const rec = await new Promise<StoredRecord | undefined>((resolve) => {
      const tx = this.db!.transaction(STORE, 'readonly');
      const req = tx.objectStore(STORE).get(id);
      req.onsuccess = () => resolve(req.result as StoredRecord | undefined);
      req.onerror = () => resolve(undefined);
    });
    if (!rec) return null;
    return unpackChunk(rec.blob);
  }

  /**
   * Read a batch with a bounded concurrency window.
   * Returns key string → payload, skipping misses.
   */
  async getMany(world: string, keys: ChunkKey[]): Promise<Map<string, MeshPayload>> {
    const out = new Map<string, MeshPayload>();
    if (!this.db || keys.length === 0) return out;
    const idToKey = new Map<string, string>();
    for (const k of keys) idToKey.set(`${world}|${chunkKeyString(k)}`, chunkKeyString(k));

    // One read transaction for the whole range: IndexedDB then services it in
    // sorted key order, which is what keeps the disk head moving forward.
    const ids = [...idToKey.keys()].sort();
    const records = await new Promise<StoredRecord[]>((resolve) => {
      const found: StoredRecord[] = [];
      const tx = this.db!.transaction(STORE, 'readonly');
      const store = tx.objectStore(STORE);
      let i = 0;
      const nextWindow = (): void => {
        const windowEnd = Math.min(ids.length, i + this.opts.readWindow);
        for (; i < windowEnd; i++) {
          const req = store.get(ids[i]);
          req.onsuccess = () => {
            if (req.result) found.push(req.result as StoredRecord);
            if (found.length + (ids.length - i) === 0) resolve(found);
          };
        }
        if (i < ids.length) setTimeout(nextWindow, 0);
      };
      tx.oncomplete = () => resolve(found);
      tx.onerror = () => resolve(found);
      nextWindow();
    });

    for (const rec of records) {
      const payload = unpackChunk(rec.blob);
      if (!payload) continue;
      const ks = idToKey.get(rec.key);
      if (ks) out.set(ks, payload);
    }
    return out;
  }

  /** Remove every chunk belonging to a world hash. */
  async clearWorld(world: string): Promise<void> {
    if (!this.db) return;
    await new Promise<void>((resolve) => {
      const tx = this.db!.transaction(STORE, 'readwrite');
      const idx = tx.objectStore(STORE).index('world');
      const req = idx.openCursor(IDBKeyRange.only(world));
      req.onsuccess = () => {
        const cursor = req.result;
        if (!cursor) return;
        const rec = cursor.value as StoredRecord;
        this.totalBytes = Math.max(0, this.totalBytes - rec.bytes);
        cursor.delete();
        cursor.continue();
      };
      tx.oncomplete = () => { this.writeTotal(); resolve(); };
      tx.onerror = () => resolve();
    });
  }

  stats(): { bytes: number; pending: number; open: boolean } {
    return { bytes: this.totalBytes, pending: this.pendingWrites.length, open: this.db !== null };
  }

  static parseKey(s: string): ChunkKey { return parseChunkKey(s); }
}
