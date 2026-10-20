#=====================================================================================================================================
#                                                        CMAKELISTS.TXT
#=====================================================================================================================================
# 🧩 DraftingWorkspace — Vulkan + Slang + ImGui modelling workspace over the SolidArcRemastered feature tree

cmake_minimum_required(VERSION 3.20)
project(DraftingWorkspace LANGUAGES C CXX)

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
#                                                    OPTIONS
#-------------------------------------------------------------------------------------------------------------------

get_filename_component(FRONTIER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(DRAFTING_PACKAGES "${FRONTIER_ROOT}/ExternalPackages" CACHE PATH "Pinned third-party sources (Tooling/PrepareDraftingDependencies.py)")
set(DRAFTING_SLANG_COMPILER "" CACHE FILEPATH "slang-wasm.js of slang-loader 0.0.7: regenerate the shader archive when the .slang files change")
option(DRAFTING_WINDOWED_HOST "Build the GLFW windowed host (Linux: GLFW null platform, compile coverage only)" ON)

foreach(Witness imgui/imgui.cpp tomlpp/include/toml++/toml.hpp glfw/include/GLFW/glfw3.h vulkan-headers/include/vulkan/vulkan.h stb/stb_image_write.h volk/volk.h)
    if(NOT EXISTS "${DRAFTING_PACKAGES}/${Witness}")
        message(FATAL_ERROR "Missing ${DRAFTING_PACKAGES}/${Witness} — run: python Experimental/DraftingWorkspace/Tooling/PrepareDraftingDependencies.py")
    endif()
endforeach()

#-------------------------------------------------------------------------------------------------------------------
#                                                    THIRD PARTY
#-------------------------------------------------------------------------------------------------------------------

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../SolidArcRemastered" SolidArcRemastered)

# 📝 volk: the Vulkan meta-loader. No link-time dependency on vulkan-1 / libvulkan; the loader (or a driver such as
#    SwiftShader) is resolved at run time.
add_library(DraftingVolk STATIC "${DRAFTING_PACKAGES}/volk/volk.c")
target_include_directories(DraftingVolk SYSTEM PUBLIC "${DRAFTING_PACKAGES}/volk" "${DRAFTING_PACKAGES}/vulkan-headers/include")
target_compile_definitions(DraftingVolk PUBLIC VK_NO_PROTOTYPES)
if(WIN32)
    target_compile_definitions(DraftingVolk PUBLIC VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_link_libraries(DraftingVolk PUBLIC ${CMAKE_DL_LIBS})
endif()

set(DRAFTING_IMGUI "${DRAFTING_PACKAGES}/imgui")
add_library(DraftingImGui STATIC
    "${DRAFTING_IMGUI}/imgui.cpp"
    "${DRAFTING_IMGUI}/imgui_draw.cpp"
    "${DRAFTING_IMGUI}/imgui_tables.cpp"
    "${DRAFTING_IMGUI}/imgui_widgets.cpp"
    "${DRAFTING_IMGUI}/backends/imgui_impl_vulkan.cpp"
)
target_include_directories(DraftingImGui SYSTEM PUBLIC "${DRAFTING_IMGUI}" "${DRAFTING_IMGUI}/backends")
target_compile_definitions(DraftingImGui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_link_libraries(DraftingImGui PUBLIC DraftingVolk)

add_library(DraftingTomlPlusPlus INTERFACE)
target_include_directories(DraftingTomlPlusPlus SYSTEM INTERFACE "${DRAFTING_PACKAGES}/tomlpp/include")

add_library(DraftingStb INTERFACE)
target_include_directories(DraftingStb SYSTEM INTERFACE "${DRAFTING_PACKAGES}/stb")

if(DRAFTING_WINDOWED_HOST)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    if(NOT WIN32 AND NOT APPLE)
        find_path(DRAFTING_X11_HEADERS X11/Xlib.h)
        if(NOT DRAFTING_X11_HEADERS)
            set(GLFW_BUILD_X11     OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        endif()
    endif()
    add_subdirectory("${DRAFTING_PACKAGES}/glfw" glfw EXCLUDE_FROM_ALL)
endif()

#-------------------------------------------------------------------------------------------------------------------
#                                                    SHADER ARCHIVE
#-------------------------------------------------------------------------------------------------------------------

# 📝 The SPIR-V archive is committed (Source/Viewport/Generated/DraftingShaderArchive.h) so a build needs no shader
#    compiler. With DRAFTING_SLANG_COMPILER set, editing a .slang file regenerates it through Node + slang-loader.
set(DRAFTING_SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/DraftingProjection.slang"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/DraftingSurface.slang"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/DraftingEdge.slang"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/DraftingGrid.slang"
)
set(DRAFTING_SHADER_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/Source/Viewport/Generated/DraftingShaderArchive.h")
if(DRAFTING_SLANG_COMPILER)
    find_program(DRAFTING_NODE node REQUIRED)
    add_custom_command(
        OUTPUT  "${DRAFTING_SHADER_ARCHIVE}"
        COMMAND "${DRAFTING_NODE}" "${CMAKE_CURRENT_SOURCE_DIR}/Tooling/CompileDraftingShaders.mjs" --compiler "${DRAFTING_SLANG_COMPILER}"
        DEPENDS ${DRAFTING_SHADERS} "${CMAKE_CURRENT_SOURCE_DIR}/Tooling/CompileDraftingShaders.mjs"
        COMMENT "Compiling DraftingWorkspace Slang shaders to SPIR-V"
        VERBATIM)
endif()
add_custom_target(DraftingShaderArchive DEPENDS "${DRAFTING_SHADER_ARCHIVE}")

#-------------------------------------------------------------------------------------------------------------------
#                                                    LIBRARY
#-------------------------------------------------------------------------------------------------------------------

add_library(DraftingWorkspace STATIC
    Source/Document/DraftingDocument.cpp
    Source/Document/DraftingDocumentCodec.cpp
    Source/Document/DraftingScene.cpp
    Source/Document/DraftingSpecimen.cpp
    Source/Viewport/DraftingCamera.cpp
    Source/Viewport/DraftingDevice.cpp
    Source/Viewport/DraftingRasterization.cpp
    Source/Viewport/DraftingPortableImage.cpp
    Source/Interface/DraftingInterface.cpp
    Source/Interface/DraftingFrame.cpp
    Source/Interface/DraftingOptions.cpp
    Source/Interface/DraftingScript.cpp
)
add_dependencies(DraftingWorkspace DraftingShaderArchive)
target_include_directories(DraftingWorkspace PUBLIC Source)
target_link_libraries(DraftingWorkspace PUBLIC SolidArcRemastered DraftingVolk DraftingImGui DraftingTomlPlusPlus DraftingStb)
target_compile_definitions(DraftingWorkspace PRIVATE DRAFTING_FONT_PATH="${FRONTIER_ROOT}/EngineContent/FontArchives/DM Sans/DMSans-Regular.ttf")
if(NOT MSVC)
    target_compile_options(DraftingWorkspace PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

#-------------------------------------------------------------------------------------------------------------------
#                                                    EXECUTABLES
#-------------------------------------------------------------------------------------------------------------------

add_executable(DraftingWorkspaceCapture Source/Applications/DraftingWorkspaceCapture.cpp)
target_link_libraries(DraftingWorkspaceCapture PRIVATE DraftingWorkspace)

if(DRAFTING_WINDOWED_HOST)
    add_executable(DraftingWorkspaceHost Source/Applications/DraftingWorkspaceHost.cpp "${DRAFTING_IMGUI}/backends/imgui_impl_glfw.cpp")
    target_compile_definitions(DraftingWorkspaceHost PRIVATE GLFW_INCLUDE_NONE)
    target_link_libraries(DraftingWorkspaceHost PRIVATE DraftingWorkspace glfw)
    if(WIN32)
        set_target_properties(DraftingWorkspaceHost PROPERTIES WIN32_EXECUTABLE ON)
        target_link_options(DraftingWorkspaceHost PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/ENTRY:mainCRTStartup>)
    endif()
endif()

add_executable(DraftingDocumentValidation Validation/DraftingDocumentValidation.cpp)
target_link_libraries(DraftingDocumentValidation PRIVATE DraftingWorkspace)

add_executable(DraftingViewportValidation Validation/DraftingViewportValidation.cpp)
target_link_libraries(DraftingViewportValidation PRIVATE DraftingWorkspace)

enable_testing()
add_test(NAME DraftingDocumentValidation COMMAND DraftingDocumentValidation)
add_test(NAME DraftingViewportValidation COMMAND DraftingViewportValidation)
set_tests_properties(DraftingViewportValidation PROPERTIES SKIP_RETURN_CODE 77)

# 📝 Interface walkthroughs: real ImGui input on the offscreen Vulkan frame (set DRAFTING_VULKAN_DRIVER for SwiftShader).
#    Run from the repository root because the scripts name _ClaudeScratch/DraftingWorkspace/… files; the recovery
#    walkthrough consumes the autosave the first one leaves behind.
set(DRAFTING_WALKTHROUGH_CAPTURES "${CMAKE_CURRENT_BINARY_DIR}/WalkthroughCaptures")
add_test(NAME DraftingWorkspaceWalkthrough
         COMMAND DraftingWorkspaceCapture --specimen --reset-files
                 --recovery _ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml
                 --document _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml
                 --script "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/WorkspaceWalkthrough.txt"
                 --exhibits "${DRAFTING_WALKTHROUGH_CAPTURES}"
         WORKING_DIRECTORY "${FRONTIER_ROOT}")
add_test(NAME DraftingRecoveryWalkthrough
         COMMAND DraftingWorkspaceCapture
                 --options "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/RecoveryOptions.toml"
                 --script "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/RecoveryWalkthrough.txt"
                 --exhibits "${DRAFTING_WALKTHROUGH_CAPTURES}"
         WORKING_DIRECTORY "${FRONTIER_ROOT}")
set_tests_properties(DraftingWorkspaceWalkthrough PROPERTIES SKIP_RETURN_CODE 77 FIXTURES_SETUP DraftingAutosave)
set_tests_properties(DraftingRecoveryWalkthrough PROPERTIES SKIP_RETURN_CODE 77 FIXTURES_REQUIRED DraftingAutosave)
