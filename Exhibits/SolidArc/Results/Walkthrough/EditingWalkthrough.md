# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 490 frames · 41.8115 s · slowest frame 497.193 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/EditingWalkthrough.txt
- Expectations: 76 / 76 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 31 | `steps == 2` | 2 | 🟢 |
| 32 | `volume ~ 2` | 2 | 🟢 |
| 33 | `bodies == 1` | 1 | 🟢 |
| 40 | `transform == 0` | 0 | 🟢 |
| 41 | `status contains needs picked bodies` | Move needs picked bodies (mode 4, or a face / edge of the body) | 🟢 |
| 48 | `selected-bodies contains Extrude#` | Extrude#2 | 🟢 |
| 53 | `transform == 1` | 1 | 🟢 |
| 58 | `transform-readout contains Move x (1, 0, 0)` | Move x (1, 0, 0) | 🟢 |
| 59 | `transform-preview-volume ~ 2` | 2 | 🟢 |
| 60 | `steps == 2` | 2 | 🟢 |
| 64 | `transform == 0` | 0 | 🟢 |
| 65 | `transform-commits == 1` | 1 | 🟢 |
| 66 | `steps == 3` | 3 | 🟢 |
| 67 | `volume ~ 2` | 2 | 🟢 |
| 68 | `sound == 1` | 1 | 🟢 |
| 69 | `status contains Move` | Move (Extrude1) · 8 ms | 🟢 |
| 78 | `transform-readout contains Rotate z 45` | Rotate z 45° · typed 45 | 🟢 |
| 81 | `transform-commits == 2` | 2 | 🟢 |
| 82 | `steps == 4` | 4 | 🟢 |
| 83 | `volume ~ 2` | 2 | 🟢 |
| 99 | `transform-readout contains Scale z ×2` | Scale z ×2 · typed 2 | 🟢 |
| 102 | `transform-commits == 3` | 3 | 🟢 |
| 103 | `volume ~ 4` | 4 | 🟢 |
| 104 | `sound == 1` | 1 | 🟢 |
| 111 | `transform == 0` | 0 | 🟢 |
| 112 | `steps == 5` | 5 | 🟢 |
| 113 | `volume ~ 4` | 4 | 🟢 |
| 119 | `transform == 1` | 1 | 🟢 |
| 124 | `transform-readout contains Move y (0, 3, 0)` | Move y (0, 3, 0) · typed 3 | 🟢 |
| 127 | `bodies == 2` | 2 | 🟢 |
| 128 | `steps == 6` | 6 | 🟢 |
| 129 | `volume ~ 8` | 8 | 🟢 |
| 130 | `active-body contains Duplicate#` | Duplicate#6 | 🟢 |
| 131 | `selected-bodies contains Duplicate#` | Duplicate#6 | 🟢 |
| 132 | `body-captions contains Copy1` | Extrude1,Extrude1Copy1 | 🟢 |
| 140 | `clipboard == 1` | 1 | 🟢 |
| 141 | `status contains Copied` | Copied Extrude1Copy1 | 🟢 |
| 144 | `bodies == 1` | 1 | 🟢 |
| 147 | `bodies == 2` | 2 | 🟢 |
| 148 | `volume ~ 8` | 8 | 🟢 |
| 149 | `status contains Paste` | Paste (Extrude1Copy1) · 24 ms | 🟢 |
| 150 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 156 | `hidden == 1` | 1 | 🟢 |
| 157 | `scene-bodies == 1` | 1 | 🟢 |
| 158 | `bodies == 2` | 2 | 🟢 |
| 162 | `hidden == 0` | 0 | 🟢 |
| 163 | `scene-bodies == 2` | 2 | 🟢 |
| 170 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 173 | `hidden == 1` | 1 | 🟢 |
| 174 | `scene-bodies == 1` | 1 | 🟢 |
| 177 | `hidden == 1` | 1 | 🟢 |
| 178 | `scene-bodies == 1` | 1 | 🟢 |
| 179 | `volume ~ 8` | 8 | 🟢 |
| 183 | `scene-bodies == 1` | 1 | 🟢 |
| 186 | `hidden == 0` | 0 | 🟢 |
| 187 | `scene-bodies == 2` | 2 | 🟢 |
| 214 | `sketches == 2` | Line1,Line2 | 🟢 |
| 217 | `sketches == 1` | Line2 | 🟢 |
| 218 | `sketch-curves == 4` | 4 | 🟢 |
| 219 | `status contains Join` | Join (Line2, Line1) · 0 ms | 🟢 |
| 230 | `volume ~ 1` | 1 | 🟢 |
| 231 | `bodies == 1` | 1 | 🟢 |
| 238 | `sweep == 1` | 1 | 🟢 |
| 241 | `sweep-amount ~ 0.25` | 0.25 | 🟢 |
| 242 | `sweep-outline == 4` | 4 | 🟢 |
| 246 | `sweep == 0` | 0 | 🟢 |
| 247 | `sweep-commits == 1` | 1 | 🟢 |
| 248 | `sketches == 2` | Line2,Offset1 | 🟢 |
| 249 | `focused-caption == Offset1` | Offset1 | 🟢 |
| 262 | `sweep == 1` | 1 | 🟢 |
| 267 | `sweep-amount ~ 0.2` | 0.2 | 🟢 |
| 275 | `sweep-commits == 2` | 2 | 🟢 |
| 276 | `bodies == 2` | 2 | 🟢 |
| 277 | `body-volumes contains Pipe1 0.251327` | Extrude1 1.000000 · Pipe1 0.251327 | 🟢 |
| 278 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 111MovePreview | 98 | `607e17c0bcfed4b5` |
| 112Rotate | 149 | `1e448cd7f9350b9a` |
| 113Scale | 187 | `eaeb78c2121a9d63` |
| 114Duplicate | 216 | `21d8d71fc5ad66bb` |
| 115Paste | 235 | `97d8cd5cb45f4d1b` |
| 116Hide | 243 | `3a2bcdb2e048822a` |
| 117InvertHidden | 280 | `9948630ee5b32ead` |
| 118Join | 372 | `195e387d6d480eb8` |
| 119OffsetPreview | 414 | `f28d5d98ac4b889d` |
| 120PipePreview | 476 | `81b051597199042f` |
| 121Pipe | 490 | `df67fb9a60bc7eff` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selection cleared
- Move needs picked bodies (mode 4, or a face / edge of the body)
- Selected Extrude1 (6 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Move (Extrude1) · 8 ms
- Rotate 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Rotate (Extrude1) · 7 ms
- View Isometric
- Framed the document (Space)
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Scale (Extrude1) · 11 ms
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Scale cancelled
- Duplicate + Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Duplicate (Extrude1) · 21 ms
- Framed the document (Space)
- Copied Extrude1Copy1
- Remove (Extrude1Copy1) · 7 ms
- Paste (Extrude1Copy1) · 24 ms
- Hide selected · 1 hidden
- Unhide all · 0 hidden
- Selected Extrude1Copy2 (6 faces)
- Hide unselected · 1 hidden
- Invert hidden · 1 hidden
- Undo
- Unhide all · 0 hidden
- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Line1 · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line2 · 0 ms
- Join (Line2, Line1) · 0 ms
- Extrude Line2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Line2 · New · 5 ms
- Offset Line2 · move sideways (Shift fine, Ctrl free) or type a value · click / Enter commits · Esc cancels
- Offset Line2 by 0.25 · 3 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line1 · 3 ms
- Pipe Line1 · move sideways (Shift fine, Ctrl free) or type a value · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Pipe: New body
- View Isometric
- Pipe r 0.2 along Line1 · 14 ms
- Framed the document (Space)
