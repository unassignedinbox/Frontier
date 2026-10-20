/*====================================================================================================================================
                                                    DRAFTINGPROJECTION.SLANG
====================================================================================================================================*/
// 🧩 Shared push-constant ABI, selection-state bits and palette of the DraftingWorkspace viewport programs

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUSH CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 128 bytes (the Vulkan guaranteed minimum). Mirrored by DraftingProjectionBlock in DraftingRasterization.h; the
//    compile script asserts the reflected offsets. ViewRight.w = 1 for orthographic projection.
struct DraftingProjection
{
    column_major float4x4 ClipFromWorld;                                                     // [-]  - Projection × view
    float4                ViewRight;                                                         // [-]  - Camera right (world), w: orthographic flag
    float4                ViewUp;                                                            // [-]  - Camera up (world)
    float4                ViewForward;                                                       // [-]  - Eye → target (world), w: grid spacing [len]
    float4                DisplayStroke;                                                     // [px] - Width, height, edge stroke, edge depth bias
};

// 📝 Grid program: needs the inverse to unproject each pixel onto the ground plane z = 0.
struct DraftingGridProjection
{
    column_major float4x4 ClipFromWorld;                                                     // [-] - Projection × view
    column_major float4x4 WorldFromClip;                                                     // [-] - Its inverse
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Per-face / per-edge state words (storage buffers, one uint per scene face / edge).
static const uint StateSelected = 1u;                                                        // In the selection
static const uint StateActive   = 2u;                                                        // Last selected (operand in focus)
static const uint StateHovered  = 4u;                                                        // Under the pointer

// 📝 SolidArc browser palette: selection #4099ff, active selection #33ff73, viewport #171b23.
static const float3 SelectedTint = float3(0.251, 0.600, 1.000);
static const float3 ActiveTint   = float3(0.200, 1.000, 0.451);
static const float3 HoverTint    = float3(0.780, 0.880, 1.000);

static const uint IdentityEdgeBit = 0x80000000u;                                             // Identity code: edge flag
