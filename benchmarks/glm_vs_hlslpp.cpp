// hlslpp vs glm micro-benchmarks (Google Benchmark).
//
// Every benchmark is written once as a template over a "Lib" traits struct,
// so both libraries run exactly the same loop, the same data and the same
// harness code. Only the math calls differ.
//
// Each iteration processes a whole batch (kVecCount vectors / kMatCount
// matrices) that fits in L1, so the numbers reflect throughput of the math,
// not memory bandwidth. Items/s is reported, so compare "items_per_second".

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

#include <glm/glm.hpp>
#include <hlsl++.h>

namespace {

    constexpr size_t kVecCount = 256;
    constexpr size_t kMatCount = 64;

    // ---------------------------------------------------------------------
    // Library traits
    // ---------------------------------------------------------------------

    struct GlmLib {
        static constexpr const char *kName = "glm";
        using Vec3 = glm::vec3;
        using Vec4 = glm::vec4;
        using Mat4 = glm::mat4;

        static Vec3 MakeVec3(float x, float y, float z) { return {x, y, z}; }
        static Vec4 MakeVec4(float x, float y, float z, float w) { return {x, y, z, w}; }
        // f = 16 floats, treated as 4 vectors (columns here, rows in hlslpp;
        // irrelevant for benchmarking as the data is diagonally dominant).
        static Mat4 MakeMat4(const float *f) {
            return {Vec4(f[0], f[1], f[2], f[3]), Vec4(f[4], f[5], f[6], f[7]),
                    Vec4(f[8], f[9], f[10], f[11]), Vec4(f[12], f[13], f[14], f[15])};
        }

        static auto Dot3(const Vec3 &a, const Vec3 &b) { return glm::dot(a, b); }
        static auto Dot4(const Vec4 &a, const Vec4 &b) { return glm::dot(a, b); }
        static auto Cross(const Vec3 &a, const Vec3 &b) { return glm::cross(a, b); }
        static auto Length3(const Vec3 &a) { return glm::length(a); }
        static auto Normalize3(const Vec3 &a) { return glm::normalize(a); }
        static auto Lerp4(const Vec4 &a, const Vec4 &b, const Vec4 &t) { return glm::mix(a, b, t); }
        static auto MatMul(const Mat4 &a, const Mat4 &b) { return a * b; }
        static auto Transform(const Mat4 &m, const Vec4 &v) { return m * v; }
        static auto Transpose(const Mat4 &m) { return glm::transpose(m); }
        static auto Inverse(const Mat4 &m) { return glm::inverse(m); }
    };

    struct HlslppLib {
        static constexpr const char *kName = "hlslpp";
        using Vec3 = hlslpp::float3;
        using Vec4 = hlslpp::float4;
        using Mat4 = hlslpp::float4x4;

        static Vec3 MakeVec3(float x, float y, float z) { return {x, y, z}; }
        static Vec4 MakeVec4(float x, float y, float z, float w) { return {x, y, z, w}; }
        static Mat4 MakeMat4(const float *f) {
            return {Vec4(f[0], f[1], f[2], f[3]), Vec4(f[4], f[5], f[6], f[7]),
                    Vec4(f[8], f[9], f[10], f[11]), Vec4(f[12], f[13], f[14], f[15])};
        }

        // Note: results stay in hlslpp's native types (float1 is a SIMD register),
        // which is how you'd use them in real code.
        static auto Dot3(const Vec3 &a, const Vec3 &b) { return hlslpp::dot(a, b); }
        static auto Dot4(const Vec4 &a, const Vec4 &b) { return hlslpp::dot(a, b); }
        static auto Cross(const Vec3 &a, const Vec3 &b) { return hlslpp::cross(a, b); }
        static auto Length3(const Vec3 &a) { return hlslpp::length(a); }
        static auto Normalize3(const Vec3 &a) { return hlslpp::normalize(a); }
        static auto Lerp4(const Vec4 &a, const Vec4 &b, const Vec4 &t) { return hlslpp::lerp(a, b, t); }
        // operator* on hlslpp matrices is component-wise, so use mul() for a real matrix product.
        static auto MatMul(const Mat4 &a, const Mat4 &b) { return hlslpp::mul(a, b); }
        // hlslpp is row-vector convention: v * M.
        static auto Transform(const Mat4 &m, const Vec4 &v) { return hlslpp::mul(v, m); }
        static auto Transpose(const Mat4 &m) { return hlslpp::transpose(m); }
        static auto Inverse(const Mat4 &m) { return hlslpp::inverse(m); }
    };

    // ---------------------------------------------------------------------
    // Deterministic input data
    // ---------------------------------------------------------------------

    std::vector<float> RandomFloats(size_t n, uint32_t seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(0.5f, 2.0f); // never zero: safe for normalize/inverse
        std::vector<float> v(n);
        for (auto &x: v) x = dist(rng);
        return v;
    }

    template<class L>
    std::vector<typename L::Vec3> MakeVec3s(size_t n, uint32_t seed) {
        auto f = RandomFloats(n * 3, seed);
        std::vector<typename L::Vec3> v;
        v.reserve(n);
        for (size_t i = 0; i < n; ++i) v.push_back(L::MakeVec3(f[i * 3], f[i * 3 + 1], f[i * 3 + 2]));
        return v;
    }

    template<class L>
    std::vector<typename L::Vec4> MakeVec4s(size_t n, uint32_t seed) {
        auto f = RandomFloats(n * 4, seed);
        std::vector<typename L::Vec4> v;
        v.reserve(n);
        for (size_t i = 0; i < n; ++i) v.push_back(L::MakeVec4(f[i * 4], f[i * 4 + 1], f[i * 4 + 2], f[i * 4 + 3]));
        return v;
    }

    // Diagonally dominant => always invertible and well conditioned.
    template<class L>
    std::vector<typename L::Mat4> MakeMat4s(size_t n, uint32_t seed) {
        auto f = RandomFloats(n * 16, seed);
        std::vector<typename L::Mat4> v;
        v.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            float *m = &f[i * 16];
            m[0] += 4.0f;
            m[5] += 4.0f;
            m[10] += 4.0f;
            m[15] += 4.0f;
            v.push_back(L::MakeMat4(m));
        }
        return v;
    }

    // ---------------------------------------------------------------------
    // Harness: out[i] = fn(inputs[i]...) over a batch, per benchmark iteration
    // ---------------------------------------------------------------------

    template<class A, class Fn>
    void Run1(benchmark::State &state, const std::vector<A> &a, Fn fn) {
        using R = std::decay_t<decltype(fn(a[0]))>;
        const size_t n = a.size();
        std::vector<R> out(n);
        const A *pa = a.data();
        R *po = out.data();
        benchmark::DoNotOptimize(pa);
        benchmark::DoNotOptimize(po);
        for (auto _: state) {
            for (size_t i = 0; i < n; ++i) po[i] = fn(pa[i]);
            benchmark::DoNotOptimize(po);
            benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
    }

    template<class A, class B, class Fn>
    void Run2(benchmark::State &state, const std::vector<A> &a, const std::vector<B> &b, Fn fn) {
        using R = std::decay_t<decltype(fn(a[0], b[0]))>;
        const size_t n = a.size();
        std::vector<R> out(n);
        const A *pa = a.data();
        const B *pb = b.data();
        R *po = out.data();
        benchmark::DoNotOptimize(pa);
        benchmark::DoNotOptimize(pb);
        benchmark::DoNotOptimize(po);
        for (auto _: state) {
            for (size_t i = 0; i < n; ++i) po[i] = fn(pa[i], pb[i]);
            benchmark::DoNotOptimize(po);
            benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
    }

    template<class A, class B, class C, class Fn>
    void Run3(benchmark::State &state, const std::vector<A> &a, const std::vector<B> &b,
              const std::vector<C> &c, Fn fn) {
        using R = std::decay_t<decltype(fn(a[0], b[0], c[0]))>;
        const size_t n = a.size();
        std::vector<R> out(n);
        const A *pa = a.data();
        const B *pb = b.data();
        const C *pc = c.data();
        R *po = out.data();
        benchmark::DoNotOptimize(pa);
        benchmark::DoNotOptimize(pb);
        benchmark::DoNotOptimize(pc);
        benchmark::DoNotOptimize(po);
        for (auto _: state) {
            for (size_t i = 0; i < n; ++i) po[i] = fn(pa[i], pb[i], pc[i]);
            benchmark::DoNotOptimize(po);
            benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
    }

    // ---------------------------------------------------------------------
    // Vector benchmarks
    // ---------------------------------------------------------------------

    template<class L>
    void Add4(benchmark::State &s) {
        auto a = MakeVec4s<L>(kVecCount, 1), b = MakeVec4s<L>(kVecCount, 2);
        Run2(s, a, b, [](const auto &x, const auto &y) { return x + y; });
    }

    template<class L>
    void Mad4(benchmark::State &s) {
        auto a = MakeVec4s<L>(kVecCount, 1), b = MakeVec4s<L>(kVecCount, 2), c = MakeVec4s<L>(kVecCount, 3);
        Run3(s, a, b, c, [](const auto &x, const auto &y, const auto &z) { return x * y + z; });
    }

    template<class L>
    void Lerp4(benchmark::State &s) {
        auto a = MakeVec4s<L>(kVecCount, 1), b = MakeVec4s<L>(kVecCount, 2), t = MakeVec4s<L>(kVecCount, 3);
        Run3(s, a, b, t, [](const auto &x, const auto &y, const auto &z) { return L::Lerp4(x, y, z); });
    }

    template<class L>
    void Dot3(benchmark::State &s) {
        auto a = MakeVec3s<L>(kVecCount, 1), b = MakeVec3s<L>(kVecCount, 2);
        Run2(s, a, b, [](const auto &x, const auto &y) { return L::Dot3(x, y); });
    }

    template<class L>
    void Dot4(benchmark::State &s) {
        auto a = MakeVec4s<L>(kVecCount, 1), b = MakeVec4s<L>(kVecCount, 2);
        Run2(s, a, b, [](const auto &x, const auto &y) { return L::Dot4(x, y); });
    }

    template<class L>
    void Cross3(benchmark::State &s) {
        auto a = MakeVec3s<L>(kVecCount, 1), b = MakeVec3s<L>(kVecCount, 2);
        Run2(s, a, b, [](const auto &x, const auto &y) { return L::Cross(x, y); });
    }

    template<class L>
    void Length3(benchmark::State &s) {
        auto a = MakeVec3s<L>(kVecCount, 1);
        Run1(s, a, [](const auto &x) { return L::Length3(x); });
    }

    template<class L>
    void Normalize3(benchmark::State &s) {
        auto a = MakeVec3s<L>(kVecCount, 1);
        Run1(s, a, [](const auto &x) { return L::Normalize3(x); });
    }

    // ---------------------------------------------------------------------
    // Matrix benchmarks
    // ---------------------------------------------------------------------

    template<class L>
    void Mat4Mul(benchmark::State &s) {
        auto a = MakeMat4s<L>(kMatCount, 1), b = MakeMat4s<L>(kMatCount, 2);
        Run2(s, a, b, [](const auto &x, const auto &y) { return L::MatMul(x, y); });
    }

    template<class L>
    void Mat4Vec4(benchmark::State &s) {
        auto v = MakeVec4s<L>(kMatCount, 1);
        auto m = MakeMat4s<L>(kMatCount, 2);
        Run2(s, m, v, [](const auto &mat, const auto &vec) { return L::Transform(mat, vec); });
    }

    template<class L>
    void Mat4Transpose(benchmark::State &s) {
        auto a = MakeMat4s<L>(kMatCount, 1);
        Run1(s, a, [](const auto &x) { return L::Transpose(x); });
    }

    template<class L>
    void Mat4Inverse(benchmark::State &s) {
        auto a = MakeMat4s<L>(kMatCount, 1);
        Run1(s, a, [](const auto &x) { return L::Inverse(x); });
    }

} // namespace

BENCHMARK_TEMPLATE(Add4, GlmLib)->Name("GlmVsHlslpp/Add4/glm");
BENCHMARK_TEMPLATE(Add4, HlslppLib)->Name("GlmVsHlslpp/Add4/hlslpp");
BENCHMARK_TEMPLATE(Mad4, GlmLib)->Name("GlmVsHlslpp/Mad4/glm");
BENCHMARK_TEMPLATE(Mad4, HlslppLib)->Name("GlmVsHlslpp/Mad4/hlslpp");
BENCHMARK_TEMPLATE(Lerp4, GlmLib)->Name("GlmVsHlslpp/Lerp4/glm");
BENCHMARK_TEMPLATE(Lerp4, HlslppLib)->Name("GlmVsHlslpp/Lerp4/hlslpp");
BENCHMARK_TEMPLATE(Dot3, GlmLib)->Name("GlmVsHlslpp/Dot3/glm");
BENCHMARK_TEMPLATE(Dot3, HlslppLib)->Name("GlmVsHlslpp/Dot3/hlslpp");
BENCHMARK_TEMPLATE(Dot4, GlmLib)->Name("GlmVsHlslpp/Dot4/glm");
BENCHMARK_TEMPLATE(Dot4, HlslppLib)->Name("GlmVsHlslpp/Dot4/hlslpp");
BENCHMARK_TEMPLATE(Cross3, GlmLib)->Name("GlmVsHlslpp/Cross3/glm");
BENCHMARK_TEMPLATE(Cross3, HlslppLib)->Name("GlmVsHlslpp/Cross3/hlslpp");
BENCHMARK_TEMPLATE(Length3, GlmLib)->Name("GlmVsHlslpp/Length3/glm");
BENCHMARK_TEMPLATE(Length3, HlslppLib)->Name("GlmVsHlslpp/Length3/hlslpp");
BENCHMARK_TEMPLATE(Normalize3, GlmLib)->Name("GlmVsHlslpp/Normalize3/glm");
BENCHMARK_TEMPLATE(Normalize3, HlslppLib)->Name("GlmVsHlslpp/Normalize3/hlslpp");
BENCHMARK_TEMPLATE(Mat4Mul, GlmLib)->Name("GlmVsHlslpp/Mat4Mul/glm");
BENCHMARK_TEMPLATE(Mat4Mul, HlslppLib)->Name("GlmVsHlslpp/Mat4Mul/hlslpp");
BENCHMARK_TEMPLATE(Mat4Vec4, GlmLib)->Name("GlmVsHlslpp/Mat4Vec4/glm");
BENCHMARK_TEMPLATE(Mat4Vec4, HlslppLib)->Name("GlmVsHlslpp/Mat4Vec4/hlslpp");
BENCHMARK_TEMPLATE(Mat4Transpose, GlmLib)->Name("GlmVsHlslpp/Mat4Transpose/glm");
BENCHMARK_TEMPLATE(Mat4Transpose, HlslppLib)->Name("GlmVsHlslpp/Mat4Transpose/hlslpp");
BENCHMARK_TEMPLATE(Mat4Inverse, GlmLib)->Name("GlmVsHlslpp/Mat4Inverse/glm");
BENCHMARK_TEMPLATE(Mat4Inverse, HlslppLib)->Name("GlmVsHlslpp/Mat4Inverse/hlslpp");