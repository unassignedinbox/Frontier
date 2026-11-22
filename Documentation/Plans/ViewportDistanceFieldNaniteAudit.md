# Viewport, Distance Field & Nanite Audit

**Date:** 2026-11-22
**Scope:** Editor camera rotation/range, direct-PBR ambient occlusion, stale GDF reads during transforms, cluster LOD behavior, gizmo interaction.
**Evidence:** Editor-profile build plus four targeted Vulkan/CPU validation suites.

## Findings and changes

| Symptom | Verified cause | Change in this pass | Status |
|---|---|---|---|
| RMB turn feels delayed; Shift+RMB still feels slow | The editor applied `CameraLagComponent` rotation damping (`16/s`) to every mouse-look update. The look gain was `0.0042 rad/pixel`. | Editor camera rotation lag is disabled while reusable camera lag remains available to other cameras. RMB yaw/pitch gain is now `0.0075 rad/pixel` (60 px ≈ 0.45 rad); Shift remains a translation sprint modifier, not a look-mode switch. | Fixed and covered by a Shift+RMB integration assertion. |
| Viewport loses scene geometry after moving far away | Primary scene rays were hard-limited to `120 m` in both the raster viewport and path-tracing entry. | Ray range now derives from camera-to-scene distance plus a conservative scene-bounds radius, with a `10,000,000 m` safety ceiling. | Fixed; ControlVehicle viewport validation now traces beyond 120 m. |
| SDF shadows/AO stay behind a moved object | The renderer uses a finite, two-cascade `64³`-per-cascade field centered on scene bounds, not a camera-following Unreal-style clipmap. Fast transform sync intentionally skipped the expensive geometry bake but left the previous field marked usable. | Track whether field geometry matches the uploaded BVH. Deferred transform sync gates the old field out of shader use, falling back to current BVH visibility/AO; the field becomes usable again only after the committed bake completes on the GPU timeline. | Stale field reads fixed. A continuously updated, object-local/scrolling clipmap is **not implemented**; committed edits still bake the finite field. |
| Direct PBR has dirty SDF AO despite the “zero SDF” contract | `OpenPbrDirect` was still using GDF cone AO, GDF fallbacks for shadows/area lights, and GDF glossy reflection. This contradicted the renderer’s documented zero-SDF mode. | Direct PBR no longer samples the GDF for AO, shadow fallback, area-light visibility, or glossy cones. XeGTAO is the optional PBR AO path; GTX GI retains GDF cone AO. | Fixed in the direct renderer path. Coarse-field quality remains a limitation of the GTX GI mode. |
| Nanite clusters/material debug unclear | The runtime is a cluster hierarchy, not one cluster per face. Clusters may contain up to 128 triangles, but each cluster is material-pure and each triangle keeps its source face/material slot. | Kept the existing per-asset `CoarsestAllowedLevel` cap and material-boundary locking; verified the ControlVehicle’s actual viewport cut at near/far distances. Existing channel 17 colours by LOD, channel 18 by cluster, and channel 6 by filled material slot. | ControlVehicle proof passes. One material can own many clusters; faces with one material are not forced into one shared cluster. |
| Gizmo drag is stolen by camera input | Existing input routing uses LMB for gizmo/selection only and falls back to analytic ray hover when GPU picking misses. | No new gizmo code was needed in this pass; retained that routing and re-ran its no-camera-drift/CPU-hover test. | Targeted integration test passes. |

## Field behavior, stated accurately

The current GDF is a baked finite volume around the scene bounds. It is not the camera-following global clipmap or incrementally updated object-local field requested. During an interactive transform, using the old volume produces incorrect shadows/AO; this pass instead marks it invalid and uses current triangle/BVH fallbacks. On the committed scene sync, the field is rebuilt and re-enabled only after successful GPU completion. This avoids displaying stale SDF results, but does not eliminate the final bake or provide a live SDF while dragging.

For clean direct-PBR AO, use **OpenPBR Direct** with the **XeGTAO** toggle enabled. Debug channels 12–16 remain available for distance, material, normal, raw AO, and cascade coverage; channels 17–18 show LOD level and cluster assignment.

## Verification

- Editor-profile build: `ProjectBrowserValidation`, `ClusterLodBakeValidation`, `GtxGlobalDistanceFieldViewportValidation`, and `NaniteViewportVehicleValidation` built successfully; modified Slang modules compiled.
- CTest: **4/4 passed** — `ProjectBrowserValidation`, `ClusterLodBakeValidation`, `GtxGlobalDistanceFieldViewportValidation`, `NaniteViewportVehicleValidation`.
- Camera integration: Shift+RMB moved the look direction by at least `0.30 rad` over 60 px; LMB did not move the camera; CPU gizmo hover resolved the translate handle.
- GDF integration: initial bake reports current, deferred transform sync gates the old field, and a committed rebuild restores current status.
- Nanite integration: ControlVehicle rendered with LOD enabled/disabled, material-filled faces, LOD-level colours, cluster colours, and a >120 m trace case.

## Proof files

- `Exhibits/Images/NaniteViewport/VehicleLodContactSheet.png` — near/far lit, face-material, LOD-level, and cluster views.
- `Exhibits/Images/NaniteViewport/VehicleBeyond120mClusters.png` — geometry still hit beyond the prior 120 m cutoff.
- `Exhibits/Images/GlobalIllumination/04-VulkanGtxGlobalDistanceFieldEngines.png` — renderer comparison after removing GDF from direct PBR.

## Remaining work

A true movable global-distance-field solution needs dirty-brick updates or per-asset local distance volumes with transform-time sampling, plus coverage/latency diagnostics. The finite full-scene bake should not be described as Unreal-equivalent. No claim is made that this remaining architecture is solved by invalidating stale data and falling back to BVH rays.
