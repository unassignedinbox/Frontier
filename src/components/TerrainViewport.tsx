import { forwardRef, useEffect, useImperativeHandle, useRef, useState } from 'react'
import { TerrainWorld } from '../terrain/TerrainWorld'
import { ViewportEngine } from '../terrain/ViewportEngine'
import type { CameraView, CursorLocation, DisplayMode, ViewportSettings, ViewportStats } from '../terrain/ViewportEngine'
import type { SculptTool } from '../terrain/TerrainWorld'

export interface TerrainViewportHandle {
  resetCamera: () => void
  setCameraView: (view: CameraView) => void
  zoom: (factor: number) => void
  setWorldCenter: (center: { x: number; z: number }) => void
  capture: () => string
}

interface TerrainViewportProps {
  world: TerrainWorld
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
  onStats?: (stats: ViewportStats) => void
  onCursor?: (cursor: CursorLocation | null) => void
  onViewChange?: (center: { x: number; z: number }) => void
}

export const TerrainViewport = forwardRef<TerrainViewportHandle, TerrainViewportProps>(function TerrainViewport(props, ref) {
  const containerRef = useRef<HTMLDivElement>(null)
  const engineRef = useRef<ViewportEngine | null>(null)
  const callbacksRef = useRef(props)
  callbacksRef.current = props
  const [ready, setReady] = useState(false)
  const [error, setError] = useState('')

  useEffect(() => {
    if (!containerRef.current) return
    const settings: ViewportSettings = {
      stage: callbacksRef.current.stage,
      tool: callbacksRef.current.tool,
      brushSize: callbacksRef.current.brushSize,
      strength: callbacksRef.current.strength,
      falloff: callbacksRef.current.falloff,
      symmetryX: callbacksRef.current.symmetryX,
      symmetryZ: callbacksRef.current.symmetryZ,
      displayMode: callbacksRef.current.displayMode,
      showGrid: callbacksRef.current.showGrid,
      oceanVisible: callbacksRef.current.oceanVisible,
      seaLevel: callbacksRef.current.seaLevel,
    }
    try {
      engineRef.current = new ViewportEngine(containerRef.current, props.world, settings, {
        onStats: (stats) => callbacksRef.current.onStats?.(stats),
        onCursor: (cursor) => callbacksRef.current.onCursor?.(cursor),
        onViewChange: (center) => callbacksRef.current.onViewChange?.(center),
      })
      setReady(true)
    } catch (error) {
      console.error('Unable to initialize the terrain viewport', error)
      setError('WebGL 2 could not be started. Enable hardware acceleration or try a current desktop browser.')
    }
    return () => {
      engineRef.current?.dispose()
      engineRef.current = null
    }
  }, [props.world])

  useEffect(() => {
    engineRef.current?.applySettings({
      stage: props.stage,
      tool: props.tool,
      brushSize: props.brushSize,
      strength: props.strength,
      falloff: props.falloff,
      symmetryX: props.symmetryX,
      symmetryZ: props.symmetryZ,
      displayMode: props.displayMode,
      showGrid: props.showGrid,
      oceanVisible: props.oceanVisible,
      seaLevel: props.seaLevel,
    })
  }, [props.stage, props.tool, props.brushSize, props.strength, props.falloff, props.symmetryX, props.symmetryZ, props.displayMode, props.showGrid, props.oceanVisible, props.seaLevel])

  useImperativeHandle(ref, () => ({
    resetCamera: () => engineRef.current?.setCameraView('perspective'),
    setCameraView: (view) => engineRef.current?.setCameraView(view),
    zoom: (factor) => engineRef.current?.zoom(factor),
    setWorldCenter: (center) => engineRef.current?.setWorldCenter(center),
    capture: () => engineRef.current?.capture() ?? '',
  }), [])

  return (
    <div className="terrain-viewport-engine" ref={containerRef}>
      {!ready && !error && <div className="viewport-loading"><span className="loading-spinner" />Building terrain tiles…</div>}
      {error && <div className="viewport-loading viewport-error"><svg viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor"><path d="m3 19 6-11 4 6 2-3 6 8H3Z" /></svg><span>{error}</span></div>}
    </div>
  )
})
