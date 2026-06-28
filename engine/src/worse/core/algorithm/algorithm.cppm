module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.algorithm;

/**
 * \file
 * \brief Umbrella that re-exports the algorithm modules.
 * \note Each `export import` makes the imported module's exported names visible to
 *       importers of this one; granular imports of the individual submodules remain
 *       available for callers who want a minimal import surface. Only this umbrella
 *       carries the container-range convenience overloads below.
 */
export import worse.core.algorithm.heap;
export import worse.core.algorithm.binary_search;
export import worse.core.algorithm.sort;
export import worse.core.algorithm.nonmodifying;
export import worse.core.algorithm.modifying;

import worse.core.utility; // Less<> default comparator for the range overloads

// --- container-range convenience overloads --------------------------------------------------
/**
 * \brief Container-range convenience overloads: thin wrappers taking a whole container
 *        (anything exposing begin()/end()) and forwarding to the iterator-pair algorithms,
 *        so `sort(v)` works alongside `sort(v.begin(), v.end())`.
 * \note NOT `<ranges>` (R12 keeps that out of the engine): just member-begin/end forwarding.
 *       Each is constrained on the `Range` concept so it never collides with the iterator-pair
 *       overload (a raw iterator is not a Range).
 */
export namespace worse::core
{
    template <typename C>
    concept Range = requires(C& c) {
        c.begin();
        c.end();
    };

    /**
     * \brief Range overload: introsort \p c in place by \p comp.
     * \tparam C container exposing begin()/end().
     */
    template <typename C, typename Compare = Less<>>
        requires Range<C>
    constexpr void sort(C& c, Compare comp = Compare{})
    {
        sort(c.begin(), c.end(), comp);
    }

    /** \brief Range overload: stable sort \p c in place by \p comp. */
    template <typename C, typename Compare = Less<>>
        requires Range<C>
    constexpr void stableSort(C& c, Compare comp = Compare{})
    {
        stableSort(c.begin(), c.end(), comp);
    }

    /** \brief Range overload: reverse \p c in place. */
    template <typename C>
        requires Range<C>
    constexpr void reverse(C& c)
    {
        reverse(c.begin(), c.end());
    }

    /** \brief Range overload: iterator to the first element of \p c equal to \p value, else end. */
    template <typename C, typename T>
        requires Range<C>
    WE_NODISCARD constexpr auto find(C& c, T const& value)
    {
        return find(c.begin(), c.end(), value);
    }

    /** \brief Range overload: iterator to the first element of \p c satisfying \p pred, else end. */
    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr auto findIf(C& c, Pred pred)
    {
        return findIf(c.begin(), c.end(), pred);
    }

    /** \brief Range overload: number of elements of \p c equal to \p value. */
    template <typename C, typename T>
        requires Range<C>
    WE_NODISCARD constexpr auto count(C& c, T const& value)
    {
        return count(c.begin(), c.end(), value);
    }

    /** \brief Range overload: apply \p func to each element of \p c in order; returns the functor. */
    template <typename C, typename Func>
        requires Range<C>
    constexpr Func forEach(C& c, Func func)
    {
        return forEach(c.begin(), c.end(), func);
    }

    /** \brief Range overload: true if every element of \p c satisfies \p pred (vacuously true if empty). */
    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr bool allOf(C& c, Pred pred)
    {
        return allOf(c.begin(), c.end(), pred);
    }

    /** \brief Range overload: true if any element of \p c satisfies \p pred. */
    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr bool anyOf(C& c, Pred pred)
    {
        return anyOf(c.begin(), c.end(), pred);
    }

    /** \brief Range overload: true if no element of \p c satisfies \p pred. */
    template <typename C, typename Pred>
        requires Range<C>
    WE_NODISCARD constexpr bool noneOf(C& c, Pred pred)
    {
        return noneOf(c.begin(), c.end(), pred);
    }
} // namespace worse::core
