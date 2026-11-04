# Portal Clipping Validation

| Check | Result |
|---|---|
| Crossing triangle split into front and back triangles | Pass |
| Area, winding and interpolated normal/texture coordinates preserved | Pass |
| Front-only, back-only, coplanar and on-plane vertex | Pass |
| Rotated portal plane | Pass |
| Invalid and non-finite inputs leave output unchanged | Pass |

13 / 13 checks passed on Linux x86-64, GCC 12.2, AddressSanitizer and UndefinedBehaviorSanitizer at `ca37904`
plus the uncommitted clipping code and tests.

```bash
g++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I. \
  Frontier/Extensions/Portal/PortalClipping.cpp Exhibits/Portal/Code/PortalClippingValidation.cpp \
  -o _ClaudeScratch/build/PortalClippingValidation
ASAN_OPTIONS=detect_leaks=1 _ClaudeScratch/build/PortalClippingValidation
```

This is a CPU triangle/plane utility only. It clips against an **infinite plane**, not the aperture rectangle, and does not
produce cap faces, a closed collider, object halves in the renderer, an actual portal view or any split physics. The
caller must determine aperture overlap, transform the other half, manage vertex streams/materials, and implement ownership,
visibility and physics. Interpolated normals are not normalized by this utility.
