# Geometry Collision Validation

## Result

| Check | Result |
|---|---|
| Actual Proving Ground jump ramp captured and added to Jolt as triangles | Pass |
| Ray contacts the real sloped face | Pass |
| Bounds-only baseline blocks the vehicle at x = 19.05 m | Pass |
| Same vehicle drives up and beyond the facet ramp; peak centre height 1.81 m | Pass |
| Edited placement follows; hidden and moving geometry excluded | Pass |
| Actual workspace button toggles on and off; Alt+C toggles on; fixed triangle draw commands emitted | Pass |
| Existing scene geometry / physics space tests | 27 / 27; 15 / 15 |

`GeometryCollisionValidation` passed 9 / 9 checks. Seven focused CTests passed on Linux x86-64, GCC 12.2, Release,
at `4e2ea03` with uncommitted outline retention, tests and CMake changes. The vehicle fixture isolates the ramp and a
single vehicle, starts it at x = 12 m and applies full throttle for 1,440 fixed steps. This is not an interactive drive.

[CollisionOutlineWorkspace.png](../Images/CollisionOutlineWorkspace.png) is rasterized from actual ImGui workspace draw
commands over a **synthetic solid viewport background**. It is not a scene GPU capture. The overlay is X-ray, not depth
tested; it only shows accepted fixed triangle colliders. It omits the ground, boxes, moving bodies and hulls, drops lines
with a behind-camera endpoint, and caps output at 60,000 segments with a truncation notice.

## Reproduction

After running `Automation/PrepareUI.py`, `Automation/PrepareVulkanCompute.py`, and `Automation/PrepareFonts.py`,
configure a host build (a recent CMake and Python fontTools are required):

```bash
cmake -S . -B _ClaudeScratch/full-build -DFRONTIER_HOST_VERIFICATION=ON -DCMAKE_BUILD_TYPE=Release \
  -DGLFW_BUILD_WAYLAND=OFF -DGLFW_BUILD_X11=OFF
cmake --build _ClaudeScratch/full-build --target GeometryCollisionValidation CollisionOutlineWorkspaceValidation \
  SceneGeometryValidation PhysicsSpaceValidation PortalProjectionValidation PhotonPacketInboxValidation TransportRetryValidation NetworkingSpaceValidation -j 3
ctest --test-dir _ClaudeScratch/full-build --output-on-failure \
  -R '^(GeometryCollisionValidation|CollisionOutlineWorkspaceValidation|SceneGeometryValidation|PortalProjectionValidation|PhotonPacketInboxValidation|TransportRetryValidation|NetworkingSpaceValidation)$'
mkdir -p _ClaudeScratch/physics-validation/{Results,Images}
_ClaudeScratch/full-build/PhysicsSpaceValidation _ClaudeScratch/physics-validation
```

The headless UI fixture generates `collision-outline-exhibits/CollisionOutlineWorkspace.png` in the build directory.

## Limits

Moving geometry remains box-shaped; moving groups can be held, fractures use bounds, fragmented facet sets and failed
captures use the existing fallback. The ground slab remains enabled. The narrow UI hides the button rather than overlap
other controls; Alt+C is still available when the viewport has focus or hover. Windows build/runtime, actual GPU
rendering, interactive vehicle driving and Internet play have not been validated.
