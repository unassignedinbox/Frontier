/*====================================================================================================================================
                                                   DRAFTINGDOCUMENTCODEC.CPP
====================================================================================================================================*/
// 🧩 DraftingWorkspace document files (.drafting.toml): parameters, steps, sketches, references, captions (toml++)

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocumentCodec.h"

#include "Document/DraftingImagePlaneToml.h"

#include <toml++/toml.hpp>

#include <cmath>
#include <fstream>
#include <sstream>

namespace Frontier::DraftingWorkspace
{

using namespace Frontier::SolidArcRemastered;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

constexpr int64_t FormatVersion = 1;                                                         // [-] - Bumped on an incompatible change

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENCODING
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] toml::array EncodeVector(const Vector3& Value) { return toml::array{ Value.X, Value.Y, Value.Z }; }

[[nodiscard]] toml::table EncodeValue(const FeatureValue& Value)
{
    toml::table Table{ { "Literal", Value.Literal } };
    if (!Value.Parameter.empty())
    {
        Table.insert("Parameter", Value.Parameter);
        Table.insert("Factor", Value.Factor);
        Table.insert("Offset", Value.Offset);
    }
    if (!Value.SecondParameter.empty())
    {
        Table.insert("SecondParameter", Value.SecondParameter);
        Table.insert("SecondFactor", Value.SecondFactor);
    }
    return Table;
}

[[nodiscard]] toml::array EncodeReferences(const std::vector<TopologyReference>& References)
{
    toml::array Entries;
    for (const TopologyReference& Reference : References)
    {
        toml::table Entry{ { "Kind", static_cast<int64_t>(Reference.Kind) }, { "Name", Reference.Name } };
        if (Reference.Signature.CapturedCondition)
        {
            Entry.insert("Position", EncodeVector(Reference.Signature.Position));
            Entry.insert("Direction", EncodeVector(Reference.Signature.Direction));
            Entry.insert("Category", Reference.Signature.Category);
            Entry.insert("Size", Reference.Signature.Size);
        }
        Entries.push_back(std::move(Entry));
    }
    return Entries;
}

[[nodiscard]] toml::table EncodeSketch(const SketchModel& Sketch)
{
    toml::array Points;
    for (const SketchPoint& Point : Sketch.Points) Points.push_back(toml::array{ Point.Position.X, Point.Position.Y });
    toml::array Curves;
    for (const SketchCurve& Curve : Sketch.Curves)
    {
        toml::table Entry{ { "Kind", static_cast<int64_t>(Curve.Kind) }, { "Identifier", Curve.Identifier },
                           { "Start", Curve.Start }, { "End", Curve.End }, { "Centre", Curve.Centre },
                           { "Radius", Curve.Radius }, { "Construction", Curve.Construction } };
        if (Curve.Kind == SketchCurveKind::Spline)
        {
            toml::array Indices;
            for (int Index : Curve.Points) Indices.push_back(Index);
            Entry.insert("Form", static_cast<int64_t>(Curve.Form));
            Entry.insert("FormLabel", SketchSplineFormLabel(Curve.Form));
            Entry.insert("Points", std::move(Indices));
        }
        Curves.push_back(std::move(Entry));
    }
    toml::array Constraints;
    for (const SketchConstraint& Constraint : Sketch.Constraints)
    {
        toml::table Entry{ { "Kind", static_cast<int64_t>(Constraint.Kind) }, { "First", Constraint.First }, { "Second", Constraint.Second },
                           { "Third", Constraint.Third }, { "Value", Constraint.Value }, { "SecondValue", Constraint.SecondValue },
                           { "Label", Constraint.Label } };
        if (!Constraint.Parameter.empty()) Entry.insert("Parameter", Constraint.Parameter);
        Constraints.push_back(std::move(Entry));
    }
    return toml::table{ { "Points", std::move(Points) }, { "Curves", std::move(Curves) }, { "Constraints", std::move(Constraints) } };
}

[[nodiscard]] toml::table EncodeStep(const FeatureStep& Step, const std::map<std::string, std::string>& Captions)
{
    toml::table Table{ { "Kind", static_cast<int64_t>(Step.Kind) }, { "KindLabel", FeatureStepKindLabel(Step.Kind) }, { "Label", Step.Label } };
    const auto Found = Captions.find(Step.Label);
    if (Found != Captions.end()) Table.insert("Caption", Found->second);
    switch (Step.Kind)
    {
    case FeatureStepKind::Sketch:
        Table.insert("PlaneOrigin", EncodeVector(Step.Plane.Origin));
        Table.insert("PlaneXAxis", EncodeVector(Step.Plane.XAxis));
        Table.insert("PlaneYAxis", EncodeVector(Step.Plane.YAxis));
        Table.insert("PlaneZAxis", EncodeVector(Step.Plane.ZAxis));
        Table.insert("Elevation", EncodeValue(Step.Elevation));
        Table.insert("Sketch", EncodeSketch(Step.Sketch));
        break;
    case FeatureStepKind::Extrude:
    case FeatureStepKind::Revolve:
        Table.insert("Profile", Step.Profile);
        Table.insert("Extent", EncodeValue(Step.Extent));
        Table.insert("Combination", static_cast<int64_t>(Step.Combination));
        Table.insert("AxisIdentifier", Step.AxisIdentifier);
        Table.insert("Angle", EncodeValue(Step.Angle));
        break;
    case FeatureStepKind::Blend:
        Table.insert("Shape", static_cast<int64_t>(Step.Shape));
        Table.insert("Size", EncodeValue(Step.Size));
        if (Step.EndSize)
        {
            Table.insert("EndSize", EncodeValue(*Step.EndSize));
            Table.insert("SizeAnchors", EncodeReferences(Step.SizeAnchors));
        }
        break;
    case FeatureStepKind::FaceOperation:
        Table.insert("Operation", static_cast<int64_t>(Step.Operation));
        Table.insert("Amount", EncodeValue(Step.Amount));
        break;
    case FeatureStepKind::SpaceCurve:
    {
        toml::array Points;
        for (const Vector3& Point : Step.Points) Points.push_back(EncodeVector(Point));
        Table.insert("Points", std::move(Points));
        Table.insert("Closed", Step.ClosedCondition);
        if (Step.PlanarCondition) Table.insert("Planar", true);
        Table.insert("Tolerance", EncodeValue(Step.Tolerance));
        if (!Step.PoleEdits.empty())
        {
            toml::array Edits;
            for (const FeaturePoleEdit& Edit : Step.PoleEdits) Edits.push_back(toml::table{ { "Index", static_cast<int64_t>(Edit.Index) }, { "Position", EncodeVector(Edit.Position) } });
            Table.insert("PoleEdits", std::move(Edits));
        }
        break;
    }
    case FeatureStepKind::Loft:
    case FeatureStepKind::Network:
    {
        toml::array Sections;
        for (const std::string& Section : Step.Sections) Sections.push_back(Section);
        Table.insert("Sections", std::move(Sections));
        if (Step.Continuity != 0) Table.insert("Continuity", static_cast<int64_t>(Step.Continuity));
        break;
    }
    case FeatureStepKind::Mirror:
        Table.insert("PlaneOrigin", EncodeVector(Step.Plane.Origin));
        Table.insert("PlaneZAxis", EncodeVector(Step.Plane.ZAxis));
        break;
    case FeatureStepKind::Close:
        break;
    }
    if (!Step.References.empty()) Table.insert("References", EncodeReferences(Step.References));
    return Table;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    DECODING
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] Vector3 DecodeVector(const toml::node_view<const toml::node> View, Vector3 Fallback)
{
    const toml::array* Array = View.as_array();
    if (!Array || Array->size() != 3) return Fallback;
    return { (*Array)[0].value_or(Fallback.X), (*Array)[1].value_or(Fallback.Y), (*Array)[2].value_or(Fallback.Z) };
}

[[nodiscard]] FeatureValue DecodeValue(const toml::node_view<const toml::node> View, FeatureValue Fallback = {})
{
    if (!View.is_table()) return Fallback;
    FeatureValue Value;
    Value.Literal         = View["Literal"].value_or(0.0);
    Value.Parameter       = View["Parameter"].value_or(std::string{});
    Value.Factor          = View["Factor"].value_or(1.0);
    Value.Offset          = View["Offset"].value_or(0.0);
    Value.SecondParameter = View["SecondParameter"].value_or(std::string{});
    Value.SecondFactor    = View["SecondFactor"].value_or(0.0);
    return Value;
}

[[nodiscard]] std::vector<TopologyReference> DecodeReferences(const toml::node_view<const toml::node> View)
{
    std::vector<TopologyReference> References;
    const toml::array* Entries = View.as_array();
    if (!Entries) return References;
    for (const toml::node& Entry : *Entries)
    {
        const toml::node_view<const toml::node> Reference{ Entry };
        TopologyReference Decoded;
        Decoded.Kind = static_cast<ReferenceKind>(Reference["Kind"].value_or(int64_t{ 1 }));
        Decoded.Name = Reference["Name"].value_or(std::string{});
        if (Reference["Position"])
        {
            Decoded.Signature.CapturedCondition = true;
            Decoded.Signature.Position  = DecodeVector(Reference["Position"], {});
            Decoded.Signature.Direction = DecodeVector(Reference["Direction"], {});
            Decoded.Signature.Category  = Reference["Category"].value_or(std::string{});
            Decoded.Signature.Size      = Reference["Size"].value_or(0.0);
        }
        References.push_back(std::move(Decoded));
    }
    return References;
}

[[nodiscard]] SketchModel DecodeSketch(const toml::node_view<const toml::node> View)
{
    SketchModel Sketch;
    if (const toml::array* Points = View["Points"].as_array())
    {
        for (const toml::node& Point : *Points)
        {
            const toml::array* Pair = Point.as_array();
            if (Pair && Pair->size() == 2) Sketch.Points.push_back({ { (*Pair)[0].value_or(0.0), (*Pair)[1].value_or(0.0) } });
        }
    }
    if (const toml::array* Curves = View["Curves"].as_array())
    {
        for (const toml::node& Entry : *Curves)
        {
            const toml::node_view<const toml::node> Curve{ Entry };
            SketchCurve Decoded;
            Decoded.Kind         = static_cast<SketchCurveKind>(Curve["Kind"].value_or(int64_t{ 0 }));
            Decoded.Identifier   = static_cast<int>(Curve["Identifier"].value_or(int64_t{ 0 }));
            Decoded.Start        = static_cast<int>(Curve["Start"].value_or(int64_t{ -1 }));
            Decoded.End          = static_cast<int>(Curve["End"].value_or(int64_t{ -1 }));
            Decoded.Centre       = static_cast<int>(Curve["Centre"].value_or(int64_t{ -1 }));
            Decoded.Radius       = Curve["Radius"].value_or(1.0);
            Decoded.Construction = Curve["Construction"].value_or(false);
            Decoded.Form         = static_cast<SketchSplineForm>(Curve["Form"].value_or(int64_t{ 4 }));
            if (const toml::array* Indices = Curve["Points"].as_array()) for (const toml::node& Index : *Indices) Decoded.Points.push_back(static_cast<int>(Index.value_or(int64_t{ -1 })));
            Sketch.Curves.push_back(Decoded);
        }
    }
    if (const toml::array* Constraints = View["Constraints"].as_array())
    {
        for (const toml::node& Entry : *Constraints)
        {
            const toml::node_view<const toml::node> Constraint{ Entry };
            SketchConstraint Decoded;
            Decoded.Kind        = static_cast<SketchConstraintKind>(Constraint["Kind"].value_or(int64_t{ 0 }));
            Decoded.First       = static_cast<int>(Constraint["First"].value_or(int64_t{ -1 }));
            Decoded.Second      = static_cast<int>(Constraint["Second"].value_or(int64_t{ -1 }));
            Decoded.Third       = static_cast<int>(Constraint["Third"].value_or(int64_t{ -1 }));
            Decoded.Value       = Constraint["Value"].value_or(0.0);
            Decoded.SecondValue = Constraint["SecondValue"].value_or(0.0);
            Decoded.Parameter   = Constraint["Parameter"].value_or(std::string{});
            Decoded.Label       = Constraint["Label"].value_or(std::string{});
            Sketch.Constraints.push_back(std::move(Decoded));
        }
    }
    return Sketch;
}

// 📝 [Appearance]: BodyMatcap, Faces = [{ Face, Matcap }], Matcaps = [{ Name, Path }]. Written only when it differs from the
//    default (Studio, no faces, no pictures), so documents without matcaps keep their exact bytes.
[[nodiscard]] bool DefaultAppearanceCondition(const DraftingAppearance& Appearance)
{
    return Appearance.BodyMatcap == DraftingStudioMatcap && Appearance.FaceMatcaps.empty() && Appearance.CustomMatcaps.empty();
}

[[nodiscard]] toml::table EncodeAppearance(const DraftingAppearance& Appearance)
{
    toml::array Faces;
    for (const auto& [Face, Matcap] : Appearance.FaceMatcaps) Faces.push_back(toml::table{ { "Face", Face }, { "Matcap", Matcap } });
    toml::array Matcaps;
    for (const DraftingCustomMatcap& Custom : Appearance.CustomMatcaps) Matcaps.push_back(toml::table{ { "Name", Custom.Name }, { "Path", Custom.Path } });
    toml::table Table{ { "BodyMatcap", Appearance.BodyMatcap } };
    if (!Faces.empty()) Table.insert("Faces", std::move(Faces));
    if (!Matcaps.empty()) Table.insert("Matcaps", std::move(Matcaps));
    return Table;
}

// Refuses malformed entries and names that are neither Studio, a built-in nor one of the file's own pictures.
[[nodiscard]] std::string DecodeAppearance(const toml::table* Table, DraftingAppearance& Appearance)
{
    Appearance = {};
    if (!Table) return {};
    if (const toml::array* Matcaps = (*Table)["Matcaps"].as_array())
    {
        for (const toml::node& Entry : *Matcaps)
        {
            const toml::node_view<const toml::node> View{ Entry };
            DraftingCustomMatcap Custom{ View["Name"].value_or(std::string{}), View["Path"].value_or(std::string{}) };
            if (Custom.Name.empty() || Custom.Path.empty()) return "Appearance: every matcap picture needs a Name and a Path";
            if (DraftingMatcapKnownCondition(Appearance, Custom.Name)) return "Appearance: matcap " + Custom.Name + " is named twice";
            Appearance.CustomMatcaps.push_back(std::move(Custom));
        }
    }
    Appearance.BodyMatcap = (*Table)["BodyMatcap"].value_or(std::string(DraftingStudioMatcap));
    if (!DraftingMatcapKnownCondition(Appearance, Appearance.BodyMatcap)) return "Appearance: no matcap named " + Appearance.BodyMatcap;
    if (const toml::array* Faces = (*Table)["Faces"].as_array())
    {
        for (const toml::node& Entry : *Faces)
        {
            const toml::node_view<const toml::node> View{ Entry };
            const std::string Face   = View["Face"].value_or(std::string{});
            const std::string Matcap = View["Matcap"].value_or(std::string{});
            if (Face.empty()) return "Appearance: a face entry has no Face name";
            if (!DraftingMatcapKnownCondition(Appearance, Matcap)) return "Appearance: no matcap named " + Matcap + " (face " + Face + ")";
            Appearance.FaceMatcaps[Face] = Matcap;
        }
    }
    return {};
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

std::string EncodeDraftingDocument(const DraftingDocument& Document)
{
    toml::array Parameters;
    for (const FeatureParameter& Parameter : Document.Tree.Parameters)
    {
        Parameters.push_back(toml::table{ { "Name", Parameter.Name }, { "Value", Parameter.Value }, { "Low", Parameter.Low },
                                          { "High", Parameter.High }, { "Description", Parameter.Description } });
    }
    toml::array Steps;
    for (const FeatureStep& Step : Document.Tree.Steps) Steps.push_back(EncodeStep(Step, Document.Captions));
    toml::table Root{ { "Format", "DraftingWorkspace" }, { "FormatVersion", FormatVersion }, { "Title", Document.Title },
                      { "Description", Document.Tree.Description }, { "Parameters", std::move(Parameters) }, { "Steps", std::move(Steps) } };
    if (!Document.ImagePlanes.empty()) Root.insert("ImagePlanes", EncodeDraftingImagePlanes(Document.ImagePlanes));
    if (!DefaultAppearanceCondition(Document.Appearance)) Root.insert("Appearance", EncodeAppearance(Document.Appearance));
    std::ostringstream Stream;
    Stream << "# DraftingWorkspace document — feature tree of SolidArcRemastered (regenerated on load)\n";
    Stream << Root << "\n";
    return Stream.str();
}

DraftingDocumentFile DecodeDraftingDocument(const std::string& Text)
{
    DraftingDocumentFile File;
    toml::table          Root;
    try
    {
        Root = toml::parse(Text);
    }
    catch (const toml::parse_error& Failure)
    {
        File.Refusal = std::string("TOML: ") + std::string(Failure.description());
        return File;
    }
    if (Root["Format"].value_or(std::string{}) != "DraftingWorkspace") { File.Refusal = "not a DraftingWorkspace document"; return File; }
    if (Root["FormatVersion"].value_or(int64_t{ 0 }) > FormatVersion) { File.Refusal = "document written by a newer DraftingWorkspace"; return File; }
    File.Title            = Root["Title"].value_or(std::string{ "Untitled" });
    File.Tree.Name        = File.Title;
    File.Tree.Description = Root["Description"].value_or(std::string{});
    if (std::string Refusal = DecodeDraftingImagePlanes(Root["ImagePlanes"].as_array(), 1.0, File.ImagePlanes); !Refusal.empty()) { File.Refusal = Refusal; return File; }
    if (std::string Refusal = DecodeAppearance(Root["Appearance"].as_table(), File.Appearance); !Refusal.empty()) { File.Refusal = Refusal; return File; }
    if (const toml::array* Parameters = Root["Parameters"].as_array())
    {
        for (const toml::node& Entry : *Parameters)
        {
            const toml::node_view<const toml::node> Parameter{ Entry };
            File.Tree.Parameters.push_back({ Parameter["Name"].value_or(std::string{}), Parameter["Value"].value_or(0.0),
                                             Parameter["Low"].value_or(0.0), Parameter["High"].value_or(1.0),
                                             Parameter["Description"].value_or(std::string{}) });
        }
    }
    if (const toml::array* Steps = Root["Steps"].as_array())
    {
        for (const toml::node& Entry : *Steps)
        {
            const toml::node_view<const toml::node> View{ Entry };
            FeatureStep Step;
            const int64_t Kind = View["Kind"].value_or(int64_t{ 0 });
            if (Kind < 0 || Kind > static_cast<int64_t>(FeatureStepKind::Close)) { File.Refusal = "a step has an unknown kind " + std::to_string(Kind) + " (newer DraftingWorkspace?)"; return File; }
            Step.Kind  = static_cast<FeatureStepKind>(Kind);
            Step.Label = View["Label"].value_or(std::string{});
            if (Step.Label.empty()) { File.Refusal = "a step has no label"; return File; }
            if (const auto Caption = View["Caption"].value<std::string>()) File.Captions[Step.Label] = *Caption;
            Step.Plane.Origin   = DecodeVector(View["PlaneOrigin"], Step.Plane.Origin);
            Step.Plane.XAxis    = DecodeVector(View["PlaneXAxis"], Step.Plane.XAxis);
            Step.Plane.YAxis    = DecodeVector(View["PlaneYAxis"], Step.Plane.YAxis);
            Step.Plane.ZAxis    = DecodeVector(View["PlaneZAxis"], Step.Plane.ZAxis);
            Step.Elevation      = DecodeValue(View["Elevation"]);
            Step.Sketch         = DecodeSketch(View["Sketch"]);
            Step.Profile        = View["Profile"].value_or(std::string{});
            Step.Extent         = DecodeValue(View["Extent"]);
            Step.Combination    = static_cast<FeatureCombination>(View["Combination"].value_or(int64_t{ 0 }));
            Step.AxisIdentifier = static_cast<int>(View["AxisIdentifier"].value_or(int64_t{ -1 }));
            Step.Angle          = DecodeValue(View["Angle"], Step.Angle);
            Step.Shape          = static_cast<BlendProfile>(View["Shape"].value_or(int64_t{ 0 }));
            Step.Size           = DecodeValue(View["Size"]);
            Step.Operation      = static_cast<FaceOperationKind>(View["Operation"].value_or(int64_t{ 1 }));
            Step.Amount         = DecodeValue(View["Amount"]);
            Step.References = DecodeReferences(View["References"]);
            if (View["EndSize"])
            {
                Step.EndSize     = DecodeValue(View["EndSize"]);
                Step.SizeAnchors = DecodeReferences(View["SizeAnchors"]);
            }
            if (const toml::array* Points = View["Points"].as_array())
            {
                for (const toml::node& Point : *Points) Step.Points.push_back(DecodeVector(toml::node_view<const toml::node>{ Point }, Vector3{ std::nan(""), 0.0, 0.0 }));
            }
            Step.ClosedCondition = View["Closed"].value_or(false);
            Step.PlanarCondition = View["Planar"].value_or(false);
            Step.Continuity      = static_cast<int>(View["Continuity"].value_or(int64_t{ 0 }));
            Step.Tolerance       = DecodeValue(View["Tolerance"], Step.Tolerance);
            if (const toml::array* Edits = View["PoleEdits"].as_array())
            {
                for (const toml::node& Edit : *Edits)
                {
                    const toml::node_view<const toml::node> Entry{ Edit };
                    Step.PoleEdits.push_back({ static_cast<int>(Entry["Index"].value_or(int64_t{ -1 })), DecodeVector(Entry["Position"], Vector3{}) });
                }
            }
            if (const toml::array* Sections = View["Sections"].as_array())
            {
                for (const toml::node& Section : *Sections) Step.Sections.push_back(Section.value_or(std::string{}));
            }
            File.Tree.Steps.push_back(std::move(Step));
        }
    }
    return File;
}

bool SaveDraftingDocument(const DraftingDocument& Document, const std::string& Path)
{
    std::ofstream Stream(Path, std::ios::binary);
    if (!Stream) return false;
    Stream << EncodeDraftingDocument(Document);
    return static_cast<bool>(Stream);
}

DraftingDocumentFile LoadDraftingDocumentFile(const std::string& Path)
{
    std::ifstream Stream(Path, std::ios::binary);
    if (!Stream)
    {
        DraftingDocumentFile File;
        File.Refusal = "cannot open " + Path;
        return File;
    }
    std::ostringstream Text;
    Text << Stream.rdbuf();
    DraftingDocumentFile File = DecodeDraftingDocument(Text.str());
    for (DraftingImagePlane& Plane : File.ImagePlanes) Plane.Path = ResolveDraftingImagePath(Plane.Path, Path);
    for (DraftingCustomMatcap& Custom : File.Appearance.CustomMatcaps) Custom.Path = ResolveDraftingImagePath(Custom.Path, Path);
    return File;
}

} // namespace Frontier::DraftingWorkspace
