/*====================================================================================================================================
                                                     DRAFTINGIMAGEPLANE.H
====================================================================================================================================*/
// 🧩 Image planes: a blueprint / photo crop placed in world space as a reference behind the model (Plasticity / Blender
//    "reference image"), plus the picture decoder the viewport uploads from

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/VectorAlgebra.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

namespace Modeller = Frontier::SolidArcRemastered;

//------------------------------------------------------------------------------------------------------------------------
//                                                    TYPES
//------------------------------------------------------------------------------------------------------------------------

// 📝 The crop rectangle of the picture maps onto the world parallelogram Corner + s·Right + t·Down (s, t ∈ [0, 1]): Corner is
//    the crop's top-left pixel corner, Right spans its width, Down its height. A side view on the Right view: Right = +y,
//    Down = −z. Not part of the feature tree (no undo entry); saved with the document.
struct DraftingImagePlane
{
    std::string                Name             = {};                                        // [-]    - "Side", "Top", "Front"
    std::string                Path             = {};                                        // [-]    - Picture file (PNG / JPEG / BMP / GIF)
    std::array<int, 4>         Crop             = { 0, 0, 0, 0 };                            // [px]   - Left, top, right, bottom (0 0 0 0: whole picture)
    Modeller::Vector3          Corner           = {};                                        // [len]  - World position of the crop's top-left corner
    Modeller::Vector3          Right            = {};                                        // [len]  - World span of the crop width
    Modeller::Vector3          Down             = {};                                        // [len]  - World span of the crop height
    double                     Opacity          = 0.6;                                       // [0-1]  - Line-art ink opacity
    bool                       VisibleCondition = true;                                      // [-]    - Drawn in the viewport
};

// Decoded picture: RGBA8 rows, top row first.
struct DraftingPicture
{
    std::string          Path    = {};                                                       // [-]  - Resolved file
    uint32_t             Width   = 0;                                                        // [px] - Columns
    uint32_t             Height  = 0;                                                        // [px] - Rows
    std::vector<uint8_t> Pixels  = {};                                                       // [-]  - Width × Height × 4
    std::string          Refusal = {};                                                       // [-]  - Why it could not be read (empty: fine)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// PNG / JPEG / BMP / GIF (first frame) via stb_image; a missing or unreadable file is a refusal, never a throw.
[[nodiscard]] DraftingPicture DecodeDraftingPicture(const std::string& Path);

// Relative paths: as given when that file exists (repository-root runs), otherwise next to the file that named it.
[[nodiscard]] std::string ResolveDraftingImagePath(const std::string& Path, const std::string& NamingFile);

// The world point at crop fraction (s, t); and the plane check used by the import (non-degenerate, finite).
[[nodiscard]] Modeller::Vector3 LocateDraftingImagePoint(const DraftingImagePlane& Plane, double Across, double Downward) noexcept;
[[nodiscard]] std::string       ExamineDraftingImagePlane(const DraftingImagePlane& Plane);

} // namespace Frontier::DraftingWorkspace
