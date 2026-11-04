# Portal Projection Validation

| Check | Result |
|---|---|
| Front-to-back point crossing, bounds and reverse rejection | Pass |
| Proportional aperture mapping and motion round trip | Pass |
| Invalid quaternion, non-finite motion and aspect mismatch | Pass |

15 / 15 checks passed on Linux x86-64, GCC 12.2, AddressSanitizer and UndefinedBehaviorSanitizer, at `4e2ea03`
with the additional uncommitted rotated-aperture test.

```bash
g++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I. \
  Frontier/Extensions/Portal/PortalProjection.cpp Exhibits/Portal/Code/PortalProjectionValidation.cpp \
  -o _ClaudeScratch/build/PortalProjectionValidation
ASAN_OPTIONS=detect_leaks=1 _ClaudeScratch/build/PortalProjectionValidation
```

This is standalone math, not a working portal. Only matching-aspect-ratio rectangular apertures are supported. The
crossing is a point test, not an object's volume. There is no renderer view, partial-object clipping, split physics,
vehicle handling, record authoring or world transfer. Scaled mass, inertia, forces and energy remain to be designed.
