/*==========================================================================================================================================
                                                        NEAREST DEPTH SPACE
                                     ------------------------------------------------------------------
                   The nearest-depth pyramid over the screen — recursively halved tiles, each
                     holding its nearest depth — kept two deep (this frame and the last).
==========================================================================================================================================*/

// 🧩 The nearest-depth pyramid: the portable (CPU) form of the demo's NearestDepthSpace.js and NearestDepthReduction.wgsl, 1:1.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Frontier
{

class NearestDepthSpace
{
  public:
    inline void Configure(uint32_t InWidth, uint32_t InHeight);

    [[nodiscard]] uint32_t LevelCount() const noexcept { return Levels; }
    [[nodiscard]] inline std::array<uint32_t, 2> LevelExtent(uint32_t Level) const noexcept;
    [[nodiscard]] inline std::array<uint32_t, 3> TransferDispatch() const noexcept;
    [[nodiscard]] inline std::array<uint32_t, 3> ReductionDispatch(uint32_t Level) const noexcept;
    [[nodiscard]] uint32_t Parity() const noexcept { return Slot; }
    void AdvanceFrame() noexcept { Slot = 1u - Slot; }

    uint32_t Width = 0;
    uint32_t Height = 0;

    [[nodiscard]] static inline std::vector<std::vector<float>> Reduce(const float* ReversedDepth, uint32_t Width, uint32_t Height);
    [[nodiscard]] static inline bool VerifyBounds(const std::vector<std::vector<float>>& Pyramid,
                                                  uint32_t Width,
                                                  uint32_t Height);

  private:
    uint32_t Levels = 0;
    uint32_t Slot = 0;
    std::vector<std::array<uint32_t, 2>> Extents;
};

inline void NearestDepthSpace::Configure(uint32_t InWidth, uint32_t InHeight)
{
    Width = InWidth;
    Height = InHeight;
    Extents.clear();
    if (Width == 0u || Height == 0u)
    {
        Levels = 0u;
        return;
    }

    const uint32_t MaxDimension = std::max(Width, Height);
    Levels = static_cast<uint32_t>(std::floor(std::log2(static_cast<double>(MaxDimension)))) + 1u;
    Extents.resize(Levels);
    for (uint32_t Level = 0u; Level < Levels; ++Level)
    {
        Extents[Level] = {
            std::max(1u, Width >> Level),
            std::max(1u, Height >> Level)
        };
    }
}

inline std::array<uint32_t, 2> NearestDepthSpace::LevelExtent(uint32_t Level) const noexcept
{
    if (Level >= Extents.size())
    {
        return { 0u, 0u };
    }
    return Extents[Level];
}

inline std::array<uint32_t, 3> NearestDepthSpace::TransferDispatch() const noexcept
{
    return {
        (Width + 7u) / 8u,
        (Height + 7u) / 8u,
        1u
    };
}

inline std::array<uint32_t, 3> NearestDepthSpace::ReductionDispatch(uint32_t Level) const noexcept
{
    const auto Extent = LevelExtent(Level);
    return {
        (Extent[0] + 7u) / 8u,
        (Extent[1] + 7u) / 8u,
        1u
    };
}

inline std::vector<std::vector<float>> NearestDepthSpace::Reduce(const float* ReversedDepth, uint32_t Width, uint32_t Height)
{
    std::vector<std::vector<float>> Pyramid;
    if (!ReversedDepth || Width == 0u || Height == 0u)
    {
        return Pyramid;
    }

    NearestDepthSpace Space;
    Space.Configure(Width, Height);
    Pyramid.resize(Space.LevelCount());
    Pyramid[0].assign(ReversedDepth, ReversedDepth + static_cast<size_t>(Width) * static_cast<size_t>(Height));

    for (uint32_t Level = 1u; Level < Space.LevelCount(); ++Level)
    {
        const auto SourceExtent = Space.LevelExtent(Level - 1u);
        const auto TargetExtent = Space.LevelExtent(Level);
        const auto& Source = Pyramid[Level - 1u];
        auto& Target = Pyramid[Level];
        Target.resize(static_cast<size_t>(TargetExtent[0]) * static_cast<size_t>(TargetExtent[1]), 0.0f);

        for (uint32_t Y = 0u; Y < TargetExtent[1]; ++Y)
        {
            const bool UnpartneredY = ((SourceExtent[1] & 1u) == 1u) && (Y + 1u == TargetExtent[1]);
            const uint32_t SpanY = 2u + (UnpartneredY ? 1u : 0u);
            for (uint32_t X = 0u; X < TargetExtent[0]; ++X)
            {
                const bool UnpartneredX = ((SourceExtent[0] & 1u) == 1u) && (X + 1u == TargetExtent[0]);
                const uint32_t SpanX = 2u + (UnpartneredX ? 1u : 0u);
                float Nearest = 0.0f;
                for (uint32_t Row = 0u; Row < SpanY; ++Row)
                {
                    const uint32_t TexelY = std::min(Y * 2u + Row, SourceExtent[1] - 1u);
                    for (uint32_t Column = 0u; Column < SpanX; ++Column)
                    {
                        const uint32_t TexelX = std::min(X * 2u + Column, SourceExtent[0] - 1u);
                        Nearest = std::max(Nearest, Source[static_cast<size_t>(TexelY) * SourceExtent[0] + TexelX]);
                    }
                }
                Target[static_cast<size_t>(Y) * TargetExtent[0] + X] = Nearest;
            }
        }
    }

    return Pyramid;
}

inline bool NearestDepthSpace::VerifyBounds(const std::vector<std::vector<float>>& Pyramid,
                                            uint32_t Width,
                                            uint32_t Height)
{
    if (Pyramid.empty() || Width == 0u || Height == 0u)
    {
        return false;
    }

    NearestDepthSpace Space;
    Space.Configure(Width, Height);
    if (Pyramid.size() != Space.LevelCount())
    {
        return false;
    }

    for (uint32_t Level = 0u; Level < Space.LevelCount(); ++Level)
    {
        const auto Extent = Space.LevelExtent(Level);
        if (Pyramid[Level].size() != static_cast<size_t>(Extent[0]) * static_cast<size_t>(Extent[1]))
        {
            return false;
        }
    }

    for (uint32_t Level = 1u; Level < Space.LevelCount(); ++Level)
    {
        const auto SourceExtent = Space.LevelExtent(Level - 1u);
        const auto TargetExtent = Space.LevelExtent(Level);
        const auto& Source = Pyramid[Level - 1u];
        const auto& Target = Pyramid[Level];

        for (uint32_t SY = 0u; SY < SourceExtent[1]; ++SY)
        {
            const uint32_t TY = std::min(SY >> 1u, TargetExtent[1] - 1u);
            for (uint32_t SX = 0u; SX < SourceExtent[0]; ++SX)
            {
                const uint32_t TX = std::min(SX >> 1u, TargetExtent[0] - 1u);
                const float Child = Source[static_cast<size_t>(SY) * SourceExtent[0] + SX];
                const float Parent = Target[static_cast<size_t>(TY) * TargetExtent[0] + TX];
                if (Parent < Child)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

} // namespace Frontier
