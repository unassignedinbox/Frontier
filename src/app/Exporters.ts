import type * as THREE from 'three';
import { clamp } from '../world/math';
import { SdfWorld } from '../world/SdfWorld';
import type { WorldFrame } from './WorldFrame';
import type { TerrainStreamer } from './TerrainStreamer';
import type { SdfHeroChunk } from './SdfHeroChunk';

function downloadBlob(blob: Blob, filename: string): void {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 2000);
}

export function downloadText(text: string, filename: string, type = 'application/json'): void {
  downloadBlob(new Blob([text], { type }), filename);
}

export function exportSDFJSON(world: SdfWorld): void {
  downloadText(JSON.stringify(world.toJSON(), null, 2), `${world.def.name.replace(/\s+/g, '-').toLowerCase()}.sdf.json`);
}

export async function importSDFJSON(file: File): Promise<SdfWorld> {
  const text = await file.text();
  return SdfWorld.fromJSON(JSON.parse(text));
}

/**
 * Exports a 16-bit-ish heightmap PNG plus a metadata sidecar.
 * R/G = height normalised into 16 bits, B = land mask, A = 255.
 * The PNG is a derived cache; the SDF JSON remains the editable source.
 */
export async function exportHeightmapBundle(world: SdfWorld, frame: WorldFrame, extentKm = 200, resolution = 1024): Promise<void> {
  const extent = extentKm * 1000;
  const minH = -5200;
  const maxH = 4200;
  const canvas = document.createElement('canvas');
  canvas.width = resolution;
  canvas.height = resolution;
  const ctx = canvas.getContext('2d')!;
  const img = ctx.createImageData(resolution, resolution);
  const cx = frame.cameraWorld.x;
  const cz = frame.cameraWorld.z;
  for (let j = 0; j < resolution; j++) {
    const z = cz + (j / (resolution - 1) - 0.5) * extent;
    for (let i = 0; i < resolution; i++) {
      const x = cx + (i / (resolution - 1) - 0.5) * extent;
      const h = world.heightAt(x, z);
      const q = Math.round(clamp((h - minH) / (maxH - minH), 0, 1) * 65535);
      const land = world.landSdf(x, z) < 0 ? 255 : 0;
      const p = (i + j * resolution) * 4;
      img.data[p] = (q >> 8) & 255;
      img.data[p + 1] = q & 255;
      img.data[p + 2] = land;
      img.data[p + 3] = 255;
    }
  }
  ctx.putImageData(img, 0, 0);
  const blob = await new Promise<Blob | null>(resolve => canvas.toBlob(resolve, 'image/png'));
  if (!blob) throw new Error('Could not encode heightmap PNG');
  const stem = `${world.def.name.replace(/\s+/g, '-').toLowerCase()}-heightmap`;
  downloadBlob(blob, `${stem}.png`);
  downloadText(JSON.stringify({
    encoding: 'RG16 height, B land mask',
    minHeight: minH,
    maxHeight: maxH,
    seaLevel: world.seaLevel,
    extentMetres: extent,
    centreWorldX: cx,
    centreWorldZ: cz,
    resolution,
    cellSizeMetres: extent / (resolution - 1),
    source: 'Frontier SdfWorld.heightAt projection',
  }, null, 2), `${stem}.meta.json`);
}

/** Bakes the currently visible SDF-derived terrain meshes to a Wavefront OBJ. */
export function exportVisibleTerrainOBJ(terrain: TerrainStreamer, hero: SdfHeroChunk, frame: WorldFrame): void {
  const lines: string[] = ['# Frontier SDF-derived terrain bake', '# units: metres, Y up'];
  const cam = frame.cameraWorld;
  let vertexOffset = 1;

  const writeGeometry = (geometry: THREE.BufferGeometry, mesh: THREE.Mesh, label: string) => {
    const pos = geometry.getAttribute('position');
    const index = geometry.getIndex();
    lines.push(`g ${label.replace(/\s+/g, '_')}`);
    for (let i = 0; i < pos.count; i++) {
      const x = pos.getX(i) + mesh.position.x + cam.x;
      const y = pos.getY(i) + mesh.position.y + cam.y;
      const z = pos.getZ(i) + mesh.position.z + cam.z;
      lines.push(`v ${x.toFixed(3)} ${y.toFixed(3)} ${z.toFixed(3)}`);
    }
    const count = index ? index.count : pos.count;
    for (let i = 0; i < count; i += 3) {
      const a = (index ? index.getX(i) : i) + vertexOffset;
      const b = (index ? index.getX(i + 1) : i + 1) + vertexOffset;
      const c = (index ? index.getX(i + 2) : i + 2) + vertexOffset;
      lines.push(`f ${a} ${b} ${c}`);
    }
    vertexOffset += pos.count;
  };

  for (const ring of terrain.rings as any[]) writeGeometry(ring.mesh.geometry, ring.mesh, ring.mesh.name);
  if (hero.ready && hero.enabled) writeGeometry(hero.mesh.geometry, hero.mesh, hero.mesh.name);
  downloadText(lines.join('\n'), 'frontier-terrain.obj', 'text/plain');
}
