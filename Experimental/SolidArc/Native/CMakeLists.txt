#=====================================================================================================================================
#                                                            CMAKELISTS.TXT
#=====================================================================================================================================
# 🧩 SolidArc standalone build: the FrontierImGui and glfw targets from the same pins as Frontier, then SolidArc.cmake

cmake_minimum_required(VERSION 3.20)
project(SolidArc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# 📝 Identifiers use Greek / mathematical Unicode (SKILL-formatting § 4): MSVC needs /utf-8.
if(MSVC)
    add_compile_options(/utf-8 /EHsc)
endif()

#-------------------------------------------------------------------------------------------------------------------
#                                                    THIRD PARTY
#-------------------------------------------------------------------------------------------------------------------

get_filename_component(FRONTIER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(SOLIDARC_IMGUI "${FRONTIER_ROOT}/ExternalPackages/imgui")
if(NOT EXISTS "${SOLIDARC_IMGUI}/imgui.cpp")
    message(FATAL_ERROR "Missing ${SOLIDARC_IMGUI} — run: python Experimental/SolidArc/Native/Tooling/PrepareSolidArcDependencies.py")
endif()
add_library(FrontierImGui STATIC
    "${SOLIDARC_IMGUI}/imgui.cpp" "${SOLIDARC_IMGUI}/imgui_draw.cpp"
    "${SOLIDARC_IMGUI}/imgui_tables.cpp" "${SOLIDARC_IMGUI}/imgui_widgets.cpp")
target_include_directories(FrontierImGui SYSTEM PUBLIC "${SOLIDARC_IMGUI}")
target_compile_definitions(FrontierImGui PUBLIC "ImDrawIdx=unsigned int")

option(SOLIDARC_WINDOWED_HOST "Build the GLFW windowed host (Linux without X11 headers: GLFW null platform, compile coverage only)" ON)
if(SOLIDARC_WINDOWED_HOST)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    if(NOT WIN32 AND NOT APPLE)
        find_path(SOLIDARC_X11_HEADERS X11/Xlib.h)
        if(NOT SOLIDARC_X11_HEADERS)
            set(GLFW_BUILD_X11     OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        endif()
    endif()
    add_subdirectory("${FRONTIER_ROOT}/ExternalPackages/glfw" glfw EXCLUDE_FROM_ALL)
endif()

#-------------------------------------------------------------------------------------------------------------------
#                                                    SOLIDARC
#-------------------------------------------------------------------------------------------------------------------

include("${CMAKE_CURRENT_SOURCE_DIR}/SolidArc.cmake")
