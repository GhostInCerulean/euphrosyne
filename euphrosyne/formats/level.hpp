#pragma once
#include "euphrosyne/formats/asset.hpp"

namespace euphrosyne
{
static constexpr uint32 kLevelMagic = MakeTag("LEVL");
static constexpr uint32 kLevelVersion = 0;

/// A level is a header plus a table of typed sections. Adding a feature = new enum value + new
/// element struct + a cooker hook; old levels keep loading because unused sections are {0, 0}.
enum class LevelSection : uint32
{
    Instances,
    Lights, // reserved: KHR_lights_punctual / baked lighting, not cooked yet
    // Future: Probes, Colliders, Entities (node extras), Cameras...
    Count,
};
static constexpr uint32 kLevelMaxSections = 8;
static_assert(static_cast<uint32>(LevelSection::Count) <= kLevelMaxSections);

struct LevelInstance
{
    uint64 model; // hash of the model entry
    /// glTF memory layout: column-major, column vectors. With hlslpp's default row-vector
    /// convention this exact memory reads as float4x4(w[0], w[1], ..., w[15]).
    float world[16];
    float aabb_min[3]; // world space
    float aabb_max[3];
    uint32 flags;
    uint32 source_node; // glTF node index, key for attaching gameplay data later
};
static_assert(sizeof(LevelInstance) == 104);
static_assert(is_trivially_copyable_v<LevelInstance>);

struct LevelHeader
{
    AssetHeader header;
    float aabb_min[3];
    float aabb_max[3];
    Range sections[kLevelMaxSections];
};
static_assert(sizeof(LevelHeader) == 104);

inline float4x4 LevelInstanceWorld(const LevelInstance &i)
{
    const auto *w = i.world;
    return float4x4(w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8], w[9], w[10], w[11], w[12], w[13], w[14],
                    w[15]);
}

class LevelView
{
  public:
    static optional<LevelView> Parse(span<const uint8> asset)
    {
        if (!ValidateAssetHeader(asset, kLevelMagic, kLevelVersion, sizeof(LevelHeader)))
            return nullopt;
        auto view = LevelView();
        view.asset_ = asset;
        view.header_ = reinterpret_cast<const LevelHeader *>(asset.data());
        auto instances = view.Section<LevelInstance>(LevelSection::Instances);
        if (!instances)
            return nullopt;
        view.instances_ = *instances;
        return view;
    }

    const LevelHeader &Header() const { return *header_; }
    span<const LevelInstance> Instances() const { return instances_; }

    /// Validated access to any section; T must match what the cooker wrote for that section.
    template <class T> optional<span<const T>> Section(LevelSection section) const
    {
        return ResolveRange<T>(asset_, header_->sections[static_cast<uint32>(section)]);
    }

  private:
    span<const uint8> asset_;
    const LevelHeader *header_ = nullptr;
    span<const LevelInstance> instances_;
};
} // namespace euphrosyne
