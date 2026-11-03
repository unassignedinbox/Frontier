import type { FrontierApp } from './FrontierApp';
import { TOOL_INFO, type EditorSettings } from './settings';
import { formatMetres } from '../world/math';
import type { SculptTool } from '../world/SdfWorld';

type Mode = 'editor' | 'viewer';

function rangeHtml(key: keyof EditorSettings, label: string, min: number, max: number, step: number, extra = ''): string {
  return `
    <label class="control">
      <span>${label}</span>
      <div class="range-row">
        <input type="range" data-setting="${key}" min="${min}" max="${max}" step="${step}" ${extra}>
        <output data-value="${key}"></output>
      </div>
    </label>`;
}

function toggleHtml(key: keyof EditorSettings, label: string): string {
  return `<label class="toggle"><input type="checkbox" data-setting="${key}"><span>${label}</span></label>`;
}

function selectHtml(key: keyof EditorSettings, label: string, options: Array<[string, string]>): string {
  return `
    <label class="control">
      <span>${label}</span>
      <select data-setting="${key}">
        ${options.map(([value, text]) => `<option value="${value}">${text}</option>`).join('')}
      </select>
    </label>`;
}

const TOOL_SHORTCUTS: Partial<Record<SculptTool, string>> = {
  inspect: '1', continent: '2', gulf: '3', island: '4', raise: '5',
  lower: '6', flatten: '7', smooth: '8', ridge: '9', cityPad: '0',
  waterDrop: 'B', waterPush: 'N',
};

export class UI {
  root: HTMLElement;
  canvas: HTMLCanvasElement;
  minimapCanvas: HTMLCanvasElement;
  fileInput: HTMLInputElement;
  mode: Mode;

  private app: FrontierApp;
  private toastTimer = 0;

  constructor(root: HTMLElement, mode: Mode, app: FrontierApp) {
    this.root = root;
    this.app = app;
    this.mode = mode;
    root.dataset.mode = mode;
    root.innerHTML = this.template();
    this.canvas = this.q<HTMLCanvasElement>('#viewport');
    this.minimapCanvas = this.q<HTMLCanvasElement>('#minimap');
    this.fileInput = this.q<HTMLInputElement>('#importFile');
    this.bind();
    this.refresh();
    this.setActiveTool(app.settings.tool);
    this.setActiveTab('world');
    this.setTopbar({
      mode: mode === 'editor' ? 'WORLD FORGE / EDITOR' : 'RUNTIME VIEWER',
      worldName: app.world.def.name,
      coords: '—',
      altitude: '—',
      speed: `${app.settings.cameraSpeed.toFixed(0)} m/s`,
      status: 'initialising',
    });
    this.setMapVisible(app.settings.showPlanetMap);
    this.setHelpVisible(app.settings.showHelp);
    this.setPlanetExpanded(app.settings.planetExpanded);
  }

  private q<T extends HTMLElement>(selector: string): T {
    const el = this.root.querySelector<T>(selector);
    if (!el) throw new Error(`UI element missing: ${selector}`);
    return el;
  }

  private template(): string {
    const tools = Object.entries(TOOL_INFO).map(([key, info]) => `
      <button class="tool-btn" data-tool="${key}" title="${info.hint}">
        <span class="tool-key">${TOOL_SHORTCUTS[key as SculptTool] ?? ''}</span>
        <span>${info.label}</span>
      </button>`).join('');

    return `
      <div id="viewportHost">
        <canvas id="viewport"></canvas>
        <div id="viewLabel">SDF WORLD · CAMERA-RELATIVE LWC</div>
      </div>

      <header id="topbar">
        <div class="brand"><b>FRONTIER</b><span id="modeLabel">WORLD FORGE</span></div>
        <div class="top-readouts">
          <span id="worldName">—</span>
          <span id="coordsReadout">—</span>
          <span id="altitudeReadout">—</span>
          <span id="speedReadout">—</span>
        </div>
        <div class="top-actions">
          <span id="statusPill">initialising</span>
          <button data-action="toggle-map" title="Toggle planet cartographer">Globe</button>
          <button data-action="toggle-help" title="Toggle controls">?</button>
          ${this.mode === 'editor'
            ? '<button class="primary" data-action="open-viewer">Runtime viewer ↗</button>'
            : '<button class="primary" data-action="open-editor">Open World Forge ↗</button>'}
        </div>
      </header>

      <nav id="toolbar" class="panel editor-only">
        <div class="panel-title">SCULPT / SIMULATE <span>LEFT DRAG</span></div>
        <div class="tool-grid">${tools}</div>
        <div class="tool-hint" id="toolHint">—</div>
      </nav>

      <aside id="inspector" class="panel editor-only">
        <div class="tabs">
          <button data-tab="world" class="active">WORLD</button>
          <button data-tab="ocean">OCEAN</button>
          <button data-tab="view">VIEW</button>
          <button data-tab="bake">BAKE</button>
        </div>

        <section data-panel="world" class="tab-panel active">
          <div class="section-title">BLOCKOUT PRESETS</div>
          <div class="button-row">
            <button data-action="preset-flat">Flat plate</button>
            <button data-action="preset-africa">Africa + gulfs</button>
            <button data-action="preset-archipelago">Archipelago</button>
            <button data-action="preset-random">Generate</button>
          </div>

          <div class="section-title">SDF BRUSH</div>
          ${rangeHtml('brushRadiusKm', 'Radius', 0.02, 400, 0.01)}
          ${rangeHtml('brushStrength', 'Strength', 10, 2600, 10)}
          ${rangeHtml('brushTarget', 'Flatten target', -300, 3200, 5)}
          ${rangeHtml('brushFeather', 'Feather', 0.05, 1, 0.01)}

          <div class="section-title">PLANET / COAST</div>
          ${rangeHtml('seaLevel', 'Sea level', -250, 250, 1)}
          ${rangeHtml('coastNoise', 'Coast roughness', 0, 260000, 1000)}
          ${rangeHtml('oceanDepth', 'Ocean depth', 1000, 6000, 50)}
          ${rangeHtml('landElevation', 'Land relief', 200, 2600, 25)}

          <div class="button-row wrap">
            <button data-action="undo">Undo stroke</button>
            <button data-action="clear-edits">Clear edits</button>
            <button data-action="reset-camera">Reset camera</button>
            <button data-action="focus-brush">Focus cursor</button>
          </div>
          <p class="note">The heightmap is a derived projection. Continents, gulfs and islands are CSG operations on the source SDF; the near chunk is voxelised by Marching Cubes.</p>
        </section>

        <section data-panel="ocean" class="tab-panel">
          <div class="section-title">ADAPTIVE CLIPMAP</div>
          ${selectHtml('gridSize', 'Simulated near grid', [['64', '64²'], ['96', '96²'], ['128', '128²'], ['160', '160²'], ['192', '192²']])}
          ${selectHtml('cellSize', 'Near cell size', [['4', '4 m'], ['6', '6 m'], ['8', '8 m'], ['12', '12 m'], ['16', '16 m'], ['24', '24 m']])}
          ${rangeHtml('farLevels', 'Tessellated LOD rings', 4, 12, 1, 'data-int="1"')}
          <div class="callout">
            <b>2.5D finite-volume SWE</b><br>
            C-grid η/u/v state · CFL substeps · wet/dry bathymetry · reflective shores · absorbing sponge. Far rings are tessellated geometry, not a wave texture.
          </div>

          <div class="section-title">SOLVER</div>
          ${toggleHtml('paused', 'Pause simulation')}
          ${rangeHtml('simSpeed', 'Simulation speed', 0, 3, 0.05)}
          ${rangeHtml('gravity', 'Gravity', 1, 20, 0.01)}
          ${rangeHtml('damping', 'Velocity damping', 0, 0.15, 0.002)}
          ${rangeHtml('wind', 'Wind force', 0, 1.5, 0.01)}
          ${rangeHtml('windAngle', 'Wind direction', 0, 360, 1)}
          ${rangeHtml('waveMaker', 'Boundary swell', 0, 0.8, 0.01)}
          ${rangeHtml('sponge', 'Edge absorption', 0, 2, 0.01)}
          ${rangeHtml('waveHeight', 'Far wave amplitude', 0, 4, 0.05)}
          ${rangeHtml('dropSize', 'Drop amplitude', 0.2, 6, 0.1)}

          <div class="button-row wrap">
            <button data-action="water-drop">Drop at cursor</button>
            <button data-action="storm">Storm pulse</button>
            <button data-action="calm">Calm surface</button>
            <button data-action="reset-solver">Reset solver</button>
          </div>
        </section>

        <section data-panel="view" class="tab-panel">
          <div class="section-title">CAMERA</div>
          ${rangeHtml('cameraSpeed', 'Fly speed', 10, 20000, 10)}
          <div class="callout">
            <b>Unreal-style viewport</b><br>
            RMB drag look · MMB drag pan · WASD fly · Q/E down/up · Shift ×5 · Ctrl ×0.2 · wheel adjusts speed.
          </div>
          <div class="section-title">DEBUG OVERLAYS</div>
          ${toggleHtml('showWireframe', 'Terrain wireframe')}
          ${toggleHtml('showLodColors', 'LOD tint')}
          ${toggleHtml('showLodRings', 'Adaptive LOD boundaries')}
          ${toggleHtml('showSdfHero', 'Voxel SDF hero chunk')}
          ${toggleHtml('showWaterWireframe', 'Water mesh wireframe')}
          ${toggleHtml('showPlanetMap', 'Planet cartographer')}
          ${toggleHtml('showHelp', 'Controls overlay')}
          ${toggleHtml('planetExpanded', 'Enlarge planet map')}
        </section>

        <section data-panel="bake" class="tab-panel">
          <div class="section-title">EXPORT / INTERCHANGE</div>
          <div class="button-column">
            <button data-action="export-sdf">Export editable SDF JSON</button>
            <button data-action="import-sdf">Import SDF JSON</button>
            <button data-action="export-heightmap">Export heightmap PNG + metadata</button>
            <button data-action="export-obj">Bake visible terrain mesh OBJ</button>
            <button data-action="copy-link">Copy runtime viewer link</button>
          </div>
          <p class="note">PNG: RG channels encode 16-bit height, B is the land mask. OBJ contains the currently streamed SDF-derived terrain rings plus the Marching Cubes hero chunk.</p>
          <div class="callout danger">The editor and runtime share the same world definition in local storage. Imported SDF JSON is the authoritative editable authoring format.</div>
        </section>
      </aside>

      <section id="mapPanel" class="panel">
        <div class="panel-title">PLANET CARTOGRAPHER <span data-action="toggle-planet" class="clickable">⤢</span></div>
        <canvas id="minimap"></canvas>
        <div class="map-legend"><i class="land"></i>Land SDF <i class="water"></i>Bathymetry <i class="cam"></i>Camera</div>
      </section>

      <section id="statsPanel" class="panel">
        <div class="panel-title">TELEMETRY <span>LIVE</span></div>
        <div id="statsBody"></div>
      </section>

      <section id="helpPanel" class="panel">
        <div class="panel-title">VIEWPORT CONTROLS</div>
        <div class="help-grid">
          <span>RMB drag</span><b>Look</b>
          <span>MMB drag</span><b>Pan</b>
          <span>W A S D</span><b>Fly</b>
          <span>Q / E</span><b>Down / up</b>
          <span>Shift / Ctrl</span><b>Boost / slow</b>
          <span>Wheel</span><b>Fly speed</b>
          <span>Left drag</span><b>Sculpt / disturb</b>
          <span>F / R</span><b>Focus / reset</b>
          <span>Space</span><b>Rise</b>
          <span>1–0, B, N</span><b>Tool shortcuts</b>
        </div>
      </section>

      <div id="toast" role="status"></div>
      <input id="importFile" type="file" accept=".json,application/json" hidden>
    `;
  }

  private bind(): void {
    this.root.querySelectorAll<HTMLElement>('[data-tool]').forEach(btn => {
      btn.addEventListener('click', () => this.app.setTool(btn.dataset.tool as SculptTool));
    });
    this.root.querySelectorAll<HTMLElement>('[data-tab]').forEach(btn => {
      btn.addEventListener('click', () => this.setActiveTab(btn.dataset.tab!));
    });
    this.root.querySelectorAll<HTMLElement>('[data-action]').forEach(btn => {
      btn.addEventListener('click', () => {
        const action = btn.dataset.action!;
        if (action === 'open-editor') window.location.href = 'editor.html';
        else if (action === 'open-viewer') window.location.href = 'index.html';
        else if (action === 'import-sdf') this.fileInput.click();
        else this.app.uiAction(action);
      });
    });
    this.fileInput.addEventListener('change', () => {
      const file = this.fileInput.files?.[0];
      if (file) this.app.importSDFFile(file);
      this.fileInput.value = '';
    });

    this.root.querySelectorAll<HTMLInputElement | HTMLSelectElement>('[data-setting]').forEach(el => {
      const key = el.dataset.setting as keyof EditorSettings;
      const eventName = el.dataset.event || (el instanceof HTMLInputElement && el.type === 'range' ? 'input' : 'change');
      el.addEventListener(eventName, () => {
        let value: number | boolean | string;
        if (el instanceof HTMLInputElement && el.type === 'checkbox') value = el.checked;
        else {
          const raw = Number(el.value);
          value = el.dataset.int === '1' || el.dataset.int === 'true' ? Math.round(raw) : raw;
        }
        this.app.setSetting(key, value as never);
        this.updateValueLabel(key, value as number);
      });
    });
  }

  private formatSetting(key: keyof EditorSettings, value: number | boolean): string {
    if (typeof value === 'boolean') return value ? 'on' : 'off';
    switch (key) {
      case 'brushRadiusKm': return value < 1 ? `${(value * 1000).toFixed(0)} m` : `${value.toFixed(value < 10 ? 1 : 0)} km`;
      case 'brushFeather': return `${Math.round(value * 100)}%`;
      case 'gridSize': return `${value}² cells`;
      case 'cellSize': return `${value} m`;
      case 'farLevels': return `${value} rings`;
      case 'simSpeed': return `${value.toFixed(2)}×`;
      case 'gravity': return `${value.toFixed(2)} m/s²`;
      case 'damping':
      case 'sponge': return value.toFixed(3);
      case 'windAngle': return `${value.toFixed(0)}°`;
      case 'waveMaker': return `${value.toFixed(2)} m`;
      case 'waveHeight':
      case 'dropSize': return `${value.toFixed(2)} m`;
      case 'cameraSpeed': return `${value.toFixed(0)} m/s`;
      case 'coastNoise': return formatMetres(value);
      default: return Number.isInteger(value) ? `${value} m` : formatMetres(value);
    }
  }

  private updateValueLabel(key: keyof EditorSettings, value: number | boolean): void {
    const out = this.root.querySelector<HTMLElement>(`[data-value="${key}"]`);
    if (out) out.textContent = this.formatSetting(key, value);
  }

  refresh(): void {
    this.root.querySelectorAll<HTMLInputElement | HTMLSelectElement>('[data-setting]').forEach(el => {
      const key = el.dataset.setting as keyof EditorSettings;
      const value = this.app.settings[key] as number | boolean | string;
      if (el instanceof HTMLInputElement && el.type === 'checkbox') el.checked = Boolean(value);
      else el.value = String(value);
      if (typeof value === 'number' || typeof value === 'boolean') this.updateValueLabel(key, value);
    });
  }

  setActiveTab(tab: string): void {
    this.root.querySelectorAll<HTMLElement>('[data-tab]').forEach(b => b.classList.toggle('active', b.dataset.tab === tab));
    this.root.querySelectorAll<HTMLElement>('[data-panel]').forEach(p => p.classList.toggle('active', p.dataset.panel === tab));
  }

  setActiveTool(tool: SculptTool): void {
    this.root.querySelectorAll<HTMLElement>('[data-tool]').forEach(b => b.classList.toggle('active', b.dataset.tool === tool));
    const hint = this.q<HTMLElement>('#toolHint');
    hint.textContent = TOOL_INFO[tool].hint;
  }

  setTopbar(data: { mode: string; worldName: string; coords: string; altitude: string; speed: string; status: string }): void {
    this.q('#modeLabel').textContent = data.mode;
    this.q('#worldName').textContent = data.worldName;
    this.q('#coordsReadout').textContent = data.coords;
    this.q('#altitudeReadout').textContent = data.altitude;
    this.q('#speedReadout').textContent = data.speed;
    this.q('#statusPill').textContent = data.status;
  }

  setStats(rows: Record<string, string>): void {
    this.q('#statsBody').innerHTML = Object.entries(rows)
      .map(([k, v]) => `<div class="stat-row"><span>${k}</span><b>${v}</b></div>`)
      .join('');
  }

  setMapVisible(value: boolean): void {
    this.q('#mapPanel').classList.toggle('hidden', !value);
  }

  setHelpVisible(value: boolean): void {
    this.q('#helpPanel').classList.toggle('hidden', !value);
  }

  setPlanetExpanded(value: boolean): void {
    this.q('#mapPanel').classList.toggle('expanded', value);
  }

  toast(message: string, kind: 'info' | 'error' | 'success' = 'info'): void {
    const el = this.q('#toast');
    el.textContent = message;
    el.className = `show ${kind}`;
    window.clearTimeout(this.toastTimer);
    this.toastTimer = window.setTimeout(() => { el.className = ''; }, 2800);
  }
}
