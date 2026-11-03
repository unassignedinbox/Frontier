import * as THREE from 'three';
import { clamp } from '../world/math';
import type { WorldFrame } from './WorldFrame';

/** Unreal-editor-style free camera: RMB look, MMB pan, WASD/QE fly, wheel speed. */
export class CameraRig {
  camera: THREE.PerspectiveCamera;
  frame: WorldFrame;
  yaw = -0.48;
  pitch = -0.22;
  speed = 700; // metres / second
  keys = new Set<string>();
  private forward = new THREE.Vector3();
  private right = new THREE.Vector3();
  private up = new THREE.Vector3();
  private flatForward = new THREE.Vector3();
  private flatRight = new THREE.Vector3();

  constructor(camera: THREE.PerspectiveCamera, frame: WorldFrame) {
    this.camera = camera;
    this.frame = frame;
    this.camera.position.set(0, 0, 0);
    this.camera.rotation.order = 'YXZ';
    this.applyRotation();
  }

  setKey(code: string, down: boolean): void {
    if (down) this.keys.add(code);
    else this.keys.delete(code);
  }

  clearKeys(): void { this.keys.clear(); }

  look(dx: number, dy: number): void {
    this.yaw -= dx * 0.0023;
    this.pitch = clamp(this.pitch - dy * 0.0023, -1.48, 1.48);
    this.applyRotation();
  }

  pan(dx: number, dy: number): void {
    this.updateBasis();
    const scale = this.speed * 0.0016;
    this.frame.cameraWorld.addScaledVector(this.right, -dx * scale);
    this.frame.cameraWorld.addScaledVector(this.up, dy * scale);
  }

  adjustSpeed(deltaY: number): void {
    this.speed = clamp(this.speed * Math.exp(-deltaY * 0.0012), 8, 120000);
  }

  reset(x: number, y: number, z: number): void {
    this.frame.setCameraWorld(x, y, z);
    this.yaw = -0.48;
    this.pitch = -0.22;
    this.speed = 700;
    this.applyRotation();
  }

  private applyRotation(): void {
    this.camera.rotation.set(this.pitch, this.yaw, 0, 'YXZ');
    this.camera.updateMatrixWorld();
  }

  private updateBasis(): void {
    this.camera.getWorldDirection(this.forward);
    this.right.set(1, 0, 0).applyQuaternion(this.camera.quaternion);
    this.up.set(0, 1, 0).applyQuaternion(this.camera.quaternion);
    this.flatForward.copy(this.forward);
    this.flatForward.y = 0;
    if (this.flatForward.lengthSq() < 1e-8) this.flatForward.set(-Math.sin(this.yaw), 0, -Math.cos(this.yaw));
    this.flatForward.normalize();
    this.flatRight.copy(this.right);
    this.flatRight.y = 0;
    this.flatRight.normalize();
  }

  update(dt: number): void {
    this.updateBasis();
    const boost = this.keys.has('ShiftLeft') || this.keys.has('ShiftRight') ? 5 : 1;
    const slow = this.keys.has('ControlLeft') || this.keys.has('ControlRight') ? 0.2 : 1;
    const v = this.speed * boost * slow * dt;
    const w = this.frame.cameraWorld;
    if (this.keys.has('KeyW') || this.keys.has('ArrowUp')) w.addScaledVector(this.flatForward, v);
    if (this.keys.has('KeyS') || this.keys.has('ArrowDown')) w.addScaledVector(this.flatForward, -v);
    if (this.keys.has('KeyD') || this.keys.has('ArrowRight')) w.addScaledVector(this.flatRight, v);
    if (this.keys.has('KeyA') || this.keys.has('ArrowLeft')) w.addScaledVector(this.flatRight, -v);
    if (this.keys.has('KeyE') || this.keys.has('Space')) w.y += v;
    if (this.keys.has('KeyQ')) w.y -= v;
    // Keep the camera out of the SDF solid, with a small clearance.
    // The app supplies a ground clamp through `groundHeight` when needed.
  }

  get basis(): { forward: THREE.Vector3; right: THREE.Vector3; up: THREE.Vector3 } {
    this.updateBasis();
    return { forward: this.forward.clone(), right: this.right.clone(), up: this.up.clone() };
  }
}
