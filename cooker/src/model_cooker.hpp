#pragma once
#include "cook_context.hpp"
#include "euphrosyne/formats/model.hpp"

namespace cooker
{
struct CookedModel
{
    vector<uint8> data;
    float aabb_min[3];
    float aabb_max[3];
};

/// One glTF mesh -> one Model; each triangle primitive becomes a SubMesh. Materials are cooked
/// on demand through ctx.RequireMaterial().
CookedModel CookModel(CookContext &ctx, uintsize mesh_index);
} // namespace cooker
