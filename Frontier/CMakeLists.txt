# Frontier engine — session & game mechanics (standalone slice).
#
# The session/validation slice is C++17. The native ReSTIR path uses C++20 and Vulkan headers;
# FRONTIER_ENABLE_GLFW optionally fetches GLFW/OpenGL and adds the interactive EverestWindow;
# SampleGlfwWindow / DriveFromWindow are enabled only for that build.
#
#   cmake -S Frontier -B /tmp/frontier-build && cmake --build /tmp/frontier-build
#   /tmp/frontier-build/SessionValidation

cmake_minimum_required(VERSION 3.16)
project(FrontierSession CXX)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
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

add_executable(SessionValidation ../Validation/SessionValidation.cpp)
target_link_libraries(SessionValidation PRIVATE FrontierSession EverestNavigation)

# Native ReSTIR/Vulkan port imported from Frontier 118dc3e40ff7ccc6a1ceb42d5b4eca7601e437c5.
option(FRONTIER_ENABLE_RESTIR "Build the native Vulkan ReSTIR renderer" ON)
if(FRONTIER_ENABLE_RESTIR)
    include(FetchContent)
    FetchContent_Declare(vulkan_headers
        URL https://codeload.github.com/KhronosGroup/Vulkan-Headers/tar.gz/refs/tags/v1.4.321
        URL_HASH SHA256=7c1874f9186d9e7d5245e279cdccaa77a66ed8f52871b428630aeb771dc7623c)
    FetchContent_MakeAvailable(vulkan_headers)
    find_package(Threads REQUIRED)
    file(GLOB FRONTIER_GI_SOURCES CONFIGURE_DEPENDS Engine/Rendering/GlobalIllumination/*.cpp)
    file(GLOB FRONTIER_VIRTUAL_SOURCES CONFIGURE_DEPENDS Engine/Rendering/VirtualizedGeometry/*.cpp)
    add_library(FrontierGlobalIllumination STATIC Engine/DeviceExchange/DeviceExchange.cpp
        ${FRONTIER_GI_SOURCES} ${FRONTIER_VIRTUAL_SOURCES})
    target_include_directories(FrontierGlobalIllumination PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/Engine ${vulkan_headers_SOURCE_DIR}/include)
    target_compile_definitions(FrontierGlobalIllumination PUBLIC VK_NO_PROTOTYPES)
    target_compile_features(FrontierGlobalIllumination PUBLIC cxx_std_20)
    target_link_libraries(FrontierGlobalIllumination PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(FrontierGlobalIllumination PRIVATE -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(FrontierGlobalIllumination PRIVATE /utf-8 /fp:precise)
    endif()
endif()

add_library(FrontierCamera STATIC Game/CameraController.cpp)
target_link_libraries(FrontierCamera PUBLIC FrontierSession)
target_link_libraries(SessionValidation PRIVATE FrontierCamera)

add_library(FrontierGame STATIC Game/SoftwarePresentation.cpp)
target_link_libraries(FrontierGame PUBLIC FrontierSession FrontierCamera)

add_executable(LobbySession Game/LobbySessionMain.cpp)
target_link_libraries(LobbySession PRIVATE FrontierGame FrontierSession)

if(FRONTIER_ENABLE_RESTIR)
    add_library(FrontierRestirPresentation STATIC Game/RestirPresentation.cpp)
    target_link_libraries(FrontierRestirPresentation PUBLIC FrontierGlobalIllumination FrontierGame)
    add_executable(EverestRestir ../Projects/Everest/Source/Game/EverestRestirMain.cpp)
    target_link_libraries(EverestRestir PRIVATE FrontierRestirPresentation)
endif()

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
    find_package(OpenGL REQUIRED)

    target_compile_definitions(FrontierSession PUBLIC FRONTIER_WITH_GLFW)
    target_link_libraries(FrontierSession PUBLIC glfw)
    add_executable(EverestWindow ../Projects/Everest/Source/Game/EverestGlfwMain.cpp)
    target_link_libraries(EverestWindow PRIVATE FrontierGame FrontierSession EverestNavigation OpenGL::GL glfw)
    if(FRONTIER_ENABLE_RESTIR)
        target_link_libraries(EverestWindow PRIVATE FrontierRestirPresentation)
        target_compile_definitions(EverestWindow PRIVATE FRONTIER_WITH_RESTIR)
    endif()
endif()

if(BUILD_TESTING)
    add_test(NAME SessionValidation COMMAND SessionValidation)
endif()
