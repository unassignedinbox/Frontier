# Incremental replay — E1–E4

Each regeneration saves the replay state after every step (partition copy + bodies, sketches, curves, panels); the next
regeneration restores the longest prefix whose step digests (fields + bound parameter values; SpaceCurves also the
Mirror planes) are unchanged and whose Duplicate snapshots are all taken, and replays from the first changed step.

| Gate | Subject | Limit | Worst | Checks | Result |
|---|---|---|---|---|---|
| E1 | Randomized edits (parameter, literal, delete, truncate, restore) on the 9 catalogue trees and a 34-step studded plate: the incremental replay equals a full replay on a fresh partition — step reports, body keys / tokens, persistent names, solved sketches, fitted curves, record stores (slot · generation lists), vertex positions and body volumes bit for bit, tessellation of the active body bit for bit | 0 differing bytes | 0 | 138 | 🟢 |
| E2 | Resume point: an edit of step k (literal / delete) resumes at k, a parameter edit at the first step binding it, truncation and an unchanged tree restore every step, a re-added Duplicate resumes at its source step (the snapshot is taken there), a Mirror change at the first SpaceCurve | exact index | 0 | 10 | 🟢 |
| E3 | Studded plate (34 steps): replay time after an edit of the final fillet / a middle boss vs the full replay; recording overhead of the checkpoints | tail ≥ 3× faster · middle ≥ 0.85× · overhead ≤ 40 % | 4.45 | 5 | 🟢 |
| E4 | Capture runs (authoring): steps last replayed without capture are replayed again; the captured signatures and the result equal a full capture replay | 0 differing bytes | 0 | 10 | 🟢 |

## Details

| Gate | Case | Detail | Result |
|---|---|---|---|
| E1 | BlockFillet · edit 1 | delete Blend#3 · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 2 | restore the authored tree · resumed at 2/3 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 3 | truncate to 1 steps · resumed at 1/1 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 4 | parameter Radius = 0.0771 · resumed at 1/1 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 5 | parameter Radius = 0.1530 · resumed at 1/1 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 6 | restore the authored tree · resumed at 1/3 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 7 | parameter Height = 0.5513 · resumed at 1/3 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 8 | restore the authored tree · resumed at 1/3 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 9 | restore the authored tree · resumed at 3/3 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 10 | parameter Width = 2.9543 · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 11 | parameter Radius = 0.0867 · resumed at 2/3 · 0 differing bytes | 🟢 |
| E1 | BlockFillet · edit 12 | restore the authored tree · resumed at 0/3 · 0 differing bytes | 🟢 |
| E2 | BlockFillet | 12/12 resume points exact · 15 steps restored instead of replayed | 🟢 |
| E4 | BlockFillet | capture run resumed at 3/3 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | BlockVariableFillet · edit 1 | parameter StartRadius = 0.2412 · resumed at 2/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 2 | parameter Height = -1.1613 · resumed at 1/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 3 | restore the authored tree · resumed at 1/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 4 | restore the authored tree · resumed at 3/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 5 | truncate to 1 steps · resumed at 1/1 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 6 | parameter Width = 3.7283 · resumed at 0/1 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 7 | restore the authored tree · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 8 | restore the authored tree · resumed at 3/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 9 | restore the authored tree · resumed at 3/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 10 | truncate to 1 steps · resumed at 1/1 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 11 | restore the authored tree · resumed at 1/3 · 0 differing bytes | 🟢 |
| E1 | BlockVariableFillet · edit 12 | restore the authored tree · resumed at 3/3 · 0 differing bytes | 🟢 |
| E2 | BlockVariableFillet | 12/12 resume points exact · 19 steps restored instead of replayed | 🟢 |
| E4 | BlockVariableFillet | capture run resumed at 3/3 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | BossFillet · edit 1 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 2 | parameter BossHeight = 1.1306 · resumed at 3/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 3 | restore the authored tree · resumed at 3/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 4 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 5 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 6 | truncate to 2 steps · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 7 | no change · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 8 | restore the authored tree · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 9 | parameter BossX = 1.8195 · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 10 | delete Extrude#4 · resumed at 3/4 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 11 | restore the authored tree · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | BossFillet · edit 12 | truncate to 1 steps · resumed at 1/1 · 0 differing bytes | 🟢 |
| E2 | BossFillet | 12/12 resume points exact · 35 steps restored instead of replayed | 🟢 |
| E4 | BossFillet | capture run resumed at 1/1 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | SteppedShaft · edit 1 | parameter MainLength = 1.9777 · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 2 | restore the authored tree · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 3 | parameter PinRadius = 0.3822 · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 4 | parameter PinRadius = 0.4215 · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 5 | delete Blend#3 · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 6 | parameter MainRadius = 0.9908 · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 7 | restore the authored tree · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 8 | delete Revolve#2 · resumed at 1/2 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 9 | parameter MainRadius = 0.9106 · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 10 | no change · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 11 | parameter MainLength = 2.0468 · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | SteppedShaft · edit 12 | restore the authored tree · resumed at 0/3 · 0 differing bytes | 🟢 |
| E2 | SteppedShaft | 12/12 resume points exact · 5 steps restored instead of replayed | 🟢 |
| E4 | SteppedShaft | capture run resumed at 3/3 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | SlotPocket · edit 1 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 2 | no change · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 3 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 4 | parameter SlotRadius = 0.4542 · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 5 | restore the authored tree · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 6 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 7 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 8 | restore the authored tree · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 9 | delete FaceOperation#5 · resumed at 4/4 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 10 | parameter SlotX = 0.9214 · resumed at 2/4 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 11 | parameter Depth = 2.5516 · resumed at 0/4 · 0 differing bytes | 🟢 |
| E1 | SlotPocket · edit 12 | restore the authored tree · resumed at 0/5 · 0 differing bytes | 🟢 |
| E2 | SlotPocket | 12/12 resume points exact · 40 steps restored instead of replayed | 🟢 |
| E4 | SlotPocket | capture run resumed at 5/5 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | TorusRing · edit 1 | restore the authored tree · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 2 | parameter Major = 2.9191 · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 3 | restore the authored tree · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 4 | parameter Turn = 3.8178 · resumed at 1/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 5 | parameter Minor = 0.3206 · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 6 | no change · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 7 | restore the authored tree · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 8 | restore the authored tree · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 9 | parameter Major = 2.1437 · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 10 | no change · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 11 | restore the authored tree · resumed at 0/2 · 0 differing bytes | 🟢 |
| E1 | TorusRing · edit 12 | restore the authored tree · resumed at 2/2 · 0 differing bytes | 🟢 |
| E2 | TorusRing | 12/12 resume points exact · 11 steps restored instead of replayed | 🟢 |
| E4 | TorusRing | capture run resumed at 2/2 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | HoleLeavesPlate · edit 1 | restore the authored tree · resumed at 6/6 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 2 | truncate to 5 steps · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 3 | truncate to 4 steps · resumed at 4/4 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 4 | parameter HoleY = 0.8529 · resumed at 2/4 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 5 | restore the authored tree · resumed at 2/6 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 6 | restore the authored tree · resumed at 6/6 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 7 | parameter HoleY = 1.1476 · resumed at 2/6 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 8 | delete Sketch#3 · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 9 | truncate to 3 steps · resumed at 3/3 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 10 | restore the authored tree · resumed at 2/6 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 11 | restore the authored tree · resumed at 6/6 · 0 differing bytes | 🟢 |
| E1 | HoleLeavesPlate · edit 12 | delete Extrude#4 · resumed at 3/5 · 0 differing bytes | 🟢 |
| E2 | HoleLeavesPlate | 12/12 resume points exact · 43 steps restored instead of replayed | 🟢 |
| E4 | HoleLeavesPlate | capture run resumed at 5/5 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | SplitFacePush · edit 1 | no change · resumed at 5/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 2 | parameter GrooveX = 3.4566 · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 3 | restore the authored tree · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 4 | parameter GrooveX = 0.9548 · resumed at 2/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 5 | parameter Height = 0.7890 · resumed at 1/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 6 | truncate to 4 steps · resumed at 4/4 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 7 | parameter Height = 1.2784 · resumed at 1/4 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 8 | parameter Push = -0.1706 · resumed at 4/4 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 9 | restore the authored tree · resumed at 1/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 10 | parameter Depth = 2.6244 · resumed at 0/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 11 | parameter Height = 0.6940 · resumed at 1/5 · 0 differing bytes | 🟢 |
| E1 | SplitFacePush · edit 12 | no change · resumed at 5/5 · 0 differing bytes | 🟢 |
| E2 | SplitFacePush | 12/12 resume points exact · 28 steps restored instead of replayed | 🟢 |
| E4 | SplitFacePush | capture run resumed at 5/5 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | SplineArch · edit 1 | no change · resumed at 4/4 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 2 | parameter Span = 4.6371 · resumed at 0/4 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 3 | no change · resumed at 4/4 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 4 | parameter Shoulder = 2.7409 · resumed at 0/4 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 5 | parameter Crown = 3.7765 · resumed at 0/4 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 6 | parameter Shoulder = 1.3577 · resumed at 0/4 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 7 | literal of Extrude#4 × 0.9869 · resumed at 3/4 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 8 | delete Extrude#2 · resumed at 1/3 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 9 | parameter Crown = 3.7521 · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 10 | parameter Span = 3.9593 · resumed at 0/3 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 11 | no change · resumed at 3/3 · 0 differing bytes | 🟢 |
| E1 | SplineArch · edit 12 | parameter Crown = 3.7798 · resumed at 0/3 · 0 differing bytes | 🟢 |
| E2 | SplineArch | 12/12 resume points exact · 15 steps restored instead of replayed | 🟢 |
| E4 | SplineArch | capture run resumed at 3/3 · 0 differing bytes (result + signatures) | 🟢 |
| E1 | StuddedPlate · edit 1 | parameter Post = 0.3980 · resumed at 26/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 2 | restore the authored tree · resumed at 26/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 3 | literal of Extrude#4 × 1.0050 · resumed at 3/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 4 | parameter Hole3 = 0.2716 · resumed at 24/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 5 | literal of Extrude#16 × 1.0124 · resumed at 15/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 6 | parameter Boss2 = 0.3478 · resumed at 6/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 7 | restore the authored tree · resumed at 3/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 8 | truncate to 22 steps · resumed at 22/22 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 9 | parameter Boss1 = 0.2211 · resumed at 4/22 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 10 | literal of Extrude#6 × 0.9419 · resumed at 5/22 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 11 | no change · resumed at 22/22 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 12 | truncate to 11 steps · resumed at 11/11 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 13 | parameter Hole2 = 0.1028 · resumed at 11/11 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 14 | literal of Extrude#8 × 0.9347 · resumed at 7/11 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 15 | literal of Extrude#4 × 0.9273 · resumed at 3/11 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 16 | delete Sketch#11 · resumed at 10/10 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 17 | parameter Hole1 = 0.2379 · resumed at 10/10 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 18 | no change · resumed at 10/10 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 19 | truncate to 2 steps · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 20 | parameter Boss0 = 0.2902 · resumed at 2/2 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 21 | restore the authored tree · resumed at 2/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 22 | no change · resumed at 34/34 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 23 | delete Extrude#14 · resumed at 13/33 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 24 | delete Extrude#20 · resumed at 18/32 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 25 | parameter Hole3 = 0.2548 · resumed at 22/32 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 26 | truncate to 18 steps · resumed at 18/18 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 27 | literal of Extrude#6 × 1.0247 · resumed at 5/18 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 28 | delete Extrude#12 · resumed at 11/17 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 29 | parameter Boss5 = 0.1613 · resumed at 11/17 · 0 differing bytes | 🟢 |
| E1 | StuddedPlate · edit 30 | restore the authored tree · resumed at 5/34 · 0 differing bytes | 🟢 |
| E2 | StuddedPlate | 30/30 resume points exact · 361 steps restored instead of replayed | 🟢 |
| E4 | StuddedPlate | capture run resumed at 34/34 · 0 differing bytes (result + signatures) | 🟢 |
| E3 | full replay | 34 steps · 1794.53 ms (median of 5) | 🟢 |
| E3 | recording | full replay saving a checkpoint per step · 1788.10 ms · overhead -0.4 % | 🟢 |
| E3 | tail edit (fillet radius) | resumed at 33/34 · 403.03 ms · 4.5× faster | 🟢 |
| E3 | middle edit (Boss4 radius) | resumed at 10/34 · 1609.97 ms · 1.1× faster | 🟢 |
| E3 | unchanged tree | resumed at 34/34 · 0.44 ms · 4048× faster | 🟢 |

ALL GATES PASSED
