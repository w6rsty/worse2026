module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.nonmodifying;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

/**
 * \file
 * \brief Non-mutating sequence queries over iterator-pair ranges: linear search, counting,
 *        universal/existential predicate tests, and range equality/mismatch.
 * \note All O(n), no allocation, no element modification. Flat `worse::core` namespace.
 */
export namespace worse::core
{
    // --- search ---------------------------------------------------------------

    /**
     * \brief First element in [first, last) equal to \p value.
     * \tparam It input iterator.
     * \return iterator to the match, or \p last if none.
     */
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

    /** \brief First element in [first, last) satisfying \p pred, else \p last. */
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

    /** \brief First element in [first, last) NOT satisfying \p pred, else \p last. */
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

    /** \brief Number of elements in [first, last) equal to \p value. */
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

    /** \brief Number of elements in [first, last) satisfying \p pred. */
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

    /**
     * \brief Apply \p func to each element of [first, last) in order.
     * \tparam It input iterator.
     * \return the (possibly stateful) functor.
     */
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

    /** \brief True if every element of [first, last) satisfies \p pred (vacuously true if empty). */
    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr bool allOf(It first, It last, Pred pred)
    {
        return findIfNot(first, last, pred) == last;
    }

    /** \brief True if any element of [first, last) satisfies \p pred. */
    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr bool anyOf(It first, It last, Pred pred)
    {
        return findIf(first, last, pred) != last;
    }

    /** \brief True if no element of [first, last) satisfies \p pred. */
    template <typename It, typename Pred>
        requires InputIterator<It>
    WE_NODISCARD constexpr bool noneOf(It first, It last, Pred pred)
    {
        return findIf(first, last, pred) == last;
    }

    // --- two-range comparison -------------------------------------------------

    /**
     * \brief First position where [first1, last1) and the second range stop matching under \p pred.
     * \param first1 iterator to the first element of the first range.
     * \param last1 iterator one past the last element of the first range.
     * \param first2 iterator to the first element of the second range.
     * \tparam It1,It2 input iterators.
     * \param pred binary equality predicate (default `EqualTo<>`).
     * \return the pair of iterators at the first mismatch (both at-end if all matched).
     * \note 3-iterator form: assumes the second range is at least as long as the first.
     */
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

    /** \brief 4-iterator mismatch: bounds both ranges (stops at the shorter end). */
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

    /**
     * \brief Whether [first1, last1) matches the second range element-wise under \p pred.
     * \param first1 iterator to the first element of the first range.
     * \param last1 iterator one past the last element of the first range.
     * \param first2 iterator to the first element of the second range.
     * \tparam It1,It2 input iterators.
     * \param pred binary equality predicate (default `EqualTo<>`).
     * \note 3-iterator form: assumes the second range is at least as long as the first.
     */
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

    /** \brief 4-iterator equal: bounds both ranges; equal only if both end together. */
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
