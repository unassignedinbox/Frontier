# Banned Words Vocabulary

**Status:** ratified 2026-11-10 · **Authority:** highest, alongside [`FolderStructure.md`](FolderStructure.md)
**Applies to:** identifiers, file names, folder names, comment concepts, branch names, change descriptions, and chart headings.

A name in Frontier is a specification. It must state the mechanism and the thing the mechanism acts on.
A name that could be swapped into any other codebase without changing a line is not a name — it is a
placeholder that costs every future reader a trip to the source.

This document states what a thing **may not** be called, and which mechanism name replaces the barred
word. Where a file disagrees with it, the file is wrong.

---

## §0 The gates

Run every proposed name through the gates in order. The first gate that fails ends the discussion —
there is no "but it reads nicely" appeal.

| Gate | Test | Failure sounds like |
| --- | --- | --- |
| 0.1 | **Does it state what the code does?** Name = subject + mechanism + domain. | `AssetPipeline` — which mechanism, over which payload, for which domain? |
| 0.2 | **Is the mechanism a real mechanism?** It must correspond to something you could describe to another engineer in one sentence without hand-waving. | `Orchestrator`, `Engine` |
| 0.3 | **Is the word borrowed from another codebase's furniture?** Framework, engine, and tutorial vocabulary is not a specification. | `RenderPass`, `TransformComponent`, `BaseManager` |
| 0.4 | **Is it a compliment rather than a description?** Power words describe how the author feels about the code. | `ApexSolver`, `SentinelGuard` |
| 0.5 | **Is it a human job title?** Job titles describe people; code describes mechanisms. | `MemoryManager` |
| 0.6 | **Is it a semantic void?** A word with no domain content hides the domain. | `Data`, `Info`, `Object` |
| 0.7 | **Is it barred by a dated ruling (§1, §3)?** | `ParticleEmitter` |
| 0.8 | **Is a composite form approved (§4)?** If the bare token is barred but a composite is ratified, use the composite. | `Record` → `RecordEntry` |
| 0.9 | **Does no honest name exist?** Then the responsibility is not yet understood. Split it until each piece has a mechanism you can name. Never name the confusion. | `SimulationCore` |

**A name that needs a ruling blocks the change.** Decisions are not made in review comments; they are
made once, dated, and written into §7.

---

## §1 Barred vocabulary

Blacklisted everywhere. Not exhaustive — §0 gates apply to words that appear on no list at all.

### 1.1 Sci-fi / power-trip

`Apex` `Sentinel` `Vanguard` `Oracle` `Titan` `Goliath`

### 1.2 Center-of-universe

`Nexus` `Hub` `Maestro` `Core` `Matrix` `Catalyst` `Conductor`

### 1.3 Pseudo-space / mythology

`Vortex` `Aether` `Zenith` `Nova` `Horizon` `Genesis` `Odyssey` `Helios` `Orion` `Kronos`

### 1.4 Generic-programmer / AI tropes

`Manager` `Handler` `Dispatcher` `Processor` `Controller` `Service` `Utility` `Helper`
`Representation` `Traversal`\* `Kernel` `Graph` `Record`† `Frame` `Node` `Module` `Data` `Info`
`Object` `Stuff` `Thing` `Item` `Common` `Base`

\* `Traversal` is exempt only at `Topology/Selection/SelectionTraversal.{h,cpp}` (user direction).
† bare `Record` is barred; the composites `RecordEntry` / `RecordStore` / `RecordToken` are approved (§4).

### 1.5 Human job titles

`Manager` `Worker` `Provider` `Dispatcher` `Conductor` `Listener` `Observer`

Barred even when a plausible role exists, unless a strict named design pattern is *genuinely* in use
(e.g. a literal std::observer-style registration mechanism). "It is basically an observer" is not
in use.

### 1.6 Kinship / family — the whole category is barred

`Parent` `Child` `Children` `Sibling` `Ancestor` `Descendant` `lineage` `family` `kin` `Orphan`

A tree link is a **technical memory link**, not a relationship. Name it by §4 and §5:
up-link, down-link, same-tier link.

### 1.7 State-change verbs

| Barred | Approved |
| --- | --- |
| `Commit` | `Finalize` / `Enforce` / `Align` / `Resolve` |
| `Compose` (as verb **and** as noun) | `Stack` / `Order` |

### 1.8 Banned variable words

`flag` `state` `value` — describe what it holds: `isVisible`, `mode`, `radiusMetres`, `tintLinear`.

### 1.9 Barred bare nouns (no composite relief)

Blocked outright in any position. A composite form does not rescue them — if the concept is real,
§0.1 gives it a different, honest name.

`action` `affinity` `arena` `badge` `binding` `card` `carousel` `classification`
`concurrency` / `concurrence` / `concurrent` `context` `continuum` `contract` `dev` `element`
`emitter` `evaluator` `fence` `floating` `flow` `glyph` `hook` `inscription` `listener` `map` `member`
`menu` `mesh` `mutation` `nexus` `occupant` `palette` `pass` `pill` `pipeline` `pool`
`popup` / `popups` `probe` `reservoir` `stage` `table`

`extension` is **strictly restricted** (not barred): permitted only for designated subsystem
lifecycle coordinators (§3.1).

---

## §2 Render units — the mechanism suffix

A GPU unit is named for the mechanism it performs, not for the borrow-able API concept it resembles.
`Stage` (2026-06-20) and `Pass` (2026-07-26) were both withdrawn as borrowed render jargon; nothing in
this repository is called a stage or a pass again.

The ratified form is **`<Subject><MechanismSuffix>`**:

| What the unit actually does | Approved form | Ratified example | Barred forms |
| --- | --- | --- | --- |
| Rasterizes geometry into a bound target that already holds content (decals, ribbons, screen-space text, grading curves) | `<Subject>Rasterization` | — (ruling 2026-08-02) | `…Inscription`, `…Pass`, `…Stage`, `…Composite` |
| Writes depth only, ahead of color | `<Subject>DepthRasterization` *(derived)* | — | `…DepthPrePass`, `…DepthStage` |
| Executes an ordered run of image operations over a target | `<Subject>Sequence` | `PostProcessSequence` | `…Chain`, `…Pipeline` |
| Groups draw work and hands it to the device | `<Subject>Submitter` *(proposed)* | — | `…Dispatcher` |

*Ratified* = carries a dated ruling in §7.
*Derived* = same mechanism as a ratified row, applied to a neighbouring target; safe to use, still
worth a dated entry when it first lands.
*Proposed* = awaiting a ruling; do **not** create a new file under a proposed suffix.

Existing `…Inscription` files are deferred renames (see [`FolderStructure.md`](FolderStructure.md) §7).
New code never uses `Inscription`, `Pass`, or `Stage`.

---

## §3 Restricted and conditionally permitted tokens

### 3.1 `Extension` — lifecycle coordinators only

`Extension` may appear only on the **designated lifecycle coordinator of an engine space**: the one
file per space that owns startup order, per-tick polling, and shutdown for that space.

Ratified holders: `RenderExtension`, `PhysicsExtension`, `InputExtension`, `WorkspaceExtension`,
`RevisionExtension`, `NetworkingExtension`.

Not a coordinator? Then it is not an `Extension`. A domain object that merely "extends" something gets
its own mechanism name.

### 3.2 Sanctioned mechanism nouns (approved by use)

These appear in ratified paths and need no new ruling. Use them where they are literally true; do not
stretch them.

`Router` `Resolver` `Registry` `Issuer` `Sequence` `Integrator` `Solver` `Packer` `Parameterizer`
`Reflector` `Rasterizer` `Outliner` `Browser` `Inspector` `Console` `Gizmo` `Overlay` `Ribbon`
`Cluster` `Launcher` `Scroller` `Selector` `Entry` `Coordinator`

`Coordinator` is reserved for lifecycle and ordering responsibility (it appears in ratified comments).
It is *not* a synonym for "the class that touches everything".

---

## §4 Approved composites

A composite is approved only when this document names it. `RecordEntry` is approved; `RecordObject`
is not.

| Barred bare token | Approved composite | Meaning |
| --- | --- | --- |
| `Record` | `RecordEntry` | one recorded datum plus its identity |
| `Record` | `RecordStore` | the collection a `RecordEntry` lives in |
| `Record` | `RecordToken` | stale-detecting handle to a `RecordEntry` |
| kinship terms | `UpLink` / `DownLink` / `SameTierLink` | the three technical tree links (§5) |
| `Frame` | `Snapshot` (captured) / `Extent` (bounded) | pick by what the thing *is* |

---

## §5 Naming tree links

Graph-shaped and tree-shaped structures are common in `GeometryProcessing`, `SpatialAcceleration`,
`Workspaces/Revision`, and `Shaders`. The kinship vocabulary (§1.6) is barred for all of them.

| The link points… | Name it |
| --- | --- |
| toward the container | `upLink` |
| toward a contained entry | `downLink` |
| across the same depth | `sameTierLink` |
| to the owning region of memory | `ownerRegion` |

Comments say "up-link region", "down-link region", "same-tier link" — never "parent", "child", or
"sibling", even in prose.

---

## §6 Replacement chart

For the words that keep reappearing in review, the honest replacement is usually *more specific than
the barred word*, which is the point.

| Barred | Replace with |
| --- | --- |
| `Manager` | the mechanism: `…Store`, `…Archive`, `…Registry`, `…Allocator` |
| `Handler` | the mechanism: `…Route`, `…Intake`, `…Sink` |
| `Dispatcher` | `…Router` (routing) or `…Submitter` (device hand-off) |
| `Processor` | the mechanism: `…Solver`, `…Transform`, `…Rasterization` |
| `Controller` | `…Resolver`, `…Coordinator`, `…Kinematic` names |
| `Service` | the mechanism, or delete it |
| `Utility` / `Helper` | delete; name what it holds (`…Foundation`, `…Primitives`) |
| `Data` / `Info` / `Object` / `Thing` / `Stuff` / `Item` | the payload: `…Specification`, `…Descriptor`, `…Packet`, `…Entry`, `…Archive` |
| `Module` / `Subsystem` | the territory: `…Space` |
| `Node` | the payload of the node: `…Entry`, `…Vertex` |
| `Graph` | `…Topology` |
| `Frame` | `…Extent` (bounds), `…Snapshot` (captured) |
| `Parent` / `Child` / `Sibling` | up-link / down-link / same-tier link |
| `Hierarchy` | `…Tree` (e.g. `BoundingVolumeTree`) |
| `Entity` | `Actor` *(proposed)* |
| `Base` | `…Foundation`, or the concrete form (`Stock`, `Surface`) |
| `Core` / `Hub` / `Kernel` | delete the word; name the mechanism |
| `Stage` / `Pass` | §2 mechanism suffix |
| `Emit` / `Emission` | `…Source`, `…Release`, `…Seed` |
| `Commit` | `Finalize` / `Enforce` / `Align` / `Resolve` |
| `Compose` | `Stack` / `Order` |
| `flag` / `state` / `value` | what it holds |

---

## §7 Ruling log

Dated rulings. Quoted rulings are historical text and may contain the barred word themselves; the
quotation is the record, not a licence to reuse the word.

**2026-06-20** — `system` `compose` `mesh` `harness` `shell` `popover` `Tween` `Kind` `proof`
`glide` `binding` `Role` `Common` barred.
> `harness` was withdrawn in favour of **Validation**: a test scaffold is a *…Validation*, and a
> validation bench is not a harness.

**2026-07-15** — `stage` barred as render jargon.

**2026-07-25** — `Hierarchy` `Entity` `Element` `Subsystem` `Designation` barred (`Designation` was
reuse-authorized, now barred). `system` re-affirmed barred **as any noun**. `Trial` / `Trials` barred
(was the harness proposal — courtroom metaphor) → **a test scaffold is a …Validation**.
> `backend` (web "layer beneath" jargon) barred → raw-ingestion layer = *…Substrate*.

Same day, tree links: kinship vocabulary barred for tree structures → up-link / down-link /
same-tier link (§5).

**2026-07-26** — `Pass` barred (borrowed from `VkRenderPass`; this revokes the earlier
`stage` → `Pass` ruling) and `Emission` / `Emit` barred (AI default) → a GPU render unit takes a
mechanism suffix (§2, render-unit chart).

**2026-08-02** — `Inscription` barred (it had been the §2 replacement for composite-onto-target
units) → a GPU render unit that composites onto an existing target takes **`…Rasterization`** (it
still rasterizes ribbon / glyph / decal geometry into the target). New code never uses
`…Inscription`; existing `…Inscription` files are deferred renames (see
[`FolderStructure.md`](FolderStructure.md) §7).

---

## §8 Adding a ruling

1. Propose the name **and** the mechanism it states, in one sentence.
2. Show it failing no gate in §0.
3. Get it dated and written here, newest last, in the same session as the first file that uses it.
4. Update [`FolderStructure.md`](FolderStructure.md) §7 if the name replaces a legacy path.

A proposal that exists only in a chat message or a review comment is not a ruling, and the file that
depends on it does not get created.
