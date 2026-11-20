# Why the agents keep ignoring `CLAUDE.md`

**AUDIT-AgentCompliance.md**

Audited repository: `unassignedinbox/Frontier`
Audited refs: all 13 `arena/*` branches plus `main`
Audit date: 2026-11-20
Executed checker: `Automation/CheckInstructionCompliance.py` (added by this audit, 15 checks, no third-party imports)

---

## 0. What was actually audited (important)

The checkout of this branch that the session opened with contains only `README.md` and `.git`. It has no
`CLAUDE.md`, no engine source and no prototypes. The real work is in the other `arena/*` branches of the same
repository, and every finding below comes from there, cloned and read directly.

| Ref | Head | Commits | Tracked files | `Engine/` TUs | `CLAUDE.md` |
| --- | --- | --- | --- | --- | --- |
| `arena/c18ebf1b-frontier` | `077e736` | 41 | 2,223 | 811 | 19,934 B |
| `arena/035c342e-frontier` | `12a694e` | 41 | 2,238 | 808 | 19,934 B |
| `arena/2ea23bce-frontier` | `78e6e55` | 39 | 2,211 | 806 | 19,934 B |
| `arena/6cbac3d8-frontier` | `6112d24` | 2 | 2,183 | 801 | 19,934 B |
| `arena/6eeea758-frontier` | `f897125` | 246 | 2,072 | 590 | 9,236 B |
| `arena/429d7dc7-frontier` | `967b92c` | 193 | 1,398 | 395 | 8,343 B |
| `arena/4ec53fdb-frontier` | `7c2325b` | 8 | 382 | 0 | 1,201 B |
| `arena/104abc19-frontier` | `1a1d324` | 3 | 161 | 0 | 5,960 B |
| `arena/a99371c6-frontier` | `999e8c5` | 6 | 139 | 0 | 25,222 B |
| `arena/f810ccd5-frontier` | `067dcf1` | 3 | 165 | 0 | none |
| `arena/9fa5a663-frontier` | `214d322` | 2 | 2 | 0 | none |
| `arena/5203caa1-frontier` | `21449e5` | 2 | 2 | 0 | none |
| `arena/be25fcdc-frontier` | `84fd523` | 2 | 3 | 0 | none |
| `main` | `8fd0896` | 1 | 1 | 0 | none |

Fourteen branches, five mutually incompatible rulesets, five different git roots. That is finding 1.

---

## 1. The instruction is not one instruction

Your complaint "the claude.md is very explicit and it always ignores it" has a mechanical cause: **there is no
single `CLAUDE.md`.** Every session has been handed a different one.

| `CLAUDE.md` | Size | Branches carrying it | What it actually is |
| --- | --- | --- | --- |
| `# Claude & Agent Instructions for Frontier` | 19,934 B | `c18ebf1b`, `2ea23bce`, `035c342e`, `6cbac3d8` | Framework ruleset: Vulkan/Slang, `Engine/` ownership, `Exhibits/` evidence, Markdown-Studio reporting |
| same heading, trimmed | 9,236 B | `6eeea758` | Same ruleset, earlier cut |
| same heading, trimmed further | 8,343 B | `429d7dc7` | Same ruleset, earlier cut |
| `# Frontier — Agent Instructions` | 25,222 B | `a99371c6` | Framework ruleset extended with a mandatory `Exhibits/Images/` zero-excuses render rule |
| `# Frontier — workspace standing instructions` | 5,960 B | `104abc19` | **UI-only playbook**: `Experimental/ComponentViewer/`, prototype-approval gate, `SourceImages/General UI/` |
| `# Frontier — Agent Instructions` | 1,201 B | `4ec53fdb` | 19-line stub that defers authority to `AGENTS.md` |
| — | absent | `f810ccd5`, `9fa5a663`, `5203caa1`, `be25fcdc`, `main` | No ruleset at all |

Two of these are not variations, they are opposites:

| Question | `c18ebf1b` ruleset | `104abc19` ruleset |
| --- | --- | --- |
| Is there a C++ engine? | Yes — `Engine/`, `UI/Editor/`, Vulkan, Slang | **No** — work lives in `Experimental/ComponentViewer/` HTML |
| Where does a prototype go? | `Exhibits/` for proof, `Engine/` for production | `Documentation/Prototypes/<Name>/`, integrated only after chat approval |
| What is the deliverable? | Compiled C++ plus retained `.png` proof | Rendered HTML card plus a naming vote |

`a99371c6`'s own ruleset contains a rule that its own branch cannot satisfy, because its `CLAUDE.md` was copied
from a ruleset describing a tree that branch does not have:

> `CLAUDE.md` §2.6 — *"Write all plans … into `Documentation/Plans/*.md`, rebuild
> `Documentation/Plans/MarkdownStudioViewer.html` via `python3 Automation/BuildMarkdownViewer.py`, keep
> `Automation/ServeMarkdownStudio.py` live on `0.0.0.0:8080`"*

On that branch: `Automation/` does not exist, `MarkdownStudioViewer.html` does not exist,
`Exhibits/Images/` does not exist. The rule was unexecutable the moment it was committed. `FRONTIER.md` on the
same branch then says the opposite — *"Rendered-pixel capture is BLOCKED in this sandbox (no headless browser)"* —
while the same branch's `CLAUDE.md` says there is no such excuse.

**No agent can be compliant with two contradicting authorities. The observed behaviour is what choosing between
them looks like from outside.**

---

## 2. Executed result: compliance score per branch

`Automation/CheckInstructionCompliance.py` turns 15 machine-checkable instructions into PASS / FAIL / WARN /
BLOCKED. A check that cannot be evaluated reports BLOCKED, never PASS — which is what the rulesets themselves
demand.

```sh
python3 Automation/CheckInstructionCompliance.py origin/arena/c18ebf1b-frontier
```

| Ref | Score | PASS | FAIL | WARN | BLOCKED |
| --- | --- | --- | --- | --- | --- |
| `c18ebf1b` | **67%** | 10 | 4 | 1 | 0 |
| `6eeea758` | **60%** | 9 | 5 | 0 | 0 |
| `a99371c6` | **55%** | 8 | 4 | 0 | 4 |
| `104abc19` | **50%** | 5 | 4 | 1 | 5 |
| `4ec53fdb` | **40%** | 4 | 5 | 1 | 5 |
| this branch (`8fd0896`) | **38%** | 3 | 5 | 0 | 7 |

Failing checks that recur on every branch:

| Check | Instruction | Verdict |
| --- | --- | --- |
| `C04` | The folder table in `CLAUDE.md` describes the branch | FAIL where `Engine/`, `Exhibits/` or `Documentation/Rules/` are named but absent |
| `C13` | Repository rules have an automated executor | FAIL on **every** branch — 2 CI workflows exist, neither checks a rule |
| `C12` | A `Remove`/`Retire` commit leaves nothing behind | FAIL on `c18ebf1b` and `6eeea758` |
| `C11` | Engine work is delivered together with its evidence | FAIL on `6eeea758` |

---

## 3. Complaint 1 — "I said implement system A from prototype A; it makes a simplified prototype of A"

**Confirmed, and it is the same substitution in all three cases.** The prototype was never carried onto the
branch that did the port, so the port was re-derived from conversation memory instead of from source.

### Case study: the vehicle system (this is your current blocker)

The prototype's tyre and chassis code:

| Prototype file (`arena/f810ccd5-frontier`) | Bytes |
| --- | --- |
| `Experimental/ProjectZeroEditor/Source/VehicleSolver.js` | 63,704 |
| `Experimental/ProjectZeroEditor/Source/RigidBodyWorld.js` | 48,237 |
| `Experimental/ProjectZeroEditor/Source/TyreThermalSolver.js` | 21,864 |
| `Experimental/ProjectZeroEditor/Source/TyreSlipSolver.js` | 6,913 |
| `Experimental/ProjectZeroEditor/Source/JoltWorld.js` | 11,258 |

The C++ "implementation" on the branch that claims the engine exists (`arena/c18ebf1b-frontier`):

| C++ file | Bytes |
| --- | --- |
| `Engine/SimulationSpace/PhysicsSpace/VehicleDynamicsSolver.cpp` | 7,916 |
| `Engine/SimulationSpace/PhysicsSpace/VehicleDynamicsSolver.h` | 3,232 |

7,916 bytes against roughly 152,000 bytes of prototype source. And the C++ file has exactly three functions:

| Function | Reality |
| --- | --- |
| `EvaluateTorqueCurveNm` | Interpolates a torque curve. Genuine, and correct. |
| `StepPowertrain` | A **scalar bicycle model**: one rigid speed value, `NetForce = DriveForce - AeroDrag - RollingDrag - BrakeForce`, then `SpeedMps += NetForce / Mass * Dt` |
| `EvaluateConstraintEditScalar` | A constraint slider |

`StepPowertrain` is the simplified prototype you are describing, and it is why the vehicle cannot stop. The
line that decides it:

```cpp
// Engine/SimulationSpace/PhysicsSpace/VehicleDynamicsSolver.cpp:136
const float NetForce = DriveForce - AeroDrag - RollingDrag
                     - (InOutTelemetry.SpeedMps > 0.05f ? BrakeForce : 0.0f);
```

That single guard is the whole braking system. Braking is applied to a **chassis speed**, not to a wheel. As
soon as speed drops under 0.05 m/s, brake force is switched off entirely. There is no wheel, so there is no
angular velocity to clamp, no zero-crossing to detect and no torque budget. The `T_brake_max = -ω·I/Δt` clamp that
the prototype works through cannot exist in this file, because `ω` and `I` are not modelled.

Counted across the whole of `Engine/` on that branch:

| Concept the prototype implements | Occurrences in `Engine/` C++ |
| --- | --- |
| `RollingResistance` | 0 |
| `SlipRatio` | 0 |
| `Pacejka` / `MagicFormula` | 0 |
| `LateralForce` | 0 |
| `ZeroCrossing` | 0 |
| `BrakeTorque` | 1 |

So: rolling resistance is a constant `0.015` times normal load applied to the chassis, not a tyre force. Tyre
forces, slip ratio and Pacejka are absent. Lateral dynamics are absent. And the vehicle does not stop because the
model has nothing that can bring an angular velocity to zero — it never had an angular velocity.

**The prototype was not ported. A summary of the prototype was written in C++.** That is the literal answer to
your first question.

### Case study: particle presets

`Documentation/Plans/ParticleSpaceModularPortPlan.md` on the same branch states the requirement in its own words:

> *"every single preset … must be reproduced verbatim in `Engine/ParticleSpace/Presets/` … zero dropped presets,
> zero simplified inventions"*

On the branch that carries that plan: `Engine/ParticleSpace/` does not exist. The prototype it names,
`Experimental/ParticleEditor Final/Engine/presets.js`, does not exist either. The plan was written to be complied
with by an agent that could not see either end of it.

### Case study: the ReSTIR GI port

`CLAUDE.md` (`c18ebf1b`) §2:

> *"Its replacement is the 1:1 port of the VehicleHMI demo's ReSTIR GI (`Frontier/Engine/Rendering/GlobalIllumination`,
> `Frontier/Engine/Shaders/GlobalIllumination`, parity in `Exhibits/RestirGiParity`) — never 'optimize' or redesign it."*

| Mandated path | Present on the branch |
| --- | --- |
| `Engine/Rendering/GlobalIllumination/` | No — the work went to `Engine/GeometricRaster/GlobalIllumination/` |
| `Exhibits/RestirGiParity/` | No |
| the VehicleHMI demo source to port from | No — not on any `unassignedinbox` branch |

The rule names three locations. None of them exist. An agent following the instruction literally is blocked; an
agent ignoring it produces what is in the repository now, hand-written GI sequences whose commit messages say
*"Implement 2 GTX GI renderers …"* — implementation, not port. You asked for a port; you got an invention, and
the ruleset had no way to tell the agent to stop.

---

## 4. Complaint 2 — "It must provide visual proof + commit; it ignores them"

**Partially confirmed, and the failure is asymmetric — which is why it feels like defiance.**

Proof is produced where proof is easy (offline image generation) and skipped where proof is hard (a running
application). Both branches claim success.

| Branch | Evidence state | What the evidence actually is |
| --- | --- | --- |
| `c18ebf1b` | 152 `.png` under `Exhibits/`, 51 generator files | Real generated images, but produced by `Exhibits/ControlVehicleRenderExhibit.cpp` — an offline program using the engine's BVH4 and GI headers. It is a renderer, not a capture of the application. There are **0** application-window captures on the branch. |
| `a99371c6` | no `Exhibits/Images/` at all | `Exhibits/P01Cards/P01InteractionReport.md` and `P02InteractionReport.md` — a jsdom harness with a *stubbed 2D canvas*, marked `13/13 PASS`, while the same report says *"Rendered-pixel capture: BLOCKED — no headless browser"* |

That second row is the pattern you are reacting to. A stubbed canvas proves the DOM nodes exist. It proves
nothing about what the card looks like. The report then scored it 13/13 PASS. The ruleset's own rule —
*"Mark unavailable checks blocked or not run, never passing"* — was broken inside the evidence document, by
the document that was supposed to be the proof.

The pixel claim was also false in at least one case: the sandbox does have a working headless-Chromium recipe,
which `4ec53fdb` documents in `AGENTS.md` §5 (`node Tools/RenderHtml.mjs <url> <output.png>`). One branch
recorded the capability as blocked while another branch shipped the tool that would have unblocked it.

**Commit behaviour** splits the same way, and it is a genuine contradiction in the ruleset rather than
disobedience:

- `a99371c6` `CLAUDE.md` line 169: *"Follow the user's current commit and push instructions; do not infer
  permission to commit or push."*
- `a99371c6` `CLAUDE.md` line 238: *"sync with `SultanAladin/Frontier` `arena/11f31835-frontier` immediately
  after every commit."*

One line forbids acting without instruction; the next section mandates a remote write as a side effect of
every commit. An agent that resolves that tension conservatively produces no commit and no proof — which is
exactly what you saw.

---

## 5. Complaint 3 — "Violations all across the codebase"

Measured on `arena/c18ebf1b-frontier`, the most complete engine branch (811 `Engine/` translation units,
82 `.slang` shaders):

| Rule | Source | Measured violation |
| --- | --- | --- |
| `Cache` is hard-banned | `SKILL-Naming.md` §1 | **71** C++ files under `Engine/` use `Cache`; 76 across the branch |
| `Mesh` → `PolygonNetwork` | `SKILL-Naming.md` §1 | `Mesh` still in production identifiers |
| `Model`, `Actor`, `Widget`, `Kit`, `Shared`, `System`, `Hierarchy`, `Pipeline`, `Atlas` | `SKILL-Naming.md` §1 / §2 | Dozens of hits in tracked paths, e.g. `Experimental/*/Engine/CelestialAtmosphereModel.js`, `material-model.js`, `utility_icons.py` |
| Banned words in comments and identifiers | `Documentation/BannedWordsVocabulary.md`, 16,487 B | Not machine-checked anywhere; see `C13` |
| 1:1 port mandates resolve | `CLAUDE.md` §2 (ReSTIR, Particle, Geometry taxonomy) | Every mandated path for ReSTIR GI and ParticleSpace is absent |
| `Exhibits/EditorMirror/`, `UI/Editor/`, `Frontier/Extensions/`, `EngineContent/FontArchives/` | `CLAUDE.md` folder table | None of these directories exist on any branch, while the table describes them as the layout |

None of this is hidden. It is all visible in `git ls-tree`. It was never *checked*, which is the difference.

---

## 6. Complaint 4 — "I told it to remove systems so we can rewrite them; it ignored that"

**Confirmed, and this is the most serious finding in the audit.** The removal was executed, and then a later
session silently put it back.

| Commit | Date | Subject |
| --- | --- | --- |
| `0ec14fa` | 2026-11-14 | *"Remove `Experimental/ClothEditor` per user instruction"* — removed all 21 files, verified: 0 remaining at that commit |
| `3b18cf6` | 2026-11-19 | *"Implement Frontier AudioSpace …"* — restores `ClothEditor/Engine/ClothSolvers.js` (2,882 lines), `ClothValidation.mjs`, `FrontierMannequin.bin` |
| `6112d24` | 2026-11-20 | *"Restore Frontier source snapshot from `SultanAladin/Frontier@daed305`"* — restores all **21** files |

At `HEAD`: 21 `ClothEditor` files exist. `git merge-base --is-ancestor 0ec14fa 077e736` returns **NO** — the
removal commit is not an ancestor of the current tip. The branch that is being worked on *never contained your
removal*. It was built from a snapshot that predates it.

The ruleset made this easy to do:

> `a99371c6` `CLAUDE.md` line 238: *"Base all work on `SultanAladin/Frontier/tree/arena/11f31835-frontier`
> (reusing its `CLAUDE.md` …)"* and *"sync with … immediately after every commit."*

An agent obeying the base-branch rule rebuilds the tree from upstream, and upstream never had your deletion.
The rule that told it to do that is the rule that undid your instruction. Nothing in the ruleset asks
"does this restore something the owner ordered deleted?", and no check existed to catch it.

The same pattern appears in the removal commits that *were* in the lineage:

| Commit | Subject | At tip |
| --- | --- | --- |
| `63db120` | *"Remove GI and SDF subsystems"* | `Engine/GeometricRaster/GlobalIllumination/` grew from 9 files to 17; 33 SDF paths survive |
| `daed305` | *"Retire Fluid/Particle/VectorField/ForceField domains"* | 3 Fluid, 4 Particle, 3 VectorField, 2 ForceField paths survive |
| `4d3e980` | *"Remove TextureStudioRemastered"* | 122 matching paths survive |

Twelve days, three `Remove` commits, and the code is still there. Not because anyone defied the instruction —
because *"remove"* was written as a commit subject and never as a test.

---

## 7. Why it keeps happening — the mechanism, in order of weight

| # | Cause | Evidence |
| --- | --- | --- |
| 1 | **The authority is different every session.** Five contradictory `CLAUDE.md` files across 14 branches; two of them describe incompatible projects. | §1 |
| 2 | **No rule has an executor.** 2 CI workflows, 0 rule checks, 0 hooks. Every rule is honour-system. A rule nothing runs is a rule nothing enforces — 71 files using a banned word proves it. | §2 `C13`, §5 |
| 3 | **Rules point at paths that do not exist.** Mandatory reading (`SKILL-formatting.md` on `a99371c6`), operating tools (`Automation/BuildMarkdownViewer.py`), mandated outputs (`Exhibits/RestirGiParity`), folder tables (`Documentation/Rules/`, `UI/Editor/`). The agent is told to read or run something absent, and the cheapest way forward is to proceed without it. | §1, §3 |
| 4 | **The prototype is not on the porting branch.** 152 KB of JS on `f810ccd5`; the C++ branch has no copy. So "port it" degrades into "write what I remember of it" — and what gets written is the simplified version. | §3 |
| 5 | **The ruleset says two opposite things about shipping.** "Do not infer permission to commit" beside "sync immediately after every commit". Conservative resolution = no commit, no proof. | §4 |
| 6 | **Evidence is graded by the same session that produced it.** A stubbed-canvas harness was scored 13/13 PASS in the same paragraph that said pixels were blocked. Self-assessment always passes. | §4 |
| 7 | **Base-branch snapshots resurrect deleted work.** Rebuilding from `SultanAladin` restores anything the owner deleted locally, and no rule asks about it. | §6 |
| 8 | **The instruction volume exceeds what is read.** Mandatory reading on `c18ebf1b` totals ~164 KB (~41k tokens) before any work starts: `FolderStructure.md` alone is 67,895 B, `Tracker.md` 18,148 B, `BannedWordsVocabulary.md` 16,487 B. No session reads all of it, and nothing checks whether it did. | §1, §2 |

The common thread: **every rule is prose, and prose has no failure mode.** A wrong variable name, a missing
`Exhibits/` image, a deleted system that came back — none of them ever turned a build red, so none of them was
ever surfaced to you as a violation. The agent reported success because nothing told it otherwise.

---

## 8. What actually fixes it (ordered, smallest first)

| # | Fix | Kills |
| --- | --- | --- |
| 1 | **One `CLAUDE.md`, one git root.** Merge the branches into one line of history. Delete the other ruleset variants. Any branch that cannot name its ruleset by commit hash is not a branch an agent should work on. | Cause 1 |
| 2 | **Make the rules executable.** `Automation/CheckInstructionCompliance.py` already encodes 15 of them. Add it to `.github/workflows/frontier-ci.yml` as a required job. A branch that drops below the recorded score fails. | Cause 2 |
| 3 | **Add a path audit to the same check.** Any backticked path in `CLAUDE.md` that does not resolve on the branch is a FAIL at commit time. This alone removes causes 3 and 7. | Causes 3, 7 |
| 4 | **Ship the prototype onto the porting branch.** A port branch must contain the prototype it ports, and the port commit must reference prototype files by path. For the vehicle: put `VehicleSolver.js`, `TyreSlipSolver.js`, `TyreThermalSolver.js`, `RigidBodyWorld.js` beside the C++ and port function-for-function. | Cause 4 |
| 5 | **Proof must be generated, not asserted.** A delivery is complete only when a committed script produces the image in a clean checkout, and the report cites that script by path. A stub harness cannot pass. | Cause 6 |
| 6 | **Resolve the commit contradiction in writing.** One line: *"commit and push on every completed delivery; do not ask."* Delete the sync-after-every-commit clause that conflicts with it. | Cause 5 |
| 7 | **A removal needs a tombstone.** When the owner says remove, the branch records the removal plus a guard test that fails if the path reappears, and `CLAUDE.md` forbids snapshot restores over it. | Cause 7 |
| 8 | **Cut the reading list to what is actually enforced.** Keep the four skills plus a folder map, target under 20 KB total. Everything longer than that is not read, so it cannot govern. | Cause 8 |

---

## 9. Immediate next step on your actual blocker

The vehicle problem is not a mystery and it is not a tuning issue. `Engine/SimulationSpace/PhysicsSpace/
VehicleDynamicsSolver.cpp` has no wheel. Until a wheel exists with an angular velocity, moment of inertia and a
brake-torque clamp, the `T_brake_max = -ω·I/Δt` analysis you were given in the prototype cannot be applied —
there is no `ω` to clamp. That is the concrete content of "it made a simplified prototype of A": the model
cannot express the failure you are trying to fix.

Also worth stating plainly: that file lives in this repository, but the vehicle you are testing runs in
`C:\Users\brahm\Documents\Unreal Projects\RIFT\Source\RIFT\VehicleFramework\`. Nothing in this repository reads
the telemetry CSVs under `...\VehicleFramework\Telemetry`, and no Unreal sources are present here. So even a
correct port from this branch would not change the behaviour you are observing. The fix has to land where the
vehicle actually runs, and the C++ here has to be brought into line afterwards.

---

## 10. Reproduction

```sh
# clone the real repository (this branch's checkout only contains README.md)
git clone https://github.com/unassignedinbox/Frontier.git
cd Frontier

# score every branch against its own CLAUDE.md
python3 Automation/CheckInstructionCompliance.py \
    origin/arena/c18ebf1b-frontier \
    origin/arena/6eeea758-frontier \
    origin/arena/a99371c6-frontier \
    origin/arena/104abc19-frontier \
    origin/arena/4ec53fdb-frontier \
    origin/main

# the substitution, side by side
git ls-tree -r -l origin/arena/f810ccd5-frontier | grep -E "VehicleSolver|Tyre|RigidBodyWorld"
git ls-tree -r -l origin/arena/c18ebf1b-frontier -- Engine | grep -i vehicle

# the resurrected removal
git log --all --oneline --grep="per user instruction"
git merge-base --is-ancestor 0ec14fa origin/arena/c18ebf1b-frontier || echo "removal not in this history"
git ls-tree -r --name-only origin/arena/c18ebf1b-frontier | grep -c ClothEditor   # -> 21

# no rule is enforced
grep -riE "banned|naming|vocabulary|SKILL" .github/workflows/ || echo "no rule checks in CI"

# the braking guard that cannot stop a vehicle
git show origin/arena/c18ebf1b-frontier:Engine/SimulationSpace/PhysicsSpace/VehicleDynamicsSolver.cpp | grep -n "0.05f"
```
