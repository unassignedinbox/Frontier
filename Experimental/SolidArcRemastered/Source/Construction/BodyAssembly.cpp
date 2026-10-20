/*====================================================================================================================================
                                                       BODYASSEMBLY.CPP
====================================================================================================================================*/
// 🧩 Direct assembly of a solid body from a blueprint of vertices, edges with carriers and faces with SP-curved loops

#include "Construction/BodyAssembly.h"

#include "Audit/EnclosedVolume.h"
#include "Containment/WindingNumber.h"

#include <functional>
#include <limits>
#include <map>
#include <numeric>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

AssemblyOutcome AssembleBody(ModelPartition& Partition, const BodyBlueprint& Blueprint, const std::string& Label)
{
    AssemblyOutcome Outcome;
    if (Blueprint.Faces.empty()) { Outcome.Refusal = Label + ": empty result"; return Outcome; }

    // ① Manifold check: every used edge is used exactly twice, in opposite directions.
    std::vector<int> UseCount(Blueprint.Edges.size(), 0), Balance(Blueprint.Edges.size(), 0);
    for (const BlueprintFace& Face : Blueprint.Faces)
    {
        for (const auto& Loop : Face.Loops)
        {
            for (const BlueprintUse& Use : Loop)
            {
                if (Use.Edge < 0 || Use.Edge >= static_cast<int>(Blueprint.Edges.size()))
                {
                    Outcome.Refusal = Label + ": use of a missing edge " + std::to_string(Use.Edge);
                    return Outcome;
                }
                ++UseCount[Use.Edge];
                Balance[Use.Edge] += Use.Sense == Orientation::Forward ? 1 : -1;
            }
        }
    }
    for (size_t Edge = 0; Edge < Blueprint.Edges.size(); ++Edge)
    {
        if (UseCount[Edge] != 0 && (UseCount[Edge] != 2 || Balance[Edge] != 0))
        {
            Outcome.Refusal = Label + ": non-manifold or open result (pave block " + std::to_string(Edge) + " used " + std::to_string(UseCount[Edge]) + " times)";
            return Outcome;
        }
    }

    // ② Shells = connected components of faces through shared edges.
    std::vector<int> Component(Blueprint.Faces.size());
    std::iota(Component.begin(), Component.end(), 0);
    const std::function<int(int)> Root = [&](int Index) { return Component[Index] == Index ? Index : Component[Index] = Root(Component[Index]); };
    std::map<int, int> FirstUser;
    for (size_t Slot = 0; Slot < Blueprint.Faces.size(); ++Slot)
    {
        for (const auto& Loop : Blueprint.Faces[Slot].Loops)
        {
            for (const BlueprintUse& Use : Loop)
            {
                const auto Found = FirstUser.find(Use.Edge);
                if (Found == FirstUser.end()) FirstUser[Use.Edge] = static_cast<int>(Slot);
                else Component[Root(static_cast<int>(Slot))] = Root(Found->second);
            }
        }
    }

    // ③ Records: body, vertices, edges (carrier copies), shells, faces, loops, fins — issued on first use.
    const BodyToken   Body     = Partition.Bodies.Issue({ BodyCategory::Solid, {} });
    const RegionToken Exterior = Partition.Regions.Issue({ Body, RegionMaterial::Void, {} });
    std::map<int, VertexToken> Vertices;
    Outcome.Edges.assign(Blueprint.Edges.size(), EdgeToken{});
    const auto IssueVertex = [&](int Index)
    {
        const auto Found = Vertices.find(Index);
        if (Found != Vertices.end()) return Found->second;
        return Vertices[Index] = Partition.Vertices.Issue({ Blueprint.Vertices[Index].Position, ExactPrecision });
    };
    const auto IssueEdge = [&](int Index)
    {
        if (Outcome.Edges[Index].PresentCondition()) return Outcome.Edges[Index];
        const BlueprintEdge& Block = Blueprint.Edges[Index];
        const EdgeToken      Edge  = Partition.Edges.Issue({ {}, IssueVertex(Block.Start), IssueVertex(Block.End), Block.Low, Block.High, ExactPrecision, {} });
        Partition.AttachCurve(Edge, Block.Curve, Block.Low, Block.High);
        Outcome.EdgeOrder.push_back(Index);
        return Outcome.Edges[Index] = Edge;
    };
    std::map<int, SolidShellToken> Shells;
    std::vector<SolidShellToken>   ShellOrder;
    for (size_t Slot = 0; Slot < Blueprint.Faces.size(); ++Slot)
    {
        const int Group = Root(static_cast<int>(Slot));
        if (Shells.count(Group) == 0)
        {
            Shells[Group] = Partition.Shells.Issue({ {}, {} });
            ShellOrder.push_back(Shells[Group]);
        }
        const SolidShellToken SolidShell = Shells[Group];
        const BlueprintFace&  Planned    = Blueprint.Faces[Slot];
        const FaceToken       Face       = Partition.Faces.Issue({ SolidShell, {}, Orientation::Forward, {} });
        Partition.AttachSurface(Face, Planned.Surface, Planned.Sense);
        Partition.Shells.Resolve(SolidShell).Faces.push_back(Face);
        Outcome.Faces.push_back(Face);
        for (const auto& LoopUses : Planned.Loops)
        {
            if (LoopUses.empty()) continue;
            const LoopToken Loop = Partition.Loops.Issue({ Face, {}, {} });
            Partition.Faces.Resolve(Face).Loops.push_back(Loop);
            std::vector<FinToken> Fins;
            for (const BlueprintUse& Use : LoopUses)
            {
                const EdgeToken Edge = IssueEdge(Use.Edge);
                const FinToken  Fin  = Partition.Fins.Issue({ Loop, Edge, Use.Sense, {}, {}, {}, Use.SurfaceCurve });
                EdgeProfile& EdgeRecord = Partition.Edges.Resolve(Edge);
                if (!EdgeRecord.RadialFin.PresentCondition()) EdgeRecord.RadialFin = Fin;
                else
                {
                    Partition.Fins.Resolve(Fin).RadialSuccessor                 = EdgeRecord.RadialFin;
                    Partition.Fins.Resolve(EdgeRecord.RadialFin).RadialSuccessor = Fin;
                }
                Fins.push_back(Fin);
            }
            for (size_t Index = 0; Index < Fins.size(); ++Index)
            {
                Partition.Fins.Resolve(Fins[Index]).Successor                      = Fins[(Index + 1) % Fins.size()];
                Partition.Fins.Resolve(Fins[(Index + 1) % Fins.size()]).Predecessor = Fins[Index];
            }
            Partition.Loops.Resolve(Loop).FirstFin = Fins.front();
        }
    }

    // ④ Regions: outer shells (positive volume) → solid regions; cavities → the smallest solid region containing them.
    std::vector<SolidShellToken> OuterShells, CavityShells;
    for (SolidShellToken SolidShell : ShellOrder) (IntegrateShellVolume(Partition, SolidShell) > 0.0 ? OuterShells : CavityShells).push_back(SolidShell);
    std::vector<RegionToken> Regions = { Exterior };
    std::vector<double>      OuterVolumes;
    for (SolidShellToken SolidShell : OuterShells)
    {
        const RegionToken Region = Partition.Regions.Issue({ Body, RegionMaterial::Solid, { SolidShell } });
        Partition.Shells.Resolve(SolidShell).Region = Region;
        Regions.push_back(Region);
        OuterVolumes.push_back(IntegrateShellVolume(Partition, SolidShell));
    }
    for (SolidShellToken Cavity : CavityShells)
    {
        const FaceToken AnyFace  = Partition.Shells.Resolve(Cavity).Faces.front();
        const FinToken  AnyFin   = Partition.Loops.Resolve(Partition.Faces.Resolve(AnyFace).Loops.front()).FirstFin;
        const Vector3   Witness  = Partition.Vertices.Resolve(Partition.FinStart(AnyFin)).Position;
        int    Owner       = -1;
        double OwnerVolume = std::numeric_limits<double>::max();
        for (size_t Index = 0; Index < OuterShells.size(); ++Index)
        {
            const BodyContainment Containment = PrepareFaceContainment(Partition, Partition.Shells.Resolve(OuterShells[Index]).Faces);
            if (ClassifySolidPoint(Containment, Witness).Membership == PointMembership::Inside && OuterVolumes[Index] < OwnerVolume)
            {
                Owner       = static_cast<int>(Index);
                OwnerVolume = OuterVolumes[Index];
            }
        }
        if (Owner < 0) { Outcome.Refusal = Label + ": a cavity shell lies in no solid region"; return Outcome; }
        const RegionToken Region = Regions[Owner + 1];
        Partition.Regions.Resolve(Region).Shells.push_back(Cavity);
        Partition.Shells.Resolve(Cavity).Region = Region;
    }
    Partition.Bodies.Resolve(Body).Regions = Regions;
    Outcome.Body   = Body;
    Outcome.Solids = static_cast<int>(OuterShells.size());
    Outcome.Shells = static_cast<int>(ShellOrder.size());
    return Outcome;
}

} // namespace Frontier::SolidArcRemastered
