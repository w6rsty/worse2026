module;

#include "worse/core/macro.hpp"

#include <initializer_list>

export module worse.core.container.priority_queue;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.array;
import worse.core.algorithm.heap;

export namespace worse::core::container
{
    /**
     * \brief Container ADAPTER keeping an underlying random-access sequence (default `Array<T>`)
     *        arranged as a binary heap, so the highest-priority element is always at the front.
     *
     * The default comparator `Less<>` makes a MAX-heap -- `top()` is the largest element --
     * matching `std::priority_queue` (pass `Greater<>` for a min-heap).
     * \note All heap maintenance routes through the algorithm layer (`makeHeap`/`pushHeap`/
     *       `popHeap`): push appends then sifts the newcomer up; pop swaps the root to the back,
     *       re-sifts, then drops the back.
     * \note The adapter owns nothing the container does not -- it is just policy over a sequence --
     *       so it inherits the container's allocator and relocation behaviour for free.
     */
    template <typename T, typename Container = Array<T>, typename Compare = Less<>>
    class PriorityQueue
    {
    public:
        using ContainerType  = Container;
        using ValueCompare   = Compare;
        using ValueType      = typename Container::ValueType;
        using SizeType       = typename Container::SizeType;
        using Reference      = typename Container::Reference;
        using ConstReference = typename Container::ConstReference;

        static_assert(IsSame<T, ValueType>, "PriorityQueue<T>: Container::ValueType must be T");

        // --- construction ------------------------------------------------------

        PriorityQueue() = default;

        explicit PriorityQueue(Compare const& comp) : mComp(comp) {}

        /** \brief Seed from an existing container (copy), then heapify in O(n). */
        PriorityQueue(Compare const& comp, Container const& cont) : mContainer(cont), mComp(comp)
        {
            worse::core::makeHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        /** \brief Seed from an existing container (move), then heapify in O(n). */
        PriorityQueue(Compare const& comp, Container&& cont) : mContainer(worse::core::move(cont)), mComp(comp)
        {
            worse::core::makeHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        /**
         * \brief Build from the range [\p first, \p last): append every element, then heapify.
         * \tparam InIt input iterator type.
         * \param first iterator to the first element.
         * \param last iterator one past the last element.
         * \param comp comparator selecting heap order.
         * \note A single O(n) heapify, much cheaper than n pushes of O(log n) each.
         */
        template <typename InIt>
            requires InputIterator<InIt>
        PriorityQueue(InIt first, InIt last, Compare const& comp = Compare{}) : mComp(comp)
        {
            for (; first != last; ++first)
            {
                mContainer.emplaceBack(*first);
            }
            worse::core::makeHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        PriorityQueue(std::initializer_list<T> init, Compare const& comp = Compare{}) : mComp(comp)
        {
            for (T const& value : init)
            {
                mContainer.emplaceBack(value);
            }
            worse::core::makeHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mContainer.empty(); }
        WE_NODISCARD SizeType size() const noexcept { return mContainer.size(); }
        WE_NODISCARD SizeType capacity() const noexcept { return mContainer.capacity(); }

        /**
         * \brief Pre-size the backing container so a known burst of push/emplace is realloc-free.
         * \param n requested minimum capacity; forwards to the underlying container.
         * \note Bounded push latency for the per-frame uses (open-set, timers, event queue);
         *       `getAllocator` exposes the container's allocator seam. (R45)
         */
        void reserve(SizeType n) { mContainer.reserve(n); }
        WE_NODISCARD decltype(auto) getAllocator() noexcept { return mContainer.getAllocator(); }
        WE_NODISCARD decltype(auto) getAllocator() const noexcept { return mContainer.getAllocator(); }

        // --- element access ----------------------------------------------------

        /**
         * \brief The highest-priority element (heap root).
         * \return reference to the front element.
         * \pre `!empty()` (debug `WE_ASSERT`; release: UB).
         */
        WE_NODISCARD ConstReference top() const noexcept
        {
            WE_ASSERT(!empty());
            return mContainer.front();
        }

        // --- modifiers ---------------------------------------------------------

        /**
         * \brief Construct an element in place and sift it into heap order.
         * \tparam Args constructor argument types for `T`.
         * \param args forwarded to `T`'s constructor.
         * \note O(log n): appends then sifts the newcomer up.
         */
        template <typename... Args>
        void emplace(Args&&... args)
        {
            mContainer.emplaceBack(worse::core::forward<Args>(args)...);
            worse::core::pushHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        /** \brief Insert \p value (copy) and sift it into heap order; see emplace(). */
        void push(ConstReference value) { emplace(value); }
        /** \brief Insert \p value (move) and sift it into heap order; see emplace(). */
        void push(T&& value) { emplace(worse::core::move(value)); }

        /**
         * \brief Remove the highest-priority element.
         * \pre `!empty()` (debug `WE_ASSERT`).
         * \note `popHeap` rotates the root to the back; `popBack` drops it and the remaining
         *       [begin, end) is a valid heap again.
         * \note DETERMINISM: a binary heap is NOT stable -- equal-priority elements pop in an
         *       unspecified order that may differ across runs/platforms. For deterministic
         *       tie-breaking (replay, netcode), fold a sequence/insertion counter into Compare. (R45)
         */
        void pop() noexcept
        {
            WE_ASSERT(!empty());
            worse::core::popHeap(mContainer.begin(), mContainer.end(), mComp);
            mContainer.popBack();
        }

        /** \brief Remove all elements; size becomes 0. */
        void clear() noexcept { mContainer.clear(); }

        /** \brief Swap contents (and comparator) with \p other. */
        void swap(PriorityQueue& other) noexcept
        {
            mContainer.swap(other.mContainer);
            worse::core::swap(mComp, other.mComp);
        }

    private:
        Container mContainer{};
        WE_NO_UNIQUE_ADDRESS Compare mComp{};
    };

    /** \brief Free-function swap: exchanges the contents of \p a and \p b. */
    template <typename T, typename Container, typename Compare>
    void swap(PriorityQueue<T, Container, Compare>& a, PriorityQueue<T, Container, Compare>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
