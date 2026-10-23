#=====================================================================================================================================
#                                                            CMAKELISTS.TXT
#=====================================================================================================================================
# 🧩 FrontierOrbitCamera — shared Z-up orbit camera (orbit / pan / dolly, presets, fit, Vulkan clip matrices, pixel rays)

# 📝 Production code shared by every application with a 3D viewport. Navigation schemes (SolidArc: Plasticity) live with
#    the application. Headers are included from the repository root: #include "Engine/Render/OrbitCamera.h".

cmake_minimum_required(VERSION 3.20)

get_filename_component(FRONTIER_RENDER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

#-------------------------------------------------------------------------------------------------------------------
#                                                      LIBRARY
#-------------------------------------------------------------------------------------------------------------------

add_library(FrontierOrbitCamera STATIC OrbitCamera.cpp)
target_include_directories(FrontierOrbitCamera PUBLIC "${FRONTIER_RENDER_ROOT}")
target_compile_features(FrontierOrbitCamera PUBLIC cxx_std_20)

# 📝 Comments use Unicode (×, ∓, …): MSVC needs /utf-8.
if(MSVC)
    target_compile_options(FrontierOrbitCamera PUBLIC /utf-8)
    target_compile_options(FrontierOrbitCamera PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(FrontierOrbitCamera PRIVATE -Wall -Wextra -Wpedantic)
endif()
