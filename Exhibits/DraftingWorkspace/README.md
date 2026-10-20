# 🧩 DraftingWorkspace — evidence for step 9

**TL;DR**: These are real Vulkan frames from SwiftShader Device (Subzero) · Vulkan 1.3.0, driven by real ImGui input. ctest passes 5 / 5. **131 / 131** walkthrough expectations pass, and the parity checklist scores 🟢 48 · ⚠️ 4 · 🚧 6 out of 58 rows (`Experimental/DraftingWorkspace/PARITY-SolidArcBrowserEditor.md`).

| Report | Result |
|---|---|
| `DocumentValidation.md` | W1–W8 🟢 in 2.64 s; worst \|V − closed form\| / V = 6.08e-16; TOML round trip byte-equal |
| `ViewportValidation.md` | V1–V7 🟢; GPU identity = CPU pick on 0 / ≈ 925 000 sampled pixels differing (14 views); selection colours hold; digests colour 98dff767c0c745d2 · identity edc87526f403b936 |
| `Walkthrough/WorkspaceWalkthrough.md` | 78 / 78: selection, bevel preview / apply / refusal, undo / redo, navigation, cube, views + view-aligned grid, face push, parameter edit, construct, save, autosave, New + Restore previous |
| `Walkthrough/ShortcutWalkthrough.md` | 41 / 41: modes 1 / 2, Ctrl+A, signed bevel → chamfer, right-click cancel, numeric pin + viewport-click apply, Ctrl+Shift+Z, Delete, F, E |
| `Walkthrough/RecoveryWalkthrough.md` | 12 / 12: Welcome back → Continue (12 steps, genus 2, sound) → Open `Bracket.drafting.toml` |

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

Every capture also has a `.json` state snapshot. Seven frames (03, 08, 11, 12, 14, 15, 18) are regenerated but not committed; see `.gitignore`. `Viewport/` holds the V-gate colour and identity frames (Isometric, Top, Front, Selection).

Regenerate everything with the commands in `Experimental/DraftingWorkspace/README.md` § "Regenerate the exhibits".
