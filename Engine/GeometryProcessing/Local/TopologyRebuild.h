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
[[nodiscard]] std::string ThickenSheet(ModelPartition& Partition, BodyToken Body, double Thickness, BodyToken& Result,
                                       std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method);

} // namespace Frontier::GeometryProcessing
