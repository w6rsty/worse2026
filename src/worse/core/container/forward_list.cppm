module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.forward_list;
import worse.core.basic_type;
import worse.core.memory;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;
import worse.core.container.iterator;

// Allocating singly-linked list (EASTL slist / Unreal TLinkedList analogue). Each element
// lives in its own heap node carrying a SINGLE forward pointer; the chain is null-terminated
// (NOT circular). An embedded head sentinel `mBeforeBegin` whose `mpNext` is the first real
// node lets every insert/erase be expressed as O(1) pointer surgery "after" a known node.
//
// This is deliberately NOT the deliberately-crippled std::forward_list: per the engine's
// game-first direction it keeps a cached O(1) `size()` (R30) and the full allocation-free
// algorithm surface (spliceAfter/merge/sort/unique/remove/reverse). What it does NOT have is
// `back()`/`pushBack`/a tail pointer -- those would change the storage shape; the whole point
// of a singly-linked list over `List` is the smaller node (one pointer) and the minimal
// object. The correct mutation API is therefore the `*After` family (insertAfter,
// eraseAfter, spliceAfter), which is what a singly-linked list can do in O(1); to mutate
// before a position you hold the predecessor (`beforeBegin()` is the handle to the head).
//
// WARNING / when to use: like `List` this allocates once per element on the default allocator
// and is cache-hostile (a pointer chase per element). It exists for stable element addresses,
// O(1) front insert/erase, and O(1) single/range splice between lists (free-lists, hash-chain
// buckets, simple stacks). For hot paths prefer `Array`, supply a pool/arena allocator, or use
// the upcoming `fixed_slist` (inline zero-heap node pool). Some operations are unavoidably
// O(n) on a singly-linked list and are documented as such: whole-list spliceAfter and resize
// (no tail pointer), and size-correcting range spliceAfter across lists (R26).
//
// Iterator / reference stability: insert never invalidates; erase invalidates only the erased
// element; spliceAfter keeps iterators to moved nodes valid; sort/merge/reverse relink and
// invalidate nothing. NB: a singly-linked list is NOT circular, so unlike `List` it needs NO
// sentinel reseat on move/swap -- only the head pointer transfers (nothing points back at the
// sentinel). Internal move/forward/swap are FULLY QUALIFIED (worse::core::*). NOT declared
// trivially relocatable (R29), for consistency with `List`.
namespace worse::core::container
{
    // --- nodes (internal: in the namespace, deliberately NOT exported) ---------
    struct ForwardListNodeBase
    {
        ForwardListNodeBase* mpNext = nullptr;
    };

    template <typename T>
    struct ForwardListNode : ForwardListNodeBase
    {
        T mValue;
        // Never constructed/destroyed as a whole: allocate raw bytes, set the base link by
        // hand, construct/destroy ONLY the `mValue` subobject.
    };

    // Forward (single-pass-capable, multipass) iterator. `ValueT` carries const-ness. No
    // `operator--` and no reverse iterators -- a singly-linked node has no back pointer.
    export template <typename T, typename ValueT>
    class ForwardListIterator
    {
    public:
        using IteratorCategory = ForwardIteratorTag;
        using ValueType        = T;
        using DifferenceType   = isize;
        using Pointer          = ValueT*;
        using Reference        = ValueT&;

        constexpr ForwardListIterator() = default;
        constexpr explicit ForwardListIterator(ForwardListNodeBase* node) noexcept : mpNode(node) {}

        template <typename U>
            requires(IsSame<ValueT, U const>)
        constexpr ForwardListIterator(ForwardListIterator<T, U> const& other) noexcept : mpNode(other.node())
        {
        }

        WE_NODISCARD Reference operator*() const noexcept { return static_cast<ForwardListNode<T>*>(mpNode)->mValue; }
        WE_NODISCARD Pointer operator->() const noexcept { return &static_cast<ForwardListNode<T>*>(mpNode)->mValue; }

        constexpr ForwardListIterator& operator++() noexcept
        {
            mpNode = mpNode->mpNext;
            return *this;
        }
        constexpr ForwardListIterator operator++(int) noexcept
        {
            ForwardListIterator tmp = *this;
            mpNode                  = mpNode->mpNext;
            return tmp;
        }

        WE_NODISCARD constexpr ForwardListNodeBase* node() const noexcept { return mpNode; }

        WE_NODISCARD friend constexpr bool operator==(ForwardListIterator const& a, ForwardListIterator const& b) noexcept
        {
            return a.mpNode == b.mpNode;
        }
        WE_NODISCARD friend constexpr bool operator!=(ForwardListIterator const& a, ForwardListIterator const& b) noexcept
        {
            return a.mpNode != b.mpNode;
        }

    private:
        ForwardListNodeBase* mpNode = nullptr;
    };

    // Storage + RAII half (non-exported). Owns the allocator (EBO), the embedded head sentinel
    // and the O(1) size counter, plus per-node allocate/construct/free. The BASE destructor
    // frees ALL nodes (value + storage) -- a node is inseparably storage + a live value and
    // both need the allocator the base owns (R25); the derived needs no destructor.
    template <typename T, typename Allocator>
    class ForwardListBase
    {
    public:
        using ValueType      = T;
        using AllocatorType  = Allocator;
        using AllocTraits    = AllocatorTraits<Allocator>;
        using SizeType       = usize;
        using DifferenceType = isize;
        using NodeBase       = ForwardListNodeBase;
        using Node           = ForwardListNode<T>;

        ForwardListBase() noexcept(IsNothrowDefaultConstructible<Allocator>) { resetHead(); }
        explicit ForwardListBase(AllocatorType const& allocator) noexcept : mAllocator{allocator} { resetHead(); }
        ~ForwardListBase() noexcept { clearNodes(); }

        WE_NODISCARD AllocatorType& getAllocator() noexcept { return mAllocator; }
        WE_NODISCARD AllocatorType const& getAllocator() const noexcept { return mAllocator; }

    protected:
        ForwardListNodeBase mBeforeBegin{};
        SizeType mSize = 0;
        WE_NO_UNIQUE_ADDRESS AllocatorType mAllocator{};

        WE_NODISCARD ForwardListNodeBase* beforeBeginPtr() const noexcept
        {
            return const_cast<ForwardListNodeBase*>(&mBeforeBegin);
        }

        WE_NODISCARD static T& valueOf(ForwardListNodeBase* n) noexcept
        {
            return static_cast<Node*>(n)->mValue;
        }

        void resetHead() noexcept
        {
            mBeforeBegin.mpNext = nullptr;
            mSize               = 0;
        }

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
            AllocTraits::construct(mAllocator, &node->mValue, worse::core::forward<Args>(args)...);
            return node;
        }

        void destroyNode(ForwardListNodeBase* n) noexcept
        {
            Node* node = static_cast<Node*>(n);
            AllocTraits::destroy(mAllocator, &node->mValue);
            AllocTraits::deallocate(mAllocator, node, sizeof(Node), alignof(Node));
        }

        void clearNodes() noexcept
        {
            ForwardListNodeBase* cur = mBeforeBegin.mpNext;
            while (cur != nullptr)
            {
                ForwardListNodeBase* const nxt = cur->mpNext;
                destroyNode(cur);
                cur = nxt;
            }
            resetHead();
        }
    };

    export template <typename T, typename Allocator = WE_DEFAULT_ALLOCATOR>
    class ForwardList : public ForwardListBase<T, Allocator>
    {
        using BaseType = ForwardListBase<T, Allocator>;
        using ThisType = ForwardList<T, Allocator>;

        using BaseType::beforeBeginPtr;
        using BaseType::clearNodes;
        using BaseType::createNode;
        using BaseType::destroyNode;
        using BaseType::mAllocator;
        using BaseType::mBeforeBegin;
        using BaseType::mSize;
        using BaseType::resetHead;
        using BaseType::valueOf;

        using AllocTraits = AllocatorTraits<Allocator>;
        using Node        = typename BaseType::Node;
        using NodeBase    = typename BaseType::NodeBase;

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

        using Iterator      = ForwardListIterator<T, T>;
        using ConstIterator = ForwardListIterator<T, T const>;

        static_assert(!IsConst<T>, "ForwardList<T>::ValueType must be non-const");
        static_assert(!IsVolatile<T>, "ForwardList<T>::ValueType must be non-volatile");

        // --- construction / destruction ---------------------------------------

        ForwardList() noexcept(IsNothrowDefaultConstructible<AllocatorType>) : BaseType{} {}

        explicit ForwardList(AllocatorType const& allocator) noexcept : BaseType{allocator} {}

        explicit ForwardList(SizeType n, AllocatorType const& allocator = AllocatorType{}) : BaseType{allocator}
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (SizeType i = 0; i < n; ++i)
            {
                tail = appendNode(tail);
            }
        }

        ForwardList(SizeType n, ConstReference value, AllocatorType const& allocator = AllocatorType{})
            : BaseType{allocator}
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (SizeType i = 0; i < n; ++i)
            {
                tail = appendNode(tail, value);
            }
        }

        template <typename InIt>
            requires InputIterator<InIt>
        ForwardList(InIt first, InIt last, AllocatorType const& allocator = AllocatorType{}) : BaseType{allocator}
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (; first != last; ++first)
            {
                tail = appendNode(tail, *first);
            }
        }

        ForwardList(std::initializer_list<T> init, AllocatorType const& allocator = AllocatorType{}) : BaseType{allocator}
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (auto const& v : init)
            {
                tail = appendNode(tail, v);
            }
        }

        ForwardList(ThisType const& other)
            : BaseType{AllocTraits::selectOnContainerCopyConstruction(other.getAllocator())}
        {
            copyFrom(other);
        }

        ForwardList(ThisType&& other) noexcept : BaseType{}
        {
            mAllocator          = worse::core::move(other.mAllocator);
            mBeforeBegin.mpNext = other.mBeforeBegin.mpNext;
            mSize               = other.mSize;
            other.resetHead();
        }

        ~ForwardList() noexcept = default;

        ThisType& operator=(ThisType const& other)
        {
            if (this != &other)
            {
                clear();
                if constexpr (AllocTraits::propagateOnContainerCopyAssignment)
                {
                    mAllocator = other.mAllocator;
                }
                copyFrom(other);
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
                    mAllocator          = worse::core::move(other.mAllocator);
                    mBeforeBegin.mpNext = other.mBeforeBegin.mpNext;
                    mSize               = other.mSize;
                    other.resetHead();
                }
                else if (AllocTraits::equal(mAllocator, other.mAllocator))
                {
                    mBeforeBegin.mpNext = other.mBeforeBegin.mpNext;
                    mSize               = other.mSize;
                    other.resetHead();
                }
                else
                {
                    // Non-propagating + non-equal allocators: cannot adopt nodes allocated by
                    // `other`'s allocator. Move each value into freshly-allocated nodes of OUR
                    // allocator (ordered append via a running tail), then let `other` free its.
                    ForwardListNodeBase* tail = beforeBeginPtr();
                    for (ForwardListNodeBase* cur = other.mBeforeBegin.mpNext; cur != nullptr; cur = cur->mpNext)
                    {
                        tail = appendNode(tail, worse::core::move(static_cast<Node*>(cur)->mValue));
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

        WE_NODISCARD AllocatorType getAllocator() const noexcept { return BaseType::getAllocator(); }

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
        WE_NODISCARD SizeType size() const noexcept { return mSize; }

        // --- element access ----------------------------------------------------

        WE_NODISCARD Reference front() noexcept
        {
            WE_ASSERT(!empty());
            return valueOf(mBeforeBegin.mpNext);
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

        // Insert n copies after pos; returns an iterator to the LAST inserted element (or pos
        // when n == 0), matching std::forward_list::insert_after.
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

        // Erase the single element AFTER pos; returns an iterator to the element after the
        // erased one (or end()).
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

        // Erase the open range (first, last) -- the elements strictly between them; returns
        // last.
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

        void clear() noexcept { clearNodes(); }

        // O(n): no tail pointer, so a grow first walks to the end. Shrink walks to the new
        // last node and drops the tail.
        void resize(SizeType n)
        {
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

        void swap(ThisType& other) noexcept(AllocTraits::isAlwaysEqual)
        {
            // Non-circular: only the head pointer + size move. No sentinel reseat (nothing
            // points back at mBeforeBegin), unlike List.
            worse::core::swap(mBeforeBegin.mpNext, other.mBeforeBegin.mpNext);
            worse::core::swap(mSize, other.mSize);
            if constexpr (AllocTraits::propagateOnContainerSwap)
            {
                worse::core::swap(mAllocator, other.mAllocator);
            }
        }

        // --- list algorithms (allocation-free: pure node relinking) ------------

        // Whole-list splice: O(other.size) -- a singly-linked list has no tail pointer, so we
        // walk `other` to find its last node. Element order is preserved; `other` ends empty.
        void spliceAfter(ConstIterator pos, ThisType& other)
        {
            if (this == &other || other.empty())
            {
                return;
            }
            ForwardListNodeBase* const p     = pos.node();
            ForwardListNodeBase* const first = other.mBeforeBegin.mpNext;
            ForwardListNodeBase* tail        = first;
            while (tail->mpNext != nullptr)
            {
                tail = tail->mpNext;
            }
            tail->mpNext = p->mpNext;
            p->mpNext    = first;
            mSize += other.mSize;
            other.resetHead();
        }
        void spliceAfter(ConstIterator pos, ThisType&& other) { spliceAfter(pos, other); }

        // Move the single element AFTER `it` (in `other`) to after `pos`. O(1).
        void spliceAfter(ConstIterator pos, ThisType& other, ConstIterator it) noexcept
        {
            ForwardListNodeBase* const p     = pos.node();
            ForwardListNodeBase* const ip    = it.node();
            ForwardListNodeBase* const moved = ip->mpNext;
            WE_ASSERT(moved != nullptr);
            if (p == moved || moved == p->mpNext)
            {
                return; // already in place
            }
            ip->mpNext    = moved->mpNext; // excise from source
            moved->mpNext = p->mpNext;     // splice after pos
            p->mpNext     = moved;
            if (this != &other)
            {
                ++mSize;
                --other.mSize;
            }
        }
        void spliceAfter(ConstIterator pos, ThisType&& other, ConstIterator it) noexcept { spliceAfter(pos, other, it); }

        // Move the open range (first, last) from `other` to after `pos`. O(range) when crossing
        // lists (counting the range to keep both O(1) size() counters correct -- R26).
        void spliceAfter(ConstIterator pos, ThisType& other, ConstIterator first, ConstIterator last) noexcept
        {
            ForwardListNodeBase* const p  = pos.node();
            ForwardListNodeBase* const f  = first.node();
            ForwardListNodeBase* const l  = last.node();
            ForwardListNodeBase* const rb = f->mpNext; // first moved node
            if (rb == l || rb == nullptr)
            {
                return; // empty range
            }
            // Find the last moved node (the one whose mpNext == l), counting as we go.
            ForwardListNodeBase* tail = rb;
            SizeType n                = 1;
            while (tail->mpNext != l)
            {
                tail = tail->mpNext;
                ++n;
            }
            f->mpNext    = l;         // excise (first, last) from source
            tail->mpNext = p->mpNext; // splice after pos
            p->mpNext    = rb;
            if (this != &other)
            {
                mSize += n;
                other.mSize -= n;
            }
        }
        void spliceAfter(ConstIterator pos, ThisType&& other, ConstIterator first, ConstIterator last) noexcept
        {
            spliceAfter(pos, other, first, last);
        }

        // remove / removeIf: drop every matching element; returns the count removed (R27).
        SizeType remove(ConstReference value)
        {
            SizeType removed          = 0;
            ForwardListNodeBase* prev = beforeBeginPtr();
            ForwardListNodeBase* cur  = mBeforeBegin.mpNext;
            while (cur != nullptr)
            {
                if (valueOf(cur) == value)
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
                if (pred(valueOf(cur)))
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
                if (pred(valueOf(cur), valueOf(nxt)))
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

        void merge(ThisType& other) { merge(other, Less<>{}); }
        void merge(ThisType&& other) { merge(other, Less<>{}); }

        // Stable merge of sorted `other` into this sorted list; `other` ends empty.
        template <typename Compare>
            requires CompareFor<Compare, T>
        void merge(ThisType& other, Compare comp)
        {
            if (this == &other)
            {
                return;
            }
            ForwardListNodeBase* tail = beforeBeginPtr();
            ForwardListNodeBase* a    = mBeforeBegin.mpNext;
            ForwardListNodeBase* b    = other.mBeforeBegin.mpNext;
            while (a != nullptr && b != nullptr)
            {
                if (comp(valueOf(b), valueOf(a)))
                {
                    tail->mpNext = b;
                    tail         = b;
                    b            = b->mpNext;
                }
                else
                {
                    tail->mpNext = a;
                    tail         = a;
                    a            = a->mpNext;
                }
            }
            tail->mpNext = (a != nullptr) ? a : b;
            mSize += other.mSize;
            other.resetHead();
        }
        template <typename Compare>
            requires CompareFor<Compare, T>
        void merge(ThisType&& other, Compare comp)
        {
            merge(other, comp);
        }

        // sort: allocation-free, stable, bottom-up binned merge sort over bare null-terminated
        // chains. Only O(log n) stack pointers; never copies a value or allocates. `mSize`
        // unchanged.
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
                carry->mpNext              = nullptr; // single-node run
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

        // reverse: in-place three-pointer re-thread of the mpNext chain. O(n), O(1) extra.
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
        template <typename... Args>
        ForwardListNodeBase* appendNode(ForwardListNodeBase* tail, Args&&... args)
        {
            Node* node   = createNode(worse::core::forward<Args>(args)...);
            node->mpNext = nullptr;
            tail->mpNext = node;
            ++mSize;
            return node;
        }

        void copyFrom(ThisType const& other)
        {
            ForwardListNodeBase* tail = beforeBeginPtr();
            for (ForwardListNodeBase* cur = other.mBeforeBegin.mpNext; cur != nullptr; cur = cur->mpNext)
            {
                tail = appendNode(tail, static_cast<Node const*>(cur)->mValue);
            }
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
                destroyNode(cur);
                --mSize;
                cur = nxt;
            }
            prev->mpNext = nullptr;
        }

        // Stably merge two sorted bare chains; `dst` elements win ties (kept before `src`).
        // Returns the merged head. Used by sort on stack-local chain heads.
        template <typename Compare>
        static ForwardListNodeBase* mergeChains(ForwardListNodeBase* dst, ForwardListNodeBase* src, Compare& comp)
        {
            ForwardListNodeBase dummy;
            ForwardListNodeBase* tail = &dummy;
            while (dst != nullptr && src != nullptr)
            {
                if (comp(valueOf(src), valueOf(dst)))
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

    export template <typename T, typename Allocator>
    void swap(ForwardList<T, Allocator>& a, ForwardList<T, Allocator>& b) noexcept(noexcept(a.swap(b)))
    {
        a.swap(b);
    }

} // namespace worse::core::container
