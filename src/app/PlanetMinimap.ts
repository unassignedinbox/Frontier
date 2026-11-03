import { projectOrthographic, unprojectOrthographic, type RGB } from '../world/math';
import type { SdfWorld } from '../world/SdfWorld';
import type { WorldFrame } from './WorldFrame';

/** Orthographic planet map generated from the same SDF as the 3D world. */
export class PlanetMinimap {
  canvas: HTMLCanvasElement;
  onTeleport?: (x: number, z: number) => void;

  private ctx: CanvasRenderingContext2D;
  private base: HTMLCanvasElement;
  private baseCtx: CanvasRenderingContext2D;
  private world: SdfWorld;
  private frame: WorldFrame;
  private dirty = true;
  private cssW = 0;
  private cssH = 0;
  private dpr = 1;
  private brush: { x: number; z: number; radius: number } | null = null;

  constructor(canvas: HTMLCanvasElement, world: SdfWorld, frame: WorldFrame) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d')!;
    this.world = world;
    this.frame = frame;
    this.base = document.createElement('canvas');
    this.baseCtx = this.base.getContext('2d')!;
    this.canvas.addEventListener('click', this.onClick);
    window.addEventListener('resize', this.markDirty);
    this.ensureSize();
  }

  setWorld(world: SdfWorld): void {
    this.world = world;
    this.markDirty();
  }

  setFrame(frame: WorldFrame): void { this.frame = frame; }

  markDirty = (): void => { this.dirty = true; };

  setBrush(x: number, z: number, radius: number): void { this.brush = { x, z, radius }; }
  clearBrush(): void { this.brush = null; }

  private ensureSize(): void {
    const w = Math.max(160, this.canvas.clientWidth || 240);
    const h = Math.max(160, this.canvas.clientHeight || 240);
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    if (w === this.cssW && h === this.cssH && dpr === this.dpr) return;
    this.cssW = w;
    this.cssH = h;
    this.dpr = dpr;
    this.canvas.width = Math.round(w * dpr);
    this.canvas.height = Math.round(h * dpr);
    this.base.width = this.canvas.width;
    this.base.height = this.canvas.height;
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.baseCtx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.dirty = true;
  }

  private globe(): { cx: number; cy: number; r: number } {
    return { cx: this.cssW / 2, cy: this.cssH / 2, r: Math.min(this.cssW, this.cssH) * 0.43 };
  }

  private toRGB(c: RGB, shade: number): [number, number, number] {
    return [
      Math.round(Math.min(1, c[0] * shade) * 255),
      Math.round(Math.min(1, c[1] * shade) * 255),
      Math.round(Math.min(1, c[2] * shade) * 255),
    ];
  }

  private rebuildBase(): void {
    const w = this.cssW;
    const h = this.cssH;
    const { cx, cy, r } = this.globe();
    const ctx = this.baseCtx;
    ctx.clearRect(0, 0, w, h);

    // Space / atmosphere backdrop.
    const bg = ctx.createRadialGradient(cx, cy, r * 0.72, cx, cy, r * 1.35);
    bg.addColorStop(0, '#0a1626');
    bg.addColorStop(0.65, '#07101c');
    bg.addColorStop(1, '#050a12');
    ctx.fillStyle = bg;
    ctx.fillRect(0, 0, w, h);

    // Cap the SDF sampling resolution; the enlarged panel is just a scaled blit.
    const size = Math.min(512, Math.max(160, Math.round(r * 2 * this.dpr)));
    const img = ctx.createImageData(size, size);
    const pxCenter = size / 2;
    for (let py = 0; py < img.height; py++) {
      for (let px = 0; px < size; px++) {
        const nx = (px - pxCenter) / pxCenter;
        const ny = (pxCenter - py) / pxCenter;
        const rho = Math.hypot(nx, ny);
        const idx = (px + py * size) * 4;
        if (rho > 1) { img.data[idx + 3] = 0; continue; }
        const geo = unprojectOrthographic(nx, ny, this.world.def.originLon, this.world.def.originLat);
        if (!geo) { img.data[idx + 3] = 0; continue; }
        const c = this.world.mapColorAt(geo.lon, geo.lat);
        const shade = 0.58 + 0.48 * Math.sqrt(Math.max(0, 1 - rho * rho));
        const rgb = this.toRGB(c, shade);
        img.data[idx] = rgb[0];
        img.data[idx + 1] = rgb[1];
        img.data[idx + 2] = rgb[2];
        img.data[idx + 3] = 255;
      }
    }
    const globeCanvas = document.createElement('canvas');
    globeCanvas.width = size;
    globeCanvas.height = img.height;
    globeCanvas.getContext('2d')!.putImageData(img, 0, 0);
    ctx.drawImage(globeCanvas, cx - r, cy - r, r * 2, r * 2);

    // Lat/lon graticule on the visible hemisphere.
    ctx.save();
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.clip();
    ctx.strokeStyle = 'rgba(160, 205, 230, 0.16)';
    ctx.lineWidth = 1;
    const lon0 = this.world.def.originLon;
    const lat0 = this.world.def.originLat;
    for (let lon = -180; lon < 180; lon += 15) {
      ctx.beginPath();
      let started = false;
      for (let lat = -89; lat <= 89; lat += 2) {
        const p = projectOrthographic(lon, lat, lon0, lat0);
        const x = cx + p.x * r;
        const y = cy - p.y * r;
        if (!p.visible) { started = false; continue; }
        if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
      }
      ctx.stroke();
    }
    for (let lat = -75; lat <= 75; lat += 15) {
      ctx.beginPath();
      let started = false;
      for (let lon = -180; lon <= 180; lon += 2) {
        const p = projectOrthographic(lon, lat, lon0, lat0);
        const x = cx + p.x * r;
        const y = cy - p.y * r;
        if (!p.visible) { started = false; continue; }
        if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
      }
      ctx.stroke();
    }
    ctx.restore();

    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(130, 205, 255, 0.55)';
    ctx.lineWidth = 1.5;
    ctx.stroke();

    ctx.fillStyle = 'rgba(210, 235, 255, 0.78)';
    ctx.font = '10px ui-monospace, monospace';
    ctx.fillText('PLANET BLOCKOUT', 10, 16);
    ctx.fillStyle = 'rgba(150, 190, 215, 0.7)';
    ctx.fillText('click globe to teleport', 10, h - 10);
    this.dirty = false;
  }

  private projectWorld(x: number, z: number): { x: number; y: number; visible: boolean } | null {
    const geo = this.world.localToGeo(x, z);
    const { cx, cy, r } = this.globe();
    const p = projectOrthographic(geo.lon, geo.lat, this.world.def.originLon, this.world.def.originLat);
    if (!p.visible) return null;
    return { x: cx + p.x * r, y: cy - p.y * r, visible: true };
  }

  render(): void {
    this.ensureSize();
    if (this.dirty) this.rebuildBase();
    const ctx = this.ctx;
    const w = this.cssW;
    const h = this.cssH;
    ctx.clearRect(0, 0, w, h);
    ctx.drawImage(this.base, 0, 0, this.base.width, this.base.height, 0, 0, w, h);

    const cam = this.frame.cameraWorld;
    const camP = this.projectWorld(cam.x, cam.z);
    const { r } = this.globe();
    if (camP) {
      ctx.beginPath();
      ctx.arc(camP.x, camP.y, 7, 0, Math.PI * 2);
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.35)';
      ctx.lineWidth = 1;
      ctx.stroke();
      ctx.beginPath();
      ctx.arc(camP.x, camP.y, 3.2, 0, Math.PI * 2);
      ctx.fillStyle = '#ffe082';
      ctx.fill();
      ctx.strokeStyle = 'rgba(20, 30, 40, 0.9)';
      ctx.lineWidth = 1.2;
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(camP.x, camP.y - 10);
      ctx.lineTo(camP.x, camP.y - 18);
      ctx.strokeStyle = 'rgba(255, 224, 130, 0.85)';
      ctx.stroke();
    }

    if (this.brush) {
      const p = this.projectWorld(this.brush.x, this.brush.z);
      if (p) {
        const visualRadius = Math.max(3, Math.min(26, (this.brush.radius / 6_371_000) * r * 8));
        ctx.beginPath();
        ctx.arc(p.x, p.y, visualRadius, 0, Math.PI * 2);
        ctx.strokeStyle = 'rgba(255, 224, 130, 0.9)';
        ctx.setLineDash([3, 3]);
        ctx.stroke();
        ctx.setLineDash([]);
      }
    }
  }

  private onClick = (event: MouseEvent) => {
    const rect = this.canvas.getBoundingClientRect();
    const { cx, cy, r } = this.globe();
    const nx = (event.clientX - rect.left - cx) / r;
    const ny = (cy - (event.clientY - rect.top)) / r;
    const geo = unprojectOrthographic(nx, ny, this.world.def.originLon, this.world.def.originLat);
    if (!geo) return;
    const [x, z] = this.world.geoToLocal(geo.lon, geo.lat);
    this.onTeleport?.(x, z);
  };

  dispose(): void {
    this.canvas.removeEventListener('click', this.onClick);
    window.removeEventListener('resize', this.markDirty);
  }
}
