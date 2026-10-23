#!/usr/bin/env bash
#====================================================================================================================================
#                                                    PREPARELINUXBUILD.SH
#====================================================================================================================================
# 🧩 The README's Linux build in one idempotent script (every step skips when its output exists), run from the repository root:
#    bash Experimental/DraftingWorkspace/Tooling/PrepareLinuxBuild.sh [kernel] [workspace] [swiftshader] [slang] [exchange]
#    No arguments: kernel, workspace and swiftshader. Tools (cmake, ninja) come from a venv under _ClaudeScratch/.
#    exchange: trimesh and OpenCascade (cadquery-ocp) for Tooling/ExchangeCrossCheck.py, plus a no-op libGL.so.1 for hosts
#    without OpenGL (OCP links its viewer against it; STEP reading never calls it): LD_LIBRARY_PATH=_ClaudeScratch/stubs.
set -euo pipefail
cd "$(dirname "$0")/../../.."
Steps="${*:-kernel workspace swiftshader}"
Venv=_ClaudeScratch/.venv
mkdir -p _ClaudeScratch/tmp
if [ ! -x $Venv/bin/ninja ]; then python3 -m venv $Venv && $Venv/bin/pip install -q cmake ninja matplotlib ezdxf; fi
export PATH="$PWD/$Venv/bin:$PATH"

#------------------------------------------------------------------------------------------------------------------------
#                                                    KERNEL
#------------------------------------------------------------------------------------------------------------------------
if [[ " $Steps " == *" kernel "* ]]; then
    cmake -S Experimental/SolidArcRemastered -B _ClaudeScratch/build/kernel -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 > /dev/null
    ninja -C _ClaudeScratch/build/kernel -j2
fi

#------------------------------------------------------------------------------------------------------------------------
#                                                    WORKSPACE
#------------------------------------------------------------------------------------------------------------------------
if [[ " $Steps " == *" workspace "* ]]; then
    [ -d ExternalPackages/imgui ] || python3 Experimental/DraftingWorkspace/Tooling/PrepareDraftingDependencies.py
    cmake -S Experimental/DraftingWorkspace -B Experimental/DraftingWorkspace/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DDRAFTING_WINDOWED_HOST=OFF > /dev/null
    ninja -C Experimental/DraftingWorkspace/build -j2
fi

#------------------------------------------------------------------------------------------------------------------------
#                                                    SWIFTSHADER (headless Vulkan driver)
#------------------------------------------------------------------------------------------------------------------------
if [[ " $Steps " == *" swiftshader "* ]] && [ ! -f _ClaudeScratch/build/swiftshader/out/Linux/libvk_swiftshader.so ]; then
    [ -d _ClaudeScratch/build/swiftshader/.git ] || git clone -q https://github.com/google/swiftshader.git _ClaudeScratch/build/swiftshader
    git -C _ClaudeScratch/build/swiftshader checkout -q cddb5c91b23ff70a1f607ea31610aa0d7f4b5ad5
    cmake -S _ClaudeScratch/build/swiftshader -B _ClaudeScratch/build/swiftshader/out -G Ninja -DCMAKE_BUILD_TYPE=Release \
          -DREACTOR_BACKEND=Subzero -DSWIFTSHADER_BUILD_TESTS=OFF -DSWIFTSHADER_BUILD_BENCHMARKS=OFF -DSWIFTSHADER_BUILD_PVR=OFF \
          -DSWIFTSHADER_BUILD_WSI_XCB=OFF -DSWIFTSHADER_BUILD_WSI_WAYLAND=OFF -DSWIFTSHADER_WARNINGS_AS_ERRORS=OFF > /dev/null
    ninja -C _ClaudeScratch/build/swiftshader/out -j2 vk_swiftshader
    mkdir -p _ClaudeScratch/SwiftShaderDriver && cp _ClaudeScratch/build/swiftshader/out/Linux/libvk_swiftshader.so _ClaudeScratch/SwiftShaderDriver/
fi

#------------------------------------------------------------------------------------------------------------------------
#                                                    SLANG (only when a .slang file changes)
#------------------------------------------------------------------------------------------------------------------------
if [[ " $Steps " == *" slang "* ]]; then
    Loader=_ClaudeScratch/tmp/SlangCompiler/node_modules/slang-loader/vendor/slang-wasm.js
    [ -f $Loader ] || npm install -q --prefix _ClaudeScratch/tmp/SlangCompiler --ignore-scripts slang-loader@0.0.7
    node Experimental/DraftingWorkspace/Tooling/CompileDraftingShaders.mjs --compiler $Loader
fi
echo "PrepareLinuxBuild: done ($Steps)"

#------------------------------------------------------------------------------------------------------------------------
#                                                    EXCHANGE (independent STL / STEP readers)
#------------------------------------------------------------------------------------------------------------------------
if [[ " $Steps " == *" exchange "* ]]; then
    $Venv/bin/python -c "import trimesh, OCP" 2> /dev/null || $Venv/bin/pip install -q trimesh numpy networkx scipy matplotlib cadquery-ocp
    if [ ! -f _ClaudeScratch/stubs/libGL.so.1 ] && ! ldconfig -p 2> /dev/null | grep -q "libGL.so.1 "; then
        mkdir -p _ClaudeScratch/stubs
        Users=$(ls $Venv/lib/python3*/site-packages/OCP/*.so $Venv/lib/python3*/site-packages/cadquery_ocp.libs/libTKOpenGl*.so* 2> /dev/null)
        { echo "/* no-op OpenGL entry points for headless OCP */"; nm -D --undefined-only $Users | awk '{print $2}' | sed 's/@.*//' | grep -E "^gl" | sort -u | sed 's/.*/long &(void) { return 0; }/'; } > _ClaudeScratch/stubs/GlStub.c
        gcc -shared -fPIC -Wl,-soname,libGL.so.1 -o _ClaudeScratch/stubs/libGL.so.1 _ClaudeScratch/stubs/GlStub.c
    fi
fi
