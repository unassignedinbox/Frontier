#!/usr/bin/env python3
# ===================================================================================================================================
#                                              RESTOREVEHICLEHMIRENDERER.PY
# ===================================================================================================================================
# 🧩 Restores the pinned VehicleHMI WebGPU renderer into the ignored ExternalPackages/ folder, then applies the lobby overlays.
#
# 📝 The renderer is not vendored: the pin (VehicleHmiRendererPin.json) names one upstream commit. The archive is extracted,
#    the overlay modules are copied in, and the patches are applied in order. Any patch that fails stops the restore.
#
# Usage: python3 Automation/RestoreVehicleHmiRenderer.py [--keep-clone]

import json
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

RepositoryRoot = Path(__file__).resolve().parents[1]


def RunCommand(Arguments, Working=None):
    Result = subprocess.run(Arguments, cwd=Working, capture_output=True, text=True)
    if Result.returncode != 0:
        raise RuntimeError(f"{' '.join(map(str, Arguments))} failed:\n{Result.stderr.strip()}")
    return Result.stdout


def RestoreArchive(Pin, Clone, Destination):
    if Destination.exists():
        shutil.rmtree(Destination)
    Destination.mkdir(parents=True)
    RunCommand(["git", "clone", "--filter=blob:none", "--no-checkout", "--quiet", Pin["repository"], str(Clone)])
    Archive = Clone.parent / "renderer-archive.tar"
    RunCommand(["git", "-C", str(Clone), "archive", "--format=tar", "-o", str(Archive), Pin["commit"], *Pin["paths"],
                *[f":(exclude){Excluded}" for Excluded in Pin["exclude"]]])
    with tarfile.open(Archive) as Tar:
        Tar.extractall(Destination)


def ApplyOverlays(Pin, Destination):
    for Source, Target in Pin["overlays"]:
        SourceFile = RepositoryRoot / Source
        Goal = Destination / Target
        Goal.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(SourceFile, Goal)


def ApplyPatches(Pin, RendererDirectory):
    for Patch in Pin["patches"]:
        RunCommand(["patch", "-p1", "--forward", "--batch", "-i", str(RepositoryRoot / Patch)], Working=RendererDirectory)


# 📝 node_modules is excluded from the archive. The renderer imports jolt-physics from it, so the lock file is installed here.
def InstallDependencies(RendererDirectory):
    RunCommand(["npm", "ci", "--no-audit", "--no-fund"], Working=RendererDirectory)


def Main():
    Pin = json.loads((RepositoryRoot / "Automation/VehicleHmiRendererPin.json").read_text(encoding="utf-8"))
    Restore = RepositoryRoot / Pin["restore_root"]
    Work = Path(tempfile.mkdtemp(prefix="vhm-renderer-"))
    try:
        RestoreArchive(Pin, Work / "clone", Restore)
        RendererDirectory = RepositoryRoot / Pin["renderer_directory"]
        ApplyOverlays(Pin, RendererDirectory)
        ApplyPatches(Pin, RendererDirectory)
        InstallDependencies(RendererDirectory)
    finally:
        if "--keep-clone" not in sys.argv:
            shutil.rmtree(Work, ignore_errors=True)
    print(f"Restored {Pin['commit'][:12]} into {Restore.relative_to(RepositoryRoot)} with {len(Pin['patches'])} patches.")


if __name__ == "__main__":
    Main()
