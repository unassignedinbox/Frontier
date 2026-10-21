# 🧩 DraftingWorkspace — evidence for step 9

**TL;DR**: These are real Vulkan frames from SwiftShader Device (Subzero) · Vulkan 1.3.0, driven by real ImGui input. ctest passes 7 / 7. **253 / 253** walkthrough expectations pass, and the parity checklist scores 🟢 53 · ⚠️ 1 · 🚧 4 out of 58 rows (`Experimental/DraftingWorkspace/PARITY-SolidArcBrowserEditor.md`).

| Report | Result |
|---|---|
| `DocumentValidation.md` | W1–W10 🟢; W10: drawn sketches (every tool and curve form) → extrude / Boolean match closed forms and exact profile areas to 4.45e-16, refusals leave the document untouched, reload is byte-equal, deleting a sketch removes its extrude; worst \|V − closed form\| / V = 6.08e-16; TOML round trip byte-equal; W9: variable fillet 0.1 → 0.3 (unflipped and flipped) keeps each size at its corner through a Width edit and a reload |
| `ViewportValidation.md` | V1–V7 🟢; GPU identity = CPU pick on 0 / ≈ 925 000 sampled pixels differing (14 views); selection colours hold; digests colour 98dff767c0c745d2 · identity edc87526f403b936 |
| `Walkthrough/WorkspaceWalkthrough.md` | 78 / 78: selection, bevel preview / apply / refusal, undo / redo, navigation, cube, views + view-aligned grid, face push, parameter edit, construct, save, autosave, New + Restore previous |
| `Walkthrough/ShortcutWalkthrough.md` | 68 / 68: modes 1 / 2, Ctrl+A, signed bevel → chamfer, right-click cancel, numeric pin + viewport-click apply, Ctrl+Shift+Z, Delete, F, E |
| `Walkthrough/RecoveryWalkthrough.md` | 12 / 12: Welcome back → Continue (12 steps, genus 2, sound) → Open `Bracket.drafting.toml` |
| `Walkthrough/SketchDrawingWalkthrough.md` | 64 / 64: Ctrl+N, Top view; Shift+Q rectangle → E (New) 1.5 → V = 3; Shift+W circle → E · W → V = 3 − 0.24π; Shift+A triangle closed by clicking the first point → E · E · Q → + 1/3; Shift+S curve, Ctrl+Z, forms 2 / 3 / 4 / 1, closed Hermite → E · W pocket 0.5 → − 0.123; collinear arc refused, Esc; all volumes to 10⁻⁶ |
| `Walkthrough/VariableBlendWalkthrough.md` | 31 / 31: B, numeric 0.3, V (end size starts at 0.15, typed 0.1), Front view, F flips the anchor, C variable chamfer, D, Enter (one undo entry, caption `0.3 → 0.1`), Ctrl+Z / Ctrl+Shift+Z; preview and applied volumes equal the closed form to 10⁻⁶ |

## Key frames (`Walkthrough/`)

| Frame | Shows |
|---|---|
| `01Workspace` | The full shell: header 48 px, document bar, Outliner 300 px, viewport with bar, pills, cube and hint, Inspector 330 px |
| `02FacesSelected` | Ctrl-click multiselection (two faces, #4099ff) |
| `04BevelPreview` | B fillet pinned at `300mm`: staged 12-face preview while the document keeps 11 faces |
| `05FilletApplied` | Enter applies: 12 faces, 9 steps, sound |
| `06BevelRefusal` | Shift+B on an edge meeting the fillet: the kernel's refusal shown, Apply disabled |
| `07Navigated` | After RMB orbit, Shift+RMB and middle pan, wheel dolly |
| `09FrontParallel` · `09RightParallel` | Parallel views with the grid on XZ (X red, Z blue) and on YZ (Y green, Z blue) |
| `10FaceEdit` | Mode 3 face selected; Face edit card |
| `13Constructed` | Through-hole cylinder (Difference): 12 steps, genus 2 |
| `16WelcomeBack` · `17Recovered` | Recovery modal, then the continued document |
| `19BevelSignChamfer` | Pointer moved left past zero: the session became a chamfer (0.8514 m, refused as wider than the edge) |
| `20ShortcutExtrude` | E on the boss top: V 10.2228 → 10.6155 m³ |
| `22VariableFillet` | V in the fillet command, Front view: the contact line on the front face runs from 0.3 m at the left corner down to 0.1 m at the right; preview 10.1856 m³ (closed form 10.185607) |
| `23VariableFlipped` | F: the same fillet mirrored, 0.3 m now at the right corner; same volume, pill `0.3 → 0.1 (flipped)` |
| `24VariableChamfer` | C on the flipped anchor: a flat ruled band 0.3 → 0.1; preview 10.1361 m³ (closed form 10.136107) |
| `25VariableApplied` | Enter: 9 steps, 12 faces, sound; history caption `Fillet 0.3 → 0.1 on 1 edge(s)` |
| `30RectanglePreview` | Shift+Q after one click at the origin: live corner rectangle to the pointer at (2, 1), Draw card with Corner / Centre |
| `31CurveForms` | Curve with 4 points in cubic-spline form (key 4); Inspector form toggles Hermite (1) · Bezier (2) · NURBS (3) · Cubic spline (4) |
| `32HermiteClosing` | Pointer on the first point closes the Hermite; box with the through hole and the triangle boss behind it |
| `33DrawnSolid` | Isometric: the drawn solid (genus 1, sound, V = 2.4564) with the four used sketches dimmed on z = 0 |

Every capture also has a `.json` state snapshot. Seven frames (03, 08, 11, 12, 14, 15, 18) are regenerated but not committed; see `.gitignore`. `Viewport/` holds the V-gate colour and identity frames (Isometric, Top, Front, Selection).

Regenerate everything with the commands in `Experimental/DraftingWorkspace/README.md` § "Regenerate the exhibits".
