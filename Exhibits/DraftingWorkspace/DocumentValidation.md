# DraftingDocumentValidation

Document layer of DraftingWorkspace over the SolidArcRemastered feature tree (2.64 s).

| Gate | Subject | Limit | Worst | Checks | Verdict |
|------|---------|-------|-------|--------|---------|
| W1 | Construct tiles (block, cylinder, sphere, torus, union, difference): |V − closed form| / V, sound audit | ≤ 10⁻⁹ | 4.14e-16 | 5 | 🟢 |
| W2 | CPU ray pick: face under the ray / edge within reach resolves to the expected persistent name | all equal | 0 | 4 | 🟢 |
| W3 | Fillet / chamfer on picked edges: |V − closed form| / V | ≤ 10⁻⁹ | 4.17e-16 | 2 | 🟢 |
| W4 | Face edit on a picked face (push): |V − closed form| / V | ≤ 10⁻⁹ | 5.49e-16 | 1 | 🟢 |
| W5 | Parameter edit regenerates downstream steps; selection / references survive by name | ≤ 10⁻⁹, names kept | 6.08e-16 | 3 | 🟢 |
| W6 | Refused command (oversize fillet, empty selection) leaves tree, body and history untouched | all equal | 0 | 2 | 🟢 |
| W7 | Undo / redo restore exact volumes; delete removes a whole tile | ≤ 10⁻¹² | 6.08e-16 | 4 | 🟢 |
| W8 | TOML save → load → save is byte-equal; reloaded body volume and names equal | equal | 0 | 4 | 🟢 |

## Walkthrough

| Gate | Check | Value | Verdict |
|------|-------|-------|---------|
| W1 | Sphere r 0.8 at (0.3, −0.2, 1) (V = 2.144661) | 4.14e-16 | 🟢 |
| W1 | Torus R 1.5 r 0.4 at (1, 2, 0.5) (V = 4.737410) | 3.75e-16 | 🟢 |
| W1 | Block 3 × 2 × 1 − through cylinder r 0.4 (V = 5.497345) | 1.62e-16 | 🟢 |
| W1 | Block1 4 × 2 × 1 (V = 8.000000) | 2.22e-16 | 🟢 |
| W1 | + Cylinder1 boss r 0.5 h 0.75 (union on the top face) (V = 8.589049) | 4.14e-16 | 🟢 |
| W2 | ray ↓ at (3, 1) → Extrude#2.Cap(End) | 0 | 🟢 |
| W2 | ray at the x = 4 top edge → Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line2)) | 0 | 🟢 |
| W2 | ray away from the body → no pick | 0 | 🟢 |
| W3 | Fillet1 r 0.25 on the picked edge (V = 8.562223) | 4.15e-16 | 🟢 |
| W3 | Chamfer1 0.2 on the x = 0 top edge (V = 8.522223) | 4.17e-16 | 🟢 |
| W2 | ray → front face → Extrude#2.Side(Line1) | 0 | 🟢 |
| W4 | Push1 0.3 on the picked front face (V = 9.712200) | 5.49e-16 | 🟢 |
| W5 | Block1.Depth = 2.5 → fillet, chamfer, push regenerate (V = 11.695493) | 6.08e-16 | 🟢 |
| W5 | no step refused after the edit | 0 | 🟢 |
| W5 | selection still names Extrude#2.Side(Line1) | 0 | 🟢 |
| W6 | fillet r 5 on Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3)) refused: Blend#8: Blend: vertex V57: blend ends on a curved face (🚧 cap on non-planar faces) | 0 | 🟢 |
| W6 | push with no faces refused: select one or more faces first | 0 | 🟢 |
| W7 | undo Block1.Depth → exact earlier volume | 0 | 🟢 |
| W7 | redo → edited volume | 6.08e-16 | 🟢 |
| W7 | delete Cylinder1 (sketch + extrude) → volume without the boss (V = 11.106445) | 4.8e-16 | 🟢 |
| W7 | both steps and every Cylinder1.* parameter removed | 0 | 🟢 |
| W8 | reloaded volume equal (5548 bytes of TOML) | 0 | 🟢 |
| W8 | reloaded persistent names equal | 0 | 🟢 |
| W8 | save → load → save byte-equal | 0 | 🟢 |
| W8 | malformed file refused: a step has no label | 0 | 🟢 |

## Saved document (TOML, first 40 lines)

```toml
# DraftingWorkspace document — feature tree of SolidArcRemastered (regenerated on load)
Description = ''
Format = 'DraftingWorkspace'
FormatVersion = 1
Title = 'Bracket'

[[Parameters]]
Description = 'corner x'
High = 5.0
Low = -5.0
Name = 'Block1.X'
Value = 0.0

[[Parameters]]
Description = 'corner y'
High = 5.0
Low = -5.0
Name = 'Block1.Y'
Value = 0.0

[[Parameters]]
Description = 'corner z (sketch elevation)'
High = 5.0
Low = -5.0
Name = 'Block1.Z'
Value = 0.0

[[Parameters]]
Description = 'extent along x'
High = 10.0
Low = 0.10000000000000001
Name = 'Block1.Width'
Value = 4.0

[[Parameters]]
Description = 'extent along y'
High = 10.0
Low = 0.10000000000000001
Name = 'Block1.Depth'
Value = 2.5
```
