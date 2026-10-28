#!/usr/bin/env bash
#====================================================================================================================================
#                                                  RECOVERSOLIDARCBUILD.SH
#====================================================================================================================================
# 🧩 Rebuilds the whole scratch build after a sandbox reset (every step skips when its output exists), from the repository root:
#    bash Experimental/SolidArc/Native/Tooling/RecoverSolidArcBuild.sh
#    Upstream Frontier clone → overlay → UI dependencies → fonts → configure → first build → SwiftShader. Prints RECOVERDONE.
set -uo pipefail
cd "$(dirname "$0")/../../../.."
R=$PWD/_ClaudeScratch
mkdir -p $R/logs $R/tmp
Step() { echo "[$(date +%H:%M:%S)] $*"; }

#------------------------------------------------------------------------------------------------------------------------
#                                                    TOOLS
#------------------------------------------------------------------------------------------------------------------------
[ -x $R/.venv/bin/ninja ] || { python3 -m venv $R/.venv && $R/.venv/bin/pip install -q cmake ninja matplotlib ezdxf; }
$R/.venv/bin/python -c "import fontTools" 2> /dev/null || $R/.venv/bin/pip install -q fonttools==4.60.1
export PATH="$R/.venv/bin:$PATH"

#------------------------------------------------------------------------------------------------------------------------
#                                                    UPSTREAM FRONTIER
#------------------------------------------------------------------------------------------------------------------------
if [ ! -d $R/Upstream/.git ]; then
    Step "clone Upstream"
    git clone -q https://github.com/SultanAladin/Frontier.git $R/Upstream && git -C $R/Upstream checkout -q 18d988e
fi
Step "overlay"
python3 Experimental/SolidArc/Native/Tooling/OverlayFrontier.py --frontier $R/Upstream > $R/logs/overlay.log 2>&1 || { echo "overlay failed"; exit 1; }
Step "PrepareUI"                                                                          # After the overlay: else "Dependency not prepared: volk"
(cd $R/Upstream && $R/.venv/bin/python Automation/PrepareUI.py) > $R/logs/prepareui.log 2>&1 || { echo "PrepareUI failed"; tail -5 $R/logs/prepareui.log; exit 1; }
Step "PrepareFonts"
(cd $R/Upstream && GH_TOKEN=$(gh auth token) $R/.venv/bin/python Automation/PrepareFonts.py) > $R/logs/fonts.log 2>&1 || { echo "PrepareFonts failed"; tail -5 $R/logs/fonts.log; exit 1; }

#------------------------------------------------------------------------------------------------------------------------
#                                                    CONFIGURE AND BUILD
#------------------------------------------------------------------------------------------------------------------------
if [ ! -f $R/build/Upstream/build.ninja ]; then
    Step "configure"
    cmake -S $R/Upstream -B $R/build/Upstream -G Ninja -DCMAKE_BUILD_TYPE=Release -DFRONTIER_HOST_VERIFICATION=ON -DBUILD_TESTING=ON \
          -DGLFW_BUILD_WAYLAND=OFF -DGLFW_BUILD_X11=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DPython3_EXECUTABLE=$R/.venv/bin/python \
          > $R/logs/configure.log 2>&1 || { echo "configure failed"; tail -20 $R/logs/configure.log; exit 1; }
fi
Step "build"
ninja -C $R/build/Upstream -j2 SolidArcDocumentValidation SolidArcCapture > $R/logs/build.log 2>&1 || { echo "build failed"; grep -m5 "error" $R/logs/build.log; exit 1; }

#------------------------------------------------------------------------------------------------------------------------
#                                                    SWIFTSHADER (headless Vulkan for the walkthroughs)
#------------------------------------------------------------------------------------------------------------------------
Step "SwiftShader"
bash Experimental/SolidArc/Native/Tooling/PrepareLinuxBuild.sh swiftshader > $R/logs/swiftshader.log 2>&1 || { echo "SwiftShader failed"; tail -5 $R/logs/swiftshader.log; exit 1; }
Step "RECOVERDONE"
