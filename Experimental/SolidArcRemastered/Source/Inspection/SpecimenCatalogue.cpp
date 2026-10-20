/*====================================================================================================================================
                                                     SPECIMENCATALOGUE.CPP
====================================================================================================================================*/
// 🧩 Named specimen bodies with independently derived expectations (counts, genus, analytic volume) for the gates

#include "Inspection/SpecimenCatalogue.h"
#include "Construction/PrimitiveConstruction.h"

#include <cmath>
#include <functional>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double π = std::numbers::pi;                                                   // [-] - Circle constant

    using SpecimenAssembly = std::function<SpecimenOutcome(ModelPartition&, const SpecimenParameters&)>;

    // Record a parameter as used and return its value.
    double Use(SpecimenOutcome& Outcome, const SpecimenParameters& Parameters, const std::string& Key, double Default)
    {
        const double Resolved = Parameters.Resolve(Key, Default);                            // [-] - Override or default
        Outcome.Parameters[Key] = Resolved;
        return Resolved;
    }

    template <typename Result>
    void Adopt(SpecimenOutcome& Outcome, const Result& Edit)
    {
        if (Edit.SucceededCondition()) Outcome.Body = Edit.Value;
        else Outcome.Refusal = Edit.Refusal;
    }

    const std::vector<std::pair<std::string, SpecimenAssembly>>& Catalogue()
    {
        static const std::vector<std::pair<std::string, SpecimenAssembly>> Entries = {
            { "Block", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "Prism sweep of a rectangle: MVFS, 3 MEV, MEF, then 4 MEV + 4 MEF" };
                const Vector3 Extent{ Use(Outcome, Parameters, "SizeX", 4.0), Use(Outcome, Parameters, "SizeY", 3.0), Use(Outcome, Parameters, "SizeZ", 2.0) };
                Adopt(Outcome, ConstructBlock(Partition, {}, Extent));
                Outcome.Expectation = { 8, 12, 6, 6, 1, 0, Extent.X * Extent.Y * Extent.Z };
                return Outcome;
            } },
            { "HexagonalPrism", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "Generic translational sweep of a regular hexagon" };
                const double Radius = Use(Outcome, Parameters, "Radius", 2.0);
                const double Height = Use(Outcome, Parameters, "Height", 3.0);
                std::vector<Vector3> Profile;
                for (int CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
                {
                    const double θ = π / 3.0 * CornerIndex;                                  // [rad] - Corner azimuth
                    Profile.push_back({ Radius * std::cos(θ), Radius * std::sin(θ), 0.0 });
                }
                const auto Edit = ConstructPrism(Partition, Profile, { 0.0, 0.0, Height });
                if (Edit.SucceededCondition()) Outcome.Body = Edit.Value.Body; else Outcome.Refusal = Edit.Refusal;
                Outcome.Expectation = { 12, 18, 8, 8, 1, 0, 1.5 * std::sqrt(3.0) * Radius * Radius * Height };
                return Outcome;
            } },
            { "Cylinder", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "Seam edge on a periodic chart; closed circle edges via MEF on one corner" };
                const double Radius = Use(Outcome, Parameters, "Radius", 1.5);
                const double Height = Use(Outcome, Parameters, "Height", 3.0);
                Adopt(Outcome, ConstructCylinder(Partition, OrthonormalFrame{}, Radius, Height));
                Outcome.Expectation = { 2, 3, 3, 3, 1, 0, π * Radius * Radius * Height };
                return Outcome;
            } },
            { "Cone", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "Apex vertex on a singular row of the cone chart" };
                const double Radius = Use(Outcome, Parameters, "Radius", 2.0);
                const double Height = Use(Outcome, Parameters, "Height", 3.0);
                Adopt(Outcome, ConstructCone(Partition, OrthonormalFrame{}, Radius, Height));
                Outcome.Expectation = { 2, 2, 2, 2, 1, 0, π * Radius * Radius * Height / 3.0 };
                return Outcome;
            } },
            { "Sphere", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "One face, one seam, two singular pole rows" };
                const double Radius = Use(Outcome, Parameters, "Radius", 2.0);
                Adopt(Outcome, ConstructSphere(Partition, OrthonormalFrame{}, Radius));
                Outcome.Expectation = { 2, 1, 1, 1, 1, 0, 4.0 / 3.0 * π * Radius * Radius * Radius };
                return Outcome;
            } },
            { "Torus", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "Doubly periodic chart; genus 1 from KFMRH, closed by MEKR" };
                const double MajorRadius = Use(Outcome, Parameters, "MajorRadius", 3.0);
                const double MinorRadius = Use(Outcome, Parameters, "MinorRadius", 1.0);
                Adopt(Outcome, ConstructTorus(Partition, OrthonormalFrame{}, MajorRadius, MinorRadius));
                Outcome.Expectation = { 1, 2, 1, 1, 1, 1, 2.0 * π * π * MajorRadius * MinorRadius * MinorRadius };
                return Outcome;
            } },
            { "PerforatedBlock", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "Through hole: ring by KEMR, inward sweep, KFMRH into the bottom face" };
                const Vector3 Extent{ Use(Outcome, Parameters, "SizeX", 6.0), Use(Outcome, Parameters, "SizeY", 4.0), Use(Outcome, Parameters, "SizeZ", 2.0) };
                const Vector3 Hole{ Use(Outcome, Parameters, "HoleX", 2.0), Use(Outcome, Parameters, "HoleY", 1.5), 0.0 };
                Adopt(Outcome, ConstructPerforatedBlock(Partition, Extent, Hole));
                Outcome.Expectation = { 16, 24, 10, 12, 1, 1, (Extent.X * Extent.Y - Hole.X * Hole.Y) * Extent.Z };
                return Outcome;
            } },
            { "HollowBlock", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "Cavity: a reversed second shell bounding the same solid region" };
                const Vector3 Extent{ Use(Outcome, Parameters, "SizeX", 6.0), Use(Outcome, Parameters, "SizeY", 5.0), Use(Outcome, Parameters, "SizeZ", 4.0) };
                const Vector3 Cavity{ Use(Outcome, Parameters, "CavityX", 3.0), Use(Outcome, Parameters, "CavityY", 2.0), Use(Outcome, Parameters, "CavityZ", 1.0) };
                Adopt(Outcome, ConstructHollowBlock(Partition, Extent, Cavity));
                Outcome.Expectation = { 16, 24, 12, 12, 2, 0, Extent.X * Extent.Y * Extent.Z - Cavity.X * Cavity.Y * Cavity.Z };
                return Outcome;
            } },
            { "TolerantBlock", [](ModelPartition& Partition, const SpecimenParameters& Parameters)
            {
                SpecimenOutcome Outcome{ .Description = "One vertex displaced off its exact edges and faces; local precision covers it (0 = exact)" };
                const Vector3 Extent{ Use(Outcome, Parameters, "SizeX", 4.0), Use(Outcome, Parameters, "SizeY", 3.0), Use(Outcome, Parameters, "SizeZ", 2.0) };
                const double  Displacement = Use(Outcome, Parameters, "Displacement", 4.0e-6);
                const double  Precision    = Use(Outcome, Parameters, "Precision", 1.0e-5);
                Adopt(Outcome, ConstructBlock(Partition, {}, Extent));
                Outcome.Expectation = { 8, 12, 6, 6, 1, 0, Extent.X * Extent.Y * Extent.Z };
                if (!Outcome.Refusal.empty()) return Outcome;
                // 📝 Displace the top corner nearest (SizeX, SizeY, SizeZ); its three edges and three faces stay exact.
                const BodyInventory Inventory = Partition.CollectInventory(Outcome.Body);
                VertexToken Corner = Inventory.Vertices.front();                             // [-] - Vertex to displace
                for (VertexToken Vertex : Inventory.Vertices)
                {
                    if (Distance(Partition.Vertices.Resolve(Vertex).Position, Extent) < Distance(Partition.Vertices.Resolve(Corner).Position, Extent)) Corner = Vertex;
                }
                Partition.Vertices.Resolve(Corner).Position = Partition.Vertices.Resolve(Corner).Position + Vector3{ Displacement, -Displacement, Displacement } * (1.0 / std::sqrt(3.0));
                if (Precision > 0.0) Partition.AssignVertexPrecision(Corner, Precision);
                return Outcome;
            } },
        };
        return Entries;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

std::vector<std::string> EnumerateSpecimens()
{
    std::vector<std::string> Names;
    for (const auto& Entry : Catalogue()) Names.push_back(Entry.first);
    return Names;
}

SpecimenOutcome ConstructSpecimen(ModelPartition& Partition, const std::string& Name, const SpecimenParameters& Parameters)
{
    for (const auto& Entry : Catalogue())
    {
        if (Entry.first == Name) return Entry.second(Partition, Parameters);
    }
    SpecimenOutcome Unknown;
    Unknown.Refusal = "unknown specimen '" + Name + "'";
    return Unknown;
}

} // namespace Frontier::SolidArcRemastered
