/*====================================================================================================================================
                                                       TOPOLOGYREBUILD.H
====================================================================================================================================*/
// 🧩 Face verbs that change topology: extrude (side ring), inset (border ring + inner face), delete (heal or sheet), heal (merge),
//    thicken (sheet → solid)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/GeometryProcessing/Local/FaceChange.h"
#include "Engine/GeometryProcessing/Sketch/SketchProfile.h"

#include <string>
#include <vector>

namespace Frontier::GeometryProcessing
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Each returns an empty string on success (Body, histories, audit and method filled) or the refusal; the input body is
//    never modified.
[[nodiscard]] std::string ExtrudePlanarFaces(ModelPartition& Partition, BodyToken Body, const std::vector<FaceToken>& Faces, double Distance,
                                             BodyToken& Result, std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method);
[[nodiscard]] std::string InsetPlanarFaces(ModelPartition& Partition, BodyToken Body, const std::vector<FaceToken>& Faces, double Distance, double Depth,
                                           BodyToken& Result, std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method);
[[nodiscard]] std::string DeleteBodyFaces(ModelPartition& Partition, BodyToken Body, const std::vector<FaceToken>& Faces, BodyToken& Result,
                                          BodyCategory& Category, std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method);
[[nodiscard]] std::string HealBody(ModelPartition& Partition, BodyToken Body, BodyToken& Result, std::vector<FaceChangeHistory>& Histories,
                                   AuditReport& Audit, std::string& Method);
// 💡 Extrude manifold outline (Blender's push-in): a planar face's loops as a sketch profile in a frame on its plane whose
//    𝑧 is the outward normal (lines, circular arcs, circles and non-rational splines; segments named E<slot>), arranged
//    outer CCW / holes CW. IslandCondition: every neighbour across its edges is a coplanar plane facing the same way —
//    an imprinted region, which no push can move (its neighbours would have to move with it).
[[nodiscard]] std::string DescribePlanarFaceProfile(const ModelPartition& Partition, BodyToken Body, FaceToken Face, SketchProfile& Profile, OrthonormalFrame& Plane,
                                                    bool& IslandCondition);
[[nodiscard]] std::string ThickenSheet(ModelPartition& Partition, BodyToken Body, double Thickness, BodyToken& Result,
                                       std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method);

} // namespace Frontier::GeometryProcessing
