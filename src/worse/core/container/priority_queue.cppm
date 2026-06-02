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

// Priority queue: a container ADAPTER that keeps an underlying random-access sequence
// (default `Array<T>`) arranged as a binary heap, so the highest-priority element is
// always at the front. The default comparator `Less<>` makes a MAX-heap -- `top()` is
// the largest element -- matching std::priority_queue (pass `Greater<>` for a min-heap).
//
// All heap maintenance routes through the algorithm layer (`makeHeap`/`pushHeap`/
// `popHeap`): push appends then sifts the newcomer up, pop swaps the root to the back,
// re-sifts, then drops the back. The adapter owns nothing the container does not -- it
// is just policy over a sequence -- so it inherits the container's allocator and
// relocation behaviour for free.
export namespace worse::core::container
{
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

        // Seed from an existing container (copy or move), then heapify in O(n).
        PriorityQueue(Compare const& comp, Container const& cont) : mContainer(cont), mComp(comp)
        {
            worse::core::makeHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        PriorityQueue(Compare const& comp, Container&& cont) : mContainer(worse::core::move(cont)), mComp(comp)
        {
            worse::core::makeHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        // Build from a range: append every element, then a single O(n) heapify (much
        // cheaper than n pushes of O(log n) each).
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

        // Pre-size the backing container so a known burst of push/emplace is realloc-free --
        // bounded push latency for the per-frame uses (open-set, timers, event queue) (R45).
        // Forwards to the underlying container; getAllocator exposes its allocator seam.
        void reserve(SizeType n) { mContainer.reserve(n); }
        WE_NODISCARD decltype(auto) getAllocator() noexcept { return mContainer.getAllocator(); }
        WE_NODISCARD decltype(auto) getAllocator() const noexcept { return mContainer.getAllocator(); }

        // --- element access ----------------------------------------------------

        // The highest-priority element (heap root). UB if empty (debug WE_ASSERT).
        WE_NODISCARD ConstReference top() const noexcept
        {
            WE_ASSERT(!empty());
            return mContainer.front();
        }

        // --- modifiers ---------------------------------------------------------

        template <typename... Args>
        void emplace(Args&&... args)
        {
            mContainer.emplaceBack(worse::core::forward<Args>(args)...);
            worse::core::pushHeap(mContainer.begin(), mContainer.end(), mComp);
        }

        void push(ConstReference value) { emplace(value); }
        void push(T&& value) { emplace(worse::core::move(value)); }

        // Remove the highest-priority element. popHeap rotates it to the back; popBack
        // then drops it and the remaining [begin, end) is a valid heap again.
        void pop() noexcept
        {
            WE_ASSERT(!empty());
            worse::core::popHeap(mContainer.begin(), mContainer.end(), mComp);
            mContainer.popBack();
        }

        void clear() noexcept { mContainer.clear(); }

        void swap(PriorityQueue& other) noexcept
        {
            mContainer.swap(other.mContainer);
            worse::core::swap(mComp, other.mComp);
        }

    private:
        Container mContainer{};
        WE_NO_UNIQUE_ADDRESS Compare mComp{};
    };

    template <typename T, typename Container, typename Compare>
    void swap(PriorityQueue<T, Container, Compare>& a, PriorityQueue<T, Container, Compare>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
