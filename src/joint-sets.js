// Regional joint sets: ONE rock-mass model evaluated twice — as displacement by the SDF
// carve (JS) and as grooves + block tone by the surface shader (GLSL) — so the painted
// blocks are the carved blocks. Everything is a pure function of world (x, z), bed index
// and the set parameters, so chunks agree with each other and with the paint.
//
// A set is a family of near-vertical planes: strike direction + spacing + per-plane
// position jitter + per-(plane, bed) dropout. Blocks are the cells between the planes of
// the two sets within one bed; each block has a deterministic tone, face offset and a
// chance of having fallen out (a scar socket).
//
// The GLSL mirror below MUST evaluate identically: same hash, same plane search, same
// cell rule. Float32 vs float64 can only disagree on measure-zero cell edges, where the
// groove shadow hides it.

import { mulberry32 } from './noise.js';

export function makeJointSets(seed) {
  const rand = mulberry32(((seed | 0) * 131 + 7) >>> 0);
  const a = rand() * Math.PI;
  const b = a + ((55 + rand() * 40) * Math.PI) / 180;
  return {
    ax: Math.cos(a), az: Math.sin(a),
    bx: Math.cos(b), bz: Math.sin(b),
    ox: (rand() - 0.5) * 2, oz: (rand() - 0.5) * 2,
    sa: Math.floor(rand() * 900) + 11, sb: Math.floor(rand() * 900) + 311,
  };
}

// integer hash identical to noise.hash2 (and to jsHash in the GLSL mirror)
export function jshash(x, y, seed) {
  let h = (Math.imul(x | 0, 374761393) + Math.imul(y | 0, 668265263) + Math.imul(seed | 0, 2246822519)) | 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177);
  h ^= h >>> 16;
  return (h >>> 0) / 4294967296;
}

const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);
const sstep = (a, b, v) => { const t = clamp((v - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); };

// spacing widens in thick beds (massive sandstone breaks into big blocks, shale into flags)
export function setSpacing(spacing, bedThick) {
  return spacing * clamp(0.65 + (bedThick / Math.max(0.5, spacing)) * 0.35, 0.65, 1.6);
}

// Evaluate one set. Returns { d (m to nearest plane), groove 0..1, cell, joint }.
// `stagger` offsets the lattice a little at every bed (joints step at bed boundaries);
// `domain` (m, 0 = off) varies the spacing panel by panel on a coarse world grid.
function evalSet(sx, sz, nx, nz, org, salt, bedIndex, spacing, width, dropout, stagger, domain, vary) {
  let sp = spacing;
  if (domain > 1) sp *= 1 + (0.7 + 0.6 * jshash(Math.floor((sx * 0.913 + 7.3) / domain), Math.floor((sz * 0.913 + 3.1) / domain), salt + 77) - 1) * vary;
  const ph = stagger > 0 ? (jshash(bedIndex, salt + 5, 3) - 0.5) * stagger * 0.5 : 0;
  const sc = (sx * nx + sz * nz) / sp + org + ph;
  const i0 = Math.floor(sc);
  const p0 = (i0 - 1) + (jshash(i0 - 1, salt, 9) - 0.5) * 0.5;
  const p1 = i0 + (jshash(i0, salt, 9) - 0.5) * 0.5;
  const p2 = (i0 + 1) + (jshash(i0 + 1, salt, 9) - 0.5) * 0.5;
  const p3 = (i0 + 2) + (jshash(i0 + 2, salt, 9) - 0.5) * 0.5;
  let dMin = Infinity, jIdx = i0;
  if (Math.abs(sc - p0) < dMin) { dMin = Math.abs(sc - p0); jIdx = i0 - 1; }
  if (Math.abs(sc - p1) < dMin) { dMin = Math.abs(sc - p1); jIdx = i0; }
  if (Math.abs(sc - p2) < dMin) { dMin = Math.abs(sc - p2); jIdx = i0 + 1; }
  if (Math.abs(sc - p3) < dMin) { dMin = Math.abs(sc - p3); jIdx = i0 + 2; }
  let cell = i0 - 1;
  if (p0 <= sc) cell = i0 - 1;
  if (p1 <= sc) cell = i0;
  if (p2 <= sc) cell = i0 + 1;
  if (p3 <= sc) cell = i0 + 2;
  const live = jshash(jIdx, bedIndex, salt + 50) >= dropout ? 1 : 0;
  const groove = (1 - sstep(0, width / sp, dMin)) * live;
  // closed joints are flush but still show a hairline (paint-only cue, no carve)
  const hair = (1 - sstep(0, (width * 0.45) / sp, dMin)) * (1 - live);
  return { d: dMin * sp, groove, hair, live, cell, joint: jIdx };
}

export function jointField(x, z, bedIndex, sets, spacing, bedThick, o = {}) {
  const {
    width = 0.5, dropout = 0.25, stagger = 0.8, domain = 0, loss = 0.1, lossSeed = 1237, vary = 0.7,
  } = o;
  const sp = setSpacing(Math.max(0.5, spacing), Math.max(0.3, bedThick));
  const A = evalSet(x, z, sets.ax, sets.az, sets.ox, sets.sa, bedIndex, sp, width, dropout, stagger, domain, vary);
  const B = evalSet(x, z, sets.bx, sets.bz, sets.oz, sets.sb, bedIndex, sp, width, dropout, stagger, domain, vary);
  const groove = Math.max(A.groove, B.groove);
  const hair = Math.max(A.hair, B.hair);
  const d = Math.min(A.d, B.d);
  const key = A.cell * 73 + B.cell * 191;
  // tone and offset persist across beds (a joint-bounded column of rock), with a smaller
  // per-bed jitter - resetting them at every bed boundary reads as brickwork
  const blockTone = (jshash(key, 7, 501) - 0.5) * 0.45 + (jshash(key, bedIndex, 501) - 0.5) * 0.55;
  const off = ((jshash(key, 7, 503) - 0.5) * 0.6 + (jshash(key, bedIndex, 503) - 0.5) * 0.4) * 2;
  const off2 = ((jshash(key, 7, 505) - 0.5) * 0.6 + (jshash(key, bedIndex, 505) - 0.5) * 0.4) * 2;
  const scar = jshash(key, bedIndex, lossSeed) < loss ? 1 : 0;
  return { d, groove, hair, cellA: A.cell, cellB: B.cell, blockTone, off, off2, scar };
}

// vec4 uniforms for the shader: (normalX, normalZ, origin, seed)
export function jointSetUniforms(seed) {
  const s = makeJointSets(seed);
  return { A: [s.ax, s.az, s.ox, s.sa], B: [s.bx, s.bz, s.oz, s.sb] };
}

export const jointSetsGLSL = /* glsl */`
float jsHash( int x, int y, int seed ) {
  uint h = uint( x ) * 374761393u + uint( y ) * 668265263u + uint( seed ) * 2246822519u;
  h = ( h ^ ( h >> 13u ) ) * 1274126177u;
  h = h ^ ( h >> 16u );
  return float( h ) / 4294967296.0;
}
float jsPlane( int i, int salt ) {
  return float( i ) + ( jsHash( i, salt, 9 ) - 0.5 ) * 0.5;
}
// one joint set; returns (groove, signed metres to the nearest plane, cell, live).
// Plane search, cell rule and hashes mirror evalSet above exactly.
vec4 jsSet( vec2 xz, vec2 nrm, float org, int salt, int bed, float sp, float width, float dropout, float stagger, float domain, float vary ) {
  float spp = sp;
  if ( domain > 1.0 ) spp *= 1.0 + ( 0.7 + 0.6 * jsHash( int( floor( ( xz.x * 0.913 + 7.3 ) / domain ) ), int( floor( ( xz.y * 0.913 + 3.1 ) / domain ) ), salt + 77 ) - 1.0 ) * vary;
  float ph = stagger > 0.0 ? ( jsHash( bed, salt + 5, 3 ) - 0.5 ) * stagger * 0.5 : 0.0;
  float sc = dot( xz, nrm ) / spp + org + ph;
  int i0 = int( floor( sc ) );
  float dMin = 1e9; int jIdx = i0;
  for ( int i = i0 - 1; i <= i0 + 2; i++ ) {
    float d = abs( sc - jsPlane( i, salt ) );
    if ( d < dMin ) { dMin = d; jIdx = i; }
  }
  int cell = i0 - 1;
  for ( int i = i0 - 1; i <= i0 + 2; i++ ) if ( jsPlane( i, salt ) <= sc ) cell = i;
  float live = jsHash( jIdx, bed, salt + 50 ) >= dropout ? 1.0 : 0.0;
  float groove = ( 1.0 - smoothstep( 0.0, width / spp, dMin ) ) * live;
  float signed = ( sc - jsPlane( jIdx, salt ) ) * spp;
  return vec4( groove, signed, float( cell ), live );
}
`;
