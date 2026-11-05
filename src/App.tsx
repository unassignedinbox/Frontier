import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import type { CSSProperties, ReactNode } from 'react'
import {
  Box,
  Brush,
  Camera,
  Check,
  CheckCircle2,
  ChevronDown,
  ChevronRight,
  CircleDot,
  Command,
  Download,
  Eye,
  EyeOff,
  FileJson,
  FolderOpen,
  Globe2,
  Grid2X2,
  Grid3X3,
  HardDrive,
  HelpCircle,
  Layers3,
  Lock,
  Map as MapIcon,
  Maximize2,
  Minimize2,
  Minus,
  Mountain,
  MountainSnow,
  MousePointer2,
  Move,
  Plus,
  Redo2,
  RotateCcw,
  Save,
  Scan,
  Settings2,
  SlidersHorizontal,
  Sparkles,
  Spline,
  Undo2,
  Unlock,
  Waves,
  X,
  ZoomIn,
  ZoomOut,
} from 'lucide-react'
import type { LucideIcon } from 'lucide-react'
import { Logo } from './components/Logo'
import { MiniMap } from './components/MiniMap'
import { ShapeCanvas } from './components/ShapeCanvas'
import type { ShapeCanvasHandle } from './components/ShapeCanvas'
import { TerrainViewport } from './components/TerrainViewport'
import type { TerrainViewportHandle } from './components/TerrainViewport'
import {
  METERS_PER_SCENE_UNIT,
  SDF_CHUNK_SIZE,
  SHAPE_CHUNK_RESOLUTION,
  VOLUME_MAX_Y,
  VOLUME_MIN_Y,
  VOLUME_RESOLUTION,
  TerrainWorld,
} from './terrain/TerrainWorld'
import type { SculptTool, ShapeTool, TerrainLayer } from './terrain/TerrainWorld'
import type { CameraView, CursorLocation, DisplayMode, ViewportStats } from './terrain/ViewportEngine'

type Stage = 'shape' | 'sculpt'
type ModalName = 'bake' | 'export' | 'help' | 'new' | 'world' | null
type MenuName = 'file' | 'edit' | 'terrain' | 'view' | null

interface ToastState {
  id: number
  message: string
  detail?: string
}

const STORAGE_KEY = 'frontier.terrain.project.v3'
const LEGACY_STORAGE_KEY = 'frontier.terrain.project.v2'
const formatNumber = (value: number) => Math.round(value).toLocaleString('en-US')
const signedNumber = (value: number) => `${value >= 0 ? '+' : '−'}${formatNumber(Math.abs(value))}`
const sculptToolDetails: Partial<Record<SculptTool, { title: string; description: string }>> = {
  sculpt: { title: 'Additive / subtractive sculpt', description: 'Drag to build volume. Hold Shift to carve into it.' },
  smooth: { title: 'Relax the surface', description: 'Averages neighboring 3D density while preserving the broader form.' },
  flatten: { title: 'Flatten to the picked plane', description: 'The target height is picked where the stroke begins.' },
  ramp: { title: 'Two-point ramp', description: 'Press on the first point, drag to the second, then release.' },
  erosion: { title: 'Slope-selective erosion', description: 'Relaxes steep density gradients more strongly than flat areas.' },
  noise: { title: 'Volumetric noise', description: 'Adds 3D fractal detail. Hold Shift to erode with noise.' },
}

function IconButton({ icon: Icon, label, onClick, active = false, disabled = false, className = '' }: {
  icon: LucideIcon
  label: string
  onClick?: () => void
  active?: boolean
  disabled?: boolean
  className?: string
}) {
  return (
    <button className={`icon-button ${active ? 'active' : ''} ${className}`} onClick={onClick} disabled={disabled} aria-label={label} title={label}>
      <Icon size={15} strokeWidth={1.7} />
    </button>
  )
}

function Toggle({ checked, onChange, label }: { checked: boolean; onChange: () => void; label: string }) {
  return <button className={`toggle ${checked ? 'on' : ''}`} onClick={onChange} role="switch" aria-checked={checked} aria-label={label}><span /></button>
}

function Slider({ label, value, min, max, step = 1, onChange, display, hint }: {
  label: string
  value: number
  min: number
  max: number
  step?: number
  onChange: (value: number) => void
  display: string
  hint?: string
}) {
  const progress = ((value - min) / (max - min)) * 100
  return (
    <label className="slider-field">
      <span className="field-heading"><span>{label}</span><output>{display}</output></span>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(event) => onChange(Number(event.target.value))}
        style={{ '--range-progress': `${progress}%` } as CSSProperties}
      />
      {hint && <span className="field-hint">{hint}</span>}
    </label>
  )
}

function Modal({ title, eyebrow, children, onClose, wide = false }: {
  title: string
  eyebrow?: string
  children: ReactNode
  onClose: () => void
  wide?: boolean
}) {
  return (
    <div className="modal-backdrop" onMouseDown={(event) => { if (event.target === event.currentTarget) onClose() }}>
      <section className={`modal ${wide ? 'modal-wide' : ''}`} role="dialog" aria-modal="true" aria-label={title}>
        <header className="modal-header">
          <div>{eyebrow && <span className="eyebrow">{eyebrow}</span>}<h2>{title}</h2></div>
          <IconButton icon={X} label="Close dialog" onClick={onClose} />
        </header>
        {children}
      </section>
    </div>
  )
}

function MenuItem({ icon: Icon, label, shortcut, onClick, disabled = false }: {
  icon?: LucideIcon
  label: string
  shortcut?: string
  onClick: () => void
  disabled?: boolean
}) {
  return (
    <button className="menu-item" onClick={onClick} disabled={disabled}>
      <span className="menu-item-icon">{Icon && <Icon size={14} />}</span>
      <span>{label}</span>
      {shortcut && <kbd>{shortcut}</kbd>}
    </button>
  )
}

function App() {
  const world = useMemo(() => new TerrainWorld(), [])
  const viewportRef = useRef<TerrainViewportHandle>(null)
  const shapeRef = useRef<ShapeCanvasHandle>(null)
  const fileInputRef = useRef<HTMLInputElement>(null)
  const saveTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null)
  const loadedProjectRef = useRef(false)
  const [projectName, setProjectName] = useState('Aurelia')
  const [stage, setStage] = useState<Stage>('sculpt')
  const [shapeTool, setShapeTool] = useState<ShapeTool>('add')
  const [sculptTool, setSculptTool] = useState<SculptTool>('sculpt')
  const [shapeBrushSize, setShapeBrushSize] = useState(1400)
  const [sculptBrushSize, setSculptBrushSize] = useState(850)
  const [strength, setStrength] = useState(0.45)
  const [falloff, setFalloff] = useState(0.65)
  const [erosion, setErosion] = useState(0.72)
  const [symmetryX, setSymmetryX] = useState(false)
  const [symmetryZ, setSymmetryZ] = useState(false)
  const [displayMode, setDisplayMode] = useState<DisplayMode>('lit')
  const [cameraView, setCameraView] = useState<CameraView>('perspective')
  const [showGrid, setShowGrid] = useState(false)
  const [oceanVisible, setOceanVisible] = useState(true)
  const [seaLevel, setSeaLevel] = useState(0)
  const [leftTab, setLeftTab] = useState<'layers' | 'world'>('layers')
  const [layerVersion, setLayerVersion] = useState(0)
  const [canUndo, setCanUndo] = useState(false)
  const [canRedo, setCanRedo] = useState(false)
  const [saveState, setSaveState] = useState<'saved' | 'dirty' | 'saving'>('saved')
  const [modal, setModal] = useState<ModalName>(null)
  const [menu, setMenu] = useState<MenuName>(null)
  const [viewportMenu, setViewportMenu] = useState<'camera' | 'display' | null>(null)
  const [focusMode, setFocusMode] = useState(false)
  const [toast, setToast] = useState<ToastState | null>(null)
  const [cursor, setCursor] = useState<{ x: number; z: number; height?: number } | null>(null)
  const [viewCenter, setViewCenter] = useState({ x: 0, z: 0 })
  const viewCenterRef = useRef(viewCenter)
  viewCenterRef.current = viewCenter
  const [stats, setStats] = useState<ViewportStats>({ fps: 60, visibleTiles: 49, triangles: 0, frameMs: 0, renderMs: 0, drawCalls: 0, renderedTriangles: 0, geometries: 0, renderScale: 1 })

  const brushSize = stage === 'shape' ? shapeBrushSize : sculptBrushSize
  const setBrushSize = stage === 'shape' ? setShapeBrushSize : setSculptBrushSize
  const selectedLayer = world.getLayer(world.activeLayerId) ?? world.getLayer('sculpt')!
  const allLayers = world.getAllLayers()
  const islands = world.getIslands()
  const activeIsland = world.activeIsland
  void layerVersion

  const notify = useCallback((message: string, detail?: string) => {
    setToast({ id: Date.now(), message, detail })
  }, [])

  useEffect(() => {
    if (!toast) return
    const timeout = setTimeout(() => setToast(null), 3600)
    return () => clearTimeout(timeout)
  }, [toast])

  const saveWorld = useCallback((withToast = true) => {
    setSaveState('saving')
    try {
      localStorage.setItem(STORAGE_KEY, world.serialize(projectName))
      setSaveState('saved')
      if (withToast) notify('World saved locally', `${projectName} is available after a reload.`)
    } catch {
      setSaveState('dirty')
      if (withToast) notify('Could not save locally', 'Export a project file to keep this world.')
    }
  }, [world, projectName, notify])

  useEffect(() => {
    if (loadedProjectRef.current) return
    loadedProjectRef.current = true
    const saved = localStorage.getItem(STORAGE_KEY) ?? localStorage.getItem(LEGACY_STORAGE_KEY)
    if (saved) {
      try {
        world.deserialize(saved)
        setLayerVersion((value) => value + 1)
        const parsed = JSON.parse(saved) as { name?: string }
        if (parsed.name) setProjectName(parsed.name)
      } catch {
        localStorage.removeItem(STORAGE_KEY)
      }
    }
  }, [world])

  useEffect(() => {
    const unsubscribe = world.subscribe(() => {
      setLayerVersion((value) => value + 1)
      setCanUndo(world.canUndo)
      setCanRedo(world.canRedo)
      setSaveState('dirty')
      if (saveTimerRef.current) clearTimeout(saveTimerRef.current)
      saveTimerRef.current = setTimeout(() => saveWorld(false), 2800)
    })
    saveWorld(false)
    return () => {
      unsubscribe()
      if (saveTimerRef.current) clearTimeout(saveTimerRef.current)
    }
  }, [world, saveWorld])

  const switchStage = useCallback((next: Stage) => {
    setStage(next)
    setViewportMenu(null)
    if (next === 'sculpt' && world.getLayer(world.activeLayerId)?.kind !== 'sculpt') world.setActiveLayer('sculpt')
  }, [world])

  useEffect(() => {
    if (stage === 'shape') shapeRef.current?.setWorldCenter(viewCenterRef.current)
    else viewportRef.current?.setWorldCenter(viewCenterRef.current)
  }, [stage])

  const undo = useCallback(() => {
    if (world.undo()) notify('Undid last terrain edit')
  }, [world, notify])
  const redo = useCallback(() => {
    if (world.redo()) notify('Redid terrain edit')
  }, [world, notify])

  useEffect(() => {
    const keyboard = (event: KeyboardEvent) => {
      const target = event.target as HTMLElement
      if (['INPUT', 'TEXTAREA', 'SELECT'].includes(target.tagName)) return
      const key = event.key.toLowerCase()
      if ((event.ctrlKey || event.metaKey) && key === 's') {
        event.preventDefault()
        saveWorld()
      } else if ((event.ctrlKey || event.metaKey) && key === 'z') {
        event.preventDefault()
        if (event.shiftKey) redo()
        else undo()
      } else if ((event.ctrlKey || event.metaKey) && key === 'y') {
        event.preventDefault()
        redo()
      } else if (key === '1') switchStage('shape')
      else if (key === '2') switchStage('sculpt')
      else if (key === '[') setBrushSize((value) => Math.max(50, value - 100))
      else if (key === ']') setBrushSize((value) => Math.min(4000, value + 100))
      else if (key === 'f' && stage === 'sculpt') setSculptTool('flatten')
      else if (key === 's' && !event.ctrlKey && !event.metaKey) stage === 'shape' ? setShapeTool('subtract') : setSculptTool('smooth')
      else if (key === 'a' && stage === 'shape') setShapeTool('add')
      else if (key === 'r' && stage === 'sculpt') setSculptTool('sculpt')
      else if (key === 'e' && stage === 'sculpt') setSculptTool('erosion')
      else if (key === 'n' && stage === 'sculpt') setSculptTool('noise')
      else if (key === 'g' && stage === 'sculpt') setSculptTool('ramp')
      else if (key === 'escape') {
        setModal(null)
        setMenu(null)
        setViewportMenu(null)
        setFocusMode(false)
      }
    }
    window.addEventListener('keydown', keyboard)
    return () => window.removeEventListener('keydown', keyboard)
  }, [saveWorld, undo, redo, switchStage, stage, setBrushSize])

  useEffect(() => {
    const close = () => { setMenu(null); setViewportMenu(null) }
    window.addEventListener('click', close)
    return () => window.removeEventListener('click', close)
  }, [])

  const download = (url: string, filename: string) => {
    const anchor = document.createElement('a')
    anchor.href = url
    anchor.download = filename
    anchor.click()
  }

  const exportProject = () => {
    const blob = new Blob([world.serialize(projectName)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    download(url, `${projectName.toLowerCase().replace(/\s+/g, '-')}.frontier.json`)
    setTimeout(() => URL.revokeObjectURL(url), 1000)
    notify('Project exported', 'Independent island SDFs, visibility, layer settings, and sculpt data are included.')
  }

  const exportVolume = () => {
    const blob = new Blob([world.serializeVolume()], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    download(url, `${projectName.toLowerCase().replace(/\s+/g, '-')}.sdf.json`)
    setTimeout(() => URL.revokeObjectURL(url), 1000)
    notify('Sparse SDF volume exported', `${world.getIslands().length} independent islands · positive-solid Float32 density`)
  }

  const exportViewport = () => {
    if (stage !== 'sculpt') {
      notify('Open the 3D viewport first', 'Viewport capture is available in Sculpt mode.')
      return
    }
    const data = viewportRef.current?.capture()
    if (data) {
      download(data, `${projectName.toLowerCase().replace(/\s+/g, '-')}-viewport.png`)
      notify('Viewport captured', 'A clean render was saved without editor overlays.')
    }
  }

  const changeCamera = (view: CameraView) => {
    setCameraView(view)
    viewportRef.current?.setCameraView(view)
    setViewportMenu(null)
  }

  const selectLayer = (layer: TerrainLayer) => {
    world.setActiveLayer(layer.id)
  }

  const selectIsland = (islandId: string) => {
    world.setActiveIsland(islandId)
  }

  const focusIsland = (islandId: string) => {
    world.setActiveIsland(islandId)
    const islandCenter = world.getIslandCenter(islandId)
    if (!islandCenter) return
    const center = { x: islandCenter.x * METERS_PER_SCENE_UNIT, z: islandCenter.z * METERS_PER_SCENE_UNIT }
    setViewCenter(center)
    if (stage === 'shape') shapeRef.current?.setWorldCenter(center)
    else viewportRef.current?.setWorldCenter(center)
  }

  const addIsland = () => {
    const id = world.addIsland({ x: viewCenterRef.current.x / METERS_PER_SCENE_UNIT, z: viewCenterRef.current.z / METERS_PER_SCENE_UNIT }).id
    setShapeTool('add')
    switchStage('shape')
    notify('Independent island created', 'Draw its coastline in Shape mode. Other island SDFs are unchanged.')
    return id
  }

  const bake = () => {
    world.beginStroke()
    world.bakeVisibleLayers()
    world.endStroke('Bake visible layers')
    setModal(null)
    switchStage('sculpt')
    notify(`${activeIsland.name} SDF chunks baked`, 'Only this island was baked. Its extracted isosurface is preserved; other islands are unchanged.')
  }

  const blankWorld = () => {
    world.createBlank()
    setProjectName('Untitled world')
    setModal(null)
    switchStage('shape')
    setShapeTool('add')
    notify('Blank ocean created', 'Draw with the Add brush to establish your first landmass.')
  }

  const openProject = async (file: File) => {
    try {
      world.deserialize(await file.text())
      setProjectName(file.name.replace(/\.frontier\.json$|\.json$/g, ''))
      switchStage('sculpt')
      notify('Project opened', file.name)
    } catch {
      notify('Could not open project', 'Choose a valid Frontier project file.')
    }
  }

  const renderBrushPreview = () => (
    <div className={`brush-preview ${stage === 'shape' && shapeTool === 'subtract' ? 'subtract' : ''}`}>
      <div className="brush-preview-rings"><span /><span /><span /><i /></div>
      <div className="brush-preview-meta"><span>BRUSH RADIUS</span><strong>{formatNumber(brushSize)} <small>m</small></strong></div>
      <div className="brush-preview-mode"><CircleDot size={12} /> {stage === 'shape' ? 'COASTLINE BRUSH' : 'LANDSCAPE BRUSH'}</div>
    </div>
  )

  const renderShapeInspector = () => (
    <>
      <section className="inspector-section">
        <div className="section-title"><span>SDF OPERATION</span><span className="section-tag">2D</span></div>
        <div className="tool-grid tool-grid-three">
          <button className={shapeTool === 'add' ? 'selected' : ''} onClick={() => setShapeTool('add')}><Plus size={17} /><span>Add land</span><kbd>A</kbd></button>
          <button className={shapeTool === 'subtract' ? 'selected' : ''} onClick={() => setShapeTool('subtract')}><Minus size={17} /><span>Subtract</span><kbd>S</kbd></button>
          <button className={shapeTool === 'smooth' ? 'selected' : ''} onClick={() => setShapeTool('smooth')}><Waves size={17} /><span>Smooth</span></button>
        </div>
      </section>
      <section className="inspector-section">
        <div className="section-title"><span>BRUSH</span><SlidersHorizontal size={13} /></div>
        {renderBrushPreview()}
        <Slider label="Radius" value={shapeBrushSize} min={100} max={4000} step={50} onChange={setShapeBrushSize} display={`${formatNumber(shapeBrushSize)} m`} />
        <Slider label="Strength" value={strength} min={0.05} max={1} step={0.01} onChange={setStrength} display={`${Math.round(strength * 100)}%`} />
        <Slider label="Smooth blend" value={falloff} min={0} max={1} step={0.01} onChange={setFalloff} display={`${Math.round(falloff * 100)}%`} hint="Blends the new SDF into the existing coastline." />
        <Slider label="Stroke erosion" value={erosion} min={0} max={1} step={0.01} onChange={setErosion} display={`${Math.round(erosion * 100)}%`} hint="Domain warping + ridged channels create natural bays and peninsulas." />
      </section>
      <section className="inspector-section compact-section">
        <div className="section-title"><span>FIELD SETTINGS</span><Settings2 size={13} /></div>
        <div className="property-row"><span>Combination</span><strong>Smooth union</strong></div>
        <div className="property-row"><span>Shape chunks</span><strong>{SHAPE_CHUNK_RESOLUTION}² / chunk · R32F</strong></div>
        <div className="property-row"><span>World span</span><strong>Unbounded</strong></div>
        <div className="property-row"><span>Volume rebuild</span><strong className="live-text"><i /> On stroke end</strong></div>
      </section>
    </>
  )

  const renderSculptInspector = () => {
    const detail = sculptToolDetails[sculptTool] ?? sculptToolDetails.sculpt!
    return (
      <>
        <section className="inspector-section">
          <div className="section-title"><span>LANDSCAPE TOOLS</span><span className="section-tag">SDF</span></div>
          <div className="tool-grid landscape-tool-grid">
            <button className={sculptTool === 'sculpt' ? 'selected' : ''} onClick={() => setSculptTool('sculpt')}><Brush size={17} /><span>Sculpt</span><kbd>R</kbd></button>
            <button className={sculptTool === 'smooth' ? 'selected' : ''} onClick={() => setSculptTool('smooth')}><Waves size={17} /><span>Smooth</span><kbd>S</kbd></button>
            <button className={sculptTool === 'flatten' ? 'selected' : ''} onClick={() => setSculptTool('flatten')}><Scan size={17} /><span>Flatten</span><kbd>F</kbd></button>
            <button className={sculptTool === 'ramp' ? 'selected' : ''} onClick={() => setSculptTool('ramp')}><Move size={17} /><span>Ramp</span><kbd>G</kbd></button>
            <button className={sculptTool === 'erosion' ? 'selected' : ''} onClick={() => setSculptTool('erosion')}><Mountain size={17} /><span>Erosion</span><kbd>E</kbd></button>
            <button className={sculptTool === 'noise' ? 'selected' : ''} onClick={() => setSculptTool('noise')}><Sparkles size={17} /><span>Noise</span><kbd>N</kbd></button>
          </div>
        </section>
        <section className="inspector-section">
          <div className="section-title"><span>BRUSH SETTINGS</span><SlidersHorizontal size={13} /></div>
          {renderBrushPreview()}
          <Slider label="Radius" value={sculptBrushSize} min={100} max={4000} step={50} onChange={setSculptBrushSize} display={`${formatNumber(sculptBrushSize)} m`} />
          <Slider label="Strength" value={strength} min={0.05} max={1} step={0.01} onChange={setStrength} display={`${Math.round(strength * 100)}%`} />
          <Slider label="Falloff" value={falloff} min={0} max={1} step={0.01} onChange={setFalloff} display={`${Math.round(falloff * 100)}%`} />
          <div className="landscape-tool-note"><strong>{detail.title}</strong><span>{detail.description}</span></div>
        </section>
        {sculptTool !== 'ramp' && <section className="inspector-section compact-section">
          <div className="section-title"><span>SYMMETRY</span><span className="section-tag">ISLAND LOCAL</span></div>
          <div className="property-row"><span>Mirror X axis</span><Toggle checked={symmetryX} onChange={() => setSymmetryX((value) => !value)} label="Mirror X axis" /></div>
          <div className="property-row"><span>Mirror Z axis</span><Toggle checked={symmetryZ} onChange={() => setSymmetryZ((value) => !value)} label="Mirror Z axis" /></div>
        </section>}
      </>
    )
  }

  const renderLayerInspector = (layer: TerrainLayer) => (
    <>
      <section className="inspector-section">
        <div className="section-title"><span>{layer.kind === 'base' ? 'SDF BASE LAYER' : 'PROCEDURAL LAYER'}</span><span className="section-tag">{layer.baked ? 'BAKED' : 'LIVE'}</span></div>
        <div className="selected-layer-hero">
          {layer.id === 'base' ? <Spline size={25} /> : <MountainSnow size={25} />}
          <div><strong>{layer.name}</strong><span>{layer.id === 'base' ? 'Continental silhouette and lowlands' : 'Closed 3D CSG volumes and domain-warped erosion'}</span></div>
        </div>
        {layer.kind !== 'base' && <Slider label="Layer opacity" value={layer.opacity} min={0} max={1} step={0.01} onChange={(opacity) => world.updateLayer(layer.id, { opacity })} display={`${Math.round(layer.opacity * 100)}%`} />}
        <div className="property-row"><span>Layer visible</span><Toggle checked={layer.visible} onChange={() => world.updateLayer(layer.id, { visible: !layer.visible })} label="Layer visible" /></div>
        <div className="property-row"><span>Locked</span><Toggle checked={layer.locked} onChange={() => world.updateLayer(layer.id, { locked: !layer.locked })} label="Layer locked" /></div>
      </section>
      <section className="inspector-section compact-section">
        <div className="section-title"><span>INPUT & OUTPUT</span></div>
        <div className="property-row"><span>Input</span><strong>{layer.id === 'base' ? 'Editable SDF' : 'Continental SDF'}</strong></div>
        <div className="property-row"><span>Resolution</span><strong>{layer.id === 'base' ? `${SHAPE_CHUNK_RESOLUTION}² / chunk` : `${VOLUME_RESOLUTION}³ / chunk`}</strong></div>
        <div className="property-row"><span>Precision</span><strong>32-bit float</strong></div>
        <button className="secondary-button full-width" onClick={() => layer.id === 'base' ? switchStage('shape') : world.setActiveLayer('sculpt')}>
          {layer.id === 'base' ? <Grid2X2 size={14} /> : <Brush size={14} />}
          {layer.id === 'base' ? 'Edit continental shape' : 'Return to hand sculpt'}
        </button>
      </section>
    </>
  )

  return (
    <div className={`app-shell ${focusMode ? 'focus-mode' : ''}`}>
      <header className="app-header">
        <div className="header-brand"><Logo /></div>
        <div className="project-identity">
          <span className="project-icon"><Mountain size={16} /></span>
          <div><strong>{projectName}</strong><span>Terrain project</span></div>
          <ChevronDown size={13} />
        </div>
        <nav className="app-menus" aria-label="Application menu">
          {(['file', 'edit', 'terrain', 'view'] as const).map((item) => (
            <div className="menu-anchor" key={item} onClick={(event) => event.stopPropagation()}>
              <button className={menu === item ? 'open' : ''} onClick={() => setMenu(menu === item ? null : item)}>{item.charAt(0).toUpperCase() + item.slice(1)}</button>
              {menu === item && (
                <div className="dropdown-menu app-dropdown">
                  {item === 'file' && <>
                    <MenuItem icon={Plus} label="New blank world" onClick={() => { setModal('new'); setMenu(null) }} />
                    <MenuItem icon={FolderOpen} label="Open project…" onClick={() => { fileInputRef.current?.click(); setMenu(null) }} />
                    <div className="menu-divider" />
                    <MenuItem icon={Save} label="Save locally" shortcut="⌘ S" onClick={() => { saveWorld(); setMenu(null) }} />
                    <MenuItem icon={Download} label="Export sparse SDF volume" onClick={() => { exportVolume(); setMenu(null) }} />
                  </>}
                  {item === 'edit' && <>
                    <MenuItem icon={Undo2} label="Undo" shortcut="⌘ Z" onClick={() => { undo(); setMenu(null) }} disabled={!canUndo} />
                    <MenuItem icon={Redo2} label="Redo" shortcut="⇧ ⌘ Z" onClick={() => { redo(); setMenu(null) }} disabled={!canRedo} />
                    <div className="menu-divider" />
                    <MenuItem icon={RotateCcw} label="Reset camera" onClick={() => { viewportRef.current?.resetCamera(); shapeRef.current?.resetView(); setMenu(null) }} />
                  </>}
                  {item === 'terrain' && <>
                    <MenuItem icon={Plus} label="New independent island" onClick={() => { addIsland(); setMenu(null) }} />
                    <MenuItem icon={Grid2X2} label="Draw island SDF" shortcut="1" onClick={() => { switchStage('shape'); setMenu(null) }} />
                    <MenuItem icon={Mountain} label="Sculpt volumetric SDF" shortcut="2" onClick={() => { switchStage('sculpt'); setMenu(null) }} />
                    <div className="menu-divider" />
                    <MenuItem icon={Layers3} label="Bake visible layers" onClick={() => { setModal('bake'); setMenu(null) }} />
                  </>}
                  {item === 'view' && <>
                    <MenuItem icon={Camera} label="Perspective camera" onClick={() => { changeCamera('perspective'); switchStage('sculpt'); setMenu(null) }} />
                    <MenuItem icon={MapIcon} label="Top camera" onClick={() => { changeCamera('top'); switchStage('sculpt'); setMenu(null) }} />
                    <MenuItem icon={Grid3X3} label={showGrid ? 'Hide partition grid' : 'Show partition grid'} onClick={() => { setShowGrid((value) => !value); setMenu(null) }} />
                    <MenuItem icon={Maximize2} label="Toggle focus mode" onClick={() => { setFocusMode((value) => !value); setMenu(null) }} />
                  </>}
                </div>
              )}
            </div>
          ))}
          <button onClick={() => setModal('help')}>Help</button>
        </nav>
        <div className="header-spacer" />
        <div className={`save-indicator ${saveState}`}>
          {saveState === 'saved' ? <CheckCircle2 size={13} /> : <HardDrive size={13} />}
          <span>{saveState === 'saved' ? 'Saved locally' : saveState === 'saving' ? 'Saving…' : 'Unsaved changes'}</span>
        </div>
        <div className="history-actions"><IconButton icon={Undo2} label="Undo" onClick={undo} disabled={!canUndo} /><IconButton icon={Redo2} label="Redo" onClick={redo} disabled={!canRedo} /></div>
        <button className="button button-quiet save-button" onClick={() => saveWorld()}><Save size={14} /> Save</button>
        <button className="button button-primary" onClick={() => setModal('export')}><Download size={14} /> Export</button>
        <button className="avatar" title="Local workspace">FR</button>
      </header>

      <div className="workflow-bar">
        <div className="workflow-title"><span>WORLD BUILDER</span><strong>Large-world SDF workspace</strong></div>
        <div className="workflow-steps">
          <button className={stage === 'shape' ? 'active' : ''} onClick={() => switchStage('shape')}>
            <span className="step-number">01</span><Grid2X2 size={15} /><span>Shape</span><small>Island SDF</small>
          </button>
          <ChevronRight className="workflow-chevron" size={15} />
          <button className={stage === 'sculpt' ? 'active' : ''} onClick={() => switchStage('sculpt')}>
            <span className="step-number">02</span><Mountain size={16} /><span>Sculpt</span><small>Landscape SDF</small>
          </button>
          <ChevronRight className="workflow-chevron" size={15} />
          <button onClick={() => setModal('bake')}>
            <span className="step-number">03</span><Box size={15} /><span>Bake</span><small>Commit layers</small>
          </button>
        </div>
        <div className="workflow-world-info"><Globe2 size={14} /><span>∞ sparse world</span><i /><Grid3X3 size={13} /><span>7 × 7 search / island</span></div>
      </div>

      <main className="editor-main">
        <aside className="left-panel panel">
          <div className="panel-tabs">
            <button className={leftTab === 'layers' ? 'active' : ''} onClick={() => setLeftTab('layers')}><Layers3 size={14} /> Layers</button>
            <button className={leftTab === 'world' ? 'active' : ''} onClick={() => setLeftTab('world')}><Globe2 size={14} /> World</button>
          </div>
          {leftTab === 'layers' ? (
            <>
              <div className="layer-panel-scroll">
                <div className="world-summary">
                  <div className="world-summary-icon"><Mountain size={21} /></div>
                  <div><strong>{activeIsland.name}</strong><span>Active island · independent SDF</span></div>
                  <span className="world-badge">{activeIsland.visible ? 'LIVE' : 'HIDDEN'}</span>
                </div>
                <div className="panel-section-heading"><span>ISLAND SDFs</span><span className="count-badge">{islands.length}</span><IconButton icon={Plus} label="Add independent island" onClick={addIsland} /></div>
                <div className="island-list">
                  {islands.map((island) => {
                    const bounds = world.getIslandBounds(island.id)
                    return <div key={island.id} className={`island-row ${island.id === world.activeIslandId ? 'selected' : ''} ${!island.visible ? 'island-hidden' : ''}`} onClick={() => selectIsland(island.id)}>
                      <span className="island-row-icon"><Mountain size={14} /></span>
                      <div><strong>{island.name}</strong><span>{bounds ? `${((bounds.maxX - bounds.minX) * METERS_PER_SCENE_UNIT / 1000).toFixed(1)} km field` : 'Empty field · draw in Shape'}</span></div>
                      <button className="island-focus" title={`Focus ${island.name}`} aria-label={`Focus ${island.name}`} onClick={(event) => { event.stopPropagation(); focusIsland(island.id) }}><Scan size={12} /></button>
                      <button title={island.visible ? `Hide ${island.name}` : `Show ${island.name}`} aria-label={island.visible ? `Hide ${island.name}` : `Show ${island.name}`} onClick={(event) => { event.stopPropagation(); world.updateIsland(island.id, { visible: !island.visible }) }}>{island.visible ? <Eye size={13} /> : <EyeOff size={13} />}</button>
                    </div>
                  })}
                </div>
                <button className="add-layer-button" onClick={addIsland}><Plus size={13} /> New independent island</button>
                <div className="panel-section-heading island-layer-heading"><span>{activeIsland.name.toUpperCase()} LAYERS</span><IconButton icon={Plus} label="Add SDF sculpt layer" onClick={() => world.addSculptLayer()} /></div>
                <div className="layer-list">
                  {allLayers.map((layer, index) => {
                    const LayerIcon = layer.id === 'base' ? Spline : layer.kind === 'sculpt' ? Brush : layer.id === 'detail' ? Waves : MountainSnow
                    return (
                      <div key={layer.id} className={`layer-row ${world.activeLayerId === layer.id ? 'selected' : ''} ${!layer.visible ? 'layer-hidden' : ''}`} onClick={() => selectLayer(layer)}>
                        <div className={`layer-icon layer-${layer.kind}`}><LayerIcon size={15} /></div>
                        <div className="layer-copy"><strong>{layer.name}</strong><span>{layer.baked && layer.id !== 'sculpt' ? 'Baked resident chunks' : layer.kind === 'base' ? 'SDF · Continental field' : layer.kind === 'procedural' ? `Procedural · ${Math.round(layer.opacity * 100)}%` : '3D SDF · Non-destructive'}</span></div>
                        {layer.baked && layer.id !== 'base' && <Check className="baked-check" size={12} />}
                        <div className="layer-actions">
                          <button title={layer.visible ? 'Hide layer' : 'Show layer'} onClick={(event) => { event.stopPropagation(); world.updateLayer(layer.id, { visible: !layer.visible }) }}>{layer.visible ? <Eye size={13} /> : <EyeOff size={13} />}</button>
                          <button title={layer.locked ? 'Unlock layer' : 'Lock layer'} onClick={(event) => { event.stopPropagation(); world.updateLayer(layer.id, { locked: !layer.locked }) }}>{layer.locked ? <Lock size={12} /> : <Unlock size={12} />}</button>
                        </div>
                        <span className="layer-index">0{index + 1}</span>
                      </div>
                    )
                  })}
                </div>
                <button className="add-layer-button" onClick={() => world.addSculptLayer()}><Plus size={13} /> Add SDF sculpt layer</button>

                <div className="panel-section-heading masks-heading"><span>FIELD MASKS</span><span className="count-badge">2</span></div>
                <div className="mask-list">
                  <div className="mask-row"><span className="mask-swatch coast-mask"><Spline size={13} /></span><div><strong>Island boundary</strong><span>From Continental SDF</span></div><span className="mask-linked"><Check size={11} /> Linked</span></div>
                  <div className="mask-row"><span className="mask-swatch ocean-mask"><Waves size={13} /></span><div><strong>Ocean exclusion</strong><span>Below sea level</span></div><span className="mask-linked"><Check size={11} /> Live</span></div>
                </div>
              </div>
              <div className="partition-card">
                <div className="partition-card-heading"><span><Grid3X3 size={13} /> SPARSE RESIDENCY</span><span className="live-text"><i /> ORIGIN FLOATING</span></div>
                <div className="partition-grid" aria-label="Bounds-clipped resident SDF chunk search grid">
                  {Array.from({ length: 49 }, (_, index) => <span key={index} className={[16, 17, 18, 23, 24, 25, 30, 31, 32].includes(index) ? 'active' : ''} />)}
                </div>
                <div className="partition-stats"><span><strong>{stats.visibleTiles}</strong> resident</span><span><strong>{world.editedVolumeChunks}</strong> persisted</span></div>
              </div>
            </>
          ) : (
            <div className="world-tab-content">
              <div className="world-tab-hero"><Globe2 size={30} /><span>PROJECT WORLD</span><h3>{projectName}</h3><p>Unbounded sparse volumes with a floating render origin and camera-following chunk residency.</p></div>
              <div className="panel-section-heading"><span>WORLD SETUP</span></div>
              <div className="world-property"><span>World extent</span><strong>Unbounded horizontal</strong></div>
              <div className="world-property"><span>Resident search</span><strong>7 × 7 / island · bounds clipped</strong></div>
              <div className="world-property"><span>Chunk extent</span><strong>{(SDF_CHUNK_SIZE * METERS_PER_SCENE_UNIT / 1000).toFixed(3)} km</strong></div>
              <div className="world-property"><span>Independent islands</span><strong>{islands.length} SDFs</strong></div>
              <div className="world-property"><span>Continental SDF</span><strong>{SHAPE_CHUNK_RESOLUTION}² / chunk</strong></div>
              <div className="world-property"><span>Volumetric SDF</span><strong>{VOLUME_RESOLUTION}³ / chunk</strong></div>
              <div className="world-property"><span>Base generator</span><strong>3D CSG · no heightmap</strong></div>
              <div className="world-property"><span>Density precision</span><strong>32-bit float</strong></div>
              <div className="world-property"><span>Vertical field range</span><strong>{Math.round(VOLUME_MIN_Y * METERS_PER_SCENE_UNIT)} to {Math.round(VOLUME_MAX_Y * METERS_PER_SCENE_UNIT)} m</strong></div>
              <div className="world-property"><span>Origin</span><strong>Floating · absolute coordinates</strong></div>
              <div className="world-note"><CircleDot size={15} /><p>Marching cubes extracts each resident volume. Unedited chunks are evicted; sculpt residuals and baked volumes persist sparsely.</p></div>
              <button className="secondary-button full-width" onClick={() => setModal('world')}><Settings2 size={14} /> World details</button>
            </div>
          )}
        </aside>

        <section className={`viewport-panel ${stage === 'shape' ? 'shape-mode' : 'sculpt-mode'}`}>
          <div className="viewport-render-area">
            <TerrainViewport
              ref={viewportRef}
              world={world}
              stage={stage}
              tool={sculptTool}
              brushSize={sculptBrushSize}
              strength={strength}
              falloff={falloff}
              symmetryX={symmetryX}
              symmetryZ={symmetryZ}
              displayMode={displayMode}
              showGrid={showGrid}
              oceanVisible={oceanVisible}
              seaLevel={seaLevel}
              onStats={setStats}
              onCursor={(value: CursorLocation | null) => setCursor(value)}
              onViewChange={setViewCenter}
            />
            {stage === 'shape' && <ShapeCanvas ref={shapeRef} world={world} tool={shapeTool} brushSize={shapeBrushSize} strength={strength} falloff={falloff} erosion={erosion} showGrid onCursor={setCursor} onViewChange={setViewCenter} />}

            <div className="viewport-topbar" onClick={(event) => event.stopPropagation()}>
              <div className="viewport-top-left">
                <div className="menu-anchor">
                  <button className="viewport-dropdown" onClick={() => setViewportMenu(viewportMenu === 'camera' ? null : 'camera')}>
                    <Camera size={13} /><span>{stage === 'shape' ? 'ORTHOGRAPHIC' : cameraView.toUpperCase()}</span><ChevronDown size={11} />
                  </button>
                  {viewportMenu === 'camera' && stage === 'sculpt' && <div className="dropdown-menu viewport-dropdown-menu">
                    <MenuItem label="Perspective" onClick={() => changeCamera('perspective')} />
                    <MenuItem label="Top" onClick={() => changeCamera('top')} />
                    <MenuItem label="Front" onClick={() => changeCamera('front')} />
                  </div>}
                </div>
                <span className="viewport-divider" />
                {stage === 'sculpt' ? <div className="menu-anchor">
                  <button className="viewport-dropdown subtle" onClick={() => setViewportMenu(viewportMenu === 'display' ? null : 'display')}><CircleDot size={12} /><span>{displayMode === 'lit' ? 'Lit' : displayMode === 'clay' ? 'Clay' : 'Wireframe'}</span><ChevronDown size={11} /></button>
                  {viewportMenu === 'display' && <div className="dropdown-menu viewport-dropdown-menu display-dropdown">
                    <MenuItem label="Lit landscape" onClick={() => { setDisplayMode('lit'); setViewportMenu(null) }} />
                    <MenuItem label="Sculpt clay" onClick={() => { setDisplayMode('clay'); setViewportMenu(null) }} />
                    <MenuItem label="Wireframe" onClick={() => { setDisplayMode('wireframe'); setViewportMenu(null) }} />
                  </div>}
                </div> : <span className="sdf-view-label"><Spline size={12} /> Signed distance field</span>}
              </div>
              <div className="viewport-live-badge"><i /> {stage === 'shape' ? 'LIVE FIELD' : 'REAL-TIME'}</div>
              <div className="viewport-top-right">
                {stage === 'sculpt' && <IconButton icon={Grid3X3} label="Toggle SDF chunk grid" active={showGrid} onClick={() => setShowGrid((value) => !value)} />}
                <IconButton icon={RotateCcw} label="Reset view" onClick={() => stage === 'shape' ? shapeRef.current?.resetView() : viewportRef.current?.resetCamera()} />
                <IconButton icon={focusMode ? Minimize2 : Maximize2} label="Toggle focus mode" onClick={() => setFocusMode((value) => !value)} />
              </div>
            </div>

            <div className="viewport-scene-label">
              <span>{stage === 'shape' ? 'INDEPENDENT ISLAND FIELD' : 'ACTIVE ISLAND'}</span>
              <strong>{stage === 'shape' ? `Draw ${activeIsland.name}` : activeIsland.name}</strong>
              <small>{stage === 'shape' ? 'Independent SDF · Pan with RMB / Space · Eroded strokes' : `Independent volumetric SDF  /  Sea level ${seaLevel} m`}</small>
            </div>

            <div className="viewport-tool-rail">
              {stage === 'shape' ? <>
                <IconButton icon={Plus} label="Add land (A)" active={shapeTool === 'add'} onClick={() => setShapeTool('add')} />
                <IconButton icon={Minus} label="Subtract land (S)" active={shapeTool === 'subtract'} onClick={() => setShapeTool('subtract')} />
                <IconButton icon={Waves} label="Smooth shoreline" active={shapeTool === 'smooth'} onClick={() => setShapeTool('smooth')} />
              </> : <>
                <IconButton icon={Brush} label="Sculpt (R) · Shift to lower" active={sculptTool === 'sculpt'} onClick={() => setSculptTool('sculpt')} />
                <IconButton icon={Waves} label="Smooth (S)" active={sculptTool === 'smooth'} onClick={() => setSculptTool('smooth')} />
                <IconButton icon={Scan} label="Flatten (F)" active={sculptTool === 'flatten'} onClick={() => setSculptTool('flatten')} />
                <IconButton icon={Move} label="Ramp (G)" active={sculptTool === 'ramp'} onClick={() => setSculptTool('ramp')} />
                <IconButton icon={Mountain} label="Erosion (E)" active={sculptTool === 'erosion'} onClick={() => setSculptTool('erosion')} />
                <IconButton icon={Sparkles} label="Noise (N)" active={sculptTool === 'noise'} onClick={() => setSculptTool('noise')} />
              </>}
              <span />
              <IconButton icon={ZoomIn} label="Zoom in" onClick={() => stage === 'shape' ? shapeRef.current?.zoomBy(1.2) : viewportRef.current?.zoom(0.82)} />
              <IconButton icon={ZoomOut} label="Zoom out" onClick={() => stage === 'shape' ? shapeRef.current?.zoomBy(0.82) : viewportRef.current?.zoom(1.2)} />
            </div>

            <div className="viewport-bottom-left">
              <div className="axis-gizmo">
                <svg viewBox="0 0 64 64" aria-label="World orientation">
                  <path d="M30 35V10" stroke="#9dc986" /><path d="m30 35 22 10" stroke="#d58575" /><path d="M30 35 10 46" stroke="#739fc2" />
                  <circle cx="30" cy="35" r="3" fill="#d8e0d9" />
                  <text x="27" y="8" fill="#a9d592">Y</text><text x="53" y="50" fill="#df9283">X</text><text x="2" y="52" fill="#83add0">Z</text>
                </svg>
              </div>
              <span>WORLD UP · Y</span>
            </div>

            <div className="viewport-minimap">
              <div className="minimap-heading"><span>WORLD OVERVIEW</span><span className="minimap-north">N ↑</span></div>
              <MiniMap world={world} center={viewCenter} />
              <div className="minimap-footer"><span>{stats.visibleTiles} RESIDENT</span><span>2.048 KM / CHUNK</span></div>
            </div>

            <div className="viewport-tool-hint">
              <span className="active-tool-dot" />
              <strong>{stage === 'shape' ? shapeTool === 'add' ? 'Add land' : shapeTool === 'subtract' ? 'Subtract' : 'Smooth SDF' : sculptTool.charAt(0).toUpperCase() + sculptTool.slice(1)}</strong>
              <span className="hint-divider" />
              <span><b>{formatNumber(brushSize)}</b> m</span>
              <span><b>{Math.round(strength * 100)}</b>%</span>
              <span className="hint-divider" />
              <MousePointer2 size={12} /><span>{stage === 'shape' ? 'LMB Draw' : sculptTool === 'ramp' ? 'LMB Drag endpoints' : 'LMB Sculpt · Shift Lower'}</span>
              <span className="desktop-hint">Alt + LMB Orbit</span>
              <span className="desktop-hint">RMB Pan</span>
            </div>

            <div className="viewport-quality"><Sparkles size={11} /><span>{stage === 'shape' ? 'SPARSE R32F SDF' : 'MARCHING CUBES'}</span><i />{stage === 'shape' ? 'UNBOUNDED' : `${stats.visibleTiles} ISLAND CHUNKS`}</div>
          </div>
        </section>

        <aside className="right-panel panel">
          <div className="inspector-header"><div><span>INSPECTOR</span><strong>{stage === 'shape' ? `${activeIsland.name} SDF` : selectedLayer.kind === 'sculpt' ? 'Landscape' : selectedLayer.name}</strong></div><Settings2 size={15} /></div>
          <div className="inspector-scroll">
            {stage === 'shape' ? renderShapeInspector() : selectedLayer.kind === 'sculpt' ? renderSculptInspector() : renderLayerInspector(selectedLayer)}
            <section className="inspector-section ocean-section">
              <div className="section-title"><span><Waves size={13} /> OCEAN</span><Toggle checked={oceanVisible} onChange={() => setOceanVisible((value) => !value)} label="Ocean visible" /></div>
              <Slider label="Sea level" value={seaLevel} min={-100} max={100} step={1} onChange={setSeaLevel} display={`${seaLevel} m`} />
              <div className="property-row"><span>Wave model</span><strong>Multi-wave</strong></div>
              <div className="property-row"><span>Shore foam</span><strong className="live-text"><i /> Visible island SDFs</strong></div>
              <div className="property-row"><span>Reflection</span><strong>Sky + Fresnel</strong></div>
            </section>
            <div className="inspector-tip"><Command size={14} /><p><strong>Quick brush sizing</strong><span>Use <kbd>[</kbd> and <kbd>]</kbd> to change diameter by 100 m.</span></p></div>
          </div>
          <div className="inspector-footer"><span><i /> NON-DESTRUCTIVE</span><button onClick={() => setModal('bake')}>Bake layers <ChevronRight size={12} /></button></div>
        </aside>
      </main>

      <footer className="status-bar">
        <div className="status-left"><span className="status-ready"><i /> READY</span><span>{stage === 'shape' ? 'SDF EDIT' : 'SCULPT EDIT'}</span><span>{stats.triangles.toLocaleString()} TRIS</span></div>
        <div className="status-coordinates">{cursor ? <><span>X <b>{signedNumber(cursor.x)}</b> m</span><span>Z <b>{signedNumber(cursor.z)}</b> m</span>{cursor.height !== undefined && <span>Y <b>{formatNumber(cursor.height)}</b> m</span>}</> : <span>Move over the viewport for world coordinates</span>}</div>
        <div className="status-right"><span>{stage === 'shape' ? `${SHAPE_CHUNK_RESOLUTION}² / CHUNK` : `${VOLUME_RESOLUTION}³ VOXELS`}</span><span className="fps-indicator" title={`${stats.frameMs.toFixed(0)} ms frame · ${stats.renderMs.toFixed(1)} ms render submit · ${stats.drawCalls} draw calls · ${stats.geometries} geometries · ${Math.round(stats.renderScale * 100)}% scale`}><i />{stage === 'shape' ? 'UNBOUNDED 2D' : `${stats.fps} FPS`}</span><span>FRONTIER 0.2</span></div>
      </footer>

      <input ref={fileInputRef} type="file" accept=".json,.frontier.json" hidden onChange={(event) => { const file = event.target.files?.[0]; if (file) openProject(file); event.target.value = '' }} />

      {toast && <div className="toast" key={toast.id}><span className="toast-icon"><Check size={15} /></span><div><strong>{toast.message}</strong>{toast.detail && <span>{toast.detail}</span>}</div><button onClick={() => setToast(null)}><X size={13} /></button></div>}

      {modal === 'new' && <Modal title="Create a blank world" eyebrow="NEW PROJECT" onClose={() => setModal(null)}>
        <div className="modal-body"><div className="modal-hero-icon"><Globe2 size={28} /></div><p className="modal-description">Start with an empty ocean and draw your continent from the first SDF stroke. Your current project stays in local storage until you save over it.</p><div className="modal-summary"><div><span>World extent</span><strong>Unbounded horizontal</strong></div><div><span>Starting field</span><strong>Sparse ocean · −0.65 SDF</strong></div><div><span>First stage</span><strong>Eroded continental shape</strong></div></div></div>
        <div className="modal-actions"><button className="button button-quiet" onClick={() => setModal(null)}>Cancel</button><button className="button button-primary" onClick={blankWorld}><Plus size={14} /> Create blank world</button></div>
      </Modal>}

      {modal === 'bake' && <Modal title="Bake terrain layers" eyebrow="WORLD BUILDER · 03" onClose={() => setModal(null)}>
        <div className="modal-body"><p className="modal-description">Collapse the visible layer result for {activeIsland.name} into its allocated SDF chunks. Your ocean remains independent, and new sculpt strokes are stored as fresh volumetric residuals.</p><div className="bake-flow"><div><Layers3 size={21} /><strong>{activeIsland.name} · {world.getIslandChunkCount(activeIsland.id)} chunks</strong><span>Continental + procedural + sculpt</span></div><ChevronRight size={18} /><div className="bake-result"><Box size={21} /><strong>Baked volumes</strong><span>32-bit 3D density</span></div></div><div className="bake-checklist"><span><Check size={13} /> Preserve the exact extracted isosurface</span><span><Check size={13} /> Persist baked chunks after eviction</span><span><Check size={13} /> Create an undo snapshot before baking</span></div><div className="modal-note"><CircleDot size={15} /><span>Baking affects allocated chunks ({VOLUME_RESOLUTION}³ voxels each), not the infinite unallocated field. Newly visited chunks continue to use the procedural source.</span></div></div>
        <div className="modal-actions"><button className="button button-quiet" onClick={() => setModal(null)}>Keep editing</button><button className="button button-primary" onClick={bake} disabled={world.getIslandChunkCount(activeIsland.id) === 0}><Box size={14} />Bake {activeIsland.name}</button></div>
      </Modal>}

      {modal === 'export' && <Modal title="Export your world" eyebrow="FRONTIER EXCHANGE" onClose={() => setModal(null)} wide>
        <div className="modal-body"><p className="modal-description">Take the working data into your pipeline, save an editable project, or capture the current landscape.</p><div className="export-options">
          <button onClick={exportVolume}><span className="export-icon"><Box size={22} /></span><strong>Sparse SDF volume</strong><span>3D density chunks</span><small>{VOLUME_RESOLUTION}³ · Float32 voxels</small><Download size={14} className="export-download" /></button>
          <button onClick={exportProject}><span className="export-icon"><FileJson size={22} /></span><strong>Frontier project</strong><span>Editable world data</span><small>SDF + layers + sculpt</small><Download size={14} className="export-download" /></button>
          <button onClick={exportViewport}><span className="export-icon"><Camera size={22} /></span><strong>Viewport</strong><span>Clean render</span><small>PNG · current camera</small><Download size={14} className="export-download" /></button>
        </div><div className="modal-note"><CircleDot size={15} /><span>The volume format stores positive-solid 32-bit density, absolute chunk coordinates, resolution, and vertical range. No heightmap is generated.</span></div></div>
        <div className="modal-actions"><span className="modal-action-note"><HardDrive size={13} /> Exports are generated locally</span><button className="button button-quiet" onClick={() => setModal(null)}>Done</button></div>
      </Modal>}

      {modal === 'help' && <Modal title="Terrain studio essentials" eyebrow="QUICK GUIDE" onClose={() => setModal(null)} wide>
        <div className="modal-body help-body"><div className="help-workflow"><div><span>01</span><Grid2X2 size={20} /><strong>Shape</strong><p>Add, subtract, and smooth SDF stamps to establish the island or continent.</p></div><div><span>02</span><Mountain size={20} /><strong>Sculpt</strong><p>Raise mountains, cut basins, smooth slopes, or flatten sites for future roads and cities.</p></div><div><span>03</span><Box size={20} /><strong>Bake</strong><p>Commit the visible layer stack, then keep sculpting on a fresh non-destructive layer.</p></div></div><div className="shortcut-grid">{[['Shape / Sculpt', '1 / 2'], ['Add land', 'A'], ['Subtract / Smooth', 'S'], ['Sculpt / Smooth / Flatten', 'R / S / F'], ['Ramp / Erosion / Noise', 'G / E / N'], ['Subtract while sculpting', 'Shift + drag'], ['Brush size', '[ / ]'], ['Orbit', 'Alt + LMB'], ['Pan', 'RMB'], ['Zoom', 'Mouse wheel'], ['Save', 'Ctrl / ⌘ + S'], ['Undo / Redo', 'Ctrl / ⌘ + Z / Y']].map(([label, key]) => <div key={label}><span>{label}</span><kbd>{key}</kbd></div>)}</div></div>
        <div className="modal-actions"><span className="modal-action-note">All editing and rendering happens on this device.</span><button className="button button-primary" onClick={() => setModal(null)}>Start building</button></div>
      </Modal>}

      {modal === 'world' && <Modal title="World partition details" eyebrow="WORLD SETUP" onClose={() => setModal(null)}>
        <div className="modal-body"><p className="modal-description">The renderer keeps a camera-following resident set of sparse 3D fields. Marching cubes extracts each isosurface, and a floating origin keeps local GPU coordinates precise as absolute world coordinates grow.</p><div className="modal-summary"><div><span>Horizontal extent</span><strong>Unbounded</strong></div><div><span>Resident search</span><strong>7 × 7 / island · bounds clipped</strong></div><div><span>Chunk extent</span><strong>{(SDF_CHUNK_SIZE * METERS_PER_SCENE_UNIT / 1000).toFixed(3)} km</strong></div><div><span>Current triangles</span><strong>{stats.triangles.toLocaleString()}</strong></div><div><span>Continental field</span><strong>{SHAPE_CHUNK_RESOLUTION}² / chunk · Float32</strong></div><div><span>Volume field</span><strong>{VOLUME_RESOLUTION}³ / chunk · Float32</strong></div></div><div className="modal-note"><CircleDot size={15} /><span>Unedited resident chunks can be evicted. Modified continental chunks, sculpt residuals, and baked densities remain in the sparse project. This is in-memory residency, not a disk streaming server.</span></div></div>
        <div className="modal-actions"><button className="button button-primary" onClick={() => setModal(null)}>Done</button></div>
      </Modal>}
    </div>
  )
}

export default App
