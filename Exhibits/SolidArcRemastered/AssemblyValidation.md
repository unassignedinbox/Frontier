# Assembly validation

🟢 Priority 11 A1: joint origins, the seven Fusion joint kinds, Joint steps in the feature tree, assembly mass properties and interference (13.56 s)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | J1 | Joint origins read off geometry: circle edges (centre, axis out of the adjacent plane face), a line edge (midpoint, direction), plane faces (area centroid, outward normal), cylinder / cone sides (axis point level with the centroid), sphere and torus centres, a vertex — upright and tilted, against closed forms | ≤ 10⁻¹⁰ | 1.33e-15 | 14 | 0 |
| 🟢 | J2 | The seven Fusion joint kinds (rigid with offset and angle, revolute and flipped revolute, slider, cylindrical, pin-slot, planar, ball): every vertex of the placed block where the scalar closed form puts it, the re-measured moving origin on its target frame, volume kept, audit sound; a pin's circle edge onto a tilted cylinder axis | ≤ 10⁻¹² | 3.72e-15 | 9 | 0 |
| 🟢 | J3 | Joint steps in the feature tree: a two-link arm on a base boss (revolute θ₁, θ₂; the second link jointed first, so it must follow the first) — link centroids = (2,2) + 1.2(cos θ₁, sin θ₁) and (2,2) + 2.4(cos θ₁, sin θ₁) + 0.7(cos(θ₁+θ₂), sin(θ₁+θ₂)); editing θ₁ / θ₂ resumes at its joint and equals a full replay bit for bit; a rigid Transform of the base carries both links; removing a link drops its joints | ≤ 10⁻¹² · resumed step exact · 0 differing values | 2.82e-15 | 16 | 0 |
| 🟢 | J4 | Assembly mass properties with densities (steel block, aluminium cylinder, unit sphere): mass, centre of mass, inertia tensor about it by the parallel-axis theorem — against closed forms, and equal to the unit-density mass of their disjoint union | rel. ≤ 10⁻⁹ | 1.15e-15 | 4 | 0 |
| 🟢 | J5 | Interference: overlapping blocks (0.225), a pin r 0.3 pressed into a hole r 0.25 (π(r₁² − r₀²)·t), a cylinder through a block (πr²h), a sphere inside a block (contained, 4πr³/3), a pin r 0.2 in the hole (clear, gap 0.05), blocks 0.3 apart (clear, gap 0.3), blocks face to face (contact, no volume) | rel. ≤ 10⁻⁹ · verdict exact | 2.78e-16 | 7 | 0 |
| 🟢 | J6 | Refusals name the cause: joint to itself, unknown body, a second joint on a placed body, a joint closing a loop, angle and slide outside limits, an unresolvable joint origin, a NURBS face as joint origin, interference with itself, assembly mass without bodies or with a zero density — refused steps keep the previous bodies | named | 0.00e+00 | 11 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | J1 | Cylinder top circle edge | circle centre · origin / axis 0.00e+00 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Cylinder bottom circle edge (axis out of the bottom face) | circle centre · origin / axis 0.00e+00 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Cylinder side face | cylinder axis · origin / axis 1.33e-15 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Cylinder top face | plane centroid · origin / axis 2.22e-16 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Block top face | plane centroid · origin / axis 0.00e+00 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Block front face (y = 0) | plane centroid · origin / axis 9.04e-16 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Block bottom front line edge | line midpoint · origin / axis 0.00e+00 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Block line edge direction is ±x̂ | yes |
| 🟢 | J1 | Block corner vertex (world axes) | vertex · origin / axis 0.00e+00 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Tilted cylinder top circle edge | circle centre · origin / axis 1.11e-16 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Tilted cylinder side face | cylinder axis · origin / axis 3.14e-16 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Cone side face | cone axis · origin / axis 1.11e-16 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Sphere face | sphere centre · origin / axis 0.00e+00 · frame orthonormality 0.00e+00 |
| 🟢 | J1 | Torus face | torus centre · origin / axis 0.00e+00 · frame orthonormality 0.00e+00 |
| 🟢 | J2 | Rigid · offset 0.1, angle 30° | 8 vertices 2.63e-15 · landing frame 2.05e-15 · volume 1.19e-15 · audit sound |
| 🟢 | J2 | Revolute · 72° | 8 vertices 1.42e-15 · landing frame 8.01e-16 · volume 1.14e-15 · audit sound |
| 🟢 | J2 | Revolute · 72° flipped (origins aligned) | 8 vertices 8.88e-16 · landing frame 2.29e-16 · volume 1.47e-15 · audit sound |
| 🟢 | J2 | Slider · 0.35 along z | 8 vertices 4.97e-16 · landing frame 0.00e+00 · volume 1.22e-15 · audit sound |
| 🟢 | J2 | Cylindrical · 40°, −0.2 along z | 8 vertices 3.72e-15 · landing frame 1.35e-15 · volume 1.22e-15 · audit sound |
| 🟢 | J2 | Pin-slot · 15°, 0.6 along x | 8 vertices 2.39e-15 · landing frame 2.11e-15 · volume 1.69e-15 · audit sound |
| 🟢 | J2 | Planar · (0.4, −0.3), 110° | 8 vertices 2.39e-15 · landing frame 4.44e-16 · volume 1.55e-15 · audit sound |
| 🟢 | J2 | Ball · yaw 20°, pitch 35°, roll −50° | 8 vertices 3.15e-15 · landing frame 1.22e-15 · volume 2.78e-16 · audit sound |
| 🟢 | J2 | Revolute 25° · pin circle edge onto a tilted cylinder axis | centroid 1.09e-15 (closed form: axis point + 0.5·axis) · origin 1.09e-15 |
| 🟢 | J3 | Joint origins named on the regenerated part | Edge(Extrude#8.Cap(Start)\|Extrude#8.Side(Circle5)) → Edge(Extrude#6.Cap(End)\|Extrude#6.Side(Circle6)) · Edge(Extrude#6.Cap(Start)\|Extrude#6.Side(Circle5)) → Edge(Extrude#2.Cap(End)\|Extrude#4.Side(Circle1)) |
| 🟢 | J3 | θ₁ 0°, θ₂ 0° · link centroids | link 2 centroid (5.1, 2, 1.3) · link 1 / 2 vs closed form 1.57e-15 · 2 joints |
| 🟢 | J3 | θ₁ 30°, θ₂ 0° · link centroids | link 2 centroid (4.68467875173, 3.55, 1.3) · link 1 / 2 vs closed form 2.82e-15 · 2 joints |
| 🟢 | J3 | θ₁ 30°, θ₂ 0° · incremental replay | resumed at step 9 (expected 9) · 0 differing values vs a full replay |
| 🟢 | J3 | θ₁ 30°, θ₂ 45° · link centroids | link 2 centroid (4.25963430065, 3.8761480784, 1.3) · link 1 / 2 vs closed form 9.16e-16 · 2 joints |
| 🟢 | J3 | θ₁ 30°, θ₂ 45° · incremental replay | resumed at step 8 (expected 8) · 0 differing values vs a full replay |
| 🟢 | J3 | θ₁ 120°, θ₂ -60° · link centroids | link 2 centroid (1.15, 4.68467875173, 1.3) · link 1 / 2 vs closed form 2.67e-15 · 2 joints |
| 🟢 | J3 | θ₁ 120°, θ₂ -60° · incremental replay | resumed at step 8 (expected 8) · 0 differing values vs a full replay |
| 🟢 | J3 | θ₁ -90°, θ₂ -60° · link centroids | link 2 centroid (1.39378221735, -0.75, 1.3) · link 1 / 2 vs closed form 1.67e-15 · 2 joints |
| 🟢 | J3 | θ₁ -90°, θ₂ -60° · incremental replay | resumed at step 9 (expected 9) · 0 differing values vs a full replay |
| 🟢 | J3 | θ₁ -90°, θ₂ 180° · link centroids | link 2 centroid (2, 0.3, 1.3) · link 1 / 2 vs closed form 1.35e-15 · 2 joints |
| 🟢 | J3 | θ₁ -90°, θ₂ 180° · incremental replay | resumed at step 8 (expected 8) · 0 differing values vs a full replay |
| 🟢 | J3 | θ₁ 45°, θ₂ 45° · link centroids | link 2 centroid (3.69705627485, 4.39705627485, 1.3) · link 1 / 2 vs closed form 9.16e-16 · 2 joints |
| 🟢 | J3 | θ₁ 45°, θ₂ 45° · incremental replay | resumed at step 8 (expected 8) · 0 differing values vs a full replay |
| 🟢 | J3 | Transform of the base (90° about z, then +5 x) · both links follow | link centroids vs the mapped ones 8.01e-16 · joint origins still coincide 1.78e-15 |
| 🟢 | J3 | Remove the first link · its joints are dropped | 0 joints left · second link moved 0.00e+00 |
| 🟢 | J6 | Joint a body to itself | "Joint: Extrude#6 cannot be jointed to itself" · 3 bodies kept |
| 🟢 | J6 | Joint to an unknown body | "Joint: no body named Extrude#99" · 3 bodies kept |
| 🟢 | J6 | A second joint on a placed body | "Joint: Extrude#8 is already jointed to Extrude#6 (🚧 one joint per placed body)" · 3 bodies kept |
| 🟢 | J6 | A joint that closes a loop (base onto the second link) | "Joint: Extrude#8 is already placed relative to Extrude#2, so this joint would close a loop (🚧 loop solving)" · 3 bodies kept |
| 🟢 | J6 | Angle 120° outside limits [0°, 90°] | "Joint: Revolute angle 120° is outside its limits [0°, 90°]" · 3 bodies kept |
| 🟢 | J6 | Slide 0.8 outside limits [0, 0.5] | "Joint: Slider slide 0.8 is outside its limits [0, 0.5]" · 3 bodies kept |
| 🟢 | J6 | An unresolvable joint origin | "Joint: reference Edge(Nowhere\|Nothing) not found (no signature to rebind)" · 3 bodies kept |
| 🟢 | J4 | Mass ρ₁abc + ρ₂πr²h + ρ₃·4πR³/3 | 9.13700579042 vs 9.13700579042 → 1.94e-16 |
| 🟢 | J4 | Centre of mass Σmᵢcᵢ / M | (1.1933480518, 0.601487670343, 0.313866746808) → 6.36e-17 |
| 🟢 | J4 | Inertia about the centre of mass (parallel axis, products included) | Ixx 4.62506512731 · Ixy 2.63339783967 → 1.15e-15 |
| 🟢 | J4 | Unit densities = the disjoint union (3 lumps) | volume, centroid, inertia → 4.49e-16 |
| 🟢 | J5 | Overlapping unit blocks | Overlap (expected Overlap) · V 0.225 vs 0.225 · gap 0 vs 0 → 1.39e-16 · 218 ms |
| 🟢 | J5 | Pin r 0.3 pressed into the hole r 0.25 (t 0.5) | Overlap (expected Overlap) · V 0.0431968989869 vs 0.0431968989869 · gap 0 vs 0 → 2.78e-17 · 860 ms |
| 🟢 | J5 | Pin r 0.2 in the hole r 0.25 | Clear (expected Clear) · V 0 vs 0 · gap 0.05 vs 0.05 → 1.53e-16 · 1529 ms |
| 🟢 | J5 | Cylinder r 0.3 through a block (h 1) | Overlap (expected Overlap) · V 0.282743338823 vs 0.282743338823 · gap 0 vs 0 → 2.78e-16 · 817 ms |
| 🟢 | J5 | Sphere r 0.2 inside a unit block | Contained (expected Contained) · V 0.0335103216383 vs 0.0335103216383 · gap 0 vs 0 → 4.16e-17 · 423 ms |
| 🟢 | J5 | Blocks 0.3 apart | Clear (expected Clear) · V 0 vs 0 · gap 0.3 vs 0.3 → 5.55e-17 · 2873 ms |
| 🟢 | J5 | Block resting on a block (face to face) | Contact (expected Contact) · V 0 vs 0 · gap 0 vs 0 → 0.00e+00 · 681 ms |
| 🟢 | J6 | Interference of a body with itself | "Interference: a body cannot interfere with itself" |
| 🟢 | J6 | Assembly mass without bodies | "Assembly mass: no bodies" |
| 🟢 | J6 | Assembly mass with a zero density | "Assembly mass: densities must be positive" |
| 🟢 | J6 | A NURBS face as joint origin | "Joint: a Nurbs face has no joint origin (🚧 analytic faces only)" |
