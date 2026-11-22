#pragma once
#include <functional>

#include "cook_context.hpp"
#include "euphrosyne/formats/level.hpp"

namespace cooker
{
/// What cooking collects before it is serialized. To add a level feature: add a vector here, add a
/// LevelSection + element struct in level.hpp, write the section in LevelCooker::Cook, and fill it
/// from a node hook. Nothing else in the pipeline changes.
struct LevelBuilder
{
    vector<LevelInstance> instances;
    // TODO(lighting): vector<LevelLight> lights; filled from KHR_lights_punctual nodes and/or baked data.
};

class LevelCooker
{
  public:
    /// Called for every node of the scene (after the built-in mesh handling) with its world matrix.
    /// This is where lights, colliders, spawn points (node extras), cameras... plug in.
    using NodeHook = function<void(CookContext &, uintsize node, const fastgltf::math::fmat4x4 &world, LevelBuilder &)>;

    void AddNodeHook(NodeHook hook) { hooks_.push_back(std::move(hook)); }

    /// Returns the archive hash of the emitted level.
    uint64 Cook(CookContext &ctx, uintsize scene) const;

  private:
    vector<NodeHook> hooks_;
};
} // namespace cooker
