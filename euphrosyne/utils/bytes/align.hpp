#pragma once
#include <type_traits>


#include "euphrosyne/pch.hpp"

namespace euphrosyne {
    constexpr bool IsPowerOfTwo(uint64_t v) { return v != 0 && (v & (v - 1)) == 0; }

    // Round `value` up to the next multiple of `alignment`.
    template<class T, class A>
    constexpr T AlignTo(T value, A alignment) {
        static_assert(std::is_integral_v<T> && std::is_integral_v<A>, "AlignTo needs integers");
        assert(alignment > 0);
        const T a = static_cast<T>(alignment);
        if (IsPowerOfTwo(static_cast<uint64_t>(a)))
            return static_cast<T>((value + (a - 1)) & ~static_cast<T>(a - 1));
        return static_cast<T>(((value + (a - 1)) / a) * a);
    }

    // Compile-time alignment: the power-of-two check is a static_assert, so no branch is emitted.
    //   AlignTo<256>(size)
    template<auto Alignment, class T>
    constexpr T AlignTo(T value) {
        static_assert(std::is_integral_v<T>, "AlignTo needs an integer");
        static_assert(Alignment > 0 && IsPowerOfTwo(static_cast<uint64_t>(Alignment)),
                      "compile-time alignment must be a power of two");
        constexpr T a = static_cast<T>(Alignment);
        return static_cast<T>((value + (a - 1)) & ~static_cast<T>(a - 1));
    }

    // Round `value` down to a multiple of `alignment`.
    template<class T, class A>
    constexpr T AlignDown(T value, A alignment) {
        static_assert(std::is_integral_v<T> && std::is_integral_v<A>, "AlignDown needs integers");
        assert(alignment > 0);
        const T a = static_cast<T>(alignment);
        if (IsPowerOfTwo(static_cast<uint64_t>(a)))
            return static_cast<T>(value & ~static_cast<T>(a - 1));
        return static_cast<T>((value / a) * a);
    }

    template<class T, class A>
    constexpr bool IsAligned(T value, A alignment) {
        return AlignTo(value, alignment) == value;
    }

    // Bytes of padding needed to reach the next boundary (0 if already aligned).
    template<class T, class A>
    constexpr T AlignPadding(T value, A alignment) {
        return static_cast<T>(AlignTo(value, alignment) - value);
    }

    // Integer division rounding up. Handy for tile/group/page counts.
    template<class T, class D>
    constexpr T DivRoundUp(T value, D divisor) {
        assert(divisor > 0);
        return static_cast<T>((value + (static_cast<T>(divisor) - 1)) / static_cast<T>(divisor));
    }

    // Pointer versions (runtime only, since reinterpret_cast isn't constexpr).
    template<class P, class A>
    inline P *AlignTo(P *p, A alignment) {
        return reinterpret_cast<P *>(AlignTo(reinterpret_cast<uintptr_t>(p), alignment));
    }
    template<class P, class A>
    inline bool IsAligned(const P *p, A alignment) {
        return IsAligned(reinterpret_cast<uintptr_t>(p), alignment);
    }
} // namespace euphrosyne
