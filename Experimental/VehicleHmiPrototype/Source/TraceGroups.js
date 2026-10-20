//============================================================================================================================================
//                                                           TRACEGROUPS.JS
//============================================================================================================================================
// 🧩 The one descriptor group every ray-tracing and screen-tracing pass binds at slot 3: screen pyramids and surface
//    attributes first, then the two-level BVH, the shared materials and the light uniform.

// 📝 WebGPU's default device exposes four bind groups, and the screen traces and the path tracer both want slot 3. Their
//    bindings therefore share one layout: 0-3 are the screen slots (ScreenTraversal.wgsl), 4-7 the ray-tracing slots
//    (RayTraversal.wgsl). Every program that traces binds this same group, so the RTX and GTX paths cost one group switch.
export const ScreenTraceKinds = ["Texture2DUnfiltered", "Texture2DUnfiltered", "Texture2D", "Texture2D"];
export const RayTraceKinds    = ["StorageRead", "StorageRead", "StorageRead", "Uniform"];
export const TraceLayoutKinds = [...ScreenTraceKinds, ...RayTraceKinds];

// 📝 Per parity: the two nearest-depth pyramids (the current one first), the surface attributes and the previous frame's
//    diffuse exitance, then the BVH nodes, BVH triangles, materials and the light uniform. The per-parity pair is the only
//    part that changes between frames; the ray-tracing entries are the same objects for both parities.
export function ConstructTraceLayout(Exchange)
{
    return Exchange.ConstructDescriptorLayout(TraceLayoutKinds, GPUShaderStage.COMPUTE, "Trace traversal (screen + ray tracing)");
}

export function ConstructTraceGroups(Exchange, Layout, Screen, Surface, Display, RayTracing)
{
    return [0, 1].map((Parity) => Exchange.ConstructDescriptorGroup(Layout,
        [Screen.Views[Parity], Screen.Views[1 - Parity], Surface.Views.Attributes, Display.Views.Exitance, ...RayTracing.TraceEntries],
        `Trace traversal ${Parity}`));
}

// 📝 The direct pass needs only BVH visibility (TraceSceneVisibility reads the nodes and triangles, never the materials or
//    lights). Its layout therefore stops after the two BVH slots: WebGPU counts every storage binding of a pipeline's
//    layouts against the per-stage limit, and the direct pass already holds four. The slot numbers match RayTraversal.wgsl.
export const VisibilityTraceKinds = [...ScreenTraceKinds, "StorageRead", "StorageRead"];

export function ConstructVisibilityLayout(Exchange)
{
    return Exchange.ConstructDescriptorLayout(VisibilityTraceKinds, GPUShaderStage.COMPUTE, "Trace visibility (screen + BVH)");
}

export function ConstructVisibilityGroups(Exchange, Layout, Screen, Surface, Display, RayTracing)
{
    return [0, 1].map((Parity) => Exchange.ConstructDescriptorGroup(Layout,
        [Screen.Views[Parity], Screen.Views[1 - Parity], Surface.Views.Attributes, Display.Views.Exitance,
         RayTracing.TraceEntries[0], RayTracing.TraceEntries[1]],
        `Trace visibility ${Parity}`));
}
