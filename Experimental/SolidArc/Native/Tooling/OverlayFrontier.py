#============================================================================================================================================
#                                                          OVERLAYFRONTIER.PY
#============================================================================================================================================
# 🧩 Seats SolidArc in a SultanAladin/Frontier checkout so it builds with the upstream CMake, pins, ImGui patches and fonts

# 📝 Port step S1. The SolidArc sources stay in this repository; the Frontier checkout receives links to them, one
#    include() line in its root CMakeLists.txt and a volk pin in Automation/UIDependencies.json. The same edits are the
#    ones the final pull request makes, so a green build here is a green build there.
#
#    Run:  python Experimental/SolidArc/Native/Tooling/OverlayFrontier.py --frontier <Frontier checkout> [--copy]
#          python <Frontier>/Automation/PrepareUI.py            (downloads volk next to imgui, glfw, …)
#          cmake -S <Frontier> -B <build> -G Ninja -DFRONTIER_HOST_VERIFICATION=ON -DBUILD_TESTING=ON   (non-Windows)
#
#    --copy copies instead of linking (Windows without developer mode). Rerunning is safe: present links are kept.

import argparse
import json
import os
import shutil
import sys
from pathlib import Path

#------------------------------------------------------------------------------------------------------------------------
#                                                    CONSTANTS
#------------------------------------------------------------------------------------------------------------------------

RepositoryRoot = Path(__file__).resolve().parents[4]

# 📝 Folders and files SolidArc owns in the target layout (Documentation/PLAN-SolidArcPort.md § Target layout).
SeatedPaths = [
    'Engine/GeometryProcessing',
    'Engine/Render/OrbitCamera.h',
    'Engine/Render/OrbitCamera.cpp',
    'UI/SolidArc',
    'Experimental/SolidArc',
    'Experimental/VehicleSurfacing',
    'Exhibits/SolidArc',
    'Exhibits/GeometryProcessing',
]

IncludeLine = 'include("${CMAKE_SOURCE_DIR}/Experimental/SolidArc/Native/SolidArc.cmake")'

VolkPin = {
    'name':       'volk',
    'repository': 'zeux/volk',
    'revision':   'a8da8ef3368482b0ee9b0ec0c6079a16a89c6924',
    'url':        'https://codeload.github.com/zeux/volk/tar.gz/a8da8ef3368482b0ee9b0ec0c6079a16a89c6924',
    'sha256':     '9ded768eb21498868b6c2e0ebd8006b658f40460202f8b80d827c848b4aa8ece',
    'proof':      True,
    'origin':     'SolidArc pin (Experimental/SolidArc/Native/Tooling/PrepareSolidArcDependencies.py)',
    'witness':    'volk.h',
}

#------------------------------------------------------------------------------------------------------------------------
#                                                    FUNCTIONS
#------------------------------------------------------------------------------------------------------------------------

def SeatPath(Frontier, Relative, CopyEnabled):
    Source = RepositoryRoot / Relative
    Target = Frontier / Relative
    if not Source.exists():
        raise RuntimeError('Missing in this repository: ' + Relative)
    if Target.is_symlink() and Target.resolve() == Source.resolve():
        return 'kept'
    if Target.exists() or Target.is_symlink():
        if not CopyEnabled:
            raise RuntimeError('Already present in the Frontier checkout (move it first): ' + str(Target))
        shutil.rmtree(Target) if Target.is_dir() and not Target.is_symlink() else Target.unlink()
    Target.parent.mkdir(parents=True, exist_ok=True)
    if CopyEnabled:
        Ignore = shutil.ignore_patterns('node_modules', '__pycache__')
        shutil.copytree(Source, Target, ignore=Ignore) if Source.is_dir() else shutil.copy2(Source, Target)
        return 'copied'
    os.symlink(Source, Target, target_is_directory=Source.is_dir())
    return 'linked'


def SeatInclude(Frontier):
    Lists = Frontier / 'CMakeLists.txt'
    Text  = Lists.read_text(encoding='utf-8')
    if IncludeLine in Text:
        return 'kept'
    Block = '\n# SolidArc: CAD workspace on Engine/GeometryProcessing (after FrontierImGui and glfw).\n' + IncludeLine + '\n'
    Lists.write_text(Text.rstrip('\n') + '\n' + Block, encoding='utf-8')
    return 'added'


def SeatVolk(Frontier):
    ManifestPath = Frontier / 'Automation/UIDependencies.json'
    Manifest     = json.loads(ManifestPath.read_text(encoding='utf-8'))
    if any(Package['name'] == 'volk' for Package in Manifest['packages']):
        return 'kept'
    Manifest['packages'].append(VolkPin)
    ManifestPath.write_text(json.dumps(Manifest, indent=2) + '\n', encoding='utf-8')
    return 'added'


def Main():
    Parser = argparse.ArgumentParser(description='Seat SolidArc in a Frontier checkout')
    Parser.add_argument('--frontier', required=True, help='SultanAladin/Frontier checkout')
    Parser.add_argument('--copy', action='store_true', help='copy instead of linking')
    Arguments = Parser.parse_args()
    Frontier  = Path(Arguments.frontier).resolve()
    if not (Frontier / 'Engine/Interface/ControlKit.h').exists():
        raise RuntimeError('Not a Frontier checkout: ' + str(Frontier))
    for Relative in SeatedPaths:
        print('%-8s %s' % (SeatPath(Frontier, Relative, Arguments.copy), Relative))
    print('%-8s %s' % (SeatInclude(Frontier), 'CMakeLists.txt include(SolidArc.cmake)'))
    print('%-8s %s' % (SeatVolk(Frontier), 'Automation/UIDependencies.json volk pin'))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(Main())
    except RuntimeError as Failure:
        print('OverlayFrontier: ' + str(Failure), file=sys.stderr)
        sys.exit(1)
