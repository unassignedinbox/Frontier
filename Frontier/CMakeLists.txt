# Frontier engine — session & game mechanics (standalone slice).
#
# This library has no engine, renderer or GLFW dependency: it builds and validates anywhere a C++17
# compiler does. Define FRONTIER_WITH_GLFW and link glfw to enable InputEncoder::SampleGlfwWindow /
# InputDriver::DriveFromWindow against a live window.
#
#   cmake -S Frontier -B /tmp/frontier-build && cmake --build /tmp/frontier-build
#   /tmp/frontier-build/SessionValidation

cmake_minimum_required(VERSION 3.16)
project(FrontierSession CXX)

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

add_executable(SessionValidation ../Validation/SessionValidation.cpp)
target_link_libraries(SessionValidation PRIVATE FrontierSession)

add_library(FrontierGame STATIC Game/SoftwarePresentation.cpp)
target_link_libraries(FrontierGame PUBLIC FrontierSession)

add_executable(LobbySession Game/LobbySessionMain.cpp)
target_link_libraries(LobbySession PRIVATE FrontierGame FrontierSession)

add_library(FrontierPhase3 STATIC
    Engine/Diagnostics/LogCategories.cpp
    Engine/Diagnostics/LogRingStore.cpp
    Engine/Diagnostics/LogStream.cpp
    Engine/Rendering/BarrierDerivation.cpp
    Engine/Rendering/EmissiveLightSpace.cpp
    Engine/Rendering/EnvironmentUnits.cpp
    Engine/Rendering/HmiViewportUnits.cpp
    Engine/Rendering/IlluminationUnits.cpp
    Engine/Rendering/LightClusterSpace.cpp
    Engine/Rendering/NearestDepthSpace.cpp
    Engine/Rendering/Photometry.cpp
    Engine/Rendering/SubmissionOrder.cpp
    Engine/Rendering/TransientTargetAtlas.cpp
    Engine/VolumetricDynamics/PlumeSpace.cpp
    Engine/PhysicalDynamics/ParticleDynamics/VehicleEffectSpace.cpp)
target_include_directories(FrontierPhase3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(FrontierPhase3 PUBLIC cxx_std_20)
target_link_libraries(FrontierPhase3 PUBLIC FrontierGame FrontierSession)

add_executable(Phase3Validation ../Validation/Phase3Validation.cpp)
target_compile_features(Phase3Validation PRIVATE cxx_std_20)
target_link_libraries(Phase3Validation PRIVATE FrontierPhase3)

add_executable(Phase3RenderFluidParticleValidation ../Validation/Phase3RenderFluidParticleValidation.cpp)
target_compile_features(Phase3RenderFluidParticleValidation PRIVATE cxx_std_20)
target_link_libraries(Phase3RenderFluidParticleValidation PRIVATE FrontierPhase3)

