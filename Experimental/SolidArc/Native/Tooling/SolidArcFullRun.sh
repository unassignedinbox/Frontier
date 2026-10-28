#!/usr/bin/env bash
#====================================================================================================================================
#                                                    SOLIDARCFULLRUN.SH
#====================================================================================================================================
# 🧩 The whole evidence run, from the repository root (after RecoverSolidArcBuild.sh):
#    bash Experimental/SolidArc/Native/Tooling/SolidArcFullRun.sh [nowin]
#    overlay → full build → document validation (logs/docval.md) → every SolidArc test but the browser one (needs the upstream surface shader archive) (logs/ctest.log) → Windows package
#    (logs/win.log; skipped with nowin). One "<STEP> rc <code>" line each, then FULLDONE.
set -uo pipefail
cd "$(dirname "$0")/../../../.."
R=$PWD/_ClaudeScratch
export PATH="$R/.venv/bin:$PATH"
python3 Experimental/SolidArc/Native/Tooling/OverlayFrontier.py --frontier $R/Upstream > $R/logs/overlay.log 2>&1; echo "OVERLAY rc $?"
ninja -C $R/build/Upstream -j2 > $R/logs/build.log 2>&1; echo "BUILD rc $?"
(cd $R/Upstream && ../build/Upstream/SolidArcDocumentValidation > $R/logs/docval.md 2>&1); echo "DOCVAL rc $?"
(cd $R/build/Upstream && SOLIDARC_VULKAN_DRIVER=$R/SwiftShaderDriver/libvk_swiftshader.so $R/.venv/bin/ctest -R SolidArc -E SolidArcBrowserValidation --timeout 1500 > $R/logs/ctest.log 2>&1); echo "CTEST rc $?"
if [ "${1:-}" != "nowin" ]; then
    bash Experimental/SolidArc/Native/Tooling/CrossBuildWindows.sh $R/Upstream > $R/logs/win.log 2>&1; echo "WIN rc $?"
fi
echo FULLDONE
