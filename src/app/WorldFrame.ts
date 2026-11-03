import * as THREE from 'three';
import { localToGeo, PLANET_RADIUS } from '../world/math';

/**
 * Large-world coordinate frame.
 *
 * The simulation keeps absolute planet-tangent coordinates in JS doubles.
 * WebGL only ever sees `world - cameraWorld`, so Float32 meshes stay precise
 * even when the camera is thousands of kilometres from the editor origin.
 * This is the same principle as camera-relative rendering / origin rebasing
 * in large-world engines.
 */
export class WorldFrame {
  readonly cameraWorld = new THREE.Vector3(0, 250, 0);
  originLon = 20;
  originLat = 0;

  setOrigin(lon: number, lat: number): void {
    this.originLon = lon;
    this.originLat = lat;
  }

  setCameraWorld(x: number, y: number, z: number): void {
    this.cameraWorld.set(x, y, z);
  }

  toRenderXYZ(x: number, y: number, z: number, out = new THREE.Vector3()): THREE.Vector3 {
    return out.set(x - this.cameraWorld.x, y - this.cameraWorld.y, z - this.cameraWorld.z);
  }

  fromRender(local: THREE.Vector3): THREE.Vector3 {
    return local.clone().add(this.cameraWorld);
  }

  geo(): { lat: number; lon: number; altitude: number } {
    const g = localToGeo(this.cameraWorld.x, this.cameraWorld.z, this.originLon, this.originLat);
    return { ...g, altitude: this.cameraWorld.y };
  }

  setFromGeo(lon: number, lat: number, altitude?: number): void {
    const lat0 = this.originLat * Math.PI / 180;
    const dLon = ((lon - this.originLon + 540) % 360 - 180) * Math.PI / 180;
    const dLat = (lat - this.originLat) * Math.PI / 180;
    this.cameraWorld.x = PLANET_RADIUS * Math.cos(lat0) * dLon;
    this.cameraWorld.z = -PLANET_RADIUS * dLat;
    if (altitude !== undefined) this.cameraWorld.y = altitude;
  }
}
