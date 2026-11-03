import type { CameraRig } from './CameraRig';

export interface InputCallbacks {
  onBrushStart?: (clientX: number, clientY: number, shift: boolean) => void;
  onBrushMove?: (clientX: number, clientY: number, dt: number, shift: boolean) => void;
  onBrushEnd?: () => void;
  onHover?: (clientX: number, clientY: number) => void;
  onKey?: (code: string, down: boolean, event: KeyboardEvent) => void;
  onWheel?: (deltaY: number) => void;
}

type DragMode = 'none' | 'look' | 'pan' | 'brush';

export class InputController {
  private dom: HTMLElement;
  private rig: CameraRig;
  private cb: InputCallbacks;
  private mode: DragMode = 'none';
  private activeButton = -1;
  private pointerId = -1;
  private lastX = 0;
  private lastY = 0;
  private lastMove = performance.now();

  constructor(dom: HTMLElement, rig: CameraRig, cb: InputCallbacks) {
    this.dom = dom;
    this.rig = rig;
    this.cb = cb;
    dom.addEventListener('pointerdown', this.onPointerDown);
    dom.addEventListener('pointermove', this.onPointerMove);
    dom.addEventListener('pointerup', this.onPointerUp);
    dom.addEventListener('pointercancel', this.onPointerUp);
    dom.addEventListener('wheel', this.onWheel, { passive: false });
    dom.addEventListener('contextmenu', this.onContextMenu);
    window.addEventListener('keydown', this.onKeyDown);
    window.addEventListener('keyup', this.onKeyUp);
    window.addEventListener('blur', this.onBlur);
  }

  dispose(): void {
    this.dom.removeEventListener('pointerdown', this.onPointerDown);
    this.dom.removeEventListener('pointermove', this.onPointerMove);
    this.dom.removeEventListener('pointerup', this.onPointerUp);
    this.dom.removeEventListener('pointercancel', this.onPointerUp);
    this.dom.removeEventListener('wheel', this.onWheel);
    this.dom.removeEventListener('contextmenu', this.onContextMenu);
    window.removeEventListener('keydown', this.onKeyDown);
    window.removeEventListener('keyup', this.onKeyUp);
    window.removeEventListener('blur', this.onBlur);
  }

  private isFormTarget(target: EventTarget | null): boolean {
    const el = target as HTMLElement | null;
    if (!el) return false;
    return ['INPUT', 'TEXTAREA', 'SELECT'].includes(el.tagName) || el.isContentEditable;
  }

  private onContextMenu = (e: Event) => e.preventDefault();

  private onPointerDown = (e: PointerEvent) => {
    if (e.button === 2) this.mode = 'look';
    else if (e.button === 1) this.mode = 'pan';
    else if (e.button === 0) this.mode = 'brush';
    else return;
    this.activeButton = e.button;
    this.pointerId = e.pointerId;
    this.lastX = e.clientX;
    this.lastY = e.clientY;
    this.lastMove = performance.now();
    this.dom.setPointerCapture?.(e.pointerId);
    if (this.mode === 'brush') this.cb.onBrushStart?.(e.clientX, e.clientY, e.shiftKey);
    e.preventDefault();
  };

  private onPointerMove = (e: PointerEvent) => {
    const now = performance.now();
    const dt = Math.min(0.1, (now - this.lastMove) / 1000);
    const dx = e.clientX - this.lastX;
    const dy = e.clientY - this.lastY;
    this.lastX = e.clientX;
    this.lastY = e.clientY;
    this.lastMove = now;
    if (this.mode === 'look') this.rig.look(dx, dy);
    else if (this.mode === 'pan') this.rig.pan(dx, dy);
    else if (this.mode === 'brush') this.cb.onBrushMove?.(e.clientX, e.clientY, dt, e.shiftKey);
    this.cb.onHover?.(e.clientX, e.clientY);
  };

  private onPointerUp = (e: PointerEvent) => {
    if (this.pointerId !== e.pointerId || this.activeButton !== e.button) return;
    if (this.mode === 'brush') this.cb.onBrushEnd?.();
    this.mode = 'none';
    this.activeButton = -1;
    this.pointerId = -1;
    this.dom.releasePointerCapture?.(e.pointerId);
  };

  private onWheel = (e: WheelEvent) => {
    this.rig.adjustSpeed(e.deltaY);
    this.cb.onWheel?.(e.deltaY);
    e.preventDefault();
  };

  private onKeyDown = (e: KeyboardEvent) => {
    if (this.isFormTarget(e.target)) return;
    this.rig.setKey(e.code, true);
    this.cb.onKey?.(e.code, true, e);
    if (['Space', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(e.code)) e.preventDefault();
  };

  private onKeyUp = (e: KeyboardEvent) => {
    this.rig.setKey(e.code, false);
    this.cb.onKey?.(e.code, false, e);
  };

  private onBlur = () => {
    this.rig.clearKeys();
    this.mode = 'none';
  };
}
