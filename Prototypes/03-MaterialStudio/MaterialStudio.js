/*====================================================================================================================================
                                                      MATERIALSTUDIO.JS
====================================================================================================================================*/
// 🧩 Standalone Material & Texture Studio (Prototype 03) — Component Viewer theme + full material/lighting debug controls
//    + real Frontier ShaderBall.mesh viewport + live procedural/uploaded texture swapping.

import * as THREE from '/Experimental/Shared/Studio/Vendor/three.esm.min.js';

const $ = (id) => document.getElementById(id);

let ToastTimer = null;
function ShowToast(msg) {
    const t = $('AtelierToast');
    t.textContent = msg;
    t.classList.add('show');
    clearTimeout(ToastTimer);
    ToastTimer = setTimeout(() => t.classList.remove('show'), 2000);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRESETS & PALETTES
//------------------------------------------------------------------------------------------------------------------------

const PRESETS = [
    { id: 'amber',      name: 'Amber Lacquer',      kind: 'Dielectric Clearcoat', color: '#e59a2d', roughness: 0.22, metalness: 0.08, ior: 1.52, clearcoat: 0.55, coatRough: 0.09, specular: 1.0, sheen: 0.0, pattern: 'glaze',    repeat: 2, uvRot: 0, bump: 0.20, roughMix: 0.35, emissive: '#ffb238', emissiveInt: 0 },
    { id: 'titanium',   name: 'Brushed Titanium',   kind: 'Anisotropic Alloy',    color: '#aeb4ba', roughness: 0.28, metalness: 0.94, ior: 1.50, clearcoat: 0.08, coatRough: 0.25, specular: 1.0, sheen: 0.0, pattern: 'brushed',  repeat: 3, uvRot: 0, bump: 0.42, roughMix: 0.55, emissive: '#4d9bff', emissiveInt: 0 },
    { id: 'carbon',     name: 'Forged Carbon',      kind: 'Woven Composite',      color: '#25272b', roughness: 0.30, metalness: 0.22, ior: 1.56, clearcoat: 0.72, coatRough: 0.08, specular: 1.0, sheen: 0.0, pattern: 'carbon',   repeat: 4, uvRot: 0, bump: 0.55, roughMix: 0.65, emissive: '#ffb238', emissiveInt: 0 },
    { id: 'copper',     name: 'Anodized Copper',    kind: 'Satin Conductor',      color: '#c97a4b', roughness: 0.22, metalness: 0.96, ior: 1.50, clearcoat: 0.12, coatRough: 0.18, specular: 1.0, sheen: 0.0, pattern: 'brushed',  repeat: 2, uvRot: 0, bump: 0.26, roughMix: 0.38, emissive: '#ff8a4c', emissiveInt: 0 },
    { id: 'ceramic',    name: 'Kiln Ceramic',       kind: 'Glazed Mineral',       color: '#e5e2dc', roughness: 0.18, metalness: 0.00, ior: 1.54, clearcoat: 0.85, coatRough: 0.06, specular: 1.0, sheen: 0.0, pattern: 'terrazzo', repeat: 2, uvRot: 0, bump: 0.18, roughMix: 0.35, emissive: '#ffe4b8', emissiveInt: 0 },
    { id: 'cobalt',     name: 'Cobalt Enamel',      kind: 'Automotive Coat',      color: '#2b67b5', roughness: 0.16, metalness: 0.42, ior: 1.62, clearcoat: 0.95, coatRough: 0.04, specular: 1.0, sheen: 0.0, pattern: 'none',     repeat: 2, uvRot: 0, bump: 0.00, roughMix: 0.00, emissive: '#4d9bff', emissiveInt: 0 },
    { id: 'hexsteel',   name: 'Machined Hex',       kind: 'Milled Relief',        color: '#525b65', roughness: 0.34, metalness: 0.82, ior: 1.50, clearcoat: 0.00, coatRough: 0.20, specular: 1.0, sheen: 0.0, pattern: 'hex',      repeat: 3, uvRot: 0, bump: 0.75, roughMix: 0.70, emissive: '#46d267', emissiveInt: 0 },
    { id: 'embercore',  name: 'Signal Core',        kind: 'Emissive Surface',     color: '#262421', roughness: 0.46, metalness: 0.15, ior: 1.50, clearcoat: 0.15, coatRough: 0.22, specular: 1.0, sheen: 0.0, pattern: 'terrazzo', repeat: 2, uvRot: 0, bump: 0.45, roughMix: 0.50, emissive: '#ffb238', emissiveInt: 0.65 },
];

const PIGMENTS = ['#e59a2d', '#46d267', '#4d9bff', '#d95b52', '#7e63b8', '#c8c4bc', '#666b72', '#202226'];
const GLOW_SWATCHES = ['#ffb238', '#46d267', '#4d9bff', '#ff6b5e', '#c87aff', '#ffffff', '#777777', '#000000'];

const PATTERNS = [
    { id: 'none',     label: 'Uniform' },
    { id: 'carbon',   label: 'Carbon' },
    { id: 'brushed',  label: 'Brushed' },
    { id: 'terrazzo', label: 'Terrazzo' },
    { id: 'hex',      label: 'Hex Grid' },
    { id: 'glaze',    label: 'Ceramic' },
];

const State = {
    presetId: 'amber',
    ...PRESETS[0],
    customImage: null,
    customImageName: '',
    form: 'shaderball',
    rig: 'softbox',
    solo: 'pbr',
    wireframe: false,
    showShadow: true,
    exposure: 1.12,
    lightRotDeg: 35,
    envBoost: 1.25,
    autoOrbit: true,
    yaw: 0.55,
    pitch: 0.28,
    distance: 2.75,
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PROCEDURAL CANVAS TEXTURES
//------------------------------------------------------------------------------------------------------------------------

function SynthesizePatternCanvas(patternId, size = 256, mode = 'color', baseHex = '#ffffff') {
    const c = document.createElement('canvas');
    c.width = c.height = size;
    const g = c.getContext('2d');
    const base = new THREE.Color(baseHex);
    const rgb = (mul, bias = 0) => {
        const r = Math.max(0, Math.min(255, Math.round((base.r * mul + bias) * 255)));
        const gr = Math.max(0, Math.min(255, Math.round((base.g * mul + bias) * 255)));
        const b = Math.max(0, Math.min(255, Math.round((base.b * mul + bias) * 255)));
        return `rgb(${r},${gr},${b})`;
    };

    if (mode === 'uvchecker') {
        const cells = 12, cw = size / cells;
        for (let y = 0; y < cells; y++) for (let x = 0; x < cells; x++) {
            g.fillStyle = (x + y) % 2 ? '#d8d8dc' : '#26262b';
            g.fillRect(x * cw, y * cw, cw, cw);
        }
        g.strokeStyle = '#ffb238';
        g.lineWidth = 3;
        g.strokeRect(2, 2, size - 4, size - 4);
        return c;
    }

    g.fillStyle = mode === 'height' ? '#808080' : baseHex;
    g.fillRect(0, 0, size, size);

    if (patternId === 'carbon') {
        const cell = size / 16;
        for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
            const bright = (x + y) % 4 < 2;
            if (mode === 'height') {
                const grad = g.createLinearGradient(x * cell, y * cell, (x + 1) * cell, (y + 1) * cell);
                grad.addColorStop(0, bright ? '#383838' : '#b8b8b8');
                grad.addColorStop(0.5, bright ? '#e0e0e0' : '#505050');
                grad.addColorStop(1, bright ? '#383838' : '#b8b8b8');
                g.fillStyle = grad;
            } else {
                g.fillStyle = bright ? rgb(1.38, 0.05) : rgb(0.58, 0.0);
            }
            g.fillRect(x * cell, y * cell, cell, cell);
        }
    } else if (patternId === 'brushed') {
        for (let y = 0; y < size; y++) {
            const v = Math.sin(y * 1.9) * 0.5 + Math.sin(y * 5.3) * 0.3 + Math.sin(y * 13.7) * 0.2;
            g.fillStyle = mode === 'height' ? `rgb(${Math.round(128 + v * 58)},${Math.round(128 + v * 58)},${Math.round(128 + v * 58)})` : rgb(0.92 + v * 0.14);
            g.fillRect(0, y, size, 1);
        }
    } else if (patternId === 'terrazzo') {
        let seed = 1337;
        const rnd = () => { seed = (seed * 16807) % 2147483647; return (seed - 1) / 2147483646; };
        for (let i = 0; i < 110; i++) {
            const cx = rnd() * size, cy = rnd() * size, rad = 4 + rnd() * 14;
            g.beginPath();
            const sides = 5 + Math.floor(rnd() * 3);
            for (let s = 0; s < sides; s++) {
                const a = (s / sides) * Math.PI * 2 + rnd() * 0.4;
                const rr = rad * (0.65 + rnd() * 0.5);
                if (s === 0) g.moveTo(cx + Math.cos(a) * rr, cy + Math.sin(a) * rr);
                else g.lineTo(cx + Math.cos(a) * rr, cy + Math.sin(a) * rr);
            }
            g.closePath();
            const shade = rnd();
            g.fillStyle = mode === 'height' ? `rgb(${Math.round(90 + shade * 110)},${Math.round(90 + shade * 110)},${Math.round(90 + shade * 110)})` : (shade > 0.68 ? rgb(1.22, 0.08) : shade > 0.34 ? rgb(0.72) : rgb(0.45));
            g.fill();
        }
    } else if (patternId === 'hex') {
        const R = size / 10, dx = R * 1.5, dy = R * Math.sqrt(3);
        g.strokeStyle = mode === 'height' ? '#242424' : rgb(0.42);
        g.fillStyle = mode === 'height' ? '#d8d8d8' : rgb(1.12, 0.02);
        g.lineWidth = Math.max(2, size / 75);
        for (let row = -1; row < 12; row++) for (let col = -1; col < 12; col++) {
            const cx = col * dx, cy = row * dy + (col % 2 ? dy / 2 : 0);
            g.beginPath();
            for (let s = 0; s < 6; s++) {
                const a = (s / 6) * Math.PI * 2;
                if (s === 0) g.moveTo(cx + Math.cos(a) * R * 0.88, cy + Math.sin(a) * R * 0.88);
                else g.lineTo(cx + Math.cos(a) * R * 0.88, cy + Math.sin(a) * R * 0.88);
            }
            g.closePath(); g.fill(); g.stroke();
        }
    } else if (patternId === 'glaze') {
        for (let i = 0; i < 42; i++) {
            const x = ((i * 73) % size), y = ((i * 139) % size), r = 24 + (i * 19) % 58;
            const grad = g.createRadialGradient(x, y, 2, x, y, r);
            if (mode === 'height') {
                grad.addColorStop(0, i % 2 ? 'rgba(220,220,220,0.22)' : 'rgba(40,40,40,0.22)');
                grad.addColorStop(1, 'rgba(128,128,128,0)');
            } else {
                grad.addColorStop(0, i % 2 ? 'rgba(255,250,240,0.12)' : 'rgba(14,14,16,0.16)');
                grad.addColorStop(1, 'rgba(0,0,0,0)');
            }
            g.fillStyle = grad;
            g.fillRect(0, 0, size, size);
        }
    }
    return c;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THREE.JS ISOLATED STAGE
//------------------------------------------------------------------------------------------------------------------------

const Canvas = $('MaterialCanvas');
const Renderer = new THREE.WebGLRenderer({ canvas: Canvas, antialias: true, alpha: true, preserveDrawingBuffer: true });
Renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
Renderer.toneMapping = THREE.ACESFilmicToneMapping;
Renderer.toneMappingExposure = State.exposure;
Renderer.outputColorSpace = THREE.SRGBColorSpace;

const Scene = new THREE.Scene();
const Camera = new THREE.PerspectiveCamera(34, 1, 0.1, 50);

function BuildStudioEnv(rig) {
    const envScene = new THREE.Scene();
    envScene.background = new THREE.Color(rig === 'overcast' ? '#2c3035' : rig === 'dark' ? '#070709' : '#111114');
    const panel = (color, pos, scale, rot = [0, 0, 0]) => {
        const m = new THREE.Mesh(new THREE.PlaneGeometry(scale[0], scale[1]), new THREE.MeshBasicMaterial({ color, side: THREE.DoubleSide }));
        m.position.set(...pos); m.rotation.set(...rot); envScene.add(m);
    };
    if (rig === 'warmrim') {
        panel(new THREE.Color(3.8, 2.7, 1.3), [4, 2.5, -2.5], [4.5, 4.5], [0, -Math.PI / 2, 0]);
        panel(new THREE.Color(1.4, 1.8, 2.5), [-4.5, 2, 2.5], [5, 3], [0, Math.PI / 3, 0]);
        panel(new THREE.Color(2.2, 2.2, 2.2), [0, 5, 0], [5, 5], [Math.PI / 2, 0, 0]);
    } else if (rig === 'overcast') {
        panel(new THREE.Color(2.4, 2.5, 2.6), [0, 4.8, 0], [10, 10], [Math.PI / 2, 0, 0]);
        panel(new THREE.Color(1.2, 1.25, 1.3), [5, 1, 0], [8, 5], [0, -Math.PI / 2, 0]);
        panel(new THREE.Color(1.2, 1.25, 1.3), [-5, 1, 0], [8, 5], [0, Math.PI / 2, 0]);
    } else if (rig === 'dark') {
        panel(new THREE.Color(4.2, 4.2, 4.4), [4.2, 3.6, 2.2], [2.4, 4.5], [0, -Math.PI / 3, 0]);
        panel(new THREE.Color(2.0, 1.5, 0.8), [-4.2, 1.8, -2.6], [2.5, 3.2], [0, Math.PI * 0.72, 0]);
    } else {
        panel(new THREE.Color(3.7, 3.7, 3.6), [3.8, 3.2, 3.2], [4.2, 4.2], [0, -Math.PI / 4, 0]);
        panel(new THREE.Color(1.9, 2.0, 2.3), [-4.2, 2.2, -1.8], [4.5, 3.5], [0, Math.PI * 0.65, 0]);
        panel(new THREE.Color(2.5, 2.5, 2.4), [0, 5.2, 0], [6, 3], [Math.PI / 2, 0, 0]);
        panel(new THREE.Color(0.45, 0.45, 0.48), [0, -3.5, 0], [9, 9], [-Math.PI / 2, 0, 0]);
    }
    const pmrem = new THREE.PMREMGenerator(Renderer);
    const rt = pmrem.fromScene(envScene, 0.04);
    pmrem.dispose();
    return rt.texture;
}

const LightRigGroup = new THREE.Group();
const KeyLight = new THREE.DirectionalLight('#ffffff', 2.4);
KeyLight.position.set(3.2, 4.2, 3.0);
const RimLight = new THREE.DirectionalLight('#ffd9a0', 1.4);
RimLight.position.set(-3.8, 2.0, -3.2);
const FillLight = new THREE.HemisphereLight('#f0f0f2', '#1a1a1d', 0.65);
LightRigGroup.add(KeyLight, RimLight);
Scene.add(LightRigGroup, FillLight);

function MakeSoftShadowTexture() {
    const c = document.createElement('canvas');
    c.width = c.height = 256;
    const g = c.getContext('2d');
    const grad = g.createRadialGradient(128, 128, 12, 128, 128, 124);
    grad.addColorStop(0.0, 'rgba(0, 0, 0, 0.62)');
    grad.addColorStop(0.45, 'rgba(0, 0, 0, 0.30)');
    grad.addColorStop(0.80, 'rgba(0, 0, 0, 0.06)');
    grad.addColorStop(1.0, 'rgba(0, 0, 0, 0.0)');
    g.fillStyle = grad;
    g.fillRect(0, 0, 256, 256);
    return new THREE.CanvasTexture(c);
}

const FloorShadow = new THREE.Mesh(
    new THREE.PlaneGeometry(2.35, 2.35),
    new THREE.MeshBasicMaterial({ map: MakeSoftShadowTexture(), transparent: true, depthWrite: false }),
);
FloorShadow.rotation.x = -Math.PI / 2;
FloorShadow.position.y = -0.52;
Scene.add(FloorShadow);

const SpecimenGroup = new THREE.Group();
Scene.add(SpecimenGroup);

const PbrMat = new THREE.MeshPhysicalMaterial({
    color: State.color,
    roughness: State.roughness,
    metalness: State.metalness,
    ior: State.ior,
    clearcoat: State.clearcoat,
    clearcoatRoughness: State.coatRough,
    specularIntensity: State.specular,
    sheen: State.sheen,
    envMapIntensity: State.envBoost,
});

const UvCheckTex = new THREE.CanvasTexture(SynthesizePatternCanvas('none', 256, 'uvchecker'));
UvCheckTex.wrapS = UvCheckTex.wrapT = THREE.RepeatWrapping;
UvCheckTex.colorSpace = THREE.SRGBColorSpace;

let ShaderBallGeo = null;
let CurrentMesh = null;
let ActiveColorTex = null;
let ActiveBumpTex = null;
let ActiveEnv = null;
let StageDirty = true;
function MarkDirty() { StageDirty = true; }

async function LoadShaderBallGeometry() {
    try {
        const res = await fetch('/Experimental/FrontierWorldMirror/ShaderBall.mesh');
        const buf = await res.arrayBuffer();
        const header = new DataView(buf);
        const count = header.getUint32(4, true);
        const indexCount = header.getUint32(8, true);
        const verts = new Float32Array(buf, 16, count * 8);
        const indices = new Uint32Array(buf, 16 + count * 32, indexCount);

        const pos = new Float32Array(count * 3), norm = new Float32Array(count * 3), uv = new Float32Array(count * 2);
        for (let i = 0; i < count; i++) {
            const o = i * 8;
            pos[i * 3] = verts[o]; pos[i * 3 + 1] = verts[o + 2] - 0.56; pos[i * 3 + 2] = -verts[o + 1];
            norm[i * 3] = verts[o + 3]; norm[i * 3 + 1] = verts[o + 5]; norm[i * 3 + 2] = -verts[o + 4];
            uv[i * 2] = verts[o + 6]; uv[i * 2 + 1] = verts[o + 7];
        }
        const geo = new THREE.BufferGeometry();
        geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
        geo.setAttribute('normal', new THREE.BufferAttribute(norm, 3));
        geo.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
        geo.setIndex(new THREE.BufferAttribute(indices, 1));
        geo.computeBoundingSphere();
        const scale = 0.72 / (geo.boundingSphere.radius || 1);
        geo.scale(scale, scale, scale);
        geo.computeBoundingBox();
        geo.translate(0, -0.49 - geo.boundingBox.min.y, 0);
        ShaderBallGeo = geo;
    } catch {
        ShaderBallGeo = new THREE.SphereGeometry(0.64, 64, 48);
    }
    BuildSpecimenForm();
    MarkDirty();
}

function BuildClothGeometry() {
    const geo = new THREE.PlaneGeometry(1.38, 1.38, 64, 64);
    const p = geo.attributes.position;
    for (let i = 0; i < p.count; i++) {
        const x = p.getX(i), y = p.getY(i);
        p.setZ(i, Math.sin(x * 4.5 + y * 1.6) * 0.14 + Math.cos(y * 3.8 - x * 1.4) * 0.08);
    }
    geo.computeVertexNormals();
    geo.rotateX(-0.35);
    return geo;
}

function BuildSpecimenForm() {
    if (CurrentMesh) {
        SpecimenGroup.remove(CurrentMesh);
        if (CurrentMesh.geometry !== ShaderBallGeo) CurrentMesh.geometry.dispose();
    }
    let geo;
    if (State.form === 'shaderball' && ShaderBallGeo) geo = ShaderBallGeo;
    else if (State.form === 'cloth') geo = BuildClothGeometry();
    else if (State.form === 'cube') geo = new THREE.BoxGeometry(0.92, 0.92, 0.92, 16, 16, 16);
    else geo = new THREE.SphereGeometry(0.62, 64, 48);
    CurrentMesh = new THREE.Mesh(geo, ActiveDisplayMaterial());
    SpecimenGroup.add(CurrentMesh);
}

function ActiveDisplayMaterial() {
    const wf = State.wireframe;
    if (State.solo === 'normal') return new THREE.MeshNormalMaterial({ wireframe: wf });
    if (State.solo === 'uv') {
        UvCheckTex.repeat.set(State.repeat, State.repeat);
        UvCheckTex.rotation = (State.uvRot * Math.PI) / 180;
        return new THREE.MeshBasicMaterial({ map: UvCheckTex, wireframe: wf });
    }
    if (State.solo === 'albedo') {
        return new THREE.MeshBasicMaterial({
            color: State.customImage ? '#ffffff' : (State.pattern === 'none' ? State.color : '#ffffff'),
            map: ActiveColorTex || null,
            wireframe: wf,
        });
    }
    if (State.solo === 'roughness') {
        const v = Math.round(State.roughness * 255);
        return new THREE.MeshBasicMaterial({ color: new THREE.Color(`rgb(${v},${v},${v})`), map: ActiveBumpTex || null, wireframe: wf });
    }
    if (State.solo === 'metallic') {
        const v = Math.round(State.metalness * 255);
        return new THREE.MeshBasicMaterial({ color: new THREE.Color(`rgb(${v},${v},${v})`), wireframe: wf });
    }
    PbrMat.wireframe = wf;
    return PbrMat;
}

function ApplyRig() {
    if (ActiveEnv) ActiveEnv.dispose();
    ActiveEnv = BuildStudioEnv(State.rig);
    Scene.environment = ActiveEnv;
    if (State.rig === 'warmrim') { KeyLight.color.set('#fff0d6'); KeyLight.intensity = 2.6; RimLight.color.set('#ffb238'); RimLight.intensity = 2.6; }
    else if (State.rig === 'overcast') { KeyLight.color.set('#edf4ff'); KeyLight.intensity = 1.6; RimLight.color.set('#d5e1ed'); RimLight.intensity = 1.2; }
    else if (State.rig === 'dark') { KeyLight.color.set('#ffffff'); KeyLight.intensity = 3.0; RimLight.color.set('#ffb238'); RimLight.intensity = 2.2; }
    else { KeyLight.color.set('#ffffff'); KeyLight.intensity = 2.4; RimLight.color.set('#ffd9a0'); RimLight.intensity = 1.4; }
    MarkDirty();
}

function SyncMaterialToThree() {
    if (ActiveColorTex) { ActiveColorTex.dispose(); ActiveColorTex = null; }
    if (ActiveBumpTex) { ActiveBumpTex.dispose(); ActiveBumpTex = null; }

    const ConfigureTex = (tex, srgb = false) => {
        tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
        tex.repeat.set(State.repeat, State.repeat);
        tex.center.set(0.5, 0.5);
        tex.rotation = (State.uvRot * Math.PI) / 180;
        tex.colorSpace = srgb ? THREE.SRGBColorSpace : THREE.NoColorSpace;
        tex.needsUpdate = true;
        return tex;
    };

    if (State.customImage) {
        ActiveColorTex = ConfigureTex(new THREE.Texture(State.customImage), true);
        ActiveBumpTex = ConfigureTex(new THREE.Texture(State.customImage), false);
        PbrMat.color.set('#ffffff');
        PbrMat.map = ActiveColorTex;
        PbrMat.bumpMap = State.bump > 0.01 ? ActiveBumpTex : null;
        PbrMat.bumpScale = State.bump * 0.05;
        PbrMat.roughnessMap = State.roughMix > 0.05 ? ActiveBumpTex : null;
    } else if (State.pattern !== 'none') {
        ActiveColorTex = ConfigureTex(new THREE.CanvasTexture(SynthesizePatternCanvas(State.pattern, 256, 'color', State.color)), true);
        ActiveBumpTex = ConfigureTex(new THREE.CanvasTexture(SynthesizePatternCanvas(State.pattern, 256, 'height', '#ffffff')), false);
        PbrMat.color.set('#ffffff');
        PbrMat.map = ActiveColorTex;
        PbrMat.bumpMap = State.bump > 0.01 ? ActiveBumpTex : null;
        PbrMat.bumpScale = State.bump * 0.045;
        PbrMat.roughnessMap = State.roughMix > 0.05 ? ActiveBumpTex : null;
    } else {
        PbrMat.color.set(State.color);
        PbrMat.map = null;
        PbrMat.bumpMap = null;
        PbrMat.roughnessMap = null;
    }

    PbrMat.roughness = State.roughness;
    PbrMat.metalness = State.metalness;
    PbrMat.ior = State.ior;
    PbrMat.clearcoat = State.clearcoat;
    PbrMat.clearcoatRoughness = State.coatRough;
    PbrMat.specularIntensity = State.specular;
    PbrMat.sheen = State.sheen;
    PbrMat.sheenColor = new THREE.Color('#ffffff');
    PbrMat.envMapIntensity = State.envBoost;
    PbrMat.emissive.set(State.emissiveInt > 0 ? State.emissive : '#000000');
    PbrMat.emissiveIntensity = State.emissiveInt * 2.2;
    PbrMat.needsUpdate = true;

    Renderer.toneMappingExposure = State.exposure;
    LightRigGroup.rotation.y = (State.lightRotDeg * Math.PI) / 180;
    FloorShadow.visible = State.showShadow;

    if (CurrentMesh) CurrentMesh.material = ActiveDisplayMaterial();
    MarkDirty();
}

function ResizeStage() {
    const frame = $('MaterialStageFrame');
    const w = Math.max(200, frame.clientWidth);
    const h = Math.max(200, frame.clientHeight);
    Renderer.setSize(w, h, false);
    Camera.aspect = w / h;
    Camera.updateProjectionMatrix();
    MarkDirty();
}

let LastFrame = performance.now();
function Loop(now) {
    requestAnimationFrame(Loop);
    const dt = (now - LastFrame) / 1000;
    if (!StageDirty && !State.autoOrbit) return;
    if (State.autoOrbit && dt < 0.032) return;
    LastFrame = now;
    if (State.autoOrbit) State.yaw += Math.min(0.05, dt) * 0.28;
    const cp = Math.cos(State.pitch), sp = Math.sin(State.pitch);
    Camera.position.set(
        Math.sin(State.yaw) * cp * State.distance,
        sp * State.distance + 0.14,
        Math.cos(State.yaw) * cp * State.distance,
    );
    Camera.lookAt(0, 0.08, 0);
    Renderer.render(Scene, Camera);
    StageDirty = false;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    INFOGRAPHIC RENDERING
//------------------------------------------------------------------------------------------------------------------------

function RenderBrdfSvg() {
    const svg = $('BrdfSvg');
    const r = Math.max(0.06, State.roughness), m = State.metalness, cx = 55, cy = 88;
    const pts = [];
    for (let deg = -80; deg <= 80; deg += 4) {
        const rad = (deg * Math.PI) / 180;
        const spec = Math.exp(-Math.pow(rad / (r * 0.95), 2)) * (0.45 + m * 0.45 + (1 - r) * 0.35);
        const diff = (1 - m) * 0.28 * Math.cos(rad);
        const len = Math.min(72, (spec + diff) * 66);
        pts.push(`${(cx + Math.sin(rad) * len).toFixed(1)},${(cy - Math.cos(rad) * len).toFixed(1)}`);
    }
    svg.innerHTML = `
      <line x1="12" y1="${cy}" x2="98" y2="${cy}" stroke="#2f2f34" stroke-width="1"/>
      <line x1="${cx}" y1="${cy}" x2="${cx}" y2="14" stroke="#242428" stroke-dasharray="3 3"/>
      <polygon points="${cx},${cy} ${pts.join(' ')} ${cx},${cy}" fill="rgba(255,178,56,0.20)" stroke="#ffb238" stroke-width="1.6" stroke-linejoin="round"/>
      <circle cx="${cx}" cy="${cy}" r="3" fill="#f0f0f0"/>
    `;
}

function RenderFresnelSvg() {
    const svg = $('FresnelSvg');
    const W = 280, H = 124, padL = 22, padR = 14, padT = 14, padB = 22;
    const plotW = W - padL - padR, plotH = H - padT - padB;
    const f0 = Math.pow((State.ior - 1) / (State.ior + 1), 2);
    const CurvePath = (baseF0) => {
        const pts = [];
        for (let i = 0; i <= 24; i++) {
            const t = i / 24;
            const refl = baseF0 + (1 - baseF0) * Math.pow(t, 5);
            pts.push(`${i === 0 ? 'M' : 'L'} ${(padL + t * plotW).toFixed(1)} ${(padT + plotH - refl * plotH).toFixed(1)}`);
        }
        return pts.join(' ');
    };
    const mainD = CurvePath(f0);
    const coatD = CurvePath(Math.min(0.95, f0 + 0.04 * State.clearcoat));
    const puckX = padL + ((State.ior - 1.0) / 1.4) * plotW;
    const puckY = padT + plotH - State.clearcoat * plotH;

    svg.innerHTML = `
      <defs><linearGradient id="FresFill" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="rgba(255,178,56,0.26)"/><stop offset="100%" stop-color="rgba(255,178,56,0)"/></linearGradient></defs>
      <line x1="${padL}" y1="${padT + plotH}" x2="${W - padR}" y2="${padT + plotH}" stroke="#2d2d32"/>
      <line x1="${padL}" y1="${padT}" x2="${padL}" y2="${padT + plotH}" stroke="#2d2d32"/>
      <path d="${mainD} L ${W - padR} ${padT + plotH} L ${padL} ${padT + plotH} Z" fill="url(#FresFill)"/>
      <path d="${coatD}" fill="none" stroke="rgba(232,232,232,0.45)" stroke-width="1.2" stroke-dasharray="4 4"/>
      <path d="${mainD}" fill="none" stroke="#ffb238" stroke-width="2"/>
      <circle cx="${puckX.toFixed(1)}" cy="${puckY.toFixed(1)}" r="5.5" fill="#ffb238" stroke="#f0f0f0" stroke-width="2"/>
      <text x="${padL}" y="${H - 5}" fill="#777" font-size="9" font-family="DM Sans">0° Normal</text>
      <text x="${W - padR}" y="${H - 5}" text-anchor="end" fill="#777" font-size="9" font-family="DM Sans">90° Grazing</text>
    `;
    $('IorChip').textContent = `IOR ${State.ior.toFixed(2)} · F0 ${(f0 * 100).toFixed(1)}%`;
}

function SetRangeFill(id, val, min, max) {
    const el = $(id);
    if (!el) return;
    el.value = String(val);
    el.style.setProperty('--fill', `${((val - min) / Math.max(1e-6, max - min)) * 100}%`);
}

function RenderUI() {
    const patLabel = State.customImage ? State.customImageName : (PATTERNS.find((p) => p.id === State.pattern)?.label || 'Uniform');

    $('StageBrandDot').style.background = State.color;
    $('StageMatName').textContent = State.name;
    $('StageMatSub').textContent = `${State.kind} · ${patLabel} · ${State.form}`;
    $('StageSoloTag').textContent = State.solo === 'pbr' ? 'LIT PBR' : `SOLO ${State.solo.toUpperCase()}`;
    $('ActivePatternBadge').textContent = patLabel.toUpperCase();

    $('HeroSwatch').style.background = State.color;
    $('MaterialNameInput').value = State.name;
    $('HeroFormula').textContent = `${State.kind} · ${patLabel}`;
    $('HeroTagRow').innerHTML = `
      <span class="chip">BASE ${State.color.toUpperCase()}</span>
      <span class="chip">ROUGH ${State.roughness.toFixed(2)}</span>
      <span class="chip">METAL ${State.metalness.toFixed(2)}</span>
      <span class="chip warn">COAT ${State.clearcoat.toFixed(2)}</span>
    `;

    $('OpticalReadout').textContent = [
        `form   ${State.form}`,
        `ior    ${State.ior.toFixed(2)}   coat ${Math.round(State.clearcoat * 100)}%`,
        `rough  ${Math.round(State.roughness * 100)}%   metal ${Math.round(State.metalness * 100)}%`,
        `map    ${patLabel} (${State.repeat}×${State.repeat})`,
    ].join('\n');

    $('ColorHexLabel').textContent = State.color.toUpperCase();
    $('BaseColorInput').value = State.color;
    for (const d of $('PigmentSwatches').querySelectorAll('button')) d.classList.toggle('active', d.dataset.hex.toLowerCase() === State.color.toLowerCase());

    $('PadPuck').style.left = `${State.metalness * 100}%`;
    $('PadPuck').style.top = `${State.roughness * 100}%`;
    $('PadCrossX').style.top = `${State.roughness * 100}%`;
    $('PadCrossY').style.left = `${State.metalness * 100}%`;
    $('PadRoughNum').textContent = State.roughness.toFixed(2);
    $('PadMetalNum').textContent = State.metalness.toFixed(2);
    $('FinishQuadrant').textContent = State.metalness > 0.5
        ? (State.roughness < 0.35 ? 'Polished Metal' : 'Brushed Alloy')
        : (State.roughness < 0.35 ? 'Gloss Dielectric' : 'Matte Mineral');
    RenderBrdfSvg();

    $('ActiveTexLabel').textContent = patLabel;
    for (const tile of $('ProceduralTexGrid').querySelectorAll('button')) tile.classList.toggle('active', !State.customImage && tile.dataset.pattern === State.pattern);
    for (const tile of $('QuickTexStrip').querySelectorAll('button')) tile.classList.toggle('active', !State.customImage && tile.dataset.pattern === State.pattern);
    $('ClearUploadBtn').hidden = !State.customImage;
    $('UploadFileName').textContent = State.customImage ? State.customImageName : 'Drop or upload texture image';
    $('RepeatLabel').textContent = `${State.repeat}×${State.repeat} · Bump ${Math.round(State.bump * 100)}%`;
    for (const b of $('TileMatrix').querySelectorAll('button')) b.classList.toggle('active', Number(b.dataset.rep) === State.repeat);

    SetRangeFill('BumpRange', State.bump, 0, 1); $('BumpValLabel').textContent = `${Math.round(State.bump * 100)}%`;
    SetRangeFill('RoughMixRange', State.roughMix, 0, 1); $('RoughMixLabel').textContent = `${Math.round(State.roughMix * 100)}%`;
    SetRangeFill('UvRotRange', State.uvRot, 0, 360); $('UvRotLabel').textContent = `${State.uvRot}°`;

    RenderFresnelSvg();
    for (const b of $('IorPresets').querySelectorAll('button')) b.classList.toggle('active', Math.abs(Number(b.dataset.ior) - State.ior) < 0.04);
    $('CoatReadout').textContent = `${Math.round(State.clearcoat * 100)}% · Polish ${Math.round((1 - State.coatRough) * 100)}%`;
    SetRangeFill('CoatWeightRange', State.clearcoat, 0, 1); $('CoatWeightNum').textContent = `${Math.round(State.clearcoat * 100)}%`;
    SetRangeFill('CoatRoughRange', State.coatRough, 0, 1); $('CoatRoughNum').textContent = `${Math.round(State.coatRough * 100)}%`;
    SetRangeFill('SpecIntRange', State.specular, 0, 1); $('SpecIntNum').textContent = `${Math.round(State.specular * 100)}%`;
    SetRangeFill('SheenRange', State.sheen, 0, 1); $('SheenNum').textContent = `${Math.round(State.sheen * 100)}%`;

    $('EmissiveLabel').textContent = `${(State.emissiveInt * 2.2).toFixed(2)} cd`;
    SetRangeFill('EmissiveRange', State.emissiveInt, 0, 1); $('EmissiveNum').textContent = `${Math.round(State.emissiveInt * 100)}%`;
    for (const d of $('EmissiveSwatches').querySelectorAll('button')) d.classList.toggle('active', d.dataset.hex.toLowerCase() === State.emissive.toLowerCase());

    SetRangeFill('ExposureSlider', State.exposure, 0.5, 2.0); $('ExposureOut').textContent = `${State.exposure.toFixed(2)} EV`;
    SetRangeFill('LightRotSlider', State.lightRotDeg, -180, 180); $('LightRotOut').textContent = `${State.lightRotDeg}°`;
    SetRangeFill('EnvBoostSlider', State.envBoost, 0.2, 2.5); $('EnvBoostOut').textContent = `${State.envBoost.toFixed(2)}×`;

    for (const card of $('PresetStrip').querySelectorAll('button')) card.classList.toggle('active', card.dataset.preset === State.presetId);
    SyncMaterialToThree();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONTROLS WIRING
//------------------------------------------------------------------------------------------------------------------------

function InitControls() {
    const strip = $('PresetStrip');
    for (const p of PRESETS) {
        const btn = document.createElement('button');
        btn.type = 'button'; btn.className = 'preset-card'; btn.dataset.preset = p.id;
        btn.innerHTML = `<span class="preset-orb" style="background:${p.color}"></span><b>${p.name}</b><small>${p.kind}</small>`;
        btn.addEventListener('click', () => {
            Object.assign(State, { ...p, presetId: p.id, customImage: null, customImageName: '' });
            $('UploadThumbWell').innerHTML = '<span>+</span>';
            RenderUI();
            ShowToast(`Loaded ${p.name}`);
        });
        strip.append(btn);
    }
    $('StripPrev').addEventListener('click', () => strip.scrollBy({ left: -260, behavior: 'smooth' }));
    $('StripNext').addEventListener('click', () => strip.scrollBy({ left: 260, behavior: 'smooth' }));

    for (const hex of PIGMENTS) {
        const b = document.createElement('button');
        b.type = 'button'; b.className = 'swatch-dot'; b.dataset.hex = hex; b.style.background = hex;
        b.addEventListener('click', () => { State.color = hex; State.presetId = ''; RenderUI(); });
        $('PigmentSwatches').append(b);
    }
    for (const hex of GLOW_SWATCHES) {
        const b = document.createElement('button');
        b.type = 'button'; b.className = 'swatch-dot'; b.dataset.hex = hex; b.style.background = hex;
        b.addEventListener('click', () => { State.emissive = hex; if (State.emissiveInt === 0 && hex !== '#000000') State.emissiveInt = 0.45; RenderUI(); });
        $('EmissiveSwatches').append(b);
    }
    $('BaseColorInput').addEventListener('input', (e) => { State.color = e.target.value; State.presetId = ''; RenderUI(); });
    $('MaterialNameInput').addEventListener('change', (e) => { State.name = e.target.value.trim() || State.name; RenderUI(); });

    // 2D Finish Pad
    const pad = $('FinishPad');
    let padDrag = false;
    const SetPad = (ev) => {
        const r = pad.getBoundingClientRect();
        State.metalness = Math.max(0, Math.min(1, (ev.clientX - r.left) / r.width));
        State.roughness = Math.max(0.04, Math.min(1, (ev.clientY - r.top) / r.height));
        State.presetId = '';
        RenderUI();
    };
    pad.addEventListener('pointerdown', (ev) => { padDrag = true; pad.setPointerCapture(ev.pointerId); SetPad(ev); });
    pad.addEventListener('pointermove', (ev) => { if (padDrag) SetPad(ev); });
    pad.addEventListener('pointerup', () => { padDrag = false; });

    // Procedural patterns + Quick strip
    const ApplyPattern = (pat) => {
        State.pattern = pat.id; State.customImage = null; State.customImageName = '';
        if (pat.id !== 'none' && State.bump === 0) State.bump = 0.35;
        $('UploadThumbWell').innerHTML = '<span>+</span>';
        RenderUI(); ShowToast(`Applied ${pat.label} pattern`);
    };
    for (const pat of PATTERNS) {
        const tile = document.createElement('button');
        tile.type = 'button'; tile.className = 'tex-tile'; tile.dataset.pattern = pat.id;
        tile.append(SynthesizePatternCanvas(pat.id, 96, 'color', '#7d7d82'), Object.assign(document.createElement('span'), { textContent: pat.label }));
        tile.addEventListener('click', () => ApplyPattern(pat));
        $('ProceduralTexGrid').append(tile);

        const q = document.createElement('button');
        q.type = 'button'; q.className = 'qtex-btn'; q.dataset.pattern = pat.id;
        q.append(SynthesizePatternCanvas(pat.id, 64, 'color', '#86868c'), Object.assign(document.createElement('span'), { textContent: pat.label.split(' ')[0] }));
        q.addEventListener('click', () => ApplyPattern(pat));
        $('QuickTexStrip').append(q);
    }
    $('QuickUploadBtn').addEventListener('click', () => $('TexFileInput').click());

    for (const rep of [1, 2, 3, 4, 6, 8]) {
        const b = document.createElement('button');
        b.type = 'button'; b.className = 'tile-btn'; b.dataset.rep = String(rep); b.textContent = `${rep}×`;
        b.addEventListener('click', () => { State.repeat = rep; RenderUI(); });
        $('TileMatrix').append(b);
    }

    const LoadImg = (file) => {
        if (!file || !file.type.startsWith('image/')) return ShowToast('Choose a PNG, JPG, or WebP texture');
        const url = URL.createObjectURL(file);
        const img = new Image();
        img.onload = () => {
            State.customImage = img; State.customImageName = file.name;
            if (State.bump === 0) State.bump = 0.35;
            $('UploadThumbWell').innerHTML = '';
            const th = document.createElement('img'); th.src = url; $('UploadThumbWell').append(th);
            RenderUI(); ShowToast(`Applied texture ${file.name}`);
        };
        img.src = url;
    };
    const drop = $('TexDropzone');
    drop.addEventListener('click', () => $('TexFileInput').click());
    $('TexFileInput').addEventListener('change', (e) => { if (e.target.files?.[0]) LoadImg(e.target.files[0]); e.target.value = ''; });
    drop.addEventListener('dragover', (e) => { e.preventDefault(); drop.classList.add('hot'); });
    drop.addEventListener('dragleave', () => drop.classList.remove('hot'));
    drop.addEventListener('drop', (e) => { e.preventDefault(); drop.classList.remove('hot'); if (e.dataTransfer.files?.[0]) LoadImg(e.dataTransfer.files[0]); });
    $('ClearUploadBtn').addEventListener('click', () => { State.customImage = null; State.customImageName = ''; $('UploadThumbWell').innerHTML = '<span>+</span>'; RenderUI(); });

    // Range Sliders
    const BindRange = (id, key) => $(id).addEventListener('input', (e) => { State[key] = +e.target.value; RenderUI(); });
    BindRange('BumpRange', 'bump');
    BindRange('RoughMixRange', 'roughMix');
    BindRange('UvRotRange', 'uvRot');
    BindRange('CoatWeightRange', 'clearcoat');
    BindRange('CoatRoughRange', 'coatRough');
    BindRange('SpecIntRange', 'specular');
    BindRange('SheenRange', 'sheen');
    BindRange('EmissiveRange', 'emissiveInt');
    BindRange('ExposureSlider', 'exposure');
    BindRange('LightRotSlider', 'lightRotDeg');
    BindRange('EnvBoostSlider', 'envBoost');

    // Fresnel Interactive Pad
    const fpad = $('FresnelPad');
    let fDrag = false;
    const SetFresnel = (ev) => {
        const r = fpad.getBoundingClientRect();
        const nx = Math.max(0, Math.min(1, (ev.clientX - r.left - 22) / (r.width - 36)));
        const ny = Math.max(0, Math.min(1, 1 - (ev.clientY - r.top - 14) / (r.height - 36)));
        State.ior = Number((1.0 + nx * 1.4).toFixed(2));
        State.clearcoat = Number(ny.toFixed(2));
        RenderUI();
    };
    fpad.addEventListener('pointerdown', (ev) => { fDrag = true; fpad.setPointerCapture(ev.pointerId); SetFresnel(ev); });
    fpad.addEventListener('pointermove', (ev) => { if (fDrag) SetFresnel(ev); });
    fpad.addEventListener('pointerup', () => { fDrag = false; });
    $('IorPresets').addEventListener('click', (e) => { const b = e.target.closest('button'); if (b) { State.ior = Number(b.dataset.ior); RenderUI(); } });

    // Tabs & Viewport Debug Toggles
    $('ChannelTabs').addEventListener('click', (e) => {
        const b = e.target.closest('button'); if (!b) return;
        for (const btn of $('ChannelTabs').querySelectorAll('button')) btn.classList.toggle('active', btn === b);
        for (const pane of $('ChannelPanels').querySelectorAll('.tab-pane')) pane.hidden = pane.dataset.pane !== b.dataset.tab;
    });
    $('FormSwitch').addEventListener('click', (e) => {
        const b = e.target.closest('button'); if (!b) return;
        State.form = b.dataset.form;
        for (const btn of $('FormSwitch').querySelectorAll('button')) btn.classList.toggle('active', btn === b);
        BuildSpecimenForm(); RenderUI();
    });
    $('RigSwitch').addEventListener('click', (e) => {
        const b = e.target.closest('button'); if (!b) return;
        State.rig = b.dataset.rig;
        for (const btn of $('RigSwitch').querySelectorAll('button')) btn.classList.toggle('active', btn === b);
        ApplyRig();
    });
    $('SoloBar').addEventListener('click', (e) => {
        const b = e.target.closest('button'); if (!b) return;
        State.solo = b.dataset.solo;
        for (const btn of $('SoloBar').querySelectorAll('button')) btn.classList.toggle('active', btn === b);
        RenderUI();
    });
    $('MatWireBtn').addEventListener('click', (e) => { State.wireframe = !State.wireframe; e.currentTarget.classList.toggle('active', State.wireframe); RenderUI(); });
    $('MatShadowBtn').addEventListener('click', (e) => { State.showShadow = !State.showShadow; e.currentTarget.classList.toggle('active', State.showShadow); RenderUI(); });
    $('TurnToggle').addEventListener('click', (e) => { State.autoOrbit = !State.autoOrbit; e.currentTarget.classList.toggle('active', State.autoOrbit); MarkDirty(); });
    $('ResetCamBtn').addEventListener('click', () => { State.yaw = 0.55; State.pitch = 0.28; State.distance = 2.75; MarkDirty(); });

    const stageFrame = $('MaterialStageFrame');
    let drag = null;
    stageFrame.addEventListener('pointerdown', (e) => {
        State.autoOrbit = false; $('TurnToggle').classList.remove('active');
        drag = { x: e.clientX, y: e.clientY, yaw: State.yaw, pitch: State.pitch };
        stageFrame.setPointerCapture(e.pointerId);
    });
    stageFrame.addEventListener('pointermove', (e) => {
        if (!drag) return;
        State.yaw = drag.yaw - (e.clientX - drag.x) * 0.008;
        State.pitch = Math.max(-1.1, Math.min(1.1, drag.pitch + (e.clientY - drag.y) * 0.007));
        MarkDirty();
    });
    stageFrame.addEventListener('pointerup', () => { drag = null; });
    stageFrame.addEventListener('wheel', (e) => {
        e.preventDefault();
        State.distance = Math.max(1.35, Math.min(4.5, State.distance * (1 + e.deltaY * 0.001)));
        MarkDirty();
    }, { passive: false });

    $('ExportMaterialBtn').addEventListener('click', async () => {
        const spec = { Name: State.name, BaseColour: State.color, Roughness: +State.roughness.toFixed(3), Metallic: +State.metalness.toFixed(3), Ior: State.ior, Clearcoat: State.clearcoat, Pattern: State.customImage ? State.customImageName : State.pattern, Repeat: State.repeat };
        try { await navigator.clipboard.writeText(JSON.stringify(spec, null, 2)); ShowToast('Copied material JSON spec'); }
        catch { ShowToast(`Spec ready: ${State.name}`); }
    });
}

InitControls();
ApplyRig();
ResizeStage();
window.addEventListener('resize', ResizeStage);
RenderUI();
LoadShaderBallGeometry();
requestAnimationFrame(Loop);
