/*====================================================================================================================================
                                                    DRAFTINGDOCUMENTCODEC.H
====================================================================================================================================*/
// 🧩 DraftingWorkspace document files (.drafting.toml): parameters, steps, sketches, references, captions

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"

#include <map>
#include <string>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

struct DraftingDocumentFile
{
    std::string                        Title    = {};                                        // [-] - Document caption
    Modeller::FeatureTree              Tree     = {};                                        // [-] - Steps + parameters (signatures re-captured on load)
    std::map<std::string, std::string> Captions = {};                                        // [-] - Step label → caption
    std::vector<DraftingImagePlane>    ImagePlanes = {};                                     // [-] - Blueprint references
    DraftingAppearance                 Appearance  = {};                                     // [-] - Body / face matcaps
    std::string                        Refusal  = {};                                        // [-] - Parse failure (empty: fine)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 📝 The text is deterministic for one tree (keys in a fixed order), so a save → load → save round trip is byte-equal.
[[nodiscard]] std::string           EncodeDraftingDocument(const DraftingDocument& Document);
[[nodiscard]] DraftingDocumentFile DecodeDraftingDocument(const std::string& Text);

// File helpers: false / Refusal on an I/O or parse failure.
bool                               SaveDraftingDocument(const DraftingDocument& Document, const std::string& Path);
[[nodiscard]] DraftingDocumentFile LoadDraftingDocumentFile(const std::string& Path);

} // namespace Frontier::DraftingWorkspace
