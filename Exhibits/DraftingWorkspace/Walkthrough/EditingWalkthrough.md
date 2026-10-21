# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 476 frames · 47.7404 s · slowest frame 413.65 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/EditingWalkthrough.txt
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
| 69 | `status contains Move` | Move (Extrude1) · 9 ms | 🟢 |
| 78 | `transform-readout contains Rotate z 45` | Rotate z 45° · typed 45 | 🟢 |
| 81 | `transform-commits == 2` | 2 | 🟢 |
| 82 | `steps == 4` | 4 | 🟢 |
| 83 | `volume ~ 2` | 2 | 🟢 |
| 97 | `transform-readout contains Scale z ×2` | Scale z ×2 · typed 2 | 🟢 |
| 100 | `transform-commits == 3` | 3 | 🟢 |
| 101 | `volume ~ 4` | 4 | 🟢 |
| 102 | `sound == 1` | 1 | 🟢 |
| 109 | `transform == 0` | 0 | 🟢 |
| 110 | `steps == 5` | 5 | 🟢 |
| 111 | `volume ~ 4` | 4 | 🟢 |
| 117 | `transform == 1` | 1 | 🟢 |
| 122 | `transform-readout contains Move y (0, 3, 0)` | Move y (0, 3, 0) · typed 3 | 🟢 |
| 125 | `bodies == 2` | 2 | 🟢 |
| 126 | `steps == 6` | 6 | 🟢 |
| 127 | `volume ~ 8` | 8 | 🟢 |
| 128 | `active-body contains Duplicate#` | Duplicate#6 | 🟢 |
| 129 | `selected-bodies contains Duplicate#` | Duplicate#6 | 🟢 |
| 130 | `body-captions contains Copy1` | Extrude1,Extrude1Copy1 | 🟢 |
| 138 | `clipboard == 1` | 1 | 🟢 |
| 139 | `status contains Copied` | Copied Extrude1Copy1 | 🟢 |
| 142 | `bodies == 1` | 1 | 🟢 |
| 145 | `bodies == 2` | 2 | 🟢 |
| 146 | `volume ~ 8` | 8 | 🟢 |
| 147 | `status contains Paste` | Paste (Extrude1Copy1) · 29 ms | 🟢 |
| 148 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 154 | `hidden == 1` | 1 | 🟢 |
| 155 | `scene-bodies == 1` | 1 | 🟢 |
| 156 | `bodies == 2` | 2 | 🟢 |
| 160 | `hidden == 0` | 0 | 🟢 |
| 161 | `scene-bodies == 2` | 2 | 🟢 |
| 168 | `selected-bodies contains Duplicate#` | Duplicate#8 | 🟢 |
| 171 | `hidden == 1` | 1 | 🟢 |
| 172 | `scene-bodies == 1` | 1 | 🟢 |
| 175 | `hidden == 1` | 1 | 🟢 |
| 176 | `scene-bodies == 1` | 1 | 🟢 |
| 177 | `volume ~ 8` | 8 | 🟢 |
| 181 | `scene-bodies == 1` | 1 | 🟢 |
| 184 | `hidden == 0` | 0 | 🟢 |
| 185 | `scene-bodies == 2` | 2 | 🟢 |
| 212 | `sketches == 2` | Line1,Line2 | 🟢 |
| 215 | `sketches == 1` | Line2 | 🟢 |
| 216 | `sketch-curves == 4` | 4 | 🟢 |
| 217 | `status contains Join` | Join (Line2, Line1) · 0 ms | 🟢 |
| 228 | `volume ~ 1` | 1 | 🟢 |
| 229 | `bodies == 1` | 1 | 🟢 |
| 236 | `sweep == 1` | 1 | 🟢 |
| 239 | `sweep-amount ~ 0.25` | 0.25 | 🟢 |
| 240 | `sweep-outline == 4` | 4 | 🟢 |
| 244 | `sweep == 0` | 0 | 🟢 |
| 245 | `sweep-commits == 1` | 1 | 🟢 |
| 246 | `sketches == 2` | Line2,Offset1 | 🟢 |
| 247 | `focused-caption == Offset1` | Offset1 | 🟢 |
| 260 | `sweep == 1` | 1 | 🟢 |
| 265 | `sweep-amount ~ 0.2` | 0.2 | 🟢 |
| 271 | `sweep-commits == 2` | 2 | 🟢 |
| 272 | `bodies == 2` | 2 | 🟢 |
| 273 | `body-volumes contains Pipe1 0.251327` | Extrude1 1.000000 · Pipe1 0.251327 | 🟢 |
| 274 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 111MovePreview | 98 | `52bcaf5a166c1fed` |
| 112Rotate | 142 | `382e0e8ab29b9ff9` |
| 113Scale | 180 | `3cfca7a307a84ec9` |
| 114Duplicate | 209 | `cd0e9f5e9ae483a7` |
| 115Paste | 228 | `7b132efe982f285d` |
| 116Hide | 236 | `1114b1a2491e866b` |
| 117InvertHidden | 273 | `f5ae985ec6cb0130` |
| 118Join | 365 | `2c40da38fd2fa749` |
| 119OffsetPreview | 407 | `5c079377b2922250` |
| 120PipePreview | 462 | `59938b9e1c6e3ad2` |
| 121Pipe | 476 | `e1d56fa82f49c475` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 6 ms
- Selection cleared
- Move needs picked bodies (mode 4, or a face / edge of the body)
- Selected Extrude1 (6 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/DraftingWorkspace/EditingRecovery.drafting.toml
- Move (Extrude1) · 9 ms
- Rotate 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Rotate (Extrude1) · 12 ms
- View Isometric
- Framed the document (Space)
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Scale (Extrude1) · 16 ms
- Scale 1 body(ies) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Scale cancelled
- Duplicate + Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/DraftingWorkspace/EditingRecovery.drafting.toml
- Duplicate (Extrude1) · 31 ms
- Framed the document (Space)
- Copied Extrude1Copy1
- Remove (Extrude1Copy1) · 21 ms
- Paste (Extrude1Copy1) · 29 ms
- Hide selected · 1 hidden
- Unhide all · 0 hidden
- Selected Extrude1Copy2 (6 faces)
- Hide unselected · 1 hidden
- Invert hidden · 1 hidden
- Undo
- Unhide all · 0 hidden
- New document (previous recovery copy kept)
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Autosaved _ClaudeScratch/DraftingWorkspace/EditingRecovery.drafting.toml
- Line1 · 0 ms
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line2 · 0 ms
- Join (Line2, Line1) · 0 ms
- Extrude Line2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Line2 · New · 4 ms
- Offset Line2 · move sideways (Shift fine, Ctrl free) or type a value · click / Enter commits · Esc cancels
- Offset Line2 by 0.25 · 5 ms
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line1 · 4 ms
- Pipe Line1 · move sideways (Shift fine, Ctrl free) or type a value · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Pipe: New body
- View Isometric
- Pipe r 0.2 along Line1 · 11 ms
- Framed the document (Space)
