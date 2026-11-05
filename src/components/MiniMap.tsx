import { useEffect, useRef } from 'react'
import { METERS_PER_SCENE_UNIT, SDF_CHUNK_SIZE, STREAM_RADIUS, TerrainWorld } from '../terrain/TerrainWorld'

export function MiniMap({ world, center = { x: 0, z: 0 }, compact = false }: {
  world: TerrainWorld
  center?: { x: number; z: number }
  compact?: boolean
}) {
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const centerRef = useRef(center)
  centerRef.current = center

  useEffect(() => {
    const canvas = canvasRef.current!
    const context = canvas.getContext('2d')!
    let frame = 0
    const draw = () => {
      frame = 0
      const rect = canvas.getBoundingClientRect()
      const dpr = Math.min(window.devicePixelRatio, 2)
      const width = Math.max(1, rect.width)
      const height = Math.max(1, rect.height)
      canvas.width = Math.round(width * dpr)
      canvas.height = Math.round(height * dpr)
      context.setTransform(dpr, 0, 0, dpr, 0, 0)
      const resolution = 112
      const image = context.createImageData(resolution, resolution)
      const coverage = SDF_CHUNK_SIZE * (STREAM_RADIUS * 2 + 1)
      const centerX = centerRef.current.x / METERS_PER_SCENE_UNIT
      const centerZ = centerRef.current.z / METERS_PER_SCENE_UNIT
      for (let y = 0; y < resolution; y += 1) {
        for (let x = 0; x < resolution; x += 1) {
          const field = world.sampleShapeWorld(
            centerX + (x / (resolution - 1) - 0.5) * coverage,
            centerZ + (y / (resolution - 1) - 0.5) * coverage,
          )
          const index = (y * resolution + x) * 4
          if (field < 0) {
            const shallow = Math.max(0, 1 + field * 4)
            image.data[index] = 13 + shallow * 15
            image.data[index + 1] = 40 + shallow * 48
            image.data[index + 2] = 49 + shallow * 43
          } else {
            const inland = Math.min(1, field / 0.5)
            image.data[index] = 70 + inland * 48
            image.data[index + 1] = 94 + inland * 48
            image.data[index + 2] = 59 + inland * 30
          }
          image.data[index + 3] = 255
        }
      }
      const imageCanvas = document.createElement('canvas')
      imageCanvas.width = imageCanvas.height = resolution
      imageCanvas.getContext('2d')!.putImageData(image, 0, 0)
      context.imageSmoothingEnabled = true
      context.drawImage(imageCanvas, 0, 0, width, height)
      context.strokeStyle = 'rgba(181,211,197,.13)'
      context.lineWidth = 0.6
      for (let i = 1; i < STREAM_RADIUS * 2 + 1; i += 1) {
        context.beginPath()
        context.moveTo(i / (STREAM_RADIUS * 2 + 1) * width, 0)
        context.lineTo(i / (STREAM_RADIUS * 2 + 1) * width, height)
        context.moveTo(0, i / (STREAM_RADIUS * 2 + 1) * height)
        context.lineTo(width, i / (STREAM_RADIUS * 2 + 1) * height)
        context.stroke()
      }
      if (!compact) {
        context.strokeStyle = 'rgba(216,240,190,.72)'
        context.lineWidth = 1
        context.strokeRect(width * 0.36, height * 0.3, width * 0.28, height * 0.4)
        context.fillStyle = 'rgba(216,240,190,.8)'
        context.fillRect(width / 2 - 1, height / 2 - 1, 2, 2)
      }
    }
    const invalidate = () => { if (!frame) frame = requestAnimationFrame(draw) }
    const observer = new ResizeObserver(invalidate)
    observer.observe(canvas)
    const unsubscribe = world.subscribe(invalidate)
    invalidate()
    return () => {
      if (frame) cancelAnimationFrame(frame)
      observer.disconnect()
      unsubscribe()
    }
  }, [world, compact, center.x, center.z])

  return <canvas className="minimap-canvas" ref={canvasRef} />
}
