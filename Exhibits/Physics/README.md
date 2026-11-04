# Physics

Evidence for the Physics extension: `Frontier/Extensions/Physics/` with JoltPhysics 5.6.0 behind it.

## What it is

| Part               | Role                                                                                        |
|--------------------|---------------------------------------------------------------------------------------------|
| `JoltExtension`    | Jolt's registration for the process, the fixed and moving layers, temporary memory, jobs   |
| `PhysicsSpace`     | The world rigid bodies live in: boxes and spheres, gravity, contacts, sleep; no Jolt types |
| `PhysicsExtension` | Level records in, transforms out: the editor's Simulate (Alt+S, the viewport bar's Play)   |

- Jolt is pinned in `Automation/UIDependencies.json` (commit `e77f1755`, the v5.6.0 tag) and downloaded by
  `Automation/PrepareUI.py`.
- It is built through its own `Jolt/Jolt.cmake` (`Automation/JoltPhysics.cmake`) for the Linux host and the
  Windows cross build.
- The build is cross-platform deterministic with an SSE4.2 baseline.

In the editor, Simulate turns the level into a PhysicsSpace:

- A Geometry record with **Physics** on (Inspector › quick controls) becomes a moving box the size of its resolved
  extent.
- Fixed nonfractured geometry uses captured placed facets as triangle colliders, including static groups. If capture is
  unavailable or rejected, a single shown object falls back to bounds; groups can remain held. RigidBody records become spheres.
- A 4 km ground slab tops out at z = 0.
- Each step writes the moving bodies back into their records. Stopping, or saving the level, puts every object back.
- The viewport box button or Alt+C toggles an X-ray overlay of accepted fixed triangle colliders during simulation.
  Boxes, moving shapes, hulls and the ground are omitted; the overlay is capped at 60,000 segments.

[Geometry collision and outline checks](Results/GeometryCollisionValidation.md) exercise the actual ramp and a headless
ImGui interaction capture. The image has a synthetic viewport background; it is not a GPU-rendered scene.

## Test

`PhysicsSpaceValidation` (ctest) covers:

- Free fall against z₀ − g t² / 2.
- A cube coming to rest and sleeping.
- A sphere knocking a box.
- A five-cube stack.
- A 30-body pile settling bit-identically twice.
- The level's Euler convention through quaternions.
- The level bridge: a Geometry record lands on a pedestal record and is put back.
- A 1000-box timing.

The ledger is in `Results/PhysicsSpaceValidation.md`.

## Capture

`Images/PhysicsSpaceFalling.png` is a side view of 13 bodies over 5 s:

- A column of boxes falls onto a pedestal.
- Spheres roll down a ramp.
- A heavy sphere is thrown into the pile.

Each body is drawn at 0, 0.5, 1, 1.5, 2 and 3 s (faint) and at 5 s (solid), with its track.

## Later

- Convex hulls for moving geometry and placed facets for fragmented shapes, in place of the current fallbacks.
- Constraints (hinges, the vehicle's suspension), a character controller, and full Jolt debug rendering.
