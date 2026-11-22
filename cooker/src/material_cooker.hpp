#pragma once
#include "cook_context.hpp"

namespace cooker
{
vector<uint8> CookMaterial(CookContext &ctx, const fastgltf::Material &material);
vector<uint8> CookDefaultMaterial();
} // namespace cooker
