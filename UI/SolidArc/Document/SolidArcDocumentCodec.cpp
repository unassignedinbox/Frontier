/*====================================================================================================================================
                                                      SOLIDARCDOCUMENTCODEC.CPP
====================================================================================================================================*/
// 🧩 SolidArc document files (.solidarc): parameters, steps, sketches, references, captions (toml++)

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Document/SolidArcDocumentCodec.h"

#include "Engine/GeometryProcessing/Diagnostics/OperationTrace.h"
#include "Engine/GeometryProcessing/Exchange/ExchangePath.h"

#include "UI/SolidArc/Document/SolidArcDrawingSheet.h"
#include "UI/SolidArc/Document/SolidArcImagePlaneToml.h"

#include <toml++/toml.hpp>

#include <cmath>
#include <fstream>
#include <sstream>

namespace Frontier::SolidArc
{

using namespace Frontier::GeometryProcessing;

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
        if (!Curve.Link.Sketch.empty())
        {
            Entry.insert("LinkSketch", Curve.Link.Sketch);
            Entry.insert("LinkIndex", Curve.Link.Index);
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
        if (Constraint.DrivenCondition) Entry.insert("Reference", true);
        Constraints.push_back(std::move(Entry));
    }
    toml::table Table{ { "Points", std::move(Points) }, { "Curves", std::move(Curves) }, { "Constraints", std::move(Constraints) } };
    // Links = [{ Point, Sketch, Index }]: points projected from earlier sketches; written only when there are any.
    toml::array Links;
    for (size_t Point = 0; Point < Sketch.Points.size(); ++Point)
        if (!Sketch.Points[Point].Link.Sketch.empty())
            Links.push_back(toml::table{ { "Point", static_cast<int64_t>(Point) }, { "Sketch", Sketch.Points[Point].Link.Sketch }, { "Index", Sketch.Points[Point].Link.Index } });
    if (!Links.empty()) Table.insert("Links", std::move(Links));
    return Table;
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
        if (Step.Kind == FeatureStepKind::Revolve && Step.AxisIdentifier < 0)              // Axis without a line (absent in older files)
        {
            Table.insert("AxisOrigin", toml::array{ Step.AxisOrigin.X, Step.AxisOrigin.Y });
            Table.insert("AxisDirection", toml::array{ Step.AxisDirection.X, Step.AxisDirection.Y });
        }
        break;
    case FeatureStepKind::Evolution:
        // The profile sketch and the path (a one-curve sketch or a SpaceCurve).
        Table.insert("Profile", Step.Profile);
        Table.insert("Path", Step.Path);
        Table.insert("Combination", static_cast<int64_t>(Step.Combination));
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
        if (Step.Operation == FaceOperationKind::Inset) Table.insert("Extent", EncodeValue(Step.Extent));
        if (!Step.Points.empty())
        {
            toml::array Points;                                                              // Tweak direction / rotation axis (gizmo)
            for (const Vector3& Point : Step.Points) Points.push_back(EncodeVector(Point));
            Table.insert("Points", std::move(Points));
        }
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
    case FeatureStepKind::Combine:
        Table.insert("Combination", static_cast<int64_t>(Step.Combination));
        Table.insert("Tool", Step.Tool);
        break;
    case FeatureStepKind::Cut:
        // A sketch cutter (Profile), or a SpaceCurve projected along PlaneZAxis (Sections).
        if (!Step.Profile.empty()) Table.insert("Profile", Step.Profile);
        if (!Step.Sections.empty())
        {
            toml::array Sections;
            for (const std::string& Section : Step.Sections) Sections.push_back(Section);
            Table.insert("Sections", std::move(Sections));
            Table.insert("PlaneOrigin", EncodeVector(Step.Plane.Origin));
            Table.insert("PlaneXAxis", EncodeVector(Step.Plane.XAxis));
            Table.insert("PlaneZAxis", EncodeVector(Step.Plane.ZAxis));
        }
        break;
    case FeatureStepKind::Remove:
        break;
    case FeatureStepKind::Joint:
    {
        // Target moves onto Tool; References = moving, fixed joint origin; every value may bind a parameter.
        const FeatureJoint& Joint = Step.Joint;
        Table.insert("Tool", Step.Tool);
        Table.insert("JointKind", static_cast<int64_t>(Joint.Kind));
        Table.insert("JointAngle", EncodeValue(Joint.Angle));
        Table.insert("JointSlide", EncodeValue(Joint.Slide));
        Table.insert("JointSecondSlide", EncodeValue(Joint.SecondSlide));
        Table.insert("JointPitch", EncodeValue(Joint.Pitch));
        Table.insert("JointRoll", EncodeValue(Joint.Roll));
        Table.insert("JointOffset", EncodeValue(Joint.Offset));
        Table.insert("JointFlipped", Joint.FlippedCondition);
        if (Joint.AngleRange) Table.insert("JointAngleRange", toml::array{ Joint.AngleRange->Low, Joint.AngleRange->High });
        if (Joint.SlideRange) Table.insert("JointSlideRange", toml::array{ Joint.SlideRange->Low, Joint.SlideRange->High });
        break;
    }
    case FeatureStepKind::Transform:
    case FeatureStepKind::Duplicate:
        // The affine map as its three linear columns and the shift (x′ = X·x + Y·y + Z·z + Shift).
        Table.insert("PlacementX", EncodeVector(Step.Placement.ColumnX));
        Table.insert("PlacementY", EncodeVector(Step.Placement.ColumnY));
        Table.insert("PlacementZ", EncodeVector(Step.Placement.ColumnZ));
        Table.insert("PlacementShift", EncodeVector(Step.Placement.Shift));
        if (!Step.Source.empty()) Table.insert("Source", Step.Source);                     // Paste: the copy-time step
        break;
    case FeatureStepKind::Import:
        // The kernel's canonical STEP text of the body (millimetres) and the model unit it is read at.
        Table.insert("Exchange", Step.Exchange);
        Table.insert("ExchangeUnit", Step.ExchangeUnit);
        break;
    case FeatureStepKind::Pipe:
        // The path: a sketch (Profile) or a SpaceCurve (Sections); the radius is Size.
        if (!Step.Profile.empty()) Table.insert("Profile", Step.Profile);
        if (!Step.Sections.empty())
        {
            toml::array Sections;
            for (const std::string& Section : Step.Sections) Sections.push_back(Section);
            Table.insert("Sections", std::move(Sections));
        }
        Table.insert("Size", EncodeValue(Step.Size));
        Table.insert("Combination", static_cast<int64_t>(Step.Combination));
        break;
    }
    if (!Step.Target.empty()) Table.insert("Target", Step.Target);                                // Absent: the active body (older files)
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
            Decoded.Link.Sketch  = Curve["LinkSketch"].value_or(std::string{});
            Decoded.Link.Index   = static_cast<int>(Curve["LinkIndex"].value_or(int64_t{ -1 }));
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
            Decoded.DrivenCondition = Constraint["Reference"].value_or(false);
            Sketch.Constraints.push_back(std::move(Decoded));
        }
    }
    if (const toml::array* Links = View["Links"].as_array())
    {
        for (const toml::node& Entry : *Links)
        {
            const toml::node_view<const toml::node> Link{ Entry };
            const int64_t Point = Link["Point"].value_or(int64_t{ -1 });
            if (Point < 0 || Point >= static_cast<int64_t>(Sketch.Points.size())) continue;
            Sketch.Points[Point].Link = { Link["Sketch"].value_or(std::string{}), static_cast<int>(Link["Index"].value_or(int64_t{ -1 })) };
        }
    }
    return Sketch;
}

// 📝 [Appearance]: BodyMatcap (default for every body), Bodies = [{ Body, Matcap }], Faces = [{ Face, Matcap }],
//    Matcaps = [{ Name, Path }]. Written only when it differs from the default (Studio, no bodies, no faces, no pictures), so
//    documents without matcaps keep their exact bytes.
[[nodiscard]] bool DefaultAppearanceCondition(const SolidArcAppearance& Appearance)
{
    return Appearance.BodyMatcap == SolidArcStudioMatcap && Appearance.BodyMatcaps.empty() && Appearance.FaceMatcaps.empty() && Appearance.HiddenBodies.empty()
        && Appearance.CustomMatcaps.empty();
}

[[nodiscard]] toml::table EncodeAppearance(const SolidArcAppearance& Appearance)
{
    toml::array Bodies;
    for (const auto& [Body, Matcap] : Appearance.BodyMatcaps) Bodies.push_back(toml::table{ { "Body", Body }, { "Matcap", Matcap } });
    toml::array Faces;
    for (const auto& [Face, Matcap] : Appearance.FaceMatcaps) Faces.push_back(toml::table{ { "Face", Face }, { "Matcap", Matcap } });
    toml::array Matcaps;
    for (const SolidArcCustomMatcap& Custom : Appearance.CustomMatcaps) Matcaps.push_back(toml::table{ { "Name", Custom.Name }, { "Path", Custom.Path } });
    toml::table Table{ { "BodyMatcap", Appearance.BodyMatcap } };
    if (!Bodies.empty()) Table.insert("Bodies", std::move(Bodies));
    if (!Faces.empty()) Table.insert("Faces", std::move(Faces));
    if (!Matcaps.empty()) Table.insert("Matcaps", std::move(Matcaps));
    if (!Appearance.HiddenBodies.empty())
    {
        toml::array Hidden;
        for (const std::string& Key : Appearance.HiddenBodies) Hidden.push_back(Key);
        Table.insert("Hidden", std::move(Hidden));
    }
    return Table;
}

// Refuses malformed entries and names that are neither Studio, a built-in nor one of the file's own pictures.
[[nodiscard]] std::string DecodeAppearance(const toml::table* Table, SolidArcAppearance& Appearance)
{
    Appearance = {};
    if (!Table) return {};
    if (const toml::array* Matcaps = (*Table)["Matcaps"].as_array())
    {
        for (const toml::node& Entry : *Matcaps)
        {
            const toml::node_view<const toml::node> View{ Entry };
            SolidArcCustomMatcap Custom{ View["Name"].value_or(std::string{}), View["Path"].value_or(std::string{}) };
            if (Custom.Name.empty() || Custom.Path.empty()) return "Appearance: every matcap picture needs a Name and a Path";
            if (SolidArcMatcapKnownCondition(Appearance, Custom.Name)) return "Appearance: matcap " + Custom.Name + " is named twice";
            Appearance.CustomMatcaps.push_back(std::move(Custom));
        }
    }
    Appearance.BodyMatcap = (*Table)["BodyMatcap"].value_or(std::string(SolidArcStudioMatcap));
    if (!SolidArcMatcapKnownCondition(Appearance, Appearance.BodyMatcap)) return "Appearance: no matcap named " + Appearance.BodyMatcap;
    if (const toml::array* Bodies = (*Table)["Bodies"].as_array())
    {
        for (const toml::node& Entry : *Bodies)
        {
            const toml::node_view<const toml::node> View{ Entry };
            const std::string Body   = View["Body"].value_or(std::string{});
            const std::string Matcap = View["Matcap"].value_or(std::string{});
            if (Body.empty()) return "Appearance: a body entry has no Body key";
            if (!SolidArcMatcapKnownCondition(Appearance, Matcap)) return "Appearance: no matcap named " + Matcap + " (body " + Body + ")";
            Appearance.BodyMatcaps[Body] = Matcap;
        }
    }
    if (const toml::array* Faces = (*Table)["Faces"].as_array())
    {
        for (const toml::node& Entry : *Faces)
        {
            const toml::node_view<const toml::node> View{ Entry };
            const std::string Face   = View["Face"].value_or(std::string{});
            const std::string Matcap = View["Matcap"].value_or(std::string{});
            if (Face.empty()) return "Appearance: a face entry has no Face name";
            if (!SolidArcMatcapKnownCondition(Appearance, Matcap)) return "Appearance: no matcap named " + Matcap + " (face " + Face + ")";
            Appearance.FaceMatcaps[Face] = Matcap;
        }
    }
    if (const toml::array* Hidden = (*Table)["Hidden"].as_array())
    {
        for (const toml::node& Entry : *Hidden)
        {
            const std::string Key = Entry.value_or(std::string{});
            if (Key.empty()) return "Appearance: a hidden entry is not a body key";
            Appearance.HiddenBodies.insert(Key);
        }
    }
    return {};
}

// 📝 [DrawingSheet] (only when not the defaults): Paper "A4" … "A0", Angle "First" / "Third", Scale (0 = automatic),
//    Isometric, Section, SectionAxis "X" / "Y" / "Z" / "Face", SectionOffset, SectionFlipped, SectionFace, HiddenLines,
//    TangentEdges, Dimensions and the title block fields.
[[nodiscard]] toml::table EncodeDrawingSheet(const SolidArcDrawingSheetSettings& Settings)
{
    static const char* Axes[] = { "X", "Y", "Z", "Face" };
    const SolidArcTitleFields& Title = Settings.Title;
    return toml::table{ { "Paper", std::string(GeometryProcessing::DrawingPaperLabel(Settings.Paper)) }, { "Angle", std::string(Settings.Angle == GeometryProcessing::DrawingProjectionAngle::First ? "First" : "Third") },
                        { "Scale", Settings.Scale }, { "Isometric", Settings.IsometricEnabled }, { "Section", Settings.SectionEnabled },
                        { "SectionAxis", std::string(Axes[static_cast<int>(Settings.Section.Axis)]) }, { "SectionOffset", Settings.Section.Offset },
                        { "SectionFlipped", Settings.Section.Flipped }, { "SectionFace", Settings.Section.Face }, { "HiddenLines", Settings.HiddenLinesEnabled },
                        { "TangentEdges", Settings.TangentEdgesEnabled }, { "Dimensions", Settings.DimensionsEnabled }, { "Title", Title.Title },
                        { "DrawingNumber", Title.DrawingNumber }, { "Author", Title.Author }, { "Date", Title.Date }, { "Material", Title.Material },
                        { "Revision", Title.Revision }, { "Company", Title.Company } };
}

[[nodiscard]] std::string DecodeDrawingSheet(const toml::table* Table, SolidArcDrawingSheetSettings& Settings)
{
    Settings = {};
    if (!Table) return {};
    const toml::table& Root = *Table;
    const std::string Paper = Root["Paper"].value_or(std::string("A3")), Angle = Root["Angle"].value_or(std::string("First")), Axis = Root["SectionAxis"].value_or(std::string("Y"));
    bool Known = false;
    for (int Index = 0; Index < 5; ++Index)
    {
        if (Paper == GeometryProcessing::DrawingPaperLabel(static_cast<GeometryProcessing::DrawingPaperSize>(Index))) { Settings.Paper = static_cast<GeometryProcessing::DrawingPaperSize>(Index); Known = true; }
    }
    if (!Known) return "DrawingSheet: unknown paper " + Paper + " (A4 … A0)";
    if (Angle != "First" && Angle != "Third") return "DrawingSheet: unknown projection angle " + Angle + " (First / Third)";
    Settings.Angle = Angle == "First" ? GeometryProcessing::DrawingProjectionAngle::First : GeometryProcessing::DrawingProjectionAngle::Third;
    Settings.Scale = Root["Scale"].value_or(0.0);
    if (!(Settings.Scale >= 0.0)) return "DrawingSheet: the scale must be 0 (automatic) or positive";
    static const char* Axes[] = { "X", "Y", "Z", "Face" };
    Known = false;
    for (int Index = 0; Index < 4; ++Index) if (Axis == Axes[Index]) { Settings.Section.Axis = static_cast<SolidArcSectionAxis>(Index); Known = true; }
    if (!Known) return "DrawingSheet: unknown section axis " + Axis + " (X / Y / Z / Face)";
    Settings.IsometricEnabled    = Root["Isometric"].value_or(true);
    Settings.SectionEnabled      = Root["Section"].value_or(false);
    Settings.Section.Offset      = Root["SectionOffset"].value_or(0.0);
    Settings.Section.Flipped     = Root["SectionFlipped"].value_or(false);
    Settings.Section.Face        = Root["SectionFace"].value_or(std::string{});
    Settings.HiddenLinesEnabled  = Root["HiddenLines"].value_or(true);
    Settings.TangentEdgesEnabled = Root["TangentEdges"].value_or(true);
    Settings.DimensionsEnabled   = Root["Dimensions"].value_or(true);
    SolidArcTitleFields& Title = Settings.Title;
    const SolidArcTitleFields Defaults;
    Title.Title         = Root["Title"].value_or(Defaults.Title);
    Title.DrawingNumber = Root["DrawingNumber"].value_or(Defaults.DrawingNumber);
    Title.Author        = Root["Author"].value_or(Defaults.Author);
    Title.Date          = Root["Date"].value_or(Defaults.Date);
    Title.Material      = Root["Material"].value_or(Defaults.Material);
    Title.Revision      = Root["Revision"].value_or(Defaults.Revision);
    Title.Company       = Root["Company"].value_or(Defaults.Company);
    return {};
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

std::string EncodeSolidArcDocument(const SolidArcDocument& Document)
{
    toml::array Parameters;
    for (const FeatureParameter& Parameter : Document.Tree.Parameters)
    {
        Parameters.push_back(toml::table{ { "Name", Parameter.Name }, { "Value", Parameter.Value }, { "Low", Parameter.Low },
                                          { "High", Parameter.High }, { "Description", Parameter.Description } });
    }
    toml::array Steps;
    for (const FeatureStep& Step : Document.Tree.Steps) Steps.push_back(EncodeStep(Step, Document.Captions));
    toml::table Root{ { "Format", "SolidArc" }, { "FormatVersion", FormatVersion }, { "Title", Document.Title },
                      { "Description", Document.Tree.Description }, { "Parameters", std::move(Parameters) }, { "Steps", std::move(Steps) } };
    if (!Document.ImagePlanes.empty()) Root.insert("ImagePlanes", EncodeSolidArcImagePlanes(Document.ImagePlanes));
    if (!DefaultAppearanceCondition(Document.Appearance)) Root.insert("Appearance", EncodeAppearance(Document.Appearance));
    if (!DefaultSolidArcDrawingSheetCondition(Document.DrawingSheet)) Root.insert("DrawingSheet", EncodeDrawingSheet(Document.DrawingSheet));
    std::ostringstream Stream;
    Stream << "# SolidArc document — feature tree of GeometryProcessing (regenerated on load)\n";
    Stream << Root << "\n";
    return Stream.str();
}

SolidArcDocumentFile DecodeSolidArcDocument(const std::string& Text)
{
    SolidArcDocumentFile File;
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
    // 📝 "DraftingWorkspace" is the format name before the SolidArc rename (2026-10-23): those documents still open.
    const std::string Format = Root["Format"].value_or(std::string{});
    if (Format != "SolidArc" && Format != "DraftingWorkspace") { File.Refusal = "not a SolidArc document"; return File; }
    if (Root["FormatVersion"].value_or(int64_t{ 0 }) > FormatVersion) { File.Refusal = "document written by a newer SolidArc"; return File; }
    File.Title            = Root["Title"].value_or(std::string{ "Untitled" });
    File.Tree.Name        = File.Title;
    File.Tree.Description = Root["Description"].value_or(std::string{});
    if (std::string Refusal = DecodeSolidArcImagePlanes(Root["ImagePlanes"].as_array(), 1.0, File.ImagePlanes); !Refusal.empty()) { File.Refusal = Refusal; return File; }
    if (std::string Refusal = DecodeAppearance(Root["Appearance"].as_table(), File.Appearance); !Refusal.empty()) { File.Refusal = Refusal; return File; }
    if (std::string Refusal = DecodeDrawingSheet(Root["DrawingSheet"].as_table(), File.DrawingSheet); !Refusal.empty()) { File.Refusal = Refusal; return File; }
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
            if (Kind < 0 || Kind > static_cast<int64_t>(FeatureStepKind::Import)) { File.Refusal = "a step has an unknown kind " + std::to_string(Kind) + " (newer SolidArc?)"; return File; }
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
            const int64_t Combination = View["Combination"].value_or(int64_t{ 0 });
            if (Combination < 0 || Combination > static_cast<int64_t>(FeatureCombination::Separate))
            {
                File.Refusal = Step.Label + " has an unknown combination " + std::to_string(Combination);
                return File;
            }
            Step.Combination    = static_cast<FeatureCombination>(Combination);
            Step.Target         = View["Target"].value_or(std::string{});
            Step.Tool           = View["Tool"].value_or(std::string{});
            Step.AxisIdentifier = static_cast<int>(View["AxisIdentifier"].value_or(int64_t{ -1 }));
            Step.Angle          = DecodeValue(View["Angle"], Step.Angle);
            const auto Planar = [&](const char* Key, Vector2 Fallback)
            {
                const toml::array* Pair = View[Key].as_array();
                if (!Pair || Pair->size() != 2) return Fallback;
                return Vector2{ (*Pair)[0].value_or(Fallback.X), (*Pair)[1].value_or(Fallback.Y) };
            };
            Step.AxisOrigin     = Planar("AxisOrigin", Step.AxisOrigin);
            Step.AxisDirection  = Planar("AxisDirection", Step.AxisDirection);
            Step.Path           = View["Path"].value_or(std::string{});
            Step.Shape          = static_cast<BlendProfile>(View["Shape"].value_or(int64_t{ 0 }));
            Step.Size           = DecodeValue(View["Size"]);
            Step.Operation      = static_cast<FaceOperationKind>(View["Operation"].value_or(int64_t{ 1 }));
            Step.Amount         = DecodeValue(View["Amount"]);
            Step.Placement.ColumnX = DecodeVector(View["PlacementX"], Step.Placement.ColumnX);
            Step.Placement.ColumnY = DecodeVector(View["PlacementY"], Step.Placement.ColumnY);
            Step.Placement.ColumnZ = DecodeVector(View["PlacementZ"], Step.Placement.ColumnZ);
            Step.Placement.Shift   = DecodeVector(View["PlacementShift"], Step.Placement.Shift);
            Step.Source            = View["Source"].value_or(std::string{});
            if (Step.Kind == FeatureStepKind::Joint)
            {
                const int64_t JointCode = View["JointKind"].value_or(int64_t{ 0 });
                if (JointCode < 0 || JointCode > static_cast<int64_t>(JointKind::Ball)) { File.Refusal = Step.Label + " has an unknown joint kind " + std::to_string(JointCode); return File; }
                FeatureJoint& Joint = Step.Joint;
                Joint.Kind             = static_cast<JointKind>(JointCode);
                Joint.Angle            = DecodeValue(View["JointAngle"]);
                Joint.Slide            = DecodeValue(View["JointSlide"]);
                Joint.SecondSlide      = DecodeValue(View["JointSecondSlide"]);
                Joint.Pitch            = DecodeValue(View["JointPitch"]);
                Joint.Roll             = DecodeValue(View["JointRoll"]);
                Joint.Offset           = DecodeValue(View["JointOffset"]);
                Joint.FlippedCondition = View["JointFlipped"].value_or(false);
                const auto Range = [&](const char* Key) -> std::optional<JointRange>
                {
                    const toml::array* Pair = View[Key].as_array();
                    if (!Pair || Pair->size() != 2) return std::nullopt;
                    return JointRange{ (*Pair)[0].value_or(0.0), (*Pair)[1].value_or(0.0) };
                };
                Joint.AngleRange = Range("JointAngleRange");
                Joint.SlideRange = Range("JointSlideRange");
            }
            if (Step.Kind == FeatureStepKind::Import)
            {
                Step.Exchange     = View["Exchange"].value_or(std::string{});
                Step.ExchangeUnit = View["ExchangeUnit"].value_or(1.0);
                if (Step.Exchange.empty()) { File.Refusal = Step.Label + " has no embedded STEP text"; return File; }
            }
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

bool SaveSolidArcDocument(const SolidArcDocument& Document, const std::string& Path)
{
    FRONTIER_TRACE_SCOPE("File", "Save · " + Path);
    // 📝 The folder is created on first save (a fresh install has no autosave folder yet); failure shows as a closed stream.
    const std::filesystem::path Target = GeometryProcessing::ResolveUtf8Path(Path);
    std::error_code Failure;
    if (Target.has_parent_path()) std::filesystem::create_directories(Target.parent_path(), Failure);
    std::ofstream Stream(Target, std::ios::binary);
    if (!Stream) return false;
    Stream << EncodeSolidArcDocument(Document);
    return static_cast<bool>(Stream);
}

SolidArcDocumentFile LoadSolidArcDocumentFile(const std::string& Path)
{
    FRONTIER_TRACE_SCOPE("File", "Load · " + Path);
    std::ifstream Stream(GeometryProcessing::ResolveUtf8Path(Path), std::ios::binary);
    if (!Stream)
    {
        SolidArcDocumentFile File;
        File.Refusal = "cannot open " + Path;
        return File;
    }
    std::ostringstream Text;
    Text << Stream.rdbuf();
    SolidArcDocumentFile File = DecodeSolidArcDocument(Text.str());
    for (SolidArcImagePlane& Plane : File.ImagePlanes) Plane.Path = ResolveSolidArcImagePath(Plane.Path, Path);
    for (SolidArcCustomMatcap& Custom : File.Appearance.CustomMatcaps) Custom.Path = ResolveSolidArcImagePath(Custom.Path, Path);
    return File;
}

} // namespace Frontier::SolidArc
