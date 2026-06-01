module;

#include "worse/core/macro.hpp"

export module worse.core.container.intrusive_list;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

// Intrusive doubly-linked list: the link pointers live INSIDE the element (the user's
// type derives from IntrusiveListNode), so the list allocates NOTHING and owns nothing
// -- splicing a node in/out is O(1) pointer surgery with zero heap traffic. This is the
// engine idiom for objects that must thread onto a list without a per-node allocation
// (e.g. active entities, dirty-flag chains, free lists).
//
// Storage model: a circular ring with an embedded sentinel `mAnchor`. An empty list is
// the anchor pointing at itself; `begin()` is `mAnchor.mpNext`, `end()` is `&mAnchor`.
// Because the sentinel is part of the list object, MOVE and SWAP must re-seat the first
// and last real nodes' links to the new anchor address (handled below).
//
// Contracts (intrusive lists are inherently sharp-edged -- documented, not enforced):
//   * The list NEVER constructs or destroys elements; lifetime is the caller's.
//   * All mutation must go through the list API; the O(1) `size()` counter relies on it.
//   * A node may be in at most one IntrusiveList<T> at a time (one link pair per node).
//   * Copying a linked element copies stale link pointers -- don't. The list itself is
//     move-only for the same reason.
export namespace worse::core::container
{
    // Embedded link. The user's element type derives from this (publicly).
    struct IntrusiveListNode
    {
        IntrusiveListNode* mpNext = nullptr;
        IntrusiveListNode* mpPrev = nullptr;
    };

    // Bidirectional iterator over the ring. `ValueT` carries the const-ness (`T` or
    // `T const`); the node pointer is always stored non-const (traversal never mutates a
    // node, and a const list still yields valid node addresses). Dereference downcasts
    // the node back to the element via static_cast -- valid since T derives from the node.
    template <typename T, typename ValueT>
    class IntrusiveListIterator
    {
    public:
        using IteratorCategory = BidirectionalIteratorTag;
        using ValueType        = T;
        using DifferenceType   = isize;
        using Pointer          = ValueT*;
        using Reference        = ValueT&;

        constexpr IntrusiveListIterator() = default;
        constexpr explicit IntrusiveListIterator(IntrusiveListNode* node) noexcept : mpNode(node) {}

        // Non-const -> const conversion (enabled only when this is the const iterator and
        // `other` is the matching mutable one).
        template <typename U>
            requires(IsSame<ValueT, U const>)
        constexpr IntrusiveListIterator(IntrusiveListIterator<T, U> const& other) noexcept : mpNode(other.node())
        {
        }

        WE_NODISCARD Reference operator*() const noexcept { return *static_cast<ValueT*>(mpNode); }
        WE_NODISCARD Pointer operator->() const noexcept { return static_cast<ValueT*>(mpNode); }

        constexpr IntrusiveListIterator& operator++() noexcept
        {
            mpNode = mpNode->mpNext;
            return *this;
        }
        constexpr IntrusiveListIterator operator++(int) noexcept
        {
            IntrusiveListIterator tmp = *this;
            mpNode                    = mpNode->mpNext;
            return tmp;
        }
        constexpr IntrusiveListIterator& operator--() noexcept
        {
            mpNode = mpNode->mpPrev;
            return *this;
        }
        constexpr IntrusiveListIterator operator--(int) noexcept
        {
            IntrusiveListIterator tmp = *this;
            mpNode                    = mpNode->mpPrev;
            return tmp;
        }

        WE_NODISCARD constexpr IntrusiveListNode* node() const noexcept { return mpNode; }

        WE_NODISCARD friend constexpr bool
        operator==(IntrusiveListIterator const& a, IntrusiveListIterator const& b) noexcept
        {
            return a.mpNode == b.mpNode;
        }
        WE_NODISCARD friend constexpr bool
        operator!=(IntrusiveListIterator const& a, IntrusiveListIterator const& b) noexcept
        {
            return a.mpNode != b.mpNode;
        }

    private:
        IntrusiveListNode* mpNode = nullptr;
    };

    template <typename T>
    class IntrusiveList
    {
        static_assert(IsConvertible<T*, IntrusiveListNode*>, "IntrusiveList<T>: T must derive from IntrusiveListNode");

    public:
        using ValueType        = T;
        using SizeType         = usize;
        using Reference        = T&;
        using ConstReference   = T const&;
        using Pointer          = T*;
        using ConstPointer     = T const*;
        using Iterator         = IntrusiveListIterator<T, T>;
        using ConstIterator    = IntrusiveListIterator<T, T const>;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        // --- construction (move-only; owns no element storage) -----------------

        IntrusiveList() noexcept { resetAnchor(); }
        ~IntrusiveList() = default;

        IntrusiveList(IntrusiveList const&)            = delete;
        IntrusiveList& operator=(IntrusiveList const&) = delete;

        IntrusiveList(IntrusiveList&& other) noexcept
        {
            resetAnchor();
            spliceAll(other);
        }
        IntrusiveList& operator=(IntrusiveList&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                spliceAll(other);
            }
            return *this;
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mSize == 0; }
        WE_NODISCARD SizeType size() const noexcept { return mSize; }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return Iterator(mAnchor.mpNext); }
        WE_NODISCARD ConstIterator begin() const noexcept { return ConstIterator(mAnchor.mpNext); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return ConstIterator(mAnchor.mpNext); }
        WE_NODISCARD Iterator end() noexcept { return Iterator(anchorPtr()); }
        WE_NODISCARD ConstIterator end() const noexcept { return ConstIterator(anchorPtr()); }
        WE_NODISCARD ConstIterator cend() const noexcept { return ConstIterator(anchorPtr()); }

        WE_NODISCARD ReverseIter rbegin() noexcept { return ReverseIter(end()); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(cend()); }
        WE_NODISCARD ReverseIter rend() noexcept { return ReverseIter(begin()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(cbegin()); }

        // An iterator to an element already known to be in the list -- the O(1) handle
        // that makes intrusive erase/remove possible without a search.
        WE_NODISCARD static Iterator iteratorTo(T& value) noexcept { return Iterator(nodeOf(value)); }

        // --- element access ----------------------------------------------------

        WE_NODISCARD Reference front() noexcept
        {
            WE_ASSERT(!empty());
            return *static_cast<T*>(mAnchor.mpNext);
        }
        WE_NODISCARD ConstReference front() const noexcept
        {
            WE_ASSERT(!empty());
            return *static_cast<T const*>(mAnchor.mpNext);
        }
        WE_NODISCARD Reference back() noexcept
        {
            WE_ASSERT(!empty());
            return *static_cast<T*>(mAnchor.mpPrev);
        }
        WE_NODISCARD ConstReference back() const noexcept
        {
            WE_ASSERT(!empty());
            return *static_cast<T const*>(mAnchor.mpPrev);
        }

        // --- modifiers ---------------------------------------------------------

        void pushBack(T& value) noexcept
        {
            linkBefore(anchorPtr(), nodeOf(value));
            ++mSize;
        }
        void pushFront(T& value) noexcept
        {
            linkBefore(mAnchor.mpNext, nodeOf(value));
            ++mSize;
        }
        void popBack() noexcept
        {
            WE_ASSERT(!empty());
            unlink(mAnchor.mpPrev);
            --mSize;
        }
        void popFront() noexcept
        {
            WE_ASSERT(!empty());
            unlink(mAnchor.mpNext);
            --mSize;
        }

        // Insert `value` before `pos`; returns an iterator to the inserted element.
        Iterator insert(ConstIterator pos, T& value) noexcept
        {
            IntrusiveListNode* const n = nodeOf(value);
            linkBefore(pos.node(), n);
            ++mSize;
            return Iterator(n);
        }

        // Unlink the element at `pos`; returns an iterator to the following element. The
        // element itself is left intact (its link pointers nulled) -- the caller owns it.
        Iterator erase(ConstIterator pos) noexcept
        {
            WE_ASSERT(!empty());
            IntrusiveListNode* const n    = pos.node();
            IntrusiveListNode* const next = n->mpNext;
            unlink(n);
            --mSize;
            return Iterator(next);
        }

        // Unlink a specific element (must be a member of THIS list).
        void remove(T& value) noexcept
        {
            WE_ASSERT(!empty());
            unlink(nodeOf(value));
            --mSize;
        }

        // Unlink every element (nulling each one's links) and return to the empty state.
        void clear() noexcept
        {
            IntrusiveListNode* cur = mAnchor.mpNext;
            while (cur != anchorPtr())
            {
                IntrusiveListNode* const next = cur->mpNext;
                cur->mpNext = cur->mpPrev = nullptr;
                cur                       = next;
            }
            resetAnchor();
        }

        void swap(IntrusiveList& other) noexcept
        {
            worse::core::swap(mAnchor.mpNext, other.mAnchor.mpNext);
            worse::core::swap(mAnchor.mpPrev, other.mAnchor.mpPrev);
            worse::core::swap(mSize, other.mSize);
            reseat(mAnchor, other.anchorPtr());
            reseat(other.mAnchor, anchorPtr());
        }

    private:
        IntrusiveListNode mAnchor{};
        SizeType mSize = 0;

        WE_NODISCARD IntrusiveListNode* anchorPtr() const noexcept
        {
            return const_cast<IntrusiveListNode*>(&mAnchor);
        }

        WE_NODISCARD static IntrusiveListNode* nodeOf(T& value) noexcept
        {
            return static_cast<IntrusiveListNode*>(&value);
        }

        void resetAnchor() noexcept
        {
            mAnchor.mpNext = &mAnchor;
            mAnchor.mpPrev = &mAnchor;
            mSize          = 0;
        }

        // Splice `n` into the ring immediately before `pos`.
        static void linkBefore(IntrusiveListNode* pos, IntrusiveListNode* n) noexcept
        {
            n->mpPrev           = pos->mpPrev;
            n->mpNext           = pos;
            pos->mpPrev->mpNext = n;
            pos->mpPrev         = n;
        }

        // Excise `n` from its ring and null its links so the detached node is in a clean
        // state (a stale link into a gone list would be a footgun).
        static void unlink(IntrusiveListNode* n) noexcept
        {
            n->mpPrev->mpNext = n->mpNext;
            n->mpNext->mpPrev = n->mpPrev;
            n->mpNext = n->mpPrev = nullptr;
        }

        // Move every node out of `other` into this (empty) list, re-seating the boundary
        // links onto this anchor; `other` is left empty.
        void spliceAll(IntrusiveList& other) noexcept
        {
            if (other.empty())
            {
                return;
            }
            mAnchor.mpNext         = other.mAnchor.mpNext;
            mAnchor.mpPrev         = other.mAnchor.mpPrev;
            mAnchor.mpNext->mpPrev = &mAnchor;
            mAnchor.mpPrev->mpNext = &mAnchor;
            mSize                  = other.mSize;
            other.resetAnchor();
        }

        // After a swap copied another anchor's link fields into `anchor`, point the
        // boundary nodes back at `anchor` -- or make it self-circular if it ended up
        // empty (its swapped-in links still reference `otherAnchor`).
        static void reseat(IntrusiveListNode& anchor, IntrusiveListNode* otherAnchor) noexcept
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
    };

    template <typename T>
    void swap(IntrusiveList<T>& a, IntrusiveList<T>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
