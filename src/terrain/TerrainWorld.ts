export const WORLD_METERS = 16_384
export const WORLD_SCENE_SIZE = 160
export const METERS_PER_SCENE_UNIT = WORLD_METERS / WORLD_SCENE_SIZE
export const SDF_CHUNK_SIZE = 20
export const SHAPE_CHUNK_RESOLUTION = 65
export const VOLUME_RESOLUTION = 24
export const VOLUME_MIN_Y = -12
export const VOLUME_MAX_Y = 40
export const STREAM_RADIUS = 3

export type ShapeTool = 'add' | 'subtract' | 'smooth'
export type SculptTool = 'sculpt' | 'raise' | 'lower' | 'smooth' | 'flatten' | 'ramp' | 'erosion' | 'noise'
export type LayerKind = 'base' | 'procedural' | 'sculpt'

export interface TerrainLayer {
  id: string
  name: string
  kind: LayerKind
  visible: boolean
  opacity: number
  locked: boolean
  baked: boolean
}

export interface WorldChange {
  type: 'shape' | 'height' | 'layers' | 'all'
  keys?: string[]
  islandId?: string
}

interface ShapeChunk {
  cx: number
  cz: number
  data: Float32Array
  modified: boolean
}

export interface VolumeChunk {
  cx: number
  cz: number
  base: Float32Array
  field: Float32Array
  residuals: Map<string, Float32Array>
  baked: boolean
}

interface WorldSnapshot {
  shapeChunks: ShapeChunk[]
  volumeChunks: VolumeChunk[]
  layers: TerrainLayer[]
  blank: boolean
  keys?: string[]
}

interface SerializedWorldV2 {
  version: 2
  name: string
  blank: boolean
  chunkSize: number
  shapeResolution: number
  volumeResolution: number
  minY: number
  maxY: number
  shapeChunks: Array<{ cx: number; cz: number; data: string }>
  volumeChunks: Array<{
    cx: number
    cz: number
    baked: boolean
    base?: string
    residuals: Record<string, string>
  }>
  customLayers: Array<Pick<TerrainLayer, 'id' | 'name' | 'visible' | 'opacity' | 'locked' | 'baked'>>
  layerSettings: Record<string, Pick<TerrainLayer, 'visible' | 'opacity' | 'locked' | 'baked'>>
}

const clamp = (value: number, min = 0, max = 1) => Math.max(min, Math.min(max, value))
const smoothstep = (edge0: number, edge1: number, value: number) => {
  const x = clamp((value - edge0) / (edge1 - edge0))
  return x * x * (3 - 2 * x)
}
const smoothMax = (a: number, b: number, k: number) => {
  const h = Math.max(k - Math.abs(a - b), 0) / Math.max(k, 1e-5)
  return Math.max(a, b) + h * h * k * 0.25
}
const smoothMin = (a: number, b: number, k: number) => -smoothMax(-a, -b, k)
const chunkKey = (cx: number, cz: number) => `${cx},${cz}`

export interface IslandBounds {
  minX: number
  maxX: number
  minZ: number
  maxZ: number
}

export interface WorldPoint {
  x: number
  y: number
  z: number
}

interface VolumePrimitive extends WorldPoint {
  rx: number
  ry: number
  rz: number
}

const ISLAND_PROFILES: Record<string, { name: string; cx: number; cz: number; rx: number; rz: number; rotation: number; bounds: IslandBounds }> = {
  sable: { name: 'Sable Isle', cx: -0.65, cz: -0.23, rx: 0.095, rz: 0.14, rotation: -0.3, bounds: { minX: -64, maxX: -40, minZ: -34, maxZ: 0 } },
  east: { name: 'East Key', cx: 0.62, cz: 0.02, rx: 0.075, rz: 0.11, rotation: 0.2, bounds: { minX: 38, maxX: 62, minZ: -12, maxZ: 16 } },
  south: { name: 'South Atoll', cx: -0.48, cz: 0.52, rx: 0.09, rz: 0.06, rotation: 0.4, bounds: { minX: -50, maxX: -26, minZ: 32, maxZ: 54 } },
  far: { name: 'Far Shoal', cx: 0.1, cz: 0.59, rx: 0.045, rz: 0.07, rotation: -0.4, bounds: { minX: -2, maxX: 18, minZ: 36, maxZ: 60 } },
}

// Aurelia's mountains are closed 3D solids, not heights sampled on a grid.
const MAIN_LANDFORM_VOLUMES: VolumePrimitive[] = [
  { x: -31, y: -1, z: -5, rx: 15, ry: 18, rz: 13 },
  { x: -20, y: 0, z: -9, rx: 17, ry: 27, rz: 16 },
  { x: -6, y: 1, z: -4, rx: 15, ry: 32, rz: 15 },
  { x: 8, y: 0, z: -8, rx: 16, ry: 25, rz: 14 },
  { x: 22, y: 1, z: -13, rx: 15, ry: 30, rz: 16 },
  { x: 29, y: -2, z: 8, rx: 13, ry: 20, rz: 14 },
  { x: -30, y: -2, z: 17, rx: 14, ry: 15, rz: 12 },
]

const MAIN_CAVE_VOLUMES: VolumePrimitive[] = [
  { x: -14, y: 4, z: -2, rx: 7, ry: 3.2, rz: 4.5 },
  { x: 19, y: 7, z: -10, rx: 6, ry: 3.8, rz: 4 },
]

function hash(x: number, y: number) {
  let h = Math.imul(x, 374761393) + Math.imul(y, 668265263)
  h = (h ^ (h >>> 13)) * 1274126177
  return ((h ^ (h >>> 16)) >>> 0) / 4294967295
}

export function valueNoise(x: number, y: number) {
  const ix = Math.floor(x)
  const iy = Math.floor(y)
  const fx = x - ix
  const fy = y - iy
  const ux = fx * fx * (3 - 2 * fx)
  const uy = fy * fy * (3 - 2 * fy)
  const a = hash(ix, iy)
  const b = hash(ix + 1, iy)
  const c = hash(ix, iy + 1)
  const d = hash(ix + 1, iy + 1)
  return (a * (1 - ux) + b * ux) * (1 - uy) + (c * (1 - ux) + d * ux) * uy
}

export function fbm(x: number, y: number, octaves = 4) {
  let value = 0
  let amplitude = 0.5
  let frequency = 1
  let total = 0
  for (let i = 0; i < octaves; i += 1) {
    value += valueNoise(x * frequency, y * frequency) * amplitude
    total += amplitude
    amplitude *= 0.5
    frequency *= 2.03
  }
  return value / total
}

function hash3(x: number, y: number, z: number) {
  let h = Math.imul(x, 374761393) ^ Math.imul(y, 668265263) ^ Math.imul(z, 2147483647)
  h = Math.imul(h ^ (h >>> 13), 1274126177)
  return ((h ^ (h >>> 16)) >>> 0) / 4294967295
}

export function valueNoise3D(x: number, y: number, z: number) {
  const ix = Math.floor(x)
  const iy = Math.floor(y)
  const iz = Math.floor(z)
  const fx = x - ix
  const fy = y - iy
  const fz = z - iz
  const ux = fx * fx * (3 - 2 * fx)
  const uy = fy * fy * (3 - 2 * fy)
  const uz = fz * fz * (3 - 2 * fz)
  const n000 = hash3(ix, iy, iz)
  const n100 = hash3(ix + 1, iy, iz)
  const n010 = hash3(ix, iy + 1, iz)
  const n110 = hash3(ix + 1, iy + 1, iz)
  const n001 = hash3(ix, iy, iz + 1)
  const n101 = hash3(ix + 1, iy, iz + 1)
  const n011 = hash3(ix, iy + 1, iz + 1)
  const n111 = hash3(ix + 1, iy + 1, iz + 1)
  const x00 = n000 + (n100 - n000) * ux
  const x10 = n010 + (n110 - n010) * ux
  const x01 = n001 + (n101 - n001) * ux
  const x11 = n011 + (n111 - n011) * ux
  const y0 = x00 + (x10 - x00) * uy
  const y1 = x01 + (x11 - x01) * uy
  return y0 + (y1 - y0) * uz
}

export function fbm3D(x: number, y: number, z: number, octaves = 2) {
  let value = 0
  let amplitude = 0.5
  let total = 0
  let frequency = 1
  for (let octave = 0; octave < octaves; octave += 1) {
    value += valueNoise3D(x * frequency, y * frequency, z * frequency) * amplitude
    total += amplitude
    amplitude *= 0.5
    frequency *= 2.03
  }
  return value / total
}

/** Positive inside; an inexpensive signed-distance approximation for a 3D ellipsoid. */
function ellipsoidDensity(x: number, y: number, z: number, primitive: VolumePrimitive) {
  const px = (x - primitive.x) / primitive.rx
  const py = (y - primitive.y) / primitive.ry
  const pz = (z - primitive.z) / primitive.rz
  return (1 - Math.sqrt(px * px + py * py + pz * pz)) * Math.min(primitive.rx, primitive.ry, primitive.rz)
}

function ellipseSdf(x: number, z: number, cx: number, cz: number, rx: number, rz: number, rotation: number) {
  const cos = Math.cos(rotation)
  const sin = Math.sin(rotation)
  const px = x - cx
  const pz = z - cz
  const qx = (px * cos - pz * sin) / rx
  const qz = (px * sin + pz * cos) / rz
  return -(Math.sqrt(qx * qx + qz * qz) - 1) * Math.min(rx, rz)
}

function toBase64(buffer: Float32Array) {
  const bytes = new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength)
  let binary = ''
  const chunk = 0x8000
  for (let i = 0; i < bytes.length; i += chunk) binary += String.fromCharCode(...bytes.subarray(i, i + chunk))
  return btoa(binary)
}

function fromBase64(value: string) {
  const binary = atob(value)
  const bytes = new Uint8Array(binary.length)
  for (let i = 0; i < binary.length; i += 1) bytes[i] = binary.charCodeAt(i)
  return new Float32Array(bytes.buffer)
}

function cloneShapeChunk(chunk: ShapeChunk): ShapeChunk {
  return { ...chunk, data: chunk.data.slice() }
}

function cloneVolumeChunk(chunk: VolumeChunk): VolumeChunk {
  return {
    ...chunk,
    base: chunk.base.slice(),
    field: chunk.field.slice(),
    residuals: new Map([...chunk.residuals].map(([id, data]) => [id, data.slice()])),
  }
}

export class SDFIsland {
  private shapeChunks = new Map<string, ShapeChunk>()
  private volumeChunks = new Map<string, VolumeChunk>()
  private listeners = new Set<(change: WorldChange) => void>()
  private undoStack: WorldSnapshot[] = []
  private redoStack: WorldSnapshot[] = []
  private strokeSnapshot: WorldSnapshot | null = null
  private blank = false
  customLayers: TerrainLayer[] = []
  layers: TerrainLayer[] = []
  activeLayerId = 'sculpt'

  constructor(readonly profile = 'main', blank = false) {
    this.blank = blank
    this.resetLayers()
  }

  private resetLayers() {
    this.layers = [
      { id: 'base', name: 'Continental SDF', kind: 'base', visible: true, opacity: 1, locked: false, baked: false },
      { id: 'landforms', name: '3D landforms', kind: 'procedural', visible: true, opacity: 0.82, locked: false, baked: false },
      { id: 'detail', name: '3D erosion', kind: 'procedural', visible: true, opacity: 0.64, locked: false, baked: false },
      { id: 'sculpt', name: 'Volumetric sculpt', kind: 'sculpt', visible: true, opacity: 1, locked: false, baked: false },
    ]
    this.customLayers = []
    this.activeLayerId = 'sculpt'
  }

  subscribe(listener: (change: WorldChange) => void) {
    this.listeners.add(listener)
    return () => this.listeners.delete(listener)
  }

  private emit(type: WorldChange['type'], keys?: string[]) {
    this.listeners.forEach((listener) => listener({ type, keys }))
  }

  get allocatedShapeChunks() { return this.shapeChunks.size }
  get allocatedVolumeChunks() { return this.volumeChunks.size }
  get volumeKeys() { return [...this.volumeChunks.keys()] }
  get editedVolumeChunks() { return [...this.volumeChunks.values()].filter((chunk) => chunk.residuals.size > 0 || chunk.baked).length }

  getContentBounds(): IslandBounds | null {
    const profile = ISLAND_PROFILES[this.profile]
    let bounds: IslandBounds | null = profile ? { ...profile.bounds } : this.profile === 'main' ? { minX: -80, maxX: 80, minZ: -72, maxZ: 72 } : null
    const includeChunk = (cx: number, cz: number) => {
      const minX = cx * SDF_CHUNK_SIZE
      const minZ = cz * SDF_CHUNK_SIZE
      const maxX = minX + SDF_CHUNK_SIZE
      const maxZ = minZ + SDF_CHUNK_SIZE
      if (!bounds) bounds = { minX, maxX, minZ, maxZ }
      else {
        bounds.minX = Math.min(bounds.minX, minX)
        bounds.maxX = Math.max(bounds.maxX, maxX)
        bounds.minZ = Math.min(bounds.minZ, minZ)
        bounds.maxZ = Math.max(bounds.maxZ, maxZ)
      }
    }
    for (const chunk of this.shapeChunks.values()) if (chunk.modified) includeChunk(chunk.cx, chunk.cz)
    for (const chunk of this.volumeChunks.values()) if (chunk.baked || chunk.residuals.size > 0) includeChunk(chunk.cx, chunk.cz)
    return bounds
  }

  private defaultShapeAt(x: number, z: number) {
    if (this.blank) {
      return -0.65 + (fbm(x * 0.08 + 4, z * 0.08 - 7, 3) - 0.5) * 0.018
    }
    const nx = x / 80
    const nz = z / 80
    const profile = ISLAND_PROFILES[this.profile]
    if (profile) {
      const field = ellipseSdf(nx, nz, profile.cx, profile.cz, profile.rx, profile.rz, profile.rotation)
      const erosion = (fbm(nx * 18 + 31, nz * 18 - 17, 4) - 0.5) * 0.018
      return field + erosion
    }
    const warpX = (fbm(nx * 2.1 + 18, nz * 2.1 - 4, 3) - 0.5) * 0.055
    const warpZ = (fbm(nx * 2.2 - 8, nz * 2.2 + 13, 3) - 0.5) * 0.05
    const px = nx + warpX
    const pz = nz + warpZ
    let field = ellipseSdf(px, pz, -0.12, -0.015, 0.54, 0.43, -0.2)
    field = smoothMax(field, ellipseSdf(px, pz, 0.15, -0.17, 0.37, 0.41, 0.45), 0.13)
    field = smoothMax(field, ellipseSdf(px, pz, -0.27, 0.25, 0.31, 0.25, 0.2), 0.11)
    field = smoothMax(field, ellipseSdf(px, pz, 0.31, 0.23, 0.24, 0.19, -0.4), 0.09)
    field = smoothMax(field, ellipseSdf(px, pz, -0.53, 0.015, 0.2, 0.085, 0.12), 0.055)
    field = smoothMax(field, ellipseSdf(px, pz, 0.39, -0.34, 0.18, 0.095, -0.25), 0.05)
    field = smoothMin(field, -ellipseSdf(px, pz, -0.5, -0.32, 0.17, 0.135, -0.3), 0.035)
    field = smoothMin(field, -ellipseSdf(px, pz, 0.37, 0.39, 0.17, 0.13, 0.2), 0.04)
    field = smoothMin(field, -ellipseSdf(px, pz, -0.07, 0.48, 0.115, 0.17, -0.2), 0.035)
    const coastNoise = (fbm(nx * 8.5 + 31, nz * 8.5 - 17, 4) - 0.5) * 0.06
    const fineNoise = (valueNoise(nx * 35 + 5, nz * 35 + 9) - 0.5) * 0.011
    return field + coastNoise + fineNoise
  }

  private createShapeChunk(cx: number, cz: number): ShapeChunk {
    const data = new Float32Array(SHAPE_CHUNK_RESOLUTION * SHAPE_CHUNK_RESOLUTION)
    const step = SDF_CHUNK_SIZE / (SHAPE_CHUNK_RESOLUTION - 1)
    for (let z = 0; z < SHAPE_CHUNK_RESOLUTION; z += 1) {
      for (let x = 0; x < SHAPE_CHUNK_RESOLUTION; x += 1) {
        data[z * SHAPE_CHUNK_RESOLUTION + x] = this.defaultShapeAt(cx * SDF_CHUNK_SIZE + x * step, cz * SDF_CHUNK_SIZE + z * step)
      }
    }
    const chunk = { cx, cz, data, modified: false }
    this.shapeChunks.set(chunkKey(cx, cz), chunk)
    return chunk
  }

  private getShapeChunk(cx: number, cz: number) {
    const key = chunkKey(cx, cz)
    return this.shapeChunks.get(key) ?? this.createShapeChunk(cx, cz)
  }

  sampleShapeWorld(x: number, z: number) {
    const cx = Math.floor(x / SDF_CHUNK_SIZE)
    const cz = Math.floor(z / SDF_CHUNK_SIZE)
    const chunk = this.getShapeChunk(cx, cz)
    const localX = (x - cx * SDF_CHUNK_SIZE) / SDF_CHUNK_SIZE * (SHAPE_CHUNK_RESOLUTION - 1)
    const localZ = (z - cz * SDF_CHUNK_SIZE) / SDF_CHUNK_SIZE * (SHAPE_CHUNK_RESOLUTION - 1)
    const ix = Math.min(SHAPE_CHUNK_RESOLUTION - 2, Math.max(0, Math.floor(localX)))
    const iz = Math.min(SHAPE_CHUNK_RESOLUTION - 2, Math.max(0, Math.floor(localZ)))
    const fx = clamp(localX - ix)
    const fz = clamp(localZ - iz)
    const stride = SHAPE_CHUNK_RESOLUTION
    const a = chunk.data[iz * stride + ix]
    const b = chunk.data[iz * stride + ix + 1]
    const c = chunk.data[(iz + 1) * stride + ix]
    const d = chunk.data[(iz + 1) * stride + ix + 1]
    return (a * (1 - fx) + b * fx) * (1 - fz) + (c * (1 - fx) + d * fx) * fz
  }

  sampleShape(nx: number, nz: number) {
    return this.sampleShapeWorld(nx * 80, nz * 80)
  }

  pruneShapeChunks(centerX: number, centerZ: number, radius = 10) {
    const centerCx = Math.floor(centerX / SDF_CHUNK_SIZE)
    const centerCz = Math.floor(centerZ / SDF_CHUNK_SIZE)
    for (const [key, chunk] of this.shapeChunks) {
      if (!chunk.modified && (Math.abs(chunk.cx - centerCx) > radius || Math.abs(chunk.cz - centerCz) > radius)) {
        this.shapeChunks.delete(key)
      }
    }
  }

  applyShapeBrushWorld(x: number, z: number, radius: number, tool: ShapeTool, strength: number, falloff = 0.6, erosion = 0.65) {
    const minCx = Math.floor((x - radius * 1.4) / SDF_CHUNK_SIZE)
    const maxCx = Math.floor((x + radius * 1.4) / SDF_CHUNK_SIZE)
    const minCz = Math.floor((z - radius * 1.4) / SDF_CHUNK_SIZE)
    const maxCz = Math.floor((z + radius * 1.4) / SDF_CHUNK_SIZE)
    const step = SDF_CHUNK_SIZE / (SHAPE_CHUNK_RESOLUTION - 1)
    for (let cz = minCz; cz <= maxCz; cz += 1) {
      for (let cx = minCx; cx <= maxCx; cx += 1) {
        const chunk = this.getShapeChunk(cx, cz)
        let changed = false
        for (let iz = 0; iz < SHAPE_CHUNK_RESOLUTION; iz += 1) {
          const pz = cz * SDF_CHUNK_SIZE + iz * step
          for (let ix = 0; ix < SHAPE_CHUNK_RESOLUTION; ix += 1) {
            const px = cx * SDF_CHUNK_SIZE + ix * step
            if (Math.abs(px - x) > radius * 1.5 || Math.abs(pz - z) > radius * 1.5) continue
            const warpAmount = radius * erosion * 0.26
            const warpX = (fbm(px * 0.115 + 17, pz * 0.115 - 8, 3) - 0.5) * warpAmount
            const warpZ = (fbm(px * 0.115 - 5, pz * 0.115 + 19, 3) - 0.5) * warpAmount
            const distance = Math.hypot(px + warpX - x, pz + warpZ - z)
            if (distance > radius * 1.35) continue
            const broadErosion = (fbm(px * 0.31 + 9, pz * 0.31 - 6, 4) - 0.5) * radius * erosion * 0.36
            const channel = Math.pow(1 - Math.abs(valueNoise(px * 0.57 + 3, pz * 0.57 - 11) * 2 - 1), 4)
            const channelCut = channel * radius * erosion * 0.11 * smoothstep(radius * 0.2, radius * 0.95, distance)
            const circle = radius - distance + broadErosion - channelCut
            const index = iz * SHAPE_CHUNK_RESOLUTION + ix
            const current = chunk.data[index]
            const edge = clamp(1 - distance / (radius * 1.25))
            const weight = smoothstep(0, Math.max(0.08, 1 - falloff * 0.7), edge) * clamp(strength, 0.05, 1)
            if (tool === 'add') {
              chunk.data[index] = current + (smoothMax(current, circle, radius * 0.2) - current) * Math.max(0.9, weight)
            } else if (tool === 'subtract') {
              chunk.data[index] = current + (smoothMin(current, -circle, radius * 0.18) - current) * Math.max(0.9, weight)
            } else {
              const average = (
                this.sampleShapeWorld(px - step, pz) + this.sampleShapeWorld(px + step, pz) +
                this.sampleShapeWorld(px, pz - step) + this.sampleShapeWorld(px, pz + step) + current * 2
              ) / 6
              chunk.data[index] = current + (average - current) * weight * 0.72
            }
            changed = true
          }
        }
        if (changed) chunk.modified = true
      }
    }
  }

  applyShapeBrush(nx: number, nz: number, radiusMeters: number, tool: ShapeTool, strength: number, falloff = 0.6, erosion = 0.65) {
    this.applyShapeBrushWorld(nx * 80, nz * 80, radiusMeters / METERS_PER_SCENE_UNIT, tool, strength, falloff, erosion)
  }

  finishShapeStroke() {
    const keys: string[] = []
    for (const [key, chunk] of this.volumeChunks) {
      if (!chunk.baked) {
        this.regenerateVolumeBase(chunk)
        keys.push(key)
      }
    }
    this.emit('shape', keys)
  }

  private landformVolumes(): VolumePrimitive[] {
    if (this.blank) return []
    if (this.profile === 'main') return MAIN_LANDFORM_VOLUMES
    const profile = ISLAND_PROFILES[this.profile]
    if (!profile) return []
    const x = profile.cx * 80
    const z = profile.cz * 80
    const rx = profile.rx * 52
    const rz = profile.rz * 48
    return [
      { x: x - rx * 0.16, y: -0.9, z, rx, ry: 4.2, rz },
      { x: x + rx * 0.28, y: -1.4, z: z - rz * 0.18, rx: rx * 0.64, ry: 3.1, rz: rz * 0.7 },
    ]
  }

  private generateVolumeBase(cx: number, cz: number) {
    const resolution = VOLUME_RESOLUTION
    const base = new Float32Array(resolution ** 3)
    const coast = new Float32Array(resolution * resolution)
    const stepXZ = SDF_CHUNK_SIZE / (resolution - 3)
    const stepY = (VOLUME_MAX_Y - VOLUME_MIN_Y) / (resolution - 3)
    const baseLayer = this.layers.find((layer) => layer.id === 'base')
    const landforms = this.layers.find((layer) => layer.id === 'landforms')
    const details = this.layers.find((layer) => layer.id === 'detail')
    const minX = cx * SDF_CHUNK_SIZE - stepXZ - 5
    const maxX = cx * SDF_CHUNK_SIZE + SDF_CHUNK_SIZE + stepXZ + 5
    const minZ = cz * SDF_CHUNK_SIZE - stepXZ - 5
    const maxZ = cz * SDF_CHUNK_SIZE + SDF_CHUNK_SIZE + stepXZ + 5
    const touchesChunk = (primitive: VolumePrimitive) => (
      primitive.x + primitive.rx >= minX && primitive.x - primitive.rx <= maxX &&
      primitive.z + primitive.rz >= minZ && primitive.z - primitive.rz <= maxZ
    )
    const volumes = landforms?.visible ? this.landformVolumes().filter(touchesChunk) : []
    const caves = !this.blank && this.profile === 'main' && landforms?.visible ? MAIN_CAVE_VOLUMES.filter(touchesChunk) : []

    for (let z = 0; z < resolution; z += 1) {
      const wz = cz * SDF_CHUNK_SIZE + (z - 1) * stepXZ
      for (let x = 0; x < resolution; x += 1) {
        const wx = cx * SDF_CHUNK_SIZE + (x - 1) * stepXZ
        coast[z * resolution + x] = this.sampleShapeWorld(wx, wz) * 34
      }
    }

    for (let z = 0; z < resolution; z += 1) {
      const wz = cz * SDF_CHUNK_SIZE + (z - 1) * stepXZ
      for (let y = 0; y < resolution; y += 1) {
        const wy = VOLUME_MIN_Y + (y - 1) * stepY
        for (let x = 0; x < resolution; x += 1) {
          const wx = cx * SDF_CHUNK_SIZE + (x - 1) * stepXZ
          const horizontal = z * resolution + x
          const coastDistance = coast[horizontal]
          const index = z * resolution * resolution + y * resolution + x

          // Intersect the coastline SDF with a rounded continental crust volume.
          let density = -30
          if (baseLayer?.visible) {
            const submergedSlope = coastDistance + Math.min(0, wy) * 0.18
            const verticalSlab = Math.min(wy - VOLUME_MIN_Y, 1.6 - wy)
            density = smoothMin(submergedSlope, verticalSlab, 0.85)
          }

          // Union closed, domain-warped 3D ellipsoids to create mountain solids.
          if (volumes.length > 0) {
            let nearVolume = false
            for (const primitive of volumes) {
              if (
                Math.abs(wx - primitive.x) <= primitive.rx + 5 &&
                Math.abs(wy - primitive.y) <= primitive.ry + 5 &&
                Math.abs(wz - primitive.z) <= primitive.rz + 5
              ) {
                nearVolume = true
                break
              }
            }
            if (nearVolume) {
              const warpX = (valueNoise3D(wx * 0.09 + 13, wy * 0.08, wz * 0.09 - 4) - 0.5) * 4.2
              const warpY = (valueNoise3D(wx * 0.075 - 8, wy * 0.095 + 3, wz * 0.075) - 0.5) * 3.4
              const warpZ = (valueNoise3D(wx * 0.085, wy * 0.08 - 7, wz * 0.085 + 11) - 0.5) * 4.2
              let mountains = -1000
              for (const primitive of volumes) {
                const primitiveDensity = ellipsoidDensity(wx + warpX, wy + warpY, wz + warpZ, primitive)
                mountains = smoothMax(mountains, primitiveDensity, 2.6)
              }
              const combined = smoothMax(density, mountains, 2.2)
              density += (combined - density) * (landforms?.opacity ?? 0)
            }
          }

          // Erode the actual volume in all three axes, not a height column.
          if (details?.visible) {
            const broad = (fbm3D(wx * 0.19 + 12, wy * 0.23 - 4, wz * 0.19 + 7, 2) - 0.5) * 3.2
            const crag = (valueNoise3D(wx * 0.55, wy * 0.34, wz * 0.55) - 0.5) * 0.65
            const influence = smoothstep(-8, -1, coastDistance)
            density += (broad + crag) * details.opacity * influence
          }

          // Keep the CSG landforms inside the independently editable coastline.
          const horizontalBoundary = coastDistance + 2 + Math.max(0, -wy) * 0.12
          density = smoothMin(density, horizontalBoundary, 0.9)
          density = smoothMin(density, wy - VOLUME_MIN_Y, 0.6)

          // Carve cave and arch volumes last so erosion noise cannot refill them.
          // These are closed interior cavities, which no heightfield can represent.
          for (const cave of caves) {
            const caveDensity = ellipsoidDensity(wx, wy, wz, cave)
            if (caveDensity > -1.5) density = smoothMin(density, -caveDensity, 0.45)
          }
          base[index] = density
        }
      }
    }
    return base
  }

  hasVolumeSurface(cx: number, cz: number) {
    const field = this.getVolumeChunk(cx, cz).field
    let solid = false
    let empty = false
    for (let i = 0; i < field.length; i += 1) {
      if (field[i] >= 0) solid = true
      else empty = true
      if (solid && empty) return true
    }
    return false
  }

  getVolumeChunk(cx: number, cz: number): VolumeChunk {
    const key = chunkKey(cx, cz)
    const existing = this.volumeChunks.get(key)
    if (existing) return existing
    const base = this.generateVolumeBase(cx, cz)
    const chunk: VolumeChunk = { cx, cz, base, field: base.slice(), residuals: new Map(), baked: false }
    this.volumeChunks.set(key, chunk)
    this.combineVolumeChunk(chunk)
    return chunk
  }

  private regenerateVolumeBase(chunk: VolumeChunk) {
    chunk.base = this.generateVolumeBase(chunk.cx, chunk.cz)
    this.combineVolumeChunk(chunk)
  }

  private combineVolumeChunk(chunk: VolumeChunk) {
    chunk.field.set(chunk.base)
    for (const [layerId, residual] of chunk.residuals) {
      const layer = this.getLayer(layerId)
      if (!layer?.visible || layer.baked) continue
      const opacity = layer.opacity
      for (let i = 0; i < chunk.field.length; i += 1) chunk.field[i] += residual[i] * opacity
    }
  }

  releaseUnusedVolumeChunks(retain: Set<string>) {
    for (const [key, chunk] of this.volumeChunks) {
      if (!retain.has(key) && chunk.residuals.size === 0 && !chunk.baked) this.volumeChunks.delete(key)
    }
    for (const [key, chunk] of this.shapeChunks) {
      const near = [...retain].some((retained) => retained === key)
      if (!near && !chunk.modified && this.shapeChunks.size > 100) this.shapeChunks.delete(key)
    }
  }

  sampleDensity(x: number, y: number, z: number) {
    if (y > VOLUME_MAX_Y + 4) return VOLUME_MAX_Y - y
    if (y < VOLUME_MIN_Y - 4) return VOLUME_MIN_Y - y
    const cx = Math.floor(x / SDF_CHUNK_SIZE)
    const cz = Math.floor(z / SDF_CHUNK_SIZE)
    const chunk = this.getVolumeChunk(cx, cz)
    const resolution = VOLUME_RESOLUTION
    const stepXZ = SDF_CHUNK_SIZE / (resolution - 3)
    const stepY = (VOLUME_MAX_Y - VOLUME_MIN_Y) / (resolution - 3)
    const gx = clamp((x - cx * SDF_CHUNK_SIZE) / stepXZ + 1, 0, resolution - 1.001)
    const gy = clamp((y - VOLUME_MIN_Y) / stepY + 1, 0, resolution - 1.001)
    const gz = clamp((z - cz * SDF_CHUNK_SIZE) / stepXZ + 1, 0, resolution - 1.001)
    const ix = Math.floor(gx)
    const iy = Math.floor(gy)
    const iz = Math.floor(gz)
    const fx = gx - ix
    const fy = gy - iy
    const fz = gz - iz
    const at = (dx: number, dy: number, dz: number) => chunk.field[(iz + dz) * resolution * resolution + (iy + dy) * resolution + ix + dx]
    const x00 = at(0, 0, 0) * (1 - fx) + at(1, 0, 0) * fx
    const x10 = at(0, 1, 0) * (1 - fx) + at(1, 1, 0) * fx
    const x01 = at(0, 0, 1) * (1 - fx) + at(1, 0, 1) * fx
    const x11 = at(0, 1, 1) * (1 - fx) + at(1, 1, 1) * fx
    return (x00 * (1 - fy) + x10 * fy) * (1 - fz) + (x01 * (1 - fy) + x11 * fy) * fz
  }

  applySculptBrushWorld(
    x: number,
    y: number,
    z: number,
    radius: number,
    tool: SculptTool,
    strength: number,
    falloff: number,
    flattenTarget = 0,
    symmetryX = false,
    symmetryZ = false,
    subtract = false,
  ) {
    const layer = this.getLayer(this.activeLayerId)
    if (!layer || layer.kind !== 'sculpt' || layer.locked || layer.baked || !layer.visible) return
    const bounds = this.getContentBounds()
    const centerX = bounds ? (bounds.minX + bounds.maxX) * 0.5 : 0
    const centerZ = bounds ? (bounds.minZ + bounds.maxZ) * 0.5 : 0
    const points: Array<[number, number, number]> = [[x, y, z]]
    if (symmetryX) points.push([centerX * 2 - x, y, z])
    if (symmetryZ) points.push([x, y, centerZ * 2 - z])
    if (symmetryX && symmetryZ) points.push([centerX * 2 - x, y, centerZ * 2 - z])
    const affected = new Set<string>()
    for (const point of points) this.sculptAt(point[0], point[1], point[2], radius, tool, strength, falloff, flattenTarget, layer, affected, subtract)
    this.emit('height', [...affected])
  }

  applyRampBrushWorld(start: WorldPoint, end: WorldPoint, radius: number, strength: number, falloff: number) {
    const layer = this.getLayer(this.activeLayerId)
    if (!layer || layer.kind !== 'sculpt' || layer.locked || layer.baked || !layer.visible) return
    const affected = new Set<string>()
    this.sculptAt(start.x, start.y, start.z, radius, 'ramp', strength, falloff, start.y, layer, affected, false, { start, end })
    this.emit('height', [...affected])
  }

  private sculptAt(x: number, y: number, z: number, radius: number, tool: SculptTool, strength: number, falloff: number, flattenTarget: number, layer: TerrainLayer, affected: Set<string>, subtract = false, ramp?: { start: WorldPoint; end: WorldPoint }) {
    const resolution = VOLUME_RESOLUTION
    const stepXZ = SDF_CHUNK_SIZE / (resolution - 3)
    const stepY = (VOLUME_MAX_Y - VOLUME_MIN_Y) / (resolution - 3)
    const minWorldX = (ramp ? Math.min(ramp.start.x, ramp.end.x) : x) - radius
    const maxWorldX = (ramp ? Math.max(ramp.start.x, ramp.end.x) : x) + radius
    const minWorldZ = (ramp ? Math.min(ramp.start.z, ramp.end.z) : z) - radius
    const maxWorldZ = (ramp ? Math.max(ramp.start.z, ramp.end.z) : z) + radius
    const minCx = Math.floor(minWorldX / SDF_CHUNK_SIZE)
    const maxCx = Math.floor(maxWorldX / SDF_CHUNK_SIZE)
    const minCz = Math.floor(minWorldZ / SDF_CHUNK_SIZE)
    const maxCz = Math.floor(maxWorldZ / SDF_CHUNK_SIZE)
    const rampX = ramp ? ramp.end.x - ramp.start.x : 0
    const rampZ = ramp ? ramp.end.z - ramp.start.z : 0
    const rampLengthSq = Math.max(0.0001, rampX * rampX + rampZ * rampZ)
    const verticalScale = tool === 'flatten' || tool === 'ramp' ? 0.45 : tool === 'erosion' ? 0.65 : 1

    for (let cz = minCz; cz <= maxCz; cz += 1) {
      for (let cx = minCx; cx <= maxCx; cx += 1) {
        const chunk = this.getVolumeChunk(cx, cz)
        const source = chunk.field.slice()
        const existingResidual = chunk.residuals.get(layer.id)
        const residual = existingResidual ?? new Float32Array(resolution ** 3)
        let changed = false
        for (let iz = 0; iz < resolution; iz += 1) {
          const wz = cz * SDF_CHUNK_SIZE + (iz - 1) * stepXZ
          for (let iy = 0; iy < resolution; iy += 1) {
            const wy = VOLUME_MIN_Y + (iy - 1) * stepY
            for (let ix = 0; ix < resolution; ix += 1) {
              const wx = cx * SDF_CHUNK_SIZE + (ix - 1) * stepXZ
              let planeTarget = flattenTarget
              let distance: number
              if (ramp) {
                const t = clamp(((wx - ramp.start.x) * rampX + (wz - ramp.start.z) * rampZ) / rampLengthSq)
                const centerX = ramp.start.x + rampX * t
                const centerZ = ramp.start.z + rampZ * t
                planeTarget = ramp.start.y + (ramp.end.y - ramp.start.y) * t
                distance = Math.hypot(wx - centerX, wz - centerZ, (wy - planeTarget) * verticalScale)
              } else {
                distance = Math.hypot(wx - x, (wy - y) * verticalScale, wz - z)
              }
              if (distance >= radius) continue
              const index = iz * resolution * resolution + iy * resolution + ix
              const t = clamp(1 - distance / radius)
              const smoothWeight = t * t * (3 - 2 * t)
              const weight = Math.pow(smoothWeight, 0.4 + falloff * 2.1) * clamp(strength, 0.02, 1)
              const current = source[index]
              const at = (dx: number, dy: number, dz: number) => source[
                clamp(iz + dz, 0, resolution - 1) * resolution * resolution +
                clamp(iy + dy, 0, resolution - 1) * resolution + clamp(ix + dx, 0, resolution - 1)
              ]
              let next = current
              if (tool === 'sculpt' || tool === 'raise' || tool === 'lower') {
                const sphere = radius - distance
                const cut = tool === 'lower' || subtract
                const operation = cut ? smoothMin(current, -sphere, radius * 0.16) : smoothMax(current, sphere, radius * 0.18)
                next = current + (operation - current) * Math.max(0.12, weight)
              } else if (tool === 'flatten' || tool === 'ramp') {
                next = current + ((planeTarget - wy) - current) * weight * 0.52
              } else if (tool === 'smooth') {
                const average = (at(-1, 0, 0) + at(1, 0, 0) + at(0, -1, 0) + at(0, 1, 0) + at(0, 0, -1) + at(0, 0, 1) + current * 2) / 8
                next = current + (average - current) * weight * 0.85
              } else if (tool === 'erosion') {
                const average = (at(-1, 0, 0) + at(1, 0, 0) + at(0, -1, 0) + at(0, 1, 0) + at(0, 0, -1) + at(0, 0, 1)) / 6
                const gx = (at(1, 0, 0) - at(-1, 0, 0)) / Math.max(stepXZ * 2, 0.001)
                const gz = (at(0, 0, 1) - at(0, 0, -1)) / Math.max(stepXZ * 2, 0.001)
                const steepness = clamp(Math.hypot(gx, gz) * 0.7)
                next = current + (average - current) * weight * (0.15 + steepness * 1.35)
              } else if (tool === 'noise') {
                const noise = fbm3D(wx * 0.28 + 19, wy * 0.31 - 7, wz * 0.28 + 3, 2) - 0.5
                next = current + noise * radius * 0.38 * weight * (subtract ? -1 : 1)
              }
              if (Math.abs(next - current) < 1e-7) continue
              const previousResidual = residual[index] * layer.opacity
              const withoutLayer = current - previousResidual
              residual[index] = (next - withoutLayer) / Math.max(0.001, layer.opacity)
              chunk.field[index] = next
              changed = true
            }
          }
        }
        if (changed) {
          if (!existingResidual) chunk.residuals.set(layer.id, residual)
          affected.add(chunkKey(cx, cz))
          this.combineVolumeChunk(chunk)
        }
      }
    }
  }

  getLayer(id: string) {
    return [...this.layers, ...this.customLayers].find((layer) => layer.id === id)
  }

  getAllLayers() { return [...this.layers, ...this.customLayers] }

  setActiveLayer(id: string) {
    if (this.getLayer(id)) {
      this.activeLayerId = id
      this.emit('layers')
    }
  }

  updateLayer(id: string, update: Partial<Pick<TerrainLayer, 'visible' | 'opacity' | 'locked' | 'name'>>) {
    const layer = this.getLayer(id)
    if (!layer) return
    Object.assign(layer, update)
    const keys: string[] = []
    for (const [key, chunk] of this.volumeChunks) {
      if (layer.kind === 'sculpt') this.combineVolumeChunk(chunk)
      else if (!chunk.baked) this.regenerateVolumeBase(chunk)
      keys.push(key)
    }
    this.emit('layers', keys)
  }

  addSculptLayer() {
    const layer: TerrainLayer = {
      id: `sdf-sculpt-${Date.now()}`,
      name: `SDF sculpt ${String(this.customLayers.length + 2).padStart(2, '0')}`,
      kind: 'sculpt', visible: true, opacity: 1, locked: false, baked: false,
    }
    this.customLayers.push(layer)
    this.activeLayerId = layer.id
    this.emit('layers')
    return layer
  }

  bakeVisibleLayers() {
    for (const chunk of this.volumeChunks.values()) {
      chunk.base = chunk.field.slice()
      for (const [layerId] of chunk.residuals) {
        if (this.getLayer(layerId)?.visible) chunk.residuals.delete(layerId)
      }
      chunk.baked = true
      this.combineVolumeChunk(chunk)
    }
    const base = this.getLayer('base')
    if (base) base.baked = true
    for (const layer of this.getAllLayers()) if (layer.kind === 'procedural') layer.baked = true
    this.emit('all', [...this.volumeChunks.keys()])
  }

  reset() {
    this.shapeChunks.clear()
    this.volumeChunks.clear()
    this.blank = false
    this.resetLayers()
    this.undoStack = []
    this.redoStack = []
    this.emit('all')
  }

  createBlank() {
    this.shapeChunks.clear()
    this.volumeChunks.clear()
    this.blank = true
    this.resetLayers()
    this.undoStack = []
    this.redoStack = []
    this.emit('all')
  }

  private snapshot(): WorldSnapshot {
    return {
      shapeChunks: [...this.shapeChunks.values()].map(cloneShapeChunk),
      volumeChunks: [...this.volumeChunks.values()].map(cloneVolumeChunk),
      layers: this.getAllLayers().map((layer) => ({ ...layer })),
      blank: this.blank,
    }
  }

  private restore(snapshot: WorldSnapshot) {
    this.shapeChunks = new Map(snapshot.shapeChunks.map((chunk) => [chunkKey(chunk.cx, chunk.cz), cloneShapeChunk(chunk)]))
    this.volumeChunks = new Map(snapshot.volumeChunks.map((chunk) => [chunkKey(chunk.cx, chunk.cz), cloneVolumeChunk(chunk)]))
    this.layers = snapshot.layers.filter((layer) => ['base', 'landforms', 'detail', 'sculpt'].includes(layer.id)).map((layer) => ({ ...layer }))
    this.customLayers = snapshot.layers.filter((layer) => !['base', 'landforms', 'detail', 'sculpt'].includes(layer.id)).map((layer) => ({ ...layer }))
    this.blank = snapshot.blank
    if (!this.getLayer(this.activeLayerId)) this.activeLayerId = 'sculpt'
    this.emit('all', snapshot.keys ?? [...this.volumeChunks.keys()])
  }

  beginStroke() { if (!this.strokeSnapshot) this.strokeSnapshot = this.snapshot() }

  endStroke(label = 'SDF edit') {
    if (!this.strokeSnapshot) return
    const previousChunks = new Map(this.strokeSnapshot.volumeChunks.map((chunk) => [chunkKey(chunk.cx, chunk.cz), chunk]))
    const changedKeys: string[] = []
    for (const [key, chunk] of this.volumeChunks) {
      const previous = previousChunks.get(key)
      if (!previous) {
        changedKeys.push(key)
        continue
      }
      let changed = false
      for (let i = 0; i < chunk.field.length; i += 1) {
        if (Math.abs(chunk.field[i] - previous.field[i]) > 1e-6) {
          changed = true
          break
        }
      }
      if (changed) changedKeys.push(key)
    }
    this.strokeSnapshot.keys = changedKeys
    this.undoStack.push(this.strokeSnapshot)
    if (this.undoStack.length > 12) this.undoStack.shift()
    this.redoStack = []
    this.strokeSnapshot = null
    this.emit('layers')
    void label
  }

  undo() {
    const previous = this.undoStack.pop()
    if (!previous) return false
    const redoSnapshot = this.snapshot()
    redoSnapshot.keys = previous.keys
    this.redoStack.push(redoSnapshot)
    this.restore(previous)
    return true
  }

  redo() {
    const next = this.redoStack.pop()
    if (!next) return false
    const undoSnapshot = this.snapshot()
    undoSnapshot.keys = next.keys
    this.undoStack.push(undoSnapshot)
    this.restore(next)
    return true
  }

  get canUndo() { return this.undoStack.length > 0 }
  get canRedo() { return this.redoStack.length > 0 }

  serialize(name: string) {
    const layerSettings: SerializedWorldV2['layerSettings'] = {}
    for (const layer of this.layers) layerSettings[layer.id] = { visible: layer.visible, opacity: layer.opacity, locked: layer.locked, baked: layer.baked }
    const data: SerializedWorldV2 = {
      version: 2,
      name,
      blank: this.blank,
      chunkSize: SDF_CHUNK_SIZE,
      shapeResolution: SHAPE_CHUNK_RESOLUTION,
      volumeResolution: VOLUME_RESOLUTION,
      minY: VOLUME_MIN_Y,
      maxY: VOLUME_MAX_Y,
      shapeChunks: [...this.shapeChunks.values()].filter((chunk) => chunk.modified).map((chunk) => ({ cx: chunk.cx, cz: chunk.cz, data: toBase64(chunk.data) })),
      volumeChunks: [...this.volumeChunks.values()].filter((chunk) => chunk.residuals.size > 0 || chunk.baked).map((chunk) => ({
        cx: chunk.cx,
        cz: chunk.cz,
        baked: chunk.baked,
        base: chunk.baked ? toBase64(chunk.base) : undefined,
        residuals: Object.fromEntries([...chunk.residuals].map(([id, residual]) => [id, toBase64(residual)])),
      })),
      customLayers: this.customLayers.map(({ id, name, visible, opacity, locked, baked }) => ({ id, name, visible, opacity, locked, baked })),
      layerSettings,
    }
    return JSON.stringify(data)
  }

  serializeVolume() {
    const data = {
      format: 'frontier-sparse-volumetric-sdf',
      version: 2,
      chunkSize: SDF_CHUNK_SIZE,
      resolution: VOLUME_RESOLUTION,
      verticalRange: [VOLUME_MIN_Y, VOLUME_MAX_Y],
      densityConvention: 'positive-solid',
      chunks: [...this.volumeChunks.values()].map((chunk) => ({ cx: chunk.cx, cz: chunk.cz, density: toBase64(chunk.field) })),
    }
    return JSON.stringify(data)
  }

  deserialize(json: string) {
    const data = JSON.parse(json) as SerializedWorldV2
    if (data.version !== 2 || data.chunkSize !== SDF_CHUNK_SIZE || data.volumeResolution !== VOLUME_RESOLUTION) {
      throw new Error('This is not a compatible Frontier sparse-SDF project')
    }
    this.shapeChunks.clear()
    this.volumeChunks.clear()
    this.blank = data.blank
    for (const saved of data.shapeChunks) {
      const field = fromBase64(saved.data)
      if (field.length !== SHAPE_CHUNK_RESOLUTION ** 2) continue
      this.shapeChunks.set(chunkKey(saved.cx, saved.cz), { cx: saved.cx, cz: saved.cz, data: field, modified: true })
    }
    this.customLayers = data.customLayers.map((layer) => ({ ...layer, kind: 'sculpt' }))
    for (const layer of this.layers) Object.assign(layer, data.layerSettings[layer.id] ?? {})
    for (const saved of data.volumeChunks) {
      const chunk = this.getVolumeChunk(saved.cx, saved.cz)
      chunk.baked = saved.baked
      if (saved.baked && saved.base) {
        chunk.base = fromBase64(saved.base)
        if (chunk.base.length !== VOLUME_RESOLUTION ** 3) throw new Error('Invalid baked chunk size')
      }
      for (const [layerId, encoded] of Object.entries(saved.residuals)) {
        const residual = fromBase64(encoded)
        if (residual.length === VOLUME_RESOLUTION ** 3) chunk.residuals.set(layerId, residual)
      }
      this.combineVolumeChunk(chunk)
    }
    this.undoStack = []
    this.redoStack = []
    this.emit('all')
  }
}

export interface IslandInfo {
  id: string
  name: string
  visible: boolean
  locked: boolean
  profile: string
  center?: { x: number; z: number }
}

interface IslandEntry extends IslandInfo {
  center: { x: number; z: number }
  world: SDFIsland
  unsubscribe: () => void
}

interface SerializedIslandWorldV3 {
  version: 3
  name: string
  activeIslandId: string
  islands: Array<IslandInfo & { data: SerializedWorldV2 }>
}

const compositeChunkKey = (islandId: string, cx: number, cz: number) => `${islandId}:${cx},${cz}`

/** Owns independent island fields. Only the ocean combines their continental SDFs. */
export class TerrainWorld {
  private islands = new Map<string, IslandEntry>()
  private listeners = new Set<(change: WorldChange) => void>()
  activeIslandId = 'main'

  constructor() {
    this.restoreDefaults()
  }

  private restoreDefaults() {
    this.addIslandEntry({ id: 'main', name: 'Aurelia', profile: 'main', visible: true, locked: false }, new SDFIsland('main'))
    for (const [profile, definition] of Object.entries(ISLAND_PROFILES)) {
      this.addIslandEntry({ id: profile, name: definition.name, profile, visible: true, locked: false }, new SDFIsland(profile))
    }
    this.activeIslandId = 'main'
  }

  private addIslandEntry(info: IslandInfo, world: SDFIsland) {
    const bounds = world.getContentBounds()
    const entry: IslandEntry = {
      ...info,
      center: info.center ?? { x: bounds ? (bounds.minX + bounds.maxX) / 2 : 0, z: bounds ? (bounds.minZ + bounds.maxZ) / 2 : 0 },
      world,
      unsubscribe: () => {},
    }
    entry.unsubscribe = world.subscribe((change) => {
      this.listeners.forEach((listener) => listener({
        ...change,
        keys: change.keys?.map((key) => {
          const [cx, cz] = key.split(',').map(Number)
          return compositeChunkKey(info.id, cx, cz)
        }),
        islandId: info.id,
      }))
    })
    this.islands.set(info.id, entry)
    return entry
  }

  private clearIslands() {
    for (const entry of this.islands.values()) entry.unsubscribe()
    this.islands.clear()
  }

  private activeEntry() {
    const entry = this.islands.get(this.activeIslandId) ?? this.islands.values().next().value
    if (!entry) throw new Error('No island is available')
    this.activeIslandId = entry.id
    return entry
  }

  get activeIsland(): IslandInfo { return this.activeEntry() }
  get layers() { return this.activeEntry().world.layers }
  get customLayers() { return this.activeEntry().world.customLayers }
  get activeLayerId() { return this.activeEntry().world.activeLayerId }
  set activeLayerId(id: string) { this.activeEntry().world.activeLayerId = id }
  get allocatedShapeChunks() { return [...this.islands.values()].reduce((sum, entry) => sum + entry.world.allocatedShapeChunks, 0) }
  get allocatedVolumeChunks() { return [...this.islands.values()].reduce((sum, entry) => sum + entry.world.allocatedVolumeChunks, 0) }
  get editedVolumeChunks() { return [...this.islands.values()].reduce((sum, entry) => sum + entry.world.editedVolumeChunks, 0) }
  get canUndo() { return this.activeEntry().world.canUndo }
  get canRedo() { return this.activeEntry().world.canRedo }

  subscribe(listener: (change: WorldChange) => void) {
    this.listeners.add(listener)
    return () => this.listeners.delete(listener)
  }

  private emit(type: WorldChange['type'], keys?: string[], islandId?: string) {
    this.listeners.forEach((listener) => listener({ type, keys, islandId }))
  }

  getIslands(): IslandInfo[] {
    return [...this.islands.values()].map(({ id, name, visible, locked, profile, center }) => ({ id, name, visible, locked, profile, center: { ...center } }))
  }

  getIslandCenter(id: string) {
    const entry = this.islands.get(id)
    if (!entry) return null
    const bounds = entry.world.getContentBounds()
    return bounds ? { x: (bounds.minX + bounds.maxX) / 2, z: (bounds.minZ + bounds.maxZ) / 2 } : { ...entry.center }
  }

  getIslandChunkCount(id: string) { return this.islands.get(id)?.world.allocatedVolumeChunks ?? 0 }
  getIslandEditedChunkCount(id: string) { return this.islands.get(id)?.world.editedVolumeChunks ?? 0 }

  getIslandBounds(id: string) {
    return this.islands.get(id)?.world.getContentBounds() ?? null
  }

  islandIntersectsChunk(id: string, cx: number, cz: number) {
    const entry = this.islands.get(id)
    if (!entry?.visible) return false
    const bounds = entry.world.getContentBounds()
    if (!bounds) return false
    const minX = cx * SDF_CHUNK_SIZE
    const minZ = cz * SDF_CHUNK_SIZE
    const maxX = minX + SDF_CHUNK_SIZE
    const maxZ = minZ + SDF_CHUNK_SIZE
    const padding = 2
    return maxX >= bounds.minX - padding && minX <= bounds.maxX + padding && maxZ >= bounds.minZ - padding && minZ <= bounds.maxZ + padding
  }

  setActiveIsland(id: string) {
    const entry = this.islands.get(id)
    if (!entry) return
    this.activeIslandId = id
    this.emit('layers', undefined, id)
  }

  updateIsland(id: string, update: Partial<Pick<IslandInfo, 'name' | 'visible' | 'locked'>>) {
    const entry = this.islands.get(id)
    if (!entry) return
    Object.assign(entry, update)
    const keys = entry.world.volumeKeys.map((key) => {
      const [cx, cz] = key.split(',').map(Number)
      return compositeChunkKey(id, cx, cz)
    })
    this.emit(update.visible === undefined ? 'layers' : 'all', update.visible === undefined ? undefined : keys, id)
  }

  addIsland(center = { x: 0, z: 0 }, name?: string) {
    const id = `island-${Date.now().toString(36)}-${this.islands.size}`
    const info: IslandInfo = { id, name: name ?? `Island ${String(this.islands.size + 1).padStart(2, '0')}`, profile: 'blank', visible: true, locked: false, center: { ...center } }
    const field = new SDFIsland('blank', true)
    this.addIslandEntry(info, field)
    this.activeIslandId = id
    this.emit('layers', undefined, id)
    return info
  }

  sampleShapeWorld(x: number, z: number) {
    let field = -1
    for (const entry of this.islands.values()) {
      if (!entry.visible) continue
      const bounds = entry.world.getContentBounds()
      if (!bounds || x < bounds.minX - SDF_CHUNK_SIZE || x > bounds.maxX + SDF_CHUNK_SIZE || z < bounds.minZ - SDF_CHUNK_SIZE || z > bounds.maxZ + SDF_CHUNK_SIZE) continue
      field = Math.max(field, entry.world.sampleShapeWorld(x, z))
    }
    return field
  }

  sampleShape(nx: number, nz: number) { return this.sampleShapeWorld(nx * 80, nz * 80) }

  applyShapeBrushWorld(x: number, z: number, radius: number, tool: ShapeTool, strength: number, falloff = 0.6, erosion = 0.65) {
    const entry = this.activeEntry()
    if (!entry.visible || entry.locked) return
    entry.world.applyShapeBrushWorld(x, z, radius, tool, strength, falloff, erosion)
  }

  applyShapeBrush(nx: number, nz: number, radiusMeters: number, tool: ShapeTool, strength: number, falloff = 0.6, erosion = 0.65) {
    this.applyShapeBrushWorld(nx * 80, nz * 80, radiusMeters / METERS_PER_SCENE_UNIT, tool, strength, falloff, erosion)
  }

  finishShapeStroke() { this.activeEntry().world.finishShapeStroke() }
  pruneShapeChunks(centerX: number, centerZ: number, radius = 10) { this.activeEntry().world.pruneShapeChunks(centerX, centerZ, radius) }

  getVolumeChunk(cx: number, cz: number, islandId = this.activeIslandId) {
    const entry = this.islands.get(islandId) ?? this.activeEntry()
    return entry.world.getVolumeChunk(cx, cz)
  }

  hasVolumeSurface(cx: number, cz: number, islandId = this.activeIslandId) {
    const entry = this.islands.get(islandId)
    return entry?.visible ? entry.world.hasVolumeSurface(cx, cz) : false
  }

  releaseUnusedVolumeChunks(retain: Set<string>) {
    for (const entry of this.islands.values()) {
      const localRetain = new Set<string>()
      for (const key of retain) {
        const separator = key.indexOf(':')
        if (separator < 0 || key.slice(0, separator) !== entry.id) continue
        localRetain.add(key.slice(separator + 1))
      }
      entry.world.releaseUnusedVolumeChunks(localRetain)
    }
  }

  sampleDensity(x: number, y: number, z: number, islandId = this.activeIslandId) {
    const entry = this.islands.get(islandId) ?? this.activeEntry()
    return entry.world.sampleDensity(x, y, z)
  }

  applySculptBrushWorld(x: number, y: number, z: number, radius: number, tool: SculptTool, strength: number, falloff: number, flattenTarget = 0, symmetryX = false, symmetryZ = false, subtract = false) {
    const entry = this.activeEntry()
    if (!entry.visible || entry.locked) return
    entry.world.applySculptBrushWorld(x, y, z, radius, tool, strength, falloff, flattenTarget, symmetryX, symmetryZ, subtract)
  }

  applyRampBrushWorld(start: WorldPoint, end: WorldPoint, radius: number, strength: number, falloff: number) {
    const entry = this.activeEntry()
    if (!entry.visible || entry.locked) return
    entry.world.applyRampBrushWorld(start, end, radius, strength, falloff)
  }

  getLayer(id: string) { return this.activeEntry().world.getLayer(id) }
  getAllLayers() { return this.activeEntry().world.getAllLayers() }
  setActiveLayer(id: string) { this.activeEntry().world.setActiveLayer(id) }
  updateLayer(id: string, update: Partial<Pick<TerrainLayer, 'visible' | 'opacity' | 'locked' | 'name'>>) { this.activeEntry().world.updateLayer(id, update) }
  addSculptLayer() { return this.activeEntry().world.addSculptLayer() }
  bakeVisibleLayers() { this.activeEntry().world.bakeVisibleLayers() }
  beginStroke() { this.activeEntry().world.beginStroke() }
  endStroke(label = 'SDF edit') { this.activeEntry().world.endStroke(label) }
  undo() { return this.activeEntry().world.undo() }
  redo() { return this.activeEntry().world.redo() }

  reset() {
    this.clearIslands()
    this.restoreDefaults()
    this.emit('all')
  }

  createBlank() {
    this.clearIslands()
    const info: IslandInfo = { id: 'island-01', name: 'Island 01', profile: 'blank', visible: true, locked: false }
    this.addIslandEntry(info, new SDFIsland('blank', true))
    this.activeIslandId = info.id
    this.emit('all')
  }

  serialize(name: string) {
    const data: SerializedIslandWorldV3 = {
      version: 3,
      name,
      activeIslandId: this.activeIslandId,
      islands: [...this.islands.values()].map((entry) => ({
        id: entry.id,
        name: entry.name,
        visible: entry.visible,
        locked: entry.locked,
        profile: entry.profile,
        center: { ...entry.center },
        data: JSON.parse(entry.world.serialize(entry.name)) as SerializedWorldV2,
      })),
    }
    return JSON.stringify(data)
  }

  serializeVolume() {
    const chunks: Array<Record<string, unknown>> = []
    for (const entry of this.islands.values()) {
      const volume = JSON.parse(entry.world.serializeVolume()) as { chunks: Array<Record<string, unknown>> }
      for (const chunk of volume.chunks) chunks.push({ ...chunk, islandId: entry.id, islandName: entry.name, visible: entry.visible })
    }
    return JSON.stringify({
      format: 'frontier-independent-islands-sdf',
      version: 3,
      chunkSize: SDF_CHUNK_SIZE,
      resolution: VOLUME_RESOLUTION,
      verticalRange: [VOLUME_MIN_Y, VOLUME_MAX_Y],
      densityConvention: 'positive-solid',
      chunks,
    })
  }

  deserialize(json: string) {
    const data = JSON.parse(json) as SerializedIslandWorldV3 | SerializedWorldV2
    this.clearIslands()
    if (data.version === 2) {
      const field = new SDFIsland('main')
      field.deserialize(json)
      this.addIslandEntry({ id: 'main', name: data.name || 'Aurelia', profile: 'main', visible: true, locked: false }, field)
      this.activeIslandId = 'main'
      this.emit('all')
      return
    }
    if (data.version !== 3 || !Array.isArray(data.islands) || data.islands.length === 0) throw new Error('This is not a compatible independent-island SDF project')
    for (const saved of data.islands) {
      const field = new SDFIsland(saved.profile)
      field.deserialize(JSON.stringify(saved.data))
      this.addIslandEntry({ id: saved.id, name: saved.name, visible: saved.visible, locked: saved.locked, profile: saved.profile, center: saved.center }, field)
    }
    this.activeIslandId = this.islands.has(data.activeIslandId) ? data.activeIslandId : (this.islands.keys().next().value ?? 'main')
    this.emit('all')
  }
}
