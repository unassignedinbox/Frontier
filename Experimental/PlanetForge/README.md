# Planet Forge — planet-scale SDF terrain authoring + adaptive ocean

A runnable base for Frontier's world authoring. It is a **planet**, not a plate:
a 6371 km sphere addressed as a cube-sphere quadtree, with an SDF document as the
single source of truth, meshed from a volume (so cracks and overhangs are legal),
streamed under a hard RAM/VRAM budget, and rendered with an ocean that is solved
rather than textured.

Built to be the *first layer* you asked for — sculpting landmasses, islands, seas
and shelves, plus a real ocean — with the seams deliberately left in place for
the later erosion work.

```bash
cd Experimental/PlanetForge
npm install
npm run dev          # http://localhost:5173
npm test             # 49 tests: geometry, field, mesher, water, render paths
npm run typecheck
npm run build
```

---

## 1. What the model actually is

```
  SDF document (kilobytes, editable, diffable)
        │  parametric operators: caps, capsules, ellipses, bands, fractures
        ▼
  TerrainField · pure function of direction on the sphere
        │  coastDistance(dir)  heightAt(dir)  densityAt(p)
        ▼
  chunk mesh (worker)  ←──────────────  LOD cut (frontier refinement)
        │  dual contouring on an altitude-packed lattice
        ▼
  camera-relative render (float64 origin, float32 on the GPU)
```

Nothing in that chain is a bitmap, and nothing scales with world size. The cost of
a frame is a function of the **budget** (chunks, cells, rings), not of how much
planet exists.

| The brief | How it is met |
| --- | --- |
| planet scale, not a 100 km square | cube-sphere quadtree; 6 faces, up to 2^17 chunks/edge ≈ 33 m cells |
| no heightmap | the document is parametric SDF; height is a *query* on it, never storage |
| SDF because particles will erode it | the field is analytic and resolution independent; erosion can sample or edit it |
| convert to mesh afterwards | every chunk is already a mesh; the extractor is volume-based |
| cracks that SDF "cannot" do | cracks *are* SDF: `F2-F1` Worley, subtracted as real volume |
| home out on an SSD | disk cache written in batches, keyed spatially, read in a bounded window |
| do not kill RAM / VRAM | hard chunk + byte budgets, LRU eviction by on-screen importance |
| not a fake ocean | 4 rings of finite-volume shallow-water, nested coarse→fine, no normal map |
| realistic shore | wet/dry face handling, depth-driven body colour, steepness-driven foam |
| same UI as Frontier editor | near-black desk, one blue accent, hairline panels, left tool rail |

---

## 2. Why a cube-sphere, and what "planet scale" costs

The world is six quadtrees over cube faces projected onto the sphere. A chunk is a
square patch on one face; a direction on the sphere maps to `(face, s, t)` and back
exactly (round-trip error < 1e-6 rad, tested). There are no poles and no seams in
the *field*, because the field is authored on the direction vector, not on a
parameter grid.

One honest number: the projection is equal-angle, so cell area varies by up to
~5.2× between a face centre and a face corner. That is handled rather than hidden —
LOD selection measures the **real** great-circle edge length of each chunk instead
of assuming uniformity (see `chunkEdgeMetresLocal`).

Seams between chunks at *different* LODs are closed by construction rather than by
skirts: every chunk is meshed on the same dual-contouring lattice, so two neighbours
evaluate the field at identical physical points along their shared border. A stitch
pass cannot drift out of sync because there is no stitch pass.

**Altitude band packing.** A 900 m wide chunk with 300 m of relief would need a
300 m tall lattice to keep the vertical step comparable to the horizontal one.
Instead the vertical axis is packed into the same cube as the horizontal axes and
unpacked when vertices are placed on the sphere. This is not an approximation of
the surface — the surface is still `dir · (R + h)` — it is a change of lattice
coordinates that keeps the dual mesh well conditioned.

---

## 3. Meshing: one lattice, two scalars

`chunkMesher.ts` builds a `(cells+1)³` lattice and extracts its isosurface with
naive Surface Nets (dual contouring without QEFs: ~3× fewer triangles than Marching
Cubes, watertight by construction, no cracks between adjacent cells).

The **scalar** is what differs:

| Path | Scalar | Use | Measured cost (32³, detail on) |
| --- | --- | --- | --- |
| `radial` | `altitude − height(dir)` | most of a planet | **~19 ms** |
| `volumetric` | the 3D SDF, cracks included | worlds with fracture operators | **~40 ms** |

The radial scalar is separable: `height` depends only on the column, so all the
expensive field work happens once per column and the altitude walk is a
subtraction. That is why it is only 2× the cost of the volume path despite the
volume path evaluating a full 3D field per sample.

Both paths place vertices at `dir · (R + h)` — **exact sphere, zero tangent-plane
sag**. A test checks this directly: for a chunk where a flat plate would sag
kilometres below the centre, the measured radial spread across the mesh must be
under 2 % of that sag.

Measured per-chunk output (this laptop-class container, no GPU):

```
cells=16   ~19 ms   450 tris    14 KB
cells=32   ~19 ms  1922 tris    59 KB
cells=48   ~38 ms  4418 tris   133 KB
far LOD (detail off, 32³)      ~13 ms
```

Because those numbers are per chunk and independent of planet size, a resident set
of 900 chunks at 32³ is **~53 MB of vertex data and ~1.7 M triangles** — a normal
project's worth, for a whole planet.

---

## 4. Streaming that survives an HDD

### LOD is a frontier, not a truncation

`selectChunks` starts from six face roots and repeatedly splits the chunk that is
most important on screen, stopping at a hard chunk budget. Because it only ever
replaces a node with its four children, the result is **always a valid cut**: no
overlap, and no holes, at any budget. (An earlier version built a fine cut and then
truncated it — that leaves holes, which is much worse than low detail.)

Two measured cuts, 6371 km planet, 900-chunk budget:

| Camera | Chunks | LOD range | Triangles |
| --- | --- | --- | --- |
| 2 km above ground | 900 (216 splits refused) | 0–17 | 1.96 M |
| 4000 km (orbit) | 225 | 2–4 | 0.49 M |

The near cut reaches LOD 17 locally while distant terrain falls to LOD 0–2 — that
ratio is the whole point of a quadtree, and it is why one frame can contain a rock
and a continent.

### RAM and VRAM are capped, not hoped for

`ChunkStreamer` enforces a resident chunk count **and** an uploaded-byte budget.
Eviction is LRU weighted by on-screen importance, so the far side of the planet can
never evict the ground you are standing on. Jobs for chunks that left the cut are
cancelled before they cost a millisecond.

### The HDD part

`MeshCache` is the part specifically designed around a spinning disk rather than
NVMe:

1. **One blob per chunk.** Packed into a single `ArrayBuffer` with a 64-byte header.
   An HDD pays for seeks, not bytes: one sequential 60 KB write beats 40 writes of
   1.5 KB.
2. **Batched writes.** Chunks finish in bursts while flying; writes are coalesced
   into one IndexedDB transaction per flush interval, with at most one flush in
   flight.
3. **Bounded read window.** `getMany` issues a small number of concurrent gets
   (default 4). More parallelism on an HDD destroys throughput; an SSD is capped by
   the same window and stays fast.
4. **Spatially ordered keys.** Keys are `worldHash|face/lod/x/y`, so a region's
   chunks occupy a contiguous key range and are read roughly sequentially. This is
   the same locality property that makes geometry streaming fast on an SSD,
   obtained from the key layout instead of from hardware.
5. **Versioned by document hash** with byte-budget eviction, so a stale world never
   poisons a new one, and the cache cannot grow without bound.

Honest limits: IndexedDB can be cleared by the browser, and first-time (uncached)
generation is CPU-bound meshing, not disk. Measured ~19 ms/chunk ÷ N workers means
a cold 900-chunk region takes roughly **2.5 s on 6 workers** to become resident;
after that, revisiting is a disk read. That is the shape you want: pay once per
place, not once per visit.

---

## 5. The ocean is solved, not drawn

Ring 0 is 128 × 128 cells at 6 m; the four inner rings run a finite-volume
shallow-water solver on an Arakawa C-grid:

```
∂η/∂t = −∇·(H u)                     conservation, staggered so div is exact
∂u/∂t = −g ∂η/∂x + wind + friction   momentum, face velocities
```

Three details that make it stable and believable:

* **CFL substeps.** dt is chosen per frame from the fastest wave the grid can carry.
  Nothing is tuned by hand, and a 4000 m trench cannot blow the frame up.
* **Dispersion limit.** Deep water plus a 6 m grid would demand 200 m/s gravity
  waves. Real short gravity waves are dispersive (`c = √(g/k)`), so celerity is
  capped at the physically correct deep-water limit for the grid Nyquist. This is
  physics, not a fudge, and it is what keeps the near patch at a couple of substeps
  per frame.
* **Wet/dry faces.** On land, depth goes to zero and the face closes (reflective),
  so a swash front climbs a beach and retreats. The free surface rests *on* the bed
  and is never relaxed upward through it.

**Nesting.** Coarse → fine, every frame: ring 3's boundary drives ring 2, which
drives ring 1, which drives ring 0. This is one-way nesting, exactly what regional
ocean models do, and it removes the "near water is a different ocean from far
water" seam that a single patch always has.

**Where simulation stops, and why that is honest.** Rings 0–3 are solved (up to
~5 km, 6 m cells). Rings 4–8 are analytic **dispersive swell**: real gravity waves
with `c = √(g/k)` per component, used as the outer boundary condition. At 24 km
cells, shallow-water theory (wavelength ≫ depth) is simply not the right model, and
simulating it would cost CFL-bounded steps for a field smaller than one pixel of
error. The line is drawn where the physics changes, not where it got expensive.

**Shading has no wave normal map.** Per-pixel normals come from the analytic
gradient of the *simulated* η field, uploaded as a float texture. Body colour comes
from the real seabed depth under each pixel. Foam is a function of steepness and
shallowness. Reflections call the same `skyRadiance()` the sky dome uses, so a
reflected sunset matches the actual sunset.

`followCamera` shifts the patches in **whole cells**, preserving surface state, and
ring meshes are translated between rebuilds (rebuild only when drift exceeds 220 m
or the frame rotates). Without that last rule a fast flight would rebuild ~600 k
vertices every frame and the ocean would *be* the frame cost.

---

## 6. Cracks: yes, SDF can do them

This is the question you said you had not solved, so it is worth stating precisely.

A heightmap cannot represent a crack, because a crack is not a single-valued
function of height: its walls overhang by definition. A volume SDF can, and this
build already does it:

```
F1, F2 = two nearest Worley feature points at cell size c
edge   = (F2 − F1) · c            // 0 exactly on a cell boundary
carve  = max(0, width − edge)     // tapering to zero at depth `depth`
density(p) −= carve
```

That produces a polygonal crack network with real walls, real depth, and real
occlusion — desiccation polygons, tectonic joint sets, or radial impact fractures,
region-masked and gated by altitude so they do not appear underwater. A test
verifies that a crack opens at least one sample into empty space along a scan line,
and the volumetric mesher emits geometry for it (1922 tris on a test chunk).

What is *not* solved yet, stated plainly: cracks currently follow Worley cells, so
they are evenly spaced. Real jointing is hierarchical (big fractures, then
step-overs, then splays) and is driven by the stress field. Making it hierarchical
(domain-warped cells, octave-mixed widths, and a direction bias from the nearest
ridge) is a contained change inside `fractureAmount` — no change to meshing,
streaming or rendering.

---

## 7. The editor

`editor/ui.ts` — deliberately small and deliberately Frontier-shaped: near-black
desk (`#0a0a0b`), one blue accent (`#5b8cff`), hairline dividers, 18 px panel
radius, uppercase micro-labels, numeric value pills, a left tool rail rather than a
modal toolbox.

**Tools** — `1` Inspect · `2` Teleport · `3` Landmass · `4` Island · `5` Sea/gulf ·
`6` Shelf · `7` Raise · `8` Lower · `9` Smooth · `0` Flatten · `P` Plateau ·
`C` Canyon · `R` Roughen · `Z` Water drop · `X` Current.

**Viewport** — RMB look, MMB pan, WASD fly, Q/E down/up, Shift/Ctrl fast/slow, LMB
use tool, wheel changes fly speed, F focus, Ctrl+Z undo.
**Panels** — tool options, world (sea level, shelf, ocean depth, relief, coast
noise), ocean (wind stress/angle, swell, bed friction), streaming budget, outliner,
live telemetry.

Every sliders acts on the *document* or the *budget* — there is no hidden render
setting that silently changes what the world is.

---

## 8. Honest limitations

* **Chunk seams at LOD transitions are closed by matching dual meshes, but the
  curved placement of a coarse chunk's straight edge is chord, not arc.** The error
  is sub-pixel at any viewpoint where that LOD is selected; it is not zero. A
  crack-free-by-construction variant would evaluate border vertices from the fine
  side, at the cost of a neighbour query.
* **The radial path clamps the band below the lowest column.** Underground geometry
  (caves) requires the volume path, which is selected per-document, not per-chunk.
* **Water is not yet coupled to the terrain.** The bed is a boundary condition;
  there is no erosion from the water, no sediment transport, no wetting that
  carves. That is the next layer, and it is the layer that will make the coastlines
  actually evolve.
* **The far ocean is a boundary condition**, as described above. It looks right and
  it moves at the right dispersion, but it is not solved.
* **Meshing is CPU, in workers.** A GPU compute mesher would cut cold-start latency
  by an order of magnitude; the worker contract is written so that swap is local.
* **The planet's rotation is not modelled.** Wind is constant in frame space; there
  is no Coriolis term yet, so large-scale gyres will not form. That matters once
  erosion and currents need to talk to each other.

---

## 9. Roadmap, in the order the work actually makes sense

You asked to plan ahead rather than bolt features on. This order is chosen so each
step makes the next one cheaper:

1. **Sculpting polish** (current base) — rotate/scale existing operators, paint
   masks, a region layer so a landscape can be authored once and reused.
2. **Hierarchical fracture** — octave-mixed Worley, stress bias from ridges, and
   crack width driven by strata hardness. Contained inside `fractureAmount`.
3. **Strata as real material** — bands already exist as datums; give them
   depositional dip, thickness variation and a hardness term that the erosion step
   reads. This is what makes chemical erosion *differentiated* instead of uniform.
4. **Particle erosion on the SDF** — the field is already sampling-friendly; the
   erosion pass is a particle sim advecting sediment over `heightAt` and writing
   back strokes/operators. Thermal expansion and freeze–thaw are the cheap first
   two (they are local slope rules), chemical dissolution needs the hardness field
   from step 3.
5. **Mesh baking** — bake the accepted state to static meshes per region, then
   stream *those* (far smaller than re-evaluating the field), which is the last big
   win for a shipping build.

`Experimental/` is the right home for 2–4: the erosion studio is a separate tool
that consumes and mutates the same document.

---

## 10. File map

```
src/
  core/
    math/vec.ts            vector helpers, rotations
    math/noise.ts          deterministic 3D value/gradient noise, fbm, ridged, warley seeds
    sdf/cubesphere.ts      cube-sphere addressing, chunk keys, lat/lon, tangent frames
    sdf/primitives.ts      cap / capsule / ellipse SDFs, smooth booleans
    world/terrainWorld.ts  the document: operators, relief, presets, hash
    world/terrainField.ts  coastDistance, heightAt, densityAt, surfaceInfo, fractures
    world/shading.ts       procedural coast/strata/snow/crack tint
    mesh/surfaceNets.ts    dual contouring + heightfield fallback + normals
    mesh/chunkMesher.ts    altitude-packed lattice, radial + volumetric paths
    stream/lod.ts          frontier LOD refinement, horizon cull
    stream/chunkStreamer.ts worker pool, budgets, cancellations
    stream/meshCache.ts    HDD-oriented batched IndexedDB blob cache
    water/shallowWater.ts  finite-volume SWE, CFL, wet/dry, clipmap shift
  render/
    terrainRenderer.ts     camera-relative chunk placement, sky, atmosphere
    oceanRenderer.ts       nested rings, per-pixel normals, depth shading
    sky.glsl.ts            shared sky function (dome + water reflections)
  workers/
    meshWorker.ts          per-core field instance and mesher
    protocol.ts            transferred-buffer message contract
  editor/
    ui.ts, styles.css      Frontier-style chrome
  app/
    planetForge.ts         frame loop, tools, input, telemetry
    cameraRig.ts           double-precision fly camera, terrain picking
tests/                     49 tests
tools/bench.test.ts        measured performance characterisation
```

Run `npx vitest run tools/bench.test.ts` to reproduce every number quoted above.
