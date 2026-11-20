#!/usr/bin/env python3
"""CheckInstructionCompliance.py - measure how far a Frontier ref drifts from its own CLAUDE.md.

The rulesets in this repository are prose. Nothing has ever executed them, so nothing has ever
failed. This script turns the machine-checkable subset of the instructions into pass/fail rows so
drift becomes visible on a branch instead of being discovered by the owner five days later.

Usage:
    python3 Automation/CheckInstructionCompliance.py                 # inspect HEAD
    python3 Automation/CheckInstructionCompliance.py origin/main     # inspect any ref
    python3 Automation/CheckInstructionCompliance.py ref1 ref2 ...   # compare several refs
    python3 Automation/CheckInstructionCompliance.py --json HEAD

Every check names the instruction it enforces. A check that cannot be evaluated (file absent on
that ref) reports BLOCKED, never PASS, per the ruleset's own "mark unavailable checks blocked" rule.

No third-party imports. Exits non-zero when any check on the first ref FAILS.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass, field

# ----------------------------------------------------------------------------------------------
# Constants taken from the rulesets themselves
# ----------------------------------------------------------------------------------------------

# SKILL-Naming.md hard-banned highlights, restricted to words that show up in file/folder names.
BANNED_NAME_TOKENS = (
    "cache", "caches", "widget", "kit", "footprint", "hierarchy", "pipeline", "subsystem",
    "manager", "handler", "coordinator", "controller", "utility", "helper", "mesh", "model",
    "actor", "beacon", "nexus", "blob", "temp", "tmp",
)

# SKILL-Emoji.md: only this set may appear in .h/.cpp/.slang/.md.
APPROVED_EMOJI = set("🧩📝💡⚠️🔴🐞🐛🚧🔍🚩✔️🟢🏳️🏴🏁🎌🏆🥇🥈🥉🏅🎖️⭐🏷️")

EMOJI_PATTERN = re.compile(
    "[" + "".join(re.escape(e) for e in sorted(APPROVED_EMOJI)) + "]"
)
ANY_EMOJI_PATTERN = re.compile(
    "[\U0001F300-\U0001FAFF\u2600-\u27BF\uFE0F\u2B00-\u2BFF]"
)

# Files the rulesets say MUST be read at session start, per ruleset generation.
CANONICAL_RULE_READING = (
    "EngineDocs/AgenticInstuctions/SKILL-Naming.md",
    "EngineDocs/AgenticInstuctions/SKILL-formatting.md",
    "EngineDocs/AgenticInstuctions/SKILL-Emoji.md",
    "EngineDocs/AgenticInstuctions/SKILL-CompactOutputProtocol.md",
    "Documentation/FolderStructure.md",
    "Documentation/Tracker.md",
)

# Words too generic to identify what a removal commit actually removed.
GENERIC_REMOVAL_WORDS = {
    "remove", "removes", "removed", "retire", "retires", "delete", "deletes", "strip", "strips",
    "unify", "unifies", "direct", "world", "render", "renderer", "rendering", "experimental",
    "documentation", "engine", "source", "studio", "plan", "plans", "integrate", "integrates",
    "html", "code", "page", "pages", "system", "systems", "subsystem", "subsystems", "old",
}

# Folder table asserted by the framework CLAUDE.md (engine layout, not the UI-playbook layout).
MANDATED_FOLDERS = ("Engine/", "Exhibits/", "Experimental/", "Documentation/")


# ----------------------------------------------------------------------------------------------
# Git plumbing
# ----------------------------------------------------------------------------------------------

def git(repo: str, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", repo, *args],
        capture_output=True,
        text=True,
        errors="replace",
    )
    if result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout


def tracked_files(repo: str, ref: str) -> list[str]:
    return [line for line in git(repo, "ls-tree", "-r", "--name-only", ref).splitlines() if line]


def show(repo: str, ref: str, path: str) -> str | None:
    try:
        return git(repo, "show", f"{ref}:{path}")
    except RuntimeError:
        return None


def exists(repo: str, ref: str, path: str, files: list[str]) -> bool:
    prefix = path.rstrip("/") + "/"
    return path in files or any(f.startswith(prefix) for f in files)


# ----------------------------------------------------------------------------------------------
# Result model
# ----------------------------------------------------------------------------------------------

PASS, FAIL, BLOCKED, WARN = "PASS", "FAIL", "BLOCKED", "WARN"


@dataclass
class Check:
    identifier: str
    instruction: str
    verdict: str
    observed: str


@dataclass
class RefReport:
    ref: str
    head: str
    checks: list[Check] = field(default_factory=list)

    def add(self, identifier: str, instruction: str, verdict: str, observed: str) -> None:
        self.checks.append(Check(identifier, instruction, verdict, observed))

    def count(self, verdict: str) -> int:
        return sum(1 for c in self.checks if c.verdict == verdict)

    def score(self) -> int:
        evaluated = [c for c in self.checks if c.verdict in (PASS, FAIL, WARN)]
        if not evaluated:
            return 0
        return round(100 * sum(1 for c in evaluated if c.verdict == PASS) / len(evaluated))


# ----------------------------------------------------------------------------------------------
# Checks
# ----------------------------------------------------------------------------------------------

def check_ruleset_present(report: RefReport, repo: str, files: list[str], claude: str | None) -> None:
    if claude is None:
        report.add(
            "C01", "CLAUDE.md is the authority and must exist on the branch", FAIL,
            "no CLAUDE.md on this ref; the session starts with no ruleset at all",
        )
    else:
        report.add(
            "C01", "CLAUDE.md is the authority and must exist on the branch", PASS,
            f"CLAUDE.md present, {len(claude.encode()):,} B, {claude.count(chr(10))} lines",
        )


def check_reading_resolves(report: RefReport, repo: str, ref: str, files: list[str], claude: str) -> None:
    """Every path the ruleset tells the agent to read must resolve on the same ref."""
    referenced = set()
    for match in re.findall(r"`([A-Za-z0-9_\-/]+\.md)`", claude):
        referenced.add(match)
    for match in re.findall(r"^- `?([A-Za-z0-9_\-/]+SKILL-[A-Za-z0-9\-]+\.md)`?", claude, re.M):
        referenced.add(match)

    mandatory = [p for p in sorted(referenced) if "SKILL-" in p or p in CANONICAL_RULE_READING]
    if not mandatory:
        report.add("C02", "Mandatory-reading paths resolve on the branch", BLOCKED,
                   "ruleset names no resolvable rule documents")
        return

    missing = [p for p in mandatory if not exists(repo, ref, p, files)]
    if missing:
        report.add(
            "C02", "Mandatory-reading paths resolve on the branch", FAIL,
            f"{len(missing)}/{len(mandatory)} mandated documents absent: " + ", ".join(missing[:4]),
        )
    else:
        report.add("C02", "Mandatory-reading paths resolve on the branch", PASS,
                   f"all {len(mandatory)} rule documents resolve")


def check_agents_agrees(report: RefReport, repo: str, ref: str, files: list[str], claude: str) -> None:
    agents = show(repo, ref, "AGENTS.md")
    if agents is None:
        report.add("C03", "AGENTS.md must not fork the ruleset", BLOCKED, "no AGENTS.md on this ref")
        return
    points_to_claude = "CLAUDE.md" in agents
    if not points_to_claude:
        report.add(
            "C03", "AGENTS.md must not fork the ruleset", FAIL,
            f"AGENTS.md ({len(agents.encode()):,} B) never references CLAUDE.md "
            f"({len(claude.encode()):,} B): two competing authorities",
        )
    elif len(agents.encode()) > 0.5 * len(claude.encode()):
        report.add(
            "C03", "AGENTS.md must not fork the ruleset", WARN,
            f"AGENTS.md is {100 * len(agents.encode()) // max(len(claude.encode()), 1)}% of CLAUDE.md "
            "while claiming to be a pointer; risk of duplicated, drifting rules",
        )
    else:
        report.add("C03", "AGENTS.md must not fork the ruleset", PASS,
                   "AGENTS.md is a pointer to CLAUDE.md")


def check_mandated_folders(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    absent = [d for d in MANDATED_FOLDERS if not exists(repo, ref, d, files)]
    if absent:
        report.add("C04", "Folder table in CLAUDE.md describes the branch", FAIL,
                   "mandated folders absent: " + ", ".join(absent))
    else:
        report.add("C04", "Folder table in CLAUDE.md describes the branch", PASS,
                   "all mandated top-level folders present")


def check_production_present(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    engine = [f for f in files if f.startswith("Engine/") and f.endswith((".h", ".cpp", ".slang"))]
    if not engine:
        report.add("C05", "A branch that claims engine work must contain engine source", FAIL,
                   "zero Engine/ translation units on this ref")
    else:
        report.add("C05", "A branch that claims engine work must contain engine source", PASS,
                   f"{len(engine):,} Engine/ translation units")


def check_banned_names(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    hits = []
    for path in files:
        lowered = path.lower()
        stem = lowered.rsplit("/", 1)[-1]
        for token in BANNED_NAME_TOKENS:
            if re.search(rf"(^|[^a-z]){token}([^a-z]|$)", stem):
                hits.append(path)
                break
    exempt = ("Documentation/Plans/", "Experimental/", "Exhibits/", "EngineDocs/", "SourceImages/")
    production = [h for h in hits if not h.startswith(exempt)]
    if production:
        report.add("C06", "SKILL-Naming banned vocabulary in file/folder names", FAIL,
                   f"{len(production)} production paths carry a banned token "
                   f"(e.g. {', '.join(p.rsplit('/', 1)[-1] for p in production[:3])})")
    elif hits:
        report.add("C06", "SKILL-Naming banned vocabulary in file/folder names", WARN,
                   f"{len(hits)} hits, all inside Experimental/ or Documentation/")
    else:
        report.add("C06", "SKILL-Naming banned vocabulary in file/folder names", PASS,
                   "no banned token in any tracked path")


def check_and_conjunction(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    offenders = [
        f for f in files
        if f.startswith(("Engine/", "UI/", "Frontier/"))
        and re.search(r"[a-z0-9]And[A-Z]", f.rsplit("/", 1)[-1])
    ]
    if offenders:
        report.add("C07", "Gate 7: conjunction `And` is forbidden in identifiers", FAIL,
                   f"{len(offenders)} paths: {', '.join(o.rsplit('/', 1)[-1] for o in offenders[:3])}")
    else:
        report.add("C07", "Gate 7: conjunction `And` is forbidden in identifiers", PASS,
                   "no `And` conjunction in production identifiers")


def check_shader_language(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    shaders = [f for f in files if "/Shaders/" in f or f.startswith("Engine/Shaders/")]
    foreign = [f for f in shaders if f.endswith((".glsl", ".hlsl", ".wgsl"))]
    if not shaders:
        report.add("C08", "Slang (.slang) is the primary shader language", BLOCKED,
                   "no shader directory on this ref")
    elif foreign:
        report.add("C08", "Slang (.slang) is the primary shader language", FAIL,
                   f"{len(foreign)} non-Slang shaders under Shaders/ (e.g. {foreign[0]})")
    else:
        report.add("C08", "Slang (.slang) is the primary shader language", PASS,
                   f"{len(shaders)} shader files, all .slang")


def check_emoji(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    scanned = [f for f in files if f.endswith((".h", ".cpp", ".slang"))]
    offenders = 0
    worst = ""
    for path in scanned:
        text = show(repo, ref, path)
        if not text:
            continue
        for match in ANY_EMOJI_PATTERN.finditer(text):
            if match.group() not in APPROVED_EMOJI and not match.group().isspace():
                offenders += 1
                worst = worst or path
    if offenders:
        report.add("C09", "SKILL-Emoji: only the approved set may appear in source", FAIL,
                   f"{offenders} unapproved emoji in source (e.g. {worst})")
    elif scanned:
        report.add("C09", "SKILL-Emoji: only the approved set may appear in source", PASS,
                   f"{len(scanned)} source files clean")
    else:
        report.add("C09", "SKILL-Emoji: only the approved set may appear in source", BLOCKED,
                   "no source files to scan")


def check_evidence(report: RefReport, repo: str, ref: str, files: list[str], head: str) -> None:
    """Visible evidence is mandatory with every delivery."""
    exhibits = [f for f in files if f.startswith("Exhibits/")]
    images = [f for f in exhibits if f.endswith((".png", ".jpg", ".webp"))]
    if not exhibits:
        report.add("C10", "Every delivery carries visible evidence under Exhibits/", FAIL,
                   "no Exhibits/ directory: no delivery on this ref has proof")
        return
    generators = [f for f in exhibits if f.endswith((".cpp", ".py", ".mjs", ".sh"))]
    if not generators:
        report.add("C10", "Every delivery carries visible evidence under Exhibits/", WARN,
                   f"{len(images)} images but no retained generating code")
        return
    report.add("C10", "Every delivery carries visible evidence under Exhibits/", PASS,
               f"{len(exhibits)} exhibit files, {len(images)} images, {len(generators)} generators")


def check_evidence_per_delivery(report: RefReport, repo: str, ref: str) -> None:
    """A commit that touches engine source should carry its evidence with it, not later."""
    log = git(repo, "log", "--no-merges", "--format=%H%x09%s", "-n", "40", ref)
    engine_only = 0
    total = 0
    offenders = []
    for line in log.splitlines():
        if "\t" not in line:
            continue
        sha, subject = line.split("\t", 1)
        names = git(repo, "show", "--numstat", "--format=", sha).splitlines()
        paths = [n.split("\t")[-1] for n in names if "\t" in n]
        if not paths:
            continue
        total += 1
        touches_engine = any(p.startswith(("Engine/", "UI/", "Frontier/")) for p in paths)
        touches_evidence = any(p.startswith("Exhibits/") for p in paths)
        if touches_engine and not touches_evidence:
            engine_only += 1
            offenders.append(subject[:52])
    if total == 0:
        report.add("C11", "Engine work is delivered together with its evidence", BLOCKED,
                   "no non-merge commits to inspect")
    elif engine_only:
        report.add("C11", "Engine work is delivered together with its evidence", FAIL,
                   f"{engine_only}/{total} of the last commits changed engine source with no "
                   f"Exhibits/ path (e.g. \"{offenders[0]}\")")
    else:
        report.add("C11", "Engine work is delivered together with its evidence", PASS,
                   f"all {total} recent deliveries carried evidence")


def check_removals_executed(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    """When a commit says Remove/Retire, the thing must actually be gone at the tip."""
    log = git(repo, "log", "--no-merges", "--format=%H%x09%s", "-n", "400", ref)
    claims = []
    for line in log.splitlines():
        if "\t" not in line:
            continue
        sha, subject = line.split("\t", 1)
        if re.search(r"\b(Remove|Removes|Retire|Retires|Delete|Deletes|Strip|Strips)\b", subject):
            claims.append((sha, subject))
    if not claims:
        report.add("C12", "Remove/Retire commits leave nothing behind", BLOCKED,
                   "no removal commit on this ref")
        return

    surviving = []
    for sha, subject in claims[:12]:
        words = re.findall(r"\b[A-Z][A-Za-z0-9]{3,}\b", subject)
        keywords = [w for w in words if w.lower() not in GENERIC_REMOVAL_WORDS]
        if not keywords:
            continue
        still = [f for f in files if any(k.lower() in f.lower() for k in keywords[:2])]
        if still:
            surviving.append((subject[:46], len(still)))

    if surviving:
        detail = "; ".join(f"\"{s}\" -> {n} paths survive" for s, n in surviving[:3])
        report.add("C12", "Remove/Retire commits leave nothing behind", FAIL, detail)
    else:
        report.add("C12", "Remove/Retire commits leave nothing behind", PASS,
                   f"all {len(claims)} removal claims hold at the tip")


def check_enforcement(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    """Rules with no executor are wishes."""
    workflows = [f for f in files if f.startswith(".github/workflows/")]
    hooked = False
    for path in workflows:
        text = show(repo, ref, path) or ""
        if re.search(r"banned|vocabulary|namingscan|naming|lint.*md|compliance", text, re.I):
            hooked = True
            break
    hooks = [f for f in files if "pre-commit" in f.lower() or "husky" in f.lower()]
    if hooked or hooks:
        report.add("C13", "Repository rules have an automated executor", PASS,
                   "a workflow or hook enforces at least one rule")
    elif workflows:
        report.add("C13", "Repository rules have an automated executor", FAIL,
                   f"{len(workflows)} CI workflows, none checks a rule; every rule is honour-system")
    else:
        report.add("C13", "Repository rules have an automated executor", FAIL,
                   "no CI workflow and no hook: every rule is honour-system")


def check_single_tracker(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    trackers = [f for f in files if re.search(r"(Tracker|Progress|Status|Backlog)[^/]*\.md$", f)]
    if not trackers:
        report.add("C14", "One tracker, not competing backlogs", BLOCKED, "no tracker on this ref")
    elif len(trackers) > 2:
        report.add("C14", "One tracker, not competing backlogs", FAIL,
                   f"{len(trackers)} parallel progress files: " + ", ".join(t.rsplit('/', 1)[-1] for t in trackers[:4]))
    else:
        report.add("C14", "One tracker, not competing backlogs", PASS,
                   f"{len(trackers)} tracker file(s)")


def check_prototype_present(report: RefReport, repo: str, ref: str, files: list[str]) -> None:
    """A 1:1 port is impossible if the thing being ported is not on the branch."""
    if not any(f.startswith("Engine/") for f in files):
        report.add("C15", "The prototype being ported exists on the porting branch", BLOCKED,
                   "no engine work on this ref")
        return
    prototype_roots = [f for f in files if f.startswith(("Experimental/", "Prototypes/", "Reference/"))]
    if not prototype_roots:
        report.add("C15", "The prototype being ported exists on the porting branch", FAIL,
                   "engine source present but no prototype tree: ports are re-derived from memory")
    else:
        report.add("C15", "The prototype being ported exists on the porting branch", PASS,
                   f"{len(prototype_roots)} prototype files available beside the engine")


# ----------------------------------------------------------------------------------------------
# Driver
# ----------------------------------------------------------------------------------------------

def audit(repo: str, ref: str) -> RefReport:
    head = git(repo, "rev-parse", "--short", ref).strip()
    files = tracked_files(repo, ref)
    claude = show(repo, ref, "CLAUDE.md")
    report = RefReport(ref=ref, head=head)

    check_ruleset_present(report, repo, files, claude)
    if claude is None:
        report.add("C02", "Mandatory-reading paths resolve on the branch", BLOCKED,
                   "no ruleset to resolve against")
        report.add("C03", "AGENTS.md must not fork the ruleset", BLOCKED, "no ruleset")
    else:
        check_reading_resolves(report, repo, ref, files, claude)
        check_agents_agrees(report, repo, ref, files, claude)

    check_mandated_folders(report, repo, ref, files)
    check_production_present(report, repo, ref, files)
    check_banned_names(report, repo, ref, files)
    check_and_conjunction(report, repo, ref, files)
    check_shader_language(report, repo, ref, files)
    check_emoji(report, repo, ref, files)
    check_evidence(report, repo, ref, files, head)
    check_evidence_per_delivery(report, repo, ref)
    check_removals_executed(report, repo, ref, files)
    check_enforcement(report, repo, ref, files)
    check_single_tracker(report, repo, ref, files)
    check_prototype_present(report, repo, ref, files)
    return report


def render(report: RefReport) -> str:
    lines = [
        f"## {report.ref} @ {report.head}",
        "",
        f"Score **{report.score()}%** | "
        f"{report.count(PASS)} PASS · {report.count(FAIL)} FAIL · "
        f"{report.count(WARN)} WARN · {report.count(BLOCKED)} BLOCKED",
        "",
        "| Check | Instruction | Verdict | Observed |",
        "| --- | --- | --- | --- |",
    ]
    for c in report.checks:
        observed = c.observed.replace("|", "\\|")
        lines.append(f"| `{c.identifier}` | {c.instruction} | {c.verdict} | {observed} |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit Frontier refs against their own CLAUDE.md.")
    parser.add_argument("refs", nargs="*", default=["HEAD"])
    parser.add_argument("--repo", default=".", help="repository path")
    parser.add_argument("--json", action="store_true", help="emit JSON instead of Markdown")
    args = parser.parse_args()

    reports = []
    for ref in args.refs or ["HEAD"]:
        try:
            reports.append(audit(args.repo, ref))
        except RuntimeError as error:
            print(f"cannot audit {ref}: {error}", file=sys.stderr)
            return 2

    if args.json:
        payload = [
            {
                "ref": r.ref,
                "head": r.head,
                "score": r.score(),
                "checks": [c.__dict__ for c in r.checks],
            }
            for r in reports
        ]
        print(json.dumps(payload, indent=2))
    else:
        print("# Frontier instruction compliance\n")
        for report in reports:
            print(render(report))
        print("## Summary\n")
        print("| Ref | Head | Score | FAIL | BLOCKED |")
        print("| --- | --- | --- | --- | --- |")
        for report in reports:
            print(f"| `{report.ref}` | `{report.head}` | {report.score()}% | "
                  f"{report.count(FAIL)} | {report.count(BLOCKED)} |")

    return 1 if reports[0].count(FAIL) else 0


if __name__ == "__main__":
    sys.exit(main())
