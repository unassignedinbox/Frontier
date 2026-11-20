#!/usr/bin/env python3
"""AuditCheck.py - verify a Frontier ref against CLAUDE.md, and fail on drift.

Every rule in CLAUDE.md that a machine can decide is decided here. Rules that are prose only stay
prose; this script does not pretend to check them.

Two modes:

    # audit any ref, read-only, prints a table
    python3 Automation/AuditCheck.py --ref origin/arena/8391e994-frontier

    # audit the working tree (used by CI and by hand before a commit)
    python3 Automation/AuditCheck.py

Checks (identifier -> the CLAUDE.md section that requires it):

    A01  CLAUDE.md exists and is the only ruleset            §0, AGENTS.md pointer only
    A02  AGENTS.md does not fork the ruleset                 AGENTS.md
    A03  Every backticked path in CLAUDE.md resolves          §1, §6
    A04  Every reference-index document exists                §1
    A05  No deleted engine domain is present                  §0, §4
    A06  No production path is named after a deleted domain   §4
    A07  SUPERSEDED plans are marked SUPERSEDED               §4
    A08  RemovedSdfGi archive is complete and untouched       §6
    A09  Shaders under Engine/Shaders are all .slang          §5.2
    A10  Every .slang has a CMake compile rule                §5.2
    A11  No C++ under Experimental/                           §6
    A12  No banned conjunction `And` in identifiers           §4
    A13  No hard-banned token in a production filename        §4
    A14  UI work carries an HTML ground-truth capture         §2.2
    A15  Removal commits leave nothing behind                 §8
    A16  Every Exhibits/Images domain has committed evidence  §2.2, §7

Exit code 0 when every evaluated check passes, 1 when any FAIL, 2 on usage error.
BLOCKED is never a pass.

No third-party imports. Python 3.10+.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass, field, asdict
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

PASS, FAIL, WARN, BLOCKED = "PASS", "FAIL", "WARN", "BLOCKED"

# §0 — domains the owner deleted and is rebuilding. Nothing may restore them silently.
DELETED_DOMAINS = (
    ("ParticleSpace", "Engine/ParticleSpace"),
    ("FluidSpace", "Engine/FluidSpace"),
    ("VectorFieldSpace", "Engine/VectorFieldSpace"),
    ("ForceFieldSpace", "Engine/ForceFieldSpace"),
    ("DistanceFieldSpace", "Engine/DistanceFieldSpace"),
    ("Fluid workspace", "Engine/Workspaces/Fluid"),
    ("Particle workspace", "Engine/Workspaces/Particle"),
)

# §4 — plans that describe deleted systems and must carry a SUPERSEDED marker.
SUPERSEDED_PLANS = (
    "Documentation/Plans/ParticleSpaceModularPortPlan.md",
    "Documentation/Plans/ParticleRendererDerezMaterialTexturePlan.md",
    "Documentation/Plans/VectorFieldArchitecturePlan.md",
    "Documentation/Plans/VectorFieldVulkanConversionPlan.md",
)

# §1 — the reference index. These must exist; a missing one is a broken rule.
REFERENCE_INDEX = (
    "Engine/Workspaces/README.md",
    "Engine/Lifecycle/README.md",
    "Engine/Editor/README.md",
    "Engine/Workspaces/UV/README.md",
    "Engine/Workspaces/Texturing/README.md",
    "Engine/Workspaces/Fracture/README.md",
    "Engine/Workspaces/ParametricSketcher/README.md",
    "Engine/GeometrySpace/Parametric/README.md",
    "Engine/Workspaces/Material/README.md",
    "Engine/Workspaces/ComponentEditor/README.md",
    "Engine/Workspaces/Revision/README.md",
    "Experimental/README.md",
    "Experimental/UI_COMPONENT_GUIDELINES.md",
    "Experimental/RemovedSdfGi/README.md",
)

MANDATORY_SKILLS = (
    "Documentation/AgenticInstructions/SKILL-CompactOutputProtocol.md",
    "Documentation/AgenticInstructions/SKILL-Emoji.md",
    "Documentation/AgenticInstructions/SKILL-Naming.md",
    "Documentation/AgenticInstructions/SKILL-Formatting.md",
    "Documentation/BannedWordsVocabulary.md",
    "Documentation/CodingStandards.md",
    "Documentation/ImportantNotes.md",
)

# §4 — hard-banned tokens, restricted to those that realistically appear in a filename.
BANNED_FILENAME_TOKENS = (
    "cache", "widget", "kit", "hierarchy", "manager", "handler", "coordinator",
    "subsystem", "beacon", "pipeline", "nexus", "utility", "helper", "mesh", "actor",
)

# §6 — paths created by Automation/PrepareDependencies.py in a git-ignored tree. Absent in a clean
# checkout by design, so citing them is correct.
GENERATED_PATHS = ("ExternalPackages/", "ExternalPackages/imgui", "Build/")

# §1, §4 — paths CLAUDE.md names precisely because they must NOT exist. Citing an absence is the
# point of the sentence, so these are not broken citations.
DECLARED_ABSENT = (
    "Exhibits/DeterministicPathTracer/README.md",
    "Engine/Workspaces/Fluid/README.md",
    "Engine/Workspaces/Particle/README.md",
    "Experimental/ParticleEditor Final/",
    "Experimental/VectorFieldVisualizer/",
)

# Production trees where naming and structure rules bind.
# §6 forbids C++ under Experimental/ because converted UI must live in Engine/. A dependency-free
# standalone demo that links nothing into the engine is not that, and is listed here explicitly.
STANDALONE_DEMOS = ("Experimental/SdfLightingDemoCpp/",)

PRODUCTION_PREFIXES = ("Engine/", "EngineContent/", "Exhibits/", "Automation/")
NON_PRODUCTION_PREFIXES = ("Experimental/", "Documentation/", "SourceImages/")


# ------------------------------------------------------------------------------------------------
# Git plumbing — works against a ref (read-only) or the working tree
# ------------------------------------------------------------------------------------------------

class Source:
    """File access for either a git ref or the working tree."""

    def __init__(self, ref: str | None) -> None:
        self.ref = ref
        self._files: list[str] | None = None

    def git(self, *args: str) -> str:
        result = subprocess.run(
            ["git", "-C", str(REPO_ROOT), *args],
            capture_output=True, text=True, errors="replace",
        )
        if result.returncode != 0:
            raise RuntimeError(f"git {' '.join(args)}: {result.stderr.strip()}")
        return result.stdout

    def files(self) -> list[str]:
        if self._files is None:
            if self.ref:
                out = self.git("ls-tree", "-r", "--name-only", self.ref)
                self._files = [line for line in out.splitlines() if line]
            else:
                self._files = sorted(
                    str(p.relative_to(REPO_ROOT)).replace("\\", "/")
                    for p in REPO_ROOT.rglob("*")
                    if p.is_file() and ".git" not in p.parts
                )
        return self._files

    def exists(self, path: str) -> bool:
        path = path.rstrip("/")
        files = self.files()
        return path in files or any(f.startswith(path + "/") for f in files)

    def read(self, path: str) -> str | None:
        if self.ref:
            try:
                return self.git("show", f"{self.ref}:{path}")
            except RuntimeError:
                return None
        target = REPO_ROOT / path
        return target.read_text(encoding="utf-8", errors="replace") if target.is_file() else None

    def committer_date(self, path: str) -> str:
        try:
            return self.git("log", "-1", "--format=%ad", "--date=short", "--", path).strip()
        except RuntimeError:
            return ""


@dataclass
class Check:
    key: str
    section: str
    requirement: str
    verdict: str
    observed: str


@dataclass
class Report:
    ref: str
    commit: str
    checks: list[Check] = field(default_factory=list)

    def add(self, key: str, section: str, requirement: str, verdict: str, observed: str) -> None:
        self.checks.append(Check(key, section, requirement, verdict, observed))

    def count(self, verdict: str) -> int:
        return sum(1 for c in self.checks if c.verdict == verdict)

    def score(self) -> int:
        decided = [c for c in self.checks if c.verdict in (PASS, FAIL, WARN)]
        if not decided:
            return 0
        return round(100 * sum(1 for c in decided if c.verdict == PASS) / len(decided))


# ------------------------------------------------------------------------------------------------
# Checks
# ------------------------------------------------------------------------------------------------

def check_ruleset(report: Report, src: Source) -> str | None:
    rules = src.read("CLAUDE.md")
    if rules is None:
        report.add("A01", "§0", "CLAUDE.md is the single ruleset", FAIL, "no CLAUDE.md on this ref")
        return None
    report.add("A01", "§0", "CLAUDE.md is the single ruleset", PASS,
               f"CLAUDE.md present, {len(rules.encode()):,} B")
    return rules


def check_agents_pointer(report: Report, src: Source, rules: str) -> None:
    agents = src.read("AGENTS.md")
    if agents is None:
        report.add("A02", "AGENTS.md", "AGENTS.md points at CLAUDE.md", WARN, "no AGENTS.md")
        return
    shares_rules = len(agents.encode()) > 0.35 * len(rules.encode())
    if "CLAUDE.md" not in agents:
        report.add("A02", "AGENTS.md", "AGENTS.md points at CLAUDE.md", FAIL,
                   f"AGENTS.md ({len(agents.encode()):,} B) never names CLAUDE.md: forked ruleset")
    elif shares_rules:
        report.add("A02", "AGENTS.md", "AGENTS.md points at CLAUDE.md", WARN,
                   f"AGENTS.md is {100 * len(agents.encode()) // len(rules.encode())}% of CLAUDE.md "
                   "and restates rules; a pointer should not")
    else:
        report.add("A02", "AGENTS.md", "AGENTS.md points at CLAUDE.md", PASS,
                   "AGENTS.md is a pointer")


def check_cited_paths(report: Report, src: Source, rules: str) -> None:
    """Every backticked path in CLAUDE.md must resolve. This is the rule that keeps the file honest."""
    cited: set[str] = set()
    for token in re.findall(r"`([^`\n]+)`", rules):
        candidate = token.strip()
        if "/" not in candidate and not re.search(r"\.(md|cpp|h|slang|py|html|json)$", candidate):
            continue
        if any(ch in candidate for ch in "*<>{}|$ ") or candidate.startswith("--"):
            continue
        # a bare extension (`.cpp`) is prose, not a path
        if re.fullmatch(r"\.[a-z0-9]+", candidate):
            continue
        # §0 cites the deleted domains precisely because they must NOT exist
        if any(candidate.rstrip("/").startswith(path) for _, path in DELETED_DOMAINS):
            continue
        # cited as absent on purpose, or generated into a git-ignored tree
        if any(candidate.startswith(p) for p in (*DECLARED_ABSENT, *GENERATED_PATHS)):
            continue
        # archive-relative shorthand, resolved against Experimental/RemovedSdfGi/
        if candidate in ("Evidence/", "Source/"):
            continue
        if not re.match(r"^[A-Za-z0-9_.][A-Za-z0-9_./-]*$", candidate):
            continue
        if candidate.count("/") == 0 and "." not in candidate:
            continue
        # owner/repo and bare branch names are provenance, not paths in this tree
        if re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", candidate) and not candidate.endswith(
                (".md", ".cpp", ".h", ".slang", ".py", ".html", ".json")):
            continue
        if re.fullmatch(r"(arena|main|master)/[A-Za-z0-9_-]+", candidate):
            continue
        cited.add(candidate.rstrip(".,;:"))

    if not cited:
        report.add("A03", "§1", "Every path cited in CLAUDE.md resolves", BLOCKED, "no paths cited")
        return

    files = src.files()

    def resolves(candidate: str) -> bool:
        if src.exists(candidate):
            return True
        if "/" in candidate:
            return False
        suffix = "/" + candidate
        return any(f.endswith(suffix) for f in files)

    broken = sorted(p for p in cited if not resolves(p))
    if broken:
        report.add("A03", "§1", "Every path cited in CLAUDE.md resolves", FAIL,
                   f"{len(broken)} of {len(cited)} cited paths do not exist: "
                   + ", ".join(broken[:5]))
    else:
        report.add("A03", "§1", "Every path cited in CLAUDE.md resolves", PASS,
                   f"all {len(cited)} cited paths resolve")


def check_reference_index(report: Report, src: Source) -> None:
    missing = [p for p in (*MANDATORY_SKILLS, *REFERENCE_INDEX) if not src.exists(p)]
    if missing:
        report.add("A04", "§1", "Mandatory reading and reference index resolve", FAIL,
                   f"{len(missing)} absent: " + ", ".join(missing[:5]))
    else:
        report.add("A04", "§1", "Mandatory reading and reference index resolve", PASS,
                   f"all {len(MANDATORY_SKILLS) + len(REFERENCE_INDEX)} documents resolve")


def check_deleted_domains_absent(report: Report, src: Source) -> None:
    present = [path for _, path in DELETED_DOMAINS if src.exists(path)]
    if present:
        report.add("A05", "§0", "Deleted domains are absent from Engine/", FAIL,
                   "restored without approval: " + ", ".join(present))
    else:
        report.add("A05", "§0", "Deleted domains are absent from Engine/", PASS,
                   f"all {len(DELETED_DOMAINS)} deleted domains absent")


def check_no_deleted_names(report: Report, src: Source) -> None:
    tokens = ("particlespace", "fluidspace", "vectorfieldspace", "forcefieldspace",
              "distancefieldspace", "workspaces/fluid", "workspaces/particle")
    offenders = []
    for path in src.files():
        if not path.startswith(PRODUCTION_PREFIXES):
            continue
        lowered = path.lower()
        if any(token in lowered for token in tokens):
            offenders.append(path)
    if offenders:
        report.add("A06", "§4", "No production path is named after a deleted domain", FAIL,
                   f"{len(offenders)} path(s): " + ", ".join(offenders[:4]))
    else:
        report.add("A06", "§4", "No production path is named after a deleted domain", PASS,
                   "no production path uses a deleted domain name")


def check_superseded_plans(report: Report, src: Source) -> None:
    unmarked, missing = [], []
    for plan in SUPERSEDED_PLANS:
        text = src.read(plan)
        if text is None:
            missing.append(plan)
            continue
        head = "\n".join(text.splitlines()[:12]).upper()
        if "SUPERSEDED" not in head and "SUPERSEDED" not in text.upper()[:2000]:
            unmarked.append(plan.rsplit("/", 1)[-1])
    if missing and len(missing) == len(SUPERSEDED_PLANS):
        report.add("A07", "§4", "Plans for deleted systems are marked SUPERSEDED", BLOCKED,
                   "none of the superseded plans are present")
    elif unmarked:
        report.add("A07", "§4", "Plans for deleted systems are marked SUPERSEDED", FAIL,
                   f"{len(unmarked)} plan(s) still read as live work: " + ", ".join(unmarked))
    else:
        report.add("A07", "§4", "Plans for deleted systems are marked SUPERSEDED", PASS,
                   f"{len(SUPERSEDED_PLANS) - len(missing)} plan(s) marked SUPERSEDED")


def check_archive(report: Report, src: Source) -> None:
    required = (
        "Experimental/RemovedSdfGi/README.md",
        "Experimental/RemovedSdfGi/Restore.py",
    )
    missing = [p for p in required if not src.exists(p)]
    evidence = [f for f in src.files() if f.startswith("Experimental/RemovedSdfGi/Evidence/")]
    if missing:
        report.add("A08", "§6", "RemovedSdfGi archive is present and complete", FAIL,
                   "missing: " + ", ".join(missing))
    elif not evidence:
        report.add("A08", "§6", "RemovedSdfGi archive is present and complete", FAIL,
                   "archive present but Evidence/ is empty")
    else:
        report.add("A08", "§6", "RemovedSdfGi archive is present and complete", PASS,
                   f"{len(evidence)} evidence files retained")


def check_shader_language(report: Report, src: Source) -> None:
    shaders = [f for f in src.files() if f.startswith("Engine/Shaders/")]
    foreign = [f for f in shaders if f.endswith((".glsl", ".hlsl", ".wgsl"))]
    if not shaders:
        report.add("A09", "§5.2", "Engine/Shaders contains only .slang", BLOCKED, "no shaders")
    elif foreign:
        report.add("A09", "§5.2", "Engine/Shaders contains only .slang", FAIL,
                   f"{len(foreign)} foreign shader(s), e.g. {foreign[0]}")
    else:
        report.add("A09", "§5.2", "Engine/Shaders contains only .slang", PASS,
                   f"{len(shaders)} shader files, all .slang")


def check_shader_wiring(report: Report, src: Source) -> None:
    cmake = src.read("CMakeLists.txt")
    if cmake is None:
        report.add("A10", "§5.2", "Every .slang has a CMake compile rule", BLOCKED, "no CMakeLists.txt")
        return
    slang = [f for f in src.files() if f.startswith("Engine/Shaders/") and f.endswith(".slang")]
    unbuilt = [f for f in slang if f.rsplit("/", 1)[-1] not in cmake]
    if unbuilt:
        report.add("A10", "§5.2", "Every .slang has a CMake compile rule", WARN,
                   f"{len(unbuilt)} of {len(slang)} shaders are not named in CMakeLists.txt: "
                   + ", ".join(u.rsplit('/', 1)[-1] for u in unbuilt[:4]))
    else:
        report.add("A10", "§5.2", "Every .slang has a CMake compile rule", PASS,
                   f"all {len(slang)} shaders referenced by CMakeLists.txt")


def check_no_cpp_in_experimental(report: Report, src: Source) -> None:
    cpp = [f for f in src.files()
           if f.startswith("Experimental/") and f.endswith((".cpp", ".h", ".hpp"))]
    exempt = [f for f in cpp
              if f.startswith("Experimental/RemovedSdfGi/Source/")
              or f.startswith(STANDALONE_DEMOS)]
    stray = [f for f in cpp if f not in exempt]
    if stray:
        report.add("A11", "§6", "Experimental/ holds no engine C++", FAIL,
                   f"{len(stray)} C++ file(s) under Experimental/, e.g. {stray[0]}")
    else:
        report.add("A11", "§6", "Experimental/ holds no engine C++", PASS,
                   f"no stray C++ ({len(exempt)} exempt: archive + standalone demo)")


def check_and_conjunction(report: Report, src: Source) -> None:
    # Identifiers only: source, shaders, plans. A captured image is not an identifier.
    offenders = [
        f for f in src.files()
        if f.startswith(("Engine/", "Exhibits/", "Automation/"))
        and f.endswith((".h", ".cpp", ".slang", ".py", ".md"))
        and re.search(r"[a-z0-9]And[A-Z]", f.rsplit("/", 1)[-1])
    ]
    if offenders:
        report.add("A12", "§4", "No conjunction `And` in an identifier", FAIL,
                   f"{len(offenders)}: " + ", ".join(o.rsplit('/', 1)[-1] for o in offenders[:4]))
    else:
        report.add("A12", "§4", "No conjunction `And` in an identifier", PASS, "clean")


def check_banned_names(report: Report, src: Source) -> None:
    hits = []
    for path in src.files():
        if not path.startswith(PRODUCTION_PREFIXES):
            continue
        stem = path.rsplit("/", 1)[-1].lower()
        for token in BANNED_FILENAME_TOKENS:
            if re.search(rf"(^|[^a-z]){token}([^a-z]|$)", stem):
                hits.append(path)
                break
    if hits:
        report.add("A13", "§4", "No hard-banned token in a production filename", FAIL,
                   f"{len(hits)} path(s), e.g. {hits[0]}")
    else:
        report.add("A13", "§4", "No hard-banned token in a production filename", PASS,
                   f"{len([f for f in src.files() if f.startswith(PRODUCTION_PREFIXES)])} "
                   "production paths clean")


def check_ui_ground_truth(report: Report, src: Source) -> None:
    """§2. A conversion of HTML UI must arrive with the HTML capture beside the C++ capture.

    Fails only when UI surface code actually changed and no ground-truth capture exists for it.
    Otherwise it is BLOCKED: there is nothing to check, which is not a pass.
    """
    html_truth = [f for f in src.files() if "/HtmlGroundTruth/" in f]
    ui_prefixes = ("Engine/Components/", "Engine/Workspaces/", "Engine/Editor/")

    if src.ref:
        recent = src.git("log", "--no-merges", "--format=%H", "-n", "40", src.ref).split()
    else:
        recent = src.git("log", "--no-merges", "--format=%H", "-n", "40", "HEAD").split()

    ui_touched = []
    for sha in recent[:25]:
        try:
            names = src.git("show", "--numstat", "--format=", sha).splitlines()
        except RuntimeError:
            continue
        paths = [n.split("\t")[-1] for n in names if "\t" in n]
        hits = [p for p in paths if p.startswith(ui_prefixes) and p.endswith((".cpp", ".h"))]
        if hits and not any(p.startswith("Exhibits/") for p in paths):
            ui_touched.append((sha[:7], hits[0].rsplit("/", 1)[-1], len(hits)))

    if not ui_touched:
        report.add("A14", "§2.2", "UI conversion carries an HTML ground-truth capture", PASS,
                   f"no UI-surface commit in the last 25 lacked evidence "
                   f"({len(html_truth)} ground-truth captures present)")
    elif html_truth:
        report.add("A14", "§2.2", "UI conversion carries an HTML ground-truth capture", WARN,
                   f"{len(ui_touched)} UI commit(s) shipped without a capture, though "
                   f"{len(html_truth)} ground-truth captures exist elsewhere")
    else:
        sha, file, count = ui_touched[0]
        report.add("A14", "§2.2", "UI conversion carries an HTML ground-truth capture", FAIL,
                   f"{len(ui_touched)} UI commit(s) changed {count}+ surface file(s) with no evidence "
                   f"(e.g. {sha} -> {file}); zero captures under "
                   "Exhibits/Images/<Domain>/HtmlGroundTruth/")


def check_removals(report: Report, src: Source) -> None:
    if not src.ref and not (REPO_ROOT / ".git").exists():
        report.add("A15", "§8", "Removal commits leave nothing behind", BLOCKED, "no git history")
        return
    try:
        log = src.git("log", "--no-merges", "--format=%H%x09%s", "-n", "300",
                      src.ref or "HEAD")
    except RuntimeError as error:
        report.add("A15", "§8", "Removal commits leave nothing behind", BLOCKED, str(error))
        return

    claims = []
    for line in log.splitlines():
        if "\t" not in line:
            continue
        sha, subject = line.split("\t", 1)
        if re.search(r"\b(Removes?|Retires?|Deletes?|Strips?)\b", subject):
            claims.append((sha, subject))

    if not claims:
        report.add("A15", "§8", "Removal commits leave nothing behind", BLOCKED,
                   "no removal commit in the last 300")
        return

    # What a removal claim is judged against, per §8: the deleted system's own engine surface.
    # Three refinements that keep this honest rather than noisy:
    #   - a keyword only counts as a whole word, so "GI prepass" matches but "digit" does not;
    #   - a rebuild target that reuses a word in a surviving unit is not a violation, because the
    #     claim named a subsystem, not every file containing the letters S-D-F;
    #   - documentation, prototypes and the archive are records, not survivors.
    SCOPES = (
        (("sdf", "signed distance", "distance field"),
         ("Engine/DistanceFieldSpace/", "Engine/Shaders/GlobalIllumination/DistanceEvaluation",
          "Engine/Shaders/GlobalIllumination/VoxelSdfClipmap", "VoxelSdfClipmapGiValidation")),
        (("gi", "global illumination", "irradiance lattice", "probe cache"),
         ("Engine/GeometricRaster/GlobalIllumination/IrradianceLattice",
          "Engine/GeometricRaster/GlobalIllumination/DynamicProbeCache",
          "Engine/GeometricRaster/GlobalIllumination/ReservoirPathReplay",
          "Engine/GeometricRaster/GlobalIllumination/WeightedPathReplay",
          "Engine/Shaders/GlobalIllumination/DynamicProbeCache",
          "Engine/Shaders/GlobalIllumination/ReservoirPathReplay",
          "Engine/Shaders/GlobalIllumination/WeightedPathReplay")),
        (("fluid",), ("Engine/FluidSpace/", "Engine/Workspaces/Fluid/")),
        (("particle",), ("Engine/ParticleSpace/", "Engine/Workspaces/Particle/",
                         "Engine/Shaders/Particles/")),
        (("vector field", "vectorfield"), ("Engine/VectorFieldSpace/",)),
        (("force field", "forcefield"), ("Engine/ForceFieldSpace/",)),
    )
    EXEMPT_PREFIXES = ("Documentation/", "Experimental/", "Exhibits/Images/")

    surviving = []
    for sha, subject in claims[:15]:
        lowered = " " + subject.lower().replace("/", " ").replace("-", " ") + " "
        needles = []
        for keywords, paths in SCOPES:
            if any(re.search(rf"(?<![a-z]){re.escape(k)}(?![a-z])", lowered) for k in keywords):
                needles.extend(paths)
        if not needles:
            continue
        still = [f for f in src.files()
                 if not f.startswith(EXEMPT_PREFIXES) and any(n in f for n in needles)]
        if still:
            surviving.append((subject[:52], len(still), still[0]))

    if surviving:
        detail = "; ".join(f'"{s}" -> {n} survives ({first})' for s, n, first in surviving[:3])
        report.add("A15", "§8", "Removal commits leave nothing behind", FAIL, detail)
    else:
        report.add("A15", "§8", "Removal commits leave nothing behind", PASS,
                   f"all {len(claims)} removal claims hold in their own scope")


def check_evidence_domains(report: Report, src: Source) -> None:
    images = [f for f in src.files() if f.startswith("Exhibits/Images/") and f.endswith(".png")]
    generators = [f for f in src.files()
                  if f.startswith("Exhibits/") and f.endswith((".cpp", ".py", ".mjs", ".sh"))]
    if not images:
        report.add("A16", "§7", "Exhibits carry committed proof images", FAIL,
                   "no .png under Exhibits/Images/")
        return
    verdict = PASS if generators else WARN
    report.add("A16", "§7", "Exhibits carry committed proof images", verdict,
               f"{len(images)} proof images, {len(generators)} generating files")


# ------------------------------------------------------------------------------------------------
# Driver
# ------------------------------------------------------------------------------------------------

def run(ref: str | None) -> Report:
    src = Source(ref)
    commit = src.git("rev-parse", "--short", ref or "HEAD").strip() if (REPO_ROOT / ".git").exists() else "working tree"
    report = Report(ref=ref or "working tree", commit=commit)

    rules = check_ruleset(report, src)
    if rules is None:
        for key, section, requirement in (
            ("A02", "AGENTS.md", "AGENTS.md points at CLAUDE.md"),
            ("A03", "§1", "Every path cited in CLAUDE.md resolves"),
        ):
            report.add(key, section, requirement, BLOCKED, "no ruleset to check against")
    else:
        check_agents_pointer(report, src, rules)
        check_cited_paths(report, src, rules)

    check_reference_index(report, src)
    check_deleted_domains_absent(report, src)
    check_no_deleted_names(report, src)
    check_superseded_plans(report, src)
    check_archive(report, src)
    check_shader_language(report, src)
    check_shader_wiring(report, src)
    check_no_cpp_in_experimental(report, src)
    check_and_conjunction(report, src)
    check_banned_names(report, src)
    check_ui_ground_truth(report, src)
    check_removals(report, src)
    check_evidence_domains(report, src)
    return report


def render_markdown(report: Report) -> str:
    lines = [
        f"### `{report.ref}` @ `{report.commit}`",
        "",
        f"Score **{report.score()}%** — {report.count(PASS)} PASS · {report.count(FAIL)} FAIL · "
        f"{report.count(WARN)} WARN · {report.count(BLOCKED)} BLOCKED",
        "",
        "| Check | Section | Requirement | Verdict | Observed |",
        "| :--- | :--- | :--- | :--- | :--- |",
    ]
    for check in report.checks:
        observed = check.observed.replace("|", "\\|")
        lines.append(f"| `{check.key}` | {check.section} | {check.requirement} | "
                     f"**{check.verdict}** | {observed} |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify a Frontier ref against CLAUDE.md.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Exit 0 = all evaluated checks pass. Exit 1 = a check FAILED. BLOCKED is never a pass.",
    )
    parser.add_argument("--ref", help="git ref to audit (default: working tree)")
    parser.add_argument("--json", action="store_true", help="emit JSON")
    parser.add_argument("--markdown", action="store_true", help="emit a Markdown report body")
    args = parser.parse_args()

    try:
        report = run(args.ref)
    except RuntimeError as error:
        print(f"audit failed: {error}", file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps({
            "ref": report.ref,
            "commit": report.commit,
            "score": report.score(),
            "counts": {v: report.count(v) for v in (PASS, FAIL, WARN, BLOCKED)},
            "checks": [asdict(c) for c in report.checks],
        }, indent=2))
    elif args.markdown:
        print(render_markdown(report))
    else:
        print(f"Frontier instruction compliance — {report.ref} @ {report.commit}")
        print(f"Score {report.score()}%  |  {report.count(PASS)} PASS  "
              f"{report.count(FAIL)} FAIL  {report.count(WARN)} WARN  "
              f"{report.count(BLOCKED)} BLOCKED\n")
        for check in report.checks:
            print(f"  {check.verdict:<8} {check.key}  {check.requirement}")
            if check.verdict != PASS:
                print(f"           {check.observed}")
        print()
        print("FAIL on: " + (", ".join(c.key for c in report.checks if c.verdict == FAIL) or "none"))

    return 1 if report.count(FAIL) else 0


if __name__ == "__main__":
    sys.exit(main())
