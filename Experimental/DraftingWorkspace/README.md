# 🧩 DraftingWorkspace — Vulkan + Slang + ImGui CAD workspace on the SolidArcRemastered kernel

**TL;DR (step 9)**: The workspace runs the SolidArcRemastered feature tree behind an ImGui shell that mirrors the SolidArc browser editor. It uses Vulkan 1.3 (volk), SPIR-V compiled from Slang, a GLFW host without a client API, and TOML options. ctest passes **21 / 21** on SwiftShader (22 / 22 where the BMW picture is present). **Object snapping (Plasticity point picker):** only while sketching and dragging — drawing, dragging a sketch point in K, dragging a pole in mode 1, and moving with G — the pointer snaps to endpoints, midpoints (half the arc length), centres of circles / arcs / planar faces, curve crossings, tangent and perpendicular points from the last point, and onto curves, edges and (Move) faces — every snap point lies exactly on the kernel carrier, hidden points snap only in x-ray, the section clip, the moved bodies and the dragged sketch / curve itself are skipped. **n / b / t / x / y / z** lock a direction through the last point (the same key releases; directions leaving the drawing plane are refused), **s** squares a rectangle, **Ctrl** held is free. A glyph and caption mark the snap at the pointer; the Inspector **Snaps** card switches kinds on and off. **Measure (I) and Section (Alt+S)** follow Fusion Inspect (Plasticity has neither). **I** opens the Measure card: with nothing selected it shows the shown bodies' mass properties (volume, area, centroid, principal moments); a face gives area and perimeter, an edge its length, two picks the exact minimum distance with its closest points (drawn in the viewport) and the angle, and more picks give totals. **Alt+S** cuts the view by a plane through X / Y / Z or a picked planar face, with a typed offset, − / + and F to flip. The GPU clips faces and edges and hatches the caps where solid material is cut; picking, hover and box selection follow the clip. The Section card reports the exact kernel cut: area, perimeter, centroid, second moments, regions / loops / bodies. **X-ray (Alt+Z, Plasticity viewport:toggle-x-ray):** faces turn to 45 % ghosts over the grid and image planes, every edge's hidden part is drawn as a pale stroke that stays pickable — hover, click, B and box selection reach through the solid (box selection now takes B-rep edges too: the visible ones, or all in x-ray). **Incremental replay:** every regeneration resumes at the first changed step; undo, redo, a refused command's rollback and preview staging (bevel, move, sweep) restore every step from shared checkpoints (≈ 1 ms instead of 1.9 s on a 27-step part), and the Inspector's Body card reads `replayed k/n` (the Bevel card too, for its preview). **Revolve / Evolution:** **Alt+R** turns the focused sketch (else the newest closed one) about an axis — its first construction line, else the sketch y-axis when the profile lies beside it, else the x-axis; X / Y pick the sketch axes and a click on one of its lines makes that the axis (dashed amber); degrees are typed (360 by default). Spline profiles revolve exactly. **Shift+P / Alt+E** (Plasticity evolution) sweeps a closed sketch along a path carried by rotation-minimising frames: a focused lone open curve, the selected curve, else the newest one-curve sketch; a click on another sketch's curve switches it. Both stage a preview and take Q / W / E / R. **Sketch constraints (K):** K opens a constraint session on the focused sketch: click points and curves, then SolveSpace's verbs (D dimension · N angle · H / V · O on · Q equal · M midpoint · Y symmetric · L parallel / tangent · [ perpendicular · ] lock). Glyphs and dimensions are drawn on the sketch, coloured by solver status (green fixed, blue free, red redundant / conflicting), refusals name the constraints they clash with, a dragged point takes the rest of the sketch with it (live preview, one undo entry), and a clicked glyph can be deleted or retyped. The Inspector **Constraints** card lists them with the degrees of freedom left. Splines take constraints on the curve itself: O point on it, H / V with an end point (end direction), L tangent (G1) and C curvature (G2, replacing the G1) where a curve meets an end. **Shift+D** adds a reference dimension (display-only, grey in parentheses) and **E** toggles a picked dimension between driving and reference. Earlier sketches draw dashed and are pickable: a constraint on their geometry links it in (amber, held at its projection, following them on regeneration), and Delete on a linked point unlinks it. **Trim / split and editing keys:** C cuts bodies and panels by a sketch or curve, T trims sketch curves, G / R / S · Shift+D · Ctrl+C / V · H family · J / O / P edit bodies and sketches. **Multi-body:** documents hold several bodies; Construct / extrude **R** adds a new body, mode 4 picks whole bodies, **Q** combines them, and each body wears its own matcap. Sketch drawing (Phase B): Line, Rectangle, Circle, Arc, Slot and Curve (Hermite · Bezier · NURBS · cubic spline) on the active plane, then E extrudes the drawn profile with the Plasticity Boolean keys. Variable fillets and chamfers run inside the B command: V makes the size variable (Plasticity `gizmo:fillet-solid:add`), F flips which corner takes the start size, and the anchor is stored as a persistent vertex name. **Vehicle surfacing (plan V1–V4):** **Import curves…** fits Blender limit points as SpaceCurve steps (≤ 0.2 mm). Mode 1 shows the poles and lets you drag them. **L** lofts the picked sections into an open NURBS sheet that regenerates after every pole edit (`Experimental/VehicleSurfacing/README.md`). **V5–V7:** crossing curves loft into a Gordon network panel, the G0 / G1 / G2 buttons make the next loft continuous with its neighbour, **Alt+X** mirrors the panels through x = 0 (sewn, G1 seam), and **Close → solid** caps planar rims. **V8:** the **Zebra** toggle draws reflection stripes on a dense tessellation and **Deviation** draws coloured needles from every imported point to the panels. **Blueprint study:** `[[ImagePlanes]]` show reference drawings behind the curves (Slang textured quads, toolbar **Images**, Inspector card). A left drag on empty viewport space box-selects curves in station order, and Construct cylinders take **Axis x / y / z**. A BMW M3 traced from an internet three-view lofts, mirrors and closes into a sound solid with four wheel-arch cuts.
- **Document gates W1–W35 🟢:** W35 object snapping (block + cylinder + coplanar sketches under an isometric camera: endpoint, midpoint, face / rim / sketch-circle centres, sketch crossing, on edge / face / rim / cylinder side, tangent ||T − C| − r| + |(T − P)·(T − C)|, perpendicular foot, Hermite midpoint |L₁ − L₂| / L 1.4e-14, hidden corner only in x-ray, section / exclusion / filter, a dragged sketch excluded by its label, lock line; 22 checks, worst 5.6e-16), W34 section clip (on a 4 × 2 × 1 block cut at x = 1, rays on the removed side miss, a ray through the opening meets the cap at the back face (distance 5, not the plane's 4), a kept face in front of the plane crossing still wins, removed edges are skipped, sheet faces show their back, the x-ray box takes the 8 edges reaching the kept side), W33 measure + section (block V 8 / A 28 / principal moments 10/3; distances 1 at 180°, 0 at 90°, √5 between skew edges; sections x = 1.5 → A 2, P 6; two bodies → A 12 by the parallel-axis theorem; hidden bodies skipped; refusals; worst 3.8e-12), W32 x-ray (on a 4 × 2 × 1 block the ray at the hidden back-bottom edge picks the top face opaque and the edge in x-ray, √(1.25² + 1) = 1.6008 farther; a visible edge picks the same either way; the isometric full box takes the 9 edges off the far corner opaque and all 12 in x-ray; an empty box none), W31 incremental replay (a 27-step studded plate: 14 commands each identical to a full replay with the cache off and resumed at the new step; tail edit 2.8×, undo / redo ≈ 1 ms vs 1.89 s, preview clone 0.5 ms; the preview's capture re-captures Fillet1 in place; 8 of 25 history entries keep checkpoints and 10 undos stay identical; bonnet pole move resumed at its curve), W29 revolve (annulus 3π about the proposed y-axis; a plate about its own edge line united → 4.625π, undo / redo; x-axis fallback quarter turn 3π/2, all ≤ 2e-16; refusals; the point axis is written only without a line), W30 evolution (rectangle along a line sketch = 1; circle along a quarter arc = π²/16 to 5.3e-9, starting from the arc end nearer the profile; a square groove through a block = 1.875; refusals; codec Kind 16 + Path); W26 reference dimensions (a driven diagonal reads √13 then 5 with DOF, positions and volume bit-equal; E while implied refused, E at the reading and back; codec `Reference = true`), W27 spline joints (line + NURBS joined: L → T1, C → K1 replacing it, H1 on the far end, every solve ≤ 1e-12; a natural cubic refuses C), W28 links (a later line linked to a plate's corner and top line follows a width edit to (4, 2); backward links, linked drags and bad indices refused; codec `[[Steps.Sketch.Links]]`; unlink keeps the shown position); W23–W25 check constraints (a drawn rectangle goes DOF 4 → 2 → 1 → 0 with exact volumes, D1 = 4 → V 8; implied / conflicting / bad edits refused by name; drag Line2.End → (4, 3) → V 12, fixed point refused, codec byte-equal); W17–W22 trim, split and the editing keys; W15 matcaps and W16 multi-body below. construct, pick, fillet / chamfer, push and parameter edit all match closed forms to ≤ 6.1e-16. Refusals are atomic, undo / redo is exact, and TOML save → load → save is byte-equal. W9 checks variable blends; W10 checks drawn sketches → extrude / Boolean against closed forms and exact profile areas (worst 4.45e-16, 20 checks). W11 checks the vehicle path: `VehicleCurves.toml` round trip bit-equal, 5 refusals, import → loft → a sheet within 0.076 mm of all 286 limit points and 0.053 mm of the analytic bonnet, reload byte-equal, pole move + undo bit-exact (18 checks). W12 checks the panels: loft + loft G2 → Alt+X gives 4 panels in one sound sheet; Close on that rim is refused; the pod goes loft → mirror → Close to a solid within 4.9e-4 of ∫πab dy; 10 crossing curves give a network 0.062 mm from H; every reload is byte-equal (11 checks). W13 checks evaluation: the deviation map on the bonnet gives 286 needles (worst 0.076 mm), each a true closest point (residual 5.5e-8); the zebra phase jumps 1.07 stripes across a G0 crease and 1.8e-13 across G2 (6 checks). W14 checks the blueprint study: the BMW side-view landmarks map to the car frame within 1e-5 m and the other views within 1.5 px; a BMP decodes pixel-exact; a missing picture refuses; documents with planes save → load → save byte-equal; degenerate planes refuse the whole file; x- and y-axis cylinders match πr²h (11 checks).
- **Multi-body (W16 🟢, 10 checks):** each body is keyed by the step that created it (`Extrude#6`) and captioned by it (Block2). R / New body (Construct, extrude) adds a separate body. Union / Difference / Intersection act on the first body picked in mode 4, else the active one. Blends and face edits act on the body that owns their names, and a selection spanning bodies is refused. **Q** opens the Boolean card (Q / W / E, Enter): one `Combine` step per tool, and the tools are consumed. Deleting or undoing the step brings them back with their matcaps. Separate bodies keep exact volumes (8 · π/2 · 1 to 1.3e-15), Block1 − Cylinder1 = 8 − π/4, and an intersection keeps half a filleted cube (1.1e-15). The Outliner lists the bodies (caption · matcap · pick order), and the Body card gives per-body figures. Files gain an optional `Target` / `Tool` per step and `[[Appearance.Bodies]]`; older files load unchanged.
- **Matcaps (W15 · W16 · V8 🟢):** 13 engineering matcaps composed in a 6-softbox HDR studio (Chrome, Brushed steel, Satin aluminium, Gunmetal, Brass, Copper; Satin / Gloss black / Red plastic; Rubber, Clay, Ceramic, Blue paint) plus user pictures (centre crop → 256²), assigned per face, per body (of several) or as the document default. Resolution order: face → body → default. Assignments are appearance-only: no regeneration, one undo entry each, kept through parameter edits by persistent face name, saved as `[Appearance]`. On the GPU every pixel of a matcap face is within 0.51 / 255 of the texel at its view-space normal; clearing the layers restores the Studio frame bit-exact.
- **Viewport gates V1–V10 🟢:** the GPU identity buffer equals the CPU ray pick (0 mismatched pixels over 14 views). 100 % of edge pixels are explained, hover probe = readback, selection colours hold, all 7 view presets are checked, and digests are deterministic. **V9 x-ray:** ghost faces = 0.45 · opaque + 0.55 · background within 0.74 / 255 (≈ 95 000 pixels, orthographic and perspective); 1708 / 1708 revealed stroke pixels are CPU-hidden edges and no other identity changes; rim spill 1.3 % (visible ↔ hidden transitions, after the 8 px world margin); the probe at the hidden back-bottom edge returns it on the GPU and in the x-ray CPU pick (the opaque pick returns the top face); box visibility = the edges the GPU drew; x-ray off restores the frame bit-exact. **V10 section:** for y = 1.25 and x = 1, orthographic and perspective, 0 of ≈ 264 000 interior pixels differ from the sectioned CPU pick (≈ 9500 of them on the cap); every one of ≈ 99 000 cap pixels has a CPU cap within 1 px; the probe on a cap returns it; switching the grid on changes no cap pixel; section off restores the frame bit-exact.
- **Walkthroughs:** **1012 / 1012** expectations pass (main 78, shortcuts 70, recovery 12, variable blend 31, sketch drawing 64, vehicle surfacing 53, panel assembly 57, surface evaluation 28, blueprint 48, matcaps 30, multi-body 32, trim / split 54, editing 76, constraints 75, linked constraints 55, revolve / sweep 47, incremental replay 29, x-ray 36, measure / section 32, snapping 62, NURBS blends 43), driven by real ImGui pointer and keyboard events on the offscreen Vulkan frame.
- **Keymap: Plasticity** (`nkallen/plasticity` default keymap):
  - middle-drag orbits, right-drag pans; click replaces, Shift adds, Ctrl removes, Alt adds the tangent chain;
  - 1 control points (curves, poles, pole drag) and 2 / 3 / 4 / 5 (Tab) selection modes; L loft through the picked curves (crossing curves → network), Alt+X mirror; B fillet (D / C inside), E extrude, Space frames;
  - numpad views, M material picker and Alt+M remove material (faces, else picked bodies, else the default), Q Boolean on the picked bodies, Shift+C / V / X / Z construct, Shift+R repeat, Alt+A / Esc deselect, X / Delete delete;
  - Shift+A line, Shift+Q corner rectangle, Shift+W centre circle, Shift+S curve (1–4 form, Ctrl+Z drops a point, Alt / V modes); E on a focused sketch with Q / W / E / R union / difference / intersect / new body;
  - C cut, T trim, G / R / S, Shift+D, Ctrl+C / V, H / Shift+H / Alt+H / Ctrl+H, J join, O offset, P pipe; K constrain (a free Plasticity key; the verbs inside it are SolveSpace's, plus C for G2, Shift+D reference and E toggle).
  - Keys for tools this modeller lacks announce that, so no non-Plasticity shortcut remains.
- **Parity gate:** `PARITY-SolidArcBrowserEditor.md` has 73 rows: 🟢 69 · ⚠️ 1 · 🚧 3 (sketch-corner bevel, workplane offset, multi-document), plus the Plasticity keymap table.
- **View-aligned grid:** Top / Bottom → XY, Front / Back → XZ, Right / Left → YZ, with X red, Y green, Z blue. Isometric keeps the active plane.

## Layout

| Path | Contents |
|---|---|
| `Source/Document/` | `DraftingDocument` (commands → `DraftingCommandOutcome`, undo / redo, selection by persistent name), `DraftingScene` (tessellated faces / edges with identities), `DraftingDocumentCodec` (TOML), `DraftingMatcap` (built-in studio matcaps, picture fitting, layer library), `DraftingSpecimen` (the Bracket) |
| `Source/Viewport/` | `DraftingDevice` (Vulkan 1.3 through volk, system loader or an explicit ICD), `DraftingRasterization` (surface, edge and grid pipelines; colour + R32 identity + depth), `DraftingCamera` (upright orbit, pan, dolly, presets, fit), `DraftingPortableImage` (PNG, digests) |
| `Source/Viewport/Generated/` | `DraftingShaderArchive.h` (SPIR-V words) + reflection JSON, generated from `Shaders/*.slang` |
| `Source/Interface/` | `DraftingInterface` (header, Outliner, viewport, Inspector, bevel session, shortcuts, autosave / recovery), `DraftingFrame` (ImGui → Vulkan), `DraftingOptions` (toml++), `DraftingScript` (walkthrough language) |
| `Source/Applications/` | `DraftingWorkspaceHost` (GLFW window + swapchain), `DraftingWorkspaceCapture` (offscreen frames driven by a script) |
| `Validation/` | `DraftingDocumentValidation` (W1–W35), `DraftingViewportValidation` (V1–V10) |
| `Scripts/` | `WorkspaceWalkthrough.txt`, `ShortcutWalkthrough.txt`, `VariableBlendWalkthrough.txt`, `SketchDrawingWalkthrough.txt`, `VehicleSurfacingWalkthrough.txt`, `PanelAssemblyWalkthrough.txt`, `SurfaceEvaluationWalkthrough.txt`, `MatcapWalkthrough.txt`, `MultiBodyWalkthrough.txt`, `TrimSplitWalkthrough.txt`, `EditingWalkthrough.txt`, `ConstraintsWalkthrough.txt`, `LinkedConstraintsWalkthrough.txt`, `RevolveSweepWalkthrough.txt`, `IncrementalReplayWalkthrough.txt`, `XRayWalkthrough.txt`, `MeasureSectionWalkthrough.txt`, `SnappingWalkthrough.txt`, `NurbsBlendWalkthrough.txt`, `RecoveryWalkthrough.txt` + `RecoveryOptions.toml` |
| `Tooling/` | `PrepareDraftingDependencies.py` (pinned, sha256-checked sources), `CompileDraftingShaders.mjs` (Slang → SPIR-V archive) |
| `DraftingWorkspace.toml` | Every option with its default |

## Build and test (Linux, from the repository root)

```sh
# ① pinned third-party sources → ExternalPackages/ (imgui, tomlpp, glfw, vulkan-headers, stb, volk)
python3 Experimental/DraftingWorkspace/Tooling/PrepareDraftingDependencies.py

# ② a Vulkan driver for headless runs: SwiftShader (google/swiftshader @ cddb5c91b23ff70a1f607ea31610aa0d7f4b5ad5)
git clone https://github.com/google/swiftshader.git _ClaudeScratch/build/swiftshader
git -C _ClaudeScratch/build/swiftshader checkout cddb5c91b23ff70a1f607ea31610aa0d7f4b5ad5
cmake -S _ClaudeScratch/build/swiftshader -B _ClaudeScratch/build/swiftshader/out -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DREACTOR_BACKEND=Subzero -DSWIFTSHADER_BUILD_TESTS=OFF -DSWIFTSHADER_BUILD_BENCHMARKS=OFF -DSWIFTSHADER_BUILD_PVR=OFF \
      -DSWIFTSHADER_BUILD_WSI_XCB=OFF -DSWIFTSHADER_BUILD_WSI_WAYLAND=OFF -DSWIFTSHADER_WARNINGS_AS_ERRORS=OFF
ninja -C _ClaudeScratch/build/swiftshader/out vk_swiftshader
mkdir -p _ClaudeScratch/SwiftShaderDriver && cp _ClaudeScratch/build/swiftshader/out/Linux/libvk_swiftshader.so _ClaudeScratch/SwiftShaderDriver/

# ③ configure + build (the windowed host is compile coverage only on a headless Linux GLFW null platform)
cmake -S Experimental/DraftingWorkspace -B Experimental/DraftingWorkspace/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DDRAFTING_WINDOWED_HOST=OFF
ninja -C Experimental/DraftingWorkspace/build

# ④ the 11 tests (12 with the BMW picture) (walkthroughs run from the repository root; without a Vulkan driver the GPU tests skip with code 77)
cd Experimental/DraftingWorkspace/build && \
  DRAFTING_VULKAN_DRIVER=$PWD/../../../_ClaudeScratch/SwiftShaderDriver/libvk_swiftshader.so ctest --output-on-failure
```

Windows (MSVC, the target platform): run ① and then `cmake -S Experimental/DraftingWorkspace -B Experimental/DraftingWorkspace/build -G "Visual Studio 17 2022"`. The host then opens a real window and uses the system Vulkan loader. CMake sets `/utf-8` for MSVC. ⚠️ This sandbox has no MSVC, so the Windows build itself is unverified here.

## Regenerate the exhibits

```sh
B=Experimental/DraftingWorkspace/build; D=_ClaudeScratch/SwiftShaderDriver/libvk_swiftshader.so; E=Exhibits/DraftingWorkspace
mkdir -p $E/Viewport $E/Walkthrough
$B/DraftingDocumentValidation > $E/DocumentValidation.md
$B/DraftingViewportValidation --vulkan-driver $D --exhibits $E/Viewport > $E/ViewportValidation.md
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/WorkspaceWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --options Experimental/DraftingWorkspace/Scripts/RecoveryOptions.toml \
    --script Experimental/DraftingWorkspace/Scripts/RecoveryWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/ShortcutRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Shortcut.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/ShortcutWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/VariableRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Variable.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/VariableBlendWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/SketchRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Sketch.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/SketchDrawingWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/VehicleRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Vehicle.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/VehicleSurfacingWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/PanelRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Panels.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/PanelAssemblyWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/EvaluationRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Evaluation.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/SurfaceEvaluationWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/MatcapRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Matcap.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/MatcapWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/MultiBodyRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/MultiBody.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/MultiBodyWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/TrimSplitRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/TrimSplit.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/TrimSplitWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/EditingRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Editing.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/EditingWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/ConstraintsRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Constraints.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/ConstraintsWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/LinkedConstraintsRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/LinkedConstraints.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/LinkedConstraintsWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
# NURBS blends (N5): about 12 minutes on SwiftShader, every blend on the spline rim takes 40–110 s
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/NurbsBlendRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/NurbsBlend.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/NurbsBlendWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
# Needs the (uncommitted) blueprint at _ClaudeScratch/Blueprint/BmwM3E90.jpg — see Experimental/VehicleSurfacing/README.md
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/BlueprintRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Blueprint.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/BlueprintModellingWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
```

The recovery run consumes the autosave the main walkthrough leaves behind, so keep that order. `DRAFTING_VULKAN_DRIVER` can replace `--vulkan-driver`.

## Regenerate the shader archive (after editing `Shaders/*.slang`)

```sh
npm install --prefix _ClaudeScratch/tmp/SlangCompiler --ignore-scripts slang-loader@0.0.7
node Experimental/DraftingWorkspace/Tooling/CompileDraftingShaders.mjs \
     --compiler _ClaudeScratch/tmp/SlangCompiler/node_modules/slang-loader/vendor/slang-wasm.js
```

Alternatively, configure with `-DDRAFTING_SLANG_COMPILER=<…>/slang-wasm.js` and the build regenerates the archive whenever a `.slang` file changes. Without it, the committed archive is used as is.

## Capture tool

| Flag | Meaning |
|---|---|
| `--vulkan-driver PATH` | ICD library to dlopen (else `DRAFTING_VULKAN_DRIVER`, else the system loader) |
| `--options PATH` | TOML options (see `DraftingWorkspace.toml`) |
| `--script PATH` / `--exhibits DIR` | Walkthrough to run, and where to write `NAME.png` + `NAME.json` per `capture` plus the `<Script>.md` report |
| `--specimen` · `--open PATH` | Start from the Bracket specimen, or open a document |
| `--recovery PATH` · `--document PATH` · `--reset-files` | Autosave and Ctrl+S paths; delete both (and the previous copy) first |
| `--curves PATH` | Default path of **Import curves…** (also on `DraftingWorkspaceHost`; options key `Files.CurvesPath`) |
| `--width` · `--height` | Frame size (default 1600 × 900) |

Exit codes: 0 all expectations pass · 1 an expectation failed · 2 bad arguments or script · 77 no Vulkan device.

## Walkthrough language (one micro-step per frame; whole-line `#` comments)

| Command | Effect |
|---|---|
| `frames N` · `move T` · `move offset DX DY` (from the last move target, e.g. mid-drag) · `click T [Left\|Right\|Middle] [Ctrl] [Shift]` · `press` / `release` | Pointer input |
| `drag BUTTON T by DX DY STEPS [mods]` · `wheel T STEPS` | Drags and wheel |
| `key NAME [Ctrl] [Shift]` · `type TEXT` | Keyboard |
| `capture NAME` | Write the frame and a JSON state snapshot |
| `expect FIELD OP VALUE` | OP one of `== != < > ~ contains lacks`; fields such as `faces`, `steps`, `volume`, `selected-edges`, `bevel-chamfer`, `bevel-combination`, `drawing`, `drawing-points`, `drawing-form`, `sketches`, `sketch-curves`, `focused-caption`, `curves`, `selected-curves`, `sheet`, `pole-drags`, `pole-drag-active`, `pole-edits`, `curve-fit-worst`, `camera-yaw`, `grid-plane`, `welcome`, `file:PATH` |

Targets: `@Widget` (a registered widget's centre, e.g. `@BevelSize`, `@Step:Blend#9`), `world X Y Z` (projected), `viewport U V`, or `X Y` pixels, each with an optional `offset DX DY`.

## Honest limits

- The captures come from a software Vulkan driver (SwiftShader, Subzero). They prove the rendering, picking and interface paths. They are **not** evidence of physical-GPU performance or of desktop window input.
- The GLFW host compiles and links on Linux (`-DDRAFTING_WINDOWED_HOST=ON`). On the headless null platform it exits 77 with a message. It has not been run in a real window here.
- Not built yet: sketch-corner bevel, polygon tool, a workplane offset, multi-document state and docking (see 🚧 rows in the parity checklist).
- Drawing happens on the three base planes (XY / XZ / YZ through the origin), fixed when the command opens. Drawn sketches carry no dimensions yet (positions snap to 0.1; Ctrl places freely), and the sketch overlay is drawn on top of the body (no depth test).
- Vehicle surfacing covers V1–V8; trim is not built. Zebra stripes come from interpolated exact vertex normals on a dense tessellation (not per-pixel evaluation) and follow the camera up axis. G1 / G2 and the G1 mirror seam hold for lofts only, not networks. The Blender add-on has not been run in Blender. Curve overlays are drawn without a depth test. Image planes are not undo entries; box selection picks curves only (not faces / edges); a Boolean on freeform faces replays the whole tree (about 12 s per wheel arch on the BMW, 🚧 incremental replay).
- ⚠️ Known artefact: dotted depth fighting where the boss-top circle edge is seen exactly edge-on in Front parallel (capture 09FrontParallel).
