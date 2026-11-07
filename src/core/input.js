/**
 * input — unified keyboard / mouse / gamepad sampling.
 *
 * The sim consumes one normalized `Sample` per frame, so the input device is
 * swappable without touching physics. Analogue axes are lightly smoothed so a
 * digital key press does not read as an instant step to full lock.
 */

import { damp } from './mathx.js';

const AXIS_SMOOTH = 16; // higher = snappier key response

export class Input {
  constructor(domElement = window) {
    this.dom = domElement;
    this.keys = new Set();
    this.mouseDX = 0;
    this.mouseDY = 0;
    this.mouseEnabled = false;
    this.locked = false;
    this.anyPressed = false;
    this.pressed = new Set(); // edge-triggered, cleared each frame

    this.onKeyDown = (e) => {
      if (e.repeat) return;
      this.keys.add(e.code);
      this.pressed.add(e.code);
      this.anyPressed = true;
      if (['Space', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Tab'].includes(e.code)) e.preventDefault();
    };
    this.onKeyUp = (e) => { this.keys.delete(e.code); };
    this.onBlur = () => { this.keys.clear(); };
    this.onMove = (e) => {
      if (!this.locked) return;
      this.mouseDX += e.movementX || 0;
      this.mouseDY += e.movementY || 0;
      this.mouseEnabled = true;
    };
    this.onLockChange = () => { this.locked = document.pointerLockElement != null; };

    window.addEventListener('keydown', this.onKeyDown);
    window.addEventListener('keyup', this.onKeyUp);
    window.addEventListener('blur', this.onBlur);
    window.addEventListener('mousemove', this.onMove);
    document.addEventListener('pointerlockchange', this.onLockChange);

    this._axis = { steer: 0, pitch: 0 };
    this.mouseSensitivity = 0.0026;
    this.deadzone = 0.16;
    this.gamepadIndex = null;
  }

  requestLock() {
    const el = document.querySelector('canvas');
    if (el && el.requestPointerLock) { try { el.requestPointerLock(); } catch { /* ignore */ } }
  }

  justPressed(code) { return this.pressed.has(code); }

  _gamepad() {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    for (const p of pads) if (p && p.connected) return p;
    return null;
  }

  /** Poll devices and return this frame's normalized command. */
  sample(dt) {
    const k = this.keys;
    let steer = 0, pitch = 0, throttle = 0, brake = 0, boost = false, drift = false;

    if (k.has('KeyA') || k.has('ArrowLeft')) steer -= 1;
    if (k.has('KeyD') || k.has('ArrowRight')) steer += 1;
    if (k.has('KeyW') || k.has('ArrowUp')) throttle += 1;
    if (k.has('KeyS') || k.has('ArrowDown')) brake += 1;
    // Altitude (pitch) is Q / E. R, C and B are reserved for restart, camera, brake-tap.
    if (k.has('KeyQ')) pitch -= 1;
    if (k.has('KeyE')) pitch += 1;
    if (k.has('Space')) boost = true;
    if (k.has('ShiftLeft') || k.has('ShiftRight')) drift = true;

    const gp = this._gamepad();
    let padActive = false;
    if (gp) {
      const dz = (v) => (Math.abs(v) < this.deadzone ? 0 : (v - Math.sign(v) * this.deadzone) / (1 - this.deadzone));
      const lx = dz(gp.axes[0] || 0);
      const ly = dz(gp.axes[1] || 0);
      const rx = dz(gp.axes[2] || 0);
      const ry = dz(gp.axes[3] || 0);
      const btn = (i) => !!gp.buttons[i] && gp.buttons[i].pressed;
      if (lx || ly || rx || ry || gp.buttons.some((b) => b.pressed)) padActive = true;
      if (Math.abs(lx) > Math.abs(steer)) steer = lx;
      if (Math.abs(rx) > Math.abs(steer)) steer = rx;
      const gpPitch = -ry || -ly;
      if (Math.abs(gpPitch) > Math.abs(pitch)) pitch = gpPitch;
      if (btn(7) || btn(0)) boost = true;    // RT / A
      if (btn(6) || btn(2)) drift = true;    // LT / X
      if (btn(5)) brake = 1;                 // RB
      if (btn(1)) { if (!this._bPressed) { this.pressed.add('KeyB'); this._bPressed = true; } } else this._bPressed = false;
      if (btn(9) && !this._startPressed) { this.pressed.add('Escape'); this._startPressed = true; } else if (!btn(9)) this._startPressed = false;
    }
    this.padActive = padActive;

    // Mouse steering (adds to keyboard so both work together)
    let mouseSteer = 0, mousePitch = 0;
    if (this.mouseEnabled && this.locked) {
      mouseSteer = Math.max(-1, Math.min(1, this.mouseDX * this.mouseSensitivity));
      mousePitch = Math.max(-1, Math.min(1, this.mouseDY * this.mouseSensitivity));
    }
    this.mouseDX = 0;
    this.mouseDY = 0;

    const targetSteer = Math.max(-1, Math.min(1, steer + mouseSteer));
    const targetPitch = Math.max(-1, Math.min(1, pitch + mousePitch));
    this._axis.steer = damp(this._axis.steer, targetSteer, AXIS_SMOOTH, dt);
    this._axis.pitch = damp(this._axis.pitch, targetPitch, AXIS_SMOOTH, dt);

    const out = {
      steer: this._axis.steer,
      pitch: this._axis.pitch,
      throttle: throttle > 0 ? 1 : 0,
      brake: brake > 0 ? 1 : 0,
      boost,
      drift,
      padActive
    };

    this.pressed.clear();
    this.anyPressed = false;
    return out;
  }

  dispose() {
    window.removeEventListener('keydown', this.onKeyDown);
    window.removeEventListener('keyup', this.onKeyUp);
    window.removeEventListener('blur', this.onBlur);
    window.removeEventListener('mousemove', this.onMove);
    document.removeEventListener('pointerlockchange', this.onLockChange);
  }
}
