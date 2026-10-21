/*====================================================================================================================================
                                                       DRAFTINGMATCAP.H
====================================================================================================================================*/
// 🧩 Matcaps: the built-in engineering library (metals, plastics, rubber, clay, ceramic, paint) composed analytically, user
//    pictures, and the document appearance (body matcap + per-face matcaps by persistent name)
#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingImagePlane.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Layer 0 is the analytic Studio shading of DraftingSurface.slang (no picture); built-in and user matcaps follow as
//    texture-array layers 1 … n. The layer travels in bits 8 … 15 of the per-face state word.
inline constexpr const char* DraftingStudioMatcap = "Studio";
constexpr uint32_t DraftingMatcapExtent     = 256;                                           // [px] - Square layer side
constexpr uint32_t DraftingMatcapLayerShift = 8;                                             // [-]  - State-word bit offset
constexpr uint32_t DraftingMatcapLayerLimit = 255;                                           // [-]  - Largest layer index

//------------------------------------------------------------------------------------------------------------------------
//                                                    TYPES
//------------------------------------------------------------------------------------------------------------------------

// A user matcap: a sphere picture (PNG / JPEG / BMP / GIF, e.g. a Blender matcap) under a document-unique name.
struct DraftingCustomMatcap
{
    std::string Name = {};                                                                   // [-] - "ShopSteel"
    std::string Path = {};                                                                   // [-] - Picture file
};

// 📝 Saved with the document and part of the undo history (appearance entries never regenerate the body). Face keys are
//    persistent names, so an assignment survives regeneration; a name the body no longer has is kept and reported, never
//    moved to another face.
struct DraftingAppearance
{
    std::string                        BodyMatcap    = DraftingStudioMatcap;                 // [-] - Faces without their own matcap
    std::map<std::string, std::string> FaceMatcaps   = {};                                   // [-] - Persistent face name → matcap
    std::vector<DraftingCustomMatcap>  CustomMatcaps = {};                                   // [-] - User pictures, in layer order
};

// One built-in matcap: its name, caption and the look it was composed for.
struct DraftingMatcapDescription
{
    std::string Name     = {};                                                               // [-] - "BrushedSteel"
    std::string Caption  = {};                                                               // [-] - "Brushed steel"
    std::string Category = {};                                                               // [-] - "Metal", "Plastic", "Other"
};

// 📝 The layers the viewport samples: Names[0] = Studio (no picture), Pictures[k − 1] is layer k. Key changes exactly when
//    the custom list changes (the built-ins are fixed), so the GPU array is re-uploaded only then.
struct DraftingMatcapLibrary
{
    std::vector<std::string>     Names    = {};                                              // [-] - Layer → matcap name
    std::vector<std::string>     Captions = {};                                              // [-] - Layer → caption
    std::vector<DraftingPicture> Pictures = {};                                              // [-] - Layers 1 … n, 256 × 256 RGBA8
    std::vector<std::string>     Refusals = {};                                              // [-] - Layer → decode refusal ("" fine)
    std::string                  Key      = {};                                              // [-] - Custom names + paths
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// The built-in library after Studio, in layer order (Chrome first).
[[nodiscard]] const std::vector<DraftingMatcapDescription>& DraftingBuiltInMatcaps();

// 💡 Composes a built-in matcap: pixel (i, j) holds the colour of a sphere whose view-space normal is
//    n = (x, y, √(1 − x² − y²)), x = 2(i + ½)/E − 1, y = 1 − 2(j + ½)/E, lit by a fixed studio rig (key and strip softboxes, top
//    panel, rim, floor). Metals reflect the rig tinted by Schlick F(θ) with F₀ = tint; plastics add a white Fresnel
//    reflection (F₀ 0.04) to a diffuse base; brushed metal averages the reflection along screen x (anisotropic). Pixels
//    outside the disc repeat the rim colour so linear filtering at the silhouette stays clean. Unknown name → refusal.
[[nodiscard]] DraftingPicture ComposeDraftingMatcap(const std::string& Name, uint32_t Extent = DraftingMatcapExtent);

// The analytic Studio shading of DraftingSurface.slang (layer 0) as a matcap picture: the Inspector thumbnail, and the
// gate that the shader formula and its mirror agree.
[[nodiscard]] DraftingPicture ComposeDraftingStudioMatcap(uint32_t Extent = DraftingMatcapExtent);

// Centre square crop + bilinear resample to Extent × Extent, alpha forced opaque (user pictures of any size).
[[nodiscard]] DraftingPicture FitDraftingMatcapPicture(const DraftingPicture& Source, uint32_t Extent = DraftingMatcapExtent);

// Studio + built-ins + the appearance's custom pictures (decoded and fitted; a refused picture keeps its layer, grey).
[[nodiscard]] DraftingMatcapLibrary AssembleDraftingMatcaps(const DraftingAppearance& Appearance);
[[nodiscard]] std::string           DraftingMatcapKey(const DraftingAppearance& Appearance);

// Layer of a matcap name in the library (0 = Studio, also for unknown names).
[[nodiscard]] uint32_t DraftingMatcapLayer(const DraftingMatcapLibrary& Library, const std::string& Name) noexcept;

// The matcap a face shows: its own assignment, else the body matcap.
[[nodiscard]] const std::string& DraftingFaceMatcap(const DraftingAppearance& Appearance, const std::string& FaceName);

// Built-in or custom name known to this appearance.
[[nodiscard]] bool DraftingMatcapKnownCondition(const DraftingAppearance& Appearance, const std::string& Name);

} // namespace Frontier::DraftingWorkspace
