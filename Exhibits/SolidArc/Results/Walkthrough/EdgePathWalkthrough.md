# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 172 frames · 10.8653 s · slowest frame 338.275 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/EdgePathWalkthrough.txt
- Expectations: 36 / 36 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 36 | `steps == 2` | 2 | 🟢 |
| 37 | `volume ~ 2` | 2 | 🟢 |
| 46 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 51 | `path-picks == 1` | 1 | 🟢 |
| 52 | `path-refused == 0` | 0 | 🟢 |
| 53 | `path-edges == 3` | 3 | 🟢 |
| 54 | `path-length > 4.999` | 5 | 🟢 |
| 55 | `path-length < 5.001` | 5 | 🟢 |
| 56 | `selected-edges == 3` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 57 | `status contains Along loop` | Edge path Along loop: 3 edge(s) along rim of Extrude#2.Cap(End), 5 long (near side · O... | 🟢 |
| 63 | `path-edges == 3` | 3 | 🟢 |
| 64 | `selected-edges == 3` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 65 | `status contains far side` | Edge path Along loop: 3 edge(s) along rim of Extrude#2.Cap(End), 5 long (far side) · I... | 🟢 |
| 68 | `path-mode == 1` | 1 | 🟢 |
| 69 | `path-edges == 4` | 4 | 🟢 |
| 70 | `path-length > 5.999` | 6 | 🟢 |
| 71 | `path-length < 6.001` | 6 | 🟢 |
| 72 | `selected-edges == 4` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 76 | `path-mode == 2` | 2 | 🟢 |
| 77 | `path-edges == 3` | 3 | 🟢 |
| 78 | `selected-edges == 3` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 81 | `path-mode == 0` | 0 | 🟢 |
| 89 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 96 | `path-picks == 2` | 2 | 🟢 |
| 97 | `path-refused == 1` | 1 | 🟢 |
| 98 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 99 | `status contains share no edge loop` | Edge path (Along loop): Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) and Edge(Extrude... | 🟢 |
| 103 | `path-refused == 0` | 0 | 🟢 |
| 104 | `path-edges == 3` | 3 | 🟢 |
| 105 | `path-length > 3.999` | 4 | 🟢 |
| 106 | `path-length < 4.001` | 4 | 🟢 |
| 107 | `selected-edges == 3` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.Sid... | 🟢 |
| 113 | `selected-edges == 2` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.Sid... | 🟢 |
| 114 | `path-picks == 2` | 2 | 🟢 |
| 117 | `path-mode == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 321EdgePathAlong | 89 | `d9c5834fcd020aab` |
| 322EdgePathWhole | 105 | `1ecbefbf95b038be` |
| 323EdgePathRefused | 148 | `7f78a301169fa8f5` |
| 324EdgePathShortest | 157 | `d5eee3b9483d9d09` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 0 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Edge path Along loop: 3 edge(s) along rim of Extrude#2.Cap(End), 5 long (near side · Other side flips) · Inspector › Edge path: Along loop · Whole loop · Shortest
- Edge path Along loop: 3 edge(s) along rim of Extrude#2.Cap(End), 5 long (far side) · Inspector › Edge path: Along loop · Whole loop · Shortest
- Autosaved _ClaudeScratch/SolidArcWorkspace/EdgePathRecovery.solidarc
- Edge path Whole loop: 4 edge(s) along rim of Extrude#2.Cap(End), 6 long · Inspector › Edge path: Along loop · Whole loop · Shortest
- Edge path Shortest: 3 edge(s) along any edges, 5 long · Inspector › Edge path: Along loop · Whole loop · Shortest
- Edge path Along loop: 3 edge(s) along rim of Extrude#2.Cap(End), 5 long (near side · Other side flips) · Inspector › Edge path: Along loop · Whole loop · Shortest
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Edge path (Along loop): Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1)) and Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line2)) share no edge loop, face rim or tangent chain (Inspector › Edge path › Shortest takes any edges)
- Edge path Shortest: 3 edge(s) along any edges, 4 long · Inspector › Edge path: Along loop · Whole loop · Shortest
- Removed Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line2))
- Edge path: Along loop (Ctrl+click an edge with an edge picked)
