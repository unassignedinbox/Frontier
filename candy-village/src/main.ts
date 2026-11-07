/**
 * Entry point. Reads `?seed=` from the URL, builds the village, and runs the
 * render loop with bloom and the output pass for tone mapping.
 */
import {
  ACESFilmicToneMapping,
  PCFShadowMap,
  PerspectiveCamera,
  SRGBColorSpace,
  Scene,
  Vector2,
  WebGLRenderer,
} from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/examples/jsm/postprocessing/UnrealBloomPass.js';
import { OutputPass } from 'three/examples/jsm/postprocessing/OutputPass.js';
import { Game } from './game/game';
import { Hud } from './ui/hud';
import { buildWorld } from './world/world';

function readSeed(): number {
  const raw = new URLSearchParams(window.location.search).get('seed');
  const n = raw === null ? NaN : Number(raw);
  return Number.isFinite(n) ? Math.floor(n) >>> 0 : 20241224;
}

function main(): void {
  const seed = readSeed();
  const app = document.getElementById('app')!;

  const renderer = new WebGLRenderer({ antialias: true, powerPreference: 'high-performance' });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.outputColorSpace = SRGBColorSpace;
  renderer.toneMapping = ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = PCFShadowMap;
  app.appendChild(renderer.domElement);

  const scene = new Scene();
  const camera = new PerspectiveCamera(48, window.innerWidth / window.innerHeight, 0.5, 2000);
  camera.position.set(44, 34, 66);

  const controls = new OrbitControls(camera, renderer.domElement);
  controls.target.set(0, 2, -6);
  controls.enableDamping = true;
  controls.maxPolarAngle = Math.PI * 0.47;
  controls.minDistance = 6;
  controls.maxDistance = 220;
  controls.update();

  const composer = new EffectComposer(renderer);
  composer.addPass(new RenderPass(scene, camera));
  const bloom = new UnrealBloomPass(new Vector2(window.innerWidth, window.innerHeight), 0.22, 0.55, 0.92);
  composer.addPass(bloom);
  composer.addPass(new OutputPass());

  const world = buildWorld(scene, seed);
  const hud = new Hud(seed, {
    onSendNearest: () => game.sendNearest(),
    onSendAll: () => game.sendAll(),
    onClear: () => game.clearTargets(),
    onNewVillage: () => {
      const next = Math.floor(Math.random() * 1_000_000_000);
      window.location.search = `?seed=${next}`;
    },
    onAccuracy: (sigma) => game.setAccuracy(sigma),
  });
  const game = new Game(world, scene, camera, hud);

  // Clicks place targets; drags orbit the camera.
  let down: { x: number; y: number; shift: boolean } | null = null;
  renderer.domElement.addEventListener('pointerdown', (e) => {
    down = { x: e.clientX, y: e.clientY, shift: e.shiftKey };
  });
  renderer.domElement.addEventListener('pointerup', (e) => {
    if (!down || e.button !== 0) return;
    const moved = Math.hypot(e.clientX - down.x, e.clientY - down.y);
    if (moved < 5) game.pick(e.clientX, e.clientY, down.shift || e.shiftKey);
    down = null;
  });

  const resize = (): void => {
    const w = window.innerWidth;
    const h = window.innerHeight;
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    renderer.setSize(w, h);
    composer.setSize(w, h);
    bloom.setSize(w, h);
  };
  window.addEventListener('resize', resize);
  resize();

  let last = performance.now();
  let time = 0;
  const frame = (): void => {
    const now = performance.now();
    const dt = Math.min((now - last) / 1000, 1 / 20);
    last = now;
    time += dt;
    game.update(dt, time);
    controls.update();
    composer.render();
    requestAnimationFrame(frame);
  };
  requestAnimationFrame(frame);
  hud.setStatus(`Seed ${seed} · ${world.guards.length} guards · ${world.layout.houses.length} houses`);
  hud.hideLoading();
}

main();
