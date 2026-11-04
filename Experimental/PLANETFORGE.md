# Planet Forge (planet-scale SDF terrain + adaptive ocean)

The base world-authoring layer for Frontier lives in `PlanetForge/`.

- Read `PlanetForge/README.md` for the architecture, measured numbers and limits.
- Read `PlanetForge/PLAN.md` for the decisions and the ordered plan for the
  erosion studio (cracks, chemical erosion, strata, snow/thermal).

```bash
cd PlanetForge && npm install && npm run dev
```

It is self-contained (Three.js + Vite + Vitest) and imports nothing from the rest of
the repository, so it can be evaluated — or ported — without touching the C++ tree.
