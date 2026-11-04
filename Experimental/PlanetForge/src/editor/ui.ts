/**
 * Frontier-style editor chrome.
 *
 * The visual language is lifted from the Frontier editor references in the repo
 * (Experimental/Project browser, Prototypes/CadWorkspace): near-black desk,
 * single blue accent, hairline dividers, 18px rounded panels, uppercase
 * micro-labels, and a left rail of tools rather than a modal toolbox.
 *
 * Kept deliberately small: this is the base authoring surface for landmasses.
 * Regional landscape layers and the erosion studio will attach to the same rail
 * as new groups rather than new windows.
 */

export type ToolId =
  | 'inspect'
  | 'landmass' | 'island' | 'sea' | 'shelf'
  | 'raise' | 'lower' | 'smooth' | 'flatten' | 'plateau' | 'canyon' | 'rough'
  | 'waterDrop' | 'waterPush' | 'teleport';

export interface ToolDef {
  id: ToolId;
  label: string;
  group: 'create' | 'sculpt' | 'water' | 'view';
  hint: string;
  /** Default radius in metres for this tool. */
  radius: number;
  /** Shortcut key. */
  key: string;
}

export const TOOLS: ToolDef[] = [
  { id: 'inspect', label: 'Inspect', group: 'view', hint: 'Read altitude, coast and biome at the cursor', radius: 500, key: '1' },
  { id: 'teleport', label: 'Teleport', group: 'view', hint: 'Fly to the clicked point', radius: 500, key: '2' },
  { id: 'landmass', label: 'Landmass', group: 'create', hint: 'Union a continent-shaped SDF cap into the sea', radius: 900_000, key: '3' },
  { id: 'island', label: 'Island', group: 'create', hint: 'Union a small noisy island', radius: 160_000, key: '4' },
  { id: 'sea', label: 'Sea / gulf', group: 'create', hint: 'Subtract a sea from the land field', radius: 500_000, key: '5' },
  { id: 'shelf', label: 'Shelf', group: 'create', hint: 'Dome the seabed up into a shoal', radius: 380_000, key: '6' },
  { id: 'raise', label: 'Raise', group: 'sculpt', hint: 'Add altitude', radius: 60_000, key: '7' },
  { id: 'lower', label: 'Lower', group: 'sculpt', hint: 'Remove altitude', radius: 60_000, key: '8' },
  { id: 'smooth', label: 'Smooth', group: 'sculpt', hint: 'Low-pass the relief', radius: 40_000, key: '9' },
  { id: 'flatten', label: 'Flatten', group: 'sculpt', hint: 'Blend toward a target altitude', radius: 30_000, key: '0' },
  { id: 'plateau', label: 'Plateau', group: 'sculpt', hint: 'Raise to at least the target altitude', radius: 120_000, key: 'P' },
  { id: 'canyon', label: 'Canyon', group: 'sculpt', hint: 'Cut a narrow gorge', radius: 12_000, key: 'C' },
  { id: 'rough', label: 'Roughen', group: 'sculpt', hint: 'Add high-frequency broken relief', radius: 20_000, key: 'R' },
  { id: 'waterDrop', label: 'Water drop', group: 'water', hint: 'Inject mass into the shallow-water solver', radius: 120, key: 'Z' },
  { id: 'waterPush', label: 'Current', group: 'water', hint: 'Inject momentum — wakes and currents', radius: 200, key: 'X' },
];

export interface SliderDef {
  id: string;
  label: string;
  min: number;
  max: number;
  step: number;
  value: number;
  /** Optional units suffix shown in the value pill. */
  unit?: string;
}

export interface UiCallbacks {
  onTool(id: ToolId): void;
  onSlider(id: string, value: number): void;
  onToggle(id: string, value: boolean): void;
  onAction(id: string): void;
  onSelectOperator(id: string): void;
  onRemoveOperator(id: string): void;
  onFlyToOperator(id: string): void;
}

export interface OutlinerItem {
  id: string;
  label: string;
  kind: string;
}

export class EditorUI {
  private root: HTMLElement;
  private cb: UiCallbacks;
  private activeTool: ToolId = 'landmass';
  private sliders = new Map<string, { input: HTMLInputElement; pill: HTMLElement }>();

  constructor(root: HTMLElement, cb: UiCallbacks) {
    this.root = root;
    this.cb = cb;
    this.build();
  }

  private build(): void {
    this.root.innerHTML = '';
    const app = document.createElement('div');
    app.className = 'pf-app';

    app.appendChild(this.buildRail());
    app.appendChild(this.buildViewport());
    app.appendChild(this.buildInspector());

    this.root.appendChild(app);
    this.bindKeys();
    this.setTool(this.activeTool);
  }

  /* ------------------------------------------------------------------ rail */

  private buildRail(): HTMLElement {
    const rail = document.createElement('aside');
    rail.className = 'pf-rail';

    const brand = document.createElement('div');
    brand.className = 'pf-brand';
    brand.innerHTML = `
      <div class="pf-brand-mark"></div>
      <div>
        <div class="pf-brand-name">Planet Forge</div>
        <div class="pf-brand-sub">SDF terrain · adaptive ocean</div>
      </div>`;
    rail.appendChild(brand);

    const groups: Array<{ title: string; group: ToolDef['group'] }> = [
      { title: 'View', group: 'view' },
      { title: 'Create', group: 'create' },
      { title: 'Sculpt', group: 'sculpt' },
      { title: 'Water', group: 'water' },
    ];

    for (const g of groups) {
      const head = document.createElement('div');
      head.className = 'pf-rail-head';
      head.textContent = g.title;
      rail.appendChild(head);
      for (const t of TOOLS.filter((x) => x.group === g.group)) {
        const btn = document.createElement('button');
        btn.className = 'pf-tool';
        btn.dataset.tool = t.id;
        btn.title = `${t.hint}  (${t.key})`;
        btn.innerHTML = `
          <span class="pf-tool-glyph">${glyph(t.id)}</span>
          <span class="pf-tool-label">${t.label}</span>
          <span class="pf-tool-key">${t.key}</span>`;
        btn.addEventListener('click', () => this.setTool(t.id));
        rail.appendChild(btn);
      }
    }

    const spacer = document.createElement('div');
    spacer.className = 'pf-rail-spacer';
    rail.appendChild(spacer);

    const world = document.createElement('div');
    world.className = 'pf-rail-foot';
    world.innerHTML = `
      <label class="pf-select-label">Preset</label>
      <select class="pf-select" id="pf-preset">
        <option value="continents">Two continents</option>
        <option value="archipelago">Thousand Isles</option>
        <option value="pangea">Pangea</option>
      </select>
      <div class="pf-row-btns">
        <button class="pf-btn" id="pf-load">Load</button>
        <button class="pf-btn" id="pf-export">Export</button>
      </div>`;
    rail.appendChild(world);

    world.querySelector('#pf-load')?.addEventListener('click', () => {
      const sel = world.querySelector('#pf-preset') as HTMLSelectElement;
      this.cb.onAction(`preset:${sel.value}`);
    });
    world.querySelector('#pf-export')?.addEventListener('click', () => this.cb.onAction('export'));

    return rail;
  }

  /* -------------------------------------------------------------- viewport */

  private buildViewport(): HTMLElement {
    const main = document.createElement('main');
    main.className = 'pf-viewport';
    main.id = 'pf-viewport';

    const top = document.createElement('div');
    top.className = 'pf-topbar';
    top.innerHTML = `
      <div class="pf-crumbs">World / <span id="pf-world-name">Terra Nova</span></div>
      <div class="pf-topbar-actions">
        <label class="pf-chip"><input type="checkbox" id="pf-toggle-wire"/> Wireframe</label>
        <label class="pf-chip"><input type="checkbox" id="pf-toggle-lod"/> LOD</label>
        <label class="pf-chip"><input type="checkbox" id="pf-toggle-ocean" checked/> Ocean</label>
        <label class="pf-chip"><input type="checkbox" id="pf-toggle-water-pause"/> Freeze water</label>
      </div>`;
    main.appendChild(top);

    const hud = document.createElement('div');
    hud.className = 'pf-hud';
    hud.innerHTML = `
      <div class="pf-hud-block" id="pf-coords">
        <div class="pf-hud-row"><span>lat</span><b id="pf-lat">—</b></div>
        <div class="pf-hud-row"><span>lon</span><b id="pf-lon">—</b></div>
        <div class="pf-hud-row"><span>alt</span><b id="pf-alt">—</b></div>
        <div class="pf-hud-row"><span>coast</span><b id="pf-coast">—</b></div>
      </div>`;
    main.appendChild(hud);

    const help = document.createElement('div');
    help.className = 'pf-help';
    help.innerHTML = `
      <b>RMB</b> look · <b>MMB</b> pan · <b>WASD</b> fly · <b>Q/E</b> down/up ·
      <b>Shift/Ctrl</b> fast/slow · <b>LMB</b> use tool · <b>Alt+LMB</b> orbit`;
    main.appendChild(help);

    const hint = document.createElement('div');
    hint.className = 'pf-tool-hint';
    hint.id = 'pf-tool-hint';
    main.appendChild(hint);

    return main;
  }

  /* ------------------------------------------------------------- inspector */

  private buildInspector(): HTMLElement {
    const aside = document.createElement('aside');
    aside.className = 'pf-inspector';

    aside.appendChild(this.section('Tool', this.buildToolOptions(), 'pf-tool-options'));
    aside.appendChild(this.section('World', [
      this.slider({ id: 'seaLevelOffset', label: 'Sea level', min: -600, max: 600, step: 5, value: 0, unit: 'm' }),
      this.slider({ id: 'shelfWidth', label: 'Shelf width', min: 40, max: 600, step: 10, value: 190, unit: 'km' }),
      this.slider({ id: 'oceanDepth', label: 'Ocean depth', min: 800, max: 9000, step: 100, value: 4300, unit: 'm' }),
      this.slider({ id: 'relief', label: 'Relief', min: 0, max: 3, step: 0.05, value: 1 }),
      this.slider({ id: 'coastRoughness', label: 'Coast noise', min: 0, max: 2, step: 0.05, value: 1 }),
    ]));
    aside.appendChild(this.section('Ocean', [
      this.slider({ id: 'windStress', label: 'Wind stress', min: 0, max: 0.4, step: 0.005, value: 0.06 }),
      this.slider({ id: 'windAngle', label: 'Wind angle', min: 0, max: 360, step: 5, value: 40, unit: '°' }),
      this.slider({ id: 'swellScale', label: 'Swell scale', min: 0.2, max: 4, step: 0.1, value: 1 }),
      this.slider({ id: 'friction', label: 'Bed friction', min: 0, max: 0.2, step: 0.005, value: 0.035 }),
    ]));
    aside.appendChild(this.section('Streaming', [
      this.slider({ id: 'maxChunks', label: 'Chunk budget', min: 120, max: 2200, step: 20, value: 900 }),
      this.slider({ id: 'meshCells', label: 'Mesh cells', min: 16, max: 64, step: 8, value: 32 }),
      this.slider({ id: 'lodBias', label: 'Detail bias', min: 0.4, max: 2.4, step: 0.1, value: 1 }),
    ], 'pf-streaming'));
    aside.appendChild(this.section('Outliner', [], 'pf-outliner-section'));
    aside.appendChild(this.section('Telemetry', [], 'pf-stats-section'));

    return aside;
  }

  private buildToolOptions(): HTMLElement {
    const wrap = document.createElement('div');
    wrap.innerHTML = `
      <div class="pf-field">
        <label>Brush radius</label>
        <div class="pf-slider">
          <input type="range" id="sl-radius" min="0" max="1000" step="1" value="640" />
          <span class="pf-value" id="pf-radius-value">—</span>
        </div>
      </div>
      <div class="pf-field">
        <label>Strength</label>
        <div class="pf-slider">
          <input type="range" id="sl-strength" min="1" max="100" step="1" value="60" />
          <span class="pf-value" id="pf-strength-value">60%</span>
        </div>
      </div>
      <div class="pf-field">
        <label>Feather</label>
        <div class="pf-slider">
          <input type="range" id="sl-feather" min="0" max="100" step="1" value="55" />
          <span class="pf-value" id="pf-feather-value">0.55</span>
        </div>
      </div>
      <div class="pf-field">
        <label>Target altitude</label>
        <div class="pf-slider">
          <input type="range" id="sl-target" min="-2000" max="6000" step="50" value="600" />
          <span class="pf-value" id="pf-target-value">600 m</span>
        </div>
      </div>
      <div class="pf-row-btns">
        <button class="pf-btn" id="pf-undo">Undo</button>
        <button class="pf-btn" id="pf-clear-strokes">Clear strokes</button>
      </div>`;

    wrap.querySelector('#sl-radius')?.addEventListener('input', (e) => {
      this.cb.onSlider('brushRadiusNorm', Number((e.target as HTMLInputElement).value) / 1000);
    });
    wrap.querySelector('#sl-strength')?.addEventListener('input', (e) => {
      this.cb.onSlider('brushStrength', Number((e.target as HTMLInputElement).value) / 100);
    });
    wrap.querySelector('#sl-feather')?.addEventListener('input', (e) => {
      this.cb.onSlider('brushFeather', Number((e.target as HTMLInputElement).value) / 100);
    });
    wrap.querySelector('#sl-target')?.addEventListener('input', (e) => {
      this.cb.onSlider('brushTarget', Number((e.target as HTMLInputElement).value));
    });
    wrap.querySelector('#pf-undo')?.addEventListener('click', () => this.cb.onAction('undo'));
    wrap.querySelector('#pf-clear-strokes')?.addEventListener('click', () => this.cb.onAction('clearStrokes'));
    return wrap;
  }

  private section(title: string, children: HTMLElement[] | HTMLElement, id?: string): HTMLElement {
    const sec = document.createElement('section');
    sec.className = 'pf-section';
    if (id) sec.id = id;
    const head = document.createElement('header');
    head.className = 'pf-section-head';
    head.innerHTML = `<span>${title}</span><span class="pf-caret">▾</span>`;
    head.addEventListener('click', () => sec.classList.toggle('collapsed'));
    sec.appendChild(head);
    const body = document.createElement('div');
    body.className = 'pf-section-body';
    if (Array.isArray(children)) for (const c of children) body.appendChild(c);
    else body.appendChild(children);
    sec.appendChild(body);
    return sec;
  }

  private slider(def: SliderDef): HTMLElement {
    const wrap = document.createElement('div');
    wrap.className = 'pf-field';
    wrap.innerHTML = `
      <label>${def.label}</label>
      <div class="pf-slider">
        <input type="range" id="${def.id}" min="${def.min}" max="${def.max}" step="${def.step}" value="${def.value}" />
        <span class="pf-value"></span>
      </div>`;
    const input = wrap.querySelector('input') as HTMLInputElement;
    const pill = wrap.querySelector('.pf-value') as HTMLElement;
    const render = (): void => {
      const v = Number(input.value);
      pill.textContent = def.unit ? `${formatNum(v)} ${def.unit}` : formatNum(v);
    };
    render();
    input.addEventListener('input', () => {
      render();
      this.cb.onSlider(def.id, Number(input.value));
    });
    this.sliders.set(def.id, { input, pill });
    return wrap;
  }

  /* ------------------------------------------------------------------ API */

  setTool(id: ToolId): void {
    this.activeTool = id;
    for (const el of this.root.querySelectorAll('.pf-tool')) {
      el.classList.toggle('active', (el as HTMLElement).dataset.tool === id);
    }
    const def = TOOLS.find((t) => t.id === id);
    const hint = this.root.querySelector('#pf-tool-hint');
    if (hint && def) hint.textContent = `${def.label} — ${def.hint}`;
    this.cb.onTool(id);
  }

  get tool(): ToolId { return this.activeTool; }

  setWorldName(name: string): void {
    const el = this.root.querySelector('#pf-world-name');
    if (el) el.textContent = name;
  }

  setCoordinates(lat: number, lon: number, altitude: number, coast: number): void {
    setText(this.root, '#pf-lat', `${lat.toFixed(3)}°`);
    setText(this.root, '#pf-lon', `${lon.toFixed(3)}°`);
    setText(this.root, '#pf-alt', `${formatNum(altitude)} m`);
    setText(this.root, '#pf-coast', `${formatNum(coast / 1000)} km`);
  }

  setBrushPill(metres: number): void {
    setText(this.root, '#pf-radius-value', `${formatNum(metres / 1000)} km`);
  }

  setOutliner(items: OutlinerItem[]): void {
    const body = this.root.querySelector('#pf-outliner-section .pf-section-body');
    if (!body) return;
    if (items.length === 0) {
      body.innerHTML = '<div class="pf-empty">No features yet — paint a landmass.</div>';
      return;
    }
    body.innerHTML = '';
    const list = document.createElement('div');
    list.className = 'pf-outliner';
    for (const item of items) {
      const row = document.createElement('div');
      row.className = 'pf-outliner-row';
      row.innerHTML = `
        <span class="pf-outliner-kind">${item.kind}</span>
        <span class="pf-outliner-name">${item.label}</span>
        <span class="pf-outliner-actions">
          <button title="Fly to">◎</button>
          <button title="Delete">✕</button>
        </span>`;
      const [fly, del] = row.querySelectorAll('button');
      fly.addEventListener('click', () => this.cb.onFlyToOperator(item.id));
      del.addEventListener('click', () => this.cb.onRemoveOperator(item.id));
      row.addEventListener('click', (e) => {
        if ((e.target as HTMLElement).tagName === 'BUTTON') return;
        this.cb.onSelectOperator(item.id);
      });
      list.appendChild(row);
    }
    body.appendChild(list);
  }

  setStats(rows: Array<[string, string]>): void {
    const body = this.root.querySelector('#pf-stats-section .pf-section-body');
    if (!body) return;
    body.innerHTML = rows
      .map(([k, v]) => `<div class="pf-stat"><span>${k}</span><b>${v}</b></div>`)
      .join('');
  }

  setStatus(text: string, kind: 'info' | 'warn' | 'error' = 'info'): void {
    let el = this.root.querySelector('#pf-status') as HTMLElement | null;
    if (!el) {
      el = document.createElement('div');
      el.id = 'pf-status';
      el.className = 'pf-status';
      this.root.querySelector('.pf-viewport')?.appendChild(el);
    }
    el.textContent = text;
    el.dataset.kind = kind;
    el.classList.add('show');
    window.clearTimeout((el as HTMLElement & { _t?: number })._t);
    (el as HTMLElement & { _t?: number })._t = window.setTimeout(() => el!.classList.remove('show'), 4200);
  }

  getSlider(id: string): number {
    const s = this.sliders.get(id);
    return s ? Number(s.input.value) : 0;
  }

  setSlider(id: string, value: number): void {
    const s = this.sliders.get(id);
    if (!s) return;
    s.input.value = String(value);
    s.input.dispatchEvent(new Event('input'));
  }

  private bindKeys(): void {
    window.addEventListener('keydown', (e) => {
      if ((e.target as HTMLElement)?.tagName === 'INPUT' || (e.target as HTMLElement)?.tagName === 'SELECT') return;
      const t = TOOLS.find((x) => x.key.toLowerCase() === e.key.toLowerCase());
      if (t) { this.setTool(t.id); e.preventDefault(); }
    });
  }
}

function setText(root: HTMLElement, sel: string, text: string): void {
  const el = root.querySelector(sel);
  if (el) el.textContent = text;
}

function formatNum(v: number): string {
  if (!Number.isFinite(v)) return '—';
  const abs = Math.abs(v);
  if (abs >= 1_000_000) return `${(v / 1_000_000).toFixed(1)}M`;
  if (abs >= 10_000) return `${Math.round(v / 1000)}k`;
  if (abs >= 100) return `${Math.round(v)}`;
  if (abs >= 1) return `${v.toFixed(1)}`;
  return v.toFixed(2);
}

function glyph(id: ToolId): string {
  switch (id) {
    case 'inspect': return '◈';
    case 'teleport': return '➤';
    case 'landmass': return '▣';
    case 'island': return '◍';
    case 'sea': return '▽';
    case 'shelf': return '⌣';
    case 'raise': return '▲';
    case 'lower': return '▼';
    case 'smooth': return '≈';
    case 'flatten': return '▭';
    case 'plateau': return '⬒';
    case 'canyon': return '⋀';
    case 'rough': return '⁂';
    case 'waterDrop': return '◉';
    case 'waterPush': return '⟿';
  }
}
