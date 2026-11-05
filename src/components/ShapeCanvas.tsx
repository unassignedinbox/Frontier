import { forwardRef, useEffect, useImperativeHandle, useRef, useState } from 'react'
import {
  METERS_PER_SCENE_UNIT,
  SDF_CHUNK_SIZE,
  TerrainWorld,
} from '../terrain/TerrainWorld'
import type { ShapeTool } from '../terrain/TerrainWorld'

export interface ShapeCanvasHandle {
  resetView: () => void
  zoomBy: (factor: number) => void
  setWorldCenter: (center: { x: number; z: number }) => void
}

interface ShapeCanvasProps {
  world: TerrainWorld
  tool: ShapeTool
  brushSize: number
  strength: number
  falloff: number
  erosion: number
  showGrid: boolean
  onCursor?: (value: { x: number; z: number } | null) => void
  onViewChange?: (center: { x: number; z: number }) => void
}

interface ViewState {
  zoom: number
  centerX: number
  centerZ: number
}

export const ShapeCanvas = forwardRef<ShapeCanvasHandle, ShapeCanvasProps>(function ShapeCanvas(props, ref) {
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const propsRef = useRef(props)
  propsRef.current = props
  const viewRef = useRef<ViewState>({ zoom: 1, centerX: 0, centerZ: 0 })
  const drawRef = useRef<() => void>(() => {})
  const [zoomLabel, setZoomLabel] = useState(100)
  const [spaceDown, setSpaceDown] = useState(false)
  const spaceRef = useRef(false)
  const interactionRef = useRef<{ mode: 'draw' | 'pan'; pointerId: number; lastX: number; lastY: number; lastWorldX: number; lastWorldZ: number } | null>(null)
  const mouseRef = useRef({ x: 0, y: 0, worldX: 0, worldZ: 0, inside: false })

  const updateZoom = (value: number) => {
    viewRef.current.zoom = Math.max(0.08, Math.min(12, value))
    setZoomLabel(Math.round(viewRef.current.zoom * 100))
    drawRef.current()
  }

  useImperativeHandle(ref, () => ({
    resetView: () => {
      viewRef.current = { zoom: 1, centerX: 0, centerZ: 0 }
      setZoomLabel(100)
      propsRef.current.onViewChange?.({ x: 0, z: 0 })
      drawRef.current()
    },
    zoomBy: (factor: number) => updateZoom(viewRef.current.zoom * factor),
    setWorldCenter: (center: { x: number; z: number }) => {
      viewRef.current.centerX = center.x / METERS_PER_SCENE_UNIT
      viewRef.current.centerZ = center.z / METERS_PER_SCENE_UNIT
      propsRef.current.onViewChange?.(center)
      drawRef.current()
    },
  }))

  useEffect(() => {
    const canvas = canvasRef.current!
    const context = canvas.getContext('2d')!
    const fieldCanvas = document.createElement('canvas')
    const fieldContext = fieldCanvas.getContext('2d')!
    let width = 1
    let height = 1
    let frame = 0
    let dirtyField = true

    const metrics = () => {
      const scale = Math.max(0.001, Math.min(width, height) / 160 * viewRef.current.zoom)
      const worldWidth = width / scale
      const worldHeight = height / scale
      const left = viewRef.current.centerX - worldWidth / 2
      const top = viewRef.current.centerZ - worldHeight / 2
      return { scale, worldWidth, worldHeight, left, top }
    }

    const screenToWorld = (screenX: number, screenY: number) => {
      const view = metrics()
      return {
        worldX: view.left + screenX / view.scale,
        worldZ: view.top + screenY / view.scale,
      }
    }

    const worldToScreen = (worldX: number, worldZ: number) => {
      const view = metrics()
      return { x: (worldX - view.left) * view.scale, y: (worldZ - view.top) * view.scale }
    }

    const rebuildField = () => {
      const view = metrics()
      const imageWidth = Math.max(120, Math.min(340, Math.ceil(width / 3)))
      const imageHeight = Math.max(100, Math.min(280, Math.ceil(height / 3)))
      if (fieldCanvas.width !== imageWidth || fieldCanvas.height !== imageHeight) {
        fieldCanvas.width = imageWidth
        fieldCanvas.height = imageHeight
      }
      const image = fieldContext.createImageData(imageWidth, imageHeight)
      for (let y = 0; y < imageHeight; y += 1) {
        const worldZ = view.top + (y + 0.5) / imageHeight * view.worldHeight
        for (let x = 0; x < imageWidth; x += 1) {
          const worldX = view.left + (x + 0.5) / imageWidth * view.worldWidth
          const field = props.world.sampleShapeWorld(worldX, worldZ)
          const index = (y * imageWidth + x) * 4
          const band = Math.abs(((field * 24) % 1 + 1) % 1 - 0.5) < 0.045 ? 5 : 0
          if (field >= 0) {
            const elevation = Math.min(1, field / 0.5)
            image.data[index] = 40 + elevation * 19 + band
            image.data[index + 1] = 61 + elevation * 24 + band
            image.data[index + 2] = 46 + elevation * 14 + band * 0.5
          } else {
            const depth = Math.min(1, -field / 0.65)
            image.data[index] = 12 + (1 - depth) * 7
            image.data[index + 1] = 28 + (1 - depth) * 13 + band * 0.25
            image.data[index + 2] = 33 + (1 - depth) * 14 + band * 0.4
          }
          image.data[index + 3] = 255
        }
      }
      fieldContext.putImageData(image, 0, 0)
      dirtyField = false
    }

    const traceContours = (level: number) => {
      const view = metrics()
      const resolution = 113
      const values = new Float32Array(resolution * resolution)
      for (let y = 0; y < resolution; y += 1) {
        for (let x = 0; x < resolution; x += 1) {
          values[y * resolution + x] = props.world.sampleShapeWorld(
            view.left + x / (resolution - 1) * view.worldWidth,
            view.top + y / (resolution - 1) * view.worldHeight,
          )
        }
      }
      context.beginPath()
      for (let y = 0; y < resolution - 1; y += 1) {
        for (let x = 0; x < resolution - 1; x += 1) {
          const v0 = values[y * resolution + x]
          const v1 = values[y * resolution + x + 1]
          const v2 = values[(y + 1) * resolution + x + 1]
          const v3 = values[(y + 1) * resolution + x]
          const crossings: Array<[number, number]> = []
          const edge = (a: number, b: number, ax: number, ay: number, bx: number, by: number) => {
            if ((a < level && b >= level) || (b < level && a >= level)) {
              const t = (level - a) / (b - a)
              crossings.push([ax + (bx - ax) * t, ay + (by - ay) * t])
            }
          }
          edge(v0, v1, x, y, x + 1, y)
          edge(v1, v2, x + 1, y, x + 1, y + 1)
          edge(v2, v3, x + 1, y + 1, x, y + 1)
          edge(v3, v0, x, y + 1, x, y)
          const segment = (a: [number, number], b: [number, number]) => {
            context.moveTo(a[0] / (resolution - 1) * width, a[1] / (resolution - 1) * height)
            context.lineTo(b[0] / (resolution - 1) * width, b[1] / (resolution - 1) * height)
          }
          if (crossings.length === 2) segment(crossings[0], crossings[1])
          else if (crossings.length === 4) {
            segment(crossings[0], crossings[1])
            segment(crossings[2], crossings[3])
          }
        }
      }
      context.stroke()
    }

    const draw = () => {
      frame = 0
      if (dirtyField) rebuildField()
      const dpr = Math.min(window.devicePixelRatio, 2)
      if (canvas.width !== Math.round(width * dpr) || canvas.height !== Math.round(height * dpr)) {
        canvas.width = Math.round(width * dpr)
        canvas.height = Math.round(height * dpr)
      }
      context.setTransform(dpr, 0, 0, dpr, 0, 0)
      context.fillStyle = '#0b1214'
      context.fillRect(0, 0, width, height)
      context.imageSmoothingEnabled = true
      context.drawImage(fieldCanvas, 0, 0, width, height)
      const view = metrics()

      if (propsRef.current.showGrid) {
        const minor = SDF_CHUNK_SIZE / 4
        const startX = Math.floor(view.left / minor) * minor
        const startZ = Math.floor(view.top / minor) * minor
        for (let worldX = startX; worldX <= view.left + view.worldWidth + minor; worldX += minor) {
          const x = (worldX - view.left) * view.scale
          const major = Math.abs(worldX / SDF_CHUNK_SIZE - Math.round(worldX / SDF_CHUNK_SIZE)) < 0.001
          context.strokeStyle = major ? 'rgba(163,193,175,.18)' : 'rgba(142,170,157,.05)'
          context.lineWidth = major ? 0.9 : 0.55
          context.beginPath(); context.moveTo(x, 0); context.lineTo(x, height); context.stroke()
        }
        for (let worldZ = startZ; worldZ <= view.top + view.worldHeight + minor; worldZ += minor) {
          const y = (worldZ - view.top) * view.scale
          const major = Math.abs(worldZ / SDF_CHUNK_SIZE - Math.round(worldZ / SDF_CHUNK_SIZE)) < 0.001
          context.strokeStyle = major ? 'rgba(163,193,175,.18)' : 'rgba(142,170,157,.05)'
          context.lineWidth = major ? 0.9 : 0.55
          context.beginPath(); context.moveTo(0, y); context.lineTo(width, y); context.stroke()
        }
      }

      for (const level of [-0.3, -0.15, 0, 0.1, 0.2, 0.35, 0.5]) {
        context.strokeStyle = level === 0 ? 'rgba(204,241,153,.95)' : level < 0 ? 'rgba(93,151,153,.28)' : 'rgba(163,190,145,.25)'
        context.lineWidth = level === 0 ? 1.6 : 0.75
        traceContours(level)
      }

      context.fillStyle = '#677a70'
      context.font = '8px "IBM Plex Mono", monospace'
      context.textAlign = 'left'
      const chunkStartX = Math.floor(view.left / SDF_CHUNK_SIZE)
      const chunkEndX = Math.ceil((view.left + view.worldWidth) / SDF_CHUNK_SIZE)
      for (let cx = chunkStartX; cx <= chunkEndX; cx += 1) {
        const point = worldToScreen(cx * SDF_CHUNK_SIZE, 0)
        if (point.x > 5 && point.x < width - 30) context.fillText(`C${cx}`, point.x + 5, 66)
      }
      const chunkStartZ = Math.floor(view.top / SDF_CHUNK_SIZE)
      const chunkEndZ = Math.ceil((view.top + view.worldHeight) / SDF_CHUNK_SIZE)
      for (let cz = chunkStartZ; cz <= chunkEndZ; cz += 1) {
        const point = worldToScreen(0, cz * SDF_CHUNK_SIZE)
        if (point.y > 80 && point.y < height - 20) context.fillText(`R${cz}`, 8, point.y - 5)
      }

      const origin = worldToScreen(0, 0)
      if (origin.x > 0 && origin.x < width && origin.y > 0 && origin.y < height) {
        context.strokeStyle = 'rgba(200,236,146,.26)'
        context.beginPath(); context.moveTo(origin.x - 6, origin.y); context.lineTo(origin.x + 6, origin.y); context.moveTo(origin.x, origin.y - 6); context.lineTo(origin.x, origin.y + 6); context.stroke()
      }

      const mouse = mouseRef.current
      if (mouse.inside) {
        const point = worldToScreen(mouse.worldX, mouse.worldZ)
        const radius = propsRef.current.brushSize / METERS_PER_SCENE_UNIT * view.scale
        context.fillStyle = propsRef.current.tool === 'subtract' ? 'rgba(241,142,112,.055)' : 'rgba(207,242,155,.055)'
        context.strokeStyle = propsRef.current.tool === 'subtract' ? 'rgba(241,142,112,.9)' : 'rgba(207,242,155,.9)'
        context.lineWidth = 1.2
        context.beginPath(); context.arc(point.x, point.y, radius, 0, Math.PI * 2); context.fill(); context.stroke()
        context.strokeStyle = 'rgba(220,239,206,.55)'
        context.beginPath(); context.moveTo(point.x - 4, point.y); context.lineTo(point.x + 4, point.y); context.moveTo(point.x, point.y - 4); context.lineTo(point.x, point.y + 4); context.stroke()
      }

      const scaleLength = 2048 / METERS_PER_SCENE_UNIT * view.scale
      const scaleX = 26
      const scaleY = height - 32
      context.strokeStyle = 'rgba(191,211,199,.65)'
      context.beginPath(); context.moveTo(scaleX, scaleY - 4); context.lineTo(scaleX, scaleY); context.lineTo(scaleX + scaleLength, scaleY); context.lineTo(scaleX + scaleLength, scaleY - 4); context.stroke()
      context.fillStyle = '#9aaba2'
      context.font = '9px "IBM Plex Mono", monospace'
      context.fillText('2 km', scaleX, scaleY - 9)
      context.fillStyle = '#63776c'
      context.font = '8px "IBM Plex Mono", monospace'
      context.fillText(`ORIGIN  ${Math.round(viewRef.current.centerX * METERS_PER_SCENE_UNIT).toLocaleString()}  /  ${Math.round(viewRef.current.centerZ * METERS_PER_SCENE_UNIT).toLocaleString()} m`, 26, 54)
    }

    const invalidate = (fieldChanged = false) => {
      if (fieldChanged) dirtyField = true
      if (!frame) frame = requestAnimationFrame(draw)
    }
    drawRef.current = () => invalidate(true)

    const reportView = () => {
      propsRef.current.onViewChange?.({
        x: viewRef.current.centerX * METERS_PER_SCENE_UNIT,
        z: viewRef.current.centerZ * METERS_PER_SCENE_UNIT,
      })
    }

    const resize = () => {
      const rect = canvas.getBoundingClientRect()
      width = Math.max(1, rect.width)
      height = Math.max(1, rect.height)
      invalidate(true)
    }
    const resizeObserver = new ResizeObserver(resize)
    resizeObserver.observe(canvas)
    resize()
    const unsubscribe = props.world.subscribe((change) => {
      if (change.type === 'shape' || change.type === 'all') invalidate(true)
    })

    const eventPoint = (event: MouseEvent | PointerEvent | WheelEvent) => {
      const rect = canvas.getBoundingClientRect()
      const x = event.clientX - rect.left
      const y = event.clientY - rect.top
      return { x, y, ...screenToWorld(x, y) }
    }

    const pointerDown = (event: PointerEvent) => {
      const point = eventPoint(event)
      const pan = event.button === 1 || event.button === 2 || event.altKey || spaceRef.current
      event.preventDefault()
      canvas.setPointerCapture(event.pointerId)
      interactionRef.current = { mode: pan ? 'pan' : 'draw', pointerId: event.pointerId, lastX: event.clientX, lastY: event.clientY, lastWorldX: point.worldX, lastWorldZ: point.worldZ }
      if (!pan) {
        propsRef.current.world.beginStroke()
        propsRef.current.world.applyShapeBrushWorld(point.worldX, point.worldZ, propsRef.current.brushSize / METERS_PER_SCENE_UNIT, event.shiftKey ? 'subtract' : propsRef.current.tool, propsRef.current.strength, propsRef.current.falloff, propsRef.current.erosion)
        invalidate(true)
      }
    }

    const pointerMove = (event: PointerEvent) => {
      const point = eventPoint(event)
      mouseRef.current = { x: point.x, y: point.y, worldX: point.worldX, worldZ: point.worldZ, inside: true }
      propsRef.current.onCursor?.({ x: point.worldX * METERS_PER_SCENE_UNIT, z: point.worldZ * METERS_PER_SCENE_UNIT })
      const interaction = interactionRef.current
      if (interaction) {
        if (interaction.mode === 'pan') {
          const view = metrics()
          viewRef.current.centerX -= (event.clientX - interaction.lastX) / view.scale
          viewRef.current.centerZ -= (event.clientY - interaction.lastY) / view.scale
          reportView()
        } else {
          const distance = Math.hypot(point.worldX - interaction.lastWorldX, point.worldZ - interaction.lastWorldZ)
          const stepWorld = propsRef.current.brushSize / METERS_PER_SCENE_UNIT * 0.2
          const steps = Math.max(1, Math.ceil(distance / stepWorld))
          for (let i = 1; i <= steps; i += 1) {
            const t = i / steps
            propsRef.current.world.applyShapeBrushWorld(
              interaction.lastWorldX + (point.worldX - interaction.lastWorldX) * t,
              interaction.lastWorldZ + (point.worldZ - interaction.lastWorldZ) * t,
              propsRef.current.brushSize / METERS_PER_SCENE_UNIT,
              event.shiftKey ? 'subtract' : propsRef.current.tool,
              propsRef.current.strength,
              propsRef.current.falloff,
              propsRef.current.erosion,
            )
          }
        }
        interaction.lastX = event.clientX
        interaction.lastY = event.clientY
        interaction.lastWorldX = point.worldX
        interaction.lastWorldZ = point.worldZ
      }
      invalidate(true)
    }

    const pointerUp = (event: PointerEvent) => {
      const interaction = interactionRef.current
      if (!interaction) return
      if (interaction.mode === 'draw') {
        propsRef.current.world.finishShapeStroke()
        propsRef.current.world.endStroke('Eroded continental SDF stroke')
      } else {
        propsRef.current.world.pruneShapeChunks(viewRef.current.centerX, viewRef.current.centerZ)
      }
      interactionRef.current = null
      if (canvas.hasPointerCapture(event.pointerId)) canvas.releasePointerCapture(event.pointerId)
      invalidate(true)
    }

    const pointerLeave = () => {
      if (!interactionRef.current) {
        mouseRef.current.inside = false
        propsRef.current.onCursor?.(null)
        invalidate()
      }
    }

    const wheel = (event: WheelEvent) => {
      event.preventDefault()
      const point = eventPoint(event)
      const newZoom = Math.max(0.08, Math.min(12, viewRef.current.zoom * Math.exp(-event.deltaY * 0.001)))
      const newScale = Math.max(0.001, Math.min(width, height) / 160 * newZoom)
      viewRef.current.centerX = point.worldX - (point.x - width / 2) / newScale
      viewRef.current.centerZ = point.worldZ - (point.y - height / 2) / newScale
      viewRef.current.zoom = newZoom
      setZoomLabel(Math.round(newZoom * 100))
      reportView()
      invalidate(true)
    }

    const keyDown = (event: KeyboardEvent) => {
      if (event.code === 'Space' && !(event.target instanceof HTMLInputElement)) {
        spaceRef.current = true
        setSpaceDown(true)
        canvas.style.cursor = 'grab'
      }
    }
    const keyUp = (event: KeyboardEvent) => {
      if (event.code === 'Space') {
        spaceRef.current = false
        setSpaceDown(false)
        canvas.style.cursor = 'crosshair'
      }
    }
    const preventMenu = (event: Event) => event.preventDefault()

    canvas.addEventListener('pointerdown', pointerDown)
    canvas.addEventListener('pointermove', pointerMove)
    canvas.addEventListener('pointerup', pointerUp)
    canvas.addEventListener('pointercancel', pointerUp)
    canvas.addEventListener('pointerleave', pointerLeave)
    canvas.addEventListener('wheel', wheel, { passive: false })
    canvas.addEventListener('contextmenu', preventMenu)
    window.addEventListener('keydown', keyDown)
    window.addEventListener('keyup', keyUp)
    return () => {
      if (frame) cancelAnimationFrame(frame)
      resizeObserver.disconnect()
      unsubscribe()
      canvas.removeEventListener('pointerdown', pointerDown)
      canvas.removeEventListener('pointermove', pointerMove)
      canvas.removeEventListener('pointerup', pointerUp)
      canvas.removeEventListener('pointercancel', pointerUp)
      canvas.removeEventListener('pointerleave', pointerLeave)
      canvas.removeEventListener('wheel', wheel)
      canvas.removeEventListener('contextmenu', preventMenu)
      window.removeEventListener('keydown', keyDown)
      window.removeEventListener('keyup', keyUp)
      drawRef.current = () => {}
    }
  }, [props.world])

  useEffect(() => { drawRef.current() }, [props.tool, props.brushSize, props.strength, props.falloff, props.erosion, props.showGrid])

  return (
    <div className="shape-canvas-wrap">
      <canvas ref={canvasRef} className="shape-canvas" style={{ cursor: spaceDown ? 'grab' : 'crosshair' }} />
      <div className="shape-zoom-label">∞ SPARSE&nbsp;&nbsp;·&nbsp;&nbsp;{zoomLabel}%</div>
      <div className="shape-axis shape-axis-n">N</div>
    </div>
  )
})
