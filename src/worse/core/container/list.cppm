module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.list;
import worse.core.basic_type;
import worse.core.memory;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;
import worse.core.container.iterator;

// Allocating doubly-linked list (the engine's std::list / EASTL list / Unreal
// TDoubleLinkedList analogue). Each element lives in its own heap-allocated node threaded
// onto a circular ring with an embedded sentinel `mAnchor`. Unlike `Array`/hash, a node
// list buys you two things and ONLY these two are worth its per-node allocation:
//
//   * STABLE addresses  — an element's address/iterator never moves for its whole lifetime,
//                         regardless of insert/erase elsewhere.
//   * O(1) splice/erase — move a sub-range between lists, or drop one element, in constant
//                         time with no element moves.
//
// The engine idiom for free-lists, LRU chains, command/undo queues, and any structure that
// hands out long-lived element handles. It is, however, CACHE-HOSTILE (a pointer chase per
// element) and hits the allocator once per element on the default allocator. For hot paths
// prefer `Array` (contiguous), or supply a pool/arena allocator, or use the upcoming
// `fixed_list` (inline zero-heap node pool). This is a conscious EASTL/Unreal-flavored
// design, NOT a std clone: `size()` is cached O(1) (R30), and the full allocation-free
// algorithm surface (splice/merge/sort/unique/remove/reverse) is provided.
//
// Iterator / reference stability contract:
//   * insert  — NEVER invalidates any existing iterator/reference/pointer.
//   * erase   — invalidates ONLY iterators/references to the erased element; all others stay.
//   * splice  — does NOT invalidate iterators to the moved nodes: an iterator taken from the
//               source list keeps pointing at the same element after it lands here.
//   * sort/merge/reverse — reorder by relinking; invalidate NOTHING.
// This is the exact opposite of `Array`/hash, which invalidate broadly on growth/erase.
//
// Internal `move`/`forward`/`swap` calls are FULLY QUALIFIED (worse::core::*) to dodge the
// std:: ADL clash. The list is NOT declared trivially relocatable: when non-empty the
// embedded sentinel is the target of self-referential boundary links, so a memcpy of the
// container object would corrupt the ring (R29).
namespace worse::core::container
{
    // --- nodes ----------------------------------------------------------------
    //
    // A pointer-only base used both as the embedded sentinel (no `T` constructed for the
    // anchor) and as the static base of every value-carrying node, so all link surgery
    // operates uniformly on `ListNodeBase*` and never needs to know `T`. Exported so the
    // inline-pool variant (`fixed_list`) can reuse the exact node layout + the `ListIterator`
    // below for full iterator interop with `List`.
    export struct ListNodeBase
    {
        ListNodeBase* mpNext = nullptr;
        ListNodeBase* mpPrev = nullptr;
    };

    export template <typename T>
    struct ListNode : ListNodeBase
    {
        T mValue;
        // Never constructed/destroyed as a whole object: we allocate raw bytes, set the two
        // base links by hand, and construct/destroy ONLY the `mValue` subobject.
    };

    // Bidirectional iterator over the ring. `ValueT` carries const-ness (`T` or `T const`);
    // the node pointer is stored non-const (traversal never mutates a node, and a const list
    // still yields valid node addresses). Dereference downcasts the base-node pointer to the
    // value-carrying node -- valid for every real node; the anchor must never be dereferenced.
    export template <typename T, typename ValueT>
    class ListIterator
    {
    public:
        using IteratorCategory = BidirectionalIteratorTag;
        using ValueType        = T;
        using DifferenceType   = isize;
        using Pointer          = ValueT*;
        using Reference        = ValueT&;

        constexpr ListIterator() = default;
        constexpr explicit ListIterator(ListNodeBase* node) noexcept : mpNode(node) {}

        // Non-const -> const conversion (enabled only when this is the const iterator and
        // `other` is the matching mutable one).
        template <typename U>
            requires(IsSame<ValueT, U const>)
        constexpr ListIterator(ListIterator<T, U> const& other) noexcept : mpNode(other.node())
        {
        }

        WE_NODISCARD Reference operator*() const noexcept { return static_cast<ListNode<T>*>(mpNode)->mValue; }
        WE_NODISCARD Pointer operator->() const noexcept { return &static_cast<ListNode<T>*>(mpNode)->mValue; }

        constexpr ListIterator& operator++() noexcept
        {
            mpNode = mpNode->mpNext;
            return *this;
        }
        constexpr ListIterator operator++(int) noexcept
        {
            ListIterator tmp = *this;
            mpNode           = mpNode->mpNext;
            return tmp;
        }
        constexpr ListIterator& operator--() noexcept
        {
            mpNode = mpNode->mpPrev;
            return *this;
        }
        constexpr ListIterator operator--(int) noexcept
        {
            ListIterator tmp = *this;
            mpNode           = mpNode->mpPrev;
            return tmp;
        }

        WE_NODISCARD constexpr ListNodeBase* node() const noexcept { return mpNode; }

        WE_NODISCARD friend constexpr bool operator==(ListIterator const& a, ListIterator const& b) noexcept
        {
            return a.mpNode == b.mpNode;
        }
        WE_NODISCARD friend constexpr bool operator!=(ListIterator const& a, ListIterator const& b) noexcept
        {
            return a.mpNode != b.mpNode;
        }

    private:
        ListNodeBase* mpNode = nullptr;
    };

    // Storage + RAII half (non-exported). Owns the allocator (EBO), the embedded circular
    // sentinel and the O(1) size counter, plus the per-node allocate/construct/free helpers
    // and the raw link-surgery primitives. The BASE destructor frees ALL nodes (value +
    // storage): unlike ArrayBase's "raw-free in base, element-destroy in derived" split, a
    // node list cannot separate the two -- each node IS both storage and a live value, freed
    // in one step, and both need the allocator the base owns (R25). The derived `List` needs
    // no destructor.
    template <typename T, typename Allocator>
    class ListBase
    {
    public:
        using ValueType      = T;
        using AllocatorType  = Allocator;
        using AllocTraits    = AllocatorTraits<Allocator>;
        using SizeType       = usize;
        using DifferenceType = isize;
        using NodeBase       = ListNodeBase;
        using Node           = ListNode<T>;

        ListBase() noexcept(IsNothrowDefaultConstructible<Allocator>) { resetAnchor(); }
        explicit ListBase(AllocatorType const& allocator) noexcept : mAllocator{allocator} { resetAnchor(); }
        ~ListBase() noexcept { clearNodes(); }

        WE_NODISCARD AllocatorType& getAllocator() noexcept { return mAllocator; }
        WE_NODISCARD AllocatorType const& getAllocator() const noexcept { return mAllocator; }

    protected:
        ListNodeBase mAnchor{};
        SizeType mSize = 0;
        WE_NO_UNIQUE_ADDRESS AllocatorType mAllocator{};

        WE_NODISCARD ListNodeBase* anchorPtr() const noexcept { return const_cast<ListNodeBase*>(&mAnchor); }

        WE_NODISCARD static Node* downcast(ListNodeBase* n) noexcept { return static_cast<Node*>(n); }
        WE_NODISCARD static T& valueOf(ListNodeBase* n) noexcept { return static_cast<Node*>(n)->mValue; }

        void resetAnchor() noexcept
        {
            mAnchor.mpNext = &mAnchor;
            mAnchor.mpPrev = &mAnchor;
            mSize          = 0;
        }

        // Allocate raw storage for a full node, init the base links, then construct ONLY the
        // value subobject from the caller's args (never construct the whole ListNode).
        template <typename... Args>
        WE_NODISCARD Node* createNode(Args&&... args)
        {
            void* raw = AllocTraits::allocate(mAllocator, sizeof(Node), alignof(Node));
            if (raw == nullptr)
            {
                memory::handleAllocationFailure(sizeof(Node), alignof(Node));
            }
            Node* node   = static_cast<Node*>(raw);
            node->mpNext = nullptr;
            node->mpPrev = nullptr;
            AllocTraits::construct(mAllocator, &node->mValue, worse::core::forward<Args>(args)...);
            return node;
        }

        // Destroy the value subobject (symmetric to createNode) then free the raw storage.
        void destroyNode(ListNodeBase* n) noexcept
        {
            Node* node = static_cast<Node*>(n);
            AllocTraits::destroy(mAllocator, &node->mValue);
            AllocTraits::deallocate(mAllocator, node, sizeof(Node), alignof(Node));
        }

        // Walk the ring, freeing every node, then return to the empty (self-circular) state.
        void clearNodes() noexcept
        {
            ListNodeBase* cur = mAnchor.mpNext;
            while (cur != anchorPtr())
            {
                ListNodeBase* const nxt = cur->mpNext;
                destroyNode(cur);
                cur = nxt;
            }
            resetAnchor();
        }

        // Splice `n` into the ring immediately before `pos`.
        static void linkBefore(ListNodeBase* pos, ListNodeBase* n) noexcept
        {
            n->mpPrev           = pos->mpPrev;
            n->mpNext           = pos;
            pos->mpPrev->mpNext = n;
            pos->mpPrev         = n;
        }

        // Excise `n` from its ring (links are NOT nulled: the node is freed immediately, so
        // nulling would be dead work).
        static void unlink(ListNodeBase* n) noexcept
        {
            n->mpPrev->mpNext = n->mpNext;
            n->mpNext->mpPrev = n->mpPrev;
        }

        // The std `__transfer` primitive: move the chain [first, last) (last exclusive) out of
        // its current ring and splice it immediately before `pos`. Pure six-pointer surgery;
        // works within one ring or across two. Workhorse of splice/merge/sort/reverse.
        static void transfer(ListNodeBase* pos, ListNodeBase* first, ListNodeBase* last) noexcept
        {
            if (pos == first || first == last)
            {
                return;
            }
            ListNodeBase* const tail    = last->mpPrev; // last real node of the moved chain
            ListNodeBase* const srcPrev = first->mpPrev;
            // Excise [first, tail] from the source ring.
            srcPrev->mpNext = last;
            last->mpPrev    = srcPrev;
            // Splice [first, tail] before pos.
            ListNodeBase* const posPrev = pos->mpPrev;
            posPrev->mpNext             = first;
            first->mpPrev               = posPrev;
            tail->mpNext                = pos;
            pos->mpPrev                 = tail;
        }

        // Adopt every node of `other` into this (assumed-empty) ring, re-seating the boundary
        // links onto this anchor; `other` is left empty. Copies the size counter wholesale.
        void spliceAll(ListBase& other) noexcept
        {
            if (other.mAnchor.mpNext == other.anchorPtr())
            {
                return;
            }
            mAnchor.mpNext         = other.mAnchor.mpNext;
            mAnchor.mpPrev         = other.mAnchor.mpPrev;
            mAnchor.mpNext->mpPrev = anchorPtr();
            mAnchor.mpPrev->mpNext = anchorPtr();
            mSize                  = other.mSize;
            other.resetAnchor();
        }

        // After a swap copied another anchor's link fields into `anchor`, point the boundary
        // nodes back at `anchor` -- or make it self-circular if it ended up empty (its
        // swapped-in links still reference `otherAnchor`).
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
    };

    export template <typename T, typename Allocator = WE_DEFAULT_ALLOCATOR>
    class List : public ListBase<T, Allocator>
    {
        using BaseType = ListBase<T, Allocator>;
        using ThisType = List<T, Allocator>;

        using BaseType::anchorPtr;
        using BaseType::clearNodes;
        using BaseType::createNode;
        using BaseType::destroyNode;
        using BaseType::downcast;
        using BaseType::linkBefore;
        using BaseType::mAllocator;
        using BaseType::mAnchor;
        using BaseType::mSize;
        using BaseType::reseat;
        using BaseType::resetAnchor;
        using BaseType::spliceAll;
        using BaseType::transfer;
        using BaseType::unlink;
        using BaseType::valueOf;

        using AllocTraits = AllocatorTraits<Allocator>;
        using Node        = typename BaseType::Node;
        using NodeBase    = typename BaseType::NodeBase;

        // Max bins for the allocation-free binned merge sort: one slot per power-of-two run
        // length, so 64 covers any list that fits in memory.
        static constexpr usize kSortBins = 64;

    public:
        using ValueType      = T;
        using SizeType       = typename BaseType::SizeType;
        using DifferenceType = typename BaseType::DifferenceType;
        using AllocatorType  = Allocator;

        using Reference      = T&;
        using ConstReference = T const&;
        using Pointer        = T*;
        using ConstPointer   = T const*;

        using Iterator         = ListIterator<T, T>;
        using ConstIterator    = ListIterator<T, T const>;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        static_assert(!IsConst<T>, "List<T>::ValueType must be non-const");
        static_assert(!IsVolatile<T>, "List<T>::ValueType must be non-volatile");

        // --- construction / destruction ---------------------------------------

        List() noexcept(IsNothrowDefaultConstructible<AllocatorType>) : BaseType{} {}

        explicit List(AllocatorType const& allocator) noexcept : BaseType{allocator} {}

        explicit List(SizeType n, AllocatorType const& allocator = AllocatorType{}) : BaseType{allocator}
        {
            for (SizeType i = 0; i < n; ++i)
            {
                emplaceBack();
            }
        }

        List(SizeType n, ConstReference value, AllocatorType const& allocator = AllocatorType{}) : BaseType{allocator}
        {
            for (SizeType i = 0; i < n; ++i)
            {
                emplaceBack(value);
            }
        }

        template <typename InIt>
            requires InputIterator<InIt>
        List(InIt first, InIt last, AllocatorType const& allocator = AllocatorType{}) : BaseType{allocator}
        {
            for (; first != last; ++first)
            {
                emplaceBack(*first);
            }
        }

        List(std::initializer_list<T> init, AllocatorType const& allocator = AllocatorType{}) : BaseType{allocator}
        {
            for (auto const& v : init)
            {
                emplaceBack(v);
            }
        }

        List(ThisType const& other) : BaseType{AllocTraits::selectOnContainerCopyConstruction(other.getAllocator())}
        {
            for (ListNodeBase* cur = other.mAnchor.mpNext; cur != other.anchorPtr(); cur = cur->mpNext)
            {
                emplaceBack(static_cast<Node const*>(cur)->mValue);
            }
        }

        List(ThisType&& other) noexcept : BaseType{}
        {
            mAllocator = worse::core::move(other.mAllocator);
            spliceAll(other);
        }

        ~List() noexcept = default;

        ThisType& operator=(ThisType const& other)
        {
            if (this != &other)
            {
                if constexpr (AllocTraits::propagateOnContainerCopyAssignment)
                {
                    clear();
                    mAllocator = other.mAllocator;
                }
                else
                {
                    clear();
                }
                for (ListNodeBase* cur = other.mAnchor.mpNext; cur != other.anchorPtr(); cur = cur->mpNext)
                {
                    emplaceBack(static_cast<Node const*>(cur)->mValue);
                }
            }
            return *this;
        }

        ThisType& operator=(ThisType&& other) noexcept(AllocTraits::isAlwaysEqual)
        {
            if (this != &other)
            {
                clear();
                if constexpr (AllocTraits::propagateOnContainerMoveAssignment)
                {
                    mAllocator = worse::core::move(other.mAllocator);
                    spliceAll(other);
                }
                else if (AllocTraits::equal(mAllocator, other.mAllocator))
                {
                    spliceAll(other);
                }
                else
                {
                    // Non-propagating + non-equal allocators: a node allocated by `other`'s
                    // allocator must be freed by an allocator equal to it, so we cannot steal
                    // its nodes. Move each element into freshly-allocated nodes of OUR
                    // allocator, then let `other` destroy its own.
                    for (ListNodeBase* cur = other.mAnchor.mpNext; cur != other.anchorPtr(); cur = cur->mpNext)
                    {
                        emplaceBack(worse::core::move(static_cast<Node*>(cur)->mValue));
                    }
                    other.clear();
                }
            }
            return *this;
        }

        ThisType& operator=(std::initializer_list<T> init)
        {
            assign(init.begin(), init.end());
            return *this;
        }

        void assign(SizeType n, ConstReference value)
        {
            clear();
            for (SizeType i = 0; i < n; ++i)
            {
                emplaceBack(value);
            }
        }

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

        void assign(std::initializer_list<T> init) { assign(init.begin(), init.end()); }

        WE_NODISCARD AllocatorType getAllocator() const noexcept { return BaseType::getAllocator(); }

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
        WE_NODISCARD SizeType size() const noexcept { return mSize; }

        // --- element access ----------------------------------------------------

        WE_NODISCARD Reference front() noexcept
        {
            WE_ASSERT(!empty());
            return valueOf(mAnchor.mpNext);
        }
        WE_NODISCARD ConstReference front() const noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node const*>(mAnchor.mpNext)->mValue;
        }
        WE_NODISCARD Reference back() noexcept
        {
            WE_ASSERT(!empty());
            return valueOf(mAnchor.mpPrev);
        }
        WE_NODISCARD ConstReference back() const noexcept
        {
            WE_ASSERT(!empty());
            return static_cast<Node const*>(mAnchor.mpPrev)->mValue;
        }

        // --- modifiers: ends ---------------------------------------------------

        template <typename... Args>
        Reference emplaceFront(Args&&... args)
        {
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(mAnchor.mpNext, node);
            ++mSize;
            return node->mValue;
        }
        template <typename... Args>
        Reference emplaceBack(Args&&... args)
        {
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(anchorPtr(), node);
            ++mSize;
            return node->mValue;
        }

        void pushFront(ConstReference value) { emplaceFront(value); }
        void pushFront(T&& value) { emplaceFront(worse::core::move(value)); }
        void pushBack(ConstReference value) { emplaceBack(value); }
        void pushBack(T&& value) { emplaceBack(worse::core::move(value)); }

        void popFront() noexcept
        {
            WE_ASSERT(!empty());
            ListNodeBase* const n = mAnchor.mpNext;
            unlink(n);
            destroyNode(n);
            --mSize;
        }
        void popBack() noexcept
        {
            WE_ASSERT(!empty());
            ListNodeBase* const n = mAnchor.mpPrev;
            unlink(n);
            destroyNode(n);
            --mSize;
        }

        // --- modifiers: arbitrary position -------------------------------------

        template <typename... Args>
        Iterator emplace(ConstIterator pos, Args&&... args)
        {
            Node* node = createNode(worse::core::forward<Args>(args)...);
            linkBefore(pos.node(), node);
            ++mSize;
            return Iterator(node);
        }

        Iterator insert(ConstIterator pos, ConstReference value) { return emplace(pos, value); }
        Iterator insert(ConstIterator pos, T&& value) { return emplace(pos, worse::core::move(value)); }

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

        Iterator insert(ConstIterator pos, std::initializer_list<T> init)
        {
            return insert(pos, init.begin(), init.end());
        }

        Iterator erase(ConstIterator pos) noexcept
        {
            ListNodeBase* const n   = pos.node();
            ListNodeBase* const nxt = n->mpNext;
            unlink(n);
            destroyNode(n);
            --mSize;
            return Iterator(nxt);
        }

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

        void clear() noexcept { clearNodes(); }

        void resize(SizeType n)
        {
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
            while (mSize > n)
            {
                popBack();
            }
            while (mSize < n)
            {
                emplaceBack(value);
            }
        }

        void swap(ThisType& other) noexcept(AllocTraits::isAlwaysEqual)
        {
            worse::core::swap(mAnchor.mpNext, other.mAnchor.mpNext);
            worse::core::swap(mAnchor.mpPrev, other.mAnchor.mpPrev);
            worse::core::swap(mSize, other.mSize);
            reseat(mAnchor, other.anchorPtr());
            reseat(other.mAnchor, anchorPtr());
            if constexpr (AllocTraits::propagateOnContainerSwap)
            {
                worse::core::swap(mAllocator, other.mAllocator);
            }
        }

        // --- list algorithms (allocation-free: pure node relinking) ------------

        // splice: steal nodes from `other` before `pos`. No allocation, no element moves.
        // Cross-list splices keep both O(1) `size()` counters correct (range splice counts the
        // range, O(distance) -- R26); same-list relinking leaves the count untouched.
        void splice(ConstIterator pos, ThisType& other) noexcept
        {
            if (this == &other || other.empty())
            {
                return;
            }
            SizeType const n = other.mSize;
            transfer(pos.node(), other.mAnchor.mpNext, other.anchorPtr());
            mSize += n;
            other.mSize = 0;
        }
        void splice(ConstIterator pos, ThisType&& other) noexcept { splice(pos, other); }

        void splice(ConstIterator pos, ThisType& other, ConstIterator it) noexcept
        {
            ListNodeBase* const n = it.node();
            if (this != &other)
            {
                ++mSize;
                --other.mSize;
            }
            transfer(pos.node(), n, n->mpNext);
        }
        void splice(ConstIterator pos, ThisType&& other, ConstIterator it) noexcept { splice(pos, other, it); }

        void splice(ConstIterator pos, ThisType& other, ConstIterator first, ConstIterator last) noexcept
        {
            if (first == last)
            {
                return;
            }
            if (this != &other)
            {
                SizeType const n = static_cast<SizeType>(worse::core::distance(first, last));
                mSize += n;
                other.mSize -= n;
            }
            transfer(pos.node(), first.node(), last.node());
        }
        void splice(ConstIterator pos, ThisType&& other, ConstIterator first, ConstIterator last) noexcept
        {
            splice(pos, other, first, last);
        }

        // remove / removeIf: drop every matching element; returns the count removed (R27).
        // `value` must not alias an element of this list.
        SizeType remove(ConstReference value)
        {
            SizeType removed  = 0;
            ListNodeBase* cur = mAnchor.mpNext;
            while (cur != anchorPtr())
            {
                ListNodeBase* const nxt = cur->mpNext;
                if (valueOf(cur) == value)
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

        template <typename Pred>
            requires PredicateFor<Pred, T>
        SizeType removeIf(Pred pred)
        {
            SizeType removed  = 0;
            ListNodeBase* cur = mAnchor.mpNext;
            while (cur != anchorPtr())
            {
                ListNodeBase* const nxt = cur->mpNext;
                if (pred(valueOf(cur)))
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

        // unique: collapse consecutive equal runs to a single element; returns count removed.
        SizeType unique() { return unique(EqualTo<>{}); }

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
                if (pred(valueOf(cur), valueOf(nxt)))
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

        // merge: stably merge sorted `other` into this sorted list; `other` ends empty. Both
        // must already be sorted by the comparator. Equal elements keep this-before-other.
        void merge(ThisType& other) { merge(other, Less<>{}); }
        void merge(ThisType&& other) { merge(other, Less<>{}); }

        template <typename Compare>
            requires CompareFor<Compare, T>
        void merge(ThisType& other, Compare comp)
        {
            if (this == &other)
            {
                return;
            }
            ListNodeBase* a          = mAnchor.mpNext;
            ListNodeBase* const aEnd = anchorPtr();
            ListNodeBase* b          = other.mAnchor.mpNext;
            ListNodeBase* const bEnd = other.anchorPtr();
            while (a != aEnd && b != bEnd)
            {
                if (comp(valueOf(b), valueOf(a)))
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
            mSize += other.mSize;
            other.mSize = 0;
        }
        template <typename Compare>
            requires CompareFor<Compare, T>
        void merge(ThisType&& other, Compare comp)
        {
            merge(other, comp);
        }

        // sort: allocation-free, stable, in-place bottom-up binned merge sort (the SGI/EASTL
        // list::sort). Uses only O(log n) stack pointers (the bin anchors) and never copies a
        // value or touches the allocator. `mSize` is invariant under sort.
        void sort() { sort(Less<>{}); }

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
                // Move one node (the current front of *this) into the empty `carry` ring.
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
            // counter[fill-1] now holds the fully sorted ring; move it back into *this (empty).
            ListNodeBase& sorted = counter[fill - 1];
            transfer(anchorPtr(), sorted.mpNext, &sorted);
        }

        // reverse: swap every node's next/prev pointers (including the anchor's). O(n), O(1)
        // extra, no element moves. `mSize` unchanged.
        void reverse() noexcept
        {
            ListNodeBase* cur = anchorPtr();
            do
            {
                worse::core::swap(cur->mpNext, cur->mpPrev);
                cur = cur->mpPrev; // the old mpNext
            } while (cur != anchorPtr());
        }

    private:
        // Stably merge the sorted bare ring `src` into the sorted bare ring `dst` (no size
        // accounting); `src` ends empty. Used only by `sort` on stack-local anchors.
        template <typename Compare>
        static void mergeRings(ListNodeBase& dst, ListNodeBase& src, Compare& comp)
        {
            ListNodeBase* a          = dst.mpNext;
            ListNodeBase* const aEnd = &dst;
            ListNodeBase* b          = src.mpNext;
            ListNodeBase* const bEnd = &src;
            while (a != aEnd && b != bEnd)
            {
                if (comp(valueOf(b), valueOf(a)))
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

        // Swap two bare rings (no size/allocator), reseating both anchors -- the link-only
        // core of swap(), used by sort on stack-local anchors.
        static void swapRings(ListNodeBase& a, ListNodeBase& b) noexcept
        {
            worse::core::swap(a.mpNext, b.mpNext);
            worse::core::swap(a.mpPrev, b.mpPrev);
            reseat(a, &b);
            reseat(b, &a);
        }
    };

    export template <typename T, typename Allocator>
    void swap(List<T, Allocator>& a, List<T, Allocator>& b) noexcept(noexcept(a.swap(b)))
    {
        a.swap(b);
    }

} // namespace worse::core::container
