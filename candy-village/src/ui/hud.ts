/**
 * DOM HUD: buttons, accuracy slider, score panel, event log and floating
 * score popups projected from world space each frame.
 */
import { PerspectiveCamera, Vector3 } from 'three';

export interface Stats {
  shots: number;
  hits: number;
  misses: number;
  score: number;
  bullseyes: number;
}

export interface HudCallbacks {
  onSendNearest: () => void;
  onSendAll: () => void;
  onClear: () => void;
  onNewVillage: () => void;
  onAccuracy: (sigma: number) => void;
}

interface Popup {
  el: HTMLDivElement;
  pos: Vector3;
  life: number;
}

const POPUP_LIFE = 1.6;

function byId<T extends HTMLElement>(id: string): T {
  const el = document.getElementById(id);
  if (!el) throw new Error(`HUD element #${id} missing`);
  return el as T;
}

export class Hud {
  private popups: Popup[] = [];
  private readonly logEl = byId<HTMLOListElement>('log');
  private readonly popupLayer = byId<HTMLDivElement>('popups');

  constructor(seed: number, cb: HudCallbacks) {
    byId<HTMLSpanElement>('seed').textContent = String(seed);
    byId<HTMLButtonElement>('btn-send').addEventListener('click', cb.onSendNearest);
    byId<HTMLButtonElement>('btn-send-all').addEventListener('click', cb.onSendAll);
    byId<HTMLButtonElement>('btn-clear').addEventListener('click', cb.onClear);
    byId<HTMLButtonElement>('btn-new').addEventListener('click', cb.onNewVillage);
    const slider = byId<HTMLInputElement>('accuracy');
    const readout = byId<HTMLSpanElement>('accuracy-value');
    slider.addEventListener('input', () => {
      readout.textContent = Number(slider.value).toFixed(2);
      cb.onAccuracy(Number(slider.value));
    });
    readout.textContent = Number(slider.value).toFixed(2);
    window.addEventListener('keydown', (e) => {
      if (e.target instanceof HTMLInputElement) return;
      if (e.code === 'Space') {
        e.preventDefault();
        cb.onSendNearest();
      } else if (e.code === 'KeyC') {
        cb.onClear();
      }
    });
    this.log('Click the ground to place a target. Press Space to send the nearest guard.');
  }

  updateStats(s: Stats): void {
    byId('stat-shots').textContent = String(s.shots);
    byId('stat-hits').textContent = String(s.hits);
    byId('stat-misses').textContent = String(s.misses);
    byId('stat-score').textContent = String(s.score);
    byId('stat-bullseyes').textContent = String(s.bullseyes);
    const acc = s.shots ? Math.round((100 * s.hits) / s.shots) : 0;
    byId('stat-accuracy').textContent = `${acc}%`;
  }

  setStatus(text: string): void {
    byId('status').textContent = text;
  }

  log(message: string): void {
    const li = document.createElement('li');
    li.textContent = message;
    this.logEl.prepend(li);
    while (this.logEl.children.length > 40) this.logEl.lastElementChild?.remove();
  }

  popup(text: string, pos: Vector3, tone: 'gold' | 'hit' | 'miss'): void {
    const el = document.createElement('div');
    el.className = `popup popup-${tone}`;
    el.textContent = text;
    this.popupLayer.appendChild(el);
    this.popups.push({ el, pos: pos.clone(), life: 0 });
  }

  /** Projects popups onto the screen and fades them out. */
  updatePopups(dt: number, camera: PerspectiveCamera): void {
    const width = window.innerWidth;
    const height = window.innerHeight;
    for (let i = this.popups.length - 1; i >= 0; i--) {
      const p = this.popups[i];
      p.life += dt;
      p.pos.y += dt * 1.2;
      const ndc = p.pos.clone().project(camera);
      const visible = ndc.z < 1 && Math.abs(ndc.x) < 1.1 && Math.abs(ndc.y) < 1.1;
      p.el.style.display = visible ? 'block' : 'none';
      p.el.style.left = `${((ndc.x + 1) / 2) * width}px`;
      p.el.style.top = `${((1 - ndc.y) / 2) * height}px`;
      p.el.style.opacity = String(Math.max(0, 1 - p.life / POPUP_LIFE));
      if (p.life >= POPUP_LIFE) {
        p.el.remove();
        this.popups.splice(i, 1);
      }
    }
  }

  hideLoading(): void {
    byId('loading').classList.add('hidden');
  }
}
