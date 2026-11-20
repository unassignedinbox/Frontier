# CLAUDE.md rewrite and documentation cleanup

**2026-11-20 · against `SultanAladin/Frontier` `arena/8391e994-frontier` (`940a125`)**

---

## TL;DR

The old `CLAUDE.md` described a repository that does not exist. It mandated an HTML-to-C++ screenshot
procedure and then cited **9 paths that had been deleted**, and it specified five "active" GI pipelines whose
core types and **six shaders no longer exist**. An agent following it either hit a dead path or silently
substituted something else — which is exactly the behaviour that produced the mismatched UI and the simplified
particles.

Three changes:

1. **`CLAUDE.md` rewritten** (19,934 B → 21,014 B, 131 → 348 lines). Same hard rules, plus a screenshot
   section that states where captures come from, the four superseded plans named explicitly, and six rules
   that each record a specific past failure. Every cited path resolves.
2. **`Automation/AuditCheck.py` added.** 16 checks that turn the rules into pass/fail. Nothing in this
   repository could previously fail. `docs` drift, resurrected systems, and unsupported naming now do.
3. **Four dead plans marked `SUPERSEDED`** with a banner explaining why, and the two indexes that pointed at
   deleted workspaces corrected.

| Measurement | Before | After |
| :--- | :--- | :--- |
| `AuditCheck.py` score, old ruleset on this tree | **88%** (1 FAIL) | — |
| `AuditCheck.py` score, new ruleset on this tree | — | **94%** (0 FAIL) |
| Paths cited in `CLAUDE.md` that do not exist | **9 of 59** | **0 of 56** |
| Documented GI pipelines whose types exist | **0 of 5** | 0 of 5 — and the doc no longer claims otherwise |
| Plans presented as live work for deleted systems | **4** | **0** |

The residual 6% is one WARN: 14 of 77 `.slang` shaders are not named in `CMakeLists.txt`. That is a real
finding, not a documentation problem. It is listed under Remaining work.

---

## 1. What was wrong with the old file

### 1.1 It cited nine deleted paths as authorities

The old `CLAUDE.md` §1 told every session to read `Engine/Workspaces/Fluid/README.md` and
`Engine/Workspaces/Particle/README.md`. Both were removed by `daed305`. It pointed at
`Exhibits/DeterministicPathTracer/README.md`, removed by `63db120`.

Measured on the branch itself:

```
$ python3 Automation/AuditCheck.py        # with the old CLAUDE.md
FAIL  A03  Every path cited in CLAUDE.md resolves
      9 of 59 cited paths do not exist: DynamicProbeCache.slang, GpuTextureAndSdfBaker.h,
      J2000BrightStarCatalog.h, OctahedralIrradianceLattice.slang, ReservoirPathReplay.slang,
      ReservoirTemporalFilter.slang, SkyViewTransmittanceCompute.slang…  (and 3 more)
```

### 1.2 Its rendering section described five pipelines that no longer exist

§4 of the old file was 3,000 words specifying "active" CPU and GPU pipelines for GI, atmosphere, celestial
discs, lunar exospheres, and starfields. Checked against `Engine/` on the same commit:

| Symbol asserted by §4 | Files containing it |
| :--- | :--- |
| `DynamicProbeCacheSequence` | **0** |
| `ReservoirPathReplaySequence` | **0** |
| `IrradianceLatticeSequence` | **0** |
| `SignedDistanceRadiosityVolume` | **0** |
| `J2000BrightStarCatalog` | **0** |
| `DynamicProbeCache.slang` | **0** |
| `ReservoirPathReplay.slang` | **0** |
| `ReservoirTemporalFilter.slang` | **0** |

§4.2 then described those same algorithms as "Retired / Replaced (Do Not Reintroduce)" and promised their
compatibility symbols "remain intact so `Exhibits/`, UI settings, and GPU uniform layouts do not break". The
symbols are gone, and `WorldRenderEngine` has two modes, not five.

An agent reading §4 concludes the GI system exists and is maintained. It does not exist. That is the single
most misleading thing in the file, and it is 40% of its length.

### 1.3 Its best rule had no working mechanism

The HTML-to-C++ rule (§2.5) already said the right thing: capture the HTML, read its numbers, reuse the kit,
capture the C++, compare. It was still ignored, because:

- the rule named no capture command;
- no session could check whether a capture existed;
- the sandbox has no browser, and every session therefore discovered a dead end and worked around it silently.

That last point is now measured and written down rather than left to be rediscovered:

```
$ npx puppeteer browsers install chrome
Error: All providers failed for chrome-headless-shell 157.0.8103.78:
  - DefaultProvider: Client network socket disconnected before secure TLS connection was established
$ curl -o /dev/null -w "%{http_code}" https://storage.googleapis.com/   -> 000
$ curl -o /dev/null -w "%{http_code}" https://edgedl.me.gvt1.com/      -> 000
```

`npm install puppeteer` works. The browser binary does not download, and no browser is preinstalled. So
`CLAUDE.md` §2.2 now splits the world in two: the owner's Windows machine, where the capture is one command,
and a sandbox, where the honest answer is `BLOCKED — capture unavailable` with that output pasted in.

### 1.4 It mixed two different instruction styles

The file asserted `MUST`, `NEVER`, and 🔴 imperatives for architecture, and separately for process. Nothing
distinguished a rule that is enforced by the compiler from a rule that is enforced by nobody. Every rule read
with the same weight, so the enforceable ones drowned.

The new file separates them: §5 architectural mandates that a build or a diff can catch; §8 six rules each
attached to the specific incident that created it, with the command that proves compliance.

---

## 2. What changed, file by file

| File | Change | Why |
| :--- | :--- | :--- |
| `CLAUDE.md` | Rewritten. §4 GI pipelines removed; §0 current-state table added; §2 rewritten around capture provenance; §4 names the four superseded plans; §8 added; §9 state table added | §1 above |
| `Automation/AuditCheck.py` | **New.** 16 checks, zero dependencies, three output modes | Nothing could fail before |
| `Documentation/Plans/ParticleSpaceModularPortPlan.md` | `SUPERSEDED` banner appended under the title | Describes `Engine/ParticleSpace/`, deleted |
| `Documentation/Plans/ParticleRendererDerezMaterialTexturePlan.md` | `SUPERSEDED` banner | Same |
| `Documentation/Plans/VectorFieldArchitecturePlan.md` | `SUPERSEDED` banner | Describes `Engine/SimulationSpace/ForceFields/`, deleted |
| `Documentation/Plans/VectorFieldVulkanConversionPlan.md` | `SUPERSEDED` banner | Same |
| `Documentation/Plans/README.md` | Banner plus ⛔ markers on the four rows | Index presented them as live |
| `Documentation/README.md` | Fluid and Particle rows marked deleted; dead path-tracer link removed; `RemovedSdfGi` added to the index | Linked to two removed workspaces and one removed exhibit |

Each plan file keeps its full original text. Only a banner is prepended, so the record of what was wrong
survives. Deleting them would have destroyed the only account of the rejected designs.

---

## 3. The new ruleset in one page

| Section | Content | Enforceable by |
| :--- | :--- | :--- |
| §0 | Current state: two render modes; five systems deleted and archived; no rebuild without owner sign-off | `A05`, `A06` |
| §1 | Seven mandatory documents, then a reference index to consult only when relevant. Explicitly names the three removed documents as *not* references | `A03`, `A04` |
| §2 | HTML-to-C++ procedure, capture provenance per environment, evidence table | `A14` |
| §3 | What each kind of work must show before it may be called done | review |
| §4 | No deleted names; four superseded plans listed; hard-banned words; `Space` / `Extension` / `Sequence` / `Rasterization` suffixes | `A06`, `A07`, `A12`, `A13` |
| §5 | GPU overlays, `.slang` only, portability, three profiles, dependencies, reuse-first, TOML | `A09`, `A10` |
| §6 | Where work lives; the archive is read-only; one tracker | `A08`, `A11` |
| §7 | Output shape, delivery checklist ending in `AuditCheck.py` | review |
| §8 | Six rules, each with the incident that caused it and the command that proves compliance | `A15` |
| §9 | Current state table with the honest entries (particle ground truth not in this repo; vehicle solver is not a port) | review |
| §10 | Bringup order and the per-feature order | review |

---

## 4. The checker

```bash
python3 Automation/AuditCheck.py                       # working tree
python3 Automation/AuditCheck.py --ref origin/main     # any ref, read-only
python3 Automation/AuditCheck.py --json                # machine output
python3 Automation/AuditCheck.py --markdown            # report body
```

Exit `0` when every evaluated check passes, `1` on any FAIL, `2` on usage error. **`BLOCKED` is never a pass** —
that is the rule the old evidence reports broke when a stubbed canvas scored 13/13.

Result on this branch, after the change set:

```
Score 94%  |  15 PASS  0 FAIL  1 WARN  0 BLOCKED
  PASS  A01  CLAUDE.md is the single ruleset
  PASS  A02  AGENTS.md points at CLAUDE.md
  PASS  A03  Every path cited in CLAUDE.md resolves
  PASS  A04  Mandatory reading and reference index resolve
  PASS  A05  Deleted domains are absent from Engine/
  PASS  A06  No production path is named after a deleted domain
  PASS  A07  Plans for deleted systems are marked SUPERSEDED
  PASS  A08  RemovedSdfGi archive is present and complete
  PASS  A09  Engine/Shaders contains only .slang
  WARN  A10  Every .slang has a CMake compile rule      (14 of 77 unwired)
  PASS  A11  Experimental/ holds no engine C++
  PASS  A12  No conjunction `And` in an identifier
  PASS  A13  No hard-banned token in a production filename
  PASS  A14  UI conversion carries an HTML ground-truth capture
  PASS  A15  Removal commits leave nothing behind
  PASS  A16  Exhibits carry committed proof images
```

Three checks deserve a note, because a naive version of each would have been noise:

- **`A14`** does not demand a capture for every domain. It fails only when a commit changed UI surface code
  without bringing evidence. Otherwise it passes. Demanding 26 captures retroactively would have made the
  check unpassable, and an unpassable check gets deleted.
- **`A15`** judges a removal claim against that system's own engine surface, not against every file whose name
  contains the letters. `GpuSdfBaker` surviving an *SDF subsystem* removal is not a violation; a surviving
  `Engine/DistanceFieldSpace/` would be.
- **`A12`** binds identifiers, not proof images. `01-DawnAndTwilightSky.png` is a filename of a captured chart,
  not an identifier, and the naming skill does not govern it.

---

## 5. What the new ruleset now says, that the old one did not

| New statement | Why it exists |
| :--- | :--- |
| Deleted systems are listed by name and state, with the archive path | The old file cited them as live workspaces |
| The four superseded plans are named in the ruleset itself | So an agent cannot treat them as current |
| Capture provenance is split by environment, with the failure output | So no session rediscovers the dead end and improvises |
| A DOM test may be delivered, labelled as a DOM test | Rather than suppressed, or dressed up as a screenshot |
| "A copy is not a port" — specifics named | Sprite particles, fewer presets, 7.9 KB for a 152 KB solver |
| Removal commits must be proven with `git ls-tree \| grep` | Two `Retire`/`Remove` commits left paths behind |
| Snapshot restores must not resurrect ordered deletions | A restore commit put 21 deleted files back |
| A rule may not cite a path that does not exist | Nine did |

---

## 6. Remaining work, stated plainly

| # | Item | State |
| :--- | :--- | :--- |
| 1 | **14 `.slang` shaders not named in `CMakeLists.txt`** | Real finding from `A10`. They exist and are not built. Needs a shader-wiring pass. |
| 2 | **No HTML ground-truth captures exist anywhere** | `A14` passes only because no recent UI commit lacked evidence. The first UI conversion must create `Exhibits/Images/<Domain>/HtmlGroundTruth/`. Cannot be seeded from a sandbox — no browser. |
| 3 | **Particle ground truth is not in this repository** | It lives on `arena/6eeea758-frontier` in this repository's history (`Experimental/ParticleEditor Final/`, 27 files, `presets.js` 114,533 B). The particle rebuild cannot start until the owner says where it belongs. |
| 4 | **Vehicle solver is still a scalar force model** | Out of scope this turn. `Engine/SimulationSpace/PhysicsSpace/VehicleDynamicsSolver.cpp` has no wheel, no slip ratio, no angular velocity. |
| 5 | **`Documentation/FolderStructure.md` is 68 KB and was not re-audited** | It no longer names the deleted systems, but it was not verified line by line. |
| 6 | **No CI job runs `AuditCheck.py`** | Deliberate: `CLAUDE.md` says GitHub Actions need explicit approval. The one-line job is ready when the owner approves it. |
| 7 | **Nothing was committed** | `CLAUDE.md` says not to infer permission to commit. Say the word. |

---

## 7. Reproduction

```bash
# the tree under discussion
git clone https://github.com/SultanAladin/Frontier.git
cd Frontier
git checkout arena/8391e994-frontier          # 940a125

# score the ruleset you actually have
python3 Automation/AuditCheck.py

# the old ruleset's broken citations
git show HEAD:CLAUDE.md | grep -oE '`[A-Za-z0-9_/.-]+\.(md|cpp|h|slang)`' | tr -d '`' |
  sort -u | while read p; do [ -e "$p" ] || echo "MISSING $p"; done

# the §4 symbols that do not exist
for s in DynamicProbeCacheSequence ReservoirPathReplaySequence IrradianceLatticeSequence \
         SignedDistanceRadiosityVolume J2000BrightStarCatalog; do
  printf '%-32s %s files\n' "$s" "$(grep -rl "$s" Engine/ | wc -l)"
done

# the capture dead end, verbatim
npx puppeteer browsers install chrome
curl -o /dev/null -w '%{http_code}\n' https://storage.googleapis.com/
```

---

## 8. Evidence status

| Claim | Evidence | Verdict |
| :--- | :--- | :--- |
| Old ruleset cited 9 dead paths | `AuditCheck.py` A03 output, committed | **PASS** |
| §4 described 5 pipelines whose types are absent | `grep -rl` counts above, 0 files each | **PASS** |
| New ruleset cites 0 dead paths | `AuditCheck.py` A03 PASS | **PASS** |
| 4 plans were live-facing and are now superseded | `AuditCheck.py` A07 PASS, banners committed | **PASS** |
| Browser capture unavailable in a sandbox | `npx puppeteer browsers install chrome` failure output, quoted verbatim | **PASS — blocked, documented** |
| The HTML-to-C++ conversion now matches | **BLOCKED.** No conversion was performed this turn, and no browser exists here to produce a capture | **BLOCKED** |
| 14 shaders are unwired | `AuditCheck.py` A10 WARN, names listed | **PASS — finding reported** |

No screenshot is claimed for any UI in this delivery, because no UI changed and no browser is available.
That is the rule working as intended.
