# Naming ledger

Every name this redesign proposes, the engine term it replaces, and its approval state.

**Status key:** 🚩 proposed, awaiting the user · 🟢 approved by the user · 🔴 rejected by the user

Nothing marked 🚩 may appear in `Experimental/`. It may appear inside a standalone prototype while the
prototype is being shown, because that is what the user is being asked to approve.

---

## Cards

| Proposed | Replaces | Reads as | Status |
| -------------------- | ------------------------------------ | ---------------------------------------------------------- | ------ |
| Placement Frame | Transform / Details panel | the frame a record is placed inside | 🚩 |
| Lattice Reach | Static Mesh / Geometry statistics | how far the lattice of faces reaches and how it packs | 🚩 |
| Heft | Mass / weight property | the load a record carries at rest | 🚩 |
| ~~Grain~~ | triangle density / cluster heatmap | cut from card 2 — too much for one card | 🔴 dropped by this pass |
| Coupling | component attachments / socket list | what fastens the record to the world | 🚩 |
| Slot | placement target / attach point | a named place on the world the frame claims | 🚩 |
| Grip | collision response / mobility | how firmly the record is held in place | 🚩 |
| Reach | bounds / extents | footprint and height, in one reading | 🚩 |

## Placement Frame sections

| Proposed | Replaces | Reads as | Status |
| -------------- | ------------------------------- | --------------------------------------------------------------- | ------ |
| Anchor | Location / position | where the frame sits on the ground plane | 🚩 |
| Turn | Rotation | which way the record faces, as three named rings | 🚩 |
| Spread | Scale | how far the record stretches along each axis | 🚩 |
| Size spread | — (new) | how many faces land in each piece, as a swept range | 🚩 |

## Turn rings

| Proposed | Replaces | Status |
| ------------- | ------------------------ | ------ |
| Yaw | Z rotation | 🚩 |
| Lean | pitch / Y rotation | 🚩 |
| Cant | roll / X rotation | 🚩 |

## Lattice Reach readings

| Proposed | Replaces | Status |
| ------------------ | ---------------------------------------- | ------ |
| Budget used | triangle budget / draw cost | 🚩 |
| Corners | vertices | 🚩 |
| Faces | triangles | 🚩 |
| Pieces | submeshes / sections | 🚩 |
| Packed size | memory footprint | 🚩 |
| Reweld below | merge threshold / LOD cut | 🚩 |

## Grip levels

| Proposed | Replaces | Status |
| ------------ | ------------------------------- | ------ |
| Loose | movable / dynamic | 🚩 |
| Settled | simulated but resting | 🚩 |
| Held | kinematic | 🚩 |
| Clamped | static | 🚩 |
| Firm | static, with contact response | 🚩 |
| Bolted | fully fixed | 🚩 |
| Fused | welded to the world | 🚩 |

## Words retired from the viewers

These are read straight off the live screens and must not survive integration:

`Component Viewer` · `Inspector` · `Details` · `Transform` · `Location / Rotation / Scale` ·
`Static Mesh` · `Actor` · `Pawn` · `Blueprint` · `Class Defaults` · `Construction Script` ·
`Compile / Compiled / Dirty` · `Viewport` · `Socket` · `Physics` · `Collision` ·
`Generate overlap events` · `Hidden in game` · `Material channels` · `UV Layout` · `SDF Volume` ·
`Cluster LOD` · `Fracture` · `Vehicle Movement` · `Rotating Movement` · `Asset Browser` · `Instance` ·
`Scene` · `re-parent` · `Nanite` · `ShaderEditor`

Replacement words arrive with their card, are approved on their own, and are struck off this list as
they land.

---

## How a name is proposed

1. Say what the mechanism does in one plain sentence, with no engine word in it.
2. Derive the name from that sentence — the mechanism, not the metaphor.
3. Check it against `SKILL-Naming.md`: no banned word, no `Manager`/`Handler`/`Controller`/`Service`/
   `Processor`/`Provider`, no `Is`/`Has`/`Can` prefix, no `System`/`Data`/`Info`/`Object`/`Item`.
4. Add a row here with its replacement and status 🚩, and show the table in the reply.
5. Move to 🟢 only when the user says the word. Move to 🔴 and propose again if they say no.
