#=====================================================================================================================================
#                                                       WINDOWSZIGTOOLCHAIN.CMAKE
#=====================================================================================================================================
# 🧩 CMake toolchain for the Windows x64 cross-build from Linux: Zig's clang + MinGW-w64 headers and import libraries

# 📝 Port step S6. MSVC stays the Windows delivery compiler; this toolchain proves the Windows branches (Win32 API,
#    CreateProcessW, GLFW Win32, volk on vulkan-1.dll) compile and link on a Linux host, and produces the preview package.
#    The wrappers zig-cc / zig-c++ / zig-ar / zig-ranlib are written by CrossBuildWindows.sh into SOLIDARC_ZIG_TOOLS.

set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT DEFINED SOLIDARC_ZIG_TOOLS OR SOLIDARC_ZIG_TOOLS STREQUAL "")
    set(SOLIDARC_ZIG_TOOLS "$ENV{SOLIDARC_ZIG_TOOLS}")
endif()
set(ENV{SOLIDARC_ZIG_TOOLS} "${SOLIDARC_ZIG_TOOLS}")                                 # try_compile projects re-read the toolchain
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES SOLIDARC_ZIG_TOOLS)
if(NOT EXISTS "${SOLIDARC_ZIG_TOOLS}/zig-c++")
    message(FATAL_ERROR "SOLIDARC_ZIG_TOOLS does not hold the zig wrappers (run Experimental/SolidArc/Native/Tooling/CrossBuildWindows.sh)")
endif()

set(CMAKE_C_COMPILER   "${SOLIDARC_ZIG_TOOLS}/zig-cc")
set(CMAKE_CXX_COMPILER "${SOLIDARC_ZIG_TOOLS}/zig-c++")
set(CMAKE_AR           "${SOLIDARC_ZIG_TOOLS}/zig-ar"     CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB       "${SOLIDARC_ZIG_TOOLS}/zig-ranlib" CACHE FILEPATH "" FORCE)
set(CMAKE_RC_COMPILER  "${SOLIDARC_ZIG_TOOLS}/zig-rc")

# 📝 Static C++ runtime: the package then needs no libc++ / libunwind DLL beside the executables.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
