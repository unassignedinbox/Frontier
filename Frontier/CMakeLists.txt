# Frontier engine — session & game mechanics (standalone slice).
#
# The standalone session/validation build has no renderer or GLFW dependency and works with a C++17
# compiler. FRONTIER_ENABLE_GLFW optionally fetches GLFW/OpenGL and adds the interactive EverestWindow;
# SampleGlfwWindow / DriveFromWindow are enabled only for that build.
#
#   cmake -S Frontier -B /tmp/frontier-build && cmake --build /tmp/frontier-build
#   /tmp/frontier-build/SessionValidation

cmake_minimum_required(VERSION 3.16)
project(FrontierSession CXX)
include(CTest)
option(FRONTIER_ENABLE_GLFW "Build the interactive GLFW/OpenGL Everest window" OFF)

add_library(FrontierSession STATIC
    Engine/Input/InputEncoder.cpp
    Engine/Input/InputDriver.cpp
    Engine/Input/SyntheticDriver.cpp
    Engine/Scene/DeployableUnit.cpp
    Engine/Scene/FacetArchive.cpp
    Engine/Session/SpawnPolicy.cpp
    Engine/Session/SessionConfiguration.cpp
    Engine/Session/SessionPolicy.cpp
    Engine/Session/SessionSpace.cpp
    Engine/Session/TransferConfiguration.cpp
    Engine/Session/LevelTransition.cpp
    Engine/Session/LobbySession.cpp
    ../Projects/Everest/Source/Session/SessionPolicy.cpp)
target_include_directories(FrontierSession PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(FrontierSession PUBLIC cxx_std_17)

add_library(EverestNavigation STATIC ../Projects/Everest/Source/Navigation/NavigationApp.cpp)
target_link_libraries(EverestNavigation PUBLIC FrontierSession)

add_library(FrontierSmoke STATIC Engine/Effects/SmokeFluidSystem.cpp)
target_link_libraries(FrontierSmoke PUBLIC FrontierSession)

add_executable(SessionValidation ../Validation/SessionValidation.cpp)
target_link_libraries(SessionValidation PRIVATE FrontierSession EverestNavigation FrontierSmoke)

add_library(FrontierCamera STATIC Game/CameraController.cpp)
target_link_libraries(FrontierCamera PUBLIC FrontierSession)
target_link_libraries(SessionValidation PRIVATE FrontierCamera)

add_library(FrontierGame STATIC Game/SoftwarePresentation.cpp)
target_link_libraries(FrontierGame PUBLIC FrontierSession FrontierCamera FrontierSmoke)
target_link_libraries(SessionValidation PRIVATE FrontierGame)

add_executable(LobbySession Game/LobbySessionMain.cpp)
target_link_libraries(LobbySession PRIVATE FrontierGame FrontierSession)

add_executable(Everest ../Projects/Everest/Source/Game/EverestMain.cpp)
target_link_libraries(Everest PRIVATE FrontierGame FrontierSession EverestNavigation)

if(FRONTIER_ENABLE_GLFW)
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz)
    FetchContent_MakeAvailable(glfw)
    FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.9b GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(imgui)
    find_package(OpenGL REQUIRED)

    add_library(FrontierImGui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp)
    target_include_directories(FrontierImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(FrontierImGui PUBLIC glfw OpenGL::GL)

    target_compile_definitions(FrontierSession PUBLIC FRONTIER_WITH_GLFW)
    target_link_libraries(FrontierSession PUBLIC glfw)
    add_executable(EverestWindow
        ../Projects/Everest/Source/Game/EverestGlfwMain.cpp
        ../Projects/Everest/Source/Game/SmokeFireEditor.cpp)
    target_link_libraries(EverestWindow PRIVATE FrontierGame FrontierSession FrontierSmoke EverestNavigation FrontierImGui OpenGL::GL glfw)
endif()

if(BUILD_TESTING)
    add_test(NAME SessionValidation COMMAND SessionValidation)
endif()
