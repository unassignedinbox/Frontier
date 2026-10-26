# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 500 frames · 45.7275 s · slowest frame 495.686 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/EditingWalkthrough.txt
- Expectations: 77 / 77 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 33 | `steps == 2` | 2 | 🟢 |
| 34 | `volume ~ 2` | 2 | 🟢 |
| 35 | `bodies == 1` | 1 | 🟢 |
| 42 | `transform == 0` | 0 | 🟢 |
| 43 | `status contains needs picked bodies` | Move needs picked bodies (mode 4, or a face / edge of the body) | 🟢 |
| 50 | `selected-bodies contains Extrude#` | Extrude#2 | 🟢 |
| 55 | `transform == 1` | 1 | 🟢 |
| 60 | `transform-readout contains Move x (1, 0, 0)` | Move x (1, 0, 0) | 🟢 |
| 61 | `transform-preview-volume ~ 2` | 2 | 🟢 |
| 62 | `steps == 2` | 2 | 🟢 |
| 66 | `transform == 0` | 0 | 🟢 |
| 67 | `transform-commits == 1` | 1 | 🟢 |
| 68 | `steps == 3` | 3 | 🟢 |
| 69 | `volume ~ 2` | 2 | 🟢 |
| 70 | `sound == 1` | 1 | 🟢 |
| 71 | `status contains Move` | Move (Extrude1) · 5 ms | 🟢 |
| 80 | `transform-readout contains Rotate z 45` | Rotate z 45° · typed 45 | 🟢 |
| 83 | `transform-commits == 2` | 2 | 🟢 |
| 84 | `steps == 4` | 4 | 🟢 |
| 85 | `volume ~ 2` | 2 | 🟢 |
| 101 | `transform-readout contains Scale z ×2` | Scale z ×2 · typed 2 | 🟢 |
| 104 | `transform-commits == 3` | 3 | 🟢 |
| 105 | `volume ~ 4` | 4 | 🟢 |
| 106 | `sound == 1` | 1 | 🟢 |
| 113 | `transform == 0` | 0 | 🟢 |
| 114 | `steps == 5` | 5 | 🟢 |
| 115 | `volume ~ 4` | 4 | 🟢 |
| 121 | `transform == 1` | 1 | 🟢 |
| 126 | `transform-readout contains Move y (0, 3, 0)` | Move y (0, 3, 0) · typed 3 | 🟢 |
| 129 | `bodies == 2` | 2 | 🟢 |
| 130 | `steps == 6` | 6 | 🟢 |
| 131 | `volume ~ 8` | 8 | 🟢 |
| 132 | `active-body contains Duplicate#` | Duplicate#6 | 🟢 |
| 133 | `selected-bodies contains Duplicate#` | Duplicate#6 | 🟢 |
| 134 | `body-captions contains Copy1` | Extrude1,Extrude1Copy1 | 🟢 |
| 142 | `clipboard == 1` | 1 | 🟢 |
| 143 | `status contains Copied` | Copied Extrude1Copy1 | 🟢 |
| 146 | `bodies == 1` | 1 | 🟢 |
| 149 | `bodies == 2` | 2 | 🟢 |
| 150 | `volume ~ 8` | 8 | 🟢 |
| 151 | `status contains Paste` | Paste (Extrude1Copy1) · 18 ms | 🟢 |
| 152 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 158 | `hidden == 1` | 1 | 🟢 |
| 159 | `scene-bodies == 1` | 1 | 🟢 |
| 160 | `bodies == 2` | 2 | 🟢 |
| 164 | `hidden == 0` | 0 | 🟢 |
| 165 | `scene-bodies == 2` | 2 | 🟢 |
| 172 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 175 | `hidden == 1` | 1 | 🟢 |
| 176 | `scene-bodies == 1` | 1 | 🟢 |
| 179 | `hidden == 1` | 1 | 🟢 |
| 180 | `scene-bodies == 1` | 1 | 🟢 |
| 181 | `volume ~ 8` | 8 | 🟢 |
| 185 | `scene-bodies == 1` | 1 | 🟢 |
| 188 | `hidden == 0` | 0 | 🟢 |
| 189 | `scene-bodies == 2` | 2 | 🟢 |
| 216 | `sketches == 2` | Line1,Line2 | 🟢 |
| 219 | `sketches == 1` | Line2 | 🟢 |
| 220 | `sketch-curves == 4` | 4 | 🟢 |
| 221 | `status contains Join` | Join (Line2, Line1) · 0 ms | 🟢 |
| 225 | `sketch-filled == 1` | 1 | 🟢 |
| 235 | `volume ~ 1` | 1 | 🟢 |
| 236 | `bodies == 1` | 1 | 🟢 |
| 243 | `sweep == 1` | 1 | 🟢 |
| 246 | `sweep-amount ~ 0.25` | 0.25 | 🟢 |
| 247 | `sweep-outline == 4` | 4 | 🟢 |
| 251 | `sweep == 0` | 0 | 🟢 |
| 252 | `sweep-commits == 1` | 1 | 🟢 |
| 253 | `sketches == 2` | Line2,Offset1 | 🟢 |
| 254 | `focused-caption == Offset1` | Offset1 | 🟢 |
| 267 | `sweep == 1` | 1 | 🟢 |
| 272 | `sweep-amount ~ 0.2` | 0.2 | 🟢 |
| 280 | `sweep-commits == 2` | 2 | 🟢 |
| 281 | `bodies == 2` | 2 | 🟢 |
| 282 | `body-volumes contains Pipe1 0.251327` | Extrude1 1.000000 · Pipe1 0.251327 | 🟢 |
| 283 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 111MovePreview | 103 | `5e515ca162846808` |
| 112Rotate | 154 | `32a2f96913a35c23` |
| 113Scale | 192 | `3f6bcb7c736af9a6` |
| 114Duplicate | 221 | `66181b635a379880` |
| 115Paste | 240 | `5437972ed551c7e1` |
| 116Hide | 248 | `071e20f0ddcd419c` |
| 117InvertHidden | 285 | `ec08a6c8af2a8eed` |
| 118Join | 377 | `8ff988792c9c3c90` |
| 119OffsetPreview | 424 | `7283a82e190c305b` |
| 120PipePreview | 486 | `05d0d25febbebe0f` |
| 121Pipe | 500 | `3be9f2a0d2972230` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selection cleared
- Move needs picked bodies (mode 4, or a face / edge of the body)
- Selected Extrude1 (6 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Move (Extrude1) · 5 ms
- Rotate 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Rotate (Extrude1) · 3 ms
- View Isometric
- Framed the document (Space)
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Scale (Extrude1) · 4 ms
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Scale cancelled
- Duplicate + Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Duplicate (Extrude1) · 15 ms
- Framed the document (Space)
- Copied Extrude1Copy1
- Remove (Extrude1Copy1) · 4 ms
- Paste (Extrude1Copy1) · 18 ms
- Hide selected · 1 hidden
- Unhide all · 0 hidden
- Selected Extrude1Copy2 (6 faces)
- Hide unselected · 1 hidden
- Invert hidden · 1 hidden
- Undo
- Unhide all · 0 hidden
- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Line1 · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line2 · 0 ms
- Join (Line2, Line1) · 0 ms
- Fill Line2 · 0 ms
- Extrude Line2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Line2 · New · 5 ms
- Offset Line2 · move sideways (Shift fine, Ctrl free) or type a value · click / Enter commits · Esc cancels
- Offset Line2 by 0.25 · 3 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 4 ms
- Pipe Line1 · move sideways (Shift fine, Ctrl free) or type a value · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Pipe: New body
- View Isometric
- Pipe r 0.2 along Line1 · 11 ms
- Framed the document (Space)
