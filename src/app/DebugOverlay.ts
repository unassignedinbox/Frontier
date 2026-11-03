import * as THREE from 'three';
import type { WorldFrame } from './WorldFrame';
import type { RGB } from '../world/math';

export interface LodExtent {
  half: number;
  label: string;
  color: RGB;
}

/** Camera-centred editor gizmos: brush footprint and adaptive LOD boundaries. */
export class DebugOverlay {
  brush: THREE.LineLoop;
  lodLines: THREE.LineSegments;
  showLod = true;

  private frame: WorldFrame;
  private lodGeometry: THREE.BufferGeometry;

  constructor(scene: THREE.Scene, frame: WorldFrame) {
    this.frame = frame;
    const circle: number[] = [];
    const segments = 96;
    for (let i = 0; i < segments; i++) {
      const a = (i / segments) * Math.PI * 2;
      circle.push(Math.cos(a), 0, Math.sin(a));
    }
    const brushGeometry = new THREE.BufferGeometry();
    brushGeometry.setAttribute('position', new THREE.Float32BufferAttribute(circle, 3));
    this.brush = new THREE.LineLoop(
      brushGeometry,
      new THREE.LineBasicMaterial({ color: 0xffe082, transparent: true, opacity: 0.98, depthTest: false }),
    );
    this.brush.renderOrder = 20;
    this.brush.frustumCulled = false;
    this.brush.visible = false;
    scene.add(this.brush);

    this.lodGeometry = new THREE.BufferGeometry();
    this.lodLines = new THREE.LineSegments(
      this.lodGeometry,
      new THREE.LineBasicMaterial({ vertexColors: true, transparent: true, opacity: 0.7, depthTest: false }),
    );
    this.lodLines.renderOrder = 19;
    this.lodLines.frustumCulled = false;
    scene.add(this.lodLines);
  }

  setExtents(extents: LodExtent[]): void {
    const positions: number[] = [];
    const colors: number[] = [];
    const square = (half: number, color: RGB) => {
      const pts: [number, number][] = [[-half, -half], [half, -half], [half, half], [-half, half], [-half, -half]];
      for (let i = 0; i < 4; i++) {
        const a = pts[i], b = pts[i + 1];
        positions.push(a[0], 0, a[1], b[0], 0, b[1]);
        colors.push(...color, ...color);
      }
    };
    for (const e of extents) square(e.half, e.color);
    this.lodGeometry.dispose();
    this.lodGeometry = new THREE.BufferGeometry();
    this.lodGeometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    this.lodGeometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
    this.lodLines.geometry = this.lodGeometry;
  }

  setBrush(worldX: number, worldY: number, worldZ: number, radius: number, visible: boolean, color = 0xffe082): void {
    this.brush.visible = visible;
    if (!visible) return;
    const cam = this.frame.cameraWorld;
    this.brush.position.set(worldX - cam.x, worldY - cam.y + 6, worldZ - cam.z);
    this.brush.scale.set(radius, 1, radius);
    (this.brush.material as THREE.LineBasicMaterial).color.setHex(color);
  }

  setShowLod(value: boolean): void {
    this.showLod = value;
    this.lodLines.visible = value;
  }

  update(frame: WorldFrame, seaLevel: number, showLod: boolean): void {
    this.frame = frame;
    this.lodLines.visible = showLod;
    this.lodLines.position.set(0, seaLevel - frame.cameraWorld.y + 24, 0);
  }

  dispose(): void {
    this.brush.geometry.dispose();
    (this.brush.material as THREE.Material).dispose();
    this.brush.removeFromParent();
    this.lodGeometry.dispose();
    (this.lodLines.material as THREE.Material).dispose();
    this.lodLines.removeFromParent();
  }
}
