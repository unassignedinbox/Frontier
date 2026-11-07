/**
 * Named, cached material library. Each look is created once and shared by every
 * mesh that uses it, which keeps shader program and draw-state churn low.
 */
import { createProcMaterial, ProcMaterial } from './procMaterial';

const cache = new Map<string, ProcMaterial>();

function cached(key: string, make: () => ProcMaterial): ProcMaterial {
  let m = cache.get(key);
  if (!m) {
    m = make();
    cache.set(key, m);
  }
  return m;
}

export const MAT = {
  gingerbread: () => cached('gingerbread', () => createProcMaterial({ kind: 'gingerbread' })),
  gingerbreadDark: () =>
    cached('gingerbreadDark', () =>
      createProcMaterial({ kind: 'gingerbread', colorA: '#6e3416', colorB: '#a8602f', colorC: '#d9b98a' }),
    ),
  icing: () => cached('icing', () => createProcMaterial({ kind: 'royalIcing', colorA: '#fff9f5', params: { x: 0.22 } })),
  icingPink: () =>
    cached('icingPink', () => createProcMaterial({ kind: 'royalIcing', colorA: '#ffd6e7', params: { x: 0.3 } })),
  icingMint: () =>
    cached('icingMint', () => createProcMaterial({ kind: 'royalIcing', colorA: '#d7fff1', params: { x: 0.3 } })),
  chocolate: () => cached('chocolate', () => createProcMaterial({ kind: 'chocolate' })),
  chocolateRoof: () =>
    cached('chocolateRoof', () => createProcMaterial({ kind: 'chocolate', colorA: '#3a1b0d', params: { x: 0.4 } })),
  fudge: () =>
    cached('fudge', () =>
      createProcMaterial({ kind: 'fudgeBrick', colorA: '#6b3418', colorB: '#f0d7b3', colorC: '#2a120a', params: { x: 0.9 } }),
    ),
  fudgePale: () =>
    cached('fudgePale', () =>
      createProcMaterial({ kind: 'fudgeBrick', colorA: '#c08a5b', colorB: '#fff1dc', colorC: '#7a4a2a', params: { x: 0.8 } }),
    ),
  peppermintRed: () =>
    cached('peppermintRed', () => createProcMaterial({ kind: 'peppermint', params: { x: 6, z: 0.6 } })),
  peppermintMint: () =>
    cached('peppermintMint', () =>
      createProcMaterial({ kind: 'peppermint', colorA: '#22c9a0', colorB: '#f7fffc', params: { x: 5, z: 0.5 } }),
    ),
  swirlPink: () =>
    cached('swirlPink', () => createProcMaterial({ kind: 'swirl', colorA: '#ff3d7f', colorB: '#fff0f6' })),
  swirlMint: () =>
    cached('swirlMint', () => createProcMaterial({ kind: 'swirl', colorA: '#1fbf8f', colorB: '#f2fff9' })),
  swirlLemon: () =>
    cached('swirlLemon', () => createProcMaterial({ kind: 'swirl', colorA: '#ffd23f', colorB: '#fffbe6' })),
  swirlGrape: () =>
    cached('swirlGrape', () => createProcMaterial({ kind: 'swirl', colorA: '#8e4dff', colorB: '#f5efff' })),
  swirlGlow: () =>
    cached('swirlGlow', () =>
      createProcMaterial({ kind: 'swirl', colorA: '#ffb3d1', colorB: '#fff7fb', params: { w: 1.4, y: 0.2 } }),
    ),
  licorice: () => cached('licorice', () => createProcMaterial({ kind: 'licorice' })),
  candyGround: () => cached('candyGround', () => createProcMaterial({ kind: 'candyGround', params: { x: 0 } })),
  candyGroundConfetti: () =>
    cached('candyGroundConfetti', () =>
      createProcMaterial({ kind: 'candyGround', colorA: '#c7739a', colorB: '#f7c6dc', params: { x: 0.18 } }),
    ),
  chocolateRiver: () =>
    cached('chocolateRiver', () => createProcMaterial({ kind: 'chocolateFlow', params: { x: 0.35, y: 0.5 } })),
  chocolateFountain: () =>
    cached('chocolateFountain', () =>
      createProcMaterial({ kind: 'chocolateFlow', colorA: '#3a1a0b', colorB: '#a86a2a', params: { x: 0.9, y: 0.4 } }),
    ),
  bullseye: () =>
    cached('bullseye', () => createProcMaterial({ kind: 'bullseye', params: { z: 1.0 } })),
  sugarGlass: () => cached('sugarGlass', () => createProcMaterial({ kind: 'sugarGlass', colorA: '#ffc46b' })),
  sugarGlassCool: () =>
    cached('sugarGlassCool', () => createProcMaterial({ kind: 'sugarGlass', colorA: '#bfe9ff', params: { w: 0.6 } })),
};

/** Jelly-bean gel in a given flavour. Used for guards and gumdrops. */
export function jellyMaterial(hex: string, opts: { jiggle?: boolean; sugar?: number } = {}): ProcMaterial {
  const key = `jelly:${hex}:${opts.jiggle ? 'j' : 's'}:${opts.sugar ?? 0.6}`;
  return cached(key, () =>
    createProcMaterial({
      kind: 'jelly',
      colorA: hex,
      jiggle: opts.jiggle,
      clearcoat: 0.8,
      params: { x: opts.sugar ?? 0.6, y: 0.7 },
    }),
  );
}

/** Non-cached jelly for animated bodies: each guard owns its wobble uniform. */
export function guardJelly(hex: string): ProcMaterial {
  return createProcMaterial({
    kind: 'jelly',
    colorA: hex,
    jiggle: true,
    clearcoat: 0.9,
    params: { x: 0.8, y: 0.9 },
  });
}

export const materialCount = (): number => cache.size;
