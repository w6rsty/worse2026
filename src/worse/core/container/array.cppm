module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <type_traits>

export module worse.core.container.array;
import worse.core.basic_type;
import worse.core.memory;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;

namespace worse::core::container
{
    template <typename T, typename Allocator>
    class ArrayBase
    {
    public:
        using ValueType      = T;
        using AllocatorType  = Allocator;
        using AllocTraits    = AllocatorTraits<Allocator>;
        using SizeType       = usize;
        using DifferenceType = isize;

        static constexpr SizeType const NPos         = static_cast<usize>(-1);
        static constexpr SizeType const kMaxSize     = static_cast<usize>(-2);
        static constexpr SizeType const kMaxElements = kMaxSize / sizeof(ValueType);

    public:
        ArrayBase() noexcept = default;

        explicit ArrayBase(AllocatorType const& allocator) noexcept
            : mAllocator{allocator}
        {
        }

        ArrayBase(SizeType n, AllocatorType const& allocator = AllocatorType{})
            : mAllocator{allocator}
        {
            if (n > 0)
            {
                mpBegin    = doAllocate(n);
                mpEnd      = mpBegin;
                mpCapacity = mpBegin + n;
            }
        }

        ~ArrayBase() noexcept
        {
            if (mpBegin != nullptr)
            {
                doFree(mpBegin, capacity());
            }
        }

        WE_NODISCARD AllocatorType& getAllocator() noexcept { return mAllocator; }
        WE_NODISCARD AllocatorType const& getAllocator() const noexcept { return mAllocator; }
        void setAllocator(AllocatorType const& allocator) noexcept { mAllocator = allocator; }

    protected:
        T* mpBegin    = nullptr;
        T* mpEnd      = nullptr;
        T* mpCapacity = nullptr;
        WE_NO_UNIQUE_ADDRESS AllocatorType mAllocator{};

        WE_NODISCARD SizeType capacity() const noexcept
        {
            return static_cast<SizeType>(mpCapacity - mpBegin);
        }

        WE_NODISCARD constexpr T* doAllocate(SizeType n) noexcept
        {
            if (n == 0)
            {
                return nullptr;
            }
            WE_ASSERT_MSG(n <= kMaxElements, "ArrayBase allocation size overflow");

            void* p = AllocTraits::allocate(mAllocator, n * sizeof(T), alignof(T));
            if (p == nullptr)
            {
                memory::handleAllocationFailure(n * sizeof(T), alignof(T));
            }
            return static_cast<T*>(p);
        }

        void doFree(T* p, SizeType n) noexcept
        {
            if (p != nullptr)
            {
                AllocTraits::deallocate(mAllocator, p, n * sizeof(T), alignof(T));
            }
        }

        WE_NODISCARD SizeType getNewCapacity(SizeType currentCapacity) const noexcept
        {
            return currentCapacity <= 1 ? 2 : currentCapacity * 2;
        }
    };

    export template <typename T, typename Allocator = WE_DEFAULT_ALLOCATOR>
    class Array : public ArrayBase<T, Allocator>
    {
        using BaseType = ArrayBase<T, Allocator>;
        using ThisType = Array<T, Allocator>;

        using BaseType::doAllocate;
        using BaseType::doFree;
        using BaseType::getNewCapacity;
        using BaseType::mpBegin;
        using BaseType::mpEnd;
        using BaseType::mpCapacity;
        using BaseType::mAllocator;

    public:
        using ValueType      = T;
        using SizeType       = typename BaseType::SizeType;
        using DifferenceType = typename BaseType::DifferenceType;
        using AllocatorType  = typename BaseType::AllocatorType;

        using Pointer        = T*;
        using ConstPointer   = T const*;
        using Reference      = T&;
        using ConstReference = T const&;

        using Iterator      = T*;
        using ConstIterator = T const*;

        static_assert(!std::is_const_v<ValueType>, "Array<T>::ValueType must be non-const");
        static_assert(!std::is_volatile_v<ValueType>, "Array<T>::ValueType must be non-volatile");

        Array() noexcept(std::is_nothrow_default_constructible_v<WE_DEFAULT_ALLOCATOR>)
            : BaseType{}
        {
        }

        explicit Array(AllocatorType const& allocator) noexcept
            : BaseType{allocator}
        {
        }

        Array(SizeType n, AllocatorType const& allocator = WE_DEFAULT_ALLOCATOR{})
            : BaseType{n, allocator}
        {
        }
    };

} // namespace worse::core::container
