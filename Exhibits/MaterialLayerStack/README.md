# Material Layer Stack — layered procedural materials, as cards

A new exhibit in the Project-Zero card language (same `generic-card`, slider-pill, split-value, toggle,
XYZ `axes`, colour-field and segmented controls as the editor) — **only cards**, no outliner/viewport.
Modelled on Unreal's **Substrate slabs**: a material is a vertical stack of slabs; the camera/light meets
the top card (outermost film) first and the base substrate last.

Open: `index.html` (serve the repo root). Everything is browser-local; stacks autosave to
`localStorage` and export/import as `frontier.layerstack/1` JSON.

## Your example, fleshed out (the default stack)

| | Slab | Carries |
|---|---|---|
| **Layer 3 · Outer coat** | clearcoat film, 35 µm | IOR 1.46 (XYZ), roughness 0.07, transmission 0.97, Fresnel from IOR |
| **Layer 2 · Metallic colour** | metal film, 18 µm | colour + procedural **flakes** (colour B, roughness, bump), metallic 0.9, specular, complex IOR (n/k XYZ), thickness |
| **Layer 1 · Base substrate** | opaque base, 1.5 mm | thickness, normal XYZ, micro-normal XYZ + scale, Fresnel F0/IOR XYZ, roughness, colour |

Every slab actually carries the **full** parameter set below — a layer "type" (Base, Metallic, Outer coat,
Transmission, Subsurface, Sheen, Thin-film, Emissive, Diffuse) is only a starting template:

- **Stacking** — blend `Coat` (physical, over the slab below) / `Mix` (horizontal) / `Add` (films, glow),
  blend weight, coverage α, film **thickness (µm)**.
- **Surface** — base colour, procedural colour B, metallic, roughness, anisotropy, specular intensity.
- **Optics** — **IOR n as XYZ** (per-channel), **extinction k as XYZ** (conductors), F0 auto-from-IOR
  (tinted by metallic) or manual **F0 XYZ**, Schlick Fresnel power.
- **Normals** — **normal XYZ** + strength (with normalize), **micro-normal XYZ** + strength + detail scale.
- **Volume** — **transmission XYZ**, Beer attenuation colour + distance, subsurface amount +
  **scatter-radius XYZ (mm)**, sheen amount/colour/roughness, thin-film amount/thickness (nm)/IOR,
  emission colour + intensity.
- **Procedural** — generated in-shader, no textures: value-noise fBm, Voronoi, flakes, checker, stripes,
  weave. Scale XY, seed, octaves, contrast, invert — and per-channel routing sliders
  (*affects → coverage / colour / roughness / normal / thickness*). Each card shows its pattern.

The **Composite card** shades the whole stack per-pixel (Schlick Fresnel + Beer-Lambert + normalized
specular lobe + sheen/thin-film/emission terms) on a sphere or as a flat Fresnel-sweep swatch, draws the
log-thickness cross-section, and reports total film thickness, stack transmittance, an estimated shader
cost grade, and warnings for the classic mistakes (transmissive slab buried under an opaque one,
metallic + transmission, buried emissive, F0 out of range…).

Presets: your example, automotive flake paint, frosted tinted glass, carbon-fibre twill, velvet,
wax/marble subsurface, iridescent film, brushed steel + oxide.

## Files

```
index.html               shell (titlebar + two card columns)
MaterialLayerStack.css   the editor's card vocabulary, copied from ProjectZeroEditor/Editor.css
MaterialLayerStack.js    UI — renders state, writes it back (no material math here)
MaterialStackModel.js    pure model: slabs, IOR/Fresnel/Beer, procedural samplers, composite shading,
                         cost/warnings, presets, JSON import/export  (no DOM)
CheckMaterialStack.mjs   node CheckMaterialStack.mjs   → optics identities, pattern ranges,
                         normalization clamps, finite shading for every preset, round-trip
```

Fonts are the repo's DM Sans (`../EngineContent/Fonts/SunReference/`).
Sibling note: keep this folder beside `EngineContent/` (font path is relative) — it otherwise stands alone.

## Honest limits

The composite preview is an **approximation for comparing stacks**, not a renderer: single fixed light,
Schlick + Blinn-style lobe instead of GGX/microfacet transport, no inter-slab multiple scattering or
refraction bending, colours treated as display-referred. The exported JSON is the source of truth —
an engine implementation should evaluate the same slab fields with real BSDFs (the native counterparts
live upstream at `Engine/Editor/*Surface.h`, reference copies in `../NativeSurfaces/`).
