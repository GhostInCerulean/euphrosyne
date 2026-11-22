#include "level_cooker.hpp"

#include <algorithm>
#include <cfloat>

#include <fastgltf/tools.hpp>

#include "blob_writer.hpp"

namespace cooker
{
namespace
{
// glTF matrices are column-major: m[column][row].
void TransformAabb(const fastgltf::math::fmat4x4 &m, const float *mn, const float *mx, float *out_min, float *out_max)
{
    for (uintsize i = 0; i < 3; i++)
    {
        out_min[i] = FLT_MAX;
        out_max[i] = -FLT_MAX;
    }
    for (uintsize corner = 0; corner < 8; corner++)
    {
        float x = (corner & 1) ? mx[0] : mn[0];
        float y = (corner & 2) ? mx[1] : mn[1];
        float z = (corner & 4) ? mx[2] : mn[2];
        for (uintsize r = 0; r < 3; r++)
        {
            float v = m[0][r] * x + m[1][r] * y + m[2][r] * z + m[3][r];
            out_min[r] = std::min(out_min[r], v);
            out_max[r] = std::max(out_max[r], v);
        }
    }
}
} // namespace

uint64 LevelCooker::Cook(CookContext &ctx, uintsize scene) const
{
    auto &gltf = ctx.Gltf();
    if (scene >= gltf.scenes.size())
        throw runtime_error("Scene index out of range");

    auto builder = LevelBuilder();
    fastgltf::iterateSceneNodes(
        gltf, scene, fastgltf::math::fmat4x4(1.0f), [&](fastgltf::Node &node, const fastgltf::math::nmat4x4 &world) {
            auto node_index = static_cast<uintsize>(&node - gltf.nodes.data());

            if (node.meshIndex)
            {
                const auto &model = ctx.RequireModel(*node.meshIndex);
                auto instance = LevelInstance();
                memset(&instance, 0, sizeof(instance));
                instance.model = model.hash;
                instance.source_node = ToU32(node_index);
                for (uintsize c = 0; c < 4; c++)
                    for (uintsize r = 0; r < 4; r++)
                        instance.world[c * 4 + r] = world[c][r];
                TransformAabb(world, model.aabb_min, model.aabb_max, instance.aabb_min, instance.aabb_max);
                builder.instances.push_back(instance);
            }

            for (const auto &hook : hooks_)
                hook(ctx, node_index, world, builder);
        });

    float level_min[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float level_max[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (const auto &i : builder.instances)
        for (uintsize k = 0; k < 3; k++)
        {
            level_min[k] = std::min(level_min[k], i.aabb_min[k]);
            level_max[k] = std::max(level_max[k], i.aabb_max[k]);
        }
    if (builder.instances.empty())
        for (uintsize k = 0; k < 3; k++)
            level_min[k] = level_max[k] = 0.0f;

    auto blob = BlobWriter<LevelHeader>(kLevelMagic, kLevelVersion);
    auto instances = blob.Append(builder.instances);
    auto &h = blob.Head();
    memcpy(h.aabb_min, level_min, sizeof(level_min));
    memcpy(h.aabb_max, level_max, sizeof(level_max));
    h.sections[static_cast<uint32>(LevelSection::Instances)] = instances;
    // TODO(lighting): h.sections[LevelSection::Lights] = blob.Append(builder.lights);

    auto name = string(gltf.scenes[scene].name);
    return ctx.Emit(ctx.Name("level", name.empty() ? "scene" + to_string(scene) : name), blob.Finish());
}
} // namespace cooker
