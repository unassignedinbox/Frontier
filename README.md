# Frontier

## Native smoke/fire preview

The optional GLFW window includes a C++ smoke/fluid inspector. Build it with
`cmake -S Frontier -B build -DFRONTIER_ENABLE_GLFW=ON -DCMAKE_BUILD_TYPE=Release`
and `cmake --build build --config Release`. Run `EverestWindow` from the repository
root so it can find the vehicle archives. Select Smoke, Fluid, Plume, or Fire in
the outliner, then select **Smoke plume** or **Fire** under the test emitter.
Right-drag over the viewport to orbit. Clear removes the current plume; an enabled
test emitter will immediately produce more.

This is a **CPU preview**, not a 1:1 port of the experimental WebGPU shaders or
HTML/CSS editor. It uses one coarser lattice and a software volume march rather
than the exhibit's GPU passes, lighting and temporal pipeline. The native world
currently supplies a ground plane and approximate vehicle box; other SDF geometry
can be supplied through `SmokeSdfWorld::SceneDistance`, but no full scene-distance
field is wired into the window. Tyre emission and ignition are not connected.
Unavailable appearance and blackbody controls, and ignition, are disabled in the
inspector. Presets retain those authored values for a future implementation.
The original shaders and editor remain under `Experimental/VehicleHmiPrototype`.

For the dependency-free validation build, omit `FRONTIER_ENABLE_GLFW` and run
`ctest --test-dir build --output-on-failure`.
