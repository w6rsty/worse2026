module;

#include "worse/core/macro.hpp"

#include <cstddef> // std::byte
#include <initializer_list>
#include <memory> // std::construct_at, std::destroy_at (allocator-less, like FixedArray)

export module worse.core.container.fixed_list;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.list; // reuse ListNodeBase / ListNode / ListIterator

export namespace worse::core::container
{
    /**
     * \brief Fixed-capacity doubly-linked list over an inline, zero-heap node pool with a
     *        hard capacity cap (the EASTL fixed_list analogue).
     * \ingroup ctr_nodelist
     * \tparam T element type (must be non-const, non-volatile).
     * \tparam N inline capacity; the whole point is ZERO heap allocation.
     *
     * Up to N nodes live in an inline, properly-aligned byte buffer; a free-list (threaded
     * through the slots' own `mpNext`) hands them out. The game-perf node list for hot paths:
     * stable element addresses + O(1) front/back/middle insert/erase like `List`, but with NO
     * allocator traffic and no cache-line indirection to a heap node -- the nodes are packed in
     * the object's own storage.
     * \note Overflow past N is a hard-cap WE_ASSERT in every build (no heap spill; matches
     *       FixedArray, R6); use the `try*` variants for a non-aborting insert.
     * \note Trade-offs vs `List` (all inherent to inline storage, NOT std-mimicry): MOVE and
     *       SWAP are O(n) element moves, not an O(1) pointer steal (the nodes live IN the
     *       object, so moving cannot transfer node identity); there is NO `splice` (its
     *       "transfer node ownership with no element moves" contract cannot hold across two
     *       distinct inline pools -- use `merge` or insert/erase); `merge` is element-wise
     *       (moves values into this pool, O(n)) and capacity-checked. In-place algorithms that
     *       relink only THIS list's own nodes -- `sort`, `reverse`, `remove`/`removeIf`,
     *       `unique` -- are full O(1)-space and identical to `List`.
     * \note Iterator/reference stability matches `List` (insert never invalidates; erase only
     *       the erased element; sort/reverse invalidate nothing) -- BUT, unlike `List`, moving
     *       or swapping the container DOES invalidate iterators/pointers (the nodes physically
     *       relocate). NOT trivially relocatable: the circular sentinel + intra-buffer links
     *       break under memcpy.
     */
    template <typename T, usize N>
    class FixedList
    {
    public:
        using ValueType      = T;
        using SizeType       = usize;
        using DifferenceType = isize;
        using Reference      = T&;
        using ConstReference = T const&;
        using Pointer        = T*;
        using ConstPointer   = T const*;

        using Node     = ListNode<T>;
        using NodeBase = ListNodeBase;

        using Iterator         = ListIterator<T, T>;
        using ConstIterator    = ListIterator<T, T const>;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        static constexpr SizeType kCapacity = N;
        static constexpr usize kSortBins    = 64;

        static_assert(N > 0, "FixedList requires N > 0");
        static_assert(!IsConst<T>, "FixedList<T>: T must be non-const");
        static_assert(!IsVolatile<T>, "FixedList<T>: T must be non-volatile");

        // --- construction / destruction ---------------------------------------

        FixedList() noexcept { initPool(); }

        explicit FixedList(SizeType n)
        {
            initPool();
            WE_ASSERT(n <= N);
            for (SizeType i = 0; i < n; ++i)
            {
                emplaceBack();
            }
        }

        FixedList(SizeType n, ConstReference value)
        {
            initPool();
            WE_ASSERT(n <= N);
            for (SizeType i = 0; i < n; ++i)
            {
                emplaceBack(value);
            }
        }

        template <typename InIt>
            requires InputIterator<InIt>
        FixedList(InIt first, InIt last)
        {
            initPool();
            for (; first != last; ++first)
            {
                emplaceBack(*first);
            }
        }

        FixedList(std::initializer_list<T> init)
        {
            initPool();
            WE_ASSERT(init.size() <= N);
            for (auto const& v : init)
            {
                emplaceBack(v);
            }
        }

        FixedList(FixedList const& other)
        {
            initPool();
            for (ListNodeBase* cur = other.mAnchor.mpNext; cur != other.anchorPtr(); cur = cur->mpNext)
            {
                emplaceBack(static_cast<Node const*>(cur)->mValue);
            }
        }

        // Inline storage cannot be stolen: a move is an element-wise move into this buffer,
        // O(n), and empties `other`.
        FixedList(FixedList&& other) noexcept
        {
            initPool();
            for (ListNodeBase* cur = other.mAnchor.mpNext; cur != other.anchorPtr(); cur = cur->mpNext)
            {
                emplaceBack(worse::core::move(static_cast<Node*>(cur)->mValue));
            }
            other.clear();
        }

        ~FixedList() { destroyAll(); }

        FixedList& operator=(FixedList const& other)
        {
            if (this != &other)
            {
                clear();
                for (ListNodeBase* cur = other.mAnchor.mpNext; cur != other.anchorPtr(); cur = cur->mpNext)
                {
                    emplaceBack(static_cast<Node const*>(cur)->mValue);
                }
            }
            return *this;
        }

        FixedList& operator=(FixedList&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                for (ListNodeBase* cur = other.mAnchor.mpNext; cur != other.anchorPtr(); cur = cur->mpNext)
                {
                    emplaceBack(worse::core::move(static_cast<Node*>(cur)->mValue));
                }
                other.clear();
            }
            return *this;
        }

        FixedList& operator=(std::initializer_list<T> init)
        {
            assign(init.begin(), init.end());
            return *this;
        }

        /**
         * \brief Replace the contents with \p n copies of \p value.
         * \pre `n <= N`.
         */
        void assign(SizeType n, ConstReference value)
        {
            clear();
            WE_ASSERT(n <= N);
            for (SizeType i = 0; i < n; ++i)
            {
                emplaceBack(value);
            }
        }

        /**
         * \brief Replace the contents with the range [\p first, \p last).
         * \tparam InIt input iterator type.
         */
        template <typename InIt>
            requires InputIterator<InIt>
        void assign(InIt first, InIt last)
        {
            clear();
            for (; first != last; ++first)
            {
                emplaceBack(*first);
            }
        }

        /** \brief Replace the contents with the elements of \p init. */
        void assign(std::initializer_list<T> init) { assign(init.begin(), init.end()); }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return Iterator(mAnchor.mpNext); }
        WE_NODISCARD ConstIterator begin() const noexcept { return ConstIterator(mAnchor.mpNext); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return ConstIterator(mAnchor.mpNext); }
        WE_NODISCARD Iterator end() noexcept { return Iterator(anchorPtr()); }
        WE_NODISCARD ConstIterator end() const noexcept { return ConstIterator(anchorPtr()); }
        WE_NODISCARD ConstIterator cend() const noexcept { return ConstIterator(anchorPtr()); }

        WE_NODISCARD ReverseIter rbegin() noexcept { return ReverseIter(end()); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(cend()); }
        WE_NODISCARD ConstReverseIter crbegin() const noexcept { return ConstReverseIter(cend()); }
        WE_NODISCARD ReverseIter rend() noexcept { return ReverseIter(begin()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(cbegin()); }
        WE_NODISCARD ConstReverseIter crend() const noexcept { return ConstReverseIter(cbegin()); }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mSize == 0; }
        WE_NODISCARD bool full() const noexcept { return mSize == N; }
        WE_NODISCARD SizeType size() const noexcept { return mSize; }
        WE_NODISCARD constexpr SizeType capacity() const noexcept { return N; }
        WE_NODISCARD SizeType remaining() const noexcept { return N - mSize; } // free slots (R46 pre-check)

        // --- element access ----------------------------------------------------

        WE_NODISCARD Reference front() noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node*>(mAnchor.mpNext)->mValue;
        }
        WE_NODISCARD ConstReference front() const noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node const*>(mAnchor.mpNext)->mValue;
        }
        WE_NODISCARD Reference back() noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node*>(mAnchor.mpPrev)->mValue;
        }
        WE_NODISCARD ConstReference back() const noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node const*>(mAnchor.mpPrev)->mValue;
        }

        // --- modifiers: ends ---------------------------------------------------

        /**
         * \brief Construct an element in place at the front in O(1).
         * \tparam Args constructor argument types for `T`.
         * \param args arguments forwarded to `T`'s constructor.
         * \return reference to the newly constructed front element.
         * \note Aborts on overflow (R6); use tryEmplaceFront for a non-aborting insert.
         */
        template <typename... Args>
        Reference emplaceFront(Args&&... args)
        {
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(mAnchor.mpNext, node);
            ++mSize;
            return node->mValue;
        }
        /**
         * \brief Construct an element in place at the back in O(1).
         * \tparam Args constructor argument types for `T`.
         * \param args arguments forwarded to `T`'s constructor.
         * \return reference to the newly constructed back element.
         * \note Aborts on overflow (R6); use tryEmplaceBack for a non-aborting insert.
         */
        template <typename... Args>
        Reference emplaceBack(Args&&... args)
        {
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(anchorPtr(), node);
            ++mSize;
            return node->mValue;
        }

        /** \brief Prepend a copy of \p value in O(1); aborts on overflow (R6). */
        void pushFront(ConstReference value) { emplaceFront(value); }
        /** \brief Prepend \p value by move in O(1); aborts on overflow (R6). */
        void pushFront(T&& value) { emplaceFront(worse::core::move(value)); }
        /** \brief Append a copy of \p value in O(1); aborts on overflow (R6). */
        void pushBack(ConstReference value) { emplaceBack(value); }
        /** \brief Append \p value by move in O(1); aborts on overflow (R6). */
        void pushBack(T&& value) { emplaceBack(worse::core::move(value)); }

        /**
         * \brief Try to construct an element at the front without aborting on overflow.
         * \tparam Args constructor argument types for `T`.
         * \param args arguments forwarded to `T`'s constructor.
         * \return pointer to the new front element, or nullptr when the inline pool is full.
         * \note Non-aborting overflow path (R46), for callers that handle overflow rather than
         *       treat it as a precondition. emplaceFront/emplaceBack abort (WE_VERIFY in allocSlot).
         */
        template <typename... Args>
        WE_NODISCARD Pointer tryEmplaceFront(Args&&... args)
        {
            if (mSize == N)
            {
                return nullptr;
            }
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(mAnchor.mpNext, node);
            ++mSize;
            return &node->mValue;
        }
        /**
         * \brief Try to construct an element at the back without aborting on overflow.
         * \tparam Args constructor argument types for `T`.
         * \param args arguments forwarded to `T`'s constructor.
         * \return pointer to the new back element, or nullptr when the inline pool is full.
         * \note Non-aborting overflow path (R46); emplaceBack itself aborts on overflow.
         */
        template <typename... Args>
        WE_NODISCARD Pointer tryEmplaceBack(Args&&... args)
        {
            if (mSize == N)
            {
                return nullptr;
            }
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(anchorPtr(), node);
            ++mSize;
            return &node->mValue;
        }

        /**
         * \brief Remove the front element in O(1).
         * \pre The list is non-empty.
         */
        void popFront() noexcept
        {
            WE_ASSERT(!empty());
            ListNodeBase* const n = mAnchor.mpNext;
            unlink(n);
            destroyNode(n);
            --mSize;
        }
        /**
         * \brief Remove the back element in O(1).
         * \pre The list is non-empty.
         */
        void popBack() noexcept
        {
            WE_ASSERT(!empty());
            ListNodeBase* const n = mAnchor.mpPrev;
            unlink(n);
            destroyNode(n);
            --mSize;
        }

        // --- modifiers: arbitrary position -------------------------------------

        /**
         * \brief Construct an element in place before \p pos in O(1).
         * \tparam Args constructor argument types for `T`.
         * \param pos iterator before which the new element is linked.
         * \param args arguments forwarded to `T`'s constructor.
         * \return iterator to the newly constructed element.
         * \note Aborts on overflow (R6).
         */
        template <typename... Args>
        Iterator emplace(ConstIterator pos, Args&&... args)
        {
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(pos.node(), node);
            ++mSize;
            return Iterator(node);
        }

        /**
         * \brief Insert a copy of \p value before \p pos in O(1).
         * \return iterator to the inserted element.
         */
        Iterator insert(ConstIterator pos, ConstReference value) { return emplace(pos, value); }
        /**
         * \brief Insert \p value by move before \p pos in O(1).
         * \return iterator to the inserted element.
         */
        Iterator insert(ConstIterator pos, T&& value) { return emplace(pos, worse::core::move(value)); }

        /**
         * \brief Insert \p n copies of \p value before \p pos.
         * \return iterator to the first inserted element (or \p pos when \p n == 0).
         */
        Iterator insert(ConstIterator pos, SizeType n, ConstReference value)
        {
            ListNodeBase* const posn = pos.node();
            ListNodeBase* firstNew   = posn;
            for (SizeType i = 0; i < n; ++i)
            {
                Node* node = createNode(value);
                linkBefore(posn, node);
                ++mSize;
                if (i == 0)
                {
                    firstNew = node;
                }
            }
            return Iterator(firstNew);
        }

        /**
         * \brief Insert the range [\p first, \p last) before \p pos.
         * \tparam InIt input iterator type.
         * \return iterator to the first inserted element (or \p pos for an empty range).
         */
        template <typename InIt>
            requires InputIterator<InIt>
        Iterator insert(ConstIterator pos, InIt first, InIt last)
        {
            ListNodeBase* const posn = pos.node();
            ListNodeBase* firstNew   = posn;
            bool seen                = false;
            for (; first != last; ++first)
            {
                Node* node = createNode(*first);
                linkBefore(posn, node);
                ++mSize;
                if (!seen)
                {
                    firstNew = node;
                    seen     = true;
                }
            }
            return Iterator(firstNew);
        }

        /**
         * \brief Insert the elements of \p init before \p pos.
         * \return iterator to the first inserted element (or \p pos when \p init is empty).
         */
        Iterator insert(ConstIterator pos, std::initializer_list<T> init)
        {
            return insert(pos, init.begin(), init.end());
        }

        /**
         * \brief Erase the element at \p pos in O(1).
         * \return iterator to the element following the erased one.
         */
        Iterator erase(ConstIterator pos) noexcept
        {
            ListNodeBase* const n   = pos.node();
            ListNodeBase* const nxt = n->mpNext;
            unlink(n);
            destroyNode(n);
            --mSize;
            return Iterator(nxt);
        }

        /**
         * \brief Erase the range [\p first, \p last).
         * \return iterator to \p last.
         */
        Iterator erase(ConstIterator first, ConstIterator last) noexcept
        {
            ListNodeBase* f       = first.node();
            ListNodeBase* const l = last.node();
            while (f != l)
            {
                ListNodeBase* const nxt = f->mpNext;
                unlink(f);
                destroyNode(f);
                --mSize;
                f = nxt;
            }
            return Iterator(l);
        }

        /** \brief Erase all elements, returning the list to empty. */
        void clear() noexcept { destroyAll(); }

        void resize(SizeType n)
        {
            WE_ASSERT(n <= N);
            while (mSize > n)
            {
                popBack();
            }
            while (mSize < n)
            {
                emplaceBack();
            }
        }

        void resize(SizeType n, ConstReference value)
        {
            WE_ASSERT(n <= N);
            while (mSize > n)
            {
                popBack();
            }
            while (mSize < n)
            {
                emplaceBack(value);
            }
        }

        // O(n) element-wise swap (inline storage cannot be pointer-swapped).
        void swap(FixedList& other) noexcept
        {
            FixedList tmp(worse::core::move(*this));
            *this = worse::core::move(other);
            other = worse::core::move(tmp);
        }

        // --- in-place algorithms (relink this list's own nodes) ----------------

        /**
         * \brief Remove every element equal to \p value.
         * \return count of elements removed.
         */
        SizeType remove(ConstReference value)
        {
            SizeType removed  = 0;
            ListNodeBase* cur = mAnchor.mpNext;
            while (cur != anchorPtr())
            {
                ListNodeBase* const nxt = cur->mpNext;
                if (static_cast<Node*>(cur)->mValue == value)
                {
                    unlink(cur);
                    destroyNode(cur);
                    --mSize;
                    ++removed;
                }
                cur = nxt;
            }
            return removed;
        }

        /**
         * \brief Remove every element satisfying \p pred.
         * \tparam Pred unary predicate over `T`.
         * \param pred predicate; an element is removed when it returns true.
         * \return count of elements removed.
         */
        template <typename Pred>
            requires PredicateFor<Pred, T>
        SizeType removeIf(Pred pred)
        {
            SizeType removed  = 0;
            ListNodeBase* cur = mAnchor.mpNext;
            while (cur != anchorPtr())
            {
                ListNodeBase* const nxt = cur->mpNext;
                if (pred(static_cast<Node*>(cur)->mValue))
                {
                    unlink(cur);
                    destroyNode(cur);
                    --mSize;
                    ++removed;
                }
                cur = nxt;
            }
            return removed;
        }

        /**
         * \brief Collapse each run of consecutive equal elements to one (using `==`).
         * \return count of elements removed.
         */
        SizeType unique() { return unique(EqualTo<>{}); }

        /**
         * \brief Collapse each run of consecutive elements deemed equal by \p pred to one.
         * \tparam BinPred binary predicate over adjacent elements.
         * \param pred returns true when two adjacent elements are duplicates.
         * \return count of elements removed.
         */
        template <typename BinPred>
        SizeType unique(BinPred pred)
        {
            if (mSize < 2)
            {
                return 0;
            }
            SizeType removed  = 0;
            ListNodeBase* cur = mAnchor.mpNext;
            while (cur->mpNext != anchorPtr())
            {
                ListNodeBase* const nxt = cur->mpNext;
                if (pred(static_cast<Node*>(cur)->mValue, static_cast<Node*>(nxt)->mValue))
                {
                    unlink(nxt);
                    destroyNode(nxt);
                    --mSize;
                    ++removed;
                }
                else
                {
                    cur = nxt;
                }
            }
            return removed;
        }

        /**
         * \brief Stably merge sorted \p other into this sorted list using `<`.
         * \param other source list, emptied by the merge.
         * \pre Both lists are already sorted; `mSize + other.size() <= N`.
         * \note Element-wise: moves `other`'s values into THIS pool (nodes cannot cross pools),
         *       O(n), capacity-checked (R46). Equal elements keep this-before-other.
         */
        void merge(FixedList& other) { merge(other, Less<>{}); }

        /**
         * \brief Stably merge sorted \p other into this sorted list using \p comp.
         * \tparam Compare strict-weak-ordering comparator over `T`.
         * \param other source list, emptied by the merge.
         * \param comp comparator; both lists must already be sorted by it.
         * \pre `mSize + other.size() <= N` (hard cap, R46).
         * \note Element-wise move into this pool, O(n); equal elements keep this-before-other.
         */
        template <typename Compare>
            requires CompareFor<Compare, T>
        void merge(FixedList& other, Compare comp)
        {
            if (this == &other || other.empty())
            {
                return;
            }
            WE_VERIFY(mSize + other.mSize <= N); // hard cap (R46): room for the merged-in values
            ListNodeBase* a = mAnchor.mpNext;
            while (a != anchorPtr() && !other.empty())
            {
                ListNodeBase* const b = other.mAnchor.mpNext;
                if (comp(static_cast<Node*>(b)->mValue, static_cast<Node*>(a)->mValue))
                {
                    Node* node = createNode(worse::core::move(static_cast<Node*>(b)->mValue));
                    linkBefore(a, node);
                    ++mSize;
                    other.popFront();
                }
                else
                {
                    a = a->mpNext;
                }
            }
            while (!other.empty())
            {
                ListNodeBase* const b = other.mAnchor.mpNext;
                Node* node            = createNode(worse::core::move(static_cast<Node*>(b)->mValue));
                linkBefore(anchorPtr(), node);
                ++mSize;
                other.popFront();
            }
        }

        /** \brief Stably sort the list in ascending order using `<`. */
        void sort() { sort(Less<>{}); }

        /**
         * \brief Stably sort the list using \p comp.
         * \tparam Compare strict-weak-ordering comparator over `T`.
         * \param comp comparator defining the order.
         * \note Allocation-free, stable, in-place binned merge sort (same as List::sort);
         *       relinks this list's own nodes among stack-local bins; `mSize` invariant.
         */
        template <typename Compare>
            requires CompareFor<Compare, T>
        void sort(Compare comp)
        {
            if (mSize < 2)
            {
                return;
            }
            ListNodeBase counter[kSortBins];
            for (usize i = 0; i < kSortBins; ++i)
            {
                counter[i].mpNext = counter[i].mpPrev = &counter[i];
            }
            ListNodeBase carry;
            carry.mpNext = carry.mpPrev = &carry;

            int fill = 0;
            while (mAnchor.mpNext != anchorPtr())
            {
                ListNodeBase* const first = mAnchor.mpNext;
                transfer(&carry, first, first->mpNext);
                int i = 0;
                while (i < fill && counter[i].mpNext != &counter[i])
                {
                    mergeRings(counter[i], carry, comp);
                    swapRings(carry, counter[i]);
                    ++i;
                }
                swapRings(carry, counter[i]);
                if (i == fill)
                {
                    ++fill;
                }
            }
            for (int i = 1; i < fill; ++i)
            {
                mergeRings(counter[i], counter[i - 1], comp);
            }
            ListNodeBase& sorted = counter[fill - 1];
            transfer(anchorPtr(), sorted.mpNext, &sorted);
        }

        /**
         * \brief Reverse element order in place in O(n).
         * \note Swaps every node's next/prev pointers (including the anchor's); O(1) extra
         *       space, no element moves, `mSize` unchanged.
         */
        void reverse() noexcept
        {
            ListNodeBase* cur = anchorPtr();
            do
            {
                worse::core::swap(cur->mpNext, cur->mpPrev);
                cur = cur->mpPrev;
            } while (cur != anchorPtr());
        }

    private:
        alignas(Node) std::byte mStorage[sizeof(Node) * N];
        ListNodeBase mAnchor;
        ListNodeBase* mpFree = nullptr;
        SizeType mSize       = 0;

        WE_NODISCARD ListNodeBase* anchorPtr() const noexcept { return const_cast<ListNodeBase*>(&mAnchor); }

        WE_NODISCARD Node* slotAt(usize i) noexcept
        {
            return reinterpret_cast<Node*>(mStorage + i * sizeof(Node));
        }

        // Thread every inline slot onto the free-list and reset to the empty ring. Called by
        // every constructor and after a full clear.
        void initPool() noexcept
        {
            mpFree = nullptr;
            for (usize i = N; i-- > 0;)
            {
                ListNodeBase* slot = slotAt(i);
                slot->mpNext       = mpFree;
                mpFree             = slot;
            }
            mAnchor.mpNext = &mAnchor;
            mAnchor.mpPrev = &mAnchor;
            mSize          = 0;
        }

        WE_NODISCARD ListNodeBase* allocSlot() noexcept
        {
            WE_VERIFY(mpFree != nullptr); // hard cap (R46, always-on) -- inline pool exhausted, no heap spill
            ListNodeBase* const slot = mpFree;
            mpFree                   = mpFree->mpNext;
            return slot;
        }

        void freeSlot(ListNodeBase* n) noexcept
        {
            n->mpNext = mpFree;
            mpFree    = n;
        }

        template <typename... Args>
        WE_NODISCARD Node* createNode(Args&&... args)
        {
            Node* node = static_cast<Node*>(allocSlot());
            std::construct_at(&node->mValue, worse::core::forward<Args>(args)...);
            return node;
        }

        void destroyNode(ListNodeBase* n) noexcept
        {
            std::destroy_at(&static_cast<Node*>(n)->mValue);
            freeSlot(n);
        }

        void destroyAll() noexcept
        {
            ListNodeBase* cur = mAnchor.mpNext;
            while (cur != anchorPtr())
            {
                ListNodeBase* const nxt = cur->mpNext;
                std::destroy_at(&static_cast<Node*>(cur)->mValue);
                cur = nxt;
            }
            initPool();
        }

        static void linkBefore(ListNodeBase* pos, ListNodeBase* n) noexcept
        {
            n->mpPrev           = pos->mpPrev;
            n->mpNext           = pos;
            pos->mpPrev->mpNext = n;
            pos->mpPrev         = n;
        }

        static void unlink(ListNodeBase* n) noexcept
        {
            n->mpPrev->mpNext = n->mpNext;
            n->mpNext->mpPrev = n->mpPrev;
        }

        static void transfer(ListNodeBase* pos, ListNodeBase* first, ListNodeBase* last) noexcept
        {
            if (pos == first || first == last)
            {
                return;
            }
            ListNodeBase* const tail    = last->mpPrev;
            ListNodeBase* const srcPrev = first->mpPrev;
            srcPrev->mpNext             = last;
            last->mpPrev                = srcPrev;
            ListNodeBase* const posPrev = pos->mpPrev;
            posPrev->mpNext             = first;
            first->mpPrev               = posPrev;
            tail->mpNext                = pos;
            pos->mpPrev                 = tail;
        }

        static void reseat(ListNodeBase& anchor, ListNodeBase* otherAnchor) noexcept
        {
            if (anchor.mpNext == otherAnchor)
            {
                anchor.mpNext = anchor.mpPrev = &anchor;
            }
            else
            {
                anchor.mpNext->mpPrev = &anchor;
                anchor.mpPrev->mpNext = &anchor;
            }
        }

        template <typename Compare>
        static void mergeRings(ListNodeBase& dst, ListNodeBase& src, Compare& comp)
        {
            ListNodeBase* a          = dst.mpNext;
            ListNodeBase* const aEnd = &dst;
            ListNodeBase* b          = src.mpNext;
            ListNodeBase* const bEnd = &src;
            while (a != aEnd && b != bEnd)
            {
                if (comp(static_cast<Node*>(b)->mValue, static_cast<Node*>(a)->mValue))
                {
                    ListNodeBase* const bNext = b->mpNext;
                    transfer(a, b, bNext);
                    b = bNext;
                }
                else
                {
                    a = a->mpNext;
                }
            }
            if (b != bEnd)
            {
                transfer(aEnd, b, bEnd);
            }
        }

        static void swapRings(ListNodeBase& a, ListNodeBase& b) noexcept
        {
            worse::core::swap(a.mpNext, b.mpNext);
            worse::core::swap(a.mpPrev, b.mpPrev);
            reseat(a, &b);
            reseat(b, &a);
        }
    };

    template <typename T, usize N>
    void swap(FixedList<T, N>& a, FixedList<T, N>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
