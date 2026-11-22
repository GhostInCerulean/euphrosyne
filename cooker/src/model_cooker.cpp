#include "model_cooker.hpp"

#include <algorithm>
#include <cfloat>
#include <meshoptimizer.h>
#include <numeric>

#include <fastgltf/tools.hpp>

#include "blob_writer.hpp"
#include "pack.hpp"

namespace cooker
{
namespace
{
/// Full-precision primitive straight from glTF, before packing and optimization.
struct RawPrimitive
{
    vector<float3> position;
    vector<float3> normal;
    vector<float3> tangent;
    vector<float> tangent_sign;
    vector<float2> uv;
    vector<uint32> indices;
};

optional<uintsize> FindAttribute(const fastgltf::Primitive &prim, string_view name)
{
    auto it = prim.findAttribute(name);
    if (it == prim.attributes.end())
        return nullopt;
    return it->accessorIndex;
}

void GenerateNormals(RawPrimitive &p)
{
    p.normal.assign(p.position.size(), float3(0.0f, 0.0f, 0.0f));
    for (uintsize i = 0; i + 2 < p.indices.size(); i += 3)
    {
        auto a = p.indices[i], b = p.indices[i + 1], c = p.indices[i + 2];
        // Area weighted, so the unnormalized cross product is exactly what we want to accumulate.
        float3 n = cross(p.position[b] - p.position[a], p.position[c] - p.position[a]);
        p.normal[a] = p.normal[a] + n;
        p.normal[b] = p.normal[b] + n;
        p.normal[c] = p.normal[c] + n;
    }
    for (auto &n : p.normal)
    {
        float len2 = dot(n, n);
        n = len2 > 1e-20f ? normalize(n) : float3(0.0f, 0.0f, 1.0f);
    }
}

// Per-triangle UV derivative tangents. Good enough for most content; swap in MikkTSpace if you
// need to match a baker exactly (glTF mandates MikkTSpace when tangents are missing).
void GenerateTangents(RawPrimitive &p)
{
    auto count = p.position.size();
    auto tan = vector<float3>(count, float3(0.0f, 0.0f, 0.0f));
    auto bit = vector<float3>(count, float3(0.0f, 0.0f, 0.0f));
    for (uintsize i = 0; i + 2 < p.indices.size(); i += 3)
    {
        auto a = p.indices[i], b = p.indices[i + 1], c = p.indices[i + 2];
        float3 e1 = p.position[b] - p.position[a];
        float3 e2 = p.position[c] - p.position[a];
        float du1 = p.uv[b].x - p.uv[a].x, dv1 = p.uv[b].y - p.uv[a].y;
        float du2 = p.uv[c].x - p.uv[a].x, dv2 = p.uv[c].y - p.uv[a].y;
        float det = du1 * dv2 - du2 * dv1;
        if (std::fabs(det) < 1e-20f)
            continue;
        float r = 1.0f / det;
        float3 t = (e1 * dv2 - e2 * dv1) * r;
        float3 bt = (e2 * du1 - e1 * du2) * r;
        for (auto v : {a, b, c})
        {
            tan[v] = tan[v] + t;
            bit[v] = bit[v] + bt;
        }
    }

    p.tangent.resize(count);
    p.tangent_sign.resize(count);
    for (uintsize i = 0; i < count; i++)
    {
        const float3 &n = p.normal[i];
        float3 t = tan[i] - n * float(dot(n, tan[i]));
        float len2 = dot(t, t);
        if (len2 < 1e-20f)
        {
            // Degenerate UVs: any vector perpendicular to the normal will do.
            float3 axis = std::fabs(float(n.x)) < 0.9f ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 1.0f, 0.0f);
            t = cross(n, axis);
        }
        p.tangent[i] = normalize(t);
        float handed = dot(cross(n, p.tangent[i]), bit[i]);
        p.tangent_sign[i] = handed < 0.0f ? -1.0f : 1.0f;
    }
}

optional<RawPrimitive> ReadPrimitive(CookContext &ctx, const fastgltf::Primitive &prim, string_view mesh_name)
{
    auto &gltf = ctx.Gltf();
    if (prim.type != fastgltf::PrimitiveType::Triangles)
    {
        ctx.Warn("mesh '" + string(mesh_name) + "': skipping non-triangle primitive (strips/fans/lines unsupported)");
        return nullopt;
    }
    auto position = FindAttribute(prim, "POSITION");
    if (!position)
        throw runtime_error("Primitive without POSITION in mesh " + string(mesh_name));

    auto raw = RawPrimitive();
    const auto &pos_acc = gltf.accessors[*position];
    auto count = pos_acc.count;
    raw.position.resize(count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        gltf, pos_acc, [&](fastgltf::math::fvec3 v, uintsize i) { raw.position[i] = float3(v[0], v[1], v[2]); });

    // Accessors are read through fastgltf so normalized/quantized and sparse data come out as float.
    if (auto a = FindAttribute(prim, "NORMAL"))
    {
        raw.normal.resize(count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            gltf, gltf.accessors[*a], [&](fastgltf::math::fvec3 v, uintsize i) {
                raw.normal[i] = normalize(float3(v[0], v[1], v[2]));
            });
    }
    if (auto a = FindAttribute(prim, "TEXCOORD_0"))
    {
        raw.uv.resize(count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
            gltf, gltf.accessors[*a], [&](fastgltf::math::fvec2 v, uintsize i) { raw.uv[i] = float2(v[0], v[1]); });
    }
    auto tangent_acc = FindAttribute(prim, "TANGENT");
    if (tangent_acc)
    {
        raw.tangent.resize(count);
        raw.tangent_sign.resize(count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
            gltf, gltf.accessors[*tangent_acc], [&](fastgltf::math::fvec4 v, uintsize i) {
                raw.tangent[i] = float3(v[0], v[1], v[2]);
                raw.tangent_sign[i] = v[3] < 0.0f ? -1.0f : 1.0f;
            });
    }

    if (prim.indicesAccessor)
    {
        const auto &acc = gltf.accessors[*prim.indicesAccessor];
        raw.indices.reserve(acc.count);
        fastgltf::iterateAccessor<uint32>(gltf, acc, [&](uint32 v) { raw.indices.push_back(v); });
    }
    else
    {
        raw.indices.resize(count);
        iota(raw.indices.begin(), raw.indices.end(), 0u);
    }

    // The runtime trusts cooked data, so every bad index must die here.
    if (raw.indices.size() % 3 != 0)
        throw runtime_error("Index count not a multiple of 3 in mesh " + string(mesh_name));
    for (auto index : raw.indices)
        if (index >= count)
            throw runtime_error("Index out of range in mesh " + string(mesh_name));

    if (raw.normal.empty())
        GenerateNormals(raw);
    if (raw.uv.empty())
        raw.uv.assign(count, float2(0.0f, 0.0f));
    if (raw.tangent.empty())
    {
        if (tangent_acc || !FindAttribute(prim, "TEXCOORD_0"))
        {
            raw.tangent.assign(count, float3(1.0f, 0.0f, 0.0f));
            raw.tangent_sign.assign(count, 1.0f);
        }
        else
            GenerateTangents(raw);
    }
    return raw;
}

ModelVertex Pack(const RawPrimitive &p, uintsize i)
{
    auto v = ModelVertex();
    memset(&v, 0, sizeof(v)); // welding compares raw bytes
    v.position[0] = p.position[i].x;
    v.position[1] = p.position[i].y;
    v.position[2] = p.position[i].z;
    OctEncode(p.normal[i].x, p.normal[i].y, p.normal[i].z, v.normal);
    OctEncode(p.tangent[i].x, p.tangent[i].y, p.tangent[i].z, v.tangent);
    v.tangent_sign = PackSnorm16(p.tangent_sign[i]);
    v.uv0[0] = p.uv[i].x;
    v.uv0[1] = p.uv[i].y;
    return v;
}

void Optimize(vector<ModelVertex> &vertices, vector<uint32> &indices, const vector<SubMesh> &submeshes)
{
    // Weld identical packed vertices (also across primitives).
    auto remap = vector<uint32>(vertices.size());
    auto unique = meshopt_generateVertexRemap(remap.data(), indices.data(), indices.size(), vertices.data(),
                                              vertices.size(), sizeof(ModelVertex));
    meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());
    auto welded = vector<ModelVertex>(unique);
    meshopt_remapVertexBuffer(welded.data(), vertices.data(), vertices.size(), sizeof(ModelVertex), remap.data());
    vertices = std::move(welded);

    // Cache optimization per submesh so index ranges stay valid.
    for (const auto &sub : submeshes)
    {
        auto source = vector<uint32>(indices.begin() + sub.first_index,
                                     indices.begin() + sub.first_index + sub.index_count);
        meshopt_optimizeVertexCache(indices.data() + sub.first_index, source.data(), source.size(), vertices.size());
    }

    // Reorder vertices by first use for fetch locality. TODO: meshopt_optimizeOverdraw if it pays off.
    auto fetched = vector<ModelVertex>(vertices.size());
    auto used = meshopt_optimizeVertexFetch(fetched.data(), indices.data(), indices.size(), vertices.data(),
                                            vertices.size(), sizeof(ModelVertex));
    fetched.resize(used);
    vertices = std::move(fetched);
}

void Grow(float *mn, float *mx, const float3 &p)
{
    float v[3] = {p.x, p.y, p.z};
    for (uintsize i = 0; i < 3; i++)
    {
        mn[i] = std::min(mn[i], v[i]);
        mx[i] = std::max(mx[i], v[i]);
    }
}
} // namespace

CookedModel CookModel(CookContext &ctx, uintsize mesh_index)
{
    const auto &mesh = ctx.Gltf().meshes[mesh_index];
    auto mesh_name = string(mesh.name);

    auto vertices = vector<ModelVertex>();
    auto indices = vector<uint32>();
    auto submeshes = vector<SubMesh>();
    float model_min[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float model_max[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto &prim : mesh.primitives)
    {
        auto raw = ReadPrimitive(ctx, prim, mesh_name);
        if (!raw)
            continue;

        auto sub = SubMesh();
        memset(&sub, 0, sizeof(sub));
        sub.first_index = ToU32(indices.size());
        sub.index_count = ToU32(raw->indices.size());
        sub.material =
            ctx.RequireMaterial(prim.materialIndex ? optional<uintsize>(*prim.materialIndex) : nullopt);
        for (uintsize i = 0; i < 3; i++)
        {
            sub.aabb_min[i] = FLT_MAX;
            sub.aabb_max[i] = -FLT_MAX;
        }

        auto base = static_cast<uint32>(vertices.size());
        for (uintsize i = 0; i < raw->position.size(); i++)
        {
            Grow(sub.aabb_min, sub.aabb_max, raw->position[i]);
            vertices.push_back(Pack(*raw, i));
        }
        for (auto index : raw->indices)
            indices.push_back(base + index);

        for (uintsize i = 0; i < 3; i++)
        {
            model_min[i] = std::min(model_min[i], sub.aabb_min[i]);
            model_max[i] = std::max(model_max[i], sub.aabb_max[i]);
        }
        submeshes.push_back(sub);
    }
    if (submeshes.empty())
        throw runtime_error("Mesh '" + mesh_name + "' has no triangle primitives");

    Optimize(vertices, indices, submeshes);

    auto index32 = vertices.size() > 0xFFFF;
    auto blob = BlobWriter<ModelHeader>(kModelMagic, kModelVersion);
    auto submesh_range = blob.Append(submeshes);
    auto vertex_range = blob.Append(vertices);
    Range index_range;
    if (index32)
        index_range = blob.Append(indices);
    else
    {
        auto narrow = vector<uint16>(indices.begin(), indices.end());
        index_range = blob.Append(narrow);
    }

    auto &h = blob.Head();
    h.flags = index32 ? kModelIndex32 : 0;
    h.submeshes = submesh_range;
    h.vertices = vertex_range;
    h.indices = index_range;
    memcpy(h.aabb_min, model_min, sizeof(model_min));
    memcpy(h.aabb_max, model_max, sizeof(model_max));

    auto cooked = CookedModel();
    memcpy(cooked.aabb_min, model_min, sizeof(model_min));
    memcpy(cooked.aabb_max, model_max, sizeof(model_max));
    cooked.data = blob.Finish();
    return cooked;
}
} // namespace cooker
