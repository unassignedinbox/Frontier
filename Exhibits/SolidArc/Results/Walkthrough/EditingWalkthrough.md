# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 501 frames · 33.3651 s · slowest frame 339.651 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/EditingWalkthrough.txt
- Expectations: 77 / 77 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 36 | `steps == 2` | 2 | 🟢 |
| 37 | `volume ~ 2` | 2 | 🟢 |
| 38 | `bodies == 1` | 1 | 🟢 |
| 45 | `transform == 0` | 0 | 🟢 |
| 46 | `status contains needs picked bodies` | Move needs picked bodies (mode 4, or a face / edge of the body) or picked sketches | 🟢 |
| 53 | `selected-bodies contains Extrude#` | Extrude#2 | 🟢 |
| 58 | `transform == 1` | 1 | 🟢 |
| 63 | `transform-readout contains Move x (1, 0, 0)` | Move x (1, 0, 0) | 🟢 |
| 64 | `transform-preview-volume ~ 2` | 2 | 🟢 |
| 65 | `steps == 2` | 2 | 🟢 |
| 69 | `transform == 0` | 0 | 🟢 |
| 70 | `transform-commits == 1` | 1 | 🟢 |
| 71 | `steps == 3` | 3 | 🟢 |
| 72 | `volume ~ 2` | 2 | 🟢 |
| 73 | `sound == 1` | 1 | 🟢 |
| 74 | `status contains Move` | Move (Extrude1) · 0 ms | 🟢 |
| 83 | `transform-readout contains Rotate z 45` | Rotate z 45° · typed 45 | 🟢 |
| 86 | `transform-commits == 2` | 2 | 🟢 |
| 87 | `steps == 4` | 4 | 🟢 |
| 88 | `volume ~ 2` | 2 | 🟢 |
| 104 | `transform-readout contains Scale z ×2` | Scale z ×2 · typed 2 | 🟢 |
| 107 | `transform-commits == 3` | 3 | 🟢 |
| 108 | `volume ~ 4` | 4 | 🟢 |
| 109 | `sound == 1` | 1 | 🟢 |
| 116 | `transform == 0` | 0 | 🟢 |
| 117 | `steps == 5` | 5 | 🟢 |
| 118 | `volume ~ 4` | 4 | 🟢 |
| 124 | `transform == 1` | 1 | 🟢 |
| 129 | `transform-readout contains Move y (0, 3, 0)` | Move y (0, 3, 0) · typed 3 | 🟢 |
| 132 | `bodies == 2` | 2 | 🟢 |
| 133 | `steps == 6` | 6 | 🟢 |
| 134 | `volume ~ 8` | 8 | 🟢 |
| 135 | `active-body contains Duplicate#` | Duplicate#6 | 🟢 |
| 136 | `selected-bodies contains Duplicate#` | Duplicate#6 | 🟢 |
| 137 | `body-captions contains Copy1` | Extrude1,Extrude1Copy1 | 🟢 |
| 145 | `clipboard == 1` | 1 | 🟢 |
| 146 | `status contains Copied` | Copied Extrude1Copy1 | 🟢 |
| 149 | `bodies == 1` | 1 | 🟢 |
| 152 | `bodies == 2` | 2 | 🟢 |
| 153 | `volume ~ 8` | 8 | 🟢 |
| 154 | `status contains Paste` | Paste (Extrude1Copy1) · 1 ms | 🟢 |
| 155 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 161 | `hidden == 1` | 1 | 🟢 |
| 162 | `scene-bodies == 1` | 1 | 🟢 |
| 163 | `bodies == 2` | 2 | 🟢 |
| 167 | `hidden == 0` | 0 | 🟢 |
| 168 | `scene-bodies == 2` | 2 | 🟢 |
| 175 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 178 | `hidden == 1` | 1 | 🟢 |
| 179 | `scene-bodies == 1` | 1 | 🟢 |
| 182 | `hidden == 1` | 1 | 🟢 |
| 183 | `scene-bodies == 1` | 1 | 🟢 |
| 184 | `volume ~ 8` | 8 | 🟢 |
| 188 | `scene-bodies == 1` | 1 | 🟢 |
| 191 | `hidden == 0` | 0 | 🟢 |
| 192 | `scene-bodies == 2` | 2 | 🟢 |
| 219 | `sketches == 2` | Line1,Line2 | 🟢 |
| 222 | `sketches == 1` | Line2 | 🟢 |
| 223 | `sketch-curves == 4` | 4 | 🟢 |
| 224 | `status contains Join` | Join (Line2, Line1) · 0 ms | 🟢 |
| 228 | `sketch-filled == 1` | 1 | 🟢 |
| 238 | `volume ~ 1` | 1 | 🟢 |
| 239 | `bodies == 1` | 1 | 🟢 |
| 246 | `sweep == 1` | 1 | 🟢 |
| 249 | `sweep-amount ~ 0.25` | 0.25 | 🟢 |
| 250 | `sweep-outline == 4` | 4 | 🟢 |
| 254 | `sweep == 0` | 0 | 🟢 |
| 255 | `sweep-commits == 1` | 1 | 🟢 |
| 256 | `sketches == 2` | Line2,Offset1 | 🟢 |
| 257 | `focused-caption == Offset1` | Offset1 | 🟢 |
| 270 | `sweep == 1` | 1 | 🟢 |
| 275 | `sweep-amount ~ 0.2` | 0.2 | 🟢 |
| 283 | `sweep-commits == 2` | 2 | 🟢 |
| 284 | `bodies == 2` | 2 | 🟢 |
| 285 | `body-volumes contains Pipe1 0.251327` | Extrude1 1.000000 · Pipe1 0.251327 | 🟢 |
| 286 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 111MovePreview | 104 | `d620e721f809d58f` |
| 112Rotate | 155 | `9c6212eb31f6e45c` |
| 113Scale | 193 | `45bee0674e98148a` |
| 114Duplicate | 222 | `285862220a77f83a` |
| 115Paste | 241 | `fd0bfcab5596c9c7` |
| 116Hide | 249 | `1d78d9fb41e5d3c2` |
| 117InvertHidden | 286 | `f2e67d3abbb7d99e` |
| 118Join | 378 | `1615bf0a90b4ade6` |
| 119OffsetPreview | 425 | `89573135af934cba` |
| 120PipePreview | 487 | `38ff75198e93c70b` |
| 121Pipe | 501 | `5791346277a39cfd` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 0 ms
- Selection cleared
- Move needs picked bodies (mode 4, or a face / edge of the body) or picked sketches
- Selected Extrude1 (6 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Move (Extrude1) · 0 ms
- Rotate 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Rotate (Extrude1) · 0 ms
- View Isometric
- Framed the document (Space)
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Scale (Extrude1) · 0 ms
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Scale cancelled
- Duplicate + Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits (at once: a copy in place) · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/EditingRecovery.solidarc
- Duplicate (Extrude1) · 1 ms
- Framed the document (Space)
- Copied Extrude1Copy1
- Remove (Extrude1Copy1) · 0 ms
- Paste (Extrude1Copy1) · 1 ms
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
- Extrude Line2 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Line2 · New · 0 ms
- Offset Line2 · move sideways (Shift fine, Ctrl free) or type a value · click / Enter commits · Esc cancels
- Offset Line2 by 0.25 · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Pipe Line1 · move sideways (Shift fine, Ctrl free) or type a value · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Pipe: New body
- View Isometric
- Pipe r 0.2 along Line1 · 0 ms
- Framed the document (Space)
