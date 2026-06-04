module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.rb_tree;
import worse.core.basic_type;
import worse.core.memory;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;
import worse.core.container.iterator;

/**
 * \file
 * \brief Red-black tree engine: the ordered counterpart to `hash_table`, backing `Set`/`Map`.
 *
 * A balanced binary search tree giving O(log n) insert/erase/find AND ordered in-order
 * iteration with STABLE element addresses (a node never moves once inserted; only the erased
 * node's iterator invalidates). The rebalance algorithms are the canonical SGI/libstdc++
 * ones (battle-tested) operating on a colour-tagged node base.
 *
 * \note GAME-PERF: a red-black tree is the LEAST cache-friendly ordered container (a pointer
 *       chase + a heap node per element). For games prefer `FlatMap`/`FlatSet` (sorted array,
 *       far better cache behaviour and lookup) or `UnorderedMap`/`UnorderedSet` (hash). This
 *       exists for completeness and for the rare case needing ordered iteration WITH stable
 *       references and O(log n) mutation. Allocator-pluggable (pool-ready) like the other
 *       node containers.
 * \note Storage model (libstdc++): an embedded header node `mHeader` whose `mpParent` is the
 *       root, `mpLeft` the leftmost (begin), `mpRight` the rightmost; the root's parent points
 *       back at the header (so move/swap must re-seat it -- see adoptFrom/swap, the same
 *       embedded-sentinel subtlety as `List`). Internal move/forward/swap fully qualified
 *       (ADL). NOT trivially relocatable. Engine is `<Value, Key, KeyOfValue, Compare,
 *       Allocator>`; `KeyOfValue` extracts the key from the stored value (identity for Set,
 *       `.first` for Map) -- mirrors `hash_table`.
 */
namespace worse::core::container
{
    enum class RBColor : u8
    {
        Red   = 0,
        Black = 1,
    };

    // Pointer/colour-only base: the embedded header is a bare RBNodeBase (no Value), and every
    // value node derives from it so all rebalancing operates uniformly on RBNodeBase*.
    struct RBNodeBase
    {
        RBColor mColor       = RBColor::Red;
        RBNodeBase* mpParent = nullptr;
        RBNodeBase* mpLeft   = nullptr;
        RBNodeBase* mpRight  = nullptr;
    };

    template <typename T>
    struct RBNode : RBNodeBase
    {
        T mValue;
    };

    WE_NODISCARD inline RBNodeBase* rbMinimum(RBNodeBase* x) noexcept
    {
        while (x->mpLeft != nullptr)
        {
            x = x->mpLeft;
        }
        return x;
    }
    WE_NODISCARD inline RBNodeBase* rbMaximum(RBNodeBase* x) noexcept
    {
        while (x->mpRight != nullptr)
        {
            x = x->mpRight;
        }
        return x;
    }

    // In-order successor / predecessor (the header trick makes decrement from end() yield the
    // rightmost; increment from the rightmost yields the header == end()).
    inline RBNodeBase* rbIncrement(RBNodeBase* x) noexcept
    {
        if (x->mpRight != nullptr)
        {
            x = x->mpRight;
            while (x->mpLeft != nullptr)
            {
                x = x->mpLeft;
            }
        }
        else
        {
            RBNodeBase* y = x->mpParent;
            while (x == y->mpRight)
            {
                x = y;
                y = y->mpParent;
            }
            if (x->mpRight != y)
            {
                x = y;
            }
        }
        return x;
    }
    inline RBNodeBase* rbDecrement(RBNodeBase* x) noexcept
    {
        if (x->mColor == RBColor::Red && x->mpParent->mpParent == x)
        {
            // x is the header: predecessor of end() is the rightmost.
            x = x->mpRight;
        }
        else if (x->mpLeft != nullptr)
        {
            x = rbMaximum(x->mpLeft);
        }
        else
        {
            RBNodeBase* y = x->mpParent;
            while (x == y->mpLeft)
            {
                x = y;
                y = y->mpParent;
            }
            x = y;
        }
        return x;
    }

    inline void rbRotateLeft(RBNodeBase* x, RBNodeBase*& root) noexcept
    {
        RBNodeBase* y = x->mpRight;
        x->mpRight    = y->mpLeft;
        if (y->mpLeft != nullptr)
        {
            y->mpLeft->mpParent = x;
        }
        y->mpParent = x->mpParent;
        if (x == root)
        {
            root = y;
        }
        else if (x == x->mpParent->mpLeft)
        {
            x->mpParent->mpLeft = y;
        }
        else
        {
            x->mpParent->mpRight = y;
        }
        y->mpLeft   = x;
        x->mpParent = y;
    }
    inline void rbRotateRight(RBNodeBase* x, RBNodeBase*& root) noexcept
    {
        RBNodeBase* y = x->mpLeft;
        x->mpLeft     = y->mpRight;
        if (y->mpRight != nullptr)
        {
            y->mpRight->mpParent = x;
        }
        y->mpParent = x->mpParent;
        if (x == root)
        {
            root = y;
        }
        else if (x == x->mpParent->mpRight)
        {
            x->mpParent->mpRight = y;
        }
        else
        {
            x->mpParent->mpLeft = y;
        }
        y->mpRight  = x;
        x->mpParent = y;
    }

    // Link `z` as a child of `p` (left iff insertLeft), maintaining header leftmost/rightmost,
    // then restore the red-black invariants. Canonical libstdc++ _Rb_tree_insert_and_rebalance.
    inline void rbInsertAndRebalance(bool insertLeft, RBNodeBase* z, RBNodeBase* p, RBNodeBase& header) noexcept
    {
        RBNodeBase*& root = header.mpParent;
        z->mpParent       = p;
        z->mpLeft         = nullptr;
        z->mpRight        = nullptr;
        z->mColor         = RBColor::Red;

        if (insertLeft)
        {
            p->mpLeft = z; // also sets leftmost when p == &header
            if (p == &header)
            {
                header.mpParent = z;
                header.mpRight  = z;
            }
            else if (p == header.mpLeft)
            {
                header.mpLeft = z;
            }
        }
        else
        {
            p->mpRight = z;
            if (p == header.mpRight)
            {
                header.mpRight = z;
            }
        }

        while (z != root && z->mpParent->mColor == RBColor::Red)
        {
            RBNodeBase* const gp = z->mpParent->mpParent;
            if (z->mpParent == gp->mpLeft)
            {
                RBNodeBase* const uncle = gp->mpRight;
                if (uncle != nullptr && uncle->mColor == RBColor::Red)
                {
                    z->mpParent->mColor = RBColor::Black;
                    uncle->mColor       = RBColor::Black;
                    gp->mColor          = RBColor::Red;
                    z                   = gp;
                }
                else
                {
                    if (z == z->mpParent->mpRight)
                    {
                        z = z->mpParent;
                        rbRotateLeft(z, root);
                    }
                    z->mpParent->mColor           = RBColor::Black;
                    z->mpParent->mpParent->mColor = RBColor::Red;
                    rbRotateRight(z->mpParent->mpParent, root);
                }
            }
            else
            {
                RBNodeBase* const uncle = gp->mpLeft;
                if (uncle != nullptr && uncle->mColor == RBColor::Red)
                {
                    z->mpParent->mColor = RBColor::Black;
                    uncle->mColor       = RBColor::Black;
                    gp->mColor          = RBColor::Red;
                    z                   = gp;
                }
                else
                {
                    if (z == z->mpParent->mpLeft)
                    {
                        z = z->mpParent;
                        rbRotateRight(z, root);
                    }
                    z->mpParent->mColor           = RBColor::Black;
                    z->mpParent->mpParent->mColor = RBColor::Red;
                    rbRotateLeft(z->mpParent->mpParent, root);
                }
            }
        }
        root->mColor = RBColor::Black;
    }

    // Unlink `z` and restore the invariants; updates header root/leftmost/rightmost. Returns
    // the node actually removed from the tree (== `z`), which the caller destroys. Canonical
    // libstdc++ _Rb_tree_rebalance_for_erase.
    inline RBNodeBase* rbRebalanceForErase(RBNodeBase* z, RBNodeBase& header) noexcept
    {
        RBNodeBase*& root      = header.mpParent;
        RBNodeBase*& leftmost  = header.mpLeft;
        RBNodeBase*& rightmost = header.mpRight;
        RBNodeBase* y          = z;
        RBNodeBase* x          = nullptr;
        RBNodeBase* xParent    = nullptr;

        if (y->mpLeft == nullptr)
        {
            x = y->mpRight;
        }
        else if (y->mpRight == nullptr)
        {
            x = y->mpLeft;
        }
        else
        {
            y = y->mpRight;
            while (y->mpLeft != nullptr)
            {
                y = y->mpLeft;
            }
            x = y->mpRight;
        }

        if (y != z)
        {
            // Relink y (z's successor) into z's position.
            z->mpLeft->mpParent = y;
            y->mpLeft           = z->mpLeft;
            if (y != z->mpRight)
            {
                xParent = y->mpParent;
                if (x != nullptr)
                {
                    x->mpParent = y->mpParent;
                }
                y->mpParent->mpLeft  = x;
                y->mpRight           = z->mpRight;
                z->mpRight->mpParent = y;
            }
            else
            {
                xParent = y;
            }
            if (root == z)
            {
                root = y;
            }
            else if (z->mpParent->mpLeft == z)
            {
                z->mpParent->mpLeft = y;
            }
            else
            {
                z->mpParent->mpRight = y;
            }
            y->mpParent = z->mpParent;
            worse::core::swap(y->mColor, z->mColor);
            y = z; // y now points at the node to delete
        }
        else
        {
            xParent = y->mpParent;
            if (x != nullptr)
            {
                x->mpParent = y->mpParent;
            }
            if (root == z)
            {
                root = x;
            }
            else if (z->mpParent->mpLeft == z)
            {
                z->mpParent->mpLeft = x;
            }
            else
            {
                z->mpParent->mpRight = x;
            }
            if (leftmost == z)
            {
                leftmost = (z->mpRight == nullptr) ? z->mpParent : rbMinimum(x);
            }
            if (rightmost == z)
            {
                rightmost = (z->mpLeft == nullptr) ? z->mpParent : rbMaximum(x);
            }
        }

        if (y->mColor != RBColor::Red)
        {
            while (x != root && (x == nullptr || x->mColor == RBColor::Black))
            {
                if (x == xParent->mpLeft)
                {
                    RBNodeBase* w = xParent->mpRight;
                    if (w->mColor == RBColor::Red)
                    {
                        w->mColor       = RBColor::Black;
                        xParent->mColor = RBColor::Red;
                        rbRotateLeft(xParent, root);
                        w = xParent->mpRight;
                    }
                    if ((w->mpLeft == nullptr || w->mpLeft->mColor == RBColor::Black) &&
                        (w->mpRight == nullptr || w->mpRight->mColor == RBColor::Black))
                    {
                        w->mColor = RBColor::Red;
                        x         = xParent;
                        xParent   = xParent->mpParent;
                    }
                    else
                    {
                        if (w->mpRight == nullptr || w->mpRight->mColor == RBColor::Black)
                        {
                            w->mpLeft->mColor = RBColor::Black;
                            w->mColor         = RBColor::Red;
                            rbRotateRight(w, root);
                            w = xParent->mpRight;
                        }
                        w->mColor       = xParent->mColor;
                        xParent->mColor = RBColor::Black;
                        if (w->mpRight != nullptr)
                        {
                            w->mpRight->mColor = RBColor::Black;
                        }
                        rbRotateLeft(xParent, root);
                        break;
                    }
                }
                else
                {
                    RBNodeBase* w = xParent->mpLeft;
                    if (w->mColor == RBColor::Red)
                    {
                        w->mColor       = RBColor::Black;
                        xParent->mColor = RBColor::Red;
                        rbRotateRight(xParent, root);
                        w = xParent->mpLeft;
                    }
                    if ((w->mpRight == nullptr || w->mpRight->mColor == RBColor::Black) &&
                        (w->mpLeft == nullptr || w->mpLeft->mColor == RBColor::Black))
                    {
                        w->mColor = RBColor::Red;
                        x         = xParent;
                        xParent   = xParent->mpParent;
                    }
                    else
                    {
                        if (w->mpLeft == nullptr || w->mpLeft->mColor == RBColor::Black)
                        {
                            w->mpRight->mColor = RBColor::Black;
                            w->mColor          = RBColor::Red;
                            rbRotateLeft(w, root);
                            w = xParent->mpLeft;
                        }
                        w->mColor       = xParent->mColor;
                        xParent->mColor = RBColor::Black;
                        if (w->mpLeft != nullptr)
                        {
                            w->mpLeft->mColor = RBColor::Black;
                        }
                        rbRotateRight(xParent, root);
                        break;
                    }
                }
            }
            if (x != nullptr)
            {
                x->mColor = RBColor::Black;
            }
        }
        return y;
    }

    /**
     * \brief Bidirectional iterator over the tree's in-order traversal.
     * \tparam T the element value type.
     * \tparam ValueT the dereference type, carrying const-ness (`Value` or `Value const`).
     */
    export template <typename T, typename ValueT>
    class RBTreeIterator
    {
    public:
        using IteratorCategory = BidirectionalIteratorTag;
        using ValueType        = T;
        using DifferenceType   = isize;
        using Pointer          = ValueT*;
        using Reference        = ValueT&;

        constexpr RBTreeIterator() = default;
        constexpr explicit RBTreeIterator(RBNodeBase* node) noexcept : mpNode(node) {}

        template <typename U>
            requires(IsSame<ValueT, U const>)
        constexpr RBTreeIterator(RBTreeIterator<T, U> const& other) noexcept : mpNode(other.node())
        {
        }

        WE_NODISCARD Reference operator*() const noexcept { return static_cast<RBNode<T>*>(mpNode)->mValue; }
        WE_NODISCARD Pointer operator->() const noexcept { return &static_cast<RBNode<T>*>(mpNode)->mValue; }

        RBTreeIterator& operator++() noexcept
        {
            mpNode = rbIncrement(mpNode);
            return *this;
        }
        RBTreeIterator operator++(int) noexcept
        {
            RBTreeIterator tmp = *this;
            mpNode             = rbIncrement(mpNode);
            return tmp;
        }
        RBTreeIterator& operator--() noexcept
        {
            mpNode = rbDecrement(mpNode);
            return *this;
        }
        RBTreeIterator operator--(int) noexcept
        {
            RBTreeIterator tmp = *this;
            mpNode             = rbDecrement(mpNode);
            return tmp;
        }

        WE_NODISCARD constexpr RBNodeBase* node() const noexcept { return mpNode; }

        WE_NODISCARD friend constexpr bool operator==(RBTreeIterator const& a, RBTreeIterator const& b) noexcept
        {
            return a.mpNode == b.mpNode;
        }
        WE_NODISCARD friend constexpr bool operator!=(RBTreeIterator const& a, RBTreeIterator const& b) noexcept
        {
            return a.mpNode != b.mpNode;
        }

    private:
        RBNodeBase* mpNode = nullptr;
    };

    /**
     * \brief Storage + RAII half of the tree: owns the allocator, embedded header, and size.
     *
     * Provides the node allocate/construct/free + recursive clear primitives.
     * \note The BASE destructor frees ALL nodes (R25).
     */
    template <typename T, typename Allocator>
    class RBTreeBase
    {
    public:
        using AllocTraits = AllocatorTraits<Allocator>;
        using Node        = RBNode<T>;
        using SizeType    = usize;

        RBTreeBase() noexcept(IsNothrowDefaultConstructible<Allocator>) { resetHeader(); }
        explicit RBTreeBase(Allocator const& allocator) noexcept : mAllocator{allocator} { resetHeader(); }
        ~RBTreeBase() noexcept { destroyFrom(mHeader.mpParent); }

        WE_NODISCARD Allocator& getAllocator() noexcept { return mAllocator; }
        WE_NODISCARD Allocator const& getAllocator() const noexcept { return mAllocator; }

    protected:
        RBNodeBase mHeader{};
        SizeType mSize = 0;
        WE_NO_UNIQUE_ADDRESS Allocator mAllocator{};

        WE_NODISCARD RBNodeBase* headerPtr() const noexcept { return const_cast<RBNodeBase*>(&mHeader); }
        WE_NODISCARD RBNodeBase* root() const noexcept { return mHeader.mpParent; }

        void resetHeader() noexcept
        {
            mHeader.mColor   = RBColor::Red; // header is red; root is black -> distinguishes them
            mHeader.mpParent = nullptr;
            mHeader.mpLeft   = &mHeader;
            mHeader.mpRight  = &mHeader;
            mSize            = 0;
        }

        template <typename... Args>
        WE_NODISCARD Node* createNode(Args&&... args)
        {
            void* raw = AllocTraits::allocate(mAllocator, sizeof(Node), alignof(Node));
            if (raw == nullptr)
            {
                memory::handleAllocationFailure(sizeof(Node), alignof(Node));
            }
            Node* node = static_cast<Node*>(raw);
            AllocTraits::construct(mAllocator, &node->mValue, worse::core::forward<Args>(args)...);
            return node;
        }

        void destroyNode(RBNodeBase* n) noexcept
        {
            Node* node = static_cast<Node*>(n);
            AllocTraits::destroy(mAllocator, &node->mValue);
            AllocTraits::deallocate(mAllocator, node, sizeof(Node), alignof(Node));
        }

        // Recursively destroy a subtree (post-order). Used by clear + dtor.
        void destroyFrom(RBNodeBase* x) noexcept
        {
            while (x != nullptr)
            {
                destroyFrom(x->mpRight);
                RBNodeBase* const left = x->mpLeft;
                destroyNode(x);
                x = left;
            }
        }
    };

    /**
     * \brief Ordered unique associative engine over a red-black tree; backs `Set` and `Map`.
     * \ingroup ctr_ordered
     * \tparam Value the stored value type (the key itself for Set, `Pair<Key, T>` for Map).
     * \tparam Key the key type used for ordering and lookup.
     * \tparam KeyOfValue functor extracting the `Key` from a `Value` (identity for Set,
     *         `.first` for Map) -- mirrors `hash_table`.
     * \tparam Compare strict-weak-ordering key comparator.
     * \tparam Allocator node allocator.
     * \note O(log n) insert/erase/find, sorted in-order iteration, stable element addresses.
     */
    export template <
        typename Value,
        typename Key,
        typename KeyOfValue,
        typename Compare   = Less<Key>,
        typename Allocator = WE_DEFAULT_ALLOCATOR>
    class RBTree : public RBTreeBase<Value, Allocator>
    {
        using BaseType = RBTreeBase<Value, Allocator>;
        using ThisType = RBTree<Value, Key, KeyOfValue, Compare, Allocator>;

        using BaseType::createNode;
        using BaseType::destroyFrom;
        using BaseType::destroyNode;
        using BaseType::headerPtr;
        using BaseType::mAllocator;
        using BaseType::mHeader;
        using BaseType::mSize;
        using BaseType::resetHeader;
        using BaseType::root;

        using AllocTraits = AllocatorTraits<Allocator>;
        using Node        = typename BaseType::Node;

    public:
        using ValueType      = Value;
        using KeyType        = Key;
        using SizeType       = typename BaseType::SizeType;
        using DifferenceType = isize;
        using AllocatorType  = Allocator;
        using Reference      = Value&;
        using ConstReference = Value const&;

        using Iterator         = RBTreeIterator<Value, Value>;
        using ConstIterator    = RBTreeIterator<Value, Value const>;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        // --- construction / destruction ---------------------------------------

        RBTree() noexcept(IsNothrowDefaultConstructible<AllocatorType>) : BaseType{} {}
        explicit RBTree(Compare const& comp, AllocatorType const& allocator = AllocatorType{})
            : BaseType{allocator}, mCompare{comp}
        {
        }
        explicit RBTree(AllocatorType const& allocator) noexcept : BaseType{allocator} {}

        RBTree(ThisType const& other)
            : BaseType{AllocTraits::selectOnContainerCopyConstruction(other.getAllocator())}, mCompare{other.mCompare}
        {
            if (other.root() != nullptr)
            {
                mHeader.mpParent = cloneSubtree(other.root(), headerPtr());
                mHeader.mpLeft   = rbMinimum(mHeader.mpParent);
                mHeader.mpRight  = rbMaximum(mHeader.mpParent);
                mSize            = other.mSize;
            }
        }

        RBTree(ThisType&& other) noexcept : BaseType{}, mCompare{worse::core::move(other.mCompare)}
        {
            mAllocator = worse::core::move(other.mAllocator);
            adoptFrom(other);
        }

        ~RBTree() noexcept = default;

        ThisType& operator=(ThisType const& other)
        {
            if (this != &other)
            {
                clear();
                if constexpr (AllocTraits::propagateOnContainerCopyAssignment)
                {
                    mAllocator = other.mAllocator;
                }
                mCompare = other.mCompare;
                if (other.root() != nullptr)
                {
                    mHeader.mpParent = cloneSubtree(other.root(), headerPtr());
                    mHeader.mpLeft   = rbMinimum(mHeader.mpParent);
                    mHeader.mpRight  = rbMaximum(mHeader.mpParent);
                    mSize            = other.mSize;
                }
            }
            return *this;
        }

        ThisType& operator=(ThisType&& other) noexcept(AllocTraits::isAlwaysEqual)
        {
            if (this != &other)
            {
                clear();
                mCompare = worse::core::move(other.mCompare);
                if constexpr (AllocTraits::propagateOnContainerMoveAssignment)
                {
                    mAllocator = worse::core::move(other.mAllocator);
                    adoptFrom(other);
                }
                else if (AllocTraits::equal(mAllocator, other.mAllocator))
                {
                    adoptFrom(other);
                }
                else
                {
                    for (ConstIterator it = other.begin(); it != other.end(); ++it)
                    {
                        insertUniqueImpl(*it);
                    }
                    other.clear();
                }
            }
            return *this;
        }

        // --- iterators / capacity ---------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return Iterator(mHeader.mpLeft); }
        WE_NODISCARD ConstIterator begin() const noexcept { return ConstIterator(mHeader.mpLeft); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return ConstIterator(mHeader.mpLeft); }
        WE_NODISCARD Iterator end() noexcept { return Iterator(headerPtr()); }
        WE_NODISCARD ConstIterator end() const noexcept { return ConstIterator(headerPtr()); }
        WE_NODISCARD ConstIterator cend() const noexcept { return ConstIterator(headerPtr()); }

        WE_NODISCARD ReverseIter rbegin() noexcept { return ReverseIter(end()); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(cend()); }
        WE_NODISCARD ReverseIter rend() noexcept { return ReverseIter(begin()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(cbegin()); }

        WE_NODISCARD bool empty() const noexcept { return mSize == 0; }
        WE_NODISCARD SizeType size() const noexcept { return mSize; }

        WE_NODISCARD AllocatorType getAllocator() const noexcept { return BaseType::getAllocator(); }

        // --- lookup ------------------------------------------------------------

        /**
         * \brief Find the element whose key is equivalent to \p key.
         * \tparam K query type; with a TRANSPARENT comparator (e.g. `Less<>`) heterogeneous
         *         lookup builds NO temporary `Key` (R45 / R21 tree analog). With a homogeneous
         *         Compare only K == Key (or a K implicitly convertible to Key) compiles.
         * \param key lookup key.
         * \return iterator to the element, or end() if absent.
         * \note O(log n).
         */
        template <typename K>
        WE_NODISCARD Iterator find(K const& key) noexcept { return Iterator(findNode(key)); }
        template <typename K>
        WE_NODISCARD ConstIterator find(K const& key) const noexcept { return ConstIterator(findNode(key)); }
        /**
         * \brief Test whether an element with key equivalent to \p key exists.
         * \tparam K query type; transparent (heterogeneous) lookup, see find().
         * \return true iff present. O(log n).
         */
        template <typename K>
        WE_NODISCARD bool contains(K const& key) const noexcept { return findNode(key) != headerPtr(); }
        /**
         * \brief Count elements equivalent to \p key (0 or 1 for a unique tree).
         * \tparam K query type; transparent (heterogeneous) lookup, see find().
         * \note O(log n).
         */
        template <typename K>
        WE_NODISCARD SizeType count(K const& key) const noexcept
        {
            return findNode(key) != headerPtr() ? SizeType{1} : SizeType{0};
        }

        /**
         * \brief First element whose key is not less than \p key.
         * \tparam K query type; transparent (heterogeneous) lookup, see find().
         * \return iterator to the lower bound, or end() if all keys order before \p key. O(log n).
         */
        template <typename K>
        WE_NODISCARD Iterator lowerBound(K const& key) noexcept { return Iterator(lowerBoundNode(key)); }
        template <typename K>
        WE_NODISCARD ConstIterator lowerBound(K const& key) const noexcept { return ConstIterator(lowerBoundNode(key)); }
        /**
         * \brief First element whose key is greater than \p key.
         * \tparam K query type; transparent (heterogeneous) lookup, see find().
         * \return iterator to the upper bound, or end() if none. O(log n).
         */
        template <typename K>
        WE_NODISCARD Iterator upperBound(K const& key) noexcept { return Iterator(upperBoundNode(key)); }
        template <typename K>
        WE_NODISCARD ConstIterator upperBound(K const& key) const noexcept { return ConstIterator(upperBoundNode(key)); }

        /**
         * \brief Range of elements equivalent to \p key, i.e. [lowerBound, upperBound).
         * \tparam K query type; transparent (heterogeneous) lookup, see find().
         * \return a pair {lowerBound(key), upperBound(key)}; empty (both equal) if absent.
         */
        template <typename K>
        WE_NODISCARD Pair<Iterator, Iterator> equalRange(K const& key) noexcept
        {
            return {Iterator(lowerBoundNode(key)), Iterator(upperBoundNode(key))};
        }

        // --- modifiers ---------------------------------------------------------

        /**
         * \brief Insert \p value if its key is absent; no-op if an equivalent key already exists.
         * \param value the value to insert (copied or moved).
         * \return a pair {iterator-to-element, inserted?}; iterator refers to the existing
         *         element when not inserted. O(log n).
         */
        Pair<Iterator, bool> insertUnique(ConstReference value) { return insertUniqueImpl(value); }
        Pair<Iterator, bool> insertUnique(Value&& value) { return insertUniqueImpl(worse::core::move(value)); }

        /**
         * \brief Construct an element in place and insert it if its key is absent.
         * \tparam Args constructor argument types for `Value`.
         * \return a pair {iterator-to-element, inserted?}.
         * \note The node is built first (args may be arbitrary), then placed, or discarded on a
         *       duplicate. O(log n).
         */
        template <typename... Args>
        Pair<Iterator, bool> emplaceUnique(Args&&... args)
        {
            // Build the node first (args may be arbitrary), then place or discard on duplicate.
            Node* z         = createNode(worse::core::forward<Args>(args)...);
            Key const& kref = KeyOfValue{}(z->mValue);
            RBNodeBase* x   = root();
            RBNodeBase* y   = headerPtr();
            bool comp       = true;
            while (x != nullptr)
            {
                y    = x;
                comp = mCompare(kref, keyOf(x));
                x    = comp ? x->mpLeft : x->mpRight;
            }
            Iterator j(y);
            if (comp)
            {
                if (j == begin())
                {
                    rbInsertAndRebalance(true, z, y, mHeader);
                    ++mSize;
                    return {Iterator(z), true};
                }
                --j;
            }
            if (mCompare(keyOf(j.node()), kref))
            {
                bool const insertLeft = (y == headerPtr()) || mCompare(kref, keyOf(y));
                rbInsertAndRebalance(insertLeft, z, y, mHeader);
                ++mSize;
                return {Iterator(z), true};
            }
            destroyNode(z); // duplicate -- discard the freshly built node
            return {j, false};
        }

        /**
         * \brief Single-descent find-or-insert: locate \p key, inserting via \p factory if absent.
         *
         * If present returns {it, false}; else constructs the value via factory() at the located
         * slot and returns {it, true}.
         * \tparam K query type for the descent.
         * \tparam Factory nullary callable producing the `Value` to insert.
         * \param key the key to locate.
         * \param factory invoked ONLY on the insert branch.
         * \pre factory() must return a Value whose key compares equal to \p key.
         * \note ONE O(log n) walk instead of find()+insert()'s two; backs
         *       `Map::operator[]`/`tryEmplace`/`insertOrAssign` (R45). Mirrors insertUniqueImpl's
         *       descent but keyed on `key`.
         */
        template <typename K, typename Factory>
        Pair<Iterator, bool> findOrInsertWith(K const& key, Factory&& factory)
        {
            RBNodeBase* x = root();
            RBNodeBase* y = headerPtr();
            bool comp     = true;
            while (x != nullptr)
            {
                y    = x;
                comp = mCompare(key, keyOf(x));
                x    = comp ? x->mpLeft : x->mpRight;
            }
            Iterator j(y);
            if (comp)
            {
                if (j == begin())
                {
                    return {insertAt(true, y, factory()), true};
                }
                --j;
            }
            if (mCompare(keyOf(j.node()), key))
            {
                bool const insertLeft = (y == headerPtr()) || mCompare(key, keyOf(y));
                return {insertAt(insertLeft, y, factory()), true};
            }
            return {j, false};
        }

        /**
         * \brief Insert each element of the range [\p first, \p last), skipping duplicate keys.
         * \tparam InIt input iterator type.
         */
        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            for (; first != last; ++first)
            {
                insertUniqueImpl(*first);
            }
        }

        /**
         * \brief Erase the element with key \p key, if any.
         * \param key the key to remove.
         * \return the number removed (0 or 1 for a unique tree). O(log n).
         * \note NOT templated on the query type: a transparent erase(K) would hijack
         *       erase(ConstIterator) for iterator args (a class-type iterator needs a conversion,
         *       but K=Iterator is an exact match), so std omits it pre-C++23 and so do we.
         *       Transparent find/count/... cover heterogeneous lookup.
         */
        SizeType erase(Key const& key) noexcept
        {
            RBNodeBase* const n = findNode(key);
            if (n == headerPtr())
            {
                return 0;
            }
            eraseNode(n);
            return 1;
        }

        /**
         * \brief Erase the element at \p pos.
         * \param pos iterator to a valid element (not end()).
         * \return iterator to the element following the erased one. O(1) amortized.
         */
        Iterator erase(ConstIterator pos) noexcept
        {
            RBNodeBase* const n    = pos.node();
            RBNodeBase* const next = rbIncrement(n);
            eraseNode(n);
            return Iterator(next);
        }

        /**
         * \brief Erase the range [\p first, \p last).
         * \return iterator equal to \p last. Clears whole-tree erases in one shot.
         */
        Iterator erase(ConstIterator first, ConstIterator last) noexcept
        {
            if (first == begin() && last == end())
            {
                clear();
                return end();
            }
            while (first != last)
            {
                first = erase(first);
            }
            return Iterator(last.node());
        }

        /**
         * \brief Remove all elements and reset to empty.
         * \note Destroys every node; iterators are invalidated.
         */
        void clear() noexcept
        {
            destroyFrom(root());
            resetHeader();
        }

        void swap(ThisType& other) noexcept(AllocTraits::isAlwaysEqual)
        {
            if (this == &other)
            {
                return;
            }
            swapHeaders(other);
            worse::core::swap(mSize, other.mSize);
            worse::core::swap(mCompare, other.mCompare);
            if constexpr (AllocTraits::propagateOnContainerSwap)
            {
                worse::core::swap(mAllocator, other.mAllocator);
            }
        }

        // --- test hook ---------------------------------------------------------
        /**
         * \brief Verify the red-black invariants + BST order + cached leftmost/rightmost.
         * \return the black-height on success, or -1 on any violation.
         * \note Compiled in all builds; cheap, used by the stress test.
         */
        WE_NODISCARD isize checkInvariant() const noexcept
        {
            if (root() == nullptr)
            {
                return (mSize == 0 && mHeader.mpLeft == headerPtr() && mHeader.mpRight == headerPtr()) ? 0 : -1;
            }
            if (root()->mColor != RBColor::Black || root()->mpParent != headerPtr())
            {
                return -1;
            }
            if (mHeader.mpLeft != rbMinimum(root()) || mHeader.mpRight != rbMaximum(root()))
            {
                return -1;
            }
            return checkNode(root());
        }

    private:
        WE_NO_UNIQUE_ADDRESS Compare mCompare{};

        WE_NODISCARD static Key const& keyOf(RBNodeBase* n) noexcept
        {
            return KeyOfValue{}(static_cast<Node*>(n)->mValue);
        }

        template <typename K>
        WE_NODISCARD RBNodeBase* lowerBoundNode(K const& key) const noexcept
        {
            RBNodeBase* x = root();
            RBNodeBase* y = headerPtr(); // last node not less than key
            while (x != nullptr)
            {
                if (!mCompare(keyOf(x), key))
                {
                    y = x;
                    x = x->mpLeft;
                }
                else
                {
                    x = x->mpRight;
                }
            }
            return y;
        }

        template <typename K>
        WE_NODISCARD RBNodeBase* upperBoundNode(K const& key) const noexcept
        {
            RBNodeBase* x = root();
            RBNodeBase* y = headerPtr();
            while (x != nullptr)
            {
                if (mCompare(key, keyOf(x)))
                {
                    y = x;
                    x = x->mpLeft;
                }
                else
                {
                    x = x->mpRight;
                }
            }
            return y;
        }

        template <typename K>
        WE_NODISCARD RBNodeBase* findNode(K const& key) const noexcept
        {
            RBNodeBase* const j = lowerBoundNode(key);
            // j == end() (header) or key < *j  => not found.
            return (j == headerPtr() || mCompare(key, keyOf(j))) ? headerPtr() : j;
        }

        template <typename V>
        Pair<Iterator, bool> insertUniqueImpl(V&& value)
        {
            RBNodeBase* x = root();
            RBNodeBase* y = headerPtr();
            bool comp     = true;
            while (x != nullptr)
            {
                y    = x;
                comp = mCompare(KeyOfValue{}(value), keyOf(x));
                x    = comp ? x->mpLeft : x->mpRight;
            }
            Iterator j(y);
            if (comp)
            {
                if (j == begin())
                {
                    return {insertAt(true, y, worse::core::forward<V>(value)), true};
                }
                --j;
            }
            if (mCompare(keyOf(j.node()), KeyOfValue{}(value)))
            {
                bool const insertLeft = (y == headerPtr()) || mCompare(KeyOfValue{}(value), keyOf(y));
                return {insertAt(insertLeft, y, worse::core::forward<V>(value)), true};
            }
            return {j, false};
        }

        template <typename V>
        Iterator insertAt(bool insertLeft, RBNodeBase* p, V&& value)
        {
            Node* z = createNode(worse::core::forward<V>(value));
            rbInsertAndRebalance(insertLeft, z, p, mHeader);
            ++mSize;
            return Iterator(z);
        }

        void eraseNode(RBNodeBase* n) noexcept
        {
            RBNodeBase* const removed = rbRebalanceForErase(n, mHeader);
            destroyNode(removed);
            --mSize;
        }

        // Deep-copy a subtree, preserving structure + colours; returns the new subtree root.
        RBNodeBase* cloneSubtree(RBNodeBase* src, RBNodeBase* parent)
        {
            Node* n     = createNode(static_cast<Node*>(src)->mValue);
            n->mColor   = src->mColor;
            n->mpParent = parent;
            n->mpLeft   = src->mpLeft != nullptr ? cloneSubtree(src->mpLeft, n) : nullptr;
            n->mpRight  = src->mpRight != nullptr ? cloneSubtree(src->mpRight, n) : nullptr;
            return n;
        }

        // Move other's tree into this (assumed-empty) tree, re-seating the root's parent onto
        // THIS header (the embedded-sentinel subtlety -- cf. List); empties other.
        void adoptFrom(ThisType& other) noexcept
        {
            if (other.root() == nullptr)
            {
                return;
            }
            mHeader.mpParent           = other.mHeader.mpParent;
            mHeader.mpLeft             = other.mHeader.mpLeft;
            mHeader.mpRight            = other.mHeader.mpRight;
            mHeader.mpParent->mpParent = headerPtr();
            mSize                      = other.mSize;
            other.resetHeader();
        }

        void swapHeaders(ThisType& other) noexcept
        {
            // Swap the header link fields, then re-seat each root's parent onto its new header.
            worse::core::swap(mHeader.mpParent, other.mHeader.mpParent);
            worse::core::swap(mHeader.mpLeft, other.mHeader.mpLeft);
            worse::core::swap(mHeader.mpRight, other.mHeader.mpRight);
            if (mHeader.mpParent != nullptr)
            {
                mHeader.mpParent->mpParent = headerPtr();
            }
            else
            {
                mHeader.mpLeft = mHeader.mpRight = headerPtr();
            }
            if (other.mHeader.mpParent != nullptr)
            {
                other.mHeader.mpParent->mpParent = other.headerPtr();
            }
            else
            {
                other.mHeader.mpLeft = other.mHeader.mpRight = other.headerPtr();
            }
        }

        // Recursive invariant check: returns black-height of the subtree, or -1 on violation.
        WE_NODISCARD isize checkNode(RBNodeBase* x) const noexcept
        {
            if (x == nullptr)
            {
                return 1; // null leaves are black
            }
            RBNodeBase* const l = x->mpLeft;
            RBNodeBase* const r = x->mpRight;
            if (x->mColor == RBColor::Red)
            {
                // red node must not have a red child
                if ((l != nullptr && l->mColor == RBColor::Red) || (r != nullptr && r->mColor == RBColor::Red))
                {
                    return -1;
                }
            }
            // BST order + parent links
            if (l != nullptr && (l->mpParent != x || mCompare(keyOf(x), keyOf(l))))
            {
                return -1;
            }
            if (r != nullptr && (r->mpParent != x || mCompare(keyOf(r), keyOf(x))))
            {
                return -1;
            }
            isize const lh = checkNode(l);
            isize const rh = checkNode(r);
            if (lh == -1 || rh == -1 || lh != rh)
            {
                return -1;
            }
            return lh + (x->mColor == RBColor::Black ? 1 : 0);
        }
    };

    export template <typename Value, typename Key, typename KeyOfValue, typename Compare, typename Allocator>
    void swap(
        RBTree<Value, Key, KeyOfValue, Compare, Allocator>& a,
        RBTree<Value, Key, KeyOfValue, Compare, Allocator>& b) noexcept(noexcept(a.swap(b)))
    {
        a.swap(b);
    }
} // namespace worse::core::container
