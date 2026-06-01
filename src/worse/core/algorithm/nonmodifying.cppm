module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.nonmodifying;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

// Non-mutating sequence queries over iterator-pair ranges: linear search, counting,
// universal/existential predicate tests, and range equality/mismatch. All O(n), no
// allocation, no element modification. Flat `worse::core` namespace.
export namespace worse::core
{
    // --- search ---------------------------------------------------------------

    template <typename It, typename T>
        requires InputIterator<It>
    WE_NODISCARD constexpr It find(It first, It last, T const& value)
    {
        for (; first != last; ++first)
        {
            if (*first == value)
            {
                return first;
            }
        }
        return last;
    }

    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr It findIf(It first, It last, Pred pred)
    {
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                return first;
            }
        }
        return last;
    }

    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr It findIfNot(It first, It last, Pred pred)
    {
        for (; first != last; ++first)
        {
            if (!pred(*first))
            {
                return first;
            }
        }
        return last;
    }

    // --- count ----------------------------------------------------------------

    template <typename It, typename T>
        requires InputIterator<It>
    WE_NODISCARD constexpr typename IteratorTraits<It>::DifferenceType count(It first, It last, T const& value)
    {
        typename IteratorTraits<It>::DifferenceType n = 0;
        for (; first != last; ++first)
        {
            if (*first == value)
            {
                ++n;
            }
        }
        return n;
    }

    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr typename IteratorTraits<It>::DifferenceType countIf(It first, It last, Pred pred)
    {
        typename IteratorTraits<It>::DifferenceType n = 0;
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                ++n;
            }
        }
        return n;
    }

    // --- traversal ------------------------------------------------------------

    // Apply `func` to each element in order; returns the (possibly stateful) functor.
    template <typename It, typename Func>
        requires InputIterator<It>
    constexpr Func forEach(It first, It last, Func func)
    {
        for (; first != last; ++first)
        {
            func(*first);
        }
        return func;
    }

    // --- predicate folds ------------------------------------------------------

    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr bool allOf(It first, It last, Pred pred)
    {
        return findIfNot(first, last, pred) == last;
    }

    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr bool anyOf(It first, It last, Pred pred)
    {
        return findIf(first, last, pred) != last;
    }

    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr bool noneOf(It first, It last, Pred pred)
    {
        return findIf(first, last, pred) == last;
    }

    // --- two-range comparison -------------------------------------------------
    //
    // 3-iterator form assumes the second range is at least as long as the first;
    // the 4-iterator form bounds both (unequal lengths => not equal / mismatch at end).

    template <typename It1, typename It2, typename Pred = EqualTo<>>
        requires InputIterator<It1> && InputIterator<It2>
    WE_NODISCARD constexpr Pair<It1, It2> mismatch(It1 first1, It1 last1, It2 first2, Pred pred = Pred{})
    {
        while (first1 != last1 && pred(*first1, *first2))
        {
            ++first1;
            ++first2;
        }
        return makePair(first1, first2);
    }

    template <typename It1, typename It2, typename Pred = EqualTo<>>
        requires InputIterator<It1> && InputIterator<It2>
    WE_NODISCARD constexpr Pair<It1, It2>
    mismatch(It1 first1, It1 last1, It2 first2, It2 last2, Pred pred = Pred{})
    {
        while (first1 != last1 && first2 != last2 && pred(*first1, *first2))
        {
            ++first1;
            ++first2;
        }
        return makePair(first1, first2);
    }

    template <typename It1, typename It2, typename Pred = EqualTo<>>
        requires InputIterator<It1> && InputIterator<It2>
    WE_NODISCARD constexpr bool equal(It1 first1, It1 last1, It2 first2, Pred pred = Pred{})
    {
        for (; first1 != last1; ++first1, ++first2)
        {
            if (!pred(*first1, *first2))
            {
                return false;
            }
        }
        return true;
    }

    template <typename It1, typename It2, typename Pred = EqualTo<>>
        requires InputIterator<It1> && InputIterator<It2>
    WE_NODISCARD constexpr bool equal(It1 first1, It1 last1, It2 first2, It2 last2, Pred pred = Pred{})
    {
        for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        {
            if (!pred(*first1, *first2))
            {
                return false;
            }
        }
        return first1 == last1 && first2 == last2; // equal only if both ended together
    }
} // namespace worse::core
