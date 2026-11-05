import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import { MarchingCubes } from 'three/addons/objects/MarchingCubes.js'
import { acceleratedRaycast, computeBoundsTree, disposeBoundsTree } from 'three-mesh-bvh'
import type { MeshBVHOptions } from 'three-mesh-bvh'
import {
  METERS_PER_SCENE_UNIT,
  SDF_CHUNK_SIZE,
  STREAM_RADIUS,
  VOLUME_MAX_Y,
  VOLUME_MIN_Y,
  VOLUME_RESOLUTION,
  TerrainWorld,
} from './TerrainWorld'
import type { SculptTool } from './TerrainWorld'

THREE.BufferGeometry.prototype.computeBoundsTree = computeBoundsTree
THREE.BufferGeometry.prototype.disposeBoundsTree = disposeBoundsTree
THREE.Mesh.prototype.raycast = acceleratedRaycast

export type DisplayMode = 'lit' | 'clay' | 'wireframe'
export type CameraView = 'perspective' | 'top' | 'front'

export interface ViewportSettings {
  stage: 'shape' | 'sculpt'
  tool: SculptTool
  brushSize: number
  strength: number
  falloff: number
  symmetryX: boolean
  symmetryZ: boolean
  displayMode: DisplayMode
  showGrid: boolean
  oceanVisible: boolean
  seaLevel: number
}

export interface ViewportStats {
  fps: number
  visibleTiles: number
  triangles: number
  frameMs: number
  renderMs: number
  drawCalls: number
  renderedTriangles: number
  geometries: number
  renderScale: number
}

export interface CursorLocation {
  x: number
  z: number
  height: number
}

interface EngineCallbacks {
  onStats?: (stats: ViewportStats) => void
  onCursor?: (cursor: CursorLocation | null) => void
  onStrokeEnd?: () => void
  onViewChange?: (center: { x: number; z: number }) => void
}

const oceanVertex = /* glsl */`
  uniform float uTime;
  uniform float uCoverage;
  uniform vec2 uOrigin;
  uniform sampler2D uShapeMap;
  varying vec3 vWorldPosition;
  void main() {
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    vec2 fieldUv = (worldPosition.xz + uOrigin) / uCoverage + 0.5;
    float continentalSdf = -1.0;
    if (fieldUv.x > 0.0 && fieldUv.x < 1.0 && fieldUv.y > 0.0 && fieldUv.y < 1.0) {
      continentalSdf = texture2D(uShapeMap, fieldUv).r;
    }
    float offshore = 1.0 - smoothstep(-0.22, -0.012, continentalSdf);
    float wave = sin(worldPosition.x * 0.105 + uTime * 0.64) * 0.034;
    wave += sin(worldPosition.z * 0.147 - uTime * 0.48) * 0.026;
    wave += sin((worldPosition.x + worldPosition.z) * 0.24 + uTime * 0.82) * 0.012;
    worldPosition.y += wave * mix(0.06, 1.0, offshore);
    vWorldPosition = worldPosition.xyz;
    gl_Position = projectionMatrix * viewMatrix * worldPosition;
  }
`

const oceanFragment = /* glsl */`
  uniform float uTime;
  uniform float uSeaLevel;
  uniform float uCoverage;
  uniform vec2 uOrigin;
  uniform sampler2D uShapeMap;
  uniform sampler2D uNoiseMap;
  uniform vec3 uSunDirection;
  uniform vec3 uDeepColor;
  uniform vec3 uShallowColor;
  uniform vec3 uSkyColor;
  uniform vec3 uFogColor;
  varying vec3 vWorldPosition;

  void main() {
    vec2 p = vWorldPosition.xz;
    float t = uTime;
    // One texture fetch provides two decorrelated wave channels; one trig pair
    // carries the broad swell. Fewer per-pixel transcendental calls matters far
    // more than extra octaves here, because the ocean covers the whole screen.
    vec3 toCamera = cameraPosition - vWorldPosition;
    float distanceToCamera = length(toCamera);
    vec3 viewDirection = toCamera / max(distanceToCamera, 0.0001);
    // Fade fine detail with distance: at grazing angles the repeating detail
    // texture and the coastline contours alias into visible banding rings.
    float detail = 1.0 - smoothstep(70.0, 300.0, distanceToCamera);
    // World-space sine waves compress to sub-pixel wavelengths at grazing
    // angles, producing moire rings; flatten them out with distance.
    float waveFade = 1.0 - smoothstep(110.0, 460.0, distanceToCamera);
    vec2 microNoise = texture2D(uNoiseMap, p * 0.006 + vec2(t * 0.0005, -t * 0.0004)).rg;
    float swell = sin(p.x * 0.36 + p.y * 0.21 + t * 0.92);
    float crossSwell = sin(p.y * 0.31 - p.x * 0.17 - t * 0.78);
    float nx = swell * 0.058 * waveFade + (microNoise.x - 0.5) * 0.075 * detail;
    float nz = crossSwell * 0.062 * waveFade + (microNoise.y - 0.5) * 0.07 * detail;
    vec3 normal = normalize(vec3(nx, 1.0, nz));
    vec3 sunDirection = uSunDirection;

    vec2 fieldUv = (p + uOrigin) / uCoverage + 0.5;
    float continentalSdf = -1.0;
    if (fieldUv.x > 0.0 && fieldUv.x < 1.0 && fieldUv.y > 0.0 && fieldUv.y < 1.0) {
      continentalSdf = texture2D(uShapeMap, fieldUv).r;
    }
    float fieldEdge = smoothstep(0.0, 0.09, fieldUv.x) * smoothstep(0.0, 0.09, fieldUv.y);
    fieldEdge *= smoothstep(0.0, 0.09, 1.0 - fieldUv.x) * smoothstep(0.0, 0.09, 1.0 - fieldUv.y);
    float shelfHeight = max(-10.0, continentalSdf * 33.0);
    float depth = max(0.0, uSeaLevel - shelfHeight);
    float shallow = (1.0 - smoothstep(0.3, 8.0, depth)) * fieldEdge;
    float facing = 1.0 - max(dot(normal, viewDirection), 0.0);
    float fresnel = facing * facing * facing;
    fresnel *= 1.0 + facing * 0.2;
    vec3 waterColor = mix(uDeepColor, uShallowColor, shallow * 0.88);
    waterColor = mix(waterColor, uSkyColor, fresnel * 0.72 + 0.08);
    vec3 reflectedSun = reflect(-sunDirection, normal);
    // Squared/powered-by-multiplication specular instead of pow(x, 420.0):
    // same glint lobe, a fraction of the cost in a software rasterizer.
    float glintDot = max(dot(reflectedSun, viewDirection), 0.0);
    float glintBase = glintDot * glintDot;
    float glint8 = glintBase * glintBase;
    glint8 *= glint8;
    float glint64 = glint8 * glint8;
    glint64 *= glint64;
    glint64 *= glint64;
    float sunGlint = glint64 * glint64 * glint64 * 0.38;
    sunGlint += glint8 * glint8 * 0.012;
    vec3 effectNoise = texture2D(uNoiseMap, p * 0.009 + vec2(t * 0.0003, -t * 0.0002)).rgb;
    float sparkle = smoothstep(0.72, 0.96, effectNoise.r) * detail;
    waterColor += vec3(1.0, 0.87, 0.63) * sunGlint * (0.04 + sparkle * 0.96);

    float foamNoise = effectNoise.g;
    float coastDistance = abs(shelfHeight - uSeaLevel + 0.025);
    float shoreline = 1.0 - smoothstep(0.025, 0.23 + foamNoise * 0.09, coastDistance);
    float foamBreak = smoothstep(0.28, 0.7, foamNoise + swell * 0.12);
    float outerFoam = (1.0 - smoothstep(0.035, 0.17, abs(shelfHeight - uSeaLevel + 0.42 + swell * 0.055))) * 0.12;
    float foam = clamp(shoreline * (0.42 + foamBreak * 0.45) + outerFoam, 0.0, 0.76) * fieldEdge * detail;
    waterColor = mix(waterColor, vec3(0.68, 0.78, 0.73), foam);
    float fogAmount = smoothstep(170.0, 490.0, distanceToCamera);
    waterColor = mix(waterColor, uFogColor, fogAmount);
    gl_FragColor = vec4(waterColor, 1.0);
    #include <tonemapping_fragment>
    #include <colorspace_fragment>
  }
`

function createOceanNoiseTexture() {
  const size = 256
  const data = new Uint8Array(size * size * 4)
  let seed = 0x2f6e2b1
  for (let i = 0; i < size * size; i += 1) {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0
    data[i * 4] = seed >>> 24
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0
    data[i * 4 + 1] = seed >>> 24
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0
    data[i * 4 + 2] = seed >>> 24
    data[i * 4 + 3] = 255
  }
  const texture = new THREE.DataTexture(data, size, size, THREE.RGBAFormat)
  texture.wrapS = texture.wrapT = THREE.RepeatWrapping
  texture.minFilter = THREE.LinearMipmapLinearFilter
  texture.magFilter = THREE.LinearFilter
  texture.generateMipmaps = true
  texture.needsUpdate = true
  return texture
}

function createSky(sun: THREE.Vector3) {
  const material = new THREE.ShaderMaterial({
    side: THREE.BackSide,
    depthWrite: false,
    depthTest: false,
    uniforms: {
      uSunDirection: { value: sun },
      uHorizon: { value: new THREE.Color('#a8bab6') },
      uZenith: { value: new THREE.Color('#537f96') },
    },
    vertexShader: /* glsl */`
      varying vec3 vDirection;
      void main() {
        vDirection = position;
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }
    `,
    fragmentShader: /* glsl */`
      uniform vec3 uSunDirection;
      uniform vec3 uHorizon;
      uniform vec3 uZenith;
      varying vec3 vDirection;
      void main() {
        vec3 direction = normalize(vDirection);
        float skyBlend = smoothstep(-0.12, 0.72, direction.y);
        vec3 color = mix(uHorizon, uZenith, skyBlend);
        float sunDot = max(dot(direction, normalize(uSunDirection)), 0.0);
        color += vec3(1.0, 0.86, 0.62) * (pow(sunDot, 180.0) * 0.55 + pow(sunDot, 14.0) * 0.08);
        gl_FragColor = vec4(color, 1.0);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }
    `,
  })
  const sky = new THREE.Mesh(new THREE.SphereGeometry(1600, 32, 16), material)
  sky.renderOrder = -1000
  return sky
}

const palette = {
  seabed: new THREE.Color('#344d49'),
  wetSand: new THREE.Color('#807d63'),
  sand: new THREE.Color('#999473'),
  grass: new THREE.Color('#526e49'),
  highGrass: new THREE.Color('#435c40'),
  rock: new THREE.Color('#7b7f77'),
  darkRock: new THREE.Color('#515b54'),
  snow: new THREE.Color('#d4d8cf'),
}
const clamp01 = (n: number) => Math.max(0, Math.min(1, n))
const blend = (a: number, b: number, t: number) => a + (b - a) * clamp01(t)

function terrainColor(height: number, slope: number, x: number, z: number, target: Float32Array, offset: number) {
  let color = palette.grass
  let next = palette.highGrass
  let mixAmount = clamp01((height - 3) / 12)
  if (height < -0.2) {
    color = palette.seabed
    next = palette.wetSand
    mixAmount = clamp01((height + 4) / 4)
  } else if (height < 0.72) {
    color = palette.wetSand
    next = palette.sand
    mixAmount = clamp01((height + 0.12) / 0.84)
  } else if (height < 2.15) {
    color = palette.sand
    next = palette.grass
    mixAmount = clamp01((height - 0.72) / 1.43)
  }
  let r = blend(color.r, next.r, mixAmount)
  let g = blend(color.g, next.g, mixAmount)
  let b = blend(color.b, next.b, mixAmount)
  const rockAmount = clamp01((slope - 0.38) / 0.72) * clamp01((height - 0.8) / 2)
  const rock = height > 15 ? palette.rock : palette.darkRock
  r = blend(r, rock.r, rockAmount)
  g = blend(g, rock.g, rockAmount)
  b = blend(b, rock.b, rockAmount)
  const highRock = clamp01((height - 18) / 11) * 0.7
  r = blend(r, palette.rock.r, highRock)
  g = blend(g, palette.rock.g, highRock)
  b = blend(b, palette.rock.b, highRock)
  const snowAmount = clamp01((height - 24 - slope * 1.2) / 6) * (1 - clamp01((slope - 2.0) / 1.5))
  r = blend(r, palette.snow.r, snowAmount)
  g = blend(g, palette.snow.g, snowAmount)
  b = blend(b, palette.snow.b, snowAmount)
  const variation = Math.sin(x * 1.47 + z * 0.83) * Math.sin(z * 1.37 - x * 0.41) * 0.035
  target[offset] = clamp01(r + variation)
  target[offset + 1] = clamp01(g + variation)
  target[offset + 2] = clamp01(b + variation)
}

interface ChunkMesh {
  cx: number
  cz: number
  islandId: string
  mesh: MarchingCubes
  bvhCount: number
}

export class ViewportEngine {
  readonly renderer: THREE.WebGLRenderer
  readonly scene = new THREE.Scene()
  readonly camera: THREE.PerspectiveCamera
  private controls: OrbitControls
  private container: HTMLElement
  private world: TerrainWorld
  private settings: ViewportSettings
  private callbacks: EngineCallbacks
  private terrainGroup = new THREE.Group()
  private chunks = new Map<string, ChunkMesh>()
  private terrainMaterial: THREE.MeshLambertMaterial
  private clayMaterial: THREE.MeshLambertMaterial
  private wireMaterial: THREE.MeshBasicMaterial
  private shapeTexture: THREE.DataTexture
  private oceanNoiseTexture: THREE.DataTexture
  private shapeData = new Float32Array(257 * 257)
  private ocean: THREE.Mesh<THREE.PlaneGeometry, THREE.ShaderMaterial>
  private gridHelper: THREE.GridHelper
  private brushVolume: THREE.LineSegments
  private rampGuide: THREE.Line<THREE.BufferGeometry, THREE.LineBasicMaterial>
  private rampStart: THREE.Vector3 | null = null
  private rampEnd: THREE.Vector3 | null = null
  private raycaster = new THREE.Raycaster()
  private pointer = new THREE.Vector2()
  private animationFrame = 0
  private resizeObserver: ResizeObserver
  private rebuildFrame = 0
  private rebuildAll = false
  private pendingKeys = new Set<string>()
  private sculpting = false
  private flattenTarget = 0
  private hoverPoint: THREE.Vector3 | null = null
  private unsubscribe: () => void
  private disposed = false
  private originX = 0
  private originZ = 0
  private centerCx = Number.NaN
  private centerCz = Number.NaN
  private streamDirty = true
  private frameCount = 0
  private lastStats = performance.now()
  private lastRenderMs = 0
  private preferredRenderScale = Math.min(window.devicePixelRatio, 1.25)
  private renderScale = this.preferredRenderScale
  private slowFrameSamples = 0
  private fastFrameSamples = 0

  constructor(container: HTMLElement, world: TerrainWorld, settings: ViewportSettings, callbacks: EngineCallbacks = {}) {
    this.container = container
    this.world = world
    this.settings = settings
    this.callbacks = callbacks
    this.raycaster.firstHitOnly = true
    this.renderer = new THREE.WebGLRenderer({ antialias: false, preserveDrawingBuffer: false, powerPreference: 'high-performance' })
    this.renderer.setPixelRatio(this.renderScale)
    this.renderer.setSize(Math.max(1, container.clientWidth), Math.max(1, container.clientHeight))
    this.renderer.outputColorSpace = THREE.SRGBColorSpace
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping
    this.renderer.toneMappingExposure = 1.03
    this.renderer.shadowMap.enabled = true
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap
    this.renderer.shadowMap.autoUpdate = false
    this.renderer.shadowMap.needsUpdate = true
    this.renderer.domElement.className = 'terrain-webgl-canvas'
    this.container.appendChild(this.renderer.domElement)

    this.camera = new THREE.PerspectiveCamera(43, container.clientWidth / Math.max(1, container.clientHeight), 0.05, 3000)
    this.camera.position.set(108, 87, 119)
    this.controls = new OrbitControls(this.camera, this.renderer.domElement)
    this.controls.target.set(0, 4, 0)
    this.controls.enableDamping = true
    this.controls.dampingFactor = 0.075
    this.controls.minDistance = 0.6
    this.controls.maxDistance = 1200
    this.controls.maxPolarAngle = Math.PI * 0.485
    this.controls.panSpeed = 0.85
    this.controls.rotateSpeed = 0.55
    this.controls.zoomSpeed = 0.8
    this.controls.update()
    this.controls.addEventListener('change', () => { this.streamDirty = true })

    this.scene.fog = new THREE.Fog('#8fa9aa', 185, 500)
    const sun = new THREE.Vector3(-0.48, 0.68, -0.35).normalize()
    this.scene.add(createSky(sun))
    this.scene.add(new THREE.HemisphereLight('#d9e6df', '#354038', 1.65))
    const sunlight = new THREE.DirectionalLight('#fff1d2', 2.75)
    sunlight.position.copy(sun).multiplyScalar(180)
    sunlight.castShadow = true
    sunlight.shadow.mapSize.set(1024, 1024)
    sunlight.shadow.camera.left = sunlight.shadow.camera.bottom = -105
    sunlight.shadow.camera.right = sunlight.shadow.camera.top = 105
    sunlight.shadow.camera.near = 10
    sunlight.shadow.camera.far = 430
    sunlight.shadow.bias = -0.00025
    sunlight.shadow.normalBias = 0.05
    this.scene.add(sunlight)
    const fill = new THREE.DirectionalLight('#9ab6bd', 0.48)
    fill.position.set(80, 45, -90)
    this.scene.add(fill)

    this.terrainMaterial = new THREE.MeshLambertMaterial({ vertexColors: true })
    this.clayMaterial = new THREE.MeshLambertMaterial({ color: '#aaa99e' })
    this.wireMaterial = new THREE.MeshBasicMaterial({ color: '#92cbb0', wireframe: true, transparent: true, opacity: 0.42 })
    this.scene.add(this.terrainGroup)

    this.shapeTexture = new THREE.DataTexture(this.shapeData, 257, 257, THREE.RedFormat, THREE.FloatType)
    this.shapeTexture.minFilter = THREE.LinearFilter
    this.shapeTexture.magFilter = THREE.LinearFilter
    this.shapeTexture.needsUpdate = true
    this.oceanNoiseTexture = createOceanNoiseTexture()
    this.ocean = new THREE.Mesh(
      new THREE.PlaneGeometry(3000, 3000, 64, 64).rotateX(-Math.PI / 2),
      new THREE.ShaderMaterial({
        depthWrite: true,
        depthTest: true,
        side: THREE.FrontSide,
        vertexShader: oceanVertex,
        fragmentShader: oceanFragment,
        uniforms: {
          uTime: { value: 0 },
          uSeaLevel: { value: 0 },
          uCoverage: { value: SDF_CHUNK_SIZE * (STREAM_RADIUS * 2 + 3) },
          uOrigin: { value: new THREE.Vector2() },
          uShapeMap: { value: this.shapeTexture },
          uNoiseMap: { value: this.oceanNoiseTexture },
          uSunDirection: { value: sun },
          uDeepColor: { value: new THREE.Color('#062d3a') },
          uShallowColor: { value: new THREE.Color('#17827f') },
          uSkyColor: { value: new THREE.Color('#82a5b0') },
          uFogColor: { value: new THREE.Color('#8fa9aa') },
        },
      }),
    )
    this.scene.add(this.ocean)
    this.gridHelper = new THREE.GridHelper(SDF_CHUNK_SIZE * (STREAM_RADIUS * 2 + 1), STREAM_RADIUS * 2 + 1, '#bad49f', '#758c7c')
    const gridMaterials = Array.isArray(this.gridHelper.material) ? this.gridHelper.material : [this.gridHelper.material]
    gridMaterials.forEach((material) => { material.transparent = true; material.opacity = 0.18; material.depthWrite = false })
    this.scene.add(this.gridHelper)
    this.brushVolume = this.createBrushVolume()
    this.scene.add(this.brushVolume)
    this.rampGuide = new THREE.Line(
      new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(), new THREE.Vector3()]),
      new THREE.LineBasicMaterial({ color: '#d3f89b', transparent: true, opacity: 0.85, depthTest: true, depthWrite: false }),
    )
    this.rampGuide.visible = false
    this.rampGuide.renderOrder = 21
    this.scene.add(this.rampGuide)
    this.updateShapeTexture()
    this.updateStreaming()
    this.applySettings(settings)

    this.unsubscribe = this.world.subscribe((change) => {
      if (change.keys) change.keys.forEach((key) => this.pendingKeys.add(key))
      else if (change.type === 'all') this.rebuildAll = true
      if (change.type === 'shape' || change.type === 'all') this.updateShapeTexture()
      if (change.type === 'shape' || (change.islandId && change.type !== 'height') || change.keys?.some((key) => !this.chunks.has(key))) {
        this.streamDirty = true
        this.centerCx = Number.NaN
      }
      if (change.type !== 'layers' || change.keys) this.scheduleRebuild()
    })
    this.resizeObserver = new ResizeObserver(() => this.resize())
    this.resizeObserver.observe(this.container)
    this.renderer.domElement.addEventListener('pointerdown', this.onPointerDown, { capture: true })
    this.renderer.domElement.addEventListener('pointermove', this.onPointerMove, { capture: true })
    this.renderer.domElement.addEventListener('pointerup', this.onPointerUp, { capture: true })
    this.renderer.domElement.addEventListener('pointercancel', this.onPointerUp, { capture: true })
    this.renderer.domElement.addEventListener('pointerleave', this.onPointerLeave)
    this.renderer.domElement.addEventListener('contextmenu', this.onContextMenu)
    this.animate()
  }

  private key(islandId: string, cx: number, cz: number) { return `${islandId}:${cx},${cz}` }

  private activeMaterial() {
    return this.settings.displayMode === 'lit' ? this.terrainMaterial : this.settings.displayMode === 'clay' ? this.clayMaterial : this.wireMaterial
  }

  private createBrushVolume() {
    const geometry = new THREE.WireframeGeometry(new THREE.SphereGeometry(1, 16, 10))
    const volume = new THREE.LineSegments(geometry, new THREE.LineBasicMaterial({ color: '#d3f89b', transparent: true, opacity: 0.5, depthTest: true, depthWrite: false }))
    volume.visible = false
    volume.frustumCulled = false
    volume.renderOrder = 20
    return volume
  }

  private positionChunk(chunk: ChunkMesh) {
    const resolution = VOLUME_RESOLUTION
    const scaleXZ = SDF_CHUNK_SIZE * 0.5 * resolution / (resolution - 3)
    const scaleY = (VOLUME_MAX_Y - VOLUME_MIN_Y) * 0.5 * resolution / (resolution - 3)
    chunk.mesh.scale.set(scaleXZ, scaleY, scaleXZ)
    chunk.mesh.position.set(
      chunk.cx * SDF_CHUNK_SIZE + scaleXZ * (1 - 2 / resolution) - this.originX,
      VOLUME_MIN_Y + scaleY * (1 - 2 / resolution),
      chunk.cz * SDF_CHUNK_SIZE + scaleXZ * (1 - 2 / resolution) - this.originZ,
    )
    chunk.mesh.updateMatrixWorld()
  }

  private ensureChunk(cx: number, cz: number, islandId: string) {
    const key = this.key(islandId, cx, cz)
    if (this.chunks.has(key)) return this.chunks.get(key)!
    const mesh = new MarchingCubes(VOLUME_RESOLUTION, this.activeMaterial(), true, true, 4096)
    mesh.isolation = 0
    mesh.castShadow = true
    mesh.receiveShadow = true
    mesh.userData.islandId = islandId
    mesh.geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(-1 / VOLUME_RESOLUTION, -1 / VOLUME_RESOLUTION, -1 / VOLUME_RESOLUTION), 1.8)
    const chunk = { cx, cz, islandId, mesh, bvhCount: -1 }
    this.chunks.set(key, chunk)
    this.terrainGroup.add(mesh)
    this.updateChunk(chunk)
    return chunk
  }

  private updateChunk(chunk: ChunkMesh) {
    const volume = this.world.getVolumeChunk(chunk.cx, chunk.cz, chunk.islandId)
    chunk.mesh.field = volume.field
    chunk.mesh.normal_cache.fill(0)
    chunk.mesh.update()
    if (chunk.mesh.count === 0) {
      chunk.mesh.geometry.disposeBoundsTree()
      chunk.bvhCount = 0
    } else if (chunk.mesh.geometry.boundsTree && chunk.bvhCount === chunk.mesh.count) {
      chunk.mesh.geometry.boundsTree.refit()
    } else {
      chunk.mesh.geometry.disposeBoundsTree()
      chunk.mesh.geometry.computeBoundsTree({ indirect: true, maxLeafTris: 12 } as MeshBVHOptions & { indirect: true })
      chunk.bvhCount = chunk.mesh.count
    }
    chunk.mesh.material = this.activeMaterial()
    this.positionChunk(chunk)
    const positions = chunk.mesh.positionArray
    const normals = chunk.mesh.normalArray
    const colors = chunk.mesh.colorArray
    const scale = chunk.mesh.scale
    for (let i = 0; i < chunk.mesh.count; i += 1) {
      const offset = i * 3
      let nx = normals[offset] / scale.x
      let ny = normals[offset + 1] / scale.y
      let nz = normals[offset + 2] / scale.z
      const length = Math.max(0.0001, Math.hypot(nx, ny, nz))
      nx /= length
      ny /= length
      nz /= length
      const worldY = chunk.mesh.position.y + positions[offset + 1] * scale.y
      const worldX = chunk.mesh.position.x + positions[offset] * scale.x + this.originX
      const worldZ = chunk.mesh.position.z + positions[offset + 2] * scale.z + this.originZ
      const slope = Math.hypot(nx, nz) / Math.max(0.08, Math.abs(ny))
      terrainColor(worldY, slope, worldX, worldZ, colors, offset)
    }
    chunk.mesh.geometry.attributes.color.needsUpdate = true
    this.renderer.shadowMap.needsUpdate = true
  }

  private removeChunk(key: string) {
    const chunk = this.chunks.get(key)
    if (!chunk) return
    this.terrainGroup.remove(chunk.mesh)
    chunk.mesh.geometry.disposeBoundsTree()
    chunk.mesh.geometry.dispose()
    this.chunks.delete(key)
    this.renderer.shadowMap.needsUpdate = true
  }

  private updateStreaming() {
    this.streamDirty = false
    const absoluteX = this.controls.target.x + this.originX
    const absoluteZ = this.controls.target.z + this.originZ
    const cx = Math.floor(absoluteX / SDF_CHUNK_SIZE)
    const cz = Math.floor(absoluteZ / SDF_CHUNK_SIZE)
    if (cx === this.centerCx && cz === this.centerCz) return
    this.centerCx = cx
    this.centerCz = cz
    const retain = new Set<string>()
    for (const island of this.world.getIslands()) {
      if (!island.visible) continue
      for (let z = cz - STREAM_RADIUS; z <= cz + STREAM_RADIUS; z += 1) {
        for (let x = cx - STREAM_RADIUS; x <= cx + STREAM_RADIUS; x += 1) {
          if (!this.world.islandIntersectsChunk(island.id, x, z)) continue
          if (!this.world.hasVolumeSurface(x, z, island.id)) continue
          const key = this.key(island.id, x, z)
          retain.add(key)
          this.ensureChunk(x, z, island.id)
        }
      }
    }
    for (const key of this.chunks.keys()) if (!retain.has(key)) this.removeChunk(key)
    this.world.releaseUnusedVolumeChunks(retain)
    this.callbacks.onViewChange?.({ x: absoluteX * METERS_PER_SCENE_UNIT, z: absoluteZ * METERS_PER_SCENE_UNIT })
  }

  private recenter() {
    const shiftX = Math.round(this.controls.target.x / SDF_CHUNK_SIZE) * SDF_CHUNK_SIZE
    const shiftZ = Math.round(this.controls.target.z / SDF_CHUNK_SIZE) * SDF_CHUNK_SIZE
    if (shiftX === 0 && shiftZ === 0) return
    this.originX += shiftX
    this.originZ += shiftZ
    this.brushVolume.visible = false
    this.hoverPoint = null
    this.camera.position.x -= shiftX
    this.camera.position.z -= shiftZ
    this.controls.target.x -= shiftX
    this.controls.target.z -= shiftZ
    for (const chunk of this.chunks.values()) this.positionChunk(chunk)
    this.gridHelper.position.x = -this.originX % SDF_CHUNK_SIZE
    this.gridHelper.position.z = -this.originZ % SDF_CHUNK_SIZE
    this.updateShapeTexture()
    this.streamDirty = true
    this.centerCx = Number.NaN
    this.controls.update()
  }

  private updateShapeTexture() {
    const coverage = SDF_CHUNK_SIZE * (STREAM_RADIUS * 2 + 3)
    for (let z = 0; z < 257; z += 1) {
      for (let x = 0; x < 257; x += 1) {
        this.shapeData[z * 257 + x] = this.world.sampleShapeWorld(
          this.originX + (x / 256 - 0.5) * coverage,
          this.originZ + (z / 256 - 0.5) * coverage,
        )
      }
    }
    this.shapeTexture.needsUpdate = true
    if (this.ocean) this.ocean.material.uniforms.uOrigin.value.set(this.originX, this.originZ)
  }

  private scheduleRebuild() {
    if (this.rebuildFrame || this.disposed) return
    this.rebuildFrame = requestAnimationFrame(() => {
      this.rebuildFrame = 0
      if (this.disposed) return
      if (this.rebuildAll) {
        for (const key of this.chunks.keys()) this.pendingKeys.add(key)
        this.rebuildAll = false
      }
      const keys = [...this.pendingKeys].slice(0, 4)
      for (const key of keys) {
        const chunk = this.chunks.get(key)
        if (chunk) this.updateChunk(chunk)
        this.pendingKeys.delete(key)
      }
      if (this.pendingKeys.size > 0) this.scheduleRebuild()
      else if (this.hoverPoint) this.updateBrushVolume(this.hoverPoint)
    })
  }

  private getIntersection(event: PointerEvent) {
    const rect = this.renderer.domElement.getBoundingClientRect()
    this.pointer.set(((event.clientX - rect.left) / rect.width) * 2 - 1, -((event.clientY - rect.top) / rect.height) * 2 + 1)
    this.raycaster.setFromCamera(this.pointer, this.camera)
    const meshes = [...this.chunks.values()].filter((chunk) => chunk.islandId === this.world.activeIslandId).map((chunk) => chunk.mesh)
    return this.raycaster.intersectObjects(meshes, false)[0] ?? null
  }

  private updateBrushVolume(point: THREE.Vector3) {
    const radius = this.settings.brushSize / METERS_PER_SCENE_UNIT
    this.brushVolume.position.copy(point)
    this.brushVolume.scale.setScalar(radius)
    this.brushVolume.visible = this.settings.stage === 'sculpt'
    this.hoverPoint = point.clone()
  }

  private updateRampGuide() {
    if (!this.rampStart || !this.rampEnd) {
      this.rampGuide.visible = false
      return
    }
    const positions = this.rampGuide.geometry.attributes.position as THREE.BufferAttribute
    positions.setXYZ(0, this.rampStart.x, this.rampStart.y + 0.18, this.rampStart.z)
    positions.setXYZ(1, this.rampEnd.x, this.rampEnd.y + 0.18, this.rampEnd.z)
    positions.needsUpdate = true
    this.rampGuide.geometry.computeBoundingSphere()
    this.rampGuide.visible = true
  }

  private sculptAt(point: THREE.Vector3, subtract = false) {
    this.world.applySculptBrushWorld(
      point.x + this.originX,
      point.y,
      point.z + this.originZ,
      this.settings.brushSize / METERS_PER_SCENE_UNIT,
      this.settings.tool,
      this.settings.strength,
      this.settings.falloff,
      this.flattenTarget,
      this.settings.symmetryX,
      this.settings.symmetryZ,
      subtract,
    )
    this.updateBrushVolume(point)
  }

  private onPointerDown = (event: PointerEvent) => {
    if (this.settings.stage !== 'sculpt' || event.button !== 0 || event.altKey || event.ctrlKey || event.metaKey) return
    const hit = this.getIntersection(event)
    if (!hit) return
    event.preventDefault()
    event.stopImmediatePropagation()
    this.renderer.domElement.setPointerCapture(event.pointerId)
    this.sculpting = true
    this.controls.enabled = false
    this.flattenTarget = hit.point.y
    this.world.beginStroke()
    if (this.settings.tool === 'ramp') {
      this.rampStart = hit.point.clone()
      this.rampEnd = hit.point.clone()
      this.updateRampGuide()
    } else {
      this.sculptAt(hit.point, event.shiftKey)
    }
  }

  private onPointerMove = (event: PointerEvent) => {
    if (this.settings.stage !== 'sculpt') return
    if (this.sculpting) {
      event.preventDefault()
      event.stopImmediatePropagation()
    }
    const hit = this.getIntersection(event)
    if (hit) {
      this.updateBrushVolume(hit.point)
      this.callbacks.onCursor?.({
        x: (hit.point.x + this.originX) * METERS_PER_SCENE_UNIT,
        z: (hit.point.z + this.originZ) * METERS_PER_SCENE_UNIT,
        height: hit.point.y * METERS_PER_SCENE_UNIT,
      })
      if (this.sculpting && this.settings.tool === 'ramp' && this.rampStart) {
        this.rampEnd = hit.point.clone()
        this.updateRampGuide()
      } else if (this.sculpting) {
        this.sculptAt(hit.point, event.shiftKey)
      }
    } else if (!this.sculpting) {
      this.brushVolume.visible = false
      this.callbacks.onCursor?.(null)
    }
  }

  private onPointerUp = (event: PointerEvent) => {
    if (!this.sculpting) return
    event.preventDefault()
    event.stopImmediatePropagation()
    this.sculpting = false
    this.controls.enabled = true
    if (this.settings.tool === 'ramp' && this.rampStart && this.rampEnd) {
      this.world.applyRampBrushWorld(
        { x: this.rampStart.x + this.originX, y: this.rampStart.y, z: this.rampStart.z + this.originZ },
        { x: this.rampEnd.x + this.originX, y: this.rampEnd.y, z: this.rampEnd.z + this.originZ },
        this.settings.brushSize / METERS_PER_SCENE_UNIT,
        this.settings.strength,
        this.settings.falloff,
      )
      this.rampGuide.visible = false
      this.rampStart = null
      this.rampEnd = null
    }
    this.world.endStroke(this.settings.tool === 'ramp' ? 'Landscape ramp' : 'Landscape sculpt')
    if (this.renderer.domElement.hasPointerCapture(event.pointerId)) this.renderer.domElement.releasePointerCapture(event.pointerId)
    this.callbacks.onStrokeEnd?.()
  }

  private onPointerLeave = () => {
    if (!this.sculpting) {
      this.brushVolume.visible = false
      this.hoverPoint = null
      this.callbacks.onCursor?.(null)
    }
  }

  private onContextMenu = (event: Event) => event.preventDefault()

  applySettings(settings: ViewportSettings) {
    this.settings = settings
    for (const chunk of this.chunks.values()) chunk.mesh.material = this.activeMaterial()
    this.gridHelper.visible = settings.showGrid
    this.gridHelper.position.y = settings.seaLevel / METERS_PER_SCENE_UNIT + 0.05
    this.ocean.visible = settings.oceanVisible
    this.ocean.position.y = settings.seaLevel / METERS_PER_SCENE_UNIT
    this.ocean.material.uniforms.uSeaLevel.value = settings.seaLevel / METERS_PER_SCENE_UNIT
    this.brushVolume.visible = false
    if (settings.tool !== 'ramp') {
      this.rampGuide.visible = false
      this.rampStart = null
      this.rampEnd = null
    }
  }

  setCameraView(view: CameraView) {
    const target = this.controls.target.clone()
    const offset = view === 'top' ? new THREE.Vector3(0, 195, 0.01) : view === 'front' ? new THREE.Vector3(0, 41, 170) : new THREE.Vector3(108, 83, 119)
    this.camera.position.copy(target).add(offset)
    this.controls.target.copy(target)
    this.brushVolume.visible = false
    this.hoverPoint = null
    this.controls.update()
    this.callbacks.onViewChange?.({ x: (target.x + this.originX) * METERS_PER_SCENE_UNIT, z: (target.z + this.originZ) * METERS_PER_SCENE_UNIT })
  }

  setWorldCenter(center: { x: number; z: number }) {
    const absoluteX = center.x / METERS_PER_SCENE_UNIT
    const absoluteZ = center.z / METERS_PER_SCENE_UNIT
    const offset = this.camera.position.clone().sub(this.controls.target)
    this.originX = Math.round(absoluteX / SDF_CHUNK_SIZE) * SDF_CHUNK_SIZE
    this.originZ = Math.round(absoluteZ / SDF_CHUNK_SIZE) * SDF_CHUNK_SIZE
    this.controls.target.set(absoluteX - this.originX, this.controls.target.y, absoluteZ - this.originZ)
    this.camera.position.copy(this.controls.target).add(offset)
    this.centerCx = Number.NaN
    this.centerCz = Number.NaN
    this.gridHelper.position.x = -this.originX % SDF_CHUNK_SIZE
    this.gridHelper.position.z = -this.originZ % SDF_CHUNK_SIZE
    for (const chunk of this.chunks.values()) this.positionChunk(chunk)
    this.updateShapeTexture()
    this.updateStreaming()
    this.controls.update()
    this.callbacks.onViewChange?.(center)
  }

  zoom(factor: number) {
    const offset = this.camera.position.clone().sub(this.controls.target)
    const distance = THREE.MathUtils.clamp(offset.length() * factor, this.controls.minDistance, this.controls.maxDistance)
    offset.setLength(distance)
    this.camera.position.copy(this.controls.target).add(offset)
    this.controls.update()
  }

  capture() {
    this.renderer.render(this.scene, this.camera)
    return this.renderer.domElement.toDataURL('image/png')
  }

  private resize() {
    const width = Math.max(1, this.container.clientWidth)
    const height = Math.max(1, this.container.clientHeight)
    this.camera.aspect = width / height
    this.camera.updateProjectionMatrix()
    this.renderer.setSize(width, height, false)
  }

  private animate = () => {
    if (this.disposed) return
    this.animationFrame = requestAnimationFrame(this.animate)
    this.controls.update()
    if (!this.sculpting) this.recenter()
    if (this.streamDirty) this.updateStreaming()
    if (this.settings.stage !== 'shape' && this.container.clientWidth > 0) {
      this.ocean.material.uniforms.uTime.value = performance.now() / 1000
      const renderStart = performance.now()
      this.renderer.render(this.scene, this.camera)
      this.lastRenderMs = performance.now() - renderStart
      this.frameCount += 1
    }
    const now = performance.now()
    if (now - this.lastStats > 1000) {
      const elapsed = now - this.lastStats
      const fps = Math.round(this.frameCount * 1000 / elapsed)
      const frameMs = this.frameCount > 0 ? elapsed / this.frameCount : 0
      this.frameCount = 0
      this.lastStats = now
      const frustum = new THREE.Frustum()
      const matrix = new THREE.Matrix4().multiplyMatrices(this.camera.projectionMatrix, this.camera.matrixWorldInverse)
      frustum.setFromProjectionMatrix(matrix)
      const visibleTiles = [...this.chunks.values()].filter((chunk) => chunk.mesh.count > 0 && frustum.intersectsObject(chunk.mesh)).length
      const triangles = [...this.chunks.values()].reduce((sum, chunk) => sum + Math.floor(chunk.mesh.count / 3), 0)
      const stats = { fps, visibleTiles, triangles, frameMs, renderMs: this.lastRenderMs, drawCalls: this.renderer.info.render.calls, renderedTriangles: this.renderer.info.render.triangles, geometries: this.renderer.info.memory.geometries, renderScale: this.renderScale }
      this.renderer.domElement.dataset.stats = JSON.stringify(stats)
      if (import.meta.env.DEV) this.renderer.domElement.dataset.chunkCounts = [...this.chunks.values()].map((chunk) => Math.floor(chunk.mesh.count / 3)).join(',')
      this.callbacks.onStats?.(stats)
      this.adaptRenderScale(fps)
    }
  }

  private adaptRenderScale(fps: number) {
    if (this.settings.stage === 'shape') return
    if (fps < 24) {
      this.slowFrameSamples += 1
      this.fastFrameSamples = 0
    } else if (fps > 52) {
      this.fastFrameSamples += 1
      this.slowFrameSamples = 0
    } else {
      this.slowFrameSamples = 0
      this.fastFrameSamples = 0
    }
    if (this.slowFrameSamples >= 2 && this.renderScale > 0.55) {
      this.renderScale = Math.max(0.55, this.renderScale - 0.15)
      this.renderer.setPixelRatio(this.renderScale)
      this.slowFrameSamples = 0
    } else if (this.fastFrameSamples >= 5 && this.renderScale < this.preferredRenderScale) {
      this.renderScale = Math.min(this.preferredRenderScale, this.renderScale + 0.1)
      this.renderer.setPixelRatio(this.renderScale)
      this.fastFrameSamples = 0
    }
  }

  dispose() {
    this.disposed = true
    cancelAnimationFrame(this.animationFrame)
    if (this.rebuildFrame) cancelAnimationFrame(this.rebuildFrame)
    this.unsubscribe()
    this.resizeObserver.disconnect()
    this.controls.dispose()
    this.renderer.domElement.removeEventListener('pointerdown', this.onPointerDown, { capture: true } as EventListenerOptions)
    this.renderer.domElement.removeEventListener('pointermove', this.onPointerMove, { capture: true } as EventListenerOptions)
    this.renderer.domElement.removeEventListener('pointerup', this.onPointerUp, { capture: true } as EventListenerOptions)
    this.renderer.domElement.removeEventListener('pointercancel', this.onPointerUp, { capture: true } as EventListenerOptions)
    this.renderer.domElement.removeEventListener('pointerleave', this.onPointerLeave)
    this.renderer.domElement.removeEventListener('contextmenu', this.onContextMenu)
    this.scene.traverse((object) => {
      if (object instanceof THREE.Mesh || object instanceof THREE.Line || object instanceof THREE.LineSegments) {
        object.geometry?.dispose()
        const materials = Array.isArray(object.material) ? object.material : [object.material]
        materials.forEach((material) => material.dispose())
      }
    })
    this.shapeTexture.dispose()
    this.oceanNoiseTexture.dispose()
    this.renderer.dispose()
    this.renderer.domElement.remove()
  }
}
