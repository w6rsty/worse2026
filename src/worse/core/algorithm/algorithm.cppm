module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.algorithm;

// Umbrella: re-export every algorithm submodule so callers can pull the whole library
// in with a single `import worse.core.algorithm.algorithm;`. Each `export import` makes
// the imported module's exported names visible to importers of this one. Granular
// imports of the individual submodules remain available for callers who want to keep
// their import surface (and build dependencies) minimal.
export import worse.core.algorithm.heap;
export import worse.core.algorithm.binary_search;
export import worse.core.algorithm.sort;
export import worse.core.algorithm.nonmodifying;
export import worse.core.algorithm.modifying;

import worse.core.utility; // Less<> default comparator for the range overloads

// --- container-range convenience overloads --------------------------------------------------
//
// Thin wrappers that take a whole container (anything exposing begin()/end()) and forward to
// the iterator-pair algorithms above -- so `sort(v)` works alongside `sort(v.begin(), v.end())`.
// NOT `<ranges>` (R12 keeps that out of the engine): just member-begin/end forwarding. Each is
// constrained on the `Range` concept so it never collides with the iterator-pair overload (a
// raw iterator is not a Range). Only the umbrella import carries these; a granular submodule
// import keeps the lean iterator-pair surface.
export namespace worse::core
{
    template <typename C>
    concept Range = requires(C& c) {
        c.begin();
        c.end();
    };

    template <typename C, typename Compare = Less<>>
        requires Range<C>
    constexpr void sort(C& c, Compare comp = Compare{})
    {
        sort(c.begin(), c.end(), comp);
    }

    template <typename C, typename Compare = Less<>>
        requires Range<C>
    constexpr void stableSort(C& c, Compare comp = Compare{})
    {
        stableSort(c.begin(), c.end(), comp);
    }

    template <typename C>
        requires Range<C>
    constexpr void reverse(C& c)
    {
        reverse(c.begin(), c.end());
    }

    template <typename C, typename T>
        requires Range<C>
    WE_NODISCARD constexpr auto find(C& c, T const& value)
    {
        return find(c.begin(), c.end(), value);
    }

    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr auto findIf(C& c, Pred pred)
    {
        return findIf(c.begin(), c.end(), pred);
    }

    template <typename C, typename T>
        requires Range<C>
    WE_NODISCARD constexpr auto count(C& c, T const& value)
    {
        return count(c.begin(), c.end(), value);
    }

    template <typename C, typename Func>
        requires Range<C>
    constexpr Func forEach(C& c, Func func)
    {
        return forEach(c.begin(), c.end(), func);
    }

    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr bool allOf(C& c, Pred pred)
    {
        return allOf(c.begin(), c.end(), pred);
    }

    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr bool anyOf(C& c, Pred pred)
    {
        return anyOf(c.begin(), c.end(), pred);
    }

    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr bool noneOf(C& c, Pred pred)
    {
        return noneOf(c.begin(), c.end(), pred);
    }
} // namespace worse::core
