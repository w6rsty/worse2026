module;

#include "worse/core/macro.hpp"

#include <cstddef> // std::byte
#include <initializer_list>
#include <memory> // std::construct_at, std::destroy_at (allocator-less, like FixedArray)

export module worse.core.container.fixed_slist;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.forward_list; // reuse ForwardListNodeBase / ForwardListNode / ForwardListIterator

// Fixed-capacity SINGLY-linked list with an INLINE, zero-heap node pool (the EASTL
// fixed_slist analogue). Up to N single-pointer nodes live in an inline byte buffer handed
// out by a free-list; ZERO heap allocation, overflow is a hard-cap WE_ASSERT (DECISIONS R6).
// The smallest, most cache-friendly node list the engine offers: one pointer per node, all
// packed in the object's own storage. Same `*After` API as `ForwardList` (the correct
// singly-linked shape), O(1) cached `size()`.
//
// Trade-offs vs `ForwardList` (inherent to inline storage, not std-mimicry): MOVE/SWAP are
// O(n) element moves (nodes live in the object, can't transfer identity); NO `spliceAfter`
// (cross-pool node identity can't hold -- use `merge`/insertAfter); `merge` is element-wise
// (O(n), capacity-checked). In-place `sort`/`reverse`/`remove`/`removeIf`/`unique` relink this
// list's own nodes and are full-power. NOT trivially relocatable.
export namespace worse::core::container
{
    template <typename T, usize N>
    class FixedSList
    {
    public:
        using ValueType      = T;
        using SizeType       = usize;
        using DifferenceType = isize;
        using Reference      = T&;
        using ConstReference = T const&;
        using Pointer        = T*;
        using ConstPointer   = T const*;

        using Node     = ForwardListNode<T>;
        using NodeBase = ForwardListNodeBase;

        using Iterator      = ForwardListIterator<T, T>;
        using ConstIterator = ForwardListIterator<T, T const>;

        static constexpr SizeType kCapacity = N;
        static constexpr usize kSortBins    = 64;

        static_assert(N > 0, "FixedSList requires N > 0");
        static_assert(!IsConst<T>, "FixedSList<T>: T must be non-const");
        static_assert(!IsVolatile<T>, "FixedSList<T>: T must be non-volatile");

        // --- construction / destruction ---------------------------------------

        FixedSList() noexcept { initPool(); }

        explicit FixedSList(SizeType n)
        {
            initPool();
            WE_ASSERT(n <= N);
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (SizeType i = 0; i < n; ++i)
            {
                tail = appendNode(tail);
            }
        }

        FixedSList(SizeType n, ConstReference value)
        {
            initPool();
            WE_ASSERT(n <= N);
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (SizeType i = 0; i < n; ++i)
            {
                tail = appendNode(tail, value);
            }
        }

        template <typename InIt>
            requires InputIterator<InIt>
        FixedSList(InIt first, InIt last)
        {
            initPool();
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (; first != last; ++first)
            {
                tail = appendNode(tail, *first);
            }
        }

        FixedSList(std::initializer_list<T> init)
        {
            initPool();
            WE_ASSERT(init.size() <= N);
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (auto const& v : init)
            {
                tail = appendNode(tail, v);
            }
        }

        FixedSList(FixedSList const& other)
        {
            initPool();
            copyFrom(other);
        }

        FixedSList(FixedSList&& other) noexcept
        {
            initPool();
            moveFrom(other);
        }

        ~FixedSList() { destroyAll(); }

        FixedSList& operator=(FixedSList const& other)
        {
            if (this != &other)
            {
                clear();
                copyFrom(other);
            }
            return *this;
        }

        FixedSList& operator=(FixedSList&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                moveFrom(other);
            }
            return *this;
        }

        FixedSList& operator=(std::initializer_list<T> init)
        {
            assign(init.begin(), init.end());
            return *this;
        }

        void assign(SizeType n, ConstReference value)
        {
            clear();
            WE_ASSERT(n <= N);
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (SizeType i = 0; i < n; ++i)
            {
                tail = appendNode(tail, value);
            }
        }

        template <typename InIt>
            requires InputIterator<InIt>
        void assign(InIt first, InIt last)
        {
            clear();
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (; first != last; ++first)
            {
                tail = appendNode(tail, *first);
            }
        }

        void assign(std::initializer_list<T> init) { assign(init.begin(), init.end()); }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator beforeBegin() noexcept { return Iterator(beforeBeginPtr()); }
        WE_NODISCARD ConstIterator beforeBegin() const noexcept { return ConstIterator(beforeBeginPtr()); }
        WE_NODISCARD ConstIterator cbeforeBegin() const noexcept { return ConstIterator(beforeBeginPtr()); }

        WE_NODISCARD Iterator begin() noexcept { return Iterator(mBeforeBegin.mpNext); }
        WE_NODISCARD ConstIterator begin() const noexcept { return ConstIterator(mBeforeBegin.mpNext); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return ConstIterator(mBeforeBegin.mpNext); }
        WE_NODISCARD Iterator end() noexcept { return Iterator(nullptr); }
        WE_NODISCARD ConstIterator end() const noexcept { return ConstIterator(nullptr); }
        WE_NODISCARD ConstIterator cend() const noexcept { return ConstIterator(nullptr); }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mBeforeBegin.mpNext == nullptr; }
        WE_NODISCARD bool full() const noexcept { return mSize == N; }
        WE_NODISCARD SizeType size() const noexcept { return mSize; }
        WE_NODISCARD constexpr SizeType capacity() const noexcept { return N; }

        // --- element access ----------------------------------------------------

        WE_NODISCARD Reference front() noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node*>(mBeforeBegin.mpNext)->mValue;
        }
        WE_NODISCARD ConstReference front() const noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node const*>(mBeforeBegin.mpNext)->mValue;
        }

        // --- modifiers: front --------------------------------------------------

        template <typename... Args>
        Reference emplaceFront(Args&&... args)
        {
            Node* node          = createNode(worse::core::forward<Args>(args)...);
            node->mpNext        = mBeforeBegin.mpNext;
            mBeforeBegin.mpNext = node;
            ++mSize;
            return node->mValue;
        }

        void pushFront(ConstReference value) { emplaceFront(value); }
        void pushFront(T&& value) { emplaceFront(worse::core::move(value)); }

        void popFront() noexcept
        {
            WE_ASSERT(!empty());
            ForwardListNodeBase* const victim = mBeforeBegin.mpNext;
            mBeforeBegin.mpNext               = victim->mpNext;
            destroyNode(victim);
            --mSize;
        }

        // --- modifiers: after a position ---------------------------------------

        template <typename... Args>
        Iterator emplaceAfter(ConstIterator pos, Args&&... args)
        {
            ForwardListNodeBase* const p = pos.node();
            Node* node                   = createNode(worse::core::forward<Args>(args)...);
            node->mpNext                 = p->mpNext;
            p->mpNext                    = node;
            ++mSize;
            return Iterator(node);
        }

        Iterator insertAfter(ConstIterator pos, ConstReference value) { return emplaceAfter(pos, value); }
        Iterator insertAfter(ConstIterator pos, T&& value) { return emplaceAfter(pos, worse::core::move(value)); }

        Iterator insertAfter(ConstIterator pos, SizeType n, ConstReference value)
        {
            ForwardListNodeBase* cur = pos.node();
            for (SizeType i = 0; i < n; ++i)
            {
                Node* node   = createNode(value);
                node->mpNext = cur->mpNext;
                cur->mpNext  = node;
                cur          = node;
                ++mSize;
            }
            return Iterator(cur);
        }

        template <typename InIt>
            requires InputIterator<InIt>
        Iterator insertAfter(ConstIterator pos, InIt first, InIt last)
        {
            ForwardListNodeBase* cur = pos.node();
            for (; first != last; ++first)
            {
                Node* node   = createNode(*first);
                node->mpNext = cur->mpNext;
                cur->mpNext  = node;
                cur          = node;
                ++mSize;
            }
            return Iterator(cur);
        }

        Iterator insertAfter(ConstIterator pos, std::initializer_list<T> init)
        {
            return insertAfter(pos, init.begin(), init.end());
        }

        Iterator eraseAfter(ConstIterator pos) noexcept
        {
            ForwardListNodeBase* const p      = pos.node();
            ForwardListNodeBase* const victim = p->mpNext;
            WE_ASSERT(victim != nullptr);
            p->mpNext = victim->mpNext;
            destroyNode(victim);
            --mSize;
            return Iterator(p->mpNext);
        }

        Iterator eraseAfter(ConstIterator first, ConstIterator last) noexcept
        {
            ForwardListNodeBase* const p = first.node();
            ForwardListNodeBase* const l = last.node();
            ForwardListNodeBase* cur     = p->mpNext;
            while (cur != l)
            {
                ForwardListNodeBase* const nxt = cur->mpNext;
                destroyNode(cur);
                --mSize;
                cur = nxt;
            }
            p->mpNext = l;
            return Iterator(l);
        }

        void clear() noexcept { destroyAll(); }

        void resize(SizeType n)
        {
            WE_ASSERT(n <= N);
            if (n < mSize)
            {
                shrinkTo(n);
            }
            else if (n > mSize)
            {
                ForwardListNodeBase* tail = lastNode();
                while (mSize < n)
                {
                    tail = appendNode(tail);
                }
            }
        }

        void resize(SizeType n, ConstReference value)
        {
            WE_ASSERT(n <= N);
            if (n < mSize)
            {
                shrinkTo(n);
            }
            else if (n > mSize)
            {
                ForwardListNodeBase* tail = lastNode();
                while (mSize < n)
                {
                    tail = appendNode(tail, value);
                }
            }
        }

        // O(n) element-wise swap (inline storage cannot be pointer-swapped).
        void swap(FixedSList& other) noexcept
        {
            FixedSList tmp(worse::core::move(*this));
            *this = worse::core::move(other);
            other = worse::core::move(tmp);
        }

        // --- in-place algorithms (relink this list's own nodes) ----------------

        SizeType remove(ConstReference value)
        {
            SizeType removed          = 0;
            ForwardListNodeBase* prev = beforeBeginPtr();
            ForwardListNodeBase* cur  = mBeforeBegin.mpNext;
            while (cur != nullptr)
            {
                if (static_cast<Node*>(cur)->mValue == value)
                {
                    prev->mpNext = cur->mpNext;
                    destroyNode(cur);
                    --mSize;
                    ++removed;
                    cur = prev->mpNext;
                }
                else
                {
                    prev = cur;
                    cur  = cur->mpNext;
                }
            }
            return removed;
        }

        template <typename Pred>
            requires PredicateFor<Pred, T>
        SizeType removeIf(Pred pred)
        {
            SizeType removed          = 0;
            ForwardListNodeBase* prev = beforeBeginPtr();
            ForwardListNodeBase* cur  = mBeforeBegin.mpNext;
            while (cur != nullptr)
            {
                if (pred(static_cast<Node*>(cur)->mValue))
                {
                    prev->mpNext = cur->mpNext;
                    destroyNode(cur);
                    --mSize;
                    ++removed;
                    cur = prev->mpNext;
                }
                else
                {
                    prev = cur;
                    cur  = cur->mpNext;
                }
            }
            return removed;
        }

        SizeType unique() { return unique(EqualTo<>{}); }

        template <typename BinPred>
        SizeType unique(BinPred pred)
        {
            SizeType removed         = 0;
            ForwardListNodeBase* cur = mBeforeBegin.mpNext;
            if (cur == nullptr)
            {
                return 0;
            }
            while (cur->mpNext != nullptr)
            {
                ForwardListNodeBase* const nxt = cur->mpNext;
                if (pred(static_cast<Node*>(cur)->mValue, static_cast<Node*>(nxt)->mValue))
                {
                    cur->mpNext = nxt->mpNext;
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

        // Element-wise merge of sorted `other` (moves values into THIS pool; O(n),
        // capacity-checked; `other` ends empty; stable).
        void merge(FixedSList& other) { merge(other, Less<>{}); }

        template <typename Compare>
            requires CompareFor<Compare, T>
        void merge(FixedSList& other, Compare comp)
        {
            if (this == &other || other.empty())
            {
                return;
            }
            WE_ASSERT(mSize + other.mSize <= N);
            ForwardListNodeBase* prev = beforeBeginPtr();
            ForwardListNodeBase* a    = mBeforeBegin.mpNext;
            while (a != nullptr && !other.empty())
            {
                ForwardListNodeBase* const b = other.mBeforeBegin.mpNext;
                if (comp(static_cast<Node*>(b)->mValue, static_cast<Node*>(a)->mValue))
                {
                    Node* node   = createNode(worse::core::move(static_cast<Node*>(b)->mValue));
                    node->mpNext = a;
                    prev->mpNext = node;
                    prev         = node;
                    ++mSize;
                    other.popFront();
                }
                else
                {
                    prev = a;
                    a    = a->mpNext;
                }
            }
            while (!other.empty())
            {
                ForwardListNodeBase* const b = other.mBeforeBegin.mpNext;
                Node* node                   = createNode(worse::core::move(static_cast<Node*>(b)->mValue));
                node->mpNext                 = nullptr;
                prev->mpNext                 = node;
                prev                         = node;
                ++mSize;
                other.popFront();
            }
        }

        // sort: allocation-free, stable, bottom-up binned merge sort over bare chains (same as
        // ForwardList::sort). `mSize` invariant.
        void sort() { sort(Less<>{}); }

        template <typename Compare>
            requires CompareFor<Compare, T>
        void sort(Compare comp)
        {
            if (mSize < 2)
            {
                return;
            }
            ForwardListNodeBase* counter[kSortBins] = {};
            int fill                                = 0;
            ForwardListNodeBase* cur                = mBeforeBegin.mpNext;
            while (cur != nullptr)
            {
                ForwardListNodeBase* carry = cur;
                cur                        = cur->mpNext;
                carry->mpNext              = nullptr;
                int i                      = 0;
                while (i < fill && counter[i] != nullptr)
                {
                    carry      = mergeChains(counter[i], carry, comp);
                    counter[i] = nullptr;
                    ++i;
                }
                counter[i] = carry;
                if (i == fill)
                {
                    ++fill;
                }
            }
            for (int i = 1; i < fill; ++i)
            {
                counter[i] = mergeChains(counter[i], counter[i - 1], comp);
            }
            mBeforeBegin.mpNext = counter[fill - 1];
        }

        void reverse() noexcept
        {
            ForwardListNodeBase* prev = nullptr;
            ForwardListNodeBase* cur  = mBeforeBegin.mpNext;
            while (cur != nullptr)
            {
                ForwardListNodeBase* const nxt = cur->mpNext;
                cur->mpNext                    = prev;
                prev                           = cur;
                cur                            = nxt;
            }
            mBeforeBegin.mpNext = prev;
        }

    private:
        alignas(Node) std::byte mStorage[sizeof(Node) * N];
        ForwardListNodeBase mBeforeBegin;
        ForwardListNodeBase* mpFree = nullptr;
        SizeType mSize              = 0;

        WE_NODISCARD ForwardListNodeBase* beforeBeginPtr() const noexcept
        {
            return const_cast<ForwardListNodeBase*>(&mBeforeBegin);
        }

        WE_NODISCARD Node* slotAt(usize i) noexcept
        {
            return reinterpret_cast<Node*>(mStorage + i * sizeof(Node));
        }

        void initPool() noexcept
        {
            mpFree = nullptr;
            for (usize i = N; i-- > 0;)
            {
                ForwardListNodeBase* slot = slotAt(i);
                slot->mpNext              = mpFree;
                mpFree                    = slot;
            }
            mBeforeBegin.mpNext = nullptr;
            mSize               = 0;
        }

        WE_NODISCARD ForwardListNodeBase* allocSlot() noexcept
        {
            WE_ASSERT(mpFree != nullptr); // hard cap -- inline pool exhausted, no heap spill
            ForwardListNodeBase* const slot = mpFree;
            mpFree                          = mpFree->mpNext;
            return slot;
        }

        void freeSlot(ForwardListNodeBase* n) noexcept
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

        void destroyNode(ForwardListNodeBase* n) noexcept
        {
            std::destroy_at(&static_cast<Node*>(n)->mValue);
            freeSlot(n);
        }

        void destroyAll() noexcept
        {
            ForwardListNodeBase* cur = mBeforeBegin.mpNext;
            while (cur != nullptr)
            {
                ForwardListNodeBase* const nxt = cur->mpNext;
                std::destroy_at(&static_cast<Node*>(cur)->mValue);
                cur = nxt;
            }
            initPool();
        }

        template <typename... Args>
        ForwardListNodeBase* appendNode(ForwardListNodeBase* tail, Args&&... args)
        {
            Node* node   = createNode(worse::core::forward<Args>(args)...);
            node->mpNext = nullptr;
            tail->mpNext = node;
            ++mSize;
            return node;
        }

        void copyFrom(FixedSList const& other)
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (ForwardListNodeBase* cur = other.mBeforeBegin.mpNext; cur != nullptr; cur = cur->mpNext)
            {
                tail = appendNode(tail, static_cast<Node const*>(cur)->mValue);
            }
        }

        void moveFrom(FixedSList& other)
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (ForwardListNodeBase* cur = other.mBeforeBegin.mpNext; cur != nullptr; cur = cur->mpNext)
            {
                tail = appendNode(tail, worse::core::move(static_cast<Node*>(cur)->mValue));
            }
            other.clear();
        }

        WE_NODISCARD ForwardListNodeBase* lastNode() noexcept
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            while (tail->mpNext != nullptr)
            {
                tail = tail->mpNext;
            }
            return tail;
        }

        void shrinkTo(SizeType n) noexcept
        {
            ForwardListNodeBase* prev = beforeBeginPtr();
            for (SizeType i = 0; i < n; ++i)
            {
                prev = prev->mpNext;
            }
            ForwardListNodeBase* cur = prev->mpNext;
            while (cur != nullptr)
            {
                ForwardListNodeBase* const nxt = cur->mpNext;
                std::destroy_at(&static_cast<Node*>(cur)->mValue);
                freeSlot(cur);
                --mSize;
                cur = nxt;
            }
            prev->mpNext = nullptr;
        }

        template <typename Compare>
        static ForwardListNodeBase* mergeChains(ForwardListNodeBase* dst, ForwardListNodeBase* src, Compare& comp)
        {
            ForwardListNodeBase dummy;
            ForwardListNodeBase* tail = &dummy;
            while (dst != nullptr && src != nullptr)
            {
                if (comp(static_cast<Node*>(src)->mValue, static_cast<Node*>(dst)->mValue))
                {
                    tail->mpNext = src;
                    tail         = src;
                    src          = src->mpNext;
                }
                else
                {
                    tail->mpNext = dst;
                    tail         = dst;
                    dst          = dst->mpNext;
                }
            }
            tail->mpNext = (dst != nullptr) ? dst : src;
            return dummy.mpNext;
        }
    };

    template <typename T, usize N>
    void swap(FixedSList<T, N>& a, FixedSList<T, N>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
