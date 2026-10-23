//============================================================================================================================================
//                                                           COMMANDEXCHANGE.H
//============================================================================================================================================
// 📦 Serializes the existing scene and gizmo vertex streams for a browser graphics device without rasterizing pixels.

#pragma once

#include "RasterExchange.h"
#include <span>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <memory>
#include <cstring>
#include <unordered_set>
#ifndef __EMSCRIPTEN__
#include "GpuPickingRaster.h"
#include <cstdlib>
#endif

namespace Frontier
{
class CommandExchange final : public RasterExchange
{
public:
#ifndef __EMSCRIPTEN__
    // Optional native own-pointer proofs; browser requests normally supply GPU IDs.
    CommandExchange() { if(std::getenv("SOLIDARC_COMMAND_GPU_PICKING")) NativePicker=CreateEditorRaster(800,600); }
#endif
    uint32_t ExtentX = 800u;
    uint32_t ExtentY = 600u;
    uint32_t Intersection = 0u;
    uint32_t IntersectionX = 0u;
    uint32_t IntersectionY = 0u;
    uint32_t IntersectionWidth = 0u;
    struct IntersectionRun
    {
        uint32_t Start, Length, Identity;
    };
    std::vector<IntersectionRun> Intersections;
    std::string Packet;

    // Content-addressed geometry for the native HTTP route. A surface or line stream is sent once per browser session;
    //    later frames carry only its digest and the client reuses its retained copy. Off by default (the WebAssembly
    //    worker shares memory with the page and has no transport cost). Shared across documents: digests are content keys.
    static inline bool References = false;                                              // [-] enabled for this request
    static inline std::unordered_set<uint64_t> Delivered;                               // [-] digests the client holds
    template<class Values>
    static void Digest(uint64_t& Hash, const Values& Items) noexcept
    {
        const auto* Bytes = reinterpret_cast<const unsigned char*>(Items.data());
        const size_t Size = Items.size() * sizeof(Items[0]);
        size_t I = 0;
        for (; I + 8 <= Size; I += 8)
        {
            uint64_t Word; std::memcpy(&Word, Bytes + I, 8);
            Hash = (Hash ^ Word) * 0x100000001b3ull; Hash ^= Hash >> 29;
        }
        for (; I < Size; ++I) Hash = (Hash ^ Bytes[I]) * 0x100000001b3ull;
        Hash = (Hash ^ Size) * 0x9e3779b97f4a7c15ull;
    }
    // Writes the digest field; true when the client already holds this stream (arrays are then omitted).
    bool Reference(uint64_t Hash) noexcept
    {
        if (!References) return false;
        char Text[17]; std::snprintf(Text, sizeof(Text), "%016llx", static_cast<unsigned long long>(Hash));
        Stream << ",\"geometry\":\"" << Text << '"';
        return !Delivered.insert(Hash).second;
    }

    void Resize(uint32_t Width, uint32_t Height) noexcept override
    {
        if(NativePicker) NativePicker->Resize(Width,Height);
        ExtentX = Width;
        ExtentY = Height;
    }

    uint32_t Width() const noexcept override { return ExtentX; }
    uint32_t Height() const noexcept override { return ExtentY; }

    template<class Values>
    static void EncodeValues(std::ostream& Stream, const Values& Items)
    {
        Stream << '[';
        bool Separator = false;
        for (auto Value : Items)
        {
            if (Separator) Stream << ',';
            Separator = true;
            Stream << Value;
        }
        Stream << ']';
    }

    void BeginTarget(const float Clear[4]) noexcept override
    {
        if(NativePicker) NativePicker->BeginTarget(Clear);
        Stream.str("");
        Stream.clear();
        Stream << std::setprecision(9) << "{\"width\":" << ExtentX << ",\"height\":" << ExtentY << ",\"clear\":";
        EncodeValues(Stream, std::span(Clear, 4));
        Overlay = false;
        Separator = false;
        Count = {};
    }

    void BindView(const ViewRecord& View) noexcept override
    {
        if(NativePicker) NativePicker->BindView(View);
        Stream << ",\"view\":{";
        bool Comma = false;
        auto Field = [&](const char* Name, const auto& Values)
        {
            if (Comma) Stream << ',';
            Comma = true;
            Stream << '"' << Name << "\":";
            EncodeValues(Stream, Values);
        };
        Field("ViewClip", View.ViewClip);
        Field("ClipView", View.ClipView);
        Field("ViewWorld", View.ViewWorld);
        Field("EyePosition", View.EyePosition);
        Field("Viewport", View.Viewport);
        Field("LatticeStyle", View.LatticeStyle);
        Field("LatticeOrigin", View.LatticeOrigin);
        Field("LatticeAxisU", View.LatticeAxisU);
        Field("LatticeAxisV", View.LatticeAxisV);
        Field("Illumination", View.Illumination);
        Field("DepthPolicy", View.DepthPolicy);
        Stream << ",\"PixelAngle\":" << View.PixelAngle << ",\"PixelWorld\":" << View.PixelWorld << "},\"draws\":[";
    }

    void DrawLattice() noexcept override
    {
        if(NativePicker) NativePicker->DrawLattice();
        Prefix("lattice");
        Stream << '}';
    }

    void DrawSurface(const SurfaceStream& Vertices, const DrawRecord& Style) noexcept override
    {
        if(NativePicker) NativePicker->DrawSurface(Vertices,Style);
        Prefix("surface");
        EncodeStyle(Style);
        Count.Triangles += uint32_t(Vertices.Triangles.size() / 3u);
        if (References)
        {
            uint64_t Hash = 0xcbf29ce484222325ull;
            Digest(Hash, Vertices.Positions); Digest(Hash, Vertices.Normals); Digest(Hash, Vertices.Parameters); Digest(Hash, Vertices.Triangles);
            if (Reference(Hash)) { Stream << '}'; return; }
        }
        Stream << ",\"positions\":";
        EncodeValues(Stream, Vertices.Positions);
        Stream << ",\"normals\":";
        EncodeValues(Stream, Vertices.Normals);
        Stream << ",\"parameters\":";
        EncodeValues(Stream, Vertices.Parameters);
        Stream << ",\"indices\":";
        EncodeValues(Stream, Vertices.Triangles);
        Stream << '}';
    }

    void DrawSegments(const SegmentStream& Vertices, const DrawRecord& Style) noexcept override
    {
        if(NativePicker) NativePicker->DrawSegments(Vertices,Style);
        Prefix("line");
        EncodeStyle(Style);
        Count.Segments += Vertices.SegmentCount();
        if (References && Vertices.Endpoints.size() >= 96)                              // small overlays are cheaper inline
        {
            uint64_t Hash = 0x84222325cbf29ce4ull;
            Digest(Hash, Vertices.Endpoints);
            if (Reference(Hash)) { Stream << '}'; return; }
        }
        Stream << ",\"positions\":";
        EncodeValues(Stream, Vertices.Endpoints);
        Stream << '}';
    }

    void DrawPoints(const PointStream& Vertices, const DrawRecord& Style) noexcept override
    {
        if(NativePicker) NativePicker->DrawPoints(Vertices,Style);
        Prefix("point");
        EncodeStyle(Style);
        Stream << ",\"positions\":";
        EncodeValues(Stream, Vertices.Points);
        Stream << '}';
        Count.Points += Vertices.PointCount();
    }

    void BeginOverlay() noexcept override { if(NativePicker)NativePicker->BeginOverlay(); Overlay = true; }

    void EndTarget() noexcept override
    {
        if(NativePicker) NativePicker->EndTarget();
        Stream << "],\"cpuFragments\":0,\"rgbaUploads\":0}";
        Packet = Stream.str();
    }

    RasterImage Readback() const noexcept override { return {}; }
    uint32_t Pick(uint32_t X, uint32_t Y) const noexcept override
    {
        if(NativePicker) return NativePicker->Pick(X,Y);
        if (IntersectionWidth)
        {
            const uint32_t Offset = Y * IntersectionWidth + X;
            auto Item = std::upper_bound(Intersections.begin(), Intersections.end(), Offset,
                [](uint32_t Position, const IntersectionRun& Run) { return Position < Run.Start; });
            if (Item == Intersections.begin()) return 0u;
            --Item;
            return Offset - Item->Start < Item->Length ? Item->Identity : 0u;
        }
        return X == IntersectionX && Y == IntersectionY ? Intersection : 0u;
    }
    float Depth(uint32_t, uint32_t) const noexcept override { return 1.0f; }
    Tally QueryTally() const noexcept override { return Count; }

private:
    std::unique_ptr<RasterExchange> NativePicker;
    std::ostringstream Stream;
    bool Overlay = false;
    bool Separator = false;
    Tally Count = {};

    void Prefix(const char* Kind)
    {
        if (Separator) Stream << ',';
        Separator = true;
        Stream << "{\"kind\":\"" << Kind << "\",\"overlay\":" << (Overlay ? "true" : "false");
    }

    void EncodeStyle(const DrawRecord& Style)
    {
        Stream << ",\"local\":";
        EncodeValues(Stream, Style.LocalWorld);
        Stream << ",\"tint\":";
        EncodeValues(Stream, Style.Tint);
        Stream << ",\"selection\":[" << Style.Highlight << ",0," << Style.LineWidth << ',' << Style.PointSize << ']';
        Stream << ",\"surface\":[" << unsigned(Style.Matcap) << ',' << unsigned(Style.Shading) << ',' << Style.Emissive << ",0]";
        Stream << ",\"dashed\":" << (Style.Dashed ? "true" : "false") << ",\"pick\":" << Style.PickIdentity;
    }
};
}
